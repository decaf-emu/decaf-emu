#include "config.h"

#include <SDL_keycode.h>

namespace config
{

namespace display
{

DisplayMode mode = DisplayMode::Windowed;
DisplayLayout layout = DisplayLayout::Split;
bool stretch = false;
bool force_sync = false;
Colour background_colour = { 153, 51, 51 };

} // namespace display

namespace input
{

std::vector<InputDevice> devices;
std::string vpad0 = "";

} // namespace input

namespace sound
{

unsigned frame_length = 30;

} // namespace sound

void
setupDefaultInputDevices()
{
   auto foundKeyboardDevice = false;
   auto foundJoystickDevice = false;

   for (auto &device : input::devices) {
      if (device.type == input::ControllerType::Keyboard) {
         foundKeyboardDevice = true;
      } else if (device.type == input::ControllerType::Joystick) {
         foundJoystickDevice = true;
      }
   }

   // Setup default keyboard device
   if (!foundKeyboardDevice) {
      input::InputDevice device;
      device.id = "default_keyboard";
      device.type = input::Keyboard;
      device.device_name = "";
      device.button_up = SDL_SCANCODE_UP;
      device.button_down = SDL_SCANCODE_DOWN;
      device.button_left = SDL_SCANCODE_LEFT;
      device.button_right = SDL_SCANCODE_RIGHT;
      device.button_a = SDL_SCANCODE_X;
      device.button_b = SDL_SCANCODE_Z;
      device.button_x = SDL_SCANCODE_S;
      device.button_y = SDL_SCANCODE_A;
      device.button_trigger_r = SDL_SCANCODE_E;
      device.button_trigger_l = SDL_SCANCODE_W;
      device.button_trigger_zr = SDL_SCANCODE_R;
      device.button_trigger_zl = SDL_SCANCODE_Q;
      device.button_stick_l = SDL_SCANCODE_KP_0;
      device.button_stick_r = SDL_SCANCODE_KP_ENTER;
      device.button_plus = SDL_SCANCODE_1;
      device.button_minus = SDL_SCANCODE_2;
      device.button_home = SDL_SCANCODE_3;
      device.button_sync = SDL_SCANCODE_4;
      device.keyboard.left_stick_up = SDL_SCANCODE_KP_8;
      device.keyboard.left_stick_down = SDL_SCANCODE_KP_2;
      device.keyboard.left_stick_left = SDL_SCANCODE_KP_4;
      device.keyboard.left_stick_right = SDL_SCANCODE_KP_6;
      device.keyboard.right_stick_up = -1;
      device.keyboard.right_stick_down = -1;
      device.keyboard.right_stick_left = -1;
      device.keyboard.right_stick_right = -1;
      input::devices.push_back(device);
   }

   // Setup default joystick device
   if (!foundJoystickDevice) {
      input::InputDevice device;
      device.id = "default_joystick";
      device.device_name = "";
      device.type = input::Joystick;
      device.button_up = -2;
      device.button_down = -2;
      device.button_left = -2;
      device.button_right = -2;
      device.button_a = -2;
      device.button_b = -2;
      device.button_x = -2;
      device.button_y = -2;
      device.button_trigger_r = -2;
      device.button_trigger_l = -2;
      device.button_trigger_zr = -2;
      device.button_trigger_zl = -2;
      device.button_stick_l = -2;
      device.button_stick_r = -2;
      device.button_plus = -2;
      device.button_minus = -2;
      device.button_home = -2;
      device.button_sync = -2;
      device.joystick.left_stick_x = -2;
      device.joystick.left_stick_x_invert = false;
      device.joystick.left_stick_y = -2;
      device.joystick.left_stick_y_invert = false;
      device.joystick.right_stick_x = -2;
      device.joystick.right_stick_x_invert = false;
      device.joystick.right_stick_y = -2;
      device.joystick.right_stick_y_invert = false;
      input::devices.push_back(device);
   }
}

bool
loadFrontendToml(std::string &error,
                 std::shared_ptr<cpptoml::table> config)
{
   auto devices = config->get_table_array_qualified("input.device");

   if (devices) {
      for (const auto &cfgDevice : *devices) {
         config::input::InputDevice device;
         auto type = cfgDevice->get_as<std::string>("type").value_or("");

         if (type == "keyboard") {
            device.type = config::input::ControllerType::Keyboard;
         } else if (type == "joystick") {
            device.type = config::input::ControllerType::Joystick;
         } else {
            continue;
         }

         device.id = cfgDevice->get_as<std::string>("id").value_or("");
         device.device_name = cfgDevice->get_as<std::string>("device_name").value_or("");
         device.button_up = cfgDevice->get_as<int>("button_up").value_or(-1);
         device.button_down = cfgDevice->get_as<int>("button_down").value_or(-1);
         device.button_left = cfgDevice->get_as<int>("button_left").value_or(-1);
         device.button_right = cfgDevice->get_as<int>("button_right").value_or(-1);
         device.button_a = cfgDevice->get_as<int>("button_a").value_or(-1);
         device.button_b = cfgDevice->get_as<int>("button_b").value_or(-1);
         device.button_x = cfgDevice->get_as<int>("button_x").value_or(-1);
         device.button_y = cfgDevice->get_as<int>("button_y").value_or(-1);
         device.button_trigger_r = cfgDevice->get_as<int>("button_trigger_r").value_or(-1);
         device.button_trigger_l = cfgDevice->get_as<int>("button_trigger_l").value_or(-1);
         device.button_trigger_zr = cfgDevice->get_as<int>("button_trigger_zr").value_or(-1);
         device.button_trigger_zl = cfgDevice->get_as<int>("button_trigger_zl").value_or(-1);
         device.button_stick_l = cfgDevice->get_as<int>("button_stick_l").value_or(-1);
         device.button_stick_r = cfgDevice->get_as<int>("button_stick_r").value_or(-1);
         device.button_plus = cfgDevice->get_as<int>("button_plus").value_or(-1);
         device.button_minus = cfgDevice->get_as<int>("button_minus").value_or(-1);
         device.button_home = cfgDevice->get_as<int>("button_home").value_or(-1);
         device.button_sync = cfgDevice->get_as<int>("button_sync").value_or(-1);

         if (device.type == config::input::ControllerType::Keyboard) {
            device.keyboard.left_stick_up = cfgDevice->get_as<int>("left_stick_up").value_or(-1);
            device.keyboard.left_stick_down = cfgDevice->get_as<int>("left_stick_down").value_or(-1);
            device.keyboard.left_stick_left = cfgDevice->get_as<int>("left_stick_left").value_or(-1);
            device.keyboard.left_stick_right = cfgDevice->get_as<int>("left_stick_right").value_or(-1);
            device.keyboard.right_stick_up = cfgDevice->get_as<int>("right_stick_up").value_or(-1);
            device.keyboard.right_stick_down = cfgDevice->get_as<int>("right_stick_down").value_or(-1);
            device.keyboard.right_stick_left = cfgDevice->get_as<int>("right_stick_left").value_or(-1);
            device.keyboard.right_stick_right = cfgDevice->get_as<int>("right_stick_right").value_or(-1);
         } else if (device.type == config::input::ControllerType::Joystick) {
            device.joystick.left_stick_x = cfgDevice->get_as<int>("left_stick_x").value_or(-1);
            device.joystick.left_stick_x_invert = cfgDevice->get_as<bool>("left_stick_x_invert").value_or(false);
            device.joystick.left_stick_y = cfgDevice->get_as<int>("left_stick_y").value_or(-1);
            device.joystick.left_stick_y_invert = cfgDevice->get_as<bool>("left_stick_y_invert").value_or(false);
            device.joystick.right_stick_x = cfgDevice->get_as<int>("right_stick_x").value_or(-1);
            device.joystick.right_stick_x_invert = cfgDevice->get_as<bool>("right_stick_x_invert").value_or(false);
            device.joystick.right_stick_y = cfgDevice->get_as<int>("right_stick_y").value_or(-1);
            device.joystick.right_stick_y_invert = cfgDevice->get_as<bool>("right_stick_y_invert").value_or(false);
         }

         config::input::devices.push_back(device);
      }
   }

   config::input::vpad0 = config->get_qualified_as<std::string>("input.vpad0").value_or(config::input::vpad0);

   if (auto bgColor = config->get_qualified_array_of<int64_t>("ui.background_colour")) {
      config::display::background_colour.r = static_cast<int>(bgColor->at(0));
      config::display::background_colour.g = static_cast<int>(bgColor->at(1));
      config::display::background_colour.b = static_cast<int>(bgColor->at(2));
   }

   return true;
}

} // namespace config
