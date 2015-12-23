#include "platform_sdl.h"
#include "platform_input.h"
#include "utils/log.h"
#include <SDL.h>
#include <SDL_gamecontroller.h>

namespace platform
{

namespace sdl {

void
handleEvent(const SDL_Event *event)
{
   switch (event->type) {
   case SDL_WINDOWEVENT:
      if (event->window.event == SDL_WINDOWEVENT_CLOSE) {
         shouldQuit = true;
      }
      break;

   case SDL_QUIT:
      shouldQuit = true;
      break;
   }
}

} // namespace sdl

namespace input
{

enum {
   MAX_SDL_JOYSTICKS = 16  // Arbitrary limit.
};

static const ControllerHandle keyboardHandle = 1;

static const ControllerHandle joystickHandleStart = 10;

static const ControllerHandle joystickHandleEnd = 10 + MAX_SDL_JOYSTICKS;

struct JoystickData
{
   SDL_Joystick *handle;
};
static JoystickData joystickData[MAX_SDL_JOYSTICKS];

static const uint8_t *keyboardState;
static int keyboardStateLen;


ControllerHandle
getControllerHandle(const std::string &name)
{
   if (name.compare("keyboard") == 0) {
      return keyboardHandle;
   }

   for (int i = 0; i < MAX_SDL_JOYSTICKS; ++i) {
      if (joystickData[i].handle) {
         if (name.compare(SDL_JoystickName(joystickData[i].handle)) == 0) {
            return joystickHandleStart + i;
         } else {
            continue;
         }
      }

      SDL_Joystick *joystick = SDL_JoystickOpen(i);
      if (!joystick) {
         continue;
      }

      if (name.compare(SDL_JoystickName(joystick)) == 0) {
         joystickData[i].handle = joystick;
         return joystickHandleStart + i;
      }

      SDL_JoystickClose(joystick);
   }

   return 0;
}

void
sampleController(ControllerHandle controller)
{
   if (controller == keyboardHandle) {
      keyboardState = SDL_GetKeyboardState(&keyboardStateLen);
   } else if (controller >= joystickHandleStart && controller < joystickHandleEnd) {
      SDL_JoystickUpdate();
   }
}

::input::ButtonStatus
getButtonStatus(ControllerHandle controller, int key)
{
   if (controller == keyboardHandle) {
      if (keyboardState) {
         const int scancode = SDL_GetScancodeFromKey(key);
         if (scancode >= 0 && scancode < keyboardStateLen
          && keyboardState[scancode]) {
            return ::input::ButtonPressed;
         }
      }
   } else if (controller >= joystickHandleStart && controller < joystickHandleEnd) {
      const int id = controller - joystickHandleStart;

      if (joystickData[id].handle) {
         if (key >= 0 && SDL_JoystickGetButton(joystickData[id].handle, key)) {
            return ::input::ButtonPressed;
         }
      }
   }

   return ::input::ButtonReleased;
}

} // namespace input

} // namespace platform
