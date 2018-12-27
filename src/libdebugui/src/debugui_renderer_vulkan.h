#pragma once
#include "debugui.h"
#include "debugui_manager.h"

#ifdef DECAF_VULKAN
#include <vulkan/vulkan.hpp>

namespace debugui
{

class RendererVulkan : public VulkanRenderer
{
public:
   RendererVulkan(const std::string &configPath, VulkanRendererInfo &info);
   ~RendererVulkan() override;

   void draw(vk::CommandBuffer cmdBuffer, unsigned width, unsigned height) override;

   bool
   onKeyAction(KeyboardKey key, KeyboardAction action) override
   {
      return mUi.onKeyAction(key, action);
   }

   bool
   onMouseAction(MouseButton button, MouseAction action) override
   {
      return mUi.onMouseAction(button, action);
   }

   bool
   onMouseMove(float x, float y) override
   {
      return mUi.onMouseMove(x, y);
   }

   bool
   onMouseScroll(float x, float y) override
   {
      return mUi.onMouseScroll(x, y);
   }

   bool
   onText(const char *text) override
   {
      return mUi.onText(text);
   }

   void
   setClipboardCallbacks(ClipboardTextGetCallback getClipboardFn,
                         ClipboardTextSetCallback setClipboardFn) override
   {
      return mUi.setClipboardCallbacks(getClipboardFn, setClipboardFn);
   }

private:
   Manager mUi;
   vk::Device mDevice;
   vk::Queue mQueue;
   vk::CommandPool mCommandPool;
};

} // namespace debugui

#endif // ifdef DECAF_VULKAN
