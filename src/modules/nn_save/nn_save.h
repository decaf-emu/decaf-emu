#pragma once
#include "kernelmodule.h"
#include "log.h"

class NNSave : public KernelModuleImpl<NNSave>
{
public:
   NNSave();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
