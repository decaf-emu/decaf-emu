#include "config_toml.h"

#include <string>
#include <optional>

namespace config
{

static const char *
translateDisplayBackend(gpu::DisplaySettings::Backend backend)
{
   if (backend == gpu::DisplaySettings::Null) {
      return "null";
   } else if (backend == gpu::DisplaySettings::OpenGL) {
      return "opengl";
   } else if (backend == gpu::DisplaySettings::Vulkan) {
      return "vulkan";
   }

   return "";
}

static std::optional<gpu::DisplaySettings::Backend>
translateDisplayBackend(const std::string &text)
{
   if (text == "null") {
      return gpu::DisplaySettings::Null;
   } else if (text == "opengl") {
      return gpu::DisplaySettings::OpenGL;
   } else if (text == "vulkan") {
      return gpu::DisplaySettings::Vulkan;
   }

   return { };
}

static const char *
translateScreenMode(gpu::DisplaySettings::ScreenMode mode)
{
   if (mode == gpu::DisplaySettings::Windowed) {
      return "windowed";
   } else if (mode == gpu::DisplaySettings::Fullscreen) {
      return "fullscreen";
   }

   return "";
}

static std::optional<gpu::DisplaySettings::ScreenMode>
translateScreenMode(const std::string &text)
{
   if (text == "windowed") {
      return gpu::DisplaySettings::Windowed;
   } else if (text == "fullscreen") {
      return gpu::DisplaySettings::Fullscreen;
   }

   return { };
}

static const char *
translateViewMode(gpu::DisplaySettings::ViewMode mode)
{
   if (mode == gpu::DisplaySettings::TV) {
      return "tv";
   } else if (mode == gpu::DisplaySettings::Gamepad1) {
      return "gamepad1";
   } else if (mode == gpu::DisplaySettings::Gamepad2) {
      return "gamepad2";
   } else if (mode == gpu::DisplaySettings::Split) {
      return "split";
   }

   return "";
}

static std::optional<gpu::DisplaySettings::ViewMode>
translateViewMode(const std::string &text)
{
   if (text == "tv") {
      return gpu::DisplaySettings::TV;
   } else if (text == "gamepad1") {
      return gpu::DisplaySettings::Gamepad1;
   } else if (text == "gamepad2") {
      return gpu::DisplaySettings::Gamepad2;
   } else if (text == "split") {
      return gpu::DisplaySettings::Split;
   }

   return { };
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             cpu::Settings &cpuSettings)
{
   readValue(config, "mem.writetrack", cpuSettings.memory.writeTrackEnabled);

   readValue(config, "jit.enabled", cpuSettings.jit.enabled);
   readValue(config, "jit.verify", cpuSettings.jit.verify);
   readValue(config, "jit.verify_addr", cpuSettings.jit.verifyAddress);
   readValue(config, "jit.code_cache_size_mb", cpuSettings.jit.codeCacheSizeMB);
   readValue(config, "jit.data_cache_size_mb", cpuSettings.jit.dataCacheSizeMB);
   readArray(config, "jit.opt_flags", cpuSettings.jit.optimisationFlags);
   readValue(config, "jit.rodata_read_only", cpuSettings.jit.rodataReadOnly);
   return true;
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             decaf::Settings &decafSettings)
{
   readValue(config, "debugger.enabled", decafSettings.debugger.enabled);
   readValue(config, "debugger.break_on_entry", decafSettings.debugger.break_on_entry);
   readValue(config, "debugger.gdb_stub", decafSettings.debugger.gdb_stub);
   readValue(config, "debugger.gdb_stub_port", decafSettings.debugger.gdb_stub_port);

   readValue(config, "gx2.dump_textures", decafSettings.gx2.dump_textures);
   readValue(config, "gx2.dump_shaders", decafSettings.gx2.dump_shaders);

   readValue(config, "log.async", decafSettings.log.async);
   readValue(config, "log.branch_trace", decafSettings.log.branch_trace);
   readValue(config, "log.directory", decafSettings.log.directory);
   readValue(config, "log.hle_trace", decafSettings.log.hle_trace);
   readValue(config, "log.hle_trace_res", decafSettings.log.hle_trace_res);
   readArray(config, "log.hle_trace_filters", decafSettings.log.hle_trace_filters);
   readValue(config, "log.level", decafSettings.log.level);
   readValue(config, "log.to_file", decafSettings.log.to_file);
   readValue(config, "log.to_stdout", decafSettings.log.to_stdout);

   readValue(config, "sound.dump_sounds", decafSettings.sound.dump_sounds);

   readValue(config, "system.region", decafSettings.system.region);
   readValue(config, "system.hfio_path", decafSettings.system.hfio_path);
   readValue(config, "system.mlc_path", decafSettings.system.mlc_path);
   readValue(config, "system.resources_path", decafSettings.system.resources_path);
   readValue(config, "system.sdcard_path", decafSettings.system.sdcard_path);
   readValue(config, "system.slc_path", decafSettings.system.slc_path);
   readValue(config, "system.content_path", decafSettings.system.content_path);
   readValue(config, "system.time_scale", decafSettings.system.time_scale);
   readArray(config, "system.lle_modules", decafSettings.system.lle_modules);
   readValue(config, "system.dump_hle_rpl", decafSettings.system.dump_hle_rpl);
   return true;
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             gpu::Settings &gpuSettings)
{
   readValue(config, "gpu.debug", gpuSettings.debug.debug_enabled);
   readArray(config, "gpu.opengl_debug_filters", gpuSettings.opengl.debug_message_filters);
   readValue(config, "gpu.dump_shaders", gpuSettings.debug.dump_shaders);
   readValue(config, "gpu.dump_shader_binaries_only", gpuSettings.debug.dump_shader_binaries_only);

   auto display = config->get_table("display");
   if (display) {
      if (auto text = display->get_as<std::string>("backend"); text) {
         if (auto backend = translateDisplayBackend(*text); backend) {
            gpuSettings.display.backend = *backend;
         }
      }

      if (auto text = display->get_as<std::string>("screen_mode"); text) {
         if (auto screenMode = translateScreenMode(*text); screenMode) {
            gpuSettings.display.screenMode = *screenMode;
         }
      }

      if (auto text = display->get_as<std::string>("view_mode"); text) {
         if (auto viewMode = translateViewMode(*text); viewMode) {
            gpuSettings.display.viewMode = *viewMode;
         }
      }

      if (auto maintainAspectRatio = display->get_as<bool>("maintain_aspect_ratio"); maintainAspectRatio) {
         gpuSettings.display.maintainAspectRatio = *maintainAspectRatio;
      }

      if (auto splitSeperation = display->get_as<double>("split_seperation"); splitSeperation) {
         gpuSettings.display.splitSeperation = *splitSeperation;
      }

      if (auto backgroundColour = display->get_array_of<int64_t>("background_colour");
          backgroundColour && backgroundColour->size() >= 3) {
         gpuSettings.display.backgroundColour = {
               static_cast<int>(backgroundColour->at(0)),
               static_cast<int>(backgroundColour->at(1)),
               static_cast<int>(backgroundColour->at(2))
            };
      }
   }

   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const cpu::Settings &cpuSettings)
{
   // jit
   auto jit = config->get_table("jit");
   if (!jit) {
      jit = cpptoml::make_table();
   }

   jit->insert("enabled", cpuSettings.jit.enabled);
   jit->insert("verify", cpuSettings.jit.verify);
   jit->insert("verify_addr", cpuSettings.jit.verifyAddress);
   jit->insert("code_cache_size_mb", cpuSettings.jit.codeCacheSizeMB);
   jit->insert("data_cache_size_mb", cpuSettings.jit.dataCacheSizeMB);
   jit->insert("rodata_read_only", cpuSettings.jit.rodataReadOnly);

   auto opt_flags = cpptoml::make_array();
   for (auto &flag : cpuSettings.jit.optimisationFlags) {
      opt_flags->push_back(flag);
   }

   jit->insert("opt_flags", opt_flags);
   config->insert("jit", jit);
   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const decaf::Settings &decafSettings)
{
   // debugger
   auto debugger = config->get_table("debugger");
   if (!debugger) {
      debugger = cpptoml::make_table();
   }

   debugger->insert("enabled", decafSettings.debugger.enabled);
   debugger->insert("break_on_entry", decafSettings.debugger.break_on_entry);
   debugger->insert("gdb_stub", decafSettings.debugger.gdb_stub);
   debugger->insert("gdb_stub_port", decafSettings.debugger.gdb_stub_port);
   config->insert("debugger", debugger);

   // gx2
   auto gx2 = config->get_table("gx2");
   if (!gx2) {
      gx2 = cpptoml::make_table();
   }

   gx2->insert("dump_textures", decafSettings.gx2.dump_textures);
   gx2->insert("dump_shaders", decafSettings.gx2.dump_shaders);
   config->insert("gx2", gx2);

   // log
   auto log = config->get_table("log");
   if (!log) {
      log = cpptoml::make_table();
   }

   log->insert("async", decafSettings.log.async);
   log->insert("branch_trace", decafSettings.log.branch_trace);
   log->insert("directory", decafSettings.log.directory);
   log->insert("hle_trace", decafSettings.log.hle_trace);
   log->insert("hle_trace_res", decafSettings.log.hle_trace_res);
   log->insert("level", decafSettings.log.level);
   log->insert("to_file", decafSettings.log.to_file);
   log->insert("to_stdout", decafSettings.log.to_stdout);

   auto hle_trace_filters = cpptoml::make_array();
   for (auto &filter : decafSettings.log.hle_trace_filters) {
      hle_trace_filters->push_back(filter);
   }

   log->insert("hle_trace_filters", hle_trace_filters);
   config->insert("log", log);

   // sound
   auto sound = config->get_table("sound");
   if (!sound) {
      sound = cpptoml::make_table();
   }

   sound->insert("dump_sounds", decafSettings.sound.dump_sounds);
   config->insert("sound", sound);

   // system
   auto system = config->get_table("system");
   if (!system) {
      system = cpptoml::make_table();
   }

   system->insert("region", static_cast<int>(decafSettings.system.region));
   system->insert("hfio_path", decafSettings.system.hfio_path);
   system->insert("mlc_path", decafSettings.system.mlc_path);
   system->insert("resources_path", decafSettings.system.resources_path);
   system->insert("sdcard_path", decafSettings.system.sdcard_path);
   system->insert("slc_path", decafSettings.system.slc_path);
   system->insert("content_path", decafSettings.system.content_path);
   system->insert("time_scale", decafSettings.system.time_scale);

   auto lle_modules = cpptoml::make_array();
   for (auto &name : decafSettings.system.lle_modules) {
      lle_modules->push_back(name);
   }

   system->insert("lle_modules", lle_modules);
   config->insert("system", system);
   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const gpu::Settings &gpuSettings)
{
   // gpu
   auto gpu = config->get_table("gpu");
   if (!gpu) {
      gpu = cpptoml::make_table();
   }

   gpu->insert("debug", gpuSettings.debug.debug_enabled);
   gpu->insert("dump_shaders", gpuSettings.debug.dump_shaders);
   gpu->insert("dump_shader_binaries_only", gpuSettings.debug.dump_shader_binaries_only);

   auto debug_filters = cpptoml::make_array();
   for (auto &filter : gpuSettings.opengl.debug_message_filters) {
      debug_filters->push_back(filter);
   }

   gpu->insert("opengl_debug_filters", debug_filters);
   config->insert("gpu", gpu);

   // display
   auto display = config->get_table("display");
   if (!display) {
      display = cpptoml::make_table();
   }

   display->insert("backend", translateDisplayBackend(gpuSettings.display.backend));
   display->insert("screen_mode", translateScreenMode(gpuSettings.display.screenMode));
   display->insert("view_mode", translateViewMode(gpuSettings.display.viewMode));
   display->insert("maintain_aspect_ratio", gpuSettings.display.maintainAspectRatio);
   display->insert("split_seperation", gpuSettings.display.splitSeperation);

   auto backgroundColour = cpptoml::make_array();
   backgroundColour->push_back(gpuSettings.display.backgroundColour[0]);
   backgroundColour->push_back(gpuSettings.display.backgroundColour[1]);
   backgroundColour->push_back(gpuSettings.display.backgroundColour[2]);
   display->insert("background_colour", backgroundColour);

   config->insert("display", display);
   return true;
}

} // namespace config
