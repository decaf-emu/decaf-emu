#pragma once
#include "kernelmodule.h"

class GameLoader : public KernelModuleImpl<GameLoader>
{
public:
   GameLoader();

   virtual void initialise() override;

public:
   static void RegisterFunctions();

private:

};

void GameLoaderInit(const char *rpxName);
