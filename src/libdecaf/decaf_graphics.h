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
};

class OpenGLDriver : public GraphicsDriver
{
public:
   using SwapFunction = std::function<void(unsigned int, unsigned int)>;
   using SetSwapIntervalFunction = std::function<void(int)>;

   virtual ~OpenGLDriver()
   {
   }

   virtual void getSwapBuffers(unsigned int *tv, unsigned int *drc) = 0;
   virtual void setSwapIntervalHandler(const SetSwapIntervalFunction &handler) = 0;
   virtual void syncPoll(const SwapFunction &swapFunc) = 0;
};

OpenGLDriver *
createGLDriver();

void
setGraphicsDriver(GraphicsDriver *driver);

GraphicsDriver *
getGraphicsDriver();

} // namespace decaf
