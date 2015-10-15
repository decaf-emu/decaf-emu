#pragma once
#include "kernelmodule.h"

class NN_temp : public KernelModuleImpl<NN_temp>
{
public:
   NN_temp();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
