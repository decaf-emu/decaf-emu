#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nsysnet
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   Module();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerSocketLibFunctions();
   static void registerSSLFunctions();
};

} // namespace nsysnet
