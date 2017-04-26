#include "config_excmd.h"

#include <excmd.h>
#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <string>

namespace config
{

std::vector<excmd::option_group *>
getExcmdGroups(excmd::parser &parser)
{
   std::vector<excmd::option_group *> groups;
   using excmd::description;
   using excmd::optional;
   using excmd::default_value;
   using excmd::allowed;
   using excmd::value;

   auto gpu_options = parser.add_option_group("GPU Options")
      .add_option("gpu-debug",
                  description { "Enable extra gpu debug info." });
   groups.push_back(gpu_options.group);

   auto jit_options = parser.add_option_group("JIT Options")
      .add_option("jit",
                  description { "Enable the JIT engine." })
      .add_option("no-jit",
                  description { "Disable the JIT engine." })
      .add_option("jit-fast-math",
                  description { "Enable JIT floating-point optimizations which may not exactly match PowerPC behavior.  May not work for all games." },
                  default_value<std::string> { "full" },
                  allowed<std::string> { {
                     "full", "no-fpscr"
                  } })
      .add_option("jit-opt-level",
                  description { "Set the JIT optimization level.  Higher levels give better performance but may cause longer translation delays.  Level 3 may not work for all games." },
                  default_value<int> { 1 },
                  allowed<int> { { 0, 1, 2, 3 } })
      .add_option("jit-verify",
                  description { "Verify JIT implementation against interpreter." })
      .add_option("jit-verify-addr",
                  description { "Select single code block for JIT verification." },
                  default_value<uint32_t> { 0 });
   groups.push_back(jit_options.group);

   auto log_options = parser.add_option_group("Log Options")
      .add_option("log-async",
                  description { "Enable asynchronous logging." })
      .add_option("log-dir",
                  description{ "Directory where log file will be written." },
                  value<std::string> {})
      .add_option("log-file",
                  description { "Enable logging to file." })
      .add_option("log-stdout",
                  description { "Enable logging to stdout." })
      .add_option("log-level",
                  description { "Only display logs with severity equal to or greater than this level." },
                  default_value<std::string> { decaf::config::log::level },
                  allowed<std::string> { {
                     "trace", "debug", "info", "notice", "warning",
                     "error", "critical", "alert", "emerg", "off"
                  } });
   groups.push_back(log_options.group);

   auto sys_options = parser.add_option_group("System Options")
      .add_option("region",
                  description { "Set the system region." },
                  default_value<std::string> { "US" },
                  allowed<std::string> { {
                     "EUR", "JAP", "US"
                  } })
      .add_option("content-path",
                  description { "Sets which path to mount to /vol/content, only set for standalone rpx files." },
                  value<std::string> {})
      .add_option("resources-path",
                  description { "Path to Decaf resource files." },
                  value<std::string> {})
      .add_option("sdcard-path",
                  description { "Sets which path to mount to /vol/sdcard01." },
                  value<std::string> {})
      .add_option("mlc-path",
                  description { "Sets which path to mount to /vol/storage_mlc01." },
                  value<std::string> {})
      .add_option("time-scale",
                  description { "Time scale factor for emulated clock." },
                  default_value<double> { 1.0 });
   groups.push_back(sys_options.group);

   return groups;
}

bool
loadFromExcmd(excmd::option_state &options)
{
   if (options.has("jit")) {
      cpu::config::jit::enabled = true;
   } else if (options.has("no-jit")) {
      cpu::config::jit::enabled = false;
   }

   if (options.has("jit-verify")) {
      cpu::config::jit::verify = true;
   }

   if (options.has("jit-verify-addr")) {
      cpu::config::jit::verify_addr = options.get<uint32_t>("jit-verify-addr");
   }

   if (options.has("jit-opt-level")) {
      auto level = options.get<int>("jit-opt-level");

      cpu::config::jit::opt_flags.clear();

      if (level >= 1) {
         cpu::config::jit::opt_flags.push_back("BASIC");
         cpu::config::jit::opt_flags.push_back("DECONDITION");
         cpu::config::jit::opt_flags.push_back("DSE");
         cpu::config::jit::opt_flags.push_back("FOLD_CONSTANTS");
         cpu::config::jit::opt_flags.push_back("PPC_FORWARD_LOADS");
         cpu::config::jit::opt_flags.push_back("PPC_PAIRED_LWARX_STWCX");
         cpu::config::jit::opt_flags.push_back("X86_BRANCH_ALIGNMENT");
         cpu::config::jit::opt_flags.push_back("X86_CONDITION_CODES");
         cpu::config::jit::opt_flags.push_back("X86_FIXED_REGS");
         cpu::config::jit::opt_flags.push_back("X86_FORWARD_CONDITIONS");
         cpu::config::jit::opt_flags.push_back("X86_STORE_IMMEDIATE");
      }

      if (level >= 2) {
         cpu::config::jit::opt_flags.push_back("CHAIN");
         // Skip DEEP_DATA_FLOW if verifying because its whole purpose is
         //  to eliminate dead stores to registers.
         if (!cpu::config::jit::verify) {
            cpu::config::jit::opt_flags.push_back("DEEP_DATA_FLOW");
         }
         cpu::config::jit::opt_flags.push_back("PPC_TRIM_CR_STORES");
         cpu::config::jit::opt_flags.push_back("PPC_USE_SPLIT_FIELDS");
         cpu::config::jit::opt_flags.push_back("X86_ADDRESS_OPERANDS");
         cpu::config::jit::opt_flags.push_back("X86_MERGE_REGS");
      }

      if (level >= 3) {
         cpu::config::jit::opt_flags.push_back("PPC_CONSTANT_GQRS");
         cpu::config::jit::opt_flags.push_back("PPC_DETECT_FCFI_EMUL");
         cpu::config::jit::opt_flags.push_back("PPC_FAST_FCTIW");
      }
   }

   if (options.has("jit-fast-math")) {
      // PPC_NO_FPSCR_STATE by itself (--jit-fast-math=no-fpscr) is safe to
      //  use with --jit-verify as long as the game doesn't actually look
      //  at FPSCR status bits.  Other optimization flags may result in
      //  verification differences even if the generated code is correct.
      cpu::config::jit::opt_flags.push_back("PPC_NO_FPSCR_STATE");
      if (options.get<std::string>("jit-fast-math").compare("full") == 0) {
         cpu::config::jit::opt_flags.push_back("DSE_FP");
         cpu::config::jit::opt_flags.push_back("FOLD_FP_CONSTANTS");
         cpu::config::jit::opt_flags.push_back("NATIVE_IEEE_NAN");
         cpu::config::jit::opt_flags.push_back("NATIVE_IEEE_UNDERFLOW");
         cpu::config::jit::opt_flags.push_back("PPC_ASSUME_NO_SNAN");
         cpu::config::jit::opt_flags.push_back("PPC_FAST_FMADDS");
         cpu::config::jit::opt_flags.push_back("PPC_FAST_FMULS");
         cpu::config::jit::opt_flags.push_back("PPC_FAST_STFS");
         cpu::config::jit::opt_flags.push_back("PPC_FNMADD_ZERO_SIGN");
         cpu::config::jit::opt_flags.push_back("PPC_IGNORE_FPSCR_VXFOO");
         cpu::config::jit::opt_flags.push_back("PPC_NATIVE_RECIPROCAL");
         cpu::config::jit::opt_flags.push_back("PPC_PS_STORE_DENORMALS");
         cpu::config::jit::opt_flags.push_back("PPC_SINGLE_PREC_INPUTS");
      }
   }

   if (options.has("gpu-debug")) {
      gpu::config::debug = true;
   }

   if (options.has("log-stdout")) {
      decaf::config::log::to_stdout = true;
   }

   if (options.has("log-file")) {
      decaf::config::log::to_file = true;
   }

   if (options.has("log-dir")) {
      decaf::config::log::directory = options.get<std::string>("log-dir");
   }

   if (options.has("log-async")) {
      decaf::config::log::async = true;
   }

   if (options.has("log-level")) {
      decaf::config::log::level = options.get<std::string>("log-level");
   }

   if (options.has("region")) {
      auto region = options.get<std::string>("region");

      if (region.compare("JAP") == 0) {
         decaf::config::system::region = 1;
      } else if (region.compare("USA") == 0) {
         decaf::config::system::region = 2;
      } else if (region.compare("EUR") == 0) {
         decaf::config::system::region = 4;
      }
   }

   if (options.has("mlc-path")) {
      decaf::config::system::mlc_path = options.get<std::string>("mlc-path");
   }

   if (options.has("resources-path")) {
      decaf::config::system::resources_path = options.get<std::string>("resources-path");
   }

   if (options.has("sdcard-path")) {
      decaf::config::system::sdcard_path = options.get<std::string>("sdcard-path");
   }

   if (options.has("content-path")) {
      decaf::config::system::content_path = options.get<std::string>("content-path");
   }

   if (options.has("time-scale")) {
      decaf::config::system::time_scale = options.get<double>("time-scale");
   }

   return true;
}

} // namespace config
