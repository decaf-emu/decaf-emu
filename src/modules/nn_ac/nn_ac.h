#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace ac
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};

} // namespace ac

} // namespace nn
