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

   virtual void run() override;
   virtual void stop() override;
   virtual gpu::GraphicsDriverType type() override;

   virtual float getAverageFPS() override;
   virtual float getAverageFrametimeMS() override;

   virtual void notifyCpuFlush(void *ptr, uint32_t size) override;
   virtual void notifyGpuFlush(void *ptr, uint32_t size) override;
};

} // namespace dx12

} // namespace gpu

#endif // DECAF_DX12
