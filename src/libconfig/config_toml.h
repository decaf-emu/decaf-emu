#pragma once
#include <cpptoml.h>
#include <string>
#include <memory>

namespace config
{

template<typename Type>
inline void
readValue(const std::shared_ptr<cpptoml::table> &config, const char *name, Type &value)
{
   value = config->get_qualified_as<Type>(name).value_or(value);
}

template<typename Type>
inline void
readArray(const std::shared_ptr<cpptoml::table> &config, const char *name, std::vector<Type> &value)
{
   value = config->get_qualified_array_of<Type>(name).value_or(value);
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config);

bool
saveToTOML(std::shared_ptr<cpptoml::table> config);

} // namespace config
