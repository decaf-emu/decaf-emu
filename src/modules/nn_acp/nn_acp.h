#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace acp
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerTitleFunctions();
};

} // namespace acp

} // namespace nn
