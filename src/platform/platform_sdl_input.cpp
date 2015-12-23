#ifdef DECAF_SDL
#include <gsl.h>
#include <SDL.h>
#include <SDL_gamecontroller.h>
#include "platform_sdl.h"
#include "platform_input.h"
#include "utils/log.h"

namespace platform
{

ControllerHandle
PlatformSDL::getControllerHandle(const std::string &name)
{
   if (name.compare("keyboard") == 0) {
      return mKeyboardHandle;
   }

   for (int i = 0; i < mJoystickData.size(); ++i) {
      auto &joystickData = mJoystickData[i];

      if (joystickData.handle) {
         if (name.compare(SDL_JoystickName(joystickData.handle)) == 0) {
            return mJoystickHandleStart + i;
         } else {
            continue;
         }
      }

      auto joystick = SDL_JoystickOpen(i);

      if (!joystick) {
         continue;
      }

      if (name.compare(SDL_JoystickName(joystickData.handle)) == 0) {
         joystickData.handle = joystick;
         return mJoystickHandleStart + i;
      }

      SDL_JoystickClose(joystick);
   }

   return 0;
}

void
PlatformSDL::sampleController(ControllerHandle controller)
{
   if (controller == mKeyboardHandle) {
      mKeyboardData.state = SDL_GetKeyboardState(&mKeyboardData.length);
   } else if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      SDL_JoystickUpdate();
   }
}

::input::ButtonStatus
PlatformSDL::getButtonStatus(ControllerHandle controller, int key)
{
   if (controller == mKeyboardHandle) {
      if (mKeyboardData.state) {
         const int scancode = SDL_GetScancodeFromKey(key);

         if (scancode >= 0 && scancode < mKeyboardData.length && mKeyboardData.state[scancode]) {
            return ::input::ButtonPressed;
         }
      }
   } else if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      const int id = gsl::narrow_cast<int>(controller - mJoystickHandleStart);

      if (mJoystickData[id].handle) {
         if (key >= 0 && SDL_JoystickGetButton(mJoystickData[id].handle, key)) {
            return ::input::ButtonPressed;
         }
      }
   }

   return ::input::ButtonReleased;
}

int
PlatformSDL::getPressedButton(ControllerHandle controller)
{
   if (controller == mKeyboardHandle) {
      if (mKeyboardData.state) {
         for (auto key = 0; key < mKeyboardData.length; ++key) {
            if (mKeyboardData.state[key]) {
               return key;
            }
         }
      }
   } else if (controller >= mJoystickHandleStart && controller < mJoystickHandleEnd) {
      auto joystickData = mJoystickData[controller - mJoystickHandleStart];
      auto count = SDL_JoystickNumButtons(joystickData.handle);

      if (joystickData.handle) {
         for (auto key = 0; key < count; ++key) {
            if (SDL_JoystickGetButton(joystickData.handle, key)) {
               return key;
            }
         }
      }
   }

   return -1;
}

} // namespace platform

#endif // ifdef DECAF_SDL
