#pragma once
#include "kernelmodule.h"

class Swkbd : public KernelModuleImpl<Swkbd>
{
public:
   Swkbd();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();

};
