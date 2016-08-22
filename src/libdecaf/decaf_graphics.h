#pragma once
#include "common/types.h"
#include <functional>

namespace decaf
{

class GraphicsDriver
{
public:
   virtual ~GraphicsDriver()
   {
   }

   virtual void run() = 0;
   virtual void stop() = 0;
   virtual float getAverageFPS() = 0;

   virtual void handleDCFlush(uint32_t addr, uint32_t size) = 0;  // May be called from any thread!
};

class OpenGLDriver : public GraphicsDriver
{
public:
   using SwapFunction = std::function<void(unsigned int, unsigned int)>;

   virtual ~OpenGLDriver()
   {
   }

   virtual void getSwapBuffers(unsigned int *tv, unsigned int *drc) = 0;
   virtual void syncPoll(const SwapFunction &swapFunc) = 0;
};

OpenGLDriver *
createGLDriver();

void
setGraphicsDriver(GraphicsDriver *driver);

GraphicsDriver *
getGraphicsDriver();

} // namespace decaf
