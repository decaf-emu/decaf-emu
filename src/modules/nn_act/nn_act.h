#pragma once
#include "kernelmodule.h"

class NNAct : public KernelModuleImpl<NNAct>
{
public:
   NNAct();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
