#pragma once
#include <string>
#include <vector>
#include <common/cerealjsonoptionalinput.h> // for inline InputDevice load/save

namespace config
{

namespace display
{

enum DisplayMode
{
   Windowed,
   Fullscreen,
   Popup
};

enum DisplayLayout
{
   Split,
   Toggle
};

extern DisplayMode mode;
extern DisplayLayout layout;
extern bool stretch;

} // namespace display

namespace gpu
{

extern bool force_sync;

} // namespace gpu

namespace log
{

extern bool async;
extern bool to_file;
extern bool to_stdout;
extern std::string level;
extern std::string directory;

} // namespace log

namespace input
{

enum ControllerType
{
   None,
   Keyboard,
   Joystick,
};

// for inline InputDevice load/save
extern std::string
controllerTypeToString(ControllerType type);

extern ControllerType
controllerTypeFromString(const std::string &typeStr);

struct InputDevice
{
   ControllerType type;
   std::string name;

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

   // For some reason this has to be inline or cereal doesn't find it...

   #define KEYBOARD_NVP(field)  cereal::make_nvp(#field, keyboard.field)
   #define JOYSTICK_NVP(field)  cereal::make_nvp(#field, joystick.field)

   template<class Archive>
   void save(Archive &ar) const
   {
      ar(cereal::make_nvp("type", controllerTypeToString(type)),
         CEREAL_NVP(name),
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

      switch (type) {
      case input::None:  // suppress warnings
         break;

      case input::Keyboard:
         ar(KEYBOARD_NVP(left_stick_up),
            KEYBOARD_NVP(left_stick_down),
            KEYBOARD_NVP(left_stick_left),
            KEYBOARD_NVP(left_stick_right),
            KEYBOARD_NVP(right_stick_up),
            KEYBOARD_NVP(right_stick_down),
            KEYBOARD_NVP(right_stick_left),
            KEYBOARD_NVP(right_stick_right));
         break;

      case input::Joystick:
         ar(JOYSTICK_NVP(left_stick_x),
            JOYSTICK_NVP(left_stick_x_invert),
            JOYSTICK_NVP(left_stick_y),
            JOYSTICK_NVP(left_stick_y_invert),
            JOYSTICK_NVP(right_stick_x),
            JOYSTICK_NVP(right_stick_x_invert),
            JOYSTICK_NVP(right_stick_y),
            JOYSTICK_NVP(right_stick_y_invert));
         break;
      }
   }

   template <class Archive>
   void load(Archive &ar)
   {
      std::string typeStr;
      ar(typeStr,
         CEREAL_NVP(name),
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

      type = controllerTypeFromString(typeStr);

      switch (type) {
      case input::None:  // suppress warnings
         break;

      case input::Keyboard:
         ar(KEYBOARD_NVP(left_stick_up),
            KEYBOARD_NVP(left_stick_down),
            KEYBOARD_NVP(left_stick_left),
            KEYBOARD_NVP(left_stick_right),
            KEYBOARD_NVP(right_stick_up),
            KEYBOARD_NVP(right_stick_down),
            KEYBOARD_NVP(right_stick_left),
            KEYBOARD_NVP(right_stick_right));
         break;

      case input::Joystick:
         ar(JOYSTICK_NVP(left_stick_x),
            JOYSTICK_NVP(left_stick_x_invert),
            JOYSTICK_NVP(left_stick_y),
            JOYSTICK_NVP(left_stick_y_invert),
            JOYSTICK_NVP(right_stick_x),
            JOYSTICK_NVP(right_stick_x_invert),
            JOYSTICK_NVP(right_stick_y),
            JOYSTICK_NVP(right_stick_y_invert));
         break;
      }
   }

   #undef KEYBOARD_NVP
   #undef JOYSTICK_NVP
};

extern std::vector<InputDevice> devices;

namespace vpad0
{

extern ControllerType type;
extern std::string name;

} // namespace vpad0

} // namespace input

namespace sound {

// Frame length factor for audio data.
// Default is 30 (x 48 = 1440 for 48kHz)
extern unsigned frame_length;

} // namespace sound

void
initialize();

bool
load(const std::string &path,
     std::string &error);

void
save(const std::string &path);

} // namespace config
