#pragma once
#include <fmt/format.h>
#include <libcpu/be2_struct.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace gpu
{

enum class GraphicsDriverType
{
   Null,
   // Previously OpenGL = 1
   Vulkan = 2,
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
   virtual GraphicsDriverDebugInfo *getDebugInfo() = 0;

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

template <>
struct fmt::formatter<gpu::WindowSystemType> :
   fmt::formatter<std::string_view>
{
   template <typename FormatContext>
   auto format(gpu::WindowSystemType value, FormatContext& ctx) {
      std::string_view name = "unknown";
      switch (value) {
      case gpu::WindowSystemType::Headless:
         name = "Headless";
         break;
      case gpu::WindowSystemType::Windows:
         name = "Windows";
         break;
      case gpu::WindowSystemType::Cocoa:
         name = "Cocoa";
         break;
      case gpu::WindowSystemType::X11:
         name = "X11";
         break;
      case gpu::WindowSystemType::Xcb:
         name = "Xcb";
         break;
      case gpu::WindowSystemType::Wayland:
         name = "Wayland";
         break;
      }
      return fmt::formatter<string_view>::format(name, ctx);
   }
};
