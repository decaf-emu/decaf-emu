#pragma once
#include <libconfig/config_toml.h>
#include <string>

namespace config
{

namespace system
{

extern int timeout_ms;

} // namespace system

bool
loadFrontendToml(std::string &error,
                 std::shared_ptr<cpptoml::table> config);

} // namespace config
