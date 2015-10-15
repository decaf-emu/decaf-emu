#pragma once
#include "kernelmodule.h"

class NN_fp : public KernelModuleImpl<NN_fp>
{
public:
   NN_fp();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
