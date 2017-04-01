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

} // namespace debugger

namespace gpu
{

//! Enable OpenGL debugging
extern bool debug;

//! OpenGL debug message IDs to filter out
// TODO: should really be a std::set, but cereal doesn't support those...
extern std::vector<unsigned> debug_filters;

} // namespace gpu

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

//! Use JIT in verification mode where it compares execution to interpreter
extern bool verify;

//! Select a single block (starting address) for verification (0 = verify everything)
extern uint32_t verify_addr;

//! JIT cache size in megabytes
extern unsigned int cache_size_mb;

//! List of JIT optimizations to enable
extern std::vector<std::string> opt_flags;

//! Treat .rodata sections as read-only regardless of RPL/RPX flags
extern bool rodata_read_only;

} // namespace jit

namespace log
{

//! Enable asynchronous logging
extern bool async;

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

//! Time scale factor for emulated clock
extern double time_scale;

} // namespace system

namespace ui
{

extern struct BackgroundColour {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} background_colour;

} // namespace ui

} // namespace config

} // namespace decaf
