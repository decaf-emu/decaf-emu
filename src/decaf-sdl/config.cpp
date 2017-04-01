#include "clilog.h"
#include <common/decaf_assert.h>
#include "config.h"
#include "libdecaf/decaf_config.h"
#include <climits>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <common/cerealjsonoptionalinput.h>
#include <fstream>
#include <SDL_keycode.h>

namespace config
{

namespace display
{

DisplayMode mode = DisplayMode::Windowed;
DisplayLayout layout = DisplayLayout::Split;
bool stretch = false;

} // namespace display

namespace gpu
{

bool force_sync = false;

} // namespace gpu

namespace input
{

std::vector<InputDevice> devices;

namespace vpad0
{

ControllerType type = Keyboard;
std::string name = "";

} // namespace vpad0

std::string
controllerTypeToString(ControllerType type)
{
   switch (type) {
   case None:
      return "none";
   case Keyboard:
      return "keyboard";
   case Joystick:
      return "joystick";
   }

   decaf_abort(fmt::format("Invalid controller type {}", type));
}

ControllerType
controllerTypeFromString(const std::string &typeStr)
{
   if (typeStr.compare("keyboard") == 0) {
      return Keyboard;
   } else if (typeStr.compare("joystick") == 0) {
      return Joystick;
   } else {
      if (typeStr.compare("none") != 0) {
         gCliLog->error("Invalid input type: {}", typeStr);
      }

      return None;
   }
}

} // namespace input

namespace sound
{

unsigned frame_length = 30;

} // namespace sound

namespace log
{

bool async = false;
bool to_file = true;
bool to_stdout = false;
std::string level = "debug";
std::string directory = ".";

} // namespace log

struct CerealDebugger
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::debugger;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(break_on_entry));
   }
};

struct CerealGPU
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace gpu;
      using namespace decaf::config::gpu;
      ar(CEREAL_NVP(debug),
         CEREAL_NVP(debug_filters),
         CEREAL_NVP(force_sync));
   }
};

struct CerealGX2
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::gx2;
      ar(CEREAL_NVP(dump_textures),
         CEREAL_NVP(dump_shaders));
   }
};

struct CerealInputVpad0
{
   template <class Archive>
   void save(Archive &ar) const
   {
      using namespace config::input::vpad0;
      ar(cereal::make_nvp("type", input::controllerTypeToString(type)),
         CEREAL_NVP(name));
   }

   template <class Archive>
   void load(Archive &ar)
   {
      using namespace config::input::vpad0;

      std::string typeStr;
      ar(typeStr,
         CEREAL_NVP(name));

      type = input::controllerTypeFromString(typeStr);
   }
};

struct CerealInput
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace config::input;
      ar(CEREAL_NVP(devices),
         cereal::make_nvp("vpad0", CerealInputVpad0 {}));
   }
};

struct CerealLog
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace config::log;
      using namespace decaf::config::log;
      ar(CEREAL_NVP(async),
         CEREAL_NVP(to_file),
         CEREAL_NVP(to_stdout),
         CEREAL_NVP(kernel_trace),
         CEREAL_NVP(kernel_trace_res),
         CEREAL_NVP(kernel_trace_filters),
         CEREAL_NVP(branch_trace),
         CEREAL_NVP(level));
   }
};

struct CerealJit
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::jit;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(verify),
         CEREAL_NVP(cache_size_mb),
         CEREAL_NVP(opt_flags),
         CEREAL_NVP(rodata_read_only));
   }
};

struct CerealSound
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace ::decaf::config::sound;
      using namespace sound;
      ar(CEREAL_NVP(dump_sounds),
         CEREAL_NVP(frame_length));
   }
};

struct CerealSystem
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::system;
      ar(CEREAL_NVP(region),
         CEREAL_NVP(mlc_path),
         CEREAL_NVP(sdcard_path));
   }
};

struct CerealUI
{
    template <class Archive>
    void serialize(Archive &ar)
    {
        using namespace decaf::config::ui;
        ar(CEREAL_NVP(background_colour));
    }
};

void
initialize()
{
   input::devices.clear();

   // Setup default keyboard device
   {
      input::InputDevice device;

      device.type = input::Keyboard;
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
   {
      input::InputDevice device;

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
load(const std::string &path,
     std::string &error)
{
   std::ifstream file { path, std::ios::binary };

   if (!file.is_open()) {
      // Create a default config file
      save(path);
      return false;
   }

   try {
      cereal::JSONOptionalInputArchive input { file };
      input(cereal::make_nvp("debugger", CerealDebugger {}),
            cereal::make_nvp("gpu", CerealGPU{}),
            cereal::make_nvp("gx2", CerealGX2 {}),
            cereal::make_nvp("input", CerealInput {}),
            cereal::make_nvp("jit", CerealJit {}),
            cereal::make_nvp("log", CerealLog {}),
            cereal::make_nvp("sound", CerealSound {}),
            cereal::make_nvp("system", CerealSystem {}),
            cereal::make_nvp("ui", CerealUI {}));
   } catch (std::exception e) {
      error = e.what();
      return false;
   }

   return true;
}

void
save(const std::string &path)
{
   std::ofstream file { path, std::ios::binary };
   cereal::JSONOutputArchive output { file };
   output(cereal::make_nvp("debugger", CerealDebugger {}),
          cereal::make_nvp("gpu", CerealGPU{}),
          cereal::make_nvp("gx2", CerealGX2 {}),
          cereal::make_nvp("input", CerealInput {}),
          cereal::make_nvp("jit", CerealJit {}),
          cereal::make_nvp("log", CerealLog {}),
          cereal::make_nvp("sound", CerealSound {}),
          cereal::make_nvp("system", CerealSystem {}),
          cereal::make_nvp("ui", CerealUI {}));
}

} // namespace config

// External serialization functions
namespace cereal
{
    // background_color_s serialization
    template<class Archive>
    void serialize(Archive &ar, decaf::config::ui::BackgroundColour &c)
    {
        ar(make_nvp("red", c.r),
           make_nvp("green", c.g),
           make_nvp("blue", c.b));
    }
}