#include "config.h"
#include "libdecaf/decaf_config.h"
#include <climits>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>
#include <SDL_keycode.h>

namespace config
{

namespace display
{

DisplayMode mode = DisplayMode::Windowed;
DisplayLayout layout = DisplayLayout::Split;

} // namespace display

namespace gpu
{

bool force_sync = false;

} // namespace gpu

namespace log
{

bool async = false;
bool to_file = true;
bool to_stdout = false;
std::string level = "debug";

} // namespace log

namespace input
{

namespace vpad0
{

std::string name = "keyboard";
int button_up = SDL_SCANCODE_UP;
int button_down = SDL_SCANCODE_DOWN;
int button_left = SDL_SCANCODE_LEFT;
int button_right = SDL_SCANCODE_RIGHT;
int button_a = SDL_SCANCODE_X;
int button_b = SDL_SCANCODE_Z;
int button_x = SDL_SCANCODE_S;
int button_y = SDL_SCANCODE_A;
int button_trigger_r = SDL_SCANCODE_E;
int button_trigger_l = SDL_SCANCODE_W;
int button_trigger_zr = SDL_SCANCODE_R;
int button_trigger_zl = SDL_SCANCODE_Q;
int button_stick_l = SDL_SCANCODE_D;
int button_stick_r = SDL_SCANCODE_C;
int button_plus = SDL_SCANCODE_1;
int button_minus = SDL_SCANCODE_2;
int button_home = SDL_SCANCODE_3;
int button_sync = SDL_SCANCODE_4;
int left_stick_x = -1;
int left_stick_y = -1;
int right_stick_x = -1;
int right_stick_y = -1;

} // namespace vpad0

} // namespace input

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
         CEREAL_NVP(verify));
   }
};

struct CerealSound
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::sound;
      ar(CEREAL_NVP(dump_sounds));
   }
};

struct CerealSystem
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace decaf::config::system;
      ar(CEREAL_NVP(region),
         CEREAL_NVP(system_path));
   }
};

struct CerealVpad0
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace input::vpad0;
      ar(CEREAL_NVP(name),
         CEREAL_NVP(button_up),
         CEREAL_NVP(button_down),
         CEREAL_NVP(button_left),
         CEREAL_NVP(button_right),
         CEREAL_NVP(button_a),
         CEREAL_NVP(button_b),
         CEREAL_NVP(button_x),
         CEREAL_NVP(button_y),
         CEREAL_NVP(button_trigger_r),
         CEREAL_NVP(button_trigger_l),
         CEREAL_NVP(button_trigger_zr),
         CEREAL_NVP(button_trigger_zl),
         CEREAL_NVP(button_stick_l),
         CEREAL_NVP(button_stick_r),
         CEREAL_NVP(button_plus),
         CEREAL_NVP(button_minus),
         CEREAL_NVP(button_home),
         CEREAL_NVP(button_sync),
         CEREAL_NVP(left_stick_x),
         CEREAL_NVP(left_stick_y),
         CEREAL_NVP(right_stick_x),
         CEREAL_NVP(right_stick_y));
   }
};

struct CerealInput
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      ar(cereal::make_nvp("vpad0", CerealVpad0 {}));
   }
};

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
      cereal::JSONInputArchive input { file };
      input(cereal::make_nvp("debugger", CerealDebugger {}),
            cereal::make_nvp("gpu", CerealGPU{}),
            cereal::make_nvp("gx2", CerealGX2 {}),
            cereal::make_nvp("input", CerealInput {}),
            cereal::make_nvp("jit", CerealJit {}),
            cereal::make_nvp("log", CerealLog {}),
            cereal::make_nvp("sound", CerealSound {}),
            cereal::make_nvp("system", CerealSystem {}));
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
          cereal::make_nvp("system", CerealSystem {}));
}

} // namespace config
