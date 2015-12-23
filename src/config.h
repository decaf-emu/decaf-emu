#pragma once
#include <string>

namespace config
{

namespace dx12
{

extern bool use_warp;

} // namespace dx12

namespace gx2
{

extern bool dump_textures;
extern bool dump_shaders;

} // namespace gx2

namespace log
{

extern bool async;
extern bool to_file;
extern bool to_stdout;
extern bool kernel_trace;
extern std::string filename;
extern std::string level;

} // namespace log

namespace jit
{

extern bool enabled;
extern bool debug;

} // namespace jit

namespace system
{

extern std::string system_path;

} // namespace system

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

} // namespace vpad0

} // namespace input

namespace ui
{

extern int tv_window_x;
extern int tv_window_y;
extern int drc_window_x;
extern int drc_window_y;

} // namespace ui

bool load(const std::string &path);
void save(const std::string &path);

} // namespace config
