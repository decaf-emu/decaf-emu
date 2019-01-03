#pragma once
#include "cpu_config.h"

#include <common/configstorage.h>

namespace cpu
{

void
registerConfigChangeListener(ConfigStorage<Settings>::ChangeListener listener);

} // namespace cpu
