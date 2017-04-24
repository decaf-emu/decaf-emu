#include "config_toml.h"

#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <string>

namespace config
{

bool
loadFromTOML(const std::string &path,
             std::string &error,
             TomlUserParserFn userParse)
{
   try {
      auto config = cpptoml::parse_file(path);

      readValue(config, "debugger.enabled", decaf::config::debugger::enabled);
      readValue(config, "debugger.break_on_entry", decaf::config::debugger::break_on_entry);
      readValue(config, "debugger.gdb_stub", decaf::config::debugger::gdb_stub);
      readValue(config, "debugger.gdb_stub_port", decaf::config::debugger::gdb_stub_port);

      readValue(config, "gx2.dump_textures", decaf::config::gx2::dump_textures);
      readValue(config, "gx2.dump_shaders", decaf::config::gx2::dump_shaders);

      readValue(config, "log.async", decaf::config::log::async);
      readValue(config, "log.kernel_trace", decaf::config::log::kernel_trace);
      readValue(config, "log.kernel_trace_res", decaf::config::log::kernel_trace_res);
      readValue(config, "log.branch_trace", decaf::config::log::branch_trace);
      readArray(config, "log.kernel_trace_filters", decaf::config::log::kernel_trace_filters);

      readValue(config, "sound.dump_sounds", decaf::config::sound::dump_sounds);

      readValue(config, "system.region", decaf::config::system::region);
      readValue(config, "system.mlc_path", decaf::config::system::mlc_path);
      readValue(config, "system.sdcard_path", decaf::config::system::sdcard_path);
      readValue(config, "system.content_path", decaf::config::system::content_path);
      readValue(config, "system.time_scale", decaf::config::system::time_scale);
      readArray(config, "system.lle_modules", decaf::config::system::lle_modules);

      if (userParse) {
         return userParse(error, config);
      }
   } catch (cpptoml::parse_exception ex) {
      error = std::string { "Parse Exception: " } + ex.what();
      return false;
   }

   return true;
}

} // namespace config
