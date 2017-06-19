#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nsysnet
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   Module();

   virtual void initialise() override;

   void initialiseSocketLib();

public:
   static void RegisterFunctions();

private:
   static void registerEndianFunctions();
   static void registerSocketLibFunctions();
   static void registerSSLFunctions();
};

} // namespace nsysnet
