#pragma once
#include "kernelmodule.h"

class NNTemp : public KernelModuleImpl<NNTemp>
{
public:
   NNTemp();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();

};
