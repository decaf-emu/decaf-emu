#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace temp
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerDirFunctions();
};

} // namespace temp

} // namespace nn
