#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include "gpu_ringbuffer.h"
#include "gpu_vulkandriver.h"
#include "gpu7_tiling.h"
#include "gpu7_tiling_vulkan.h"
#include "latte/latte_formats.h"
#include "latte/latte_constants.h"
#include "spirv/spirv_translate.h"
#include "pm4_processor.h"
#include "vk_mem_alloc.h"
#include "vulkan_descs.h"
#include "vulkan_memtracker.h"

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

   const T& operator*() const
   {
      return mDesc;
   }

   const T * operator->() const
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

enum class ResourceUsage : uint32_t
{
   Undefined,
   ColorAttachment,
   DepthStencilAttachment,
   VertexTexture,
   GeometryTexture,
   PixelTexture,
   VertexUniforms,
   GeometryUniforms,
   PixelUniforms,
   IndexBuffer,
   AttributeBuffer,
   StreamOutBuffer,
   StreamOutCounterRead,
   StreamOutCounterWrite,
   ComputeSsboRead,
   ComputeSsboWrite,
   TransferSrc,
   TransferDst,
   HostWrite,
   HostRead
};

struct ResourceUsageMeta
{
   bool isWrite;
   vk::AccessFlags accessFlags;
   vk::PipelineStageFlags stageFlags;
   vk::ImageLayout imageLayout;
};

typedef std::function<void()> DelayedMemWriteFunc;

struct MemCacheObject;
typedef MemoryTracker<MemCacheObject*> DriverMemoryTracker;
typedef DriverMemoryTracker::SegmentRef MemSegmentRef;
typedef DriverMemoryTracker::Segment MemSegment;

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
   // Pointer into the segments map for this memory range
   MemSegmentRef firstSegment;

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
   uint32_t numSections;
   uint32_t sectionSize;
   ResourceUsage activeUsage;

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

   // Intrusive linked list to enable us to chain together multiple
   // objects with different section layouts for faster lookup.
   MemCacheObject *nextObject;
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

struct RectStubShaderObject
{
   HashedDesc<spirv::RectStubShaderDesc> desc;
   spirv::RectStubShader shader;
   vk::ShaderModule module;
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
   uint32_t poolIndex;
   uint32_t maximumSize;
   uint32_t size;
   ResourceUsage activeUsage;
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
   std::vector<gpu7::tiling::vulkan::RetileHandle> retileHandles;
   std::vector<StagingBuffer *> stagingBuffers;
   std::vector<std::function<void()>> callbacks;

   vk::CommandBuffer cmdBuffer;
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

   gpu7::tiling::SurfaceDescription tilingDesc;
   gpu7::tiling::SurfaceInfo tilingInfo;

   std::vector<SurfaceSlice> slices;

   MemCacheObject *memCache;
   ResourceUsage activeUsage;

   vk::Image image;
   vk::DeviceMemory imageMem;
   vk::BufferImageCopy bufferRegion;
   vk::ImageSubresourceRange subresRange;
};

struct SurfaceViewObject
{
   HashedDesc<SurfaceViewDesc> desc;
   SurfaceObject *surface;

   SurfaceSubRange surfaceRange;

   vk::Image boundImage;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
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

struct SamplerObject
{
   HashedDesc<SamplerDesc> desc;
   vk::Sampler sampler;
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

struct RenderPassObject
{
   HashedDesc<RenderPassDesc> desc;
   vk::RenderPass renderPass;

   std::array<int, latte::MaxRenderTargets> colorAttachmentIndexes;
   int depthAttachmentIndex;
};

struct PipelineLayoutObject
{
   HashedDesc<PipelineLayoutDesc> desc;
   vk::DescriptorSetLayout descriptorLayout;
   vk::PipelineLayout pipelineLayout;
};

struct PipelineObject
{
   HashedDesc<PipelineDesc> desc;
   PipelineLayoutObject *pipelineLayout;
   vk::Pipeline pipeline;
   bool needsPremultipliedTargets;
   std::array<bool, latte::MaxRenderTargets> targetIsPremultiplied;
   uint32_t shaderLopMode;
   uint32_t shaderAlphaFunc;
   float shaderAlphaRef;
};

struct StreamContextObject
{
   VmaAllocation allocation;
   vk::Buffer buffer;
};

struct ShaderViewportData
{
   float xAdd, xMul;
   float yAdd, yMul;
   float zAdd, zMul;
};

struct IndexBufferCache
{
   latte::VGT_DI_PRIMITIVE_TYPE primitiveType;
   latte::VGT_INDEX_TYPE indexType;
   latte::VGT_DMA_SWAP swapMode;
   uint32_t numIndices;
   void *indexData;

   latte::VGT_DI_PRIMITIVE_TYPE newPrimitiveType;
   uint32_t newNumIndices;
   StagingBuffer *indexBuffer;
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

   bool streamOutEnabled = false;
   MemCacheObject *opaqueBuffer = nullptr;
   uint32_t opaqueStride = 0;

   vk::Viewport viewport;
   ShaderViewportData shaderViewportData;
   vk::Rect2D scissor;
   StagingBuffer *indexBuffer = nullptr;
   VertexShaderObject *vertexShader = nullptr;
   GeometryShaderObject *geometryShader = nullptr;
   PixelShaderObject *pixelShader = nullptr;
   RectStubShaderObject *rectStubShader = nullptr;
   FramebufferObject *framebuffer = nullptr;
   RenderPassObject *renderPass = nullptr;
   PipelineObject *pipeline = nullptr;
   bool framebufferDirty = true;
   std::array<std::array<bool, latte::MaxTextures>, 3> textureDirty = { { true } };
   std::array<DataBufferObject*, latte::MaxAttribBuffers> attribBuffers = { nullptr };
   std::array<std::array<SamplerObject*, latte::MaxSamplers>, 3> samplers = { { nullptr } };
   std::array<std::array<SurfaceViewObject*, latte::MaxTextures>, 3> textures = { { nullptr } };
   std::array<StagingBuffer*, 3> gprBuffers = { nullptr };
   std::array<std::array<DataBufferObject*, latte::MaxUniformBlocks>, 3> uniformBlocks = { { nullptr } };
   std::array<StreamContextObject*, latte::MaxStreamOutBuffers> streamOutContext = { nullptr };
   std::array<DataBufferObject*, latte::MaxStreamOutBuffers> streamOutBuffers = { nullptr };
};

struct Vec4
{
   float x;
   float y;
   float z;
   float w;
};

struct VsPushConstants
{
   Vec4 posMulAdd;
   Vec4 zSpaceMul;
};

struct PsPushConstants
{
   uint32_t alphaData;
   float alphaRef;
   uint32_t needPremultiply;
};

class Driver : public gpu::VulkanDriver, public Pm4Processor
{
public:
   Driver();
   virtual ~Driver();
   virtual gpu::GraphicsDriverType type() override;

   virtual void initialise(vk::Instance instance,
                           vk::PhysicalDevice physDevice,
                           vk::Device drive,
                           vk::Queue queue,
                           uint32_t queueFamilyIndex) override;
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

   ResourceUsageMeta getResourceUsageMeta(ResourceUsage usage);

   // Command Buffer Stuff
   void beginCommandGroup();
   void endCommandGroup();
   void beginCommandBuffer();
   void endCommandBuffer();

   // Descriptor Sets
   vk::DescriptorPool allocateDescriptorPool(uint32_t numDraws);
   vk::DescriptorSet allocateGenericDescriptorSet();
   void retireDescriptorPool(vk::DescriptorPool descriptorPool);

   // Fences
   SyncWaiter * allocateSyncWaiter();
   void releaseSyncWaiter(SyncWaiter *syncWaiter);
   void submitSyncWaiter(SyncWaiter *syncWaiter);
   void executeSyncWaiter(SyncWaiter *syncWaiter);
   void fenceWaiterThread();
   void checkSyncFences();
   void addRetireTask(std::function<void()> fn);

   // Retiling
   void dispatchGpuTile(vk::CommandBuffer &commandBuffer,
                        vk::Buffer dstBuffer, uint32_t dstOffset,
                        vk::Buffer srcBuffer, uint32_t srcOffset,
                        const gpu7::tiling::vulkan::RetileInfo& retileInfo);
   void dispatchGpuUntile(vk::CommandBuffer &commandBuffer,
                          vk::Buffer dstBuffer, uint32_t dstOffset,
                          vk::Buffer srcBuffer, uint32_t srcOffset,
                          const gpu7::tiling::vulkan::RetileInfo& retileInfo);

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
   void updateDrawSampler(ShaderStage shaderStage, uint32_t samplerIdx);
   bool checkCurrentSamplers();

   // Textures
   SurfaceViewDesc getTextureDesc(ShaderStage shaderStage, uint32_t textureIdx);
   void updateDrawTexture(ShaderStage shaderStage, uint32_t textureIdx);
   bool checkCurrentTextures();
   void prepareCurrentTextures();

   // CBuffers
   void updateDrawUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx);
   void updateDrawGprBuffer(ShaderStage shaderStage);
   bool checkCurrentShaderBuffers();

   MemCacheObject * _allocMemCache(phys_addr address, uint32_t numSections, uint32_t sectionSize);
   void _uploadMemCache(MemCacheObject *cache, SectionRange sections);
   void _downloadMemCache(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache_Check(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache_Update(MemCacheObject *cache, SectionRange sections);
   void _refreshMemCache(MemCacheObject *cache, SectionRange sections);
   void _invalidateMemCache(MemCacheObject *cache, SectionRange sections, const DelayedMemWriteFunc& delayedWriteHandler);
   void _barrierMemCache(MemCacheObject *cache, ResourceUsage usage, SectionRange sections);
   SectionRange _sectionsFromOffsets(MemCacheObject *cache, uint32_t begin, uint32_t end);
   MemCacheObject * getMemCache(phys_addr address, uint32_t numSections, uint32_t sectionSize);
   void invalidateMemCacheDelayed(MemCacheObject *cache, uint32_t offset, uint32_t size, const DelayedMemWriteFunc& delayedWriteHandler);
   void transitionMemCache(MemCacheObject *cache, ResourceUsage usage, uint32_t offset = 0, uint32_t size = 0);
   DataBufferObject * getDataMemCache(phys_addr baseAddress, uint32_t size);
   void downloadPendingMemCache();

   // Staging
   StagingBuffer * _allocStagingBuffer(uint32_t size, StagingBufferType type);
   StagingBuffer * getStagingBuffer(uint32_t size, StagingBufferType type);
   void retireStagingBuffer(StagingBuffer *sbuffer);
   void transitionStagingBuffer(StagingBuffer *sbuffer, ResourceUsage usage);
   void copyToStagingBuffer(StagingBuffer *sbuffer, uint32_t offset, const void *data, uint32_t size);
   void copyFromStagingBuffer(StagingBuffer *sbuffer, uint32_t offset, void *data, uint32_t size);

   // Surfaces
   MemCacheObject * _getSurfaceMemCache(const SurfaceDesc &info, const gpu7::tiling::SurfaceInfo& tilingInfo);
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
   void transitionSurface(SurfaceObject *surface, ResourceUsage usage, vk::ImageLayout layout, SurfaceSubRange range, bool skipChangeCheck = false);

   SurfaceViewObject * _allocateSurfaceView(const SurfaceViewDesc& info);
   void _releaseSurfaceView(SurfaceViewObject *surfaceView);
   SurfaceViewObject * getSurfaceView(const SurfaceViewDesc& info);
   void transitionSurfaceView(SurfaceViewObject *surfaceView, ResourceUsage usage, vk::ImageLayout layout, bool skipChangeCheck = false);

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
   void bindDescriptors();
   void bindShaderParams();
   void drawGenericIndexed(latte::VGT_DRAW_INITIATOR drawInit, uint32_t numIndices, void *indices);
   void flushPendingDraws();
   void drawCurrentState();

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
   bool checkCurrentRectStubShader();

   // Render Passes
   RenderPassDesc getRenderPassDesc();
   bool checkCurrentRenderPass();

   // Pipeline Layouts
   PipelineLayoutDesc generatePipelineLayoutDesc(const PipelineDesc& pipelineDesc);
   PipelineLayoutObject * getPipelineLayout(const HashedDesc<PipelineLayoutDesc>& desc, bool forPush = false);

   // Pipelines
   PipelineDesc getPipelineDesc();
   bool checkCurrentPipeline();

   // Stream Out
   StreamContextObject * allocateStreamContext(uint32_t initialOffset);
   void releaseStreamContext(StreamContextObject* stream);
   void readbackStreamContext(StreamContextObject *stream, phys_addr writeAddr);
   StreamOutBufferDesc getStreamOutBufferDesc(uint32_t bufferIndex);
   bool checkCurrentStreamOut();
   void bindStreamOutBuffers();
   void beginStreamOut();
   void endStreamOut();

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
   vk::PipelineCache mPipelineCache;

   SyncWaiter *mActiveSyncWaiter = nullptr;
   vk::CommandBuffer mActiveCommandBuffer;
   std::vector<vk::DescriptorSet> mAvailableDescriptorSets;
   RenderPassObject *mActiveRenderPass = nullptr;
   FramebufferObject *mActiveFramebuffer = nullptr;
   PipelineObject *mActivePipeline = nullptr;
   uint64_t mActiveBatchIndex = 0;

   bool mActiveVsConstantsSet = false;
   VsPushConstants mActiveVsConstants;
   bool mActivePsConstantsSet = false;
   PsPushConstants mActivePsConstants;

   bool mLastIndexBufferSet = false;
   IndexBufferCache mLastIndexBuffer;

   vk::DescriptorSetLayout mBaseDescriptorSetLayout;
   vk::PipelineLayout mPipelineLayout;

   std::array<StreamContextObject*, latte::MaxStreamOutBuffers> mStreamOutContext = { nullptr };
   std::vector<DrawDesc> mPendingDraws;
   DrawDesc *mCurrentDraw;

   DrawDesc mDrawCache;

   std::vector<MemChangeRecord> mDirtyMemCaches;

   std::vector<uint8_t> mScratchIdxSwap;
   std::vector<uint8_t> mScratchIdxDequad;
   std::vector<vk::DescriptorImageInfo> mScratchImageInfos;
   std::vector<vk::DescriptorBufferInfo> mScratchBufferInfos;
   std::vector<vk::WriteDescriptorSet> mScratchDescriptorWrites;

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
   std::array<std::array<std::vector<StagingBuffer *>, 20>, 3> mStagingBuffers;
   std::vector<StreamContextObject *> mStreamOutContextPool;
   std::vector<vk::DescriptorPool> mDescriptorPools;
   std::vector<vk::QueryPool> mOccQueryPools;
   std::unordered_map<DataHash, SurfaceGroupObject*> mSurfaceGroups;
   std::unordered_map<DataHash, SurfaceObject*> mSurfaces;
   std::unordered_map<DataHash, SurfaceViewObject*> mSurfaceViews;
   std::unordered_map<DataHash, VertexShaderObject*> mVertexShaders;
   std::unordered_map<DataHash, GeometryShaderObject*> mGeometryShaders;
   std::unordered_map<DataHash, FramebufferObject*> mFramebuffers;
   std::unordered_map<DataHash, PixelShaderObject*> mPixelShaders;
   std::unordered_map<DataHash, RectStubShaderObject*> mRectStubShaders;
   std::unordered_map<DataHash, RenderPassObject*> mRenderPasses;
   std::unordered_map<DataHash, PipelineLayoutObject *> mPipelineLayouts;
   std::unordered_map<DataHash, PipelineObject*> mPipelines;
   std::unordered_map<DataHash, SamplerObject*> mSamplers;
   std::unordered_map<uint64_t, MemCacheObject *> mMemCaches;

   gpu7::tiling::vulkan::Retiler mGpuRetiler;
   DriverMemoryTracker mMemTracker;

   bool mDebug;
   bool mDumpShaders;
   bool mDumpShaderBinariesOnly;
   bool mDumpTextures;
};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
