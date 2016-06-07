#ifdef DECAF_GLFW
#include <glbinding/gl/gl.h>
#include "common/log.h"
#include "platform_glfw.h"

static const auto DrcWidth = 854.0f;
static const auto DrcHeight = 480.0f;
static const auto DrcScaleFactor = 0.5f;

static const auto TvWidth = 1280.0f;
static const auto TvHeight = 720.0f;
static const auto TvScaleFactor = 0.65f;

namespace platform
{

bool
PlatformGLFW::init()
{
   glfwInit();

   glfwSetErrorCallback([](int error, const char *desc) {
      gLog->error("GLFW 0x{:08X}: {}", error, desc);
   });

   return true;
}

bool
PlatformGLFW::createWindows(const std::string &tvTitle, const std::string &drcTitle)
{
   auto width = getWindowWidth();
   auto height = getWindowHeight();

   mWindow = glfwCreateWindow(width, height, tvTitle.c_str(), NULL, NULL);
   return !!mWindow;
}

void
PlatformGLFW::setTvTitle(const std::string &title)
{
   glfwSetWindowTitle(mWindow, title.c_str());
}

void
PlatformGLFW::setDrcTitle(const std::string &title)
{
}

void
PlatformGLFW::run()
{
   while (!glfwWindowShouldClose(mWindow)) {
      glfwPollEvents();
   }
}

void
PlatformGLFW::shutdown()
{
}

void
PlatformGLFW::activateContext()
{
   glfwMakeContextCurrent(mWindow);
}

void
PlatformGLFW::swapBuffers()
{
   glfwSwapBuffers(mWindow);
}

void
PlatformGLFW::bindDrcWindow()
{
   auto wndWidth = static_cast<float>(getWindowWidth());
   auto wndHeight = static_cast<float>(getWindowHeight());
   auto tvWidth = static_cast<float>(getTvWidth());
   auto tvHeight = static_cast<float>(getTvHeight());
   auto drcWidth = static_cast<float>(getDrcWidth());
   auto drcHeight = static_cast<float>(getDrcHeight());

   auto tvTop = 0.0f;
   auto tvBottom = tvTop + tvHeight;
   auto drcTop = tvBottom;
   auto drcLeft = (wndWidth - drcWidth) / 2.0f;
   auto drcBottom = drcTop + drcHeight;

   float v[4] = {
      drcLeft,
      wndHeight - drcBottom,
      drcWidth,
      drcHeight
   };

   gl::glViewportArrayv(0, 1, v);
}

void
PlatformGLFW::bindTvWindow()
{
   auto wndWidth = static_cast<float>(getWindowWidth());
   auto wndHeight = static_cast<float>(getWindowHeight());
   auto tvWidth = static_cast<float>(getTvWidth());
   auto tvHeight = static_cast<float>(getTvHeight());

   auto tvTop = 0.0f;
   auto tvLeft = (wndWidth - tvWidth) / 2.0f;
   auto tvBottom = tvTop + tvHeight;

   float v[4] = {
      tvLeft,
      wndHeight - tvBottom,
      tvWidth,
      tvHeight
   };

   gl::glViewportArrayv(0, 1, v);
}

int
PlatformGLFW::getDrcWidth()
{
   return static_cast<int>(DrcWidth * DrcScaleFactor);
}

int
PlatformGLFW::getDrcHeight()
{
   return static_cast<int>(DrcHeight * DrcScaleFactor);
}

int
PlatformGLFW::getTvWidth()
{
   return static_cast<int>(TvWidth * TvScaleFactor);
}

int
PlatformGLFW::getTvHeight()
{
   return static_cast<int>(TvHeight * TvScaleFactor);
}

int
PlatformGLFW::getWindowWidth()
{
   return std::max(getTvWidth(), getDrcWidth());
}

int
PlatformGLFW::getWindowHeight()
{
   return getTvHeight() + getDrcHeight();
}

} // namespace platform

#endif // ifdef DECAF_GLFW
