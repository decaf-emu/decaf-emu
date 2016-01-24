#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace nfp
{

class Module : public KernelModuleImpl<Module>
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
