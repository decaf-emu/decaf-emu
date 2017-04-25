#pragma once
#include <set>
#include <string>
#include <vector>

namespace decaf
{

namespace config
{

namespace debugger
{

//! Enable usage of debugger
extern bool enabled;

//! Whether to break on entry point of game when debugger is enabled
extern bool break_on_entry;

//! Whether to run a gdb stub or not
extern bool gdb_stub;

//! What port to use for gdb stub
extern unsigned gdb_stub_port;

} // namespace debugger

namespace gx2
{

//! Dump all textures to file
extern bool dump_textures;

//! Dump all shaders to file
extern bool dump_shaders;

} // namespace gx2

namespace log
{

//! Enable asynchronous logging
extern bool async;

//! Enable logging to file
extern bool to_file;

//! Enable logging to stdout
extern bool to_stdout;

//! Minimum log level to show
extern std::string level;

//! Directory to output log file to
extern std::string directory;

//! Enable logging for all HLE function calls
extern bool kernel_trace;

//! Enable logging for all HLE function call results
extern bool kernel_trace_res;

//! Enable logging of every branch which targets a known symbol
extern bool branch_trace;

//! Wildcard filters for kernel trace function name matching
extern std::vector<std::string> kernel_trace_filters;

} // namespace log

namespace sound
{

//! Dump all sounds to file
extern bool dump_sounds;

} // namespace sound

namespace system
{

//! Emulated system region
extern int region;

//! Path to /vol/storage_mlc01 files
extern std::string mlc_path;

//! Path to /vol/sdcard01 files
extern std::string sdcard_path;

//! Path to /vol/content for standalone .rpx applications
extern std::string content_path;

//! Path to directory containing Decaf resources
extern std::string resources_path;

//! Time scale factor for emulated clock
extern double time_scale;

//! List of system modules to load LLE instead of HLE.
extern std::vector<std::string> lle_modules;

} // namespace system

} // namespace config

} // namespace decaf
