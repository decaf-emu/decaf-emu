#pragma once
#include "systemmodule.h"
#include "log.h"

class GX2 : public SystemModule
{
public:
   GX2();

   virtual void initialise() override;

private:
   void registerCoreFunctions();
   void registerDisplayFunctions();
   void registerDisplayListFunctions();
   void registerRenderStateFunctions();
   void registerShaderFunctions();
   void registerTextureFunctions();
};
