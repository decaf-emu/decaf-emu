#pragma once
#include "kernelmodule.h"

class PadScore : public KernelModuleImpl<PadScore>
{
public:
   PadScore();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerKPADFunctions();
   static void registerKPADStatusFunctions();
   static void registerWPADFunctions();
};
