#pragma once
#include "decaf_config.h"

#include <common/configstorage.h>

namespace decaf
{

void registerConfigChangeListener(ConfigStorage<Settings>::ChangeListener listener);

} // namespace decaf
