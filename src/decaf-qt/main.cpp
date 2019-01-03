#include "mainwindow.h"
#include "vulkanwindow.h"
#include "decafinterface.h"
#include "inputdriver.h"
#include "sounddriver.h"

#include <libconfig/config_toml.h>
#include <libdecaf/decaf.h>
#include <libdecaf/decaf_input.h>
#include <libdecaf/decaf_nullinputdriver.h>
#include <libdecaf/decaf_log.h>

#include <SDL2/SDL.h>

#include <QApplication>
#include <QVulkanInstance>

#include <common/platform_debug.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL
debugMessageCallback(VkDebugReportFlagsEXT flags,
                     VkDebugReportObjectTypeEXT objectType,
                     uint64_t object,
                     size_t location,
                     int32_t messageCode,
                     const char* pLayerPrefix,
                     const char* pMessage,
                     void* pUserData)
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
      qDebug() <<
         QString("Vulkan Debug Report: %1, %2, %3, %4, %5, %6, %7")
            .arg(QString::fromStdString(vk::to_string(vk::DebugReportFlagsEXT(flags))))
            .arg(QString::fromStdString(vk::to_string(vk::DebugReportObjectTypeEXT(objectType))))
            .arg(object)
            .arg(location)
            .arg(messageCode)
            .arg(pLayerPrefix)
            .arg(pMessage);
   } else {
      qDebug() <<
         QString("Vulkan Debug Report (Known Case): %1")
            .arg(pMessage);
   }

   // We should break to the debugger on unexpected situations.
   if (flags == VK_DEBUG_REPORT_WARNING_BIT_EXT || flags == VK_DEBUG_REPORT_ERROR_BIT_EXT) {
      if (!isKnownIssue) {
         platform::debugBreak();
      }
   }

   return VK_FALSE;
}

int main(int argc, char *argv[])
{
   QCoreApplication::setOrganizationName("decaf-emu");
   QCoreApplication::setOrganizationDomain("decaf-emu.com");
   QCoreApplication::setApplicationName("decaf-qt");

   QApplication app(argc, argv);
   QVulkanInstance inst;

   bool vulkanDebug = false;

   inst.setApiVersion({ 1, 0, 0 });
   inst.setExtensions({
      VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
   });

   if (vulkanDebug) {
      inst.setLayers({
         "VK_LAYER_LUNARG_standard_validation"
      });

      auto extensions = inst.extensions();
      extensions.append(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
      inst.setExtensions(extensions);
   }

   if (!inst.create()) {
      qFatal("Failed to create Vulkan instance: %d", inst.errorCode());
   }

   if (vulkanDebug) {
      PFN_vkCreateDebugReportCallbackEXT createDebugReportCallback = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(inst.getInstanceProcAddr("vkCreateDebugReportCallbackEXT"));

      VkDebugReportCallbackCreateInfoEXT dbgCallbackInfo;
      memset(&dbgCallbackInfo, 0, sizeof(dbgCallbackInfo));
      dbgCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
      dbgCallbackInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT
         | VK_DEBUG_REPORT_WARNING_BIT_EXT
         | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
         | VK_DEBUG_REPORT_DEBUG_BIT_EXT;
      dbgCallbackInfo.pfnCallback = debugMessageCallback;
      VkDebugReportCallbackEXT mcunt;
      VkResult err = createDebugReportCallback(inst.vkInstance(), &dbgCallbackInfo, nullptr, &mcunt);
   }

   decaf::createConfigDirectory();
   SettingsStorage settings { decaf::makeConfigPath("config.toml") };

   InputDriver inputDriver { &settings };
   SoundDriver soundDriver { &settings };
   DecafInterface decafInterface { &settings, &inputDriver, &soundDriver };

   MainWindow mainWindow { &settings, &inst, &decafInterface, &inputDriver };
   mainWindow.resize(1024, 768);
   mainWindow.show();
   return app.exec();
}
