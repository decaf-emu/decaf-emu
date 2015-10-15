#pragma once
#include "kernelmodule.h"

class NN_nfp : public KernelModuleImpl<NN_nfp>
{
public:
   NN_nfp();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
