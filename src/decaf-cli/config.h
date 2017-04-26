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
loadFrontendToml(std::shared_ptr<cpptoml::table> config);

bool
saveFrontendToml(std::shared_ptr<cpptoml::table> config);

} // namespace config
