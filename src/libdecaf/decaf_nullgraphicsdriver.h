#pragma once
#include "decaf_graphics.h"

namespace decaf
{

class NullGraphicsDriver : public GraphicsDriver
{
public:
   virtual ~NullGraphicsDriver();

   virtual void run() override;
   virtual void stop() override;
   virtual float getAverageFPS() override;
   virtual void notifyCpuFlush(void *ptr, uint32_t size) override;
   virtual void notifyGpuFlush(void *ptr, uint32_t size) override;

private:
   bool mRunning = false;
};

} // namespace decaf
