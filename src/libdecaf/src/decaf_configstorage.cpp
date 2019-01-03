#include "decaf_config.h"
#include "decaf_configstorage.h"

#include <common/configstorage.h>

namespace decaf
{

static ConfigStorage<Settings> sSettings;

void
setConfig(const Settings &settings)
{
   sSettings.set(std::make_shared<Settings>(settings));
}

std::shared_ptr<const Settings>
config()
{
   return sSettings.get();
}

void
registerConfigChangeListener(ConfigStorage<Settings>::ChangeListener listener)
{
   sSettings.addListener(listener);
}

} // namespace decaf
