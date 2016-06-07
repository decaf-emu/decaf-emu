#pragma once
#include "kernel/kernel_hlemodule.h"

namespace padscore
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerKPADFunctions();
   static void registerKPADStatusFunctions();
   static void registerWPADFunctions();
};

} // namespace padscore
