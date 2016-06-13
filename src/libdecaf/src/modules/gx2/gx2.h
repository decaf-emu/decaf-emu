#pragma once
#include "kernel/kernel_hlemodule.h"

namespace gx2
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

   void initialiseVsync();
   void initialiseResourceAllocator();

public:
   static void RegisterFunctions();

   static void RegisterGX2RResourceFunctions();
};

} // namespace gx2
