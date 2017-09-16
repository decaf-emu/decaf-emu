#pragma once
#include "gpu_graphicsdriver.h"

namespace null
{

class Driver : public gpu::GraphicsDriver
{
public:
   virtual ~Driver() = default;

   virtual void run() override;
   virtual void stop() override;
   virtual gpu::GraphicsDriverType type() override;

   virtual float getAverageFPS() override;
   virtual float getAverageFrametimeMS() override;

   virtual void notifyCpuFlush(void *ptr, uint32_t size) override;
   virtual void notifyGpuFlush(void *ptr, uint32_t size) override;

private:
   bool mRunning = false;
};

} // namespace null
