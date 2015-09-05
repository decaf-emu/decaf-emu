#pragma once
#include "kernelmodule.h"

class SysApp : public KernelModuleImpl<SysApp>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerTitleFunctions();
};
