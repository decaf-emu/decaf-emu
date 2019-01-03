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
std::string backend = "opengl";

} // namespace display

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

static display::DisplayMode
translateDisplayMode(const std::string &str)
{
   if (str == "windowed") {
      return display::DisplayMode::Windowed;
   } else if (str == "fullscreen") {
      return display::DisplayMode::Fullscreen;
   } else {
      return display::DisplayMode::Windowed; // Default to windowed
   }
}

static std::string
translateDisplayMode(display::DisplayMode mode)
{
   switch (mode) {
   case display::DisplayMode::Windowed:
      return "windowed";
   case display::DisplayMode::Fullscreen:
      return "fullscreen";
   default:
      return "windowed"; // Default to windowed
   }
}

static display::DisplayLayout
translateDisplayLayout(const std::string &str)
{
   if (str == "split") {
      return display::DisplayLayout::Split;
   } else if (str == "toggle") {
      return display::DisplayLayout::Toggle;
   } else {
      return display::DisplayLayout::Split; // Default to split
   }
}

static std::string
translateDisplayLayout(display::DisplayLayout layout)
{
   switch (layout) {
   case display::DisplayLayout::Split:
      return "split";
   case display::DisplayLayout::Toggle:
      return "toggle";
   default:
      return "split"; // Default to split
   }
}

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
loadFrontendToml(std::shared_ptr<cpptoml::table> config)
{
   // display
   config::display::mode = translateDisplayMode(config->get_qualified_as<std::string>("display.mode").value_or(translateDisplayMode(config::display::mode)));
   config::display::layout = translateDisplayLayout(config->get_qualified_as<std::string>("display.layout").value_or(translateDisplayLayout(config::display::layout)));
   config::display::force_sync = config->get_qualified_as<bool>("display.force_sync").value_or(config::display::force_sync);
   config::display::stretch = config->get_qualified_as<bool>("display.stretch").value_or(config::display::stretch);
   config::display::backend = config->get_qualified_as<std::string>("display.backend").value_or(config::display::backend);

   if (auto bgColor = config->get_qualified_array_of<int64_t>("display.background_colour")) {
      config::display::background_colour.r = static_cast<int>(bgColor->at(0));
      config::display::background_colour.g = static_cast<int>(bgColor->at(1));
      config::display::background_colour.b = static_cast<int>(bgColor->at(2));
   }

   // input
   auto devices = config->get_table_array_qualified("input.devices");
   input::devices.clear();

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
   setupDefaultInputDevices();

   // sound
   config::sound::frame_length = config->get_qualified_as<unsigned int>("sound.frame_length").value_or(config::sound::frame_length);

   // test
   config::test::timeout_ms = config->get_qualified_as<int>("test.timeout_ms").value_or(config::test::timeout_ms);
   config::test::timeout_frames = config->get_qualified_as<int>("test.timeout_frames").value_or(config::test::timeout_frames);
   config::test::dump_drc_frames = config->get_qualified_as<bool>("test.dump_drc_frames").value_or(config::test::dump_drc_frames);
   config::test::dump_tv_frames = config->get_qualified_as<bool>("test.dump_tv_frames").value_or(config::test::dump_tv_frames);
   config::test::dump_frames_dir = config->get_qualified_as<std::string>("test.dump_frames_dir").value_or(config::test::dump_frames_dir);
   return true;
}

bool
saveFrontendToml(std::shared_ptr<cpptoml::table> config)
{
   setupDefaultInputDevices();

   // display
   auto display = config->get_table("display");
   if (!display) {
      display = cpptoml::make_table();
   }

   display->insert("mode", translateDisplayMode(config::display::mode));
   display->insert("layout", translateDisplayLayout(config::display::layout));
   display->insert("force_sync", config::display::force_sync);
   display->insert("stretch", config::display::stretch);
   display->insert("backend", config::display::backend);

   auto background_colour = cpptoml::make_array();
   background_colour->push_back(config::display::background_colour.r);
   background_colour->push_back(config::display::background_colour.g);
   background_colour->push_back(config::display::background_colour.b);
   display->insert("background_colour", background_colour);
   config->insert("display", display);

   // input
   auto input = config->get_table("input");
   if (!input) {
      input = cpptoml::make_table();
   }

   input->insert("vpad0", config::input::vpad0);

   auto devices = config->get_table_array("devices");
   if (!devices) {
      devices = cpptoml::make_table_array();
   }

   for (const auto &device : config::input::devices) {
      auto cfgDevice = cpptoml::make_table();

      if (device.type == config::input::ControllerType::Joystick) {
         cfgDevice->insert("type", "joystick");
      } else if (device.type == config::input::ControllerType::Keyboard) {
         cfgDevice->insert("type", "keyboard");
      } else {
         cfgDevice->insert("type", "unknown");
      }

      cfgDevice->insert("id", device.id);
      cfgDevice->insert("device_name", device.device_name);
      cfgDevice->insert("button_up", device.button_up);
      cfgDevice->insert("button_down", device.button_down);
      cfgDevice->insert("button_left", device.button_left);
      cfgDevice->insert("button_right", device.button_right);
      cfgDevice->insert("button_a", device.button_a);
      cfgDevice->insert("button_b", device.button_b);
      cfgDevice->insert("button_x", device.button_x);
      cfgDevice->insert("button_y", device.button_y);
      cfgDevice->insert("button_trigger_r", device.button_trigger_r);
      cfgDevice->insert("button_trigger_l", device.button_trigger_l);
      cfgDevice->insert("button_trigger_zr", device.button_trigger_zr);
      cfgDevice->insert("button_trigger_zl", device.button_trigger_zl);
      cfgDevice->insert("button_stick_l", device.button_stick_l);
      cfgDevice->insert("button_stick_r", device.button_stick_r);
      cfgDevice->insert("button_plus", device.button_plus);
      cfgDevice->insert("button_minus", device.button_minus);
      cfgDevice->insert("button_home", device.button_home);
      cfgDevice->insert("button_sync", device.button_sync);

      if (device.type == config::input::ControllerType::Keyboard) {
         cfgDevice->insert("left_stick_up", device.keyboard.left_stick_up);
         cfgDevice->insert("left_stick_down", device.keyboard.left_stick_down);
         cfgDevice->insert("left_stick_left", device.keyboard.left_stick_left);
         cfgDevice->insert("left_stick_right", device.keyboard.left_stick_right);
         cfgDevice->insert("right_stick_up", device.keyboard.right_stick_up);
         cfgDevice->insert("right_stick_down", device.keyboard.right_stick_down);
         cfgDevice->insert("right_stick_left", device.keyboard.right_stick_left);
         cfgDevice->insert("right_stick_right", device.keyboard.right_stick_right);
      } else if (device.type == config::input::ControllerType::Joystick) {
         cfgDevice->insert("left_stick_x", device.joystick.left_stick_x);
         cfgDevice->insert("left_stick_x_invert", device.joystick.left_stick_x_invert);
         cfgDevice->insert("left_stick_y", device.joystick.left_stick_y);
         cfgDevice->insert("left_stick_y_invert", device.joystick.left_stick_y_invert);
         cfgDevice->insert("right_stick_x", device.joystick.right_stick_x);
         cfgDevice->insert("right_stick_x_invert", device.joystick.right_stick_x_invert);
         cfgDevice->insert("right_stick_y", device.joystick.right_stick_y);
         cfgDevice->insert("right_stick_y_invert", device.joystick.right_stick_y_invert);
      }

      devices->push_back(cfgDevice);
   }

   input->insert("devices", devices);
   config->insert("input", input);

   // sound
   auto sound = config->get_table("sound");
   if (!sound) {
      sound = cpptoml::make_table();
   }

   sound->insert("frame_length", config::sound::frame_length);
   config->insert("sound", sound);

   // test
   auto test = config->get_table("test");
   if (!test) {
      test = cpptoml::make_table();
   }

   test->insert("timeout_ms", config::test::timeout_ms);
   test->insert("timeout_frames", config::test::timeout_frames);
   test->insert("dump_drc_frames", config::test::dump_drc_frames);
   test->insert("dump_tv_frames", config::test::dump_tv_frames);
   test->insert("dump_frames_dir", config::test::dump_frames_dir);
   config->insert("test", test);
   return true;
}

} // namespace config
