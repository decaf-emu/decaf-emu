#pragma once
#include "libdecaf/decaf_input.h"

using namespace decaf::input;

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
getAxisMapping(vpad::Channel channel, vpad::Core axis)
{
   switch (axis) {
   case vpad::Core::LeftStickX:
      return config::input::vpad0::left_stick_x;
   case vpad::Core::LeftStickY:
      return config::input::vpad0::left_stick_y;
   case vpad::Core::RightStickX:
      return config::input::vpad0::right_stick_x;
   case vpad::Core::RightStickY:
      return config::input::vpad0::right_stick_y;
   }

   return -1;
}
