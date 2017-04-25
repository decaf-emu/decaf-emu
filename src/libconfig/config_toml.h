#pragma once
#include <cpptoml.h>
#include <functional>
#include <string>

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

using TomlUserParserFn = std::function<bool(std::string &, std::shared_ptr<cpptoml::table>)>;

bool
loadFromTOML(const std::string &path,
             std::string &error,
             TomlUserParserFn userParse = nullptr);

} // namespace config
