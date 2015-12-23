#include <GLFW/glfw3.h>
#include "platform_glfw.h"
#include "platform_ui.h"
#include "gpu/driver.h"
#include "utils/log.h"

static const auto DrcWidth = 854.0f;
static const auto DrcHeight = 480.0f;
static const auto DrcScaleFactor = 0.5f;

static const auto TvWidth = 1280.0f;
static const auto TvHeight = 720.0f;
static const auto TvScaleFactor = 0.65f;

namespace platform
{

namespace glfw
{

GLFWwindow *
gWindow = nullptr;

} // namespace glfw

namespace ui
{

bool
init()
{
   glfwInit();
   glfwSetErrorCallback([](int error, const char *desc) {
      gLog->error("GLFW 0x{:08X}: {}", error, desc);
   });

   return true;
}

bool
createWindow(const std::string &title)
{
   auto width = getWindowWidth();
   auto height = getWindowHeight();

   glfw::gWindow = glfwCreateWindow(width, height, "Decaf", NULL, NULL);

   if (!glfw::gWindow) {
      return false;
   }

   return true;
}

void
run()
{
   while (!glfwWindowShouldClose(glfw::gWindow)) {
      glfwPollEvents();
   }
}

void
shutdown()
{
}

void
swapBuffers()
{
   glfwSwapBuffers(glfw::gWindow);
}

void
activateContext()
{
   glfwMakeContextCurrent(glfw::gWindow);
}

void
releaseContext()
{
   glfwMakeContextCurrent(nullptr);
}

int
getWindowWidth()
{
   return std::max(getTvWidth(), getDrcWidth());
}

int
getWindowHeight()
{
   return getTvHeight() + getDrcHeight();
}

int
getDrcWidth()
{
   return static_cast<int>(854.0f * DrcScaleFactor);
}

int
getDrcHeight()
{
   return static_cast<int>(480.0f * DrcScaleFactor);
}

int
getTvWidth()
{
   return static_cast<int>(1280.0f * TvScaleFactor);
}

int
getTvHeight()
{
   return static_cast<int>(720.0f * TvScaleFactor);
}

} // namespace ui

} // namespace platform
