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

gpu::GraphicsDriverType
Driver::type()
{
   return gpu::GraphicsDriverType::DirectX;
}

float
Driver::getAverageFPS()
{
   return 0.0f;
}

float
Driver::getAverageFrametimeMS()
{
   return 0.0f;
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
