#pragma once
#include <cstdint>
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
   virtual void notifyCpuFlush(void *ptr,
                               uint32_t size) = 0;

   // Called when the emulated CPU is about to read from emulated physical RAM,
   //  such as after DCInvalidateRange().  May be called from any CPU core!
   virtual void notifyGpuFlush(void *ptr,
                               uint32_t size) = 0;
};

GraphicsDriver *
createGLDriver();

GraphicsDriver *
createDX12Driver();

void
setGraphicsDriver(GraphicsDriver *driver);

GraphicsDriver *
getGraphicsDriver();

} // namespace decaf
