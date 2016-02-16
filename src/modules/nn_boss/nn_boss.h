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

   void initialiseTask();
   void initialiseTaskSetting();
   void initialiseNetTaskSetting();
   void initialiseRawUlTaskSetting();
   void initialisePlayReportSetting();

public:
   static void RegisterFunctions();

private:
   static void registerInitFunctions();
   static void registerTask();
   static void registerTaskID();
   static void registerTaskSettingFunctions();
   static void registerTitleIDFunctions();
   static void registerNetTaskSettingFunctions();
   static void registerRawUlTaskSettingFunctions();
   static void registerPlayReportSettingFunctions();
};

} // namespace boss

} // namespace nn
