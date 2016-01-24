#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace act
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

} // namespace act

} // namespace nn
