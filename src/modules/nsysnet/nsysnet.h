#pragma once
#include "kernelmodule.h"

namespace nsysnet
{

class Module : public KernelModuleImpl<Module>
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
