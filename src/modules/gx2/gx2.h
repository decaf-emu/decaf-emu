#pragma once
#include "kernelmodule.h"

namespace gx2
{

class Module : public KernelModuleImpl<Module>
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
