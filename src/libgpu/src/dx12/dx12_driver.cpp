#ifdef DECAF_DX12

#include "dx12_driver.h"

namespace gpu
{

namespace dx12
{

Driver::Driver()
{

}

void
Driver::run()
{
}

void
Driver::stop()
{
}

float
Driver::getAverageFPS()
{
   return 0.0f;
}

float
Driver::getAverageFrametime()
{
   return 0.0f;
}

gpu::GraphicsDriver::GraphicsDebugInfo
Driver::getGraphicsDebugInfo() {
   return gpu::GraphicsDriver::GraphicsDebugInfo{ 0 };
}

void
Driver::notifyCpuFlush(void *ptr, uint32_t size)
{
}

void
Driver::notifyGpuFlush(void *ptr, uint32_t size)
{
}

} // namespace dx12

} // namespace gpu

#endif // DECAF_DX12
