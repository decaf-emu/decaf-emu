#pragma once
#include "gpu_graphicsdriver.h"

namespace null
{

class Driver : public gpu::GraphicsDriver
{
public:
   virtual ~Driver() = default;

   virtual void setWindowSystemInfo(const gpu::WindowSystemInfo &wsi) override;
   virtual void windowHandleChanged(void *handle) override;
   virtual void windowSizeChanged(int width, int height) override;

   virtual void run() override;
   virtual void runUntilFlip() override;
   virtual void stop() override;

   virtual gpu::GraphicsDriverType type() override;
   virtual gpu::GraphicsDriverDebugInfo *getDebugInfo() override;

   virtual void notifyCpuFlush(phys_addr address, uint32_t size) override;
   virtual void notifyGpuFlush(phys_addr address, uint32_t size) override;

private:
   bool mRunning = false;
};

} // namespace null
