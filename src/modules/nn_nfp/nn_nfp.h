#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nn
{

namespace nfp
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
   static void registerDetectionFunctions();
   static void registerSettingsFunctions();
};

} // namespace nfp

} // namespace nn
