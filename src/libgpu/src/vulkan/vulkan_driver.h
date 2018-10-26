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
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vulkan/vulkan.hpp>

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

   operator bool() const
   {
      return !!mHash;
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

typedef std::function<void(bool cancelled)> DelayedMemWriteFunc;

struct MemCacheObject
{
   // Meta-data about what this cache object represents.
   phys_addr address;
   uint32_t size;
   MemCacheMutator mutator;

   // The buffer data.
   VmaAllocation allocation;
   vk::Buffer buffer;

   // Stores the last computed hash for this data (from the CPU).
   uint64_t gpuWrittenIndex;
   DelayedMemWriteFunc delayedWriteFunc;
   DataHash dataHash;

   // Records the last PM4 context which refers to this data.
   uint64_t lastUsageIndex;

   // Records the number of external objects relying on this...
   uint64_t refCount;
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
};

struct DepthStencilBufferDesc
{
   uint32_t base256b;
   uint32_t pitchTileMax;
   uint32_t sliceTileMax;
   latte::DB_FORMAT format;
   latte::BUFFER_ARRAY_MODE arrayMode;
};

struct VertexBufferDesc
{
   phys_addr baseAddress;
   uint32_t size;
   uint32_t stride;
};

struct StagingBuffer
{
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
   std::vector<vk::DescriptorSet> vertexDescriptorSets;
   std::vector<vk::DescriptorSet> geometryDescriptorSets;
   std::vector<vk::DescriptorSet> pixelDescriptorSets;
   std::vector<StagingBuffer *> stagingBuffers;
   std::vector<std::function<void()>> callbacks;

   vk::CommandBuffer cmdBuffer;
};

enum class ResourceUsage : uint32_t
{
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

      auto hash = DataHash {}
         .write(calcAlignedBaseAddress())
         .write(format)
         .write(samples)
         .write(tileType);

      auto dimDimensions = latte::getTexDimDimensions(dim);
      if (dimDimensions < 1 || dimDimensions >= 4) {
         decaf_abort("Unexpected dim dimensions.");
      }

      if (byCompat) {
         // If we are looking at this in terms of compatibility mode, we can
         // actually bump the dimensions down one level, as that is the cross
         // compatibility of surface memory.
         dimDimensions--;
      } else {
         // If we are looking for a precise match, we need to include the
         // width of the surface in order to appropriately fetch it.
         hash = hash.write(width);
      }

      if (dimDimensions >= 1) {
         hash = hash.write(pitch);
      }
      if (dimDimensions >= 2) {
         hash = hash.write(height);
      }
      if (dimDimensions >= 3) {
         hash = hash.write(depth);
      }

      return hash;
   }
};

struct SurfaceObject;

struct SurfaceGroupObject
{
   SurfaceDesc desc;

   uint64_t lastUsageIndex;

   MemCacheObject *cacheMem;
   DataHash hash;

   SurfaceObject *masterImage;
   SurfaceObject *activeImage;
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

   vk::Image image;
   vk::DeviceMemory imageMem;
   vk::ImageSubresourceRange subresRange;
   vk::ImageLayout activeLayout;
};

struct SurfaceViewDesc
{
   SurfaceDesc surfaceDesc;
   std::array<latte::SQ_SEL, 4> channels;

   inline DataHash hash() const
   {
      return surfaceDesc.hash()
         .write(channels);
   }
};

struct SurfaceViewObject
{
   SurfaceViewDesc desc;
   SurfaceObject *surface;

   vk::ImageView imageView;
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
   vk::Framebuffer framebuffer;
   std::vector<SurfaceViewObject*> colorSurfaces;
   SurfaceViewObject *depthSurface;
   vk::Extent2D renderArea;
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
   bool isScreenSpace;
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
   void retireVertexDescriptorSet(vk::DescriptorSet descriptorSet);
   void retireGeometryDescriptorSet(vk::DescriptorSet descriptorSet);
   void retirePixelDescriptorSet(vk::DescriptorSet descriptorSet);

   // Fences
   SyncWaiter * allocateSyncWaiter();
   void releaseSyncWaiter(SyncWaiter *syncWaiter);
   void submitSyncWaiter(SyncWaiter *syncWaiter);
   void executeSyncWaiter(SyncWaiter *syncWaiter);
   void fenceWaiterThread();
   void checkSyncFences();
   void addRetireTask(std::function<void()> fn);

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

   // CBuffers
   void checkCurrentUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx);
   void checkCurrentGprBuffer(ShaderStage shaderStage);
   bool checkCurrentShaderBuffers();

   // Memory Cache
   MemCacheObject * _allocMemCache(phys_addr address, uint32_t size, const MemCacheMutator& mutator);
   void _uploadMemCache(MemCacheObject *cache);
   void _downloadMemCache(MemCacheObject *cache);
   void _refreshMemCache(MemCacheObject *cache);
   MemCacheObject * getMemCache(phys_addr address, uint32_t size, const MemCacheMutator& mutator = MemCacheMutator {});
   void transitionMemCache(MemCacheObject *cache, ResourceUsage usage);
   void markMemCacheDirty(MemCacheObject *cache, DelayedMemWriteFunc delayedWriteHandler);
   DataBufferObject * getDataMemCache(phys_addr baseAddress, uint32_t size, bool discardData = false);
   void downloadPendingMemCache();

   // Staging
   StagingBuffer * allocTempBuffer(uint32_t size);
   StagingBuffer * getStagingBuffer(uint32_t size);
   void retireStagingBuffer(StagingBuffer *sbuffer);
   void * mapStagingBuffer(StagingBuffer *sbuffer, bool flushGpu);
   void unmapStagingBuffer(StagingBuffer *sbuffer, bool flushCpu);

   // Surfaces
   MemCacheObject * _getSurfaceMemCache(const SurfaceDesc &info);
   void _copySurface(SurfaceObject *dst, SurfaceObject *src);

   SurfaceGroupObject * _allocateSurfaceGroup(const SurfaceDesc &info);
   void _releaseSurfaceGroup(SurfaceGroupObject *surface);
   void _readSurfaceData(SurfaceGroupObject *surface);
   void _writeSurfaceData(SurfaceGroupObject *surface);
   void _activateGroupSurface(SurfaceGroupObject *surface, SurfaceObject *newImage, ResourceUsage usage);
   SurfaceGroupObject * _getSurfaceGroup(const SurfaceDesc &info);

   SurfaceObject * _allocateSurface(const SurfaceDesc &info);
   void _releaseSurface(SurfaceObject *surfaceData);
   SurfaceObject * getSurface(const SurfaceDesc& info);
   void _barrierSurface(SurfaceObject *surfaceData, ResourceUsage usage, vk::ImageLayout layout);
   void transitionSurface(SurfaceObject *surfaceData, ResourceUsage usage, vk::ImageLayout layout);

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
   void bindShaderResources();
   void drawGenericIndexed(latte::VGT_DRAW_INITIATOR drawInit, uint32_t numIndices, void *indices);

   // Framebuffers
   FramebufferDesc getFramebufferDesc();
   bool checkCurrentFramebuffer();
   SurfaceViewObject * getColorBuffer(const ColorBufferDesc& info);
   SurfaceViewObject * getDepthStencilBuffer(const DepthStencilBufferDesc& info);

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
   virtual void drawIndexAuto(const latte::pm4::DrawIndexAuto &data) override;
   virtual void drawIndex2(const latte::pm4::DrawIndex2 &data) override;
   virtual void drawIndexImmd(const latte::pm4::DrawIndexImmd &data) override;
   virtual void memWrite(const latte::pm4::MemWrite &data) override;
   virtual void eventWrite(const latte::pm4::EventWrite &data) override;
   virtual void eventWriteEOP(const latte::pm4::EventWriteEOP &data) override;
   virtual void pfpSyncMe(const latte::pm4::PfpSyncMe &data) override;
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

   SyncWaiter *mActiveSyncWaiter = nullptr;
   uint64_t mActivePm4BufferIndex = 0;
   vk::CommandBuffer mActiveCommandBuffer;
   vk::DescriptorPool mActiveDescriptorPool;
   uint32_t mActiveDescriptorPoolDrawsLeft;
   RenderPassObject *mActiveRenderPass = nullptr;
   PipelineObject *mActivePipeline = nullptr;

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

   std::vector<MemCacheObject *> mDirtyMemCaches;
   std::list<MemCacheObject *> mPendingMemCacheDownloads;

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
   SwapChainObject *mTvSwapChain = nullptr;
   SwapChainObject *mDrcSwapChain = nullptr;
   RenderPassObject *mRenderPass;
   std::list<StagingBuffer *> mStagingBuffers;
   std::vector<vk::DescriptorPool> mDescriptorPools;
   std::vector<vk::DescriptorSet> mVertexDescriptorSets;
   std::vector<vk::DescriptorSet> mGeometryDescriptorSets;
   std::vector<vk::DescriptorSet> mPixelDescriptorSets;
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
   std::unordered_map<uint64_t, MemCacheObject *> mMemCaches;
};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
