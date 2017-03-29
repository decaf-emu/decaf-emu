#pragma once
#include "kernel/kernel_hlemodule.h"

namespace snd_core
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerAiFunctions();
   static void registerCoreFunctions();
   static void registerDeviceFunctions();
   static void registerMixFunctions();
   static void registerVoiceFunctions();
   static void registerVSFunctions();
   static void registerFXFunctions();
};

} // namespace snd_core
