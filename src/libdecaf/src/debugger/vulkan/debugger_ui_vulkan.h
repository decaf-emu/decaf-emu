#pragma once

#ifdef DECAF_VULKAN

#include "decaf_debugger.h"
#include <vulkan/vulkan.hpp>

namespace debugger
{

namespace ui
{

class RendererVulkan : public decaf::VulkanUiRenderer
{
public:
   virtual void initialise(decaf::VulkanUiRendererInitInfo *info) override;
   virtual void postInitialize() override;
   virtual void draw(unsigned width, unsigned height, vk::CommandBuffer cmdBuffer) override;

private:
   vk::Device mDevice;
   vk::Queue mQueue;
   vk::CommandPool mCommandPool;

};

} // namespace ui

} // namespace debugger

#endif // ifdef DECAF_VULKAN
