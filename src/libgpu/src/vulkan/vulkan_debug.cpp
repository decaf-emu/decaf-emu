#ifdef DECAF_VULKAN
#include "vulkan_driver.h"

namespace vulkan
{

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::Vulkan;
}

gpu::GraphicsDriverDebugInfo *
Driver::getDebugInfo()
{
   // TODO: This is not thread safe wrt updateDebuggerInfo, maybe it should
   // be some sort of double buffered thing with a std atomic pointer to the
   // latest filled out one
   return &mDebugInfo;
}

void
Driver::updateDebuggerInfo()
{
   auto averageFrameTime = std::chrono::duration_cast<duration_ms>(mAverageFrameTime).count();
   mDebugInfo.averageFrameTimeMS = averageFrameTime;

   if (averageFrameTime > 0.0) {
      static constexpr auto second = std::chrono::duration_cast<duration_system_clock>(std::chrono::seconds{ 1 }).count();
      mDebugInfo.averageFps = second / averageFrameTime;
   } else {
      mDebugInfo.averageFps = 0.0;
   }

   mDebugInfo.numVertexShaders = mVertexShaders.size();
   mDebugInfo.numGeometryShaders = mVertexShaders.size();
   mDebugInfo.numPixelShaders = mPixelShaders.size();
   mDebugInfo.numRenderPasses = mRenderPasses.size();
   mDebugInfo.numPipelines = mPipelines.size();
   mDebugInfo.numSamplers = mSamplers.size();
   mDebugInfo.numSurfaces = mSurfaceGroups.size();
   mDebugInfo.numDataBuffers = mMemCaches.size();
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
