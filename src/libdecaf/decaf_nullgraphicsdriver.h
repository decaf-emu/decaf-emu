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

   virtual void invalidateMemory(uint32_t mode, ppcaddr_t memStart, ppcaddr_t memEnd) override;

private:
   bool mRunning = false;
};

} // namespace decaf
