#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace boss
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
};

} // namespace boss

} // namespace nn
