#pragma once
#include <cstdint>
#include <vector>

namespace gpu
{

namespace config
{

//! Enable OpenGL debugging
extern bool debug;

//! OpenGL debug message IDs to filter out
extern std::vector<int64_t> debug_filters;

//! Dump shaders
extern bool dump_shaders;

} // namespace config

} // namespace gpu
