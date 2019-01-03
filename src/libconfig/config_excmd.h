#pragma once
#include <excmd.h>
#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <vector>

namespace config
{

std::vector<excmd::option_group *>
getExcmdGroups(excmd::parser &parser);

bool
loadFromExcmd(excmd::option_state &options,
              cpu::Settings &cpuSettings);

bool
loadFromExcmd(excmd::option_state &options,
              decaf::Settings &decafSettings);

bool
loadFromExcmd(excmd::option_state &options,
              gpu::Settings &gpuSettings);

} // namespace config
