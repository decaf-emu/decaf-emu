#include "decafsdl.h"
#include "config.h"

static int
getButtonMapping(vpad::Channel channel, vpad::Core button)
{
   switch (button) {
   case vpad::Core::Up:
      return config::input::vpad0::button_up;
   case vpad::Core::Down:
      return config::input::vpad0::button_down;
   case vpad::Core::Left:
      return config::input::vpad0::button_left;
   case vpad::Core::Right:
      return config::input::vpad0::button_right;
   case vpad::Core::A:
      return config::input::vpad0::button_a;
   case vpad::Core::B:
      return config::input::vpad0::button_b;
   case vpad::Core::X:
      return config::input::vpad0::button_x;
   case vpad::Core::Y:
      return config::input::vpad0::button_y;
   case vpad::Core::TriggerR:
      return config::input::vpad0::button_trigger_r;
   case vpad::Core::TriggerL:
      return config::input::vpad0::button_trigger_l;
   case vpad::Core::TriggerZR:
      return config::input::vpad0::button_trigger_zr;
   case vpad::Core::TriggerZL:
      return config::input::vpad0::button_trigger_zl;
   case vpad::Core::LeftStick:
      return config::input::vpad0::button_stick_l;
   case vpad::Core::RightStick:
      return config::input::vpad0::button_stick_r;
   case vpad::Core::Plus:
      return config::input::vpad0::button_plus;
   case vpad::Core::Minus:
      return config::input::vpad0::button_minus;
   case vpad::Core::Home:
      return config::input::vpad0::button_home;
   case vpad::Core::Sync:
      return config::input::vpad0::button_sync;
   }

   return -1;
}

static int
getAxisMapping(vpad::Channel channel, vpad::CoreAxis axis)
{
   switch (axis) {
   case vpad::CoreAxis::LeftStickX:
      return config::input::vpad0::left_stick_x;
   case vpad::CoreAxis::LeftStickY:
      return config::input::vpad0::left_stick_y;
   case vpad::CoreAxis::RightStickX:
      return config::input::vpad0::right_stick_x;
   case vpad::CoreAxis::RightStickY:
      return config::input::vpad0::right_stick_y;
   }

   return -1;
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
   int numKeys = 0;
   auto scancode = getButtonMapping(channel, button);
   auto state = SDL_GetKeyboardState(&numKeys);

   if (scancode >= 0 && scancode < numKeys && state[scancode]) {
      return ButtonStatus::ButtonPressed;
   }

   return ButtonStatus::ButtonReleased;
}

float
DecafSDL::getAxisValue(vpad::Channel channel, vpad::CoreAxis axis)
{
   return 0.0f;
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
