#pragma once
#include <common/type_traits.h>
#include <cpptoml.h>
#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <string>
#include <memory>

namespace config
{

template<typename Type>
inline void
readValue(const std::shared_ptr<cpptoml::table> &config, const char *name, Type &value)
{
   value = static_cast<Type>(
      config->get_qualified_as<typename safe_underlying_type<Type>::type>(name)
      .value_or(static_cast<typename safe_underlying_type<Type>::type>(value)));
}

template<typename Type>
inline void
readArray(const std::shared_ptr<cpptoml::table> &config, const char *name, std::vector<Type> &value)
{
   value = config->get_qualified_array_of<Type>(name).value_or(value);
}

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             cpu::Settings &cpuSettings);

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             decaf::Settings &decafSettings);

bool
loadFromTOML(std::shared_ptr<cpptoml::table> config,
             gpu::Settings &gpuSettings);

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const cpu::Settings &cpuSettings);

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const decaf::Settings &decafSettings);

bool
saveToTOML(std::shared_ptr<cpptoml::table> config,
           const gpu::Settings &gpuSettings);

} // namespace config
