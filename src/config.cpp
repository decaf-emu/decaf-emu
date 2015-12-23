#include "config.h"
#include "utils/log.h"
#include <cereal/archives/json.hpp>
#include <fstream>

#ifndef _MSC_VER
   #include <SDL_keycode.h>
#endif

namespace config
{

namespace dx12
{

bool use_warp = true;

} // namespace dx12

namespace gx2
{

bool dump_textures = false;
bool dump_shaders = false;

} // namespace gx2

namespace log
{

bool async = true;
bool to_file = false;
bool to_stdout = true;
bool kernel_trace = true;
std::string filename = "log";
std::string level = "info";

} // namespace log

namespace jit
{

bool enabled = false;
bool debug = false;

} // namespace jit

namespace system
{

std::string system_path = "/undefined_system_path";

} // namespace system

namespace input
{

namespace vpad0
{

std::string name = "keyboard";
#ifdef _MSC_VER
int button_up = 265;
int button_down = 264;
int button_left = 263;
int button_right = 262;
int button_a = 'X';
int button_b = 'Z';
int button_x = 'S';
int button_y = 'A';
int button_trigger_r = 'E';
int button_trigger_l = 'W';
int button_trigger_zr = 'R';
int button_trigger_zl = 'Q';
int button_stick_l = 'D';
int button_stick_r = 'C';
int button_plus = '1';
int button_minus = '2';
int button_home = '3';
int button_sync = '4';
#else
int button_up = SDLK_UP;
int button_down = SDLK_DOWN;
int button_left = SDLK_LEFT;
int button_right = SDLK_RIGHT;
int button_a = SDLK_x;
int button_b = SDLK_z;
int button_x = SDLK_s;
int button_y = SDLK_a;
int button_trigger_r = SDLK_e;
int button_trigger_l = SDLK_w;
int button_trigger_zr = SDLK_r;
int button_trigger_zl = SDLK_q;
int button_stick_l = SDLK_d;
int button_stick_r = SDLK_c;
int button_plus = SDLK_1;
int button_minus = SDLK_2;
int button_home = SDLK_3;
int button_sync = SDLK_4;
#endif

} // namespace vpad0

} // namespace input

namespace ui
{

int tv_window_x = INT_MIN;
int tv_window_y = INT_MIN;
int drc_window_x = INT_MIN;
int drc_window_y = INT_MIN;

} // namespace ui

struct CerealDX12
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace dx12;
      ar(CEREAL_NVP(use_warp));
   }
};

struct CerealGX2
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace gx2;
      ar(CEREAL_NVP(dump_textures),
         CEREAL_NVP(dump_shaders));
   }
};

struct CerealLog
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace log;
      ar(CEREAL_NVP(async),
         CEREAL_NVP(to_file),
         CEREAL_NVP(to_stdout),
         CEREAL_NVP(kernel_trace),
         CEREAL_NVP(filename),
         CEREAL_NVP(level));
   }
};

struct CerealJit
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace jit;
      ar(CEREAL_NVP(enabled),
         CEREAL_NVP(debug));
   }
};

struct CerealSystem
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace system;
      ar(CEREAL_NVP(system_path));
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
         CEREAL_NVP(button_sync));
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

struct CerealUi
{
   template <class Archive>
   void serialize(Archive &ar)
   {
      using namespace ui;
      ar(CEREAL_NVP(tv_window_x));
      ar(CEREAL_NVP(tv_window_y));
      ar(CEREAL_NVP(drc_window_x));
      ar(CEREAL_NVP(drc_window_y));
   }
};

bool load(const std::string &path)
{
   std::ifstream file(path, std::ios::binary);

   if (!file.is_open()) {
      // Create a default config file
      save(path);
      return false;
   }

   cereal::JSONInputArchive input(file);

   try {
      input(cereal::make_nvp("dx12", CerealDX12 {}),
            cereal::make_nvp("gx2", CerealGX2 {}),
            cereal::make_nvp("log", CerealLog {}),
            cereal::make_nvp("jit", CerealJit {}),
            cereal::make_nvp("system", CerealSystem {}),
            cereal::make_nvp("input", CerealInput {}),
            cereal::make_nvp("ui", CerealUi {}));
   } catch (std::exception e) {
      // TODO: Can't do this because gLog is NULL here.
      //gLog->error("Failed to parse config.json: {}", e.what());
      //gLog->error("Try deleting your config.json to allow a new correct one to be generated (with default settings).");
   }

   return true;
}

void save(const std::string &path)
{
   std::ofstream file(path, std::ios::binary);
   cereal::JSONOutputArchive output(file);
   output(cereal::make_nvp("dx12", CerealDX12 {}),
          cereal::make_nvp("gx2", CerealGX2 {}),
          cereal::make_nvp("log", CerealLog {}),
          cereal::make_nvp("jit", CerealJit {}),
          cereal::make_nvp("system", CerealSystem {}),
          cereal::make_nvp("input", CerealInput {}),
          cereal::make_nvp("ui", CerealUi {}));
}

} // namespace config
