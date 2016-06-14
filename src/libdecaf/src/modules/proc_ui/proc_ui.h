#pragma once
#include "kernel/kernel_hlemodule.h"

namespace proc_ui
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};

} // namespace proc_ui
