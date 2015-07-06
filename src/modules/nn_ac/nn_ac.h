#pragma once
#include "kernelmodule.h"

class NNAc : public KernelModuleImpl<NNAc>
{
public:
   NNAc();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
