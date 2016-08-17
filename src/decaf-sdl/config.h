#pragma once
#include <string>

namespace config
{

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

} // namespace log

namespace input
{

namespace vpad0
{

extern std::string name;
extern int button_up;
extern int button_down;
extern int button_left;
extern int button_right;
extern int button_a;
extern int button_b;
extern int button_x;
extern int button_y;
extern int button_trigger_r;
extern int button_trigger_l;
extern int button_trigger_zr;
extern int button_trigger_zl;
extern int button_stick_l;
extern int button_stick_r;
extern int button_plus;
extern int button_minus;
extern int button_home;
extern int button_sync;
extern int left_stick_x;
extern int left_stick_y;
extern int right_stick_x;
extern int right_stick_y;

} // namespace vpad0

} // namespace input

bool
load(const std::string &path,
     std::string &error);

void
save(const std::string &path);

} // namespace config
