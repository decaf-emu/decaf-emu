#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace save
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerCoreFunctions();
   static void registerDirFunctions();
   static void registerFileFunctions();
};

} // namespace save

} // namespace nn
