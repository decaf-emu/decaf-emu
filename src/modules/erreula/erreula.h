#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace erreula
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerErrorViewerFunctions();
};

} // namespace erreula

} // namespace nn
