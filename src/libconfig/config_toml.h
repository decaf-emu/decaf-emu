#pragma once
#include <common/type_traits.h>
#include <libcpu/cpu_config.h>
#include <libdecaf/decaf_config.h>
#include <libgpu/gpu_config.h>
#include <string>
#include <toml++/toml.h>
#include <memory>
#include <type_traits>

namespace config
{

bool
loadFromTOML(const toml::table &config,
             cpu::Settings &cpuSettings);

bool
loadFromTOML(const toml::table &config,
             decaf::Settings &decafSettings);

bool
loadFromTOML(const toml::table &config,
             gpu::Settings &gpuSettings);

bool
saveToTOML(toml::table &config,
           const cpu::Settings &cpuSettings);

bool
saveToTOML(toml::table &config,
           const decaf::Settings &decafSettings);

bool
saveToTOML(toml::table &config,
           const gpu::Settings &gpuSettings);


template<typename Type>
void
readValue(const toml::table &config,
          const char *name,
          Type &value)
{
   auto node = config.at_path(name);
   if constexpr (std::is_same_v<Type, bool>) {
      if (auto ptr = node.as_boolean()) {
         value = ptr->get();
      }
   } else if constexpr (std::is_integral_v<typename safe_underlying_type<Type>::type>) {
      if (auto ptr = node.as_integer()) {
         value = static_cast<Type>(ptr->get());
      }
   } else if constexpr (std::is_floating_point_v<typename safe_underlying_type<Type>::type>) {
      if (auto ptr = node.as_floating_point()) {
         value = static_cast<Type>(ptr->get());
      }
   } else if constexpr (std::is_same_v<Type, std::string>) {
      if (auto ptr = node.as_string()) {
         value = static_cast<Type>(ptr->get());
      }
   } else {
      static_assert(
         std::is_same_v<Type, bool> ||
         std::is_integral_v<typename safe_underlying_type<Type>::type> ||
         std::is_floating_point_v<typename safe_underlying_type<Type>::type> ||
         std::is_same_v<Type, std::string>,
         "Invalid type passed to readValue");
   }
}

} // namespace config
