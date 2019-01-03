#pragma once
#include "gpu_config.h"
#include <common/configstorage.h>

namespace gpu
{

void
registerConfigChangeListener(ConfigStorage<Settings>::ChangeListener listener);

} // namespace gpu
