#pragma once
#ifdef DECAF_DX12

#include "libdecaf/decaf_graphics.h"
#include "libdecaf/decaf_dx12.h"

namespace gpu
{

namespace dx12
{

class Driver : public decaf::DX12Driver
{
public:
   Driver();
   virtual ~Driver() = default;

   void run() override;
   void stop() override;
   float getAverageFPS() override;
   float getAverageFrametime() override;
   virtual GraphicsDebugInfo getGraphicsDebugInfo() override;

   void notifyCpuFlush(void *ptr, uint32_t size) override;
   void notifyGpuFlush(void *ptr, uint32_t size) override;

private:

};

} // namespace dx12

} // namespace gpu

#endif // DECAF_DX12
