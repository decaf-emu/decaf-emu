#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"
#include "gpu_ringbuffer.h"
#include "gpu_vulkandriver.h"
#include "pm4_processor.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vulkan/vulkan.hpp>

namespace vulkan
{

struct StagingBuffer
{
   uint32_t size;
   vk::Buffer buffer;
   vk::DeviceMemory memory;
};

struct SyncWaiter
{
   bool isCompleted = false;
   vk::Fence fence;
   std::vector<StagingBuffer *> stagingBuffers;
   std::vector<std::function<void()>> callbacks;

   vk::CommandBuffer cmdBuffer;
};

struct SurfaceInfo
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
};

struct SurfaceObject
{
   SurfaceInfo info;
   vk::Image image;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
   vk::ImageLayout activeLayout;
};

struct SwapChainInfo
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
   SwapChainInfo info;
   vk::Image image;
   vk::ImageView imageView;
   vk::ImageSubresourceRange subresRange;
   SurfaceObject *_surface;
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

   SyncWaiter * allocateSyncWaiter();
   void releaseSyncWaiter(SyncWaiter *syncWaiter);
   void submitSyncWaiter(SyncWaiter *syncWaiter);
   void executeSyncWaiter(SyncWaiter *syncWaiter);
   void fenceWaiterThread();
   void checkSyncFences();
   void addRetireTask(std::function<void()> fn);

   void executeBuffer(const gpu::ringbuffer::Item &item);
   int32_t findMemoryType(uint32_t memoryTypeBits, vk::MemoryPropertyFlags props);

   StagingBuffer * getStagingBuffer(uint32_t size);
   void retireStagingBuffer(StagingBuffer *sbuffer);
   void * mapStagingBuffer(StagingBuffer *sbuffer, bool flushGpu);
   void unmapStagingBuffer(StagingBuffer *sbuffer, bool flushCpu);

   SurfaceObject * getSurface(const SurfaceInfo& info, bool discardData);
   void transitionSurface(SurfaceObject *surface, vk::ImageLayout newLayout);
   SurfaceObject * allocateSurface(const SurfaceInfo& info);
   void releaseSurface(SurfaceObject *surface);
   void uploadSurface(SurfaceObject *surface);
   void downloadSurface(SurfaceObject *surface);

   SurfaceObject *
   getColorBuffer(latte::CB_COLORN_BASE cb_color_base,
                  latte::CB_COLORN_SIZE cb_color_size,
                  latte::CB_COLORN_INFO cb_color_info,
                  bool discardData);

   SurfaceObject *
   getDepthBuffer(latte::DB_DEPTH_BASE db_depth_base,
                  latte::DB_DEPTH_SIZE db_depth_size,
                  latte::DB_DEPTH_INFO db_depth_info,
                  bool discardData);

   SwapChainObject * allocateSwapChain(const SwapChainInfo &info);
   void releaseSwapChain(SwapChainObject *swapChain);

private:
   virtual void decafSetBuffer(const latte::pm4::DecafSetBuffer &data) override;
   virtual void decafCopyColorToScan(const latte::pm4::DecafCopyColorToScan &data) override;
   virtual void decafSwapBuffers(const latte::pm4::DecafSwapBuffers &data) override;
   virtual void decafCapSyncRegisters(const latte::pm4::DecafCapSyncRegisters &data) override;
   virtual void decafClearColor(const latte::pm4::DecafClearColor &data) override;
   virtual void decafClearDepthStencil(const latte::pm4::DecafClearDepthStencil &data) override;
   virtual void decafDebugMarker(const latte::pm4::DecafDebugMarker &data) override;
   virtual void decafOSScreenFlip(const latte::pm4::DecafOSScreenFlip &data) override;
   virtual void decafCopySurface(const latte::pm4::DecafCopySurface &data) override;
   virtual void decafSetSwapInterval(const latte::pm4::DecafSetSwapInterval &data) override;
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
   SyncWaiter *mActiveSyncWaiter;
   vk::CommandBuffer mActiveCommandBuffer;

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
   std::unordered_map<uint64_t, SurfaceObject*> mSurfaces;
};

} // namespace vulkan

#endif // DECAF_VULKAN
