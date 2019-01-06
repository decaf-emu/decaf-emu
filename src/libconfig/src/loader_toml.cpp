#include "config_toml.h"

#include <string>

namespace config
{

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
   return true;
}

} // namespace config
