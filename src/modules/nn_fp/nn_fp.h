#pragma once
#include "kernelmodule.h"

class NNFp : public KernelModuleImpl<NNFp>
{
public:
   NNFp();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
