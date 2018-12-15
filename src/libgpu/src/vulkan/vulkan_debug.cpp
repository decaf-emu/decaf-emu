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
   mDebuggerInfo.numSurfaces = mSurfaceGroups.size();
   mDebuggerInfo.numDataBuffers = mMemCaches.size();
}

template <typename ObjType>
static void
_setVkObjectName(vk::Device device, ObjType object, vk::DebugReportObjectTypeEXT type, const char *name, const vk::DispatchLoaderDynamic& dispatch)
{
   if (dispatch.vkDebugMarkerSetObjectNameEXT) {
      vk::DebugMarkerObjectNameInfoEXT nameInfo;
      nameInfo.object = *reinterpret_cast<uint64_t*>(&object);
      nameInfo.objectType = type;
      nameInfo.pObjectName = name;
      device.debugMarkerSetObjectNameEXT(nameInfo, dispatch);
   }
}

void
Driver::setVkObjectName(VkBuffer object, const char *name)
{
   _setVkObjectName(mDevice, object, vk::DebugReportObjectTypeEXT::eBuffer, name, mVkDynLoader);
}

void
Driver::setVkObjectName(VkSampler object, const char *name)
{
   _setVkObjectName(mDevice, object, vk::DebugReportObjectTypeEXT::eSampler, name, mVkDynLoader);
}

void
Driver::setVkObjectName(VkImage object, const char *name)
{
   _setVkObjectName(mDevice, object, vk::DebugReportObjectTypeEXT::eImage, name, mVkDynLoader);
}

void
Driver::setVkObjectName(VkImageView object, const char *name)
{
   _setVkObjectName(mDevice, object, vk::DebugReportObjectTypeEXT::eImageView, name, mVkDynLoader);
}

void
Driver::setVkObjectName(VkShaderModule object, const char *name)
{
   _setVkObjectName(mDevice, object, vk::DebugReportObjectTypeEXT::eShaderModule, name, mVkDynLoader);
}

} // namespace vulkan

#endif // ifdef DECAF_VULKAN
