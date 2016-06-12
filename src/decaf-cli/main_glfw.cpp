#ifdef DECAF_GLFW

#include "clilog.h"
#include "config.h"
#include "common/strutils.h"
#include "decaf/decaf.h"
#include "gl_common.h"
#include "input_common.h"
#include <GLFW/glfw3.h>
#include <thread>

using namespace decaf::input;

static GLFWwindow *
gUserWindow = nullptr;

static GLFWwindow *
gBgWindow = nullptr;

static decaf::input::KeyboardKey
translateKeyCode(int key)
{
   switch (key) {
   case GLFW_KEY_TAB:
      return decaf::input::KeyboardKey::Tab;
   case GLFW_KEY_LEFT:
      return decaf::input::KeyboardKey::LeftArrow;
   case GLFW_KEY_RIGHT:
      return decaf::input::KeyboardKey::RightArrow;
   case GLFW_KEY_UP:
      return decaf::input::KeyboardKey::UpArrow;
   case GLFW_KEY_DOWN:
      return decaf::input::KeyboardKey::DownArrow;
   case GLFW_KEY_PAGE_UP:
      return decaf::input::KeyboardKey::PageUp;
   case GLFW_KEY_PAGE_DOWN:
      return decaf::input::KeyboardKey::PageDown;
   case GLFW_KEY_HOME:
      return decaf::input::KeyboardKey::Home;
   case GLFW_KEY_END:
      return decaf::input::KeyboardKey::End;
   case GLFW_KEY_DELETE:
      return decaf::input::KeyboardKey::Delete;
   case GLFW_KEY_BACKSPACE:
      return decaf::input::KeyboardKey::Backspace;
   case GLFW_KEY_ENTER:
      return decaf::input::KeyboardKey::Enter;
   case GLFW_KEY_ESCAPE:
      return decaf::input::KeyboardKey::Escape;
   case GLFW_KEY_LEFT_CONTROL:
      return decaf::input::KeyboardKey::LeftControl;
   case GLFW_KEY_RIGHT_CONTROL:
      return decaf::input::KeyboardKey::RightControl;
   case GLFW_KEY_LEFT_SHIFT:
      return decaf::input::KeyboardKey::LeftShift;
   case GLFW_KEY_RIGHT_SHIFT:
      return decaf::input::KeyboardKey::RightShift;
   case GLFW_KEY_LEFT_ALT:
      return decaf::input::KeyboardKey::LeftAlt;
   case GLFW_KEY_RIGHT_ALT:
      return decaf::input::KeyboardKey::RightAlt;
   case GLFW_KEY_LEFT_SUPER:
      return decaf::input::KeyboardKey::LeftSuper;
   case GLFW_KEY_RIGHT_SUPER:
      return decaf::input::KeyboardKey::RightSuper;
   default:
      if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
         auto id = (key - GLFW_KEY_A) + static_cast<int>(decaf::input::KeyboardKey::A);
         return static_cast<decaf::input::KeyboardKey>(id);
      }

      return decaf::input::KeyboardKey::Unknown;
   }
}

int glfwStart()
{
   static int windowWidth = 1420;
   static int windowHeight = 768;

   glfwInit();

   glfwSetErrorCallback(
      [](int error, const char *desc) {
         gLog->error("GLFW 0x{:08X}: {}", error, desc);
      });

   gUserWindow = glfwCreateWindow(windowWidth, windowHeight, "Decaf", NULL, NULL);

   glfwWindowHint(GLFW_VISIBLE, 0);
   gBgWindow = glfwCreateWindow(1, 1, "Decaf - GPU", nullptr, gUserWindow);

   glfwMakeContextCurrent(gUserWindow);
   cglContextInitialise();

   decaf::setVpadCoreButtonCallback(
      [](auto channel, auto button) {
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

   glfwSetMouseButtonCallback(gUserWindow,
      [](GLFWwindow *, int button, int action, int /*mods*/) {
         if (action == GLFW_PRESS) {
            decaf::injectMouseButtonInput(button, decaf::input::MouseAction::Press);
         } else if (action == GLFW_RELEASE) {
            decaf::injectMouseButtonInput(button, decaf::input::MouseAction::Release);
         }
      });

   glfwSetScrollCallback(gUserWindow,
      [](GLFWwindow *, double xoffset, double yoffset) {
         decaf::injectScrollInput(static_cast<float>(xoffset), static_cast<float>(yoffset));
      });

   glfwSetKeyCallback(gUserWindow,
      [](GLFWwindow *, int key, int, int action, int /*mods*/) {
         if (action == GLFW_PRESS) {
            decaf::injectKeyInput(translateKeyCode(key), decaf::input::KeyboardAction::Press);
         } else if (action == GLFW_RELEASE) {
            decaf::injectKeyInput(translateKeyCode(key), decaf::input::KeyboardAction::Release);
         }
      });

   glfwSetCharCallback(gUserWindow,
      [](GLFWwindow *, unsigned int c) {
         if (c > 0 && c < 0x10000) {
            decaf::injectCharInput(static_cast<unsigned short>(c));
         }
      });

   glfwSetCursorPosCallback(gUserWindow,
      [](GLFWwindow *, double x, double y) {
         decaf::injectMousePos(static_cast<float>(x), static_cast<float>(y));
      });

   decaf::setClipboardTextCallbacks(
      []() {
         return glfwGetClipboardString(gUserWindow);
      },
      [](auto text) {
         glfwSetClipboardString(gUserWindow, text);
      });

   cglInitialise();
   decaf::initialiseDbgUi();

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

      decaf::drawDbgUi(windowWidth, windowHeight);

      glfwSwapBuffers(gUserWindow);

      static char newTitle[1024];
      snprintf(newTitle, 1024, "Decaf - FPS: %.02f", decaf::getAverageFps());
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
