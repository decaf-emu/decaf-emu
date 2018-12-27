#include "decafsdl.h"

#include "clilog.h"
#include "config.h"

#include <common/decaf_assert.h>

void
DecafSDL::openInputDevices()
{
   mVpad0Config = nullptr;
   mVpad0Controller = nullptr;

   for (const auto &device : config::input::devices) {
      if (config::input::vpad0.compare(device.id) != 0) {
         continue;
      }

      if (device.type == config::input::Joystick) {
         auto numJoysticks = SDL_NumJoysticks();

         for (int i = 0; i < numJoysticks; ++i) {
            if (!SDL_IsGameController(i)) {
               continue;
            }

            auto controller = SDL_GameControllerOpen(i);

            if (!controller) {
               gCliLog->error("Failed to open game controller {}: {}", i, SDL_GetError());
               continue;
            }

            auto name = SDL_GameControllerName(controller);

            if (!device.device_name.empty() && device.device_name.compare(name) != 0) {
               SDL_GameControllerClose(controller);
               continue;
            }

            mVpad0Controller = controller;
            break;
         }

         if (!mVpad0Controller) {
            continue;
         }
      }

      mVpad0Config = &device;
      break;
   }

   if (!mVpad0Config) {
      gCliLog->warn("No input device found for gamepad (VPAD0)");
   }
}

void
DecafSDL::sampleVpadController(int channel, vpad::Status &status)
{
   auto device = mVpad0Config;
   if (!device || channel > 0 || channel < 0) {
      status.connected = false;
      return;
   }

   if (device->type == config::input::Keyboard) {
      auto numKeys = 0;
      const auto keyboardState = SDL_GetKeyboardState(&numKeys);
      const auto getState =
         [keyboardState, numKeys](int scancode) -> uint32_t {
            if (scancode < 0 || scancode > numKeys) {
               return 0;
            } else {
               return keyboardState[scancode] ? 1 : 0;
            }
         };

      status.connected = true;
      status.buttons.sync = getState(device->button_sync);
      status.buttons.home = getState(device->button_home);
      status.buttons.minus = getState(device->button_minus);
      status.buttons.plus = getState(device->button_plus);
      status.buttons.r = getState(device->button_trigger_r);
      status.buttons.l = getState(device->button_trigger_l);
      status.buttons.zr = getState(device->button_trigger_zr);
      status.buttons.zl = getState(device->button_trigger_zl);
      status.buttons.down = getState(device->button_down);
      status.buttons.up = getState(device->button_up);
      status.buttons.right = getState(device->button_right);
      status.buttons.left = getState(device->button_left);
      status.buttons.y = getState(device->button_y);
      status.buttons.x = getState(device->button_x);
      status.buttons.b = getState(device->button_b);
      status.buttons.a = getState(device->button_a);
      status.buttons.stickR = getState(device->button_stick_r);
      status.buttons.stickL = getState(device->button_stick_l);

      status.leftStickX = 0.0f;
      status.leftStickY = 0.0f;

      status.rightStickX = 0.0f;
      status.rightStickY = 0.0f;

      if (getState(device->keyboard.left_stick_up)) {
         status.leftStickY -= 1.0f;
      }

      if (getState(device->keyboard.left_stick_down)) {
         status.leftStickY += 1.0f;
      }

      if (getState(device->keyboard.left_stick_left)) {
         status.leftStickX -= 1.0f;
      }

      if (getState(device->keyboard.left_stick_right)) {
         status.leftStickX += 1.0f;
      }

      if (getState(device->keyboard.right_stick_up)) {
         status.rightStickY -= 1.0f;
      }

      if (getState(device->keyboard.right_stick_down)) {
         status.rightStickY += 1.0f;
      }

      if (getState(device->keyboard.right_stick_left)) {
         status.rightStickX -= 1.0f;
      }

      if (getState(device->keyboard.right_stick_right)) {
         status.rightStickX += 1.0f;
      }

      status.touch.down = false;

      int mouseX, mouseY;
      if (SDL_GetMouseState(&mouseX, &mouseY) & SDL_BUTTON(SDL_BUTTON_LEFT)) {
         // Calculate screen position
         Viewport tvViewport, drcViewport;
         int windowWidth, windowHeight;
         SDL_GetWindowSize(mGraphicsDriver->getWindow(), &windowWidth, &windowHeight);
         calculateScreenViewports(tvViewport, drcViewport);

         // Check that mouse is inside DRC screen
         auto drcLeft = drcViewport.x;
         auto drcBottom = windowHeight - drcViewport.y;
         auto drcRight = drcLeft + drcViewport.width;
         auto drcTop = drcBottom - drcViewport.height;

         if (mouseX >= drcLeft && mouseX <= drcRight && mouseY >= drcTop && mouseY <= drcBottom) {
            status.touch.down = true;
            status.touch.x = static_cast<int>((mouseX - drcLeft) / drcViewport.width);
            status.touch.y = static_cast<int>((mouseY - drcTop) / drcViewport.height);
         }
      }
   } else if (mVpad0Controller && device->type == config::input::Joystick) {
      auto controller = mVpad0Controller;
      auto joystick = SDL_GameControllerGetJoystick(controller);
      const auto getState =
         [controller, joystick](int button, SDL_GameControllerButton name) -> uint32_t {
            if (button >= 0) {
               return SDL_JoystickGetButton(joystick, button) ? 1 : 0;
            } else if (button == -2 && name != SDL_CONTROLLER_BUTTON_INVALID) {
               return SDL_GameControllerGetButton(controller, name);
            } else {
               return 0;
            }
         };
      const auto getAxis =
         [controller, joystick](int index, SDL_GameControllerAxis name) -> float {
            auto value = 0;
            if (index >= 0) {
               value = SDL_JoystickGetAxis(joystick, index) ? 1 : 0;
            } else if (index == -2 && name != SDL_CONTROLLER_AXIS_INVALID) {
               value = SDL_GameControllerGetAxis(controller, name);
            }

            if (value < 0) {
               return value / 32768.0f;
            } else {
               return value / 32767.0f;
            }
         };

      status.connected = true;
      status.buttons.sync = getState(device->button_sync, SDL_CONTROLLER_BUTTON_INVALID);
      status.buttons.home = getState(device->button_home, SDL_CONTROLLER_BUTTON_GUIDE);
      status.buttons.minus = getState(device->button_minus, SDL_CONTROLLER_BUTTON_BACK);
      status.buttons.plus = getState(device->button_plus, SDL_CONTROLLER_BUTTON_START);
      status.buttons.r = getState(device->button_trigger_r, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
      status.buttons.l = getState(device->button_trigger_l, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
      status.buttons.zr = getAxis(device->button_trigger_zr, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > 0.5f;
      status.buttons.zl = getAxis(device->button_trigger_zl, SDL_CONTROLLER_AXIS_TRIGGERLEFT) > 0.5f;
      status.buttons.down = getState(device->button_down, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
      status.buttons.up = getState(device->button_up, SDL_CONTROLLER_BUTTON_DPAD_UP);
      status.buttons.right = getState(device->button_right, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
      status.buttons.left = getState(device->button_left, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
      status.buttons.y = getState(device->button_y, SDL_CONTROLLER_BUTTON_Y);
      status.buttons.x = getState(device->button_x, SDL_CONTROLLER_BUTTON_X);
      status.buttons.stickR = getState(device->button_stick_r, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
      status.buttons.stickL = getState(device->button_stick_l, SDL_CONTROLLER_BUTTON_LEFTSTICK);

      // Yes A and B are purposefully swapped.
      status.buttons.b = getState(device->button_b, SDL_CONTROLLER_BUTTON_A);
      status.buttons.a = getState(device->button_a, SDL_CONTROLLER_BUTTON_B);

      status.leftStickX = getAxis(device->joystick.left_stick_x, SDL_CONTROLLER_AXIS_LEFTX);
      status.leftStickY = getAxis(device->joystick.left_stick_y, SDL_CONTROLLER_AXIS_LEFTY);

      if (device->joystick.left_stick_x_invert) {
         status.leftStickX = -status.leftStickX;
      }

      if (device->joystick.left_stick_y_invert) {
         status.leftStickY = -status.leftStickY;
      }

      status.rightStickX = getAxis(device->joystick.right_stick_x, SDL_CONTROLLER_AXIS_RIGHTX);
      status.rightStickY = getAxis(device->joystick.right_stick_y, SDL_CONTROLLER_AXIS_RIGHTY);

      if (device->joystick.right_stick_x_invert) {
         status.rightStickX = -status.rightStickX;
      }

      if (device->joystick.right_stick_y_invert) {
         status.rightStickY = -status.rightStickY;
      }
   } else {
      status.connected = false;
   }
}

void
DecafSDL::sampleWpadController(int channel, wpad::Status &status)
{
   status.type = wpad::BaseControllerType::Disconnected;
}
