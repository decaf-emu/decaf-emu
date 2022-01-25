#include "config.h"

namespace config
{

namespace system
{

int timeout_ms = 0;

} // namespace system

bool
loadFrontendToml(const toml::table &config)
{
   readValue(config, "system.timeout_ms", system::timeout_ms);
   return true;
}

bool
saveFrontendToml(toml::table &config)
{
   auto system = config.insert("system", toml::table()).first->second.as_table();
   system->insert_or_assign("timeout_ms", system::timeout_ms);
   return true;
}

} // namespace config
