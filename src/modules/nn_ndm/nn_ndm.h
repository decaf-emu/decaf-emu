#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace ndm
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

} // namespace ndm

} // namespace nn
