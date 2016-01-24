#pragma once
#include "kernelmodule.h"

namespace snd_core
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerAiFunctions();
   static void registerCoreFunctions();
   static void registerDeviceFunctions();
};

} // namespace snd_core
