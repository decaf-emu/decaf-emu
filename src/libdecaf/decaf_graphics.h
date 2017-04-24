#pragma once
#include <cstdint>
#include <functional>
#include <libgpu/gpu_graphicsdriver.h>

namespace decaf
{

void
setGraphicsDriver(gpu::GraphicsDriver *driver);

gpu::GraphicsDriver *
getGraphicsDriver();

} // namespace decaf
