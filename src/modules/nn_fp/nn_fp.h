#pragma once
#include "kernelmodule.h"

namespace nn
{

namespace fp
{

class Module : public KernelModuleImpl<Module>
{
public:
   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
   static void registerFriendsFunctions();
};

} // namespace fp

} // namespace nn
