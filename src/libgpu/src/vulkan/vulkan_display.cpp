#ifdef DECAF_VULKAN
#include "vulkan_driver.h"
#include "vulkan_displayshaders.h"

#include "gpu_config.h"
#include "gpu7_displaylayout.h"


#include <common/log.h>
#include <common/platform_debug.h>
#include <common/strutils.h>
#include <cstring>
#include <fmt/format.h>
#include <optional>
#include <string_view>
#include <tuple>

namespace vulkan
{

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugReportFlagsEXT flags,
                     VkDebugReportObjectTypeEXT objectType,
                     uint64_t object,
                     size_t location,
                     int32_t messageCode,
                     const char* pLayerPrefix,
                     const char* pMessage,
                     void *pUserData)
{
   // Consider doing additional debugger behaviours based on various attributes.
   // This is to improve the chances that we don't accidentally miss incorrect
   // Vulkan-specific behaviours.

   // We keep track of known issues so we can log slightly differently, and also
   // avoid breaking to the debugger here.
   bool isKnownIssue = false;

   // There is currently a bug where the validation layers report issues with using
   // VkPipelineColorBlendStateCreateInfo in spite of our legal usage of it.
   // TODO: Remove this once validation correctly supports VkPipelineColorBlendAdvancedStateCreateInfoEXT
   if (strstr(pMessage, "VkPipelineColorBlendStateCreateInfo-pNext") != nullptr) {
      static uint64_t seenAdvancedBlendWarning = 0;
      if (seenAdvancedBlendWarning++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }

   // We intentionally mirror the behaviour of GPU7 where a shader writes to an attachement which is not bound.
   // The validation layer gives us a warning, but we should ignore it for this known case.
   if (strstr(pMessage, "Shader-OutputNotConsumed") != nullptr) {
      static uint64_t seenOutputNotConsumed = 0;
      if (seenOutputNotConsumed++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }

   // Some games rebind the same texture as an input and output at the same time.  This
   // is technically illegal, even for GPU7, but it works... so...
   if (strstr(pMessage, "VkDescriptorImageInfo-imageLayout") != nullptr) {
      static uint64_t seenImageLayoutError = 0;
      if (seenImageLayoutError++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }
   if (strstr(pMessage, "DrawState-DescriptorSetNotUpdated") != nullptr) {
      static uint64_t seenDescriptorSetError = 0;
      if (seenDescriptorSetError++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }

   // There is an issue with the validation layers and handling of transform feedback.
   if (strstr(pMessage, "VUID-vkCmdPipelineBarrier-pMemoryBarriers-01184") != nullptr) {
      static uint64_t seenXfbBarrier01184Error = 0;
      if (seenXfbBarrier01184Error++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }
   if (strstr(pMessage, "VUID-vkCmdPipelineBarrier-pMemoryBarriers-01185") != nullptr) {
      static uint64_t seenXfbBarrier01185Error = 0;
      if (seenXfbBarrier01185Error++) {
         return VK_FALSE;
      }
      isKnownIssue = true;
   }

   // Write this message to our normal logging
   if (!isKnownIssue) {
      gLog->warn("Vulkan Debug Report: {}, {}, {}, {}, {}, {}, {}",
                 vk::to_string(static_cast<vk::DebugReportFlagsEXT>(flags)),
                 vk::to_string(static_cast<vk::DebugReportObjectTypeEXT>(objectType)),
                 object,
                 location,
                 messageCode,
                 pLayerPrefix,
                 pMessage);
   } else {
      gLog->warn("Vulkan Debug Report (Known Case): {}", pMessage);
   }

   if (!isKnownIssue) {
      platform::debugLog(fmt::format("vk-dbg: {}\n", pMessage));
   } else {
      platform::debugLog(fmt::format("vk-dbg-ignored: {}\n", pMessage));
   }

   // We should break to the debugger on unexpected situations.
   if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT || flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
      if (!isKnownIssue) {
         platform::debugBreak();
      }
   }

   return VK_FALSE;
}

static void
registerDebugCallback(vk::Instance &instance,
                      vk::DispatchLoaderDynamic &dispatchLoaderDynamic,
                      void *pUserData)
{
   if (!dispatchLoaderDynamic.vkCreateDebugReportCallbackEXT) {
      return;
   }

   auto dbgReportDesc = vk::DebugReportCallbackCreateInfoEXT { };
   dbgReportDesc.flags =
      vk::DebugReportFlagBitsEXT::eDebug |
      vk::DebugReportFlagBitsEXT::eWarning |
      vk::DebugReportFlagBitsEXT::eError |
      vk::DebugReportFlagBitsEXT::ePerformanceWarning;
   dbgReportDesc.pfnCallback = debugMessageCallback;
   dbgReportDesc.pUserData = pUserData;
   instance.createDebugReportCallbackEXT(dbgReportDesc, nullptr, dispatchLoaderDynamic);
}

static bool
getWindowSystemExtensions(gpu::WindowSystemType wsiType, std::vector<const char*> &extensions)
{
   /*
    VK_USE_PLATFORM_ANDROID_KHR - Android
    VK_USE_PLATFORM_MIR_KHR - Mir
    */
   switch (wsiType)
   {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
   case gpu::WindowSystemType::Windows:
      extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
      break;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
   case gpu::WindowSystemType::X11:
      extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      extensions.push_back(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
      break;
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
   case gpu::WindowSystemType::Xcb:
      extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
      break;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
   case gpu::WindowSystemType::Wayland:
      extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
      break;
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
   case gpu::WindowSystemType::Cocoa:
      extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
      extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
      break;
#endif
   default:
      return false;
   }

   return true;
}

static vk::Instance
createVulkanInstance(const gpu::WindowSystemInfo &wsi)
{
   auto appInfo =
      vk::ApplicationInfo {
         "Decaf",
         VK_MAKE_VERSION(1, 0, 0),
         "DecafSDL",
         VK_MAKE_VERSION(1, 0, 0),
         VK_API_VERSION_1_0
      };

   std::vector<const char*> layers =
   {
   };

   std::vector<const char*> extensions =
   {
       VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
   };

   if (gpu::config()->debug.debug_enabled) {
      layers.push_back("VK_LAYER_KHRONOS_validation");
      extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
   }

   if (!getWindowSystemExtensions(wsi.type, extensions)) {
      gLog->error("createVulkanInstance: Failed to get window system extensions for type {}", wsi.type);
      return { };
   }

   if (!extensions.empty()) {
      fmt::memory_buffer msg;
      fmt::format_to(msg, "Creating instance with extensions:");
      for (auto ext : extensions) {
         fmt::format_to(msg, " {}", ext);
      }
      gLog->debug({ msg.data(), msg.size() });
   }

   if (!layers.empty()) {
      fmt::memory_buffer msg;
      fmt::format_to(msg, "Creating instance with layers:");
      for (auto layer : layers) {
         fmt::format_to(msg, " {}", layer);
      }
      gLog->debug({ msg.data(), msg.size() });
   }

   auto instanceCreateInfo = vk::InstanceCreateInfo { };
   instanceCreateInfo.pApplicationInfo = &appInfo;
   instanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
   instanceCreateInfo.ppEnabledLayerNames = layers.data();
   instanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
   instanceCreateInfo.ppEnabledExtensionNames = extensions.data();
   return vk::createInstance(instanceCreateInfo);
}

static vk::PhysicalDevice
choosePhysicalDevice(vk::Instance &instance)
{
   auto physicalDevices = instance.enumeratePhysicalDevices();
   return physicalDevices[0];
}

static vk::SurfaceKHR
createVulkanSurface(vk::Instance &instance, const gpu::WindowSystemInfo &wsi)
{
#if defined(VK_USE_PLATFORM_WIN32_KHR)
   if (wsi.type == gpu::WindowSystemType::Windows) {
      auto surfaceCreateInfo = vk::Win32SurfaceCreateInfoKHR { };
      surfaceCreateInfo.hinstance = nullptr;
      surfaceCreateInfo.hwnd = reinterpret_cast<HWND>(wsi.renderSurface);
      return instance.createWin32SurfaceKHR(surfaceCreateInfo);
   }
#endif

#if defined(VK_USE_PLATFORM_XCB_KHR)
   if (wsi.type == gpu::WindowSystemType::Xcb) {
      auto surfaceCreateInfo = vk::XcbSurfaceCreateInfoKHR { };
      surfaceCreateInfo.connection = static_cast<xcb_connection_t *>(wsi.displayConnection);
      surfaceCreateInfo.window = static_cast<xcb_window_t>(reinterpret_cast<uintptr_t>(wsi.renderSurface));
      return instance.createXcbSurfaceKHR(surfaceCreateInfo);
   }
#endif

#if defined(VK_USE_PLATFORM_XLIB_KHR)
   if (wsi.type == gpu::WindowSystemType::X11) {
      auto surfaceCreateInfo = vk::XlibSurfaceCreateInfoKHR { };
      surfaceCreateInfo.dpy = static_cast<Display *>(wsi.displayConnection);
      surfaceCreateInfo.window = reinterpret_cast<Window>(wsi.renderSurface);
      return instance.createXlibSurfaceKHR(surfaceCreateInfo);
   }
#endif

#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
   if (wsi.type == gpu::WindowSystemType::Wayland) {
      auto surfaceCreateInfo = vk::WaylandSurfaceCreateInfoKHR { };
      surfaceCreateInfo.display = static_cast<wl_display *>(wsi.displayConnection);
      surfaceCreateInfo.surface = static_cast<wl_surface *>(wsi.renderSurface);
      return instance.createWaylandSurfaceKHR(surfaceCreateInfo);
   }
#endif

#if defined(VK_USE_PLATFORM_MACOS_MVK)
   if (wsi.type == gpu::WindowSystemType::Cocoa) {
      auto surfaceCreateInfo = vk::MacOSSurfaceCreateInfoMVK { };
      surfaceCreateInfo.pView = static_cast<const void *>(wsi.renderSurface);
      return instance.createMacOSSurfaceMVK(surfaceCreateInfo);
   }
#endif

   return { };
}

static vk::Format
chooseSurfaceFormat(vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR &surface)
{
   auto formats = physicalDevice.getSurfaceFormatsKHR(surface);
   auto selected = formats[0];
   for (auto &format : formats) {
      switch (format.format) {
      case vk::Format::eR8G8B8A8Srgb:
      case vk::Format::eB8G8R8A8Srgb:
         selected = format;
         break;
      }
   }

   return selected.format;
}

static std::tuple<vk::Device, uint32_t, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceTransformFeedbackFeaturesEXT>
createDevice(vk::PhysicalDevice &physicalDevice, vk::SurfaceKHR &surface)
{
   std::vector<const char*> deviceLayers =
   {
   };

   std::vector<const char *> requiredExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME,
      VK_EXT_VERTEX_ATTRIBUTE_DIVISOR_EXTENSION_NAME,
      VK_KHR_MAINTENANCE1_EXTENSION_NAME,
      VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME,
      VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME,
   };
   std::vector<const char *> missingRequiredExtensions = {};

   std::vector<const char *> optionalExtensions = {
      VK_EXT_DEPTH_RANGE_UNRESTRICTED_EXTENSION_NAME,
      VK_EXT_TRANSFORM_FEEDBACK_EXTENSION_NAME,
   };
   std::vector<const char *> missingOptionalExtensions = {};

   std::vector<const char *> deviceExtensions = requiredExtensions;

   if (gpu::config()->debug.debug_enabled) {
      deviceLayers.push_back("VK_LAYER_KHRONOS_validation");
      optionalExtensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
   }

   auto supportedDeviceExtensions = physicalDevice.enumerateDeviceExtensionProperties();
   for (auto &name : requiredExtensions) {
      auto hasExtension = false;
      for (auto &ext : supportedDeviceExtensions) {
         if (iequals(name, ext.extensionName.data())) {
            hasExtension = true;
            break;
         }
      }

      if (!hasExtension) {
         missingRequiredExtensions.push_back(name);
      }
   }

   for (auto name : optionalExtensions) {
      auto hasExtension = false;
      for (auto &ext : supportedDeviceExtensions) {
         if (iequals(name, ext.extensionName.data())) {
            hasExtension = true;
            break;
         }
      }

      if (hasExtension) {
         deviceExtensions.push_back(name);
      } else {
         missingOptionalExtensions.push_back(name);
      }
   }

   if (!missingRequiredExtensions.empty() || !missingOptionalExtensions.empty()) {
      fmt::memory_buffer msg;
      fmt::format_to(msg, "Not all Vulkan {} extensions supported:\n", !missingRequiredExtensions.empty() ? "optional" : "required");
      fmt::format_to(msg, "  Required:\n");
      for (auto name : requiredExtensions) {
         fmt::format_to(msg, "  - {}: {}\n",
            name,
            std::find(missingRequiredExtensions.begin(), missingRequiredExtensions.end(), name) == missingRequiredExtensions.end());
      }

      fmt::format_to(msg, "  Optional:\n");
      for (auto name : optionalExtensions) {
         fmt::format_to(msg, "  - {}: {}\n",
            name,
            std::find(missingOptionalExtensions.begin(), missingOptionalExtensions.end(), name) == missingOptionalExtensions.end());
      }

      if (!missingRequiredExtensions.empty()) {
         gLog->error(std::string_view{ msg.data(), msg.size() });
      } else {
         gLog->warn(std::string_view{ msg.data(), msg.size() });
      }
   }

   auto features = physicalDevice.getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceTransformFeedbackFeaturesEXT>();
   auto supportedFeatures = features.get<vk::PhysicalDeviceFeatures2>();
   auto supportedFeaturesTransformFeedback = features.get<vk::PhysicalDeviceTransformFeedbackFeaturesEXT>();

   auto hasRequiredFeatures =
      supportedFeatures.features.depthClamp &&
      supportedFeatures.features.textureCompressionBC &&
      supportedFeatures.features.independentBlend &&
      supportedFeatures.features.fillModeNonSolid &&
      supportedFeatures.features.samplerAnisotropy;

   auto hasOptionalFeatures =
      supportedFeatures.features.geometryShader &&
      supportedFeatures.features.wideLines &&
      supportedFeatures.features.logicOp &&
      supportedFeaturesTransformFeedback.transformFeedback &&
      supportedFeaturesTransformFeedback.geometryStreams;

   if (!hasRequiredFeatures || !hasOptionalFeatures) {
      fmt::memory_buffer msg;
      fmt::format_to(msg, "Not all Vulkan {} features supported:\n", hasRequiredFeatures ? "optional" : "required");
      fmt::format_to(msg, "  Required:\n");
      fmt::format_to(msg, "  - depthClamp: {}\n", supportedFeatures.features.depthClamp);
      fmt::format_to(msg, "  - textureCompressionBC: {}\n", supportedFeatures.features.textureCompressionBC);
      fmt::format_to(msg, "  - independentBlend: {}\n", supportedFeatures.features.independentBlend);
      fmt::format_to(msg, "  - fillModeNonSolid: {}\n", supportedFeatures.features.fillModeNonSolid);
      fmt::format_to(msg, "  - samplerAnisotropy: {}\n", supportedFeatures.features.samplerAnisotropy);
      fmt::format_to(msg, "  Optional:\n");
      fmt::format_to(msg, "  - geometryShader: {}\n", supportedFeatures.features.geometryShader);
      fmt::format_to(msg, "  - wideLines: {}\n", supportedFeatures.features.wideLines);
      fmt::format_to(msg, "  - logicOp: {}\n", supportedFeatures.features.logicOp);
      fmt::format_to(msg, "  - transformFeedback: {}\n", supportedFeaturesTransformFeedback.transformFeedback);
      fmt::format_to(msg, "  - geometryStreams: {}\n", supportedFeaturesTransformFeedback.geometryStreams);

      if (!hasRequiredFeatures) {
         gLog->error(std::string_view { msg.data(), msg.size() });
      } else {
         gLog->warn(std::string_view{ msg.data(), msg.size() });
      }
   }

   if (!missingRequiredExtensions.empty() || !hasRequiredFeatures) {
      return {};
   }

   auto queueFamilyProps = physicalDevice.getQueueFamilyProperties();
   auto queueFamilyIndex = uint32_t { 0 };
   for (; queueFamilyIndex < queueFamilyProps.size(); ++queueFamilyIndex) {
      auto &qfp = queueFamilyProps[queueFamilyIndex];

      if (!physicalDevice.getSurfaceSupportKHR(queueFamilyIndex, surface)) {
         continue;
      }

      if (!(qfp.queueFlags & (vk::QueueFlagBits::eGraphics |
                              vk::QueueFlagBits::eTransfer |
                              vk::QueueFlagBits::eCompute))) {
         continue;
      }

      break;
   }

   if (queueFamilyIndex >= queueFamilyProps.size()) {
      gLog->error("Could not find compatible vulkan queue family");
      return { };
   }

   auto queuePriorities = std::array<float, 1> { 0.0f };
   auto deviceQueueCreateInfo =
      vk::DeviceQueueCreateInfo {
         vk::DeviceQueueCreateFlags { },
         queueFamilyIndex,
         static_cast<uint32_t>(queuePriorities.size()),
         queuePriorities.data()
      };

   auto createDeviceChain = vk::StructureChain<vk::DeviceCreateInfo, vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceTransformFeedbackFeaturesEXT> { };
   auto &deviceCreateInfo = createDeviceChain.get<vk::DeviceCreateInfo>();
   deviceCreateInfo.queueCreateInfoCount = 1;
   deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
   deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
   deviceCreateInfo.ppEnabledLayerNames = deviceLayers.data();
   deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
   deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

   auto &deviceCreateFeatures = createDeviceChain.get<vk::PhysicalDeviceFeatures2>();
   deviceCreateFeatures.features.depthClamp = true;
   deviceCreateFeatures.features.textureCompressionBC = true;
   deviceCreateFeatures.features.independentBlend = true;
   deviceCreateFeatures.features.fillModeNonSolid = true;
   deviceCreateFeatures.features.samplerAnisotropy = true;
   deviceCreateFeatures.features.geometryShader = supportedFeatures.features.geometryShader;
   deviceCreateFeatures.features.wideLines = supportedFeatures.features.wideLines;
   deviceCreateFeatures.features.logicOp = supportedFeatures.features.logicOp;

   auto &deviceCreateFeaturesTransformFeedback = createDeviceChain.get<vk::PhysicalDeviceTransformFeedbackFeaturesEXT>();
   deviceCreateFeaturesTransformFeedback.transformFeedback = supportedFeaturesTransformFeedback.transformFeedback;
   deviceCreateFeaturesTransformFeedback.geometryStreams = supportedFeaturesTransformFeedback.geometryStreams;

   auto device = physicalDevice.createDevice(deviceCreateInfo);
   return { device, queueFamilyIndex, supportedFeatures, supportedFeaturesTransformFeedback };
}

static bool
createRenderPass(VulkanDisplayPipeline &displayPipeline,
                 vk::Device &device)
{
   // Create our render pass that targets this attachement
   auto colorAttachmentDesc = vk::AttachmentDescription { };
   colorAttachmentDesc.format = displayPipeline.windowSurfaceFormat;
   colorAttachmentDesc.samples = vk::SampleCountFlagBits::e1;
   colorAttachmentDesc.loadOp = vk::AttachmentLoadOp::eClear;
   colorAttachmentDesc.storeOp = vk::AttachmentStoreOp::eStore;
   colorAttachmentDesc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
   colorAttachmentDesc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
   colorAttachmentDesc.initialLayout = vk::ImageLayout::eUndefined;
   colorAttachmentDesc.finalLayout = vk::ImageLayout::ePresentSrcKHR;

   auto colorAttachmentRef = vk::AttachmentReference { };
   colorAttachmentRef.attachment = 0;
   colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

   auto genericSubpass = vk::SubpassDescription { };
   genericSubpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
   genericSubpass.inputAttachmentCount = 0;
   genericSubpass.pInputAttachments = nullptr;
   genericSubpass.colorAttachmentCount = 1;
   genericSubpass.pColorAttachments = &colorAttachmentRef;
   genericSubpass.pResolveAttachments = 0;
   genericSubpass.pDepthStencilAttachment = nullptr;
   genericSubpass.preserveAttachmentCount = 0;
   genericSubpass.pPreserveAttachments = nullptr;

   auto renderPassDesc = vk::RenderPassCreateInfo { };
   renderPassDesc.attachmentCount = 1;
   renderPassDesc.pAttachments = &colorAttachmentDesc;
   renderPassDesc.subpassCount = 1;
   renderPassDesc.pSubpasses = &genericSubpass;
   renderPassDesc.dependencyCount = 0;
   renderPassDesc.pDependencies = nullptr;
   displayPipeline.renderPass = device.createRenderPass(renderPassDesc);
   return !!displayPipeline.renderPass;
}

static vk::PresentModeKHR
choosePresentMode(vk::PhysicalDevice &physicalDevice,
                  vk::SurfaceKHR &surface)
{
   auto presentModes = physicalDevice.getSurfacePresentModesKHR(surface);
   auto hasPresentMode = [&](vk::PresentModeKHR mode) {
      return std::find(presentModes.begin(), presentModes.end(), mode) != presentModes.end();
   };

   if (hasPresentMode(vk::PresentModeKHR::eMailbox)) {
      return vk::PresentModeKHR::eMailbox;
   }

   if (hasPresentMode(vk::PresentModeKHR::eImmediate)) {
      return vk::PresentModeKHR::eImmediate;
   }

   if (hasPresentMode(vk::PresentModeKHR::eFifo)) {
      return vk::PresentModeKHR::eFifo;
   }

   return presentModes[0];
}

static bool
createSwapchain(VulkanDisplayPipeline &displayPipeline,
                vk::PhysicalDevice &physicalDevice,
                vk::Device &device)
{
   auto surfaceCaps = physicalDevice.getSurfaceCapabilitiesKHR(displayPipeline.windowSurface);
   displayPipeline.swapchainExtents = surfaceCaps.currentExtent;

   // Create the swap chain itself
   auto swapchainCreateInfo = vk::SwapchainCreateInfoKHR { };
   swapchainCreateInfo.surface = displayPipeline.windowSurface;
   swapchainCreateInfo.minImageCount = surfaceCaps.minImageCount;
   swapchainCreateInfo.imageFormat = displayPipeline.windowSurfaceFormat;
   swapchainCreateInfo.imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear;
   swapchainCreateInfo.imageExtent = displayPipeline.swapchainExtents;
   swapchainCreateInfo.imageArrayLayers = 1;
   swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferSrc;
   swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
   swapchainCreateInfo.queueFamilyIndexCount = 0;
   swapchainCreateInfo.pQueueFamilyIndices = nullptr;
   swapchainCreateInfo.preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity;
   swapchainCreateInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
   swapchainCreateInfo.presentMode = displayPipeline.presentMode;
   swapchainCreateInfo.clipped = true;
   swapchainCreateInfo.oldSwapchain = nullptr;
   displayPipeline.swapchain = device.createSwapchainKHR(swapchainCreateInfo);

   // Create our framebuffers
   auto swapChainImages = device.getSwapchainImagesKHR(displayPipeline.swapchain);
   displayPipeline.swapchainImageViews.resize(swapChainImages.size());
   displayPipeline.framebuffers.resize(swapChainImages.size());

   for (auto i = 0u; i < swapChainImages.size(); ++i) {
      auto imageViewCreateInfo = vk::ImageViewCreateInfo { };
      imageViewCreateInfo.image = swapChainImages[i];
      imageViewCreateInfo.viewType = vk::ImageViewType::e2D;
      imageViewCreateInfo.format = displayPipeline.windowSurfaceFormat;
      imageViewCreateInfo.components = vk::ComponentMapping();
      imageViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
      imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
      imageViewCreateInfo.subresourceRange.levelCount = 1;
      imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
      imageViewCreateInfo.subresourceRange.layerCount = 1;
      displayPipeline.swapchainImageViews[i] = device.createImageView(imageViewCreateInfo);

      auto framebufferCreateInfo = vk::FramebufferCreateInfo { };
      framebufferCreateInfo.renderPass = displayPipeline.renderPass;
      framebufferCreateInfo.attachmentCount = 1;
      framebufferCreateInfo.pAttachments = &displayPipeline.swapchainImageViews[i];
      framebufferCreateInfo.width = displayPipeline.swapchainExtents.width;
      framebufferCreateInfo.height = displayPipeline.swapchainExtents.height;
      framebufferCreateInfo.layers = 1;
      displayPipeline.framebuffers[i] = device.createFramebuffer(framebufferCreateInfo);
   }

   return true;
}

static void
destroySwapchain(VulkanDisplayPipeline &displayPipeline,
                 vk::Device &device)
{
   for (const auto &framebuffer : displayPipeline.framebuffers) {
      device.destroyFramebuffer(framebuffer);
   }
   displayPipeline.framebuffers.clear();

   for (const auto &imageView : displayPipeline.swapchainImageViews) {
      device.destroyImageView(imageView);
   }
   displayPipeline.swapchainImageViews.clear();

   device.destroySwapchainKHR(displayPipeline.swapchain);
   displayPipeline.swapchain = vk::SwapchainKHR { };
}

static void
recreateSwapchain(VulkanDisplayPipeline &displayPipeline,
                  vk::PhysicalDevice &physicalDevice,
                  vk::Device &device)
{
   device.waitIdle();
   destroySwapchain(displayPipeline, device);
   createSwapchain(displayPipeline, physicalDevice, device);
}

static bool
createPipelineLayout(VulkanDisplayPipeline &displayPipeline,
                     vk::Device &device)
{
   auto samplerCreateInfo = vk::SamplerCreateInfo { };
   samplerCreateInfo.magFilter = vk::Filter::eLinear;
   samplerCreateInfo.minFilter = vk::Filter::eLinear;
   samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eNearest;
   samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
   samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
   samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
   samplerCreateInfo.mipLodBias = 0.0f;
   samplerCreateInfo.anisotropyEnable = false;
   samplerCreateInfo.maxAnisotropy = 0.0f;
   samplerCreateInfo.compareEnable = false;
   samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
   samplerCreateInfo.minLod = 0.0f;
   samplerCreateInfo.maxLod = 0.0f;
   samplerCreateInfo.borderColor = vk::BorderColor::eFloatTransparentBlack;
   samplerCreateInfo.unnormalizedCoordinates = false;
   displayPipeline.trivialSampler = device.createSampler(samplerCreateInfo);

   auto immutableSamplers = std::array<vk::Sampler, 1> {
      displayPipeline.trivialSampler,
   };

   auto bindings = std::array<vk::DescriptorSetLayoutBinding, 2> {
      vk::DescriptorSetLayoutBinding {
         0, vk::DescriptorType::eSampler,
         1, vk::ShaderStageFlagBits::eFragment,
         immutableSamplers.data()
      },
      vk::DescriptorSetLayoutBinding {
         1, vk::DescriptorType::eSampledImage,
         1, vk::ShaderStageFlagBits::eFragment,
         nullptr
      },
   };

   auto descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo { };
   descriptorSetLayoutCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
   descriptorSetLayoutCreateInfo.pBindings = bindings.data();
   displayPipeline.descriptorSetLayout = device.createDescriptorSetLayout(descriptorSetLayoutCreateInfo);

   auto layoutBindings = std::array<vk::DescriptorSetLayout, 1> { displayPipeline.descriptorSetLayout };
   auto pipelineLayoutCreateInfo = vk::PipelineLayoutCreateInfo { };
   pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(layoutBindings.size());
   pipelineLayoutCreateInfo.pSetLayouts = layoutBindings.data();
   pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
   pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;
   displayPipeline.pipelineLayout = device.createPipelineLayout(pipelineLayoutCreateInfo);
   return true;
}

static bool
createRenderPipeline(VulkanDisplayPipeline &displayPipeline,
                     vk::Device &device)
{
   auto scanbufferVertBytesSize = sizeof(scanbufferVertBytes) / sizeof(scanbufferVertBytes[0]);
   displayPipeline.vertexShader =
      device.createShaderModule(
         vk::ShaderModuleCreateInfo {
            {},
            scanbufferVertBytesSize,
            reinterpret_cast<const uint32_t*>(scanbufferVertBytes)
         });

   auto scanbufferFragBytesSize = sizeof(scanbufferFragBytes) / sizeof(scanbufferFragBytes[0]);
   displayPipeline.fragmentShader =
      device.createShaderModule(
         vk::ShaderModuleCreateInfo {
            {},
            scanbufferFragBytesSize,
            reinterpret_cast<const uint32_t*>(scanbufferFragBytes)
         });

   auto shaderStages = std::array<vk::PipelineShaderStageCreateInfo, 2> {
      vk::PipelineShaderStageCreateInfo {
         {},
         vk::ShaderStageFlagBits::eVertex,
         displayPipeline.vertexShader,
         "main",
         nullptr,
      },
      vk::PipelineShaderStageCreateInfo {
         {},
         vk::ShaderStageFlagBits::eFragment,
         displayPipeline.fragmentShader,
         "main",
         nullptr,
      },
   };

   auto vtxBindings = std::array<vk::VertexInputBindingDescription, 1> {
      vk::VertexInputBindingDescription { 0, 16, vk::VertexInputRate::eVertex },
   };

   auto vtxAttribs = std::array<vk::VertexInputAttributeDescription, 2> {
      vk::VertexInputAttributeDescription { 0, 0, vk::Format::eR32G32Sfloat, 0 },
      vk::VertexInputAttributeDescription { 1, 0, vk::Format::eR32G32Sfloat, 8 },
   };

   // Vertex input stage, we store all our vertices in the actual shaders
   auto vertexInputInfo = vk::PipelineVertexInputStateCreateInfo { };
   vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vtxBindings.size());
   vertexInputInfo.pVertexBindingDescriptions = vtxBindings.data();
   vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vtxAttribs.size());
   vertexInputInfo.pVertexAttributeDescriptions = vtxAttribs.data();

   auto inputAssembly = vk::PipelineInputAssemblyStateCreateInfo { };
   inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
   inputAssembly.primitiveRestartEnable = false;

   auto viewport = vk::Viewport {
      0.0f, 0.0f,
      static_cast<float>(displayPipeline.swapchainExtents.width),
      static_cast<float>(displayPipeline.swapchainExtents.height),
      0.0f, 0.0f,
   };
   auto scissor = vk::Rect2D { { 0,0 }, displayPipeline.swapchainExtents };
   auto viewportState = vk::PipelineViewportStateCreateInfo { };
   viewportState.viewportCount = 1;
   viewportState.pViewports = &viewport;
   viewportState.scissorCount = 1;
   viewportState.pScissors = &scissor;

   auto rasterizer = vk::PipelineRasterizationStateCreateInfo { };
   rasterizer.depthClampEnable = false;
   rasterizer.rasterizerDiscardEnable = false;
   rasterizer.polygonMode = vk::PolygonMode::eFill;
   rasterizer.cullMode = vk::CullModeFlagBits::eNone;
   rasterizer.frontFace = vk::FrontFace::eClockwise;
   rasterizer.depthBiasEnable = false;
   rasterizer.depthBiasConstantFactor = 0.0f;
   rasterizer.depthBiasClamp = 0.0f;
   rasterizer.depthBiasSlopeFactor = 0.0f;
   rasterizer.lineWidth = 1.0f;

   auto multisampling = vk::PipelineMultisampleStateCreateInfo { };
   multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
   multisampling.sampleShadingEnable = false;
   multisampling.minSampleShading = 1.0f;
   multisampling.pSampleMask = nullptr;
   multisampling.alphaToCoverageEnable = false;
   multisampling.alphaToOneEnable = false;

   auto colorBlendAttachement0 = vk::PipelineColorBlendAttachmentState { };
   colorBlendAttachement0.blendEnable = false;
   colorBlendAttachement0.srcColorBlendFactor = vk::BlendFactor::eOne;
   colorBlendAttachement0.dstColorBlendFactor = vk::BlendFactor::eZero;
   colorBlendAttachement0.colorBlendOp = vk::BlendOp::eAdd;
   colorBlendAttachement0.srcAlphaBlendFactor = vk::BlendFactor::eOne;
   colorBlendAttachement0.dstAlphaBlendFactor = vk::BlendFactor::eZero;
   colorBlendAttachement0.alphaBlendOp = vk::BlendOp::eAdd;
   colorBlendAttachement0.colorWriteMask =
      vk::ColorComponentFlagBits::eR |
      vk::ColorComponentFlagBits::eG |
      vk::ColorComponentFlagBits::eB |
      vk::ColorComponentFlagBits::eA;

   auto colorBlendAttachments = std::vector<vk::PipelineColorBlendAttachmentState> {
      colorBlendAttachement0
   };

   auto colorBlendState = vk::PipelineColorBlendStateCreateInfo { };
   colorBlendState.logicOpEnable = false;
   colorBlendState.logicOp = vk::LogicOp::eCopy;
   colorBlendState.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
   colorBlendState.pAttachments = colorBlendAttachments.data();
   colorBlendState.blendConstants[0] = 0.0f;
   colorBlendState.blendConstants[1] = 0.0f;
   colorBlendState.blendConstants[2] = 0.0f;
   colorBlendState.blendConstants[3] = 0.0f;

   auto dynamicStates = std::vector<vk::DynamicState> {
      vk::DynamicState::eViewport,
      vk::DynamicState::eScissor,
   };

   auto dynamicState = vk::PipelineDynamicStateCreateInfo { };
   dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
   dynamicState.pDynamicStates = dynamicStates.data();

   auto pipelineInfo = vk::GraphicsPipelineCreateInfo { };
   pipelineInfo.pStages = shaderStages.data();
   pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
   pipelineInfo.pVertexInputState = &vertexInputInfo;
   pipelineInfo.pInputAssemblyState = &inputAssembly;
   pipelineInfo.pTessellationState = nullptr;
   pipelineInfo.pViewportState = &viewportState;
   pipelineInfo.pRasterizationState = &rasterizer;
   pipelineInfo.pMultisampleState = &multisampling;
   pipelineInfo.pDepthStencilState = nullptr;
   pipelineInfo.pColorBlendState = &colorBlendState;
   pipelineInfo.pDynamicState = &dynamicState;
   pipelineInfo.layout = displayPipeline.pipelineLayout;
   pipelineInfo.renderPass = displayPipeline.renderPass;
   pipelineInfo.subpass = 0;
   pipelineInfo.basePipelineHandle = vk::Pipeline { };
   pipelineInfo.basePipelineIndex = -1;

   auto result = device.createGraphicsPipeline(vk::PipelineCache { },
                                               pipelineInfo);
   if (result.result != vk::Result::eSuccess) {
      return false;
   }

   displayPipeline.graphicsPipeline = result.value;
   return true;
}

static bool
createDescriptorPools(VulkanDisplayPipeline &displayPipeline,
                      vk::Device &device)
{
   auto descriptorPoolSizes = std::vector<vk::DescriptorPoolSize> {
      vk::DescriptorPoolSize { vk::DescriptorType::eSampler, 100 },
      vk::DescriptorPoolSize { vk::DescriptorType::eSampledImage, 100 },
      vk::DescriptorPoolSize { vk::DescriptorType::eCombinedImageSampler, 100 },
   };

   auto createInfo = vk::DescriptorPoolCreateInfo { };
   createInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());
   createInfo.pPoolSizes = descriptorPoolSizes.data();
   createInfo.maxSets = static_cast<uint32_t>(descriptorPoolSizes.size() * 100);
   displayPipeline.descriptorPool = device.createDescriptorPool(createInfo);

   return true;
}

static std::optional<uint32_t>
chooseMemoryTypeIndex(vk::PhysicalDevice &physicalDevice,
                      uint32_t typeFilter,
                      vk::MemoryPropertyFlags propertyFlags)
{
   auto memoryProperties = physicalDevice.getMemoryProperties();
   for (auto i = uint32_t { 0 }; i < memoryProperties.memoryTypeCount; i++) {
      if ((typeFilter & (1 << i)) == 0) {
         continue;
      }

      if ((memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) != propertyFlags) {
         continue;
      }

      return i;
   }

   return { };
}

static bool
createBuffers(VulkanDisplayPipeline &displayPipeline,
              vk::PhysicalDevice &physicalDevice,
              vk::Device &device)
{
   static constexpr std::array<float, 24> vertices = {
      -1.0f,  1.0f,  0.0f,  1.0f,
       1.0f,  1.0f,  1.0f,  1.0f,
       1.0f, -1.0f,  1.0f,  0.0f,

       1.0f, -1.0f,  1.0f,  0.0f,
      -1.0f, -1.0f,  0.0f,  0.0f,
      -1.0f,  1.0f,  0.0f,  1.0f,
   };

   // Allocate buffer
   auto bufferDesc = vk::BufferCreateInfo { };
   bufferDesc.size = static_cast<uint32_t>(sizeof(float) * vertices.size());
   bufferDesc.usage = vk::BufferUsageFlagBits::eVertexBuffer;
   bufferDesc.sharingMode = vk::SharingMode::eExclusive;
   bufferDesc.queueFamilyIndexCount = 1;
   bufferDesc.pQueueFamilyIndices = &displayPipeline.queueFamilyIndex;
   displayPipeline.vertexBuffer = device.createBuffer(bufferDesc);

   auto bufferMemoryRequirements =
      device.getBufferMemoryRequirements(displayPipeline.vertexBuffer);

   auto memoryTypeIndex =
      chooseMemoryTypeIndex(physicalDevice,
                            bufferMemoryRequirements.memoryTypeBits,
                            vk::MemoryPropertyFlagBits::eHostVisible);
   if (!memoryTypeIndex) {
      return false;
   }

   auto allocateInfo = vk::MemoryAllocateInfo { };
   allocateInfo.allocationSize = bufferMemoryRequirements.size;
   allocateInfo.memoryTypeIndex = *memoryTypeIndex;

   displayPipeline.vertexBufferMemory = device.allocateMemory(allocateInfo);
   device.bindBufferMemory(displayPipeline.vertexBuffer, displayPipeline.vertexBufferMemory, 0);

   // Upload vertices
   auto mappedMemory = device.mapMemory(displayPipeline.vertexBufferMemory, 0, VK_WHOLE_SIZE);
   std::memcpy(mappedMemory, vertices.data(), bufferMemoryRequirements.size);

   device.flushMappedMemoryRanges({
      vk::MappedMemoryRange { displayPipeline.vertexBufferMemory, 0, VK_WHOLE_SIZE }
   });
   device.unmapMemory(displayPipeline.vertexBufferMemory);

   return true;
}

static bool
createDescriptorSets(VulkanDisplayPipeline &displayPipeline,
                     vk::Device &device)
{
   displayPipeline.descriptorSets.resize(displayPipeline.framebuffers.size() * 2);
   for (auto i = 0u; i < displayPipeline.descriptorSets.size(); ++i) {
      auto allocateInfo = vk::DescriptorSetAllocateInfo { };
      allocateInfo.descriptorPool = displayPipeline.descriptorPool;
      allocateInfo.descriptorSetCount = 1;
      allocateInfo.pSetLayouts = &displayPipeline.descriptorSetLayout;
      displayPipeline.descriptorSets[i] = device.allocateDescriptorSets(allocateInfo)[0];
   }

   return true;
}

static bool
createFences(VulkanDisplayPipeline &displayPipeline,
             vk::Device &device)
{
   displayPipeline.imageAvailableSemaphores.resize(displayPipeline.framebuffers.size());
   displayPipeline.renderFinishedSemaphores.resize(displayPipeline.framebuffers.size());
   displayPipeline.renderFences.resize(displayPipeline.framebuffers.size());

   for (auto i = 0u; i < displayPipeline.framebuffers.size(); ++i) {
      displayPipeline.imageAvailableSemaphores[i] =
         device.createSemaphore(vk::SemaphoreCreateInfo { });

      displayPipeline.renderFinishedSemaphores[i] =
         device.createSemaphore(vk::SemaphoreCreateInfo { });

      displayPipeline.renderFences[i] =
         device.createFence(vk::FenceCreateInfo { vk::FenceCreateFlagBits::eSignaled });
   }

   displayPipeline.frameIndex = 0;
   return true;
}

void
Driver::setWindowSystemInfo(const gpu::WindowSystemInfo &wsi)
{
   auto instance = createVulkanInstance(wsi);
   if (!instance) {
      decaf_abort("createVulkanInstance failed");
   }

   mVkDynLoader.init(instance, ::vkGetInstanceProcAddr);
   registerDebugCallback(instance, mVkDynLoader, reinterpret_cast<void *>(this));

   auto physicalDevice = choosePhysicalDevice(instance);
   if (!physicalDevice) {
      decaf_abort("choosePhysicalDevice failed");
   }

   auto windowSurface = createVulkanSurface(instance, wsi);
   if (!windowSurface) {
      decaf_abort("createVulkanSurface failed");
   }

   auto surfaceFormat = chooseSurfaceFormat(physicalDevice, windowSurface);
   if (!windowSurface) {
      decaf_abort("chooseSurfaceFormat failed");
   }


   auto [device, queueFamilyIndex, supportedFeatures, supportedFeaturesTransformFeedback] =
      createDevice(physicalDevice, windowSurface);
   if (!device) {
      decaf_abort("createDevice failed");
   }

   auto queue = device.getQueue(queueFamilyIndex, 0);
   if (!queue) {
      decaf_abort("device.getQueue failed");
   }

   mSupportedFeatures = supportedFeatures;
   mSupportedFeaturesTransformFeedback = supportedFeaturesTransformFeedback;

   initialise(instance, physicalDevice, device, queue, queueFamilyIndex);

   // Create our full display pipeline
   mDisplayPipeline.windowSurface = windowSurface;
   mDisplayPipeline.windowSurfaceFormat = surfaceFormat;
   mDisplayPipeline.queueFamilyIndex = queueFamilyIndex;
   mDisplayPipeline.presentMode = choosePresentMode(physicalDevice, windowSurface);

   if (!createRenderPass(mDisplayPipeline, device)) {
      decaf_abort("createRenderPass failed");
   }

   if (!createSwapchain(mDisplayPipeline, physicalDevice, device)) {
      decaf_abort("createSwapchain failed");
   }

   if (!createPipelineLayout(mDisplayPipeline, device)) {
      decaf_abort("createPipelineLayout failed");
   }

   if (!createRenderPipeline(mDisplayPipeline, device)) {
      decaf_abort("createRenderPipeline failed");
   }

   if (!createDescriptorPools(mDisplayPipeline, device)) {
      decaf_abort("createDescriptorPools failed");
   }

   if (!createBuffers(mDisplayPipeline, physicalDevice, device)) {
      decaf_abort("createBuffers failed");

   }
   if (!createDescriptorSets(mDisplayPipeline, device)) {
      decaf_abort("createDescriptorSets failed");
   }

   if (!createFences(mDisplayPipeline, device)) {
      decaf_abort("createFences failed");
   }
}

void
Driver::destroyDisplayPipeline()
{
   // createFences
   for (auto &semaphore : mDisplayPipeline.imageAvailableSemaphores) {
      mDevice.destroySemaphore(semaphore);
   }
   mDisplayPipeline.imageAvailableSemaphores.clear();

   for (auto &semaphore : mDisplayPipeline.renderFinishedSemaphores) {
      mDevice.destroySemaphore(semaphore);
   }
   mDisplayPipeline.renderFinishedSemaphores.clear();

   for (auto &fence : mDisplayPipeline.renderFences) {
      mDevice.destroyFence(fence);
   }
   mDisplayPipeline.renderFences.clear();

   // createDescriptorSets
   mDevice.freeDescriptorSets(mDisplayPipeline.descriptorPool, mDisplayPipeline.descriptorSets);
   mDisplayPipeline.descriptorSets.clear();

   // createBuffers
   mDevice.destroyBuffer(mDisplayPipeline.vertexBuffer);
   mDevice.freeMemory(mDisplayPipeline.vertexBufferMemory);
   mDisplayPipeline.vertexBuffer = nullptr;
   mDisplayPipeline.vertexBufferMemory = nullptr;

   // createDescriptorPools
   mDevice.destroyDescriptorPool(mDisplayPipeline.descriptorPool);
   mDisplayPipeline.descriptorPool = nullptr;

   // createRenderPipeline
   mDevice.destroyShaderModule(mDisplayPipeline.vertexShader);
   mDevice.destroyShaderModule(mDisplayPipeline.fragmentShader);
   mDevice.destroyPipeline(mDisplayPipeline.graphicsPipeline);
   mDisplayPipeline.vertexShader = nullptr;
   mDisplayPipeline.fragmentShader = nullptr;
   mDisplayPipeline.graphicsPipeline = nullptr;

   // createPipelineLayout
   mDevice.destroySampler(mDisplayPipeline.trivialSampler);
   mDevice.destroyDescriptorSetLayout(mDisplayPipeline.descriptorSetLayout);
   mDevice.destroyPipelineLayout(mDisplayPipeline.pipelineLayout);
   mDisplayPipeline.trivialSampler = nullptr;
   mDisplayPipeline.descriptorSetLayout = nullptr;
   mDisplayPipeline.pipelineLayout = nullptr;

   // createSwapchain
   destroySwapchain(mDisplayPipeline, mDevice);

   // createRenderPass
   mDevice.destroyRenderPass(mDisplayPipeline.renderPass);
   mDisplayPipeline.renderPass = nullptr;
}

static void
acquireScanBuffer(vk::Device &device,
                  vk::CommandBuffer cmdBuffer,
                  vk::DescriptorSet descriptorSet,
                  vk::Image image,
                  vk::ImageView imageView)
{
   auto imageBarrier = vk::ImageMemoryBarrier { };
   imageBarrier.srcAccessMask = vk::AccessFlags { };
   imageBarrier.dstAccessMask = vk::AccessFlags { };
   imageBarrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
   imageBarrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = image;
   imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
   imageBarrier.subresourceRange.baseMipLevel = 0;
   imageBarrier.subresourceRange.levelCount = 1;
   imageBarrier.subresourceRange.baseArrayLayer = 0;
   imageBarrier.subresourceRange.layerCount = 1;

   cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
                             vk::PipelineStageFlagBits::eAllGraphics,
                             vk::DependencyFlagBits::eByRegion,
                             {},
                             {},
                             { imageBarrier });

   auto descriptorImageInfo = vk::DescriptorImageInfo { };
   descriptorImageInfo.imageView = imageView;
   descriptorImageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

   auto writeDescriptorSet = vk::WriteDescriptorSet { };
   writeDescriptorSet.dstSet = descriptorSet;
   writeDescriptorSet.dstBinding = 1;
   writeDescriptorSet.dstArrayElement = 0;
   writeDescriptorSet.descriptorCount = 1;
   writeDescriptorSet.descriptorType = vk::DescriptorType::eSampledImage;
   writeDescriptorSet.pImageInfo = &descriptorImageInfo;
   device.updateDescriptorSets({ writeDescriptorSet }, {});
}

static void
renderScanBuffer(VulkanDisplayPipeline &displayPipeline,
                 vk::Viewport viewport,
                 vk::CommandBuffer cmdBuffer,
                 vk::DescriptorSet descriptorSet,
                 vk::Image image,
                 vk::ImageView imageView)
{
   cmdBuffer.setViewport(0, { viewport });
   cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                displayPipeline.pipelineLayout,
                                0,
                                { descriptorSet },
                                {});
   cmdBuffer.bindVertexBuffers(0, { displayPipeline.vertexBuffer }, { 0 });
   cmdBuffer.draw(6, 1, 0, 0);
}

static void
releaseScanBuffer(vk::CommandBuffer cmdBuffer,
                  vk::DescriptorSet descriptorSet,
                  vk::Image image,
                  vk::ImageView imageView)
{
   auto imageBarrier = vk::ImageMemoryBarrier { };
   imageBarrier.srcAccessMask = vk::AccessFlags { };
   imageBarrier.dstAccessMask = vk::AccessFlags { };
   imageBarrier.oldLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
   imageBarrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
   imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   imageBarrier.image = image;
   imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
   imageBarrier.subresourceRange.baseMipLevel = 0;
   imageBarrier.subresourceRange.levelCount = 1;
   imageBarrier.subresourceRange.baseArrayLayer = 0;
   imageBarrier.subresourceRange.layerCount = 1;

   cmdBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllGraphics,
                             vk::PipelineStageFlagBits::eAllGraphics,
                             vk::DependencyFlagBits::eByRegion,
                             {},
                             {},
                             { imageBarrier });
}

void
Driver::renderDisplay()
{
   auto frameIndex = mDisplayPipeline.frameIndex;
   auto &renderFence = mDisplayPipeline.renderFences[frameIndex];
   auto &imageAvailableSemaphore = mDisplayPipeline.imageAvailableSemaphores[frameIndex];
   auto &renderFinishedSemaphore = mDisplayPipeline.renderFinishedSemaphores[frameIndex];

   // Wait for the previous frame to finish
   mDevice.waitForFences({ renderFence }, true, std::numeric_limits<uint64_t>::max());
   mDevice.resetFences({ renderFence });

   // Acquire the next frame to render into
   auto nextSwapImage = uint32_t { 0 };
   auto result = mDevice.acquireNextImageKHR(mDisplayPipeline.swapchain,
                                             std::numeric_limits<uint64_t>::max(),
                                             imageAvailableSemaphore,
                                             vk::Fence {},
                                             &nextSwapImage);
   if (result == vk::Result::eErrorOutOfDateKHR) {
      // Recreate swapchain
      recreateSwapchain(mDisplayPipeline, mPhysDevice, mDevice);

      // Try acquire again, this one will die if it fails :D
      mDevice.acquireNextImageKHR(mDisplayPipeline.swapchain,
                                  std::numeric_limits<uint64_t>::max(),
                                  imageAvailableSemaphore, vk::Fence {},
                                  &nextSwapImage);
   }

   // Allocate a command buffer to use
   auto renderCmdBuf = mDevice.allocateCommandBuffers(
      vk::CommandBufferAllocateInfo {
         mCommandPool,
         vk::CommandBufferLevel::ePrimary, 1
      })[0];

   // Select some descriptors to use
   auto descriptorSetTv = mDisplayPipeline.descriptorSets[frameIndex * 2 + 0];
   auto descriptorSetDrc = mDisplayPipeline.descriptorSets[frameIndex * 2 + 1];

   // Setup render layout
   const auto displayWidth = mDisplayPipeline.swapchainExtents.width;
   const auto displayHeight = mDisplayPipeline.swapchainExtents.height;
   auto layout = gpu7::DisplayLayout { };
   gpu7::updateDisplayLayout(
      layout,
      static_cast<float>(displayWidth),
      static_cast<float>(displayHeight));

   if (layout.tv.visible) {
      layout.tv.visible = (mTvSwapChain && mTvSwapChain->presentable);
   }

   if (layout.drc.visible) {
      layout.drc.visible = (mDrcSwapChain && mDrcSwapChain->presentable);
   }

   renderCmdBuf.begin(vk::CommandBufferBeginInfo({}, nullptr));
   {
      renderCmdBuf.bindPipeline(vk::PipelineBindPoint::eGraphics, mDisplayPipeline.graphicsPipeline);
      renderCmdBuf.setScissor(0, { vk::Rect2D { { 0, 0 }, mDisplayPipeline.swapchainExtents } });

      if (layout.tv.visible) {
         acquireScanBuffer(mDevice, renderCmdBuf, descriptorSetTv,
                           mTvSwapChain->image, mTvSwapChain->imageView);
      }

      if (layout.drc.visible) {
         acquireScanBuffer(mDevice, renderCmdBuf, descriptorSetDrc,
                           mDrcSwapChain->image, mDrcSwapChain->imageView);
      }

      auto renderPassBeginInfo = vk::RenderPassBeginInfo { };
      renderPassBeginInfo.renderPass = mDisplayPipeline.renderPass;
      renderPassBeginInfo.framebuffer = mDisplayPipeline.framebuffers[nextSwapImage];
      renderPassBeginInfo.renderArea =
         vk::Rect2D {
            { 0, 0 },
            { displayWidth, displayHeight },
         };

      auto clearValue = vk::ClearValue { layout.backgroundColour };
      renderPassBeginInfo.clearValueCount = 1;
      renderPassBeginInfo.pClearValues = &clearValue;

      renderCmdBuf.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

      if (layout.tv.visible) {
         auto tvViewport =
            vk::Viewport {
               layout.tv.x, layout.tv.y,
               layout.tv.width, layout.tv.height
            };

         renderScanBuffer(mDisplayPipeline, tvViewport, renderCmdBuf,
                           descriptorSetTv, mTvSwapChain->image,
                           mTvSwapChain->imageView);
      }

      if (layout.drc.visible) {
         auto drcViewport =
            vk::Viewport {
               layout.drc.x, layout.drc.y,
               layout.drc.width, layout.drc.height
            };

         renderScanBuffer(mDisplayPipeline, drcViewport, renderCmdBuf,
                           descriptorSetDrc, mDrcSwapChain->image,
                           mDrcSwapChain->imageView);
      }

      renderCmdBuf.endRenderPass();

      if (layout.tv.visible) {
         releaseScanBuffer(renderCmdBuf, descriptorSetTv, mTvSwapChain->image,
                           mTvSwapChain->imageView);
      }

      if (layout.drc.visible) {
         releaseScanBuffer(renderCmdBuf, descriptorSetDrc, mDrcSwapChain->image,
                           mDrcSwapChain->imageView);
      }
   }
   renderCmdBuf.end();

   {
      auto submitInfo = vk::SubmitInfo { };

      auto waitSemaphores = std::array<vk::Semaphore, 1> { imageAvailableSemaphore };
      submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
      submitInfo.pWaitSemaphores = waitSemaphores.data();

      auto waitStage = vk::PipelineStageFlags { vk::PipelineStageFlagBits::eColorAttachmentOutput };
      submitInfo.pWaitDstStageMask = &waitStage;

      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &renderCmdBuf;

      auto signalSemaphores = std::array<vk::Semaphore, 1> { renderFinishedSemaphore };
      submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
      submitInfo.pSignalSemaphores = signalSemaphores.data();

      mQueue.submit({ submitInfo }, renderFence);
   }

   {
      auto presentInfo = vk::PresentInfoKHR { };

      auto waitSemaphores = std::array<vk::Semaphore, 1> { renderFinishedSemaphore };
      presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
      presentInfo.pWaitSemaphores = waitSemaphores.data();

      presentInfo.pImageIndices = &nextSwapImage;

      presentInfo.swapchainCount = 1;
      presentInfo.pSwapchains = &mDisplayPipeline.swapchain;

      auto result = mQueue.presentKHR(presentInfo);
      if (result == vk::Result::eErrorOutOfDateKHR) {
         recreateSwapchain(mDisplayPipeline, mPhysDevice, mDevice);
      }
   }

   mDisplayPipeline.frameIndex = (frameIndex + 1) % 2;
}

void
Driver::windowHandleChanged(void *handle)
{
}

void
Driver::windowSizeChanged(int width, int height)
{
   // Nothing to do, this will be handled during drawing
}

} // namespace vulkan

#endif
