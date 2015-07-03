#pragma once
#include "kernelmodule.h"
#include "log.h"

class PadScore : public KernelModuleImpl<PadScore>
{
public:
   PadScore();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerKPADFunctions();
   static void registerWPADFunctions();
};
