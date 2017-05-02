#pragma once
#ifdef DECAF_VULKAN
#include "gpu_graphicsdriver.h"

namespace gpu
{

class VulkanDriver : public GraphicsDriver
{
public:
   virtual ~VulkanDriver() = default;
};

} // namespace gpu

#endif // DECAF_VULKAN
