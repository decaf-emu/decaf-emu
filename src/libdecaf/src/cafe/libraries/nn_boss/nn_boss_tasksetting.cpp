#include "nn_boss.h"
#include "nn_boss_tasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::boss
{

TaskSetting::TaskSetting() :
   mVirtualTable(TaskSetting::VirtualTable)
{
   InitializeSetting();
}

TaskSetting::~TaskSetting()
{
}

void
TaskSetting::InitializeSetting()
{
   decaf_warn_stub();
   std::memset(virt_addrof(mTaskSettingData).getRawPointer(),
               0,
               sizeof(TaskSettingData));

   mTaskSettingData.unk0x00 = 0u;
   mTaskSettingData.unk0x08 = 0u;
   mTaskSettingData.unk0x0C = 0u;
   mTaskSettingData.unk0x2A = uint8_t { 125 };
   mTaskSettingData.unk0x30 = 28800u;
   mTaskSettingData.unk0x38 = 0u;
   mTaskSettingData.unk0x3C = 7776000u;
}

void
TaskSetting::SetRunPermissionInParentalControlRestriction(bool value)
{
   if (value) {
      mTaskSettingData.unk0x2C |= 2u;
   } else {
      mTaskSettingData.unk0x2C &= ~2u;
   }
}

nn::Result
TaskSetting::RegisterPreprocess(uint32_t a1,
                                virt_ptr<TitleID> a2,
                                virt_ptr<const char> a3)
{
   return 2097280u;
}

void
TaskSetting::RegisterPostprocess(uint32_t a1,
                                 virt_ptr<TitleID> a2,
                                 virt_ptr<const char> a3,
                                 virt_ptr<nn::Result> a4)
{
}

void
Library::registerTaskSettingSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss11TaskSettingFv",
                             TaskSetting);
   RegisterDestructorExport("__dt__Q3_2nn4boss11TaskSettingFv",
                            TaskSetting);
   RegisterFunctionExportName("InitializeSetting__Q3_2nn4boss11TaskSettingFv",
                              &TaskSetting::InitializeSetting);
   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
                              &TaskSetting::RegisterPreprocess);
   RegisterFunctionExportName("RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
                              &TaskSetting::RegisterPostprocess);
   RegisterFunctionExportName("SetRunPermissionInParentalControlRestriction__Q3_2nn4boss11TaskSettingFb",
                              &TaskSetting::SetRunPermissionInParentalControlRestriction);

   registerTypeInfo<TaskSetting>(
      "nn::boss::TaskSetting",
      {
         "__dt__Q3_2nn4boss11TaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      });
}

}  // namespace namespace cafe::nn::boss
