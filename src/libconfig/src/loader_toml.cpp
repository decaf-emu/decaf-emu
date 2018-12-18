#include "config_toml.h"

#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <string>

namespace config
{

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config)
{
   readValue(config, "debugger.enabled", decaf::config::debugger::enabled);
   readValue(config, "debugger.break_on_entry", decaf::config::debugger::break_on_entry);
   readValue(config, "debugger.gdb_stub", decaf::config::debugger::gdb_stub);
   readValue(config, "debugger.gdb_stub_port", decaf::config::debugger::gdb_stub_port);

   readValue(config, "gpu.debug", gpu::config::debug);
   readArray(config, "gpu.debug_filters", gpu::config::debug_filters);
   readValue(config, "gpu.dump_shaders", gpu::config::dump_shaders);

   readValue(config, "gx2.dump_textures", decaf::config::gx2::dump_textures);
   readValue(config, "gx2.dump_shaders", decaf::config::gx2::dump_shaders);

   readValue(config, "mem.writetrack", cpu::config::mem::writetrack);

   readValue(config, "jit.enabled", cpu::config::jit::enabled);
   readValue(config, "jit.verify", cpu::config::jit::verify);
   readValue(config, "jit.verify_addr", cpu::config::jit::verify_addr);
   readValue(config, "jit.code_cache_size_mb", cpu::config::jit::code_cache_size_mb);
   readValue(config, "jit.data_cache_size_mb", cpu::config::jit::data_cache_size_mb);
   readArray(config, "jit.opt_flags", cpu::config::jit::opt_flags);
   readValue(config, "jit.rodata_read_only", cpu::config::jit::rodata_read_only);

   readValue(config, "log.async", decaf::config::log::async);
   readValue(config, "log.branch_trace", decaf::config::log::branch_trace);
   readValue(config, "log.directory", decaf::config::log::directory);
   readValue(config, "log.kernel_trace", decaf::config::log::kernel_trace);
   readValue(config, "log.kernel_trace_res", decaf::config::log::kernel_trace_res);
   readArray(config, "log.kernel_trace_filters", decaf::config::log::kernel_trace_filters);
   readValue(config, "log.level", decaf::config::log::level);
   readValue(config, "log.to_file", decaf::config::log::to_file);
   readValue(config, "log.to_stdout", decaf::config::log::to_stdout);

   readValue(config, "sound.dump_sounds", decaf::config::sound::dump_sounds);

   readValue(config, "system.region", decaf::config::system::region);
   readValue(config, "system.hfio_path", decaf::config::system::hfio_path);
   readValue(config, "system.mlc_path", decaf::config::system::mlc_path);
   readValue(config, "system.resources_path", decaf::config::system::resources_path);
   readValue(config, "system.sdcard_path", decaf::config::system::sdcard_path);
   readValue(config, "system.slc_path", decaf::config::system::slc_path);
   readValue(config, "system.content_path", decaf::config::system::content_path);
   readValue(config, "system.time_scale", decaf::config::system::time_scale);
   readArray(config, "system.lle_modules", decaf::config::system::lle_modules);
   readValue(config, "system.dump_hle_rpl", decaf::config::system::dump_hle_rpl);
   return true;
}

bool
saveToTOML(std::shared_ptr<cpptoml::table> config)
{
   // debugger
   auto debugger = config->get_table("debugger");
   if (!debugger) {
      debugger = cpptoml::make_table();
   }

   debugger->insert("enabled", decaf::config::debugger::enabled);
   debugger->insert("break_on_entry", decaf::config::debugger::break_on_entry);
   debugger->insert("gdb_stub", decaf::config::debugger::gdb_stub);
   debugger->insert("gdb_stub_port", decaf::config::debugger::gdb_stub_port);
   config->insert("debugger", debugger);

   // gpu
   auto gpu = config->get_table("gpu");
   if (!gpu) {
      gpu = cpptoml::make_table();
   }

   gpu->insert("debug", gpu::config::debug);
   gpu->insert("dump_shaders", gpu::config::dump_shaders);

   auto debug_filters = cpptoml::make_array();
   for (auto &filter : gpu::config::debug_filters) {
      debug_filters->push_back(filter);
   }

   gpu->insert("debug_filters", debug_filters);
   config->insert("gpu", gpu);

   // gx2
   auto gx2 = config->get_table("gx2");
   if (!gx2) {
      gx2 = cpptoml::make_table();
   }

   gx2->insert("dump_textures", decaf::config::gx2::dump_textures);
   gx2->insert("dump_shaders", decaf::config::gx2::dump_shaders);
   config->insert("gx2", gx2);

   // jit
   auto jit = config->get_table("jit");
   if (!jit) {
      jit = cpptoml::make_table();
   }

   jit->insert("enabled", cpu::config::jit::enabled);
   jit->insert("verify", cpu::config::jit::verify);
   jit->insert("verify_addr", cpu::config::jit::verify_addr);
   jit->insert("code_cache_size_mb", cpu::config::jit::code_cache_size_mb);
   jit->insert("data_cache_size_mb", cpu::config::jit::data_cache_size_mb);
   jit->insert("rodata_read_only", cpu::config::jit::rodata_read_only);

   auto opt_flags = cpptoml::make_array();
   for (auto &flag : cpu::config::jit::opt_flags) {
      opt_flags->push_back(flag);
   }

   jit->insert("opt_flags", opt_flags);
   config->insert("jit", jit);

   // log
   auto log = config->get_table("log");
   if (!log) {
      log = cpptoml::make_table();
   }

   log->insert("async", decaf::config::log::async);
   log->insert("branch_trace", decaf::config::log::branch_trace);
   log->insert("directory", decaf::config::log::directory);
   log->insert("kernel_trace", decaf::config::log::kernel_trace);
   log->insert("kernel_trace_res", decaf::config::log::kernel_trace_res);
   log->insert("level", decaf::config::log::level);
   log->insert("to_file", decaf::config::log::to_file);
   log->insert("to_stdout", decaf::config::log::to_stdout);

   auto kernel_trace_filters = cpptoml::make_array();
   for (auto &filter : decaf::config::log::kernel_trace_filters) {
      kernel_trace_filters->push_back(filter);
   }

   log->insert("kernel_trace_filters", kernel_trace_filters);
   config->insert("log", log);

   // sound
   auto sound = config->get_table("sound");
   if (!sound) {
      sound = cpptoml::make_table();
   }

   sound->insert("dump_sounds", decaf::config::sound::dump_sounds);
   config->insert("sound", sound);

   // system
   auto system = config->get_table("system");
   if (!system) {
      system = cpptoml::make_table();
   }

   system->insert("region", static_cast<int>(decaf::config::system::region));
   system->insert("hfio_path", decaf::config::system::hfio_path);
   system->insert("mlc_path", decaf::config::system::mlc_path);
   system->insert("resources_path", decaf::config::system::resources_path);
   system->insert("sdcard_path", decaf::config::system::sdcard_path);
   system->insert("slc_path", decaf::config::system::slc_path);
   system->insert("content_path", decaf::config::system::content_path);
   system->insert("time_scale", decaf::config::system::time_scale);

   auto lle_modules = cpptoml::make_array();
   for (auto &name : decaf::config::system::lle_modules) {
      lle_modules->push_back(name);
   }

   system->insert("lle_modules", lle_modules);
   config->insert("system", system);
   return true;
}

} // namespace config
