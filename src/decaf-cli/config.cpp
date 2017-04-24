#include "config.h"

namespace config
{

namespace system
{

int timeout_ms = 0;

} // namespace system

bool
loadFrontendToml(std::string &error,
                 std::shared_ptr<cpptoml::table> config)
{
   system::timeout_ms = config->get_qualified_as<int>("system.timeout_ms").value_or(system::timeout_ms);
   return true;
}

} // namespace config
