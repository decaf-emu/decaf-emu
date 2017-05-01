#pragma once
#include <cstdint>
#include <functional>

namespace gpu
{

class GraphicsDriver
{
public:
   struct GraphicsDebugInfo {
      uint64_t fetchShaders;
      uint64_t vertexShaders;
      uint64_t pixelShaders;

      uint64_t shaderPipelines;
      uint64_t surfaces;
      uint64_t dataBuffers;

      // TODO: Samplers?
   };

public:
   virtual ~GraphicsDriver()
   {
   }

   virtual void run() = 0;
   virtual void stop() = 0;
   virtual float getAverageFPS() = 0;
   // In milliseconds
   virtual float getAverageFrametime() = 0;
   virtual GraphicsDebugInfo getGraphicsDebugInfo() = 0;

   // Called for stores to emulated physical RAM, such as via DCFlushRange().
   //  May be called from any CPU core!
   virtual void notifyCpuFlush(void *ptr,
                               uint32_t size) = 0;

   // Called when the emulated CPU is about to read from emulated physical RAM,
   //  such as after DCInvalidateRange().  May be called from any CPU core!
   virtual void notifyGpuFlush(void *ptr,
                               uint32_t size) = 0;

   // Begin a frame capture, frames will be dumped to outPrefix0.tga -> outPrefixN.tga
   virtual bool
   startFrameCapture(const std::string &outPrefix,
                     bool captureTV,
                     bool captureDRC)
   {
      return false;
   }

   virtual size_t
   stopFrameCapture()
   {
      return 0;
   }
};

GraphicsDriver *
createDX12Driver();

GraphicsDriver *
createGLDriver();

GraphicsDriver *
createNullDriver();

GraphicsDriver *
createVulkanDriver();

} // namespace gpu
