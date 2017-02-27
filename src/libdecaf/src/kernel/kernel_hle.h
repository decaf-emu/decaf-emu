#pragma once
#include <cstdint>
#include <string>

namespace kernel
{

class HleModule;

void
initialiseHleMmodules();

HleModule *
findHleModule(const std::string &name);

uint32_t
registerUnimplementedHleFunc(const std::string &module,
                             const std::string &name);

} // namespace kernel
