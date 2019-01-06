#pragma once
#include <cstdint>
#include <memory>
#include <vector>

namespace gpu
{

struct DebugSettings
{
   //! Enable debugging
   bool debug_enabled = false;

   //! Dump shaders
   bool dump_shaders = false;

   //! Only dump shader binaries
   bool dump_shader_binaries_only = false;
};

struct OpenGLSettings
{
   //! OpenGL debug message IDs to filter out
   std::vector<int64_t> debug_message_filters = { };
};

struct Settings
{
   DebugSettings debug;
   OpenGLSettings opengl;
};

std::shared_ptr<const Settings> config();
void setConfig(const Settings &settings);

} // namespace gpu
