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

//! Show the segment view on startup
extern bool show_seg_view;

//! Show the thread view on startup
extern bool show_thread_view;

//! Show the memory view on startup
extern bool show_mem_view;

//! Show the disassembly view on startup
extern bool show_disasm_view;

//! Show the register view on startup
extern bool show_reg_view;

//! Show the stack view on startup
extern bool show_stack_view;

//! Show the stats view on startup
extern bool show_stats_view;

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
