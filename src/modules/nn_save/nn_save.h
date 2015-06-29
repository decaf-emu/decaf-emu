#pragma once
#include "systemmodule.h"
#include "log.h"

class NNSave : public SystemModuleImpl<NNSave>
{
public:
   NNSave();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
};
