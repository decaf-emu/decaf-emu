#pragma once
#include <cstdint>
#include <string_view>

namespace kernel
{

class HleModule;

HleModule *
findHleModule(const std::string_view &name);

uint32_t
registerUnimplementedHleFunc(const std::string_view &module,
                             const std::string_view &name);

} // namespace kernel
