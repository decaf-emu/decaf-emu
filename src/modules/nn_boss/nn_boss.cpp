#include "nn_boss.h"

namespace nn
{

namespace boss
{

void
Module::initialise()
{
   initialiseTask();
   initialiseTaskSetting();
   initialiseNetTaskSetting();
   initialiseRawUlTaskSetting();
   initialisePlayReportSetting();
}

void
Module::RegisterFunctions()
{
   registerInitFunctions();
   registerTask();
   registerTaskID();
   registerTaskSettingFunctions();
   registerTitleIDFunctions();
   registerNetTaskSettingFunctions();
   registerRawUlTaskSettingFunctions();
   registerPlayReportSettingFunctions();
}

} // namespace boss

} // namespace nn
