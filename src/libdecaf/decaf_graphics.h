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

   // Called for stores to emulated physical RAM, such as via DCFlushRange().
   //  May be called from any CPU core!
   virtual void notifyCpuFlush(void *ptr, uint32_t size) = 0;
   // Called when the emulated CPU is about to read from emulated physical RAM,
   //  such as after DCInvalidateRange().  May be called from any CPU core!
   virtual void notifyGpuFlush(void *ptr, uint32_t size) = 0;
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
