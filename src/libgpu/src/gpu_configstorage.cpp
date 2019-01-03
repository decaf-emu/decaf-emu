#include "gpu_config.h"
#include "gpu_configstorage.h"

#include <common/configstorage.h>

namespace gpu
{

static ConfigStorage<Settings> sSettings;

std::shared_ptr<const Settings>
config()
{
   return sSettings.get();
}

void
setConfig(const Settings &settings)
{
   sSettings.set(std::make_shared<Settings>(settings));
}

void
registerConfigChangeListener(ConfigStorage<Settings>::ChangeListener listener)
{
   sSettings.addListener(listener);
}

} // namespace gpu
