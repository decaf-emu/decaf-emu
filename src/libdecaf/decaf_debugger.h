#pragma once

#ifdef DECAF_VULKAN
#include <vulkan/vulkan.hpp>
#endif

namespace decaf
{

class DebugUiRenderer
{
public:
   virtual void onDebuggerInitialized() = 0;
};

#ifdef DECAF_GL
class GLUiRenderer : public DebugUiRenderer
{
public:
   virtual void initialise() = 0;
   virtual void shutdown() = 0;
   virtual void draw(unsigned width, unsigned height) = 0;
};
#endif

#ifdef DECAF_VULKAN
struct VulkanUiRendererInitInfo
{
   vk::PhysicalDevice physDevice;
   vk::Device device;
   vk::Queue queue;
   vk::DescriptorPool descriptorPool;
   vk::RenderPass renderPass;
   vk::CommandPool commandPool;
};

class VulkanUiRenderer : public DebugUiRenderer
{
public:
   virtual void initialise(VulkanUiRendererInitInfo *info) = 0;
   virtual void shutdown() = 0;
   virtual void draw(unsigned width, unsigned height, vk::CommandBuffer cmdBuffer) = 0;
};
#endif

DebugUiRenderer *
createDebugGLRenderer();

DebugUiRenderer *
createDebugVulkanRenderer();

void
setDebugUiRenderer(DebugUiRenderer *renderer);

DebugUiRenderer *
getDebugUiRenderer();

} // namespace decaf
