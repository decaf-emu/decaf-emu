#include "nn_boss.h"
#include "nn_boss_playreportsetting.h"
#include "nn_boss_result.h"

namespace nn
{

namespace boss
{

ghs::VirtualTableEntry *
PlayReportSetting::VirtualTable = nullptr;

ghs::TypeDescriptor *
PlayReportSetting::TypeInfo = nullptr;

PlayReportSetting::PlayReportSetting()
{
   mVirtualTable = PlayReportSetting::VirtualTable;

   mPlayReportUnk1 = 0;
   mPlayReportUnk2 = 0;
   mPlayReportUnk3 = 0;
   mPlayReportUnk4 = 0;
}

PlayReportSetting::~PlayReportSetting()
{
}

nn::Result
PlayReportSetting::RegisterPreprocess(uint32_t, nn::boss::TitleID *id, const char *)
{
   return nn::boss::Success;
}

void
PlayReportSetting::Initialize(void *, uint32_t)
{
}

bool
PlayReportSetting::Set(const char *key, uint32_t value)
{
   return true;
}

bool
PlayReportSetting::Set(uint32_t key, uint32_t value)
{
   return true;
}

void Module::registerPlayReportSettingFunctions()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss17PlayReportSettingFv", PlayReportSetting);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn4boss17PlayReportSettingFv", PlayReportSetting);
   RegisterKernelFunctionName("RegisterPreprocess__Q3_2nn4boss17PlayReportSettingFUiQ3_2nn4boss7TitleIDPCc", &PlayReportSetting::RegisterPreprocess);
   RegisterKernelFunctionName("Initialize__Q3_2nn4boss17PlayReportSettingFPvUi", &PlayReportSetting::Initialize);
   RegisterKernelFunctionName("Set__Q3_2nn4boss17PlayReportSettingFPCcUi", static_cast<bool(PlayReportSetting::*)(const char*, uint32_t)>(&PlayReportSetting::Set));
   RegisterKernelFunctionName("Set__Q3_2nn4boss17PlayReportSettingFUiT1", static_cast<bool(PlayReportSetting::*)(uint32_t, uint32_t)>(&PlayReportSetting::Set));
}

void Module::initialisePlayReportSetting()
{
   PlayReportSetting::TypeInfo = ghs::internal::makeTypeDescriptor("nn::boss::PlayReportSetting", {
      { RawUlTaskSetting::TypeInfo, 0x1600 },
   });

   PlayReportSetting::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, PlayReportSetting::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn4boss17PlayReportSettingFv") },
      { 0, findExportAddress("RegisterPreprocess__Q3_2nn4boss17PlayReportSettingFUiQ3_2nn4boss7TitleIDPCc") },
      RawUlTaskSetting::VirtualTable[3],
   });
}

}  // namespace boss

}  // namespace nn
