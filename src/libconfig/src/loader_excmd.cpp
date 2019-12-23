#include "config_excmd.h"

#include <common/strutils.h>
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

   auto display_options = parser.add_option_group("Display Options")
      .add_option("background-colour",
                  description { "Background colour." },
                  default_value<std::string> { "153,51,51" })
      .add_option("display-backend",
                  description { "Which display backend to use." },
                  default_value<std::string> { "vulkan" },
                  allowed<std::string> { {
                     "vulkan", "null",
                  } })
      .add_option("screen-mode",
                  description { "Screen display mode." },
                  default_value<std::string> { "windowed" },
                  allowed<std::string> { {
                     "windowed", "fullscreen"
                  } })
      .add_option("split-separation",
                  default_value<double> { 5.0 },
                  description { "Gap between screens when view-mode is split." })
      .add_option("stretch-screen",
                  description { "Stretch the screen to fill window rather than maintaining aspect ratio." })
      .add_option("view-mode",
                  description { "View display mode." },
                  default_value<std::string> { "split" },
                  allowed<std::string> { {
                     "split", "tv", "gamepad1", "gamepad2"
                  } });
   groups.push_back(display_options.group);

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
                  default_value<std::string> { "debug" },
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
      .add_option("resources-path",
                  description { "Path to Decaf resource files." },
                  value<std::string> {})
      .add_option("content-path",
                  description { "Sets which path to mount to /vol/content, only used for standalone rpx files." },
                  value<std::string> {})
      .add_option("hfio-path",
                  description { "Sets which path to mount to /dev/hfio01." },
                  value<std::string> {})
      .add_option("mlc-path",
                  description { "Sets which path to mount to /dev/mlc01." },
                  value<std::string> {})
      .add_option("otp-path",
                  description { "Path to otp.bin." },
                  value<std::string> {})
      .add_option("sdcard-path",
                  description { "Sets which path to mount to /dev/sdcard01." },
                  value<std::string> {})
      .add_option("slc-path",
                  description { "Sets which path to mount to /dev/slc01." },
                  value<std::string> {})
      .add_option("time-scale",
                  description { "Time scale factor for emulated clock." },
                  default_value<double> { 1.0 });
   groups.push_back(sys_options.group);

   return groups;
}

bool
loadFromExcmd(excmd::option_state &options,
              decaf::Settings &decafSettings)
{

   if (options.has("log-stdout")) {
      decafSettings.log.to_stdout = true;
   }

   if (options.has("log-file")) {
      decafSettings.log.to_file = true;
   }

   if (options.has("log-dir")) {
      decafSettings.log.directory = options.get<std::string>("log-dir");
   }

   if (options.has("log-async")) {
      decafSettings.log.async = true;
   }

   if (options.has("log-level")) {
      decafSettings.log.level = options.get<std::string>("log-level");
   }

   if (options.has("region")) {
      auto region = options.get<std::string>("region");

      if (iequals(region, "japan") == 0) {
         decafSettings.system.region = decaf::SystemRegion::Japan;
      } else if (iequals(region, "usa") == 0) {
         decafSettings.system.region = decaf::SystemRegion::USA;
      } else if (iequals(region, "europe") == 0) {
         decafSettings.system.region = decaf::SystemRegion::Europe;
      } else if (iequals(region, "china") == 0) {
         decafSettings.system.region = decaf::SystemRegion::China;
      } else if (iequals(region, "korea") == 0) {
         decafSettings.system.region = decaf::SystemRegion::Korea;
      } else if (iequals(region, "taiwan") == 0) {
         decafSettings.system.region = decaf::SystemRegion::Taiwan;
      }
   }

   if (options.has("resources-path")) {
      decafSettings.system.resources_path = options.get<std::string>("resources-path");
   }

   if (options.has("content-path")) {
      decafSettings.system.content_path = options.get<std::string>("content-path");
   }

   if (options.has("hfio-path")) {
      decafSettings.system.hfio_path = options.get<std::string>("hfio-path");
   }

   if (options.has("mlc-path")) {
      decafSettings.system.mlc_path = options.get<std::string>("mlc-path");
   }

   if (options.has("sdcard-path")) {
      decafSettings.system.sdcard_path = options.get<std::string>("sdcard-path");
   }

   if (options.has("slc-path")) {
      decafSettings.system.slc_path = options.get<std::string>("slc-path");
   }

   if (options.has("otp-path")) {
      decafSettings.system.otp_path = options.get<std::string>("otp-path");
   }

   if (options.has("time-scale")) {
      decafSettings.system.time_scale = options.get<double>("time-scale");
   }

   return true;
}

bool
loadFromExcmd(excmd::option_state &options,
              cpu::Settings &cpuSettings)
{
   if (options.has("jit")) {
      cpuSettings.jit.enabled = true;
   } else if (options.has("no-jit")) {
      cpuSettings.jit.enabled = false;
   }

   if (options.has("jit-verify")) {
      cpuSettings.jit.verify = true;
   }

   if (options.has("jit-verify-addr")) {
      cpuSettings.jit.verifyAddress = options.get<uint32_t>("jit-verify-addr");
   }

   if (options.has("jit-opt-level")) {
      auto level = options.get<int>("jit-opt-level");

      cpuSettings.jit.optimisationFlags.clear();

      if (level >= 1) {
         cpuSettings.jit.optimisationFlags.push_back("BASIC");
         cpuSettings.jit.optimisationFlags.push_back("DECONDITION");
         cpuSettings.jit.optimisationFlags.push_back("DSE");
         cpuSettings.jit.optimisationFlags.push_back("FOLD_CONSTANTS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FORWARD_LOADS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_PAIRED_LWARX_STWCX");
         cpuSettings.jit.optimisationFlags.push_back("X86_BRANCH_ALIGNMENT");
         cpuSettings.jit.optimisationFlags.push_back("X86_CONDITION_CODES");
         cpuSettings.jit.optimisationFlags.push_back("X86_FIXED_REGS");
         cpuSettings.jit.optimisationFlags.push_back("X86_FORWARD_CONDITIONS");
         cpuSettings.jit.optimisationFlags.push_back("X86_STORE_IMMEDIATE");
      }

      if (level >= 2) {
         cpuSettings.jit.optimisationFlags.push_back("CHAIN");
         // Skip DEEP_DATA_FLOW if verifying because its whole purpose is
         //  to eliminate dead stores to registers.
         if (!cpuSettings.jit.verify) {
            cpuSettings.jit.optimisationFlags.push_back("DEEP_DATA_FLOW");
         }
         cpuSettings.jit.optimisationFlags.push_back("PPC_TRIM_CR_STORES");
         cpuSettings.jit.optimisationFlags.push_back("PPC_USE_SPLIT_FIELDS");
         cpuSettings.jit.optimisationFlags.push_back("X86_ADDRESS_OPERANDS");
         cpuSettings.jit.optimisationFlags.push_back("X86_MERGE_REGS");
      }

      if (level >= 3) {
         cpuSettings.jit.optimisationFlags.push_back("PPC_CONSTANT_GQRS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_DETECT_FCFI_EMUL");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FAST_FCTIW");
      }
   }

   if (options.has("jit-fast-math")) {
      // PPC_NO_FPSCR_STATE by itself (--jit-fast-math=no-fpscr) is safe to
      //  use with --jit-verify as long as the game doesn't actually look
      //  at FPSCR status bits.  Other optimization flags may result in
      //  verification differences even if the generated code is correct.
      cpuSettings.jit.optimisationFlags.push_back("PPC_NO_FPSCR_STATE");
      if (options.get<std::string>("jit-fast-math").compare("full") == 0) {
         cpuSettings.jit.optimisationFlags.push_back("DSE_FP");
         cpuSettings.jit.optimisationFlags.push_back("FOLD_FP_CONSTANTS");
         cpuSettings.jit.optimisationFlags.push_back("NATIVE_IEEE_NAN");
         cpuSettings.jit.optimisationFlags.push_back("NATIVE_IEEE_UNDERFLOW");
         cpuSettings.jit.optimisationFlags.push_back("PPC_ASSUME_NO_SNAN");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FAST_FMADDS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FAST_FMULS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FAST_STFS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_FNMADD_ZERO_SIGN");
         cpuSettings.jit.optimisationFlags.push_back("PPC_IGNORE_FPSCR_VXFOO");
         cpuSettings.jit.optimisationFlags.push_back("PPC_NATIVE_RECIPROCAL");
         cpuSettings.jit.optimisationFlags.push_back("PPC_PS_STORE_DENORMALS");
         cpuSettings.jit.optimisationFlags.push_back("PPC_SINGLE_PREC_INPUTS");
      }
   }

   return true;
}

bool
loadFromExcmd(excmd::option_state &options,
              gpu::Settings &gpuSettings)
{
   if (options.has("gpu-debug")) {
      gpuSettings.debug.debug_enabled = true;
   }

   if (options.has("background-colour")) {
      auto colour = std::vector<std::string> { };
      split_string(options.get<std::string>("background-colour"), ',', colour);
      if (colour.size() == 3) {
         gpuSettings.display.backgroundColour[0] = std::atoi(colour[0].c_str());
         gpuSettings.display.backgroundColour[1] = std::atoi(colour[1].c_str());
         gpuSettings.display.backgroundColour[2] = std::atoi(colour[2].c_str());
      }
   }

   if (options.has("display-backend")) {
      auto mode = options.get<std::string>("screen-mode");
      if (mode.compare("vulkan") == 0) {
         gpuSettings.display.backend = gpu::DisplaySettings::Vulkan;
      } else if (mode.compare("null") == 0) {
         gpuSettings.display.backend = gpu::DisplaySettings::Null;
      }
   }

   if (options.has("screen-mode")) {
      auto mode = options.get<std::string>("screen-mode");
      if (mode.compare("windowed") == 0) {
         gpuSettings.display.screenMode = gpu::DisplaySettings::Windowed;
      } else if (mode.compare("fullscreen") == 0) {
         gpuSettings.display.screenMode = gpu::DisplaySettings::Fullscreen;
      }
   }

   if (options.has("stretch-screen")) {
      gpuSettings.display.maintainAspectRatio = false;
   }

   if (options.has("split-separation")) {
      gpuSettings.display.splitSeperation = options.get<double>("split-separation");
   }

   if (options.has("view-mode")) {
      auto layout = options.get<std::string>("view-mode");
      if (layout.compare("split") == 0) {
         gpuSettings.display.viewMode = gpu::DisplaySettings::Split;
      } else if (layout.compare("tv") == 0)  {
         gpuSettings.display.viewMode = gpu::DisplaySettings::TV;
      } else if (layout.compare("gamepad1") == 0) {
         gpuSettings.display.viewMode = gpu::DisplaySettings::Gamepad1;
      } else if (layout.compare("gamepad2") == 0) {
         gpuSettings.display.viewMode = gpu::DisplaySettings::Gamepad2;
      }
   }

   return true;
}

} // namespace config
