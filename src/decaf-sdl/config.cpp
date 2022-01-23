#include "config.h"

#include <SDL_keycode.h>

namespace config
{

namespace input
{

std::vector<InputDevice> devices;
std::string vpad0 = "default_keyboard";

} // namespace input

namespace sound
{

unsigned frame_length = 30;

} // namespace sound

namespace test
{

//! Maximum time to run for before termination in milliseconds.
int timeout_ms = -1;

//! Maximum number of frames to render before termination.
int timeout_frames = -1;

//! Whether to dump rendered DRC frames to file;
bool dump_drc_frames = false;

//! Whether to dump rendered TV frames to file;
bool dump_tv_frames = false;

//! What directory to place dumped frames in.
std::string dump_frames_dir = "frames";

} // namespace test

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

      input::InputDeviceKeyboard keyboard;
      keyboard.left_stick_up = SDL_SCANCODE_KP_8;
      keyboard.left_stick_down = SDL_SCANCODE_KP_2;
      keyboard.left_stick_left = SDL_SCANCODE_KP_4;
      keyboard.left_stick_right = SDL_SCANCODE_KP_6;
      keyboard.right_stick_up = -1;
      keyboard.right_stick_down = -1;
      keyboard.right_stick_left = -1;
      keyboard.right_stick_right = -1;
      device.typeExtra = keyboard;

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

      input::InputDeviceJoystick joystick;
      joystick.left_stick_x = -2;
      joystick.left_stick_x_invert = false;
      joystick.left_stick_y = -2;
      joystick.left_stick_y_invert = false;
      joystick.right_stick_x = -2;
      joystick.right_stick_x_invert = false;
      joystick.right_stick_y = -2;
      joystick.right_stick_y_invert = false;
      device.typeExtra = joystick;

      input::devices.push_back(device);
   }
}

bool
loadFrontendToml(const toml::table &config)
{
   // input
   auto devices = config.at_path("input.devices").as_array();
   input::devices.clear();

   if (devices) {
      for (auto itr = devices->begin(); itr != devices->end(); ++itr) {
         auto cfgDevice = itr->as_table();
         config::input::InputDevice device;

         auto type = cfgDevice->get_as<std::string>("type");
         if (!type) {
            continue;
         } else if (type->get() == "keyboard") {
            device.type = config::input::ControllerType::Keyboard;
         } else if (type->get() == "joystick") {
            device.type = config::input::ControllerType::Joystick;
         } else {
            continue;
         }

         readValue(*cfgDevice, "id", device.id);
         readValue(*cfgDevice, "device_name", device.device_name);
         readValue(*cfgDevice, "button_up", device.button_up);
         readValue(*cfgDevice, "button_down", device.button_down);
         readValue(*cfgDevice, "button_left", device.button_left);
         readValue(*cfgDevice, "button_right", device.button_right);
         readValue(*cfgDevice, "button_a", device.button_a);
         readValue(*cfgDevice, "button_b", device.button_b);
         readValue(*cfgDevice, "button_x", device.button_x);
         readValue(*cfgDevice, "button_y", device.button_y);
         readValue(*cfgDevice, "button_trigger_r", device.button_trigger_r);
         readValue(*cfgDevice, "button_trigger_l", device.button_trigger_l);
         readValue(*cfgDevice, "button_trigger_zr", device.button_trigger_zr);
         readValue(*cfgDevice, "button_trigger_zl", device.button_trigger_zl);
         readValue(*cfgDevice, "button_stick_l", device.button_stick_l);
         readValue(*cfgDevice, "button_stick_r", device.button_stick_r);
         readValue(*cfgDevice, "button_plus", device.button_plus);
         readValue(*cfgDevice, "button_minus", device.button_minus);
         readValue(*cfgDevice, "button_home", device.button_home);
         readValue(*cfgDevice, "button_sync", device.button_sync);

         if (device.type == config::input::ControllerType::Keyboard) {
            auto keyboard = config::input::InputDeviceKeyboard {};
            readValue(*cfgDevice, "left_stick_up", keyboard.left_stick_up);
            readValue(*cfgDevice, "left_stick_down", keyboard.left_stick_down);
            readValue(*cfgDevice, "left_stick_left", keyboard.left_stick_left);
            readValue(*cfgDevice, "left_stick_right", keyboard.left_stick_right);

            readValue(*cfgDevice, "right_stick_up", keyboard.right_stick_up);
            readValue(*cfgDevice, "right_stick_down", keyboard.right_stick_down);
            readValue(*cfgDevice, "right_stick_left", keyboard.right_stick_left);
            readValue(*cfgDevice, "right_stick_right", keyboard.right_stick_right);
            device.typeExtra = keyboard;
         } else if (device.type == config::input::ControllerType::Joystick) {
            auto joystick = config::input::InputDeviceJoystick {};
            readValue(*cfgDevice, "left_stick_x", joystick.left_stick_x);
            readValue(*cfgDevice, "left_stick_x_invert", joystick.left_stick_x_invert);
            readValue(*cfgDevice, "left_stick_y", joystick.left_stick_y);
            readValue(*cfgDevice, "left_stick_y_invert", joystick.left_stick_y_invert);

            readValue(*cfgDevice, "right_stick_x", joystick.right_stick_x);
            readValue(*cfgDevice, "right_stick_x_invert", joystick.right_stick_x_invert);
            readValue(*cfgDevice, "right_stick_y", joystick.right_stick_y);
            readValue(*cfgDevice, "right_stick_y_invert", joystick.right_stick_y_invert);
            device.typeExtra = joystick;
         }

         config::input::devices.push_back(device);
      }
   }

   readValue(config, "input.vpad0", config::input::vpad0);
   setupDefaultInputDevices();

   // sound
   readValue(config, "sound.frame_length", config::sound::frame_length);

   // test
   readValue(config, "test.timeout_ms", config::test::timeout_ms);
   readValue(config, "test.timeout_frames", config::test::timeout_frames);
   readValue(config, "test.dump_drc_frames", config::test::dump_drc_frames);
   readValue(config, "test.dump_tv_frames", config::test::dump_tv_frames);
   readValue(config, "test.dump_frames_dir", config::test::dump_frames_dir);
   return true;
}

bool
saveFrontendToml(toml::table &config)
{
   setupDefaultInputDevices();

   // input
   auto input = config.insert("input", toml::table()).first->second.as_table();
   input->insert("vpad0", config::input::vpad0);

   auto devices = config.insert("devices", toml::array()).first->second.as_array();
   for (const auto &device : config::input::devices) {
      auto cfgDevice = toml::table();

      if (device.type == config::input::ControllerType::Joystick) {
         cfgDevice.insert("type", "joystick");
      } else if (device.type == config::input::ControllerType::Keyboard) {
         cfgDevice.insert("type", "keyboard");
      } else {
         cfgDevice.insert("type", "unknown");
      }

      cfgDevice.insert("id", device.id);
      cfgDevice.insert("device_name", device.device_name);
      cfgDevice.insert("button_up", device.button_up);
      cfgDevice.insert("button_down", device.button_down);
      cfgDevice.insert("button_left", device.button_left);
      cfgDevice.insert("button_right", device.button_right);
      cfgDevice.insert("button_a", device.button_a);
      cfgDevice.insert("button_b", device.button_b);
      cfgDevice.insert("button_x", device.button_x);
      cfgDevice.insert("button_y", device.button_y);
      cfgDevice.insert("button_trigger_r", device.button_trigger_r);
      cfgDevice.insert("button_trigger_l", device.button_trigger_l);
      cfgDevice.insert("button_trigger_zr", device.button_trigger_zr);
      cfgDevice.insert("button_trigger_zl", device.button_trigger_zl);
      cfgDevice.insert("button_stick_l", device.button_stick_l);
      cfgDevice.insert("button_stick_r", device.button_stick_r);
      cfgDevice.insert("button_plus", device.button_plus);
      cfgDevice.insert("button_minus", device.button_minus);
      cfgDevice.insert("button_home", device.button_home);
      cfgDevice.insert("button_sync", device.button_sync);

      if (device.type == config::input::ControllerType::Keyboard) {
         const auto &keyboard =
            std::get<config::input::InputDeviceKeyboard>(device.typeExtra);
         cfgDevice.insert("left_stick_up", keyboard.left_stick_up);
         cfgDevice.insert("left_stick_down", keyboard.left_stick_down);
         cfgDevice.insert("left_stick_left", keyboard.left_stick_left);
         cfgDevice.insert("left_stick_right", keyboard.left_stick_right);
         cfgDevice.insert("right_stick_up", keyboard.right_stick_up);
         cfgDevice.insert("right_stick_down", keyboard.right_stick_down);
         cfgDevice.insert("right_stick_left", keyboard.right_stick_left);
         cfgDevice.insert("right_stick_right", keyboard.right_stick_right);
      } else if (device.type == config::input::ControllerType::Joystick) {
         const auto &joystick =
            std::get<config::input::InputDeviceJoystick>(device.typeExtra);
         cfgDevice.insert("left_stick_x", joystick.left_stick_x);
         cfgDevice.insert("left_stick_x_invert", joystick.left_stick_x_invert);
         cfgDevice.insert("left_stick_y", joystick.left_stick_y);
         cfgDevice.insert("left_stick_y_invert", joystick.left_stick_y_invert);
         cfgDevice.insert("right_stick_x", joystick.right_stick_x);
         cfgDevice.insert("right_stick_x_invert", joystick.right_stick_x_invert);
         cfgDevice.insert("right_stick_y", joystick.right_stick_y);
         cfgDevice.insert("right_stick_y_invert", joystick.right_stick_y_invert);
      }

      devices->push_back(cfgDevice);
   }

   // sound
   auto sound = config.insert("sound", toml::table()).first->second.as_table();
   sound->insert("frame_length", config::sound::frame_length);

   // test
   auto test = config.insert("test", toml::table()).first->second.as_table();
   test->insert("timeout_ms", config::test::timeout_ms);
   test->insert("timeout_frames", config::test::timeout_frames);
   test->insert("dump_drc_frames", config::test::dump_drc_frames);
   test->insert("dump_tv_frames", config::test::dump_tv_frames);
   test->insert("dump_frames_dir", config::test::dump_frames_dir);
   return true;
}

} // namespace config
