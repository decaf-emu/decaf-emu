#include "nn_boss.h"
#include "nn_boss_tasksetting.h"
#include "nn_boss_result.h"

namespace nn
{

namespace boss
{

ghs::VirtualTableEntry *
TaskSetting::VirtualTable = nullptr;

ghs::TypeDescriptor *
TaskSetting::TypeInfo = nullptr;

TaskSetting::TaskSetting()
{
   mVirtualTable = TaskSetting::VirtualTable;
   InitializeSetting();
}

TaskSetting::~TaskSetting()
{
}

void TaskSetting::InitializeSetting()
{
   std::memset(mTaskSettingData, 0, 0x1000);

   // TODO: TaskSetting::InitializeSetting
   // 0x00 uint32_t = 0
   // 0x08 uint32_t = 0
   // 0x0C uint32_t = 0
   // 0x2A uint8_t = 0x7D
   // 0x30 uint32_t = 0x7080
   // 0x38 uint32_t = 0
   // 0x3C uint32_t = 0x76A700
}

void
TaskSetting::SetRunPermissionInParentalControlRestriction(bool value)
{
   // TODO: TaskSetting::SetRunPermissionInParentalControlRestriction
   if (value) {
      // 0x2C uint8_t |= 0x02
   } else {
      // 0x2c uint8_t &= ~0x02
   }
}

nn::Result
TaskSetting::RegisterPreprocess(uint32_t, nn::boss::TitleID *id, const char *)
{
   return nn::boss::Success;
}

void
TaskSetting::RegisterPostprocess(uint32_t, nn::boss::TitleID *id, const char *, nn::Result *)
{
}

void Module::registerTaskSettingFunctions()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss11TaskSettingFv", TaskSetting);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn4boss11TaskSettingFv", TaskSetting);
   RegisterKernelFunctionName("InitializeSetting__Q3_2nn4boss11TaskSettingFv", &TaskSetting::InitializeSetting);
   RegisterKernelFunctionName("RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc", &TaskSetting::RegisterPreprocess);
   RegisterKernelFunctionName("RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result", &TaskSetting::RegisterPostprocess);
   RegisterKernelFunctionName("SetRunPermissionInParentalControlRestriction__Q3_2nn4boss11TaskSettingFb", &TaskSetting::SetRunPermissionInParentalControlRestriction);
}

void Module::initialiseTaskSetting()
{
   TaskSetting::TypeInfo = ghs::internal::makeTypeDescriptor("nn::boss::TaskSetting");

   TaskSetting::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, TaskSetting::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn4boss11TaskSettingFv") },
      { 0, findExportAddress("RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc") },
      { 0, findExportAddress("RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result") },
   });
}

}  // namespace boss

}  // namespace nn
