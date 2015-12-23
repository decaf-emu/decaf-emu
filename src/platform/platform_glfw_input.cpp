#ifdef DECAF_GLFW
#include <gsl.h>
#include <GLFW/glfw3.h>
#include "platform_glfw.h"

namespace platform
{

ControllerHandle
PlatformGLFW::getControllerHandle(const std::string &name)
{
   if (name.compare("keyboard") == 0) {
      glfwSetInputMode(mWindow, GLFW_STICKY_KEYS, GLFW_CURSOR_NORMAL);
      return mKeyboardHandle;
   }

   for (auto i = 0u; i < GLFW_JOYSTICK_LAST; ++i) {
      if (!glfwJoystickPresent(i)) {
         continue;
      }

      if (name.compare(glfwGetJoystickName(i)) == 0) {
         return mJoystickHandleStart + i;
      }
   }

   return 0;
}

void
PlatformGLFW::sampleController(ControllerHandle controller)
{
   if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      auto id = gsl::narrow_cast<int>(controller - mJoystickHandleStart);
      mJoystickData[id].buttons = glfwGetJoystickButtons(id, &mJoystickData[id].count);
   }
}

::input::ButtonStatus
PlatformGLFW::getButtonStatus(ControllerHandle controller, int key)
{
   if (controller == mKeyboardHandle) {
      if (glfwGetKey(mWindow, key)) {
         return ::input::ButtonPressed;
      }
   } else if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      auto joystickData = mJoystickData[controller - mJoystickHandleStart];

      if (joystickData.buttons && key < joystickData.count && joystickData.buttons[key]) {
         return ::input::ButtonPressed;
      }
   }

   return ::input::ButtonReleased;
}

int
PlatformGLFW::getPressedButton(ControllerHandle controller)
{
   if (controller == mKeyboardHandle) {
      for (auto key = 0; key < GLFW_KEY_LAST; ++key) {
         if (glfwGetKey(mWindow, key)) {
            return key;
         }
      }
   } else if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      auto joystickData = mJoystickData[controller - mJoystickHandleStart];

      if (joystickData.buttons) {
         for (auto key = 0; key < joystickData.count; ++key) {
            if (joystickData.buttons[key]) {
               return key;
            }
         }
      }
   }

   return -1;
}

} // namespace platform

#endif // ifdef DECAF_GLFW
