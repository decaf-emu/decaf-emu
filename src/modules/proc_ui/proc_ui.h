#pragma once
#include "kernelmodule.h"

namespace proc_ui
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

} // namespace proc_ui
