#pragma once
#include "systemmodule.h"

class CoreInit : public SystemModule
{
public:
   CoreInit();

private:
   void registerDebugFunctions();
   void registerThreadFunctions();
};