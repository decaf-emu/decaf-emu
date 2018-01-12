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

void
Driver::notifyCpuFlush(void *ptr, uint32_t size)
{
}

void
Driver::notifyGpuFlush(void *ptr, uint32_t size)
{
}

DriverType
Driver::type()
{
   return DriverType::DRIVER_DX;
}

} // namespace dx12

} // namespace gpu

#endif // DECAF_DX12
