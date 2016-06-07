#pragma once
#include "kernel/kernel_hlemodule.h"

namespace nn
{

namespace boss
{

class Module : public kernel::HleModuleImpl<Module>
{
public:
   virtual void initialise() override;

   void initialiseTask();
   void initialiseTaskSetting();
   void initialiseTitle();
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
   static void registerTitleFunctions();
   static void registerTitleIDFunctions();
   static void registerNetTaskSettingFunctions();
   static void registerRawUlTaskSettingFunctions();
   static void registerPlayReportSettingFunctions();
};

} // namespace boss

} // namespace nn
