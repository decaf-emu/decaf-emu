#pragma once
#include "systemmodule.h"

class CoreInit : public SystemModule
{
public:
   CoreInit();

private:
   void registerDebugFunctions();
   void registerMemoryFunctions();
   void registerMutexFunctions();
   void registerSystemInfoFunctions();
   void registerThreadFunctions();
};