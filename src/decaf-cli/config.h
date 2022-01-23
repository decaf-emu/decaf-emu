#pragma once
#include <libconfig/config_toml.h>
#include <string>
#include <toml++/toml.h>

namespace config
{

namespace system
{

extern int timeout_ms;

} // namespace system

bool
loadFrontendToml(const toml::table &config);

bool
saveFrontendToml(toml::table &config);

} // namespace config
