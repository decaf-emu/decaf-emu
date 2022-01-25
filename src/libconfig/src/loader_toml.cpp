#include "config_toml.h"

#include <string>
#include <optional>

namespace config
{

template<typename Type>
inline void
readArray(const toml::table &config, const char *name, std::vector<Type> &value)
{
   auto ptr = config.at_path(name).as_array();
   if (ptr) {
      value.clear();
      for (auto itr = ptr->begin(); itr != ptr->end(); ++itr) {
         value.push_back(itr->template as<Type>()->get());
      }
   }
}

static const char *
translateDisplayBackend(gpu::DisplaySettings::Backend backend)
{
   if (backend == gpu::DisplaySettings::Null) {
      return "null";
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
loadFromTOML(const toml::table &config,
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
loadFromTOML(const toml::table &config,
             decaf::Settings &decafSettings)
{
   readValue(config, "debugger.enabled", decafSettings.debugger.enabled);
   readValue(config, "debugger.break_on_entry", decafSettings.debugger.break_on_entry);
   readValue(config, "debugger.break_on_exit", decafSettings.debugger.break_on_exit);
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

   auto logLevels = config.at_path("log.levels").as_table();
   if (logLevels) {
      decafSettings.log.levels.clear();
      for (auto item : *logLevels) {
         if (auto level = item.second.as_string()) {
            decafSettings.log.levels.emplace_back(item.first, **level);
         }
      }
   }

   readValue(config, "sound.dump_sounds", decafSettings.sound.dump_sounds);

   readValue(config, "system.region", decafSettings.system.region);
   readValue(config, "system.hfio_path", decafSettings.system.hfio_path);
   readValue(config, "system.mlc_path", decafSettings.system.mlc_path);
   readValue(config, "system.otp_path", decafSettings.system.otp_path);
   readValue(config, "system.resources_path", decafSettings.system.resources_path);
   readValue(config, "system.sdcard_path", decafSettings.system.sdcard_path);
   readValue(config, "system.slc_path", decafSettings.system.slc_path);
   readValue(config, "system.content_path", decafSettings.system.content_path);
   readValue(config, "system.time_scale", decafSettings.system.time_scale);
   readArray(config, "system.lle_modules", decafSettings.system.lle_modules);
   readValue(config, "system.dump_hle_rpl", decafSettings.system.dump_hle_rpl);
   readArray(config, "system.title_directories", decafSettings.system.title_directories);
   return true;
}

bool
loadFromTOML(const toml::table &config,
             gpu::Settings &gpuSettings)
{
   readValue(config, "gpu.debug", gpuSettings.debug.debug_enabled);
   readValue(config, "gpu.dump_shaders", gpuSettings.debug.dump_shaders);
   readValue(config, "gpu.dump_shader_binaries_only", gpuSettings.debug.dump_shader_binaries_only);

   auto display = config.get_as<toml::table>("display");
   if (display) {
      if (auto text = display->get_as<std::string>("backend"); text) {
         if (auto backend = translateDisplayBackend(**text); backend) {
            gpuSettings.display.backend = *backend;
         }
      }

      if (auto text = display->get_as<std::string>("screen_mode"); text) {
         if (auto screenMode = translateScreenMode(**text); screenMode) {
            gpuSettings.display.screenMode = *screenMode;
         }
      }

      if (auto text = display->get_as<std::string>("view_mode"); text) {
         if (auto viewMode = translateViewMode(**text); viewMode) {
            gpuSettings.display.viewMode = *viewMode;
         }
      }

      if (auto maintainAspectRatio = display->get_as<bool>("maintain_aspect_ratio"); maintainAspectRatio) {
         gpuSettings.display.maintainAspectRatio = **maintainAspectRatio;
      }

      if (auto splitSeperation = display->get_as<double>("split_seperation"); splitSeperation) {
         gpuSettings.display.splitSeperation = **splitSeperation;
      }

      if (auto backgroundColour = display->get_as<toml::array>("background_colour");
          backgroundColour && backgroundColour->size() >= 3) {
         gpuSettings.display.backgroundColour = {
               static_cast<int>(**backgroundColour->at(0).as_integer()),
               static_cast<int>(**backgroundColour->at(1).as_integer()),
               static_cast<int>(**backgroundColour->at(2).as_integer())
            };
      }
   }

   return true;
}

bool
saveToTOML(toml::table &config,
           const cpu::Settings &cpuSettings)
{
   auto jit = config.insert("jit", toml::table()).first->second.as_table();
   jit->insert_or_assign("enabled", cpuSettings.jit.enabled);
   jit->insert_or_assign("verify", cpuSettings.jit.verify);
   jit->insert_or_assign("verify_addr", cpuSettings.jit.verifyAddress);
   jit->insert_or_assign("code_cache_size_mb", cpuSettings.jit.codeCacheSizeMB);
   jit->insert_or_assign("data_cache_size_mb", cpuSettings.jit.dataCacheSizeMB);
   jit->insert_or_assign("rodata_read_only", cpuSettings.jit.rodataReadOnly);

   auto opt_flags = toml::array();
   for (auto &flag : cpuSettings.jit.optimisationFlags) {
      opt_flags.push_back(flag);
   }

   jit->insert_or_assign("opt_flags", opt_flags);
   return true;
}

bool
saveToTOML(toml::table &config,
           const decaf::Settings &decafSettings)
{
   // debugger
   auto debugger = config.insert("debugger", toml::table()).first->second.as_table();
   debugger->insert_or_assign("enabled", decafSettings.debugger.enabled);
   debugger->insert_or_assign("break_on_entry", decafSettings.debugger.break_on_entry);
   debugger->insert_or_assign("break_on_exit", decafSettings.debugger.break_on_exit);
   debugger->insert_or_assign("gdb_stub", decafSettings.debugger.gdb_stub);
   debugger->insert_or_assign("gdb_stub_port", decafSettings.debugger.gdb_stub_port);

   // gx2
   auto gx2 = config.insert("gx2", toml::table()).first->second.as_table();
   gx2->insert_or_assign("dump_textures", decafSettings.gx2.dump_textures);
   gx2->insert_or_assign("dump_shaders", decafSettings.gx2.dump_shaders);

   // log
   auto log = config.insert("log", toml::table()).first->second.as_table();
   log->insert_or_assign("async", decafSettings.log.async);
   log->insert_or_assign("branch_trace", decafSettings.log.branch_trace);
   log->insert_or_assign("directory", decafSettings.log.directory);
   log->insert_or_assign("hle_trace", decafSettings.log.hle_trace);
   log->insert_or_assign("hle_trace_res", decafSettings.log.hle_trace_res);
   log->insert_or_assign("level", decafSettings.log.level);
   log->insert_or_assign("to_file", decafSettings.log.to_file);
   log->insert_or_assign("to_stdout", decafSettings.log.to_stdout);

   auto levels = toml::table();
   for (auto item : decafSettings.log.levels) {
      levels.insert_or_assign(item.first, item.second);
   }
   log->insert_or_assign("levels", std::move(levels));

   auto hle_trace_filters = toml::array();
   for (auto &filter : decafSettings.log.hle_trace_filters) {
      hle_trace_filters.push_back(filter);
   }

   log->insert_or_assign("hle_trace_filters", std::move(hle_trace_filters));

   // sound
   auto sound = config.insert("sound", toml::table()).first->second.as_table();
   sound->insert_or_assign("dump_sounds", decafSettings.sound.dump_sounds);

   // system
   auto system = config.insert("system", toml::table()).first->second.as_table();
   system->insert_or_assign("region", static_cast<int>(decafSettings.system.region));
   system->insert_or_assign("hfio_path", decafSettings.system.hfio_path);
   system->insert_or_assign("mlc_path", decafSettings.system.mlc_path);
   system->insert_or_assign("otp_path", decafSettings.system.otp_path);
   system->insert_or_assign("resources_path", decafSettings.system.resources_path);
   system->insert_or_assign("sdcard_path", decafSettings.system.sdcard_path);
   system->insert_or_assign("slc_path", decafSettings.system.slc_path);
   system->insert_or_assign("content_path", decafSettings.system.content_path);
   system->insert_or_assign("time_scale", decafSettings.system.time_scale);

   auto lle_modules = toml::array();
   for (auto &name : decafSettings.system.lle_modules) {
      lle_modules.push_back(name);
   }
   system->insert_or_assign("lle_modules", std::move(lle_modules));

   auto title_directories = toml::array();
   for (auto &name : decafSettings.system.title_directories) {
      title_directories.push_back(name);
   }
   system->insert_or_assign("title_directories", std::move(title_directories));
   return true;
}

bool
saveToTOML(toml::table &config,
           const gpu::Settings &gpuSettings)
{
   // gpu
   auto gpu = config.insert("gpu", toml::table()).first->second.as_table();
   gpu->insert_or_assign("debug", gpuSettings.debug.debug_enabled);
   gpu->insert_or_assign("dump_shaders", gpuSettings.debug.dump_shaders);
   gpu->insert_or_assign("dump_shader_binaries_only", gpuSettings.debug.dump_shader_binaries_only);

   // display
   auto display = config.insert("display", toml::table()).first->second.as_table();
   display->insert_or_assign("backend", translateDisplayBackend(gpuSettings.display.backend));
   display->insert_or_assign("screen_mode", translateScreenMode(gpuSettings.display.screenMode));
   display->insert_or_assign("view_mode", translateViewMode(gpuSettings.display.viewMode));
   display->insert_or_assign("maintain_aspect_ratio", gpuSettings.display.maintainAspectRatio);
   display->insert_or_assign("split_seperation", gpuSettings.display.splitSeperation);

   auto backgroundColour = toml::array();
   backgroundColour.push_back(gpuSettings.display.backgroundColour[0]);
   backgroundColour.push_back(gpuSettings.display.backgroundColour[1]);
   backgroundColour.push_back(gpuSettings.display.backgroundColour[2]);
   display->insert_or_assign("background_colour", std::move(backgroundColour));
   return true;
}

} // namespace config
