#include "decaf_config.h"
#include "kernel_hle.h"

#include <string>
#include <map>

namespace kernel
{

static std::map<std::string, HleModule *, std::less<>>
sHleModules;

uint32_t
registerUnimplementedHleFunc(const std::string_view &module,
                             const std::string_view &name)
{
   return 0;
}

void
registerHleModule(const std::string &name,
                  HleModule *module)
{
}

void
registerHleModuleAlias(const std::string &module,
                       const std::string &alias)
{
}

HleModule *
findHleModule(const std::string_view &name)
{
   return nullptr;
}

} // namespace kernel
