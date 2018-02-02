#pragma once
#ifdef DECAF_VULKAN
#include "decafsdl_graphics.h"
#include <libdecaf/decaf.h>
#include <libgpu/gpu_vulkandriver.h>

class DecafSDLVulkan : public DecafSDLGraphics
{
public:
   DecafSDLVulkan();
   ~DecafSDLVulkan() override;

   bool
   initialise(int width,
              int height) override;

   void
   shutdown() override;

   void
   renderFrame(Viewport &tv,
               Viewport &drc) override;

   gpu::GraphicsDriver *
   getDecafDriver() override;

   decaf::DebugUiRenderer *
   getDecafDebugUiRenderer() override;

protected:
   std::thread mGraphicsThread;
   gpu::VulkanDriver *mDecafDriver = nullptr;
};

#endif // DECAF_VULKAN
