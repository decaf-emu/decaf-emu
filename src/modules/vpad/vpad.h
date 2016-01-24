#pragma once
#include "kernelmodule.h"

namespace vpad
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerStatusFunctions();
};

} // namespace vpad
