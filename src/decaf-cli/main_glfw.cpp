#ifdef DECAF_GLFW

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include "decaf/decaf.h"
#include "clilog.h"
#include "config.h"
#include "input_common.h"
#include "gl_common.h"
#include <GLFW/glfw3.h>

using namespace decaf::input;

static GLFWwindow *
gUserWindow = nullptr;

static GLFWwindow *
gBgWindow = nullptr;

int glfwStart()
{
   static int windowWidth = 1420;
   static int windowHeight = 768;

   glfwInit();

   glfwSetErrorCallback([](int error, const char *desc) {
      gLog->error("GLFW 0x{:08X}: {}", error, desc);
   });

   gUserWindow = glfwCreateWindow(windowWidth, windowHeight, "Decaf", NULL, NULL);

   glfwWindowHint(GLFW_VISIBLE, 0);
   gBgWindow = glfwCreateWindow(1, 1, "Decaf - GPU", nullptr, gUserWindow);

   glfwMakeContextCurrent(gUserWindow);
   cglContextInitialise();

   decaf::setVpadCoreButtonCallback([](auto channel, auto button) {
      auto key = getButtonMapping(channel, button);
      if (glfwGetKey(gUserWindow, key)) {
         return decaf::input::ButtonPressed;
      }
      return decaf::input::ButtonReleased;
   });

   if (!decaf::initialise()) {
      return 1;
   }

   auto gpuThread = std::thread([]() {
      glfwMakeContextCurrent(gBgWindow);
      cglContextInitialise();
      decaf::runGpuDriver();
   });

   decaf::start();

   cglInitialise();

   while (!glfwWindowShouldClose(gUserWindow)) {
      glfwPollEvents();

      cglClear();

      // vSync

      const auto drcRatio = 0.25f;
      const auto overallScale = 0.75f;
      const auto sepGap = 5.0f;

      static const auto DrcWidth = 854.0f;
      static const auto DrcHeight = 480.0f;
      static const auto TvWidth = 1280.0f;
      static const auto TvHeight = 720.0f;

      glfwGetWindowSize(gUserWindow, &windowWidth, &windowHeight);

      auto tvWidth = windowWidth * (1.0f - drcRatio) * overallScale;
      auto tvHeight = TvHeight * (tvWidth / TvWidth);
      auto drcWidth = windowWidth * (drcRatio)* overallScale;
      auto drcHeight = DrcHeight * (drcWidth / DrcWidth);
      auto totalWidth = std::max(tvWidth, drcWidth);
      auto totalHeight = tvHeight + drcHeight + sepGap;
      auto baseLeft = (windowWidth / 2) - (totalWidth / 2);
      auto baseBottom = -(windowHeight / 2) + (totalHeight / 2);

      auto tvLeft = 0.0f;
      auto tvBottom = windowHeight - tvHeight;
      auto drcLeft = (tvWidth / 2) - (drcWidth / 2);
      auto drcBottom = windowHeight - tvHeight - drcHeight - sepGap;

      float tvVp[4] = {
         baseLeft + tvLeft,
         baseBottom + tvBottom,
         tvWidth,
         tvHeight
      };

      float drcVp[4] = {
         baseLeft + drcLeft,
         baseBottom + drcBottom,
         drcWidth,
         drcHeight
      };

      gl::GLuint tvBuffer;
      gl::GLuint drcBuffer;
      decaf::getSwapBuffers(&tvBuffer, &drcBuffer);

      gl::glViewportArrayv(0, 1, tvVp);
      cglDrawScanBuffer(tvBuffer);

      gl::glViewportArrayv(0, 1, drcVp);
      cglDrawScanBuffer(drcBuffer);

      glfwSwapBuffers(gUserWindow);

      static char newTitle[1024];
      sprintf(newTitle, "Decaf - FPS: %.02f", decaf::getAverageFps());
      glfwSetWindowTitle(gUserWindow, newTitle);
   }

   // Shut down decaf
   decaf::shutdown();

   // Shut down the GPU
   decaf::shutdownGpuDriver();
   gpuThread.join();

   return 0;
}

#endif // DECAF_GLFW
