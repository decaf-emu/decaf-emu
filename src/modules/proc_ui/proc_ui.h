#pragma once
#include "kernelmodule.h"
#include "log.h"

class ProcUI : public KernelModuleImpl<ProcUI>
{
public:
   ProcUI();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
