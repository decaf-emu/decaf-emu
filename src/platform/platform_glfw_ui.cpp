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

static int
getWindowWidth()
{
   return std::max(platform::ui::getTvWidth(), platform::ui::getDrcWidth());
}

static int
getWindowHeight()
{
   return platform::ui::getTvHeight() + platform::ui::getDrcHeight();
}

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
createWindows(const std::string &tvTitle, const std::string &drcTitle)
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
activateContext()
{
   glfwMakeContextCurrent(glfw::gWindow);
}

void
swapBuffers()
{
   glfwSwapBuffers(glfw::gWindow);
}

void
bindDrcWindow()
{
   float wndWidth = static_cast<float>(getWindowWidth());
   float wndHeight = static_cast<float>(getWindowHeight());
   float tvWidth = static_cast<float>(getTvWidth());
   float tvHeight = static_cast<float>(getTvHeight());
   float drcWidth = static_cast<float>(getDrcWidth());
   float drcHeight = static_cast<float>(getDrcHeight());

   float tvTop = 0;
   float tvBottom = tvTop + tvHeight;
   float drcTop = tvBottom;
   float drcLeft = (wndWidth - drcWidth) / 2;
   float drcBottom = drcTop + drcHeight;

   glViewport(drcLeft, wndHeight - drcBottom, drcWidth, drcHeight);
}

void
bindTvWindow()
{
   float wndWidth = static_cast<float>(getWindowWidth());
   float wndHeight = static_cast<float>(getWindowHeight());
   float tvWidth = static_cast<float>(getTvWidth());
   float tvHeight = static_cast<float>(getTvHeight());

   float tvTop = 0;
   float tvLeft = (wndWidth - tvWidth) / 2;
   float tvBottom = tvTop + tvHeight;

   glViewport(tvLeft, wndHeight - tvBottom, tvWidth, tvHeight);
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
