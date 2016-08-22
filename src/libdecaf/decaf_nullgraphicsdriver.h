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
   virtual void handleDCFlush(uint32_t addr, uint32_t size) override;

private:
   bool mRunning = false;
};

} // namespace decaf
