#pragma once
#include "kernel/kernel_hlemodule.h"

namespace snd_user
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerAXFXFunctions();
};

} // namespace snd_user
