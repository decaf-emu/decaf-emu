#include "clilog.h"
#include "common/decaf_assert.h"
#include "config.h"
#include "decafsdl.h"

static int
getKeyboardButtonMapping(const config::input::InputDevice *device,
                         vpad::Channel channel,
                         vpad::Core button)
{
   decaf_check(device);

   switch (button) {
   case vpad::Core::Up:
      return device->button_up;
   case vpad::Core::Down:
      return device->button_down;
   case vpad::Core::Left:
      return device->button_left;
   case vpad::Core::Right:
      return device->button_right;
   case vpad::Core::A:
      return device->button_a;
   case vpad::Core::B:
      return device->button_b;
   case vpad::Core::X:
      return device->button_x;
   case vpad::Core::Y:
      return device->button_y;
   case vpad::Core::TriggerR:
      return device->button_trigger_r;
   case vpad::Core::TriggerL:
      return device->button_trigger_l;
   case vpad::Core::TriggerZR:
      return device->button_trigger_zr;
   case vpad::Core::TriggerZL:
      return device->button_trigger_zl;
   case vpad::Core::LeftStick:
      return device->button_stick_l;
   case vpad::Core::RightStick:
      return device->button_stick_r;
   case vpad::Core::Plus:
      return device->button_plus;
   case vpad::Core::Minus:
      return device->button_minus;
   case vpad::Core::Home:
      return device->button_home;
   case vpad::Core::Sync:
      return device->button_sync;
   }

   return -1;
}

static int
getKeyboardAxisMapping(const config::input::InputDevice *device,
                       vpad::Channel channel,
                       vpad::CoreAxis axis,
                       bool leftOrDown)
{
   decaf_check(device);

   switch (axis) {
   case vpad::CoreAxis::LeftStickX:
      return leftOrDown ? device->keyboard.left_stick_left : device->keyboard.left_stick_right;
   case vpad::CoreAxis::LeftStickY:
      return leftOrDown ? device->keyboard.left_stick_down : device->keyboard.left_stick_up;
   case vpad::CoreAxis::RightStickX:
      return leftOrDown ? device->keyboard.right_stick_left : device->keyboard.right_stick_right;
   case vpad::CoreAxis::RightStickY:
      return leftOrDown ? device->keyboard.right_stick_down : device->keyboard.right_stick_up;
   }

   return -1;
}

static bool
getJoystickButtonState(const config::input::InputDevice *device,
                       SDL_GameController *controller,
                       vpad::Channel channel, 
                       vpad::Core button)
{
   decaf_check(device);
   decaf_check(controller);

   SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
   decaf_check(joystick);

   int index = -1;
   SDL_GameControllerButton name = SDL_CONTROLLER_BUTTON_INVALID;
   // SDL has no concept of ZR/ZL "buttons" (only axes) so we have to
   //  kludge around...
   SDL_GameControllerAxis axisName = SDL_CONTROLLER_AXIS_INVALID;

   switch (button) {
   case vpad::Core::Up:
      index = device->button_up;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::Down:
      index = device->button_down;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::Left:
      index = device->button_left;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::Right:
      index = device->button_right;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::A:
      index = device->button_a;
      name = SDL_CONTROLLER_BUTTON_B;  // SDL uses stupid Microsoft mapping :P
      break;
   case vpad::Core::B:
      index = device->button_b;
      name = SDL_CONTROLLER_BUTTON_A;
      break;
   case vpad::Core::X:
      index = device->button_x;
      name = SDL_CONTROLLER_BUTTON_Y;
      break;
   case vpad::Core::Y:
      index = device->button_y;
      name = SDL_CONTROLLER_BUTTON_X;
      break;
   case vpad::Core::TriggerR:
      index = device->button_trigger_r;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::TriggerL:
      index = device->button_trigger_l;
      name = SDL_CONTROLLER_BUTTON_DPAD_UP;
      break;
   case vpad::Core::TriggerZR:
      index = device->button_trigger_zr;
      axisName = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
      break;
   case vpad::Core::TriggerZL:
      index = device->button_trigger_zl;
      axisName = SDL_CONTROLLER_AXIS_TRIGGERLEFT;
      break;
   case vpad::Core::LeftStick:
      index = device->button_stick_l;
      name = SDL_CONTROLLER_BUTTON_LEFTSTICK;
      break;
   case vpad::Core::RightStick:
      index = device->button_stick_r;
      name = SDL_CONTROLLER_BUTTON_RIGHTSTICK;
      break;
   case vpad::Core::Plus:
      index = device->button_plus;
      name = SDL_CONTROLLER_BUTTON_START;
      break;
   case vpad::Core::Minus:
      index = device->button_minus;
      name = SDL_CONTROLLER_BUTTON_BACK;
      break;
   case vpad::Core::Home:
      index = device->button_home;
      name = SDL_CONTROLLER_BUTTON_GUIDE;
      break;
   case vpad::Core::Sync:
      index = device->button_sync;
      break;
   }

   if (index >= 0) {
      return SDL_JoystickGetButton(joystick, index);
   } else if (index == -2) {
      if (name != SDL_CONTROLLER_BUTTON_INVALID) {
         return SDL_GameControllerGetButton(controller, name);
      } else if (axisName != SDL_CONTROLLER_AXIS_INVALID) {  // ZL/ZR kludge
         int value = SDL_GameControllerGetAxis(controller, axisName);
         return value >= 16384;
      }
   }

   return false;
}

static int
getJoystickAxisState(const config::input::InputDevice *device,
                     SDL_GameController *controller,
                     vpad::Channel channel,
                     vpad::CoreAxis axis)
{
   decaf_check(device);
   decaf_check(controller);

   SDL_Joystick *joystick = SDL_GameControllerGetJoystick(controller);
   decaf_check(joystick);

   int index = -1;
   SDL_GameControllerAxis name = SDL_CONTROLLER_AXIS_INVALID;
   bool invert = false;

   switch (axis) {
   case vpad::CoreAxis::LeftStickX:
      index = device->joystick.left_stick_x;
      name = SDL_CONTROLLER_AXIS_LEFTX;
      invert = device->joystick.left_stick_x_invert;
      break;
   case vpad::CoreAxis::LeftStickY:
      index = device->joystick.left_stick_y;
      name = SDL_CONTROLLER_AXIS_LEFTY;
      invert = device->joystick.left_stick_y_invert;
      break;
   case vpad::CoreAxis::RightStickX:
      index = device->joystick.right_stick_x;
      name = SDL_CONTROLLER_AXIS_RIGHTX;
      invert = device->joystick.right_stick_x_invert;
      break;
   case vpad::CoreAxis::RightStickY:
      index = device->joystick.right_stick_y;
      name = SDL_CONTROLLER_AXIS_RIGHTY;
      invert = device->joystick.right_stick_y_invert;
      break;
   }

   int value = 0;
   if (index >= 0) {
      value = SDL_JoystickGetAxis(joystick, index);
   } else if (index == -2) {
      if (name != SDL_CONTROLLER_AXIS_INVALID) {
         value = SDL_GameControllerGetAxis(controller, name);
      }
   }

   if (invert) {
      value = -value;
   }

   return value;
}

void
DecafSDL::openInputDevices()
{
   mVpad0Config = nullptr;
   mVpad0Controller = nullptr;

   for (const auto &device : config::input::devices) {
      if (device.type != config::input::vpad0::type) {
         continue;
      }

      if (!config::input::vpad0::name.empty()
       && config::input::vpad0::name.compare(device.name) != 0) {
         continue;
      }

      if (device.type == config::input::Joystick) {
         int numJoysticks = SDL_NumJoysticks();

         for (int i = 0; i < numJoysticks; ++i) {
            if (!SDL_IsGameController(i)) {
               continue;
            }

            SDL_GameController *controller = SDL_GameControllerOpen(i);
            if (!controller) {
               gCliLog->error("Failed to open game controller {}: {}", i, SDL_GetError());
               continue;
            }

            if (!device.name.empty()
             && device.name.compare(SDL_GameControllerName(controller)) != 0) {
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

decaf::input::MouseButton
DecafSDL::translateMouseButton(int button)
{
   switch (button) {
   case SDL_BUTTON_LEFT:
      return decaf::input::MouseButton::Left;
   case SDL_BUTTON_RIGHT:
      return decaf::input::MouseButton::Right;
   case SDL_BUTTON_MIDDLE:
      return decaf::input::MouseButton::Middle;
   default:
      return decaf::input::MouseButton::Unknown;
   }
}

decaf::input::KeyboardKey
DecafSDL::translateKeyCode(SDL_Keysym sym)
{
   switch (sym.sym) {
   case SDLK_TAB:
      return decaf::input::KeyboardKey::Tab;
   case SDLK_LEFT:
      return decaf::input::KeyboardKey::LeftArrow;
   case SDLK_RIGHT:
      return decaf::input::KeyboardKey::RightArrow;
   case SDLK_UP:
      return decaf::input::KeyboardKey::UpArrow;
   case SDLK_DOWN:
      return decaf::input::KeyboardKey::DownArrow;
   case SDLK_PAGEUP:
      return decaf::input::KeyboardKey::PageUp;
   case SDLK_PAGEDOWN:
      return decaf::input::KeyboardKey::PageDown;
   case SDLK_HOME:
      return decaf::input::KeyboardKey::Home;
   case SDLK_END:
      return decaf::input::KeyboardKey::End;
   case SDLK_DELETE:
      return decaf::input::KeyboardKey::Delete;
   case SDLK_BACKSPACE:
      return decaf::input::KeyboardKey::Backspace;
   case SDLK_RETURN:
      return decaf::input::KeyboardKey::Enter;
   case SDLK_ESCAPE:
      return decaf::input::KeyboardKey::Escape;
   case SDLK_LCTRL:
      return decaf::input::KeyboardKey::LeftControl;
   case SDLK_RCTRL:
      return decaf::input::KeyboardKey::RightControl;
   case SDLK_LSHIFT:
      return decaf::input::KeyboardKey::LeftShift;
   case SDLK_RSHIFT:
      return decaf::input::KeyboardKey::RightShift;
   case SDLK_LALT:
      return decaf::input::KeyboardKey::LeftAlt;
   case SDLK_RALT:
      return decaf::input::KeyboardKey::RightAlt;
   case SDLK_LGUI:
      return decaf::input::KeyboardKey::LeftSuper;
   case SDLK_RGUI:
      return decaf::input::KeyboardKey::RightSuper;
   default:
      if (sym.sym >= SDLK_a && sym.sym <= SDLK_z) {
         auto id = (sym.sym - SDLK_a) + static_cast<int>(decaf::input::KeyboardKey::A);
         return static_cast<decaf::input::KeyboardKey>(id);
      } else if (sym.sym >= SDLK_F1 && sym.sym <= SDLK_F12) {
         auto id = (sym.sym - SDLK_F1) + static_cast<int>(decaf::input::KeyboardKey::F1);
         return static_cast<decaf::input::KeyboardKey>(id);
      }

      return decaf::input::KeyboardKey::Unknown;
   }
}

// VPAD
vpad::Type
DecafSDL::getControllerType(vpad::Channel channel)
{
   return vpad::Type::DRC;
}

ButtonStatus
DecafSDL::getButtonStatus(vpad::Channel channel, vpad::Core button)
{
   if (!mVpad0Config) {
      return ButtonStatus::ButtonReleased;
   }

   switch (mVpad0Config->type) {
   case config::input::None:
      break;

   case config::input::Keyboard:
   {
      int numKeys = 0;
      auto scancode = getKeyboardButtonMapping(mVpad0Config, channel, button);
      auto state = SDL_GetKeyboardState(&numKeys);

      if (scancode >= 0 && scancode < numKeys && state[scancode]) {
         return ButtonStatus::ButtonPressed;
      }

      break;
   }

   case config::input::Joystick:
      if (mVpad0Controller && SDL_GameControllerGetAttached(mVpad0Controller)) {
         if (getJoystickButtonState(mVpad0Config, mVpad0Controller, channel, button)) {
            return ButtonStatus::ButtonPressed;
         }
      }
      break;
   }

   return ButtonStatus::ButtonReleased;
}

float
DecafSDL::getAxisValue(vpad::Channel channel, vpad::CoreAxis axis)
{
   if (!mVpad0Config) {
      return 0.0f;
   }

   switch (mVpad0Config->type) {
   case config::input::None:
      break;

   case config::input::Keyboard:
   {
      int numKeys = 0;
      auto scancodeMinus = getKeyboardAxisMapping(mVpad0Config, channel, axis, true);
      auto scancodePlus = getKeyboardAxisMapping(mVpad0Config, channel, axis, false);
      auto state = SDL_GetKeyboardState(&numKeys);

      float result = 0.0f;
      if (scancodeMinus >= 0 && scancodeMinus < numKeys && state[scancodeMinus]) {
         result -= 1.0f;
      }
      if (scancodePlus >= 0 && scancodePlus < numKeys && state[scancodePlus]) {
         result += 1.0f;
      }

      return result;
   }

   case config::input::Joystick:
      if (mVpad0Controller && SDL_GameControllerGetAttached(mVpad0Controller)) {
         int value = getJoystickAxisState(mVpad0Config, mVpad0Controller, channel, axis);
         if (value < 0) {
            return value / 32768.0f;
         } else {
            return value / 32767.0f;
         }
      }

      break;
   }

   return 0.0f;
}

bool
DecafSDL::getTouchPosition(vpad::Channel channel, vpad::TouchPosition &position)
{
   int mouseX, mouseY;
   auto mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);

   // Only look for touch if left mouse button is pressed
   if (!(mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT))) {
      return false;
   }

   // Calculate screen position
   float tvViewport[4], drcViewport[4];
   int windowWidth, windowHeight;
   SDL_GetWindowSize(mWindow, &windowWidth, &windowHeight);
   calculateScreenViewports(tvViewport, drcViewport);

   auto drcWidth = drcViewport[2];
   auto drcHeight = drcViewport[3];
   auto drcLeft = drcViewport[0];
   auto drcBottom = windowHeight - drcViewport[1];
   auto drcRight = drcLeft + drcWidth;
   auto drcTop = drcBottom - drcHeight;

   // Check that mouse is inside DRC screen
   if (mouseX >= drcLeft && mouseX <= drcRight && mouseY >= drcTop && mouseY <= drcBottom) {
      position.x = (mouseX - drcLeft) / drcWidth;
      position.y = (mouseY - drcTop) / drcHeight;
      return true;
   }

   return false;
}

// WPAD
wpad::Type
DecafSDL::getControllerType(wpad::Channel channel)
{
   return wpad::Type::Disconnected;
}

ButtonStatus
DecafSDL::getButtonStatus(wpad::Channel channel, wpad::Core button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
DecafSDL::getButtonStatus(wpad::Channel channel, wpad::Classic button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
DecafSDL::getButtonStatus(wpad::Channel channel, wpad::Nunchuck button)
{
   return ButtonStatus::ButtonReleased;
}

ButtonStatus
DecafSDL::getButtonStatus(wpad::Channel channel, wpad::Pro button)
{
   return ButtonStatus::ButtonReleased;
}

float
DecafSDL::getAxisValue(wpad::Channel channel, wpad::NunchuckAxis axis)
{
   return 0.0f;
}

float
DecafSDL::getAxisValue(wpad::Channel channel, wpad::ProAxis axis)
{
   return 0.0f;
}
