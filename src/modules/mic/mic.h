#pragma once
#include "kernelmodule.h"

class Mic : public KernelModuleImpl<Mic>
{
public:
   Mic();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
