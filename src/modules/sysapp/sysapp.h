#pragma once
#include "kernelmodule.h"

namespace sysapp
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerTitleFunctions();
};

} // namespace sysapp
