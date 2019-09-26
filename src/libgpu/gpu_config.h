#pragma once
#include <array>
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

struct DisplaySettings
{
   enum Backend
   {
      Null,
      OpenGL,
      Vulkan,
   };

   enum ScreenMode
   {
      Windowed,
      Fullscreen,
   };

   enum ViewMode
   {
      Split,
      TV,
      Gamepad1,
      Gamepad2,
   };

   Backend backend = Backend::Vulkan;
   std::array<int, 3> backgroundColour = { 153, 51, 51 };
   bool maintainAspectRatio = true;
   ScreenMode screenMode = ScreenMode::Windowed;
   double splitSeperation = 5.0;
   ViewMode viewMode = ViewMode::Split;
};

struct OpenGLSettings
{
   //! OpenGL debug message IDs to filter out
   std::vector<int64_t> debug_message_filters = { };
};

struct Settings
{
   DebugSettings debug;
   DisplaySettings display;
   OpenGLSettings opengl;
};

std::shared_ptr<const Settings> config();
void setConfig(const Settings &settings);

} // namespace gpu
