#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include "gpu_ringbuffer.h"
#include "gpu_vulkandriver.h"
#include "latte/latte_formats.h"
#include "latte/latte_constants.h"
#include "spirv/spirv_translate.h"
#include "pm4_processor.h"
#include "vk_mem_alloc.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <gsl/gsl>
#include <list>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// Evaluate f and if result is not a success throw proper vk exception.
#define CHECK_VK_RESULT(x) do { \
   vk::Result res = vk::Result(x); \
   vk::createResultValue(res, __FILE__ ":" TOSTRING(__LINE__)); \
} while (0)

namespace vulkan
{

template<typename T>
class HashedDesc
{
public:
   HashedDesc()
   {
   }

   HashedDesc(const T& desc)
      : mHash(desc.hash()), mDesc(desc)
   {
   }

   T& operator*()
   {
      return mDesc;
   }

   T * operator->()
   {
      return &mDesc;
   }

   DataHash hash() const
   {
      return mHash;
   }

   bool operator==(const HashedDesc<T>& other) const
   {
      return hash() == other.hash();
   }

private:
   DataHash mHash;
   T mDesc;

};

struct MemCacheMutator
{
   enum class Mode
   {
      None,
      Retile
   };

   Mode mode = Mode::None;
   struct
   {
      latte::SQ_TILE_MODE tileMode;
      uint32_t swizzle;
      uint32_t pitch;
      uint32_t width;
      uint32_t height;
      uint32_t depth;
      uint32_t aa;
      bool isDepth;
      uint32_t bpp;
   } retile;

   inline bool operator==(const MemCacheMutator& other) const
   {
      if (mode == Mode::None && other.mode == Mode::None) {
         return true;
      }

      // We currently cheat and use the DataHash system to compare.
      auto hash = DataHash {}.write(retile);
      auto ohash = DataHash {}.write(other.retile);
      return hash == ohash;
   }
};

typedef std::function<void()> DelayedMemWriteFunc;

struct MemCacheObject;

struct MemCacheSegment
{
   // Meta-data about what this segment represents
   phys_addr address;
   uint32_t size;

   // Stores the last computed hash for this data (from the CPU).
   DataHash dataHash;

   // Tracks the last CPU check of this segment, to avoid checking the
   // memory multiple times in a single batch.
   uint64_t lastCheckIndex;

   // Records if there is a pending GPU write for this data.  This is to ensure
   // that we do not overwrite a pending GPU write with random CPU data.
   bool gpuWritten;

   // The last change for this segment
   uint64_t lastChangeIndex;

   // Represents the object owning the most up to date version of the data.
   MemCacheObject *lastChangeOwner;

};

typedef std::map<phys_addr, MemCacheSegment*> MemSegmentMap;

struct SectionRange
{
   uint32_t start = 0;
   uint32_t count = 0;

   inline bool covers(const SectionRange& other) const
   {
      auto end = start + count;
      auto other_end = other.start + other.count;
      return start <= other.start && end >= other_end;
   }

   inline bool intersects(const SectionRange& other) const
   {
      auto end = start + count;
      auto other_end = other.start + other.count;
      return !(start >= other_end || end <= other.start);
   }
};

struct MemCacheSection
{
   // The offset of this section, just to find it easier
   uint32_t offset;

   // The size of this particular section
   uint32_t size;

   // Pointer into the segments map for this memory range
   MemSegmentMap::iterator firstSegment;

   // Records the last change index for this data
   uint64_t lastChangeIndex;

   // Records some information used to optimize the uploading and
   // copying of segments between different cache objects.
   uint64_t wantedChangeIndex;
   bool needsUpload;
};

struct MemCacheObject
{
   // Meta-data about what this cache object represents.
   phys_addr address;
   uint32_t size;
   MemCacheMutator mutator;

   // The buffer data.
   VmaAllocation allocation;
   vk::Buffer buffer;

   // The various sections that make up this buffer
   std::vector<MemCacheSection> sections;

   // Stores a function used to update this buffer when the cost of
   // generating the new data is significant and we want to avoid it
   // unless the data ends up actually being needed.
   DelayedMemWriteFunc delayedWriteFunc;
   SectionRange delayedWriteRange;

   // Records the last PM4 context which refers to this data.
   uint64_t lastUsageIndex;

   // Records the number of external objects relying on this...
   uint64_t refCount;
};

struct MemChangeRecord
{
   uint64_t changeIndex;
   MemCacheObject *cache;
   SectionRange sections;
};

struct DataBufferObject : MemCacheObject { };

enum class ShaderStage
{
   Vertex = 0,
   Geometry = 1,
   Pixel = 2
};

struct VertexShaderObject
{
   HashedDesc<spirv::VertexShaderDesc> desc;
   spirv::VertexShader shader;
   vk::ShaderModule module;
   vk::ShaderModule rectStubModule;
};

struct GeometryShaderObject
{
   HashedDesc<spirv::GeometryShaderDesc> desc;
   spirv::GeometryShader shader;
   vk::ShaderModule module;
};

struct PixelShaderObject
{
   HashedDesc<spirv::PixelShaderDesc> desc;
   spirv::PixelShader shader;
   vk::ShaderModule module;
};

struct ColorBufferDesc
{
   uint32_t base256b;
   uint32_t pitchTileMax;
   uint32_t sliceTileMax;
   latte::CB_FORMAT format;
   latte::CB_NUMBER_TYPE numberType;
   latte::BUFFER_ARRAY_MODE arrayMode;
   uint32_t sliceStart;
   uint32_t sliceEnd;
};

struct DepthStencilBufferDesc
{
   uint32_t base256b;
   uint32_t pitchTileMax;
   uint32_t sliceTileMax;
   latte::DB_FORMAT format;
   latte::BUFFER_ARRAY_MODE arrayMode;
   uint32_t sliceStart;
   uint32_t sliceEnd;
};

struct VertexBufferDesc
{
   phys_addr baseAddress;
   uint32_t size;
   uint32_t stride;
};

enum class StagingBufferType : uint32_t
{
   CpuToGpu = 0,
   GpuToCpu = 1,
   GpuToGpu = 2
};

struct StagingBuffer
{
   StagingBufferType type;
   uint32_t maximumSize;
   uint32_t size;
   vk::Buffer buffer;
   VmaAllocation memory;
   void *mappedPtr;
};

struct SyncWaiter
{
   bool isCompleted = false;
   vk::Fence fence;
   std::vector<vk::DescriptorPool> descriptorPools;
   std::vector<vk::QueryPool> occQueryPools;
   std::vector<StagingBuffer *> stagingBuffers;
   std::vector<std::function<void()>> callbacks;

   vk::CommandBuffer cmdBuffer;
};

enum class ResourceUsage : uint32_t
{
   Undefined,
   ColorAttachment,
   DepthStencilAttachment,
   Texture,
   UniformBuffer,
   AttributeBuffer,
   StreamOutBuffer,
   StreamOutCounterRead,
   StreamOutCounterWrite,
   TransferSrc,
   TransferDst
};

struct SurfaceDesc
{
   // BaseAddress is a uint32 rather than a phys_addr as it actually
   // encodes information other than just the base address (swizzle).

   uint32_t baseAddress;
   uint32_t pitch;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t samples;
   latte::SQ_TEX_DIM dim;
   latte::SQ_TILE_TYPE tileType;
   latte::SQ_TILE_MODE tileMode;
   latte::SurfaceFormat format;

   inline uint32_t calcAlignedBaseAddress() const
   {
      if (tileMode >= latte::SQ_TILE_MODE::TILED_2D_THIN1) {
         return baseAddress & ~(0x800 - 1);
      } else {
         return baseAddress & ~(0x100 - 1);
      }
   }

   inline uint32_t calcSwizzle() const
   {
      return baseAddress & 0x00000F00;
   }

   inline DataHash hash(bool byCompat = false) const
   {
      // tileMode and swizzle are intentionally omited as they
      // do not affect data placement or size, but only upload/downloads.
      // It is possible that a tile-type switch may invalidate
      // old data though...

      // TODO: Handle tiling changes, major memory reuse issues...

      struct
      {
         uint32_t address;
         uint32_t format;
         uint32_t dim;
         uint32_t samples;
         uint32_t tileType;
         uint32_t tileMode;
         uint32_t width;
         uint32_t pitch;
         uint32_t height;
         uint32_t depth;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.address = calcAlignedBaseAddress();
      _dataHash.format = format;
      _dataHash.samples = samples;
      _dataHash.tileType = tileType;
      _dataHash.tileMode = tileMode;

      if (!byCompat) {
         // TODO: Figure out if we can emulate 2D_ARRAY surfaces
         // as 3D surfaces so we can get both kinds of views from
         // them?
         // Figure out which surfaces are compatible
         // at a DIM level...  Basically, anything we
         // can generate a view of.
         switch (dim) {
         case latte::SQ_TEX_DIM::DIM_3D:
            _dataHash.dim = 3;
            break;
         case latte::SQ_TEX_DIM::DIM_2D:
         case latte::SQ_TEX_DIM::DIM_2D_MSAA:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
         case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
         case latte::SQ_TEX_DIM::DIM_CUBEMAP:
            _dataHash.dim = 2;
            break;
         case latte::SQ_TEX_DIM::DIM_1D:
         case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
            _dataHash.dim = 1;
            break;
         default:
            decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
         }
         //_dataHash.dim = dim;
      }

      switch (dim) {
      case latte::SQ_TEX_DIM::DIM_3D:
         if (!byCompat) {
            _dataHash.depth = depth;
         }
         // fallthrough
      case latte::SQ_TEX_DIM::DIM_2D:
      case latte::SQ_TEX_DIM::DIM_2D_MSAA:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY:
      case latte::SQ_TEX_DIM::DIM_2D_ARRAY_MSAA:
      case latte::SQ_TEX_DIM::DIM_CUBEMAP:
         _dataHash.height = height;
         // fallthrough
      case latte::SQ_TEX_DIM::DIM_1D:
      case latte::SQ_TEX_DIM::DIM_1D_ARRAY:
         _dataHash.pitch = pitch;
         if (!byCompat) {
            _dataHash.width = width;
         }
         break;
      default:
         decaf_abort(fmt::format("Unsupported texture dim: {}", dim));
      }

      return DataHash {}.write(_dataHash);
   }
};

struct SurfaceSubRange
{
   uint32_t firstSlice;
   uint32_t numSlices;
};

struct SurfaceObject;

struct SurfaceGroupObject
{
   SurfaceDesc desc;

   std::list<SurfaceObject *> surfaces;
   std::vector<SurfaceObject *> sliceOwners;
};

struct SurfaceSlice
{
   uint64_t lastChangeIndex;
};

struct SurfaceObject
{
   SurfaceDesc desc;

   SurfaceGroupObject *group;
   uint64_t lastUsageIndex;

   uint32_t pitch;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t arrayLayers;

   std::vector<SurfaceSlice> slices;

   MemCacheObject *memCache;
   ResourceUsage activeUsage;

   vk::Image image;
   vk::DeviceMemory imageMem;
   vk::BufferImageCopy bufferRegion;
   vk::ImageSubresourceRange subresRange;
   vk::ImageLayout activeLayout;
};

struct SurfaceViewDesc
{
   SurfaceDesc surfaceDesc;
   uint32_t sliceStart;
   uint32_t sliceEnd;
   std::array<latte::SQ_SEL, 4> channels;

   inline DataHash hash() const
   {
      struct {
         uint32_t sliceStart;
         uint32_t sliceEnd;
         std::array<latte::SQ_SEL, 4> channels;
      } _dataHash;
      memset(&_dataHash, 0xFF, sizeof(_dataHash));

      _dataHash.sliceStart = sliceStart;
      _dataHash.sliceEnd = sliceEnd;
      _dataHash.channels = channels;

      return surfaceDesc.hash().write(_dataHash);
   }
};

struct SurfaceViewObject
{
   SurfaceViewDesc desc;
   SurfaceObject *surface;

   SurfaceSubRange surfaceRange;

   vk::Image boundImage;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
};

struct TextureDesc
{
   SurfaceViewDesc surfaceDesc;
};

struct FramebufferDesc
{
   std::array<ColorBufferDesc, latte::MaxRenderTargets> colorTargets;
   DepthStencilBufferDesc depthTarget;

   inline DataHash hash() const {
      return DataHash {}.write(*this);
   }
};

struct FramebufferObject
{
   HashedDesc<FramebufferDesc> desc;

   std::array<SurfaceViewObject*, latte::MaxRenderTargets> colorSurfaces;
   SurfaceViewObject *depthSurface;

   vk::Extent2D renderArea;
   std::array<vk::ImageView, 9> boundViews;
   vk::Framebuffer framebuffer;
};

struct SamplerDesc
{
   latte::SQ_TEX_SAMPLER_WORD0_N texSamplerWord0;
   latte::SQ_TEX_SAMPLER_WORD1_N texSamplerWord1;
   latte::SQ_TEX_SAMPLER_WORD2_N texSamplerWord2;

   inline DataHash hash() const {
      return DataHash {}.write(*this);
   }
};

struct SamplerObject
{
   HashedDesc<SamplerDesc> desc;
   vk::Sampler sampler;
};

struct SwapChainDesc
{
   phys_addr baseAddress;
   uint32_t width;
   uint32_t height;
};

// SwapChainObjects are backed by a surface, but expose less things
// as there are more specific guarentees provided, such as:
//  - layout is always TransferDstOptimal
struct SwapChainObject
{
   SwapChainDesc desc;
   bool presentable;
   vk::Image image;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
   SurfaceObject *_surface;
};



struct RenderPassDesc
{
   struct ColorTarget
   {
      bool isEnabled;
      latte::CB_FORMAT format;
      latte::CB_NUMBER_TYPE numberType;
      uint32_t samples;
   };

   struct DepthStencilTarget
   {
      bool isEnabled;
      latte::DB_FORMAT format;
   };

   std::array<ColorTarget, 8> colorTargets;
   DepthStencilTarget depthTarget;

   inline DataHash hash() const {
      return DataHash {}.write(*this);
   }
};


struct RenderPassObject
{
   HashedDesc<RenderPassDesc> desc;
   vk::RenderPass renderPass;

   std::array<int, latte::MaxRenderTargets> colorAttachmentIndexes;
   int depthAttachmentIndex;
};

struct PipelineDesc
{
   RenderPassObject *renderPass;
   VertexShaderObject *vertexShader;
   GeometryShaderObject *geometryShader;
   PixelShaderObject *pixelShader;

   std::array<uint32_t, latte::MaxAttribBuffers> attribBufferStride;
   latte::VGT_DI_PRIMITIVE_TYPE primitiveType;
   bool primitiveResetEnabled;
   uint32_t primitiveResetIndex;
   bool dx9Consts;

   struct StencilOpState
   {
      latte::REF_FUNC compareFunc;
      latte::DB_STENCIL_FUNC failOp;
      latte::DB_STENCIL_FUNC zPassOp;
      latte::DB_STENCIL_FUNC zFailOp;
      uint8_t ref;
      uint8_t mask;
      uint8_t writeMask;
   };
   bool stencilEnable;
   StencilOpState stencilFront;
   StencilOpState stencilBack;

   bool zEnable;
   bool zWriteEnable;
   latte::REF_FUNC zFunc;
   bool rasteriserDisable;
   uint32_t lineWidth;
   latte::PA_FACE paFace;
   bool cullFront;
   bool cullBack;
   uint32_t polyPType;
   bool polyBiasEnabled;
   float polyBiasClamp;
   float polyBiasOffset;
   float polyBiasScale;
   bool zclipDisabled;

   uint32_t rop3;

   struct BlendControl
   {
      uint8_t targetMask;
      bool blendingEnabled;
      bool opacityWeight;
      latte::CB_COMB_FUNC colorCombFcn;
      latte::CB_BLEND_FUNC colorSrcBlend;
      latte::CB_BLEND_FUNC colorDstBlend;
      latte::CB_COMB_FUNC alphaCombFcn;
      latte::CB_BLEND_FUNC alphaSrcBlend;
      latte::CB_BLEND_FUNC alphaDstBlend;
   };

   std::array<BlendControl, latte::MaxRenderTargets> cbBlendControls;
   std::array<float, 4> cbBlendConstants;

   inline DataHash hash() const {
      return DataHash {}.write(*this);
   }
};

struct PipelineObject
{
   HashedDesc<PipelineDesc> desc;
   vk::Pipeline pipeline;
   bool needsPremultipliedTargets;
   std::array<bool, latte::MaxRenderTargets> targetIsPremultiplied;
};

struct DrawDesc
{
   void *indices;
   latte::VGT_INDEX_TYPE indexType;
   latte::VGT_DMA_SWAP indexSwapMode;
   latte::VGT_DI_PRIMITIVE_TYPE primitiveType;
   bool isRectDraw;
   uint32_t numIndices;
   uint32_t baseVertex;
   uint32_t numInstances;
   uint32_t baseInstance;
};

class Driver : public gpu::VulkanDriver, public Pm4Processor
{
public:
   Driver();
   virtual ~Driver();
   virtual gpu::GraphicsDriverType type() override;

   virtual void initialise(vk::PhysicalDevice physDevice, vk::Device drive, vk::Queue queue, uint32_t queueFamilyIndex) override;
   virtual void shutdown() override;
   virtual void getSwapBuffers(vk::Image &tvImage, vk::ImageView &tvView, vk::Image &drcImage, vk::ImageView &drcView) override;

   virtual void run() override;
   virtual void stop() override;
   virtual void runUntilFlip() override;

   virtual float getAverageFPS() override;
   virtual float getAverageFrametimeMS() override;

   virtual gpu::VulkanDriver::DebuggerInfo *
   getDebuggerInfo() override;

   virtual void notifyCpuFlush(phys_addr address, uint32_t size) override;
   virtual void notifyGpuFlush(phys_addr address, uint32_t size) override;

protected:
   void initialiseBlankSampler();
   void initialiseBlankImage();
   void initialiseBlankBuffer();
   void setupResources();
   void updateDebuggerInfo();
   void validateDevice();

   // Command Buffer Stuff
   void beginCommandGroup();
   void endCommandGroup();
   void beginCommandBuffer();
   void endCommandBuffer();

   // Descriptor Sets
   vk::DescriptorPool allocateDescriptorPool(uint32_t numDraws);
   vk::DescriptorSet allocateGenericDescriptorSet(vk::DescriptorSetLayout &setLayout);
   vk::DescriptorSet allocateVertexDescriptorSet();
   vk::DescriptorSet allocateGeometryDescriptorSet();
   vk::DescriptorSet allocatePixelDescriptorSet();
   void retireDescriptorPool(vk::DescriptorPool descriptorPool);

   // Fences
   SyncWaiter * allocateSyncWaiter();
   void releaseSyncWaiter(SyncWaiter *syncWaiter);
   void submitSyncWaiter(SyncWaiter *syncWaiter);
   void executeSyncWaiter(SyncWaiter *syncWaiter);
   void fenceWaiterThread();
   void checkSyncFences();
   void addRetireTask(std::function<void()> fn);

   // Query Pools
   vk::QueryPool allocateOccQueryPool();
   void retireOccQueryPool(vk::QueryPool pool);

   // Driver
   void executeBuffer(const gpu::ringbuffer::Buffer &buffer);
   int32_t findMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags props);

   // Viewports
   bool checkCurrentViewportAndScissor();
   void bindViewportAndScissor();

   // Samplers
   SamplerDesc getSamplerDesc(ShaderStage shaderStage, uint32_t samplerIdx);
   void checkCurrentSampler(ShaderStage shaderStage, uint32_t samplerIdx);
   bool checkCurrentSamplers();

   // Textures
   TextureDesc getTextureDesc(ShaderStage shaderStage, uint32_t textureIdx);
   bool checkCurrentTexture(ShaderStage shaderStage, uint32_t textureIdx);
   bool checkCurrentTextures();
   void prepareCurrentTextures();

   // CBuffers
   void checkCurrentUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx);
   void checkCurrentGprBuffer(ShaderStage shaderStage);
   bool checkCurrentShaderBuffers();

   // Memory Cache
   MemCacheSegment * _allocateMemSegment(phys_addr address, uint32_t size);
   MemSegmentMap::iterator _splitMemSegment(MemSegmentMap::iterator iter, uint32_t newSize);
   MemSegmentMap::iterator _getMemSegment(phys_addr address, uint32_t maxSize);
   void _ensureMemSegments(MemSegmentMap::iterator firstSegment, uint32_t size);
   void _refreshMemSegment(MemCacheSegment *segment);

   MemCacheObject * _allocMemCache(phys_addr address, const std::vector<uint32_t>& sectionSizes, const MemCacheMutator& mutator);
   void _uploadMemCacheRaw(MemCacheObject *cache, SectionRange sections);
   void _uploadMemCacheRetile(MemCacheObject *cache, SectionRange sections);
   void _uploadMemCache(MemCacheObject *cache, SectionRange sections);
   void _downloadMemCacheRaw(MemCacheObject *cache, SectionRange sections);
   void _downloadMemCacheRetile(MemCacheObject *cache, SectionRange sections);
   void _downloadMemCache(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache_Check(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache_Update(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache(MemCacheObject *cache, SectionRange sections);
   void _invalidateMemCache(MemCacheObject *cache, SectionRange sections, DelayedMemWriteFunc delayedWriteHandler);
   void _barrierMemCache(MemCacheObject *cache, ResourceUsage usage, SectionRange sections);
   SectionRange _sectionsFromOffsets(MemCacheObject *cache, uint32_t begin, uint32_t end);
   MemCacheObject * getMemCache(phys_addr address, uint32_t size, const std::vector<uint32_t>& sectionSizes, const MemCacheMutator& mutator = MemCacheMutator {});
   void invalidateMemCacheDelayed(MemCacheObject *cache, uint32_t offset, uint32_t size, DelayedMemWriteFunc delayedWriteHandler);
   void transitionMemCache(MemCacheObject *cache, ResourceUsage usage, uint32_t offset = 0, uint32_t size = 0);
   DataBufferObject * getDataMemCache(phys_addr baseAddress, uint32_t size);
   void downloadPendingMemCache();

   // Staging
   StagingBuffer * _allocStagingBuffer(uint32_t size, StagingBufferType type);
   StagingBuffer * getStagingBuffer(uint32_t size, StagingBufferType type);
   void retireStagingBuffer(StagingBuffer *sbuffer);
   void * mapStagingBuffer(StagingBuffer *sbuffer);
   void unmapStagingBuffer(StagingBuffer *sbuffer);

   // Surfaces
   MemCacheObject * _getSurfaceMemCache(const SurfaceDesc &info);
   void _copySurface(SurfaceObject *dst, SurfaceObject *src, SurfaceSubRange range);

   SurfaceGroupObject * _allocateSurfaceGroup(const SurfaceDesc &info);
   void _releaseSurfaceGroup(SurfaceGroupObject *surfaceGroup);
   void _addSurfaceGroupSurface(SurfaceGroupObject *surfaceGroup, SurfaceObject *surface);
   void _removeSurfaceGroupSurface(SurfaceGroupObject *surfaceGroup, SurfaceObject *surface);
   void _updateSurfaceGroupSlice(SurfaceGroupObject *surfaceGroup, uint32_t sliceId, SurfaceObject *surface);
   SurfaceObject * _getSurfaceGroupOwner(SurfaceGroupObject *surfaceGroup, uint32_t sliceId, uint64_t minChangeIndex);
   SurfaceGroupObject * _getSurfaceGroup(const SurfaceDesc &info);

   SurfaceObject * _allocateSurface(const SurfaceDesc &info);
   void _releaseSurface(SurfaceObject *surface);
   void _upgradeSurface(SurfaceObject *surface, const SurfaceDesc &info);
   void _readSurfaceData(SurfaceObject *surface, SurfaceSubRange range);
   void _writeSurfaceData(SurfaceObject *surface, SurfaceSubRange range);
   void _refreshSurface(SurfaceObject *surface, SurfaceSubRange range);
   void _invalidateSurface(SurfaceObject *surface, SurfaceSubRange range);
   void _barrierSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range);
   SurfaceObject * getSurface(const SurfaceDesc& info);
   void transitionSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range);

   SurfaceViewObject * _allocateSurfaceView(const SurfaceViewDesc& info);
   void _releaseSurfaceView(SurfaceViewObject *surfaceView);
   SurfaceViewObject * getSurfaceView(const SurfaceViewDesc& info);
   void transitionSurfaceView(SurfaceViewObject *surfaceView, ResourceUsage usage, vk::ImageLayout layout);

   // Vertex Buffers
   VertexBufferDesc getAttribBufferDesc(uint32_t bufferIndex);
   bool checkCurrentAttribBuffers();
   void bindAttribBuffers();

   // Indices
   void maybeSwapIndices();
   void maybeUnpackQuads();
   bool checkCurrentIndices();
   void bindIndexBuffer();

   // Draws
   void bindShaderDescriptors(ShaderStage shaderStage);
   void bindShaderResources();
   void drawGenericIndexed(latte::VGT_DRAW_INITIATOR drawInit, uint32_t numIndices, void *indices);

   // Framebuffers
   FramebufferDesc getFramebufferDesc();
   bool checkCurrentFramebuffer();
   SurfaceViewObject * getColorBuffer(const ColorBufferDesc& info);
   SurfaceViewObject * getDepthStencilBuffer(const DepthStencilBufferDesc& info);
   void prepareCurrentFramebuffer();

   // Swap Chains
   SwapChainObject * allocateSwapChain(const SwapChainDesc &desc);
   void releaseSwapChain(SwapChainObject *swapChain);

   // Shaders
   spirv::VertexShaderDesc getVertexShaderDesc();
   spirv::GeometryShaderDesc getGeometryShaderDesc();
   spirv::PixelShaderDesc getPixelShaderDesc();
   bool checkCurrentVertexShader();
   bool checkCurrentGeometryShader();
   bool checkCurrentPixelShader();

   // Render Passes
   RenderPassDesc getRenderPassDesc();
   bool checkCurrentRenderPass();

   // Pipelines
   PipelineDesc getPipelineDesc();
   bool checkCurrentPipeline();

   // Debug
   void setVkObjectName(VkBuffer object, const char *name);
   void setVkObjectName(VkSampler object, const char *name);
   void setVkObjectName(VkImage object, const char *name);
   void setVkObjectName(VkImageView object, const char *name);
   void setVkObjectName(VkShaderModule object, const char *name);

private:
   virtual void decafSetBuffer(const latte::pm4::DecafSetBuffer &data) override;
   virtual void decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data) override;
   virtual void decafSwapBuffers(const latte::pm4::DecafSwapBuffers &data) override;
   virtual void decafCapSyncRegisters(const latte::pm4::DecafCapSyncRegisters &data) override;
   virtual void decafClearColor(const latte::pm4::DecafClearColor &data) override;
   virtual void decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data) override;
   virtual void decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data) override;
   virtual void decafCopySurface(const latte::pm4::DecafCopySurface &data) override;
   virtual void decafExpandColorBuffer(const latte::pm4::DecafExpandColorBuffer &data) override;
   virtual void drawIndexAuto(const latte::pm4::DrawIndexAuto &data) override;
   virtual void drawIndex2(const latte::pm4::DrawIndex2 &data) override;
   virtual void drawIndexImmd(const latte::pm4::DrawIndexImmd &data) override;
   virtual void memWrite(const latte::pm4::MemWrite &data) override;
   virtual void eventWrite(const latte::pm4::EventWrite &data) override;
   virtual void eventWriteEOP(const latte::pm4::EventWriteEOP &data) override;
   virtual void pfpSyncMe(const latte::pm4::PfpSyncMe &data) override;
   virtual void setPredication(const latte::pm4::SetPredication &data) override;
   virtual void streamOutBaseUpdate(const latte::pm4::StreamOutBaseUpdate &data) override;
   virtual void streamOutBufferUpdate(const latte::pm4::StreamOutBufferUpdate &data) override;
   virtual void surfaceSync(const latte::pm4::SurfaceSync &data) override;

   virtual void applyRegister(latte::Register reg) override;

private:
   enum class RunState
   {
      None,
      Running,
      Stopped
   };

   std::atomic<RunState> mRunState = RunState::None;
   gpu::VulkanDriver::DebuggerInfo mDebuggerInfo;
   std::thread mFenceThread;
   std::thread mThread;
   std::mutex mFenceMutex;
   std::list<SyncWaiter*> mFencesWaiting;
   std::list<SyncWaiter*> mFencesPending;
   std::condition_variable mFenceSignal;
   std::vector<SyncWaiter*> mWaiterPool;
   VmaAllocator mAllocator;
   uint64_t mMemChangeCounter = 0;
   uint64_t *mLastOccQueryAddr = nullptr;
   vk::QueryPool mLastOccQuery;

   SyncWaiter *mActiveSyncWaiter = nullptr;
   vk::CommandBuffer mActiveCommandBuffer;
   vk::DescriptorPool mActiveDescriptorPool;
   uint32_t mActiveDescriptorPoolDrawsLeft;
   RenderPassObject *mActiveRenderPass = nullptr;
   PipelineObject *mActivePipeline = nullptr;
   uint64_t mActiveBatchIndex = 0;

   vk::DescriptorSetLayout mVertexDescriptorSetLayout;
   vk::DescriptorSetLayout mGeometryDescriptorSetLayout;
   vk::DescriptorSetLayout mPixelDescriptorSetLayout;
   vk::PipelineLayout mPipelineLayout;

   DrawDesc mCurrentDrawDesc;
   vk::Viewport mCurrentViewport;
   vk::Rect2D mCurrentScissor;
   StagingBuffer *mCurrentIndexBuffer = nullptr;
   VertexShaderObject *mCurrentVertexShader = nullptr;
   GeometryShaderObject *mCurrentGeometryShader = nullptr;
   PixelShaderObject *mCurrentPixelShader = nullptr;
   FramebufferObject *mCurrentFramebuffer = nullptr;
   RenderPassObject *mCurrentRenderPass = nullptr;
   PipelineObject *mCurrentPipeline = nullptr;
   std::array<DataBufferObject*, latte::MaxAttribBuffers> mCurrentAttribBuffers = { nullptr };
   std::array<std::array<SamplerObject*, latte::MaxSamplers>, 3> mCurrentSamplers = { { nullptr } };
   std::array<std::array<SurfaceViewObject*, latte::MaxTextures>, 3> mCurrentTextures = { { nullptr } };
   std::array<StagingBuffer*, 3> mCurrentGprBuffers = { nullptr };
   std::array<std::array<DataBufferObject*, latte::MaxUniformBlocks>, 3> mCurrentUniformBlocks = { { nullptr } };

   std::vector<MemChangeRecord> mDirtyMemCaches;

   std::vector<uint8_t> mScratchRetiling;
   std::vector<uint8_t> mScratchIdxSwap;
   std::vector<uint8_t> mScratchIdxDequad;

   using duration_system_clock = std::chrono::duration<double, std::chrono::system_clock::period>;
   using duration_ms = std::chrono::duration<double, std::chrono::milliseconds::period>;
   std::chrono::time_point<std::chrono::system_clock> mLastSwap;
   duration_system_clock mAverageFrameTime;

   vk::PhysicalDevice mPhysDevice;
   vk::Device mDevice;
   vk::Queue mQueue;
   vk::DispatchLoaderDynamic mVkDynLoader;
   vk::CommandPool mCommandPool;
   vk::Sampler mBlankSampler;
   vk::Image mBlankImage;
   vk::ImageView mBlankImageView;
   vk::Buffer mBlankBuffer;
   SwapChainObject *mTvSwapChain = nullptr;
   SwapChainObject *mDrcSwapChain = nullptr;
   RenderPassObject *mRenderPass;
   std::list<StagingBuffer *> mStagingBuffers;
   std::vector<vk::DescriptorPool> mDescriptorPools;
   std::vector<vk::QueryPool> mOccQueryPools;
   MemSegmentMap mMemSegmentMap;
   std::unordered_map<DataHash, SurfaceGroupObject*> mSurfaceGroups;
   std::unordered_map<DataHash, SurfaceObject*> mSurfaces;
   std::unordered_map<DataHash, SurfaceViewObject*> mSurfaceViews;
   std::unordered_map<DataHash, VertexShaderObject*> mVertexShaders;
   std::unordered_map<DataHash, GeometryShaderObject*> mGeometryShaders;
   std::unordered_map<DataHash, FramebufferObject*> mFramebuffers;
   std::unordered_map<DataHash, PixelShaderObject*> mPixelShaders;
   std::unordered_map<DataHash, RenderPassObject*> mRenderPasses;
   std::unordered_map<DataHash, PipelineObject*> mPipelines;
   std::unordered_map<DataHash, SamplerObject*> mSamplers;
   std::unordered_map<DataHash, MemCacheObject *> mMemCaches;
};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
