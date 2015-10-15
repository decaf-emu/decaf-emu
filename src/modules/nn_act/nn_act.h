#pragma once
#include "kernelmodule.h"

class NN_act : public KernelModuleImpl<NN_act>
{
public:
   NN_act();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
