#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nsyskbd
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerKprFunctions();
   static void registerSkbdFunctions();
};

} // namespace nsyskbd
