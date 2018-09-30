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

struct DataBufferObject
{
   phys_addr baseAddress;
   uint32_t size;

   DataHash hash;
   uint64_t lastHashedIndex;

   vk::Buffer buffer;
   VmaAllocation memory;
};

struct StagingBuffer
{
   uint32_t size;
   vk::Buffer buffer;
   VmaAllocation memory;
};

struct SyncWaiter
{
   bool isCompleted = false;
   vk::Fence fence;
   std::vector<vk::DescriptorPool> descriptorPools;
   std::vector<StagingBuffer *> stagingBuffers;
   std::vector<std::function<void()>> callbacks;

   vk::CommandBuffer cmdBuffer;
};

struct SurfaceDesc
{
   phys_addr baseAddress;
   uint32_t pitch;
   uint32_t width;
   uint32_t height;
   uint32_t depth;
   uint32_t samples;
   latte::SQ_TEX_DIM dim;
   latte::SQ_DATA_FORMAT format;
   latte::SQ_NUM_FORMAT numFormat;
   latte::SQ_FORMAT_COMP formatComp;
   uint32_t degamma;
   bool isDepthBuffer;
   latte::SQ_TILE_MODE tileMode;
   uint32_t swizzle;
};

struct SurfaceObject
{
   SurfaceDesc desc;
   DataHash dataHash;
   vk::Image image;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
   vk::ImageLayout activeLayout;
};

struct TextureDesc
{
   SurfaceDesc surfaceDesc;
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
   std::vector<SurfaceObject*> colorSurfaces;
   SurfaceObject *depthSurface;
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
   VertexShaderObject *vertexShader;
   GeometryShaderObject *geometryShader;
   PixelShaderObject *pixelShader;

   std::array<uint32_t, latte::MaxAttribBuffers> attribBufferStride;
   latte::VGT_DI_PRIMITIVE_TYPE primitiveType;
   bool primitiveResetEnabled;
   uint32_t primitiveResetIndex;
   bool dx9Consts;

   bool zEnable;
   bool zWriteEnable;
   latte::REF_FUNC zFunc;
   bool rasteriserDisable;
   latte::PA_FACE paFace;
   bool cullFront;
   bool cullBack;
   uint32_t polyPType;
   bool polyBiasEnabled;
   float polyBiasClamp;
   float polyBiasOffset;
   float polyBiasScale;

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
   void checkCurrentViewportAndScissor();
   void bindViewportAndScissor();

   // Samplers
   SamplerDesc getSamplerDesc(ShaderStage shaderStage, uint32_t samplerIdx);
   void checkCurrentSampler(ShaderStage shaderStage, uint32_t samplerIdx);
   void checkCurrentSamplers();

   // Textures
   TextureDesc getTextureDesc(ShaderStage shaderStage, uint32_t textureIdx);
   void checkCurrentTexture(ShaderStage shaderStage, uint32_t textureIdx);
   void checkCurrentTextures();

   // CBuffers
   void checkCurrentUniformBuffer(ShaderStage shaderStage, uint32_t cbufferIdx);
   void checkCurrentGprBuffer(ShaderStage shaderStage);
   void checkCurrentShaderBuffers();

   // Staging
   StagingBuffer * getStagingBuffer(uint32_t size, vk::BufferUsageFlags usage = vk::BufferUsageFlagBits::eTransferSrc);
   void retireStagingBuffer(StagingBuffer *sbuffer);
   void * mapStagingBuffer(StagingBuffer *sbuffer, bool flushGpu);
   void unmapStagingBuffer(StagingBuffer *sbuffer, bool flushCpu);

   // Data Buffers
   DataBufferObject * allocateDataBuffer(phys_addr baseAddress, uint32_t size);
   void checkDataBuffer(DataBufferObject * dataBuffer);
   DataBufferObject * getDataBuffer(phys_addr baseAddress, uint32_t size, bool discardData = false);

   // Surfaces
   SurfaceObject * getSurface(const SurfaceDesc& info, bool discardData);
   void transitionSurface(SurfaceObject *surface, vk::ImageLayout newLayout);
   SurfaceObject * allocateSurface(const SurfaceDesc& info);
   void releaseSurface(SurfaceObject *surface);
   void uploadSurface(SurfaceObject *surface);
   void downloadSurface(SurfaceObject *surface);

   // Vertex Buffers
   VertexBufferDesc getAttribBufferDesc(uint32_t bufferIndex);
   void checkCurrentAttribBuffers();
   void bindAttribBuffers();

   // Indices
   void maybeSwapIndices();
   void maybeUnpackQuads();
   void checkCurrentIndices();
   void bindIndexBuffer();

   // Draws
   void bindShaderResources();
   void drawGenericIndexed(uint32_t numIndices, void *indices);

   // Framebuffers
   FramebufferDesc getFramebufferDesc();
   void checkCurrentFramebuffer();
   SurfaceObject * getColorBuffer(const ColorBufferDesc& info, bool discardData);
   SurfaceObject * getDepthStencilBuffer(const DepthStencilBufferDesc& info, bool discardData);

   // Swap Chains
   SwapChainObject * allocateSwapChain(const SwapChainDesc &desc);
   void releaseSwapChain(SwapChainObject *swapChain);

   // Shaders
   spirv::VertexShaderDesc getVertexShaderDesc();
   spirv::GeometryShaderDesc getGeometryShaderDesc();
   spirv::PixelShaderDesc getPixelShaderDesc();
   void checkCurrentVertexShader();
   void checkCurrentGeometryShader();
   void checkCurrentPixelShader();
   void checkCurrentShaders();

   // Render Passes
   RenderPassDesc getRenderPassDesc();
   void checkCurrentRenderPass();

   // Pipelines
   PipelineDesc getPipelineDesc();
   void checkCurrentPipeline();

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

   SyncWaiter *mActiveSyncWaiter;
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
   std::array<std::array<SurfaceObject*, latte::MaxTextures>, 3> mCurrentTextures = { { nullptr } };
   std::array<std::array<StagingBuffer*, latte::MaxUniformBlocks>, 3> mCurrentUniformBlocks = { { nullptr } };

   std::vector<uint8_t> mScratchIdxSwap;
   std::vector<uint8_t> mScratchIdxDequad;

   using duration_system_clock = std::chrono::duration<double, std::chrono::system_clock::period>;
   using duration_ms = std::chrono::duration<double, std::chrono::milliseconds::period>;
   std::chrono::time_point<std::chrono::system_clock> mLastSwap;
   duration_system_clock mAverageFrameTime;

   vk::PhysicalDevice mPhysDevice;
   vk::Device mDevice;
   vk::Queue mQueue;
   vk::CommandPool mCommandPool;
   SwapChainObject *mTvSwapChain;
   SwapChainObject *mDrcSwapChain;
   RenderPassObject *mRenderPass;
   std::unordered_map<uint64_t, SurfaceObject*> mSurfaces;
   std::unordered_map<DataHash, VertexShaderObject*> mVertexShaders;
   std::unordered_map<DataHash, GeometryShaderObject*> mGeometryShaders;
   std::unordered_map<DataHash, FramebufferObject*> mFramebuffers;
   std::unordered_map<DataHash, PixelShaderObject*> mPixelShaders;
   std::unordered_map<DataHash, RenderPassObject*> mRenderPasses;
   std::unordered_map<DataHash, PipelineObject*> mPipelines;
   std::unordered_map<DataHash, SamplerObject*> mSamplers;
   std::map<std::pair<phys_addr, uint32_t>, DataBufferObject*> mDataBuffers;
};

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
