#include "nn_boss.h"

namespace nn
{

namespace boss
{

void
Module::initialise()
{
   initialiseTaskSetting();
   initialiseNetTaskSetting();
   initialiseRawUlTaskSetting();
   initialisePlayReportSetting();
}

void
Module::RegisterFunctions()
{
   registerInitFunctions();
   registerTitleIDFunctions();
   registerTaskSettingFunctions();
   registerNetTaskSettingFunctions();
   registerRawUlTaskSettingFunctions();
   registerPlayReportSettingFunctions();
}

} // namespace boss

} // namespace nn
