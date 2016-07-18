#pragma once
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

} // namespace debugger

namespace gx2
{

//! Dump all textures to file
extern bool dump_textures;

//! Dump all shaders to file
extern bool dump_shaders;

} // namespace gx2

namespace jit
{

//! Enable usage of jit
extern bool enabled;

//! Use JIT in debug mode where it compares execution to interpreter
extern bool debug;

} // namespace jit

namespace log
{

//! Enable logging for all HLE function calls
extern bool kernel_trace;

//! Enable logging of every branch which targets a known symbol
extern bool branch_trace;

//! RegEx filters for kernel trace function name matching
extern std::vector<std::string> kernel_trace_filters;

} // namespace log

namespace system
{

//! Emulated system region
extern int region;

//! Path to system files
extern std::string system_path;

//! Time scale factor for emulated clock
extern double time_scale;

} // namespace system

} // namespace config

} // namespace decaf
