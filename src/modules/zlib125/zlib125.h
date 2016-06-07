#pragma once
#include "kernel/kernel_hlemodule.h"

namespace zlib125
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual ~Module() = default;

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};

} // namespace zlib125
