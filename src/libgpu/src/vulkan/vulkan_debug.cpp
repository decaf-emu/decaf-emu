#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

float
Driver::getAverageFPS()
{
   // TODO: This is not thread safe...
   static const auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds { 1 }).count();
   auto avgFrameTime = mAverageFrameTime.count();

   if (avgFrameTime == 0.0) {
      return 0.0f;
   } else {
      return static_cast<float>(second / avgFrameTime);
   }
}

float
Driver::getAverageFrametimeMS()
{
   return static_cast<float>(std::chrono::duration_cast<duration_ms>(mAverageFrameTime).count());
}

gpu::VulkanDriver::DebuggerInfo *
Driver::getDebuggerInfo()
{
   return &mDebuggerInfo;
}

void
Driver::updateDebuggerInfo()
{
   mDebuggerInfo.numVertexShaders = mVertexShaders.size();
   mDebuggerInfo.numGeometryShaders = mVertexShaders.size();
   mDebuggerInfo.numPixelShaders = mPixelShaders.size();
   mDebuggerInfo.numRenderPasses = mRenderPasses.size();
   mDebuggerInfo.numPipelines = mPipelines.size();
   mDebuggerInfo.numSamplers = mSamplers.size();
   mDebuggerInfo.numSurfaces = mSurfaces.size();
   mDebuggerInfo.numDataBuffers = mDataBuffers.size();
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
