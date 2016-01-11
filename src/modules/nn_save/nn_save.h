#pragma once
#include "kernelmodule.h"

class NN_save : public KernelModuleImpl<NN_save>
{
public:
   NN_save();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerDirFunctions();
   static void registerFileFunctions();
};
