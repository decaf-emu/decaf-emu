#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nn
{

namespace acp
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerDeviceFunctions();
   static void registerTitleFunctions();
};

} // namespace acp

} // namespace nn
