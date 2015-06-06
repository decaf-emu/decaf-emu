#pragma once
#include "systemmodule.h"

class GX2 : public SystemModule
{
public:
   GX2();

   virtual void initialise() override;

private:
   void registerTextureFunctions();
};
