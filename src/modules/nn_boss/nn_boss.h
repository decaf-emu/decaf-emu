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

   void initialiseTaskSetting();
   void initialiseNetTaskSetting();
   void initialiseRawUlTaskSetting();
   void initialisePlayReportSetting();

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
   static void registerTitleIDFunctions();
   static void registerTaskSettingFunctions();
   static void registerNetTaskSettingFunctions();
   static void registerRawUlTaskSettingFunctions();
   static void registerPlayReportSettingFunctions();
};

} // namespace boss

} // namespace nn
