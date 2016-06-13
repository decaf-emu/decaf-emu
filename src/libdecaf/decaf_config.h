#pragma once
#include <string>

namespace decaf
{

namespace config
{

namespace gx2
{

extern bool dump_textures;
extern bool dump_shaders;

} // namespace gx2

namespace jit
{

extern bool enabled;
extern bool debug;

} // namespace jit

namespace log
{

extern bool kernel_trace;

} // namespace log

namespace system
{

extern std::string system_path;

} // namespace system

} // namespace config

} // namespace decaf
