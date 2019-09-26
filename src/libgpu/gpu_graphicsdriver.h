#pragma once
#include <cstdint>
#include <functional>
#include <libcpu/be2_struct.h>

namespace gpu
{

enum class GraphicsDriverType
{
   Null,
   OpenGL,
   Vulkan,
};

enum class WindowSystemType
{
   Headless,
   Windows,
   Cocoa,
   X11,
   Xcb,
   Wayland,
};

struct WindowSystemInfo
{
   WindowSystemType type = WindowSystemType::Headless;
   void *displayConnection = nullptr;
   void *renderSurface = nullptr;
   double renderSurfaceScale = 1.0;
};

struct GraphicsDriverDebugInfo
{
   GraphicsDriverType type = GraphicsDriverType::Null;
   double averageFps = 0.0f;
   double averageFrameTimeMS = 0.0f;
};

class GraphicsDriver
{
public:
   virtual ~GraphicsDriver() = default;

   virtual void setWindowSystemInfo(const WindowSystemInfo &wsi) = 0;
   virtual void windowHandleChanged(void *handle) = 0;
   virtual void windowSizeChanged(int width, int height) = 0;

   virtual void run() = 0;
   virtual void runUntilFlip() = 0;
   virtual void stop() = 0;

   virtual GraphicsDriverType type() = 0;
   virtual gpu::GraphicsDriverDebugInfo *getDebugInfo() = 0;

   // Called for stores to emulated physical RAM, such as via DCFlushRange().
   //  May be called from any CPU core!
   virtual void notifyCpuFlush(phys_addr address,
                               uint32_t size) = 0;

   // Called when the emulated CPU is about to read from emulated physical RAM,
   //  such as after DCInvalidateRange().  May be called from any CPU core!
   virtual void notifyGpuFlush(phys_addr address,
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
createGraphicsDriver();

GraphicsDriver *
createGraphicsDriver(GraphicsDriverType type);

} // namespace gpu
