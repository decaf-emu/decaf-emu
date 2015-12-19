#pragma once
#include "kernelmodule.h"

class Snd_Core : public KernelModuleImpl<Snd_Core>
{
public:
   Snd_Core();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerAiFunctions();
   static void registerCoreFunctions();
   static void registerDeviceFunctions();
};
