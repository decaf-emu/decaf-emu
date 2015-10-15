#pragma once
#include "kernelmodule.h"

class NN_ac : public KernelModuleImpl<NN_ac>
{
public:
   NN_ac();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
