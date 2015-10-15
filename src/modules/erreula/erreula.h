#pragma once
#include "kernelmodule.h"

class ErrEula : public KernelModuleImpl<ErrEula>
{
public:
   ErrEula();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerErrorViewerFunctions();
};
