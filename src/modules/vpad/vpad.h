#pragma once
#include "kernelmodule.h"

class VPad : public KernelModuleImpl<VPad>
{
public:
   VPad();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerStatusFunctions();
};
