#pragma once
#include <libconfig/config_toml.h>
#include <string>
#include <vector>

namespace config
{

struct Colour
{
   int r;
   int g;
   int b;
};

namespace display
{

enum DisplayMode
{
   Windowed,
   Fullscreen
};

enum DisplayLayout
{
   Split,
   Toggle
};

extern DisplayMode mode;
extern DisplayLayout layout;
extern bool stretch;
extern bool force_sync;
extern Colour background_colour;
extern std::string backend;

} // namespace display

namespace input
{

enum ControllerType
{
   None,
   Keyboard,
   Joystick,
};

struct InputDevice
{
   ControllerType type;
   std::string id;
   std::string device_name;

   // For keyboard input, each entry is an SDL_SCANCODE_* constant; for
   //  joystick input, each entry is the button number, or -2 to let SDL
   //  choose an appropriate button.  In both cases, -1 means nothing is
   //  assigned.

   int button_up;
   int button_down;
   int button_left;
   int button_right;
   int button_a;
   int button_b;
   int button_x;
   int button_y;
   int button_trigger_r;
   int button_trigger_l;
   int button_trigger_zr;
   int button_trigger_zl;
   int button_stick_l;
   int button_stick_r;
   int button_plus;
   int button_minus;
   int button_home;
   int button_sync;

   union
   {
      struct
      {
         int left_stick_up;
         int left_stick_down;
         int left_stick_left;
         int left_stick_right;
         int right_stick_up;
         int right_stick_down;
         int right_stick_left;
         int right_stick_right;
      } keyboard;

      struct
      {
         int left_stick_x;
         bool left_stick_x_invert;
         int left_stick_y;
         bool left_stick_y_invert;
         int right_stick_x;
         bool right_stick_x_invert;
         int right_stick_y;
         bool right_stick_y_invert;
      } joystick;
   };
};

extern std::vector<InputDevice> devices;
extern std::string vpad0;

} // namespace input

namespace sound
{

// Frame length factor for audio data.
// Default is 30 (x 48 = 1440 for 48kHz)
extern unsigned frame_length;

} // namespace sound

namespace test
{

//! Maximum time to run for before termination in milliseconds.
extern int timeout_ms;

//! Maximum number of frames to render before termination.
extern int timeout_frames;

//! Whether to dump rendered DRC frames to file;
extern bool dump_drc_frames;

//! Whether to dump rendered TV frames to file;
extern bool dump_tv_frames;

//! What directory to place dumped frames in.
extern std::string dump_frames_dir;

} // namespace test

void
setupDefaultInputDevices();

bool
loadFrontendToml(std::shared_ptr<cpptoml::table> config);

bool
saveFrontendToml(std::shared_ptr<cpptoml::table> config);

} // namespace config
