#include <gsl.h>
#include <GLFW/glfw3.h>
#include "platform_glfw.h"
#include "platform_input.h"

namespace platform
{

namespace input
{

static const ControllerHandle
KeyboardHandle = 1;

static const ControllerHandle
JoystickHandleStart = 10;

static const ControllerHandle
JoystickHandleEnd = 10 + GLFW_JOYSTICK_LAST;

struct JoystickData
{
   int count = 0;
   const unsigned char *buttons = nullptr;
};

static JoystickData
gJoystickKeys[GLFW_JOYSTICK_LAST];

ControllerHandle
getControllerHandle(const std::string &name)
{
   if (name.compare("keyboard") == 0) {
      glfwSetInputMode(glfw::gWindow, GLFW_STICKY_KEYS, GLFW_CURSOR_NORMAL);
      return KeyboardHandle;
   }

   for (auto i = 0u; i < GLFW_JOYSTICK_LAST; ++i) {
      if (!glfwJoystickPresent(i)) {
         continue;
      }

      if (name.compare(glfwGetJoystickName(i)) == 0) {
         return JoystickHandleStart + i;
      }
   }

   return 0;
}

void
sampleController(ControllerHandle controller)
{
   if (controller >= JoystickHandleStart && controller < JoystickHandleEnd) {
      auto id = gsl::narrow_cast<int>(controller - JoystickHandleStart);
      gJoystickKeys[id].buttons = glfwGetJoystickButtons(id, &gJoystickKeys[id].count);
   }
}

::input::ButtonStatus
getButtonStatus(ControllerHandle controller, int key)
{
   if (controller == KeyboardHandle) {
      if (glfwGetKey(glfw::gWindow, key)) {
         return ::input::ButtonPressed;
      }
   } else if (controller >= JoystickHandleStart && controller < JoystickHandleEnd) {
      auto id = controller - JoystickHandleStart;

      if (gJoystickKeys[id].buttons && key < gJoystickKeys[id].count && gJoystickKeys[id].buttons[key]) {
         return ::input::ButtonPressed;
      }
   }

   return ::input::ButtonReleased;
}

} // namespace input

} // namespace platform
