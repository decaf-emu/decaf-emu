#pragma once
#include <vector>

namespace gpu
{

namespace config
{

//! Enable OpenGL debugging
extern bool debug;

//! OpenGL debug message IDs to filter out
// TODO: should really be a std::set, but cereal doesn't support those...
extern std::vector<unsigned> debug_filters;

//! Dump shaders
extern bool dump_shaders;

} // namespace config

} // namespace gpu
