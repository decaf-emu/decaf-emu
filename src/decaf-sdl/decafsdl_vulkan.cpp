#ifdef DECAF_VULKAN
#pragma optimize("", off)
#include "clilog.h"
#include "config.h"
#include "decafsdl_vulkan.h"

DecafSDLVulkan::DecafSDLVulkan()
{
}

DecafSDLVulkan::~DecafSDLVulkan()
{
}

bool
DecafSDLVulkan::initialise(int width, int height)
{
   return false;
}

void
DecafSDLVulkan::shutdown()
{
}

void
DecafSDLVulkan::renderFrame(Viewport &tv, Viewport &drc)
{
}

gpu::GraphicsDriver *
DecafSDLVulkan::getDecafDriver()
{
   return nullptr;
}

decaf::DebugUiRenderer *
DecafSDLVulkan::getDecafDebugUiRenderer()
{
   return nullptr;
}

#endif // ifdef DECAF_VULKAN
