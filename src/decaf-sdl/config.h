#pragma once
#include <libconfig/config_toml.h>
#include <string>
#include <variant>
#include <vector>

namespace config
{

namespace input
{

enum ControllerType
{
   None,
   Keyboard,
   Joystick,
};

/*
 * For keyboard input, each entry is an SDL_SCANCODE_ *constant; for
 * joystick input, each entry is the button number, or -2 to let SDL
 * choose an appropriate button.  In both cases, -1 means nothing is
 * assigned.
 */

struct InputDeviceKeyboard
{
   int left_stick_up = -1;
   int left_stick_down = -1;
   int left_stick_left = -1;
   int left_stick_right = -1;

   int right_stick_up = -1;
   int right_stick_down = -1;
   int right_stick_left = -1;
   int right_stick_right = -1;
};

struct InputDeviceJoystick
{
   int left_stick_x = -1;
   bool left_stick_x_invert = false;

   int left_stick_y = -1;
   bool left_stick_y_invert = false;

   int right_stick_x = -1;
   bool right_stick_x_invert = false;

   int right_stick_y = -1;
   bool right_stick_y_invert = false;
};

struct InputDevice
{
   ControllerType type;
   std::string id;
   std::string device_name;

   int button_up = -1;
   int button_down = -1;
   int button_left = -1;
   int button_right = -1;
   int button_a = -1;
   int button_b = -1;
   int button_x = -1;
   int button_y = -1;
   int button_trigger_r = -1;
   int button_trigger_l = -1;
   int button_trigger_zr = -1;
   int button_trigger_zl = -1;
   int button_stick_l = -1;
   int button_stick_r = -1;
   int button_plus = -1;
   int button_minus = -1;
   int button_home = -1;
   int button_sync = -1;

   std::variant<InputDeviceKeyboard, InputDeviceJoystick> typeExtra;
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
loadFrontendToml(const toml::table &config);

bool
saveFrontendToml(toml::table &config);

} // namespace config
