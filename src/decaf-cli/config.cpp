#include "config.h"

namespace config
{

namespace system
{

int timeout_ms = 0;

} // namespace system

bool
loadFrontendToml(std::shared_ptr<cpptoml::table> config)
{
   system::timeout_ms = config->get_qualified_as<int>("system.timeout_ms").value_or(system::timeout_ms);
   return true;
}

bool
saveFrontendToml(std::shared_ptr<cpptoml::table> config)
{
   auto system = config->get_table("system");
   if (!system) {
      system = cpptoml::make_table();
   }

   system->insert("timeout_ms", system::timeout_ms);
   config->insert("system", system);
   return true;
}

} // namespace config
