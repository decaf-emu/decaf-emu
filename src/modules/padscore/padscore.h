#pragma once
#include "kernelmodule.h"

namespace padscore
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerKPADFunctions();
   static void registerKPADStatusFunctions();
   static void registerWPADFunctions();
};

} // namespace padscore
