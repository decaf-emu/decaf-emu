#include "nn_boss.h"
#include "nn_boss_tasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/boss/nn_boss_result.h"

namespace cafe::nn_boss
{

using namespace nn::boss;

virt_ptr<ghs::VirtualTable> TaskSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> TaskSetting::TypeDescriptor = nullptr;

virt_ptr<TaskSetting>
TaskSetting_Constructor(virt_ptr<TaskSetting> self)
{
   if (!self) {
      self = virt_cast<TaskSetting *>(ghs::malloc(sizeof(TaskSetting)));
      if (!self) {
         return nullptr;
      }
   }

   self->virtualTable = TaskSetting::VirtualTable;
   TaskSetting_InitializeSetting(self);
   return self;
}

void
TaskSetting_Destructor(virt_ptr<TaskSetting> self,
                       ghs::DestructorFlags flags)
{
   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

void
TaskSetting_InitializeSetting(virt_ptr<TaskSetting> self)
{
   std::memset(self.get(), 0, 0x1000);
   self->unk0x00 = 0u;
   self->unk0x08 = 0u;
   self->unk0x0C = 0u;
   self->priority = uint8_t { 125 };
   self->intervalSec = 28800u;
   self->lifeTimeSec = 7776000u;
}

void
TaskSetting_SetRunPermissionInParentalControlRestriction(virt_ptr<TaskSetting> self,
                                                         bool value)
{
   if (value) {
      self->permission |= 2u;
   } else {
      self->permission &= ~2u;
   }
}

nn::Result
TaskSetting_RegisterPreprocess(virt_ptr<TaskSetting> self,
                               uint32_t a1,
                               virt_ptr<TitleID> a2,
                               virt_ptr<const char> a3)
{
   return ResultSuccess;
}

void
TaskSetting_RegisterPostprocess(virt_ptr<TaskSetting> self,
                                uint32_t a1,
                                virt_ptr<TitleID> a2,
                                virt_ptr<const char> a3,
                                virt_ptr<nn::Result> a4)
{
}

nn::Result
PrivateTaskSetting_SetIntervalSecForShortSpanRetry(virt_ptr<TaskSetting> self,
                                                   uint16_t sec)
{
   self->intervalSecForShortSpanRetry = sec;
   return ResultSuccess;
}

nn::Result
PrivateTaskSetting_SetIntervalSec(virt_ptr<TaskSetting> self,
                                  uint32_t sec)
{
   self->intervalSec = sec;
   return ResultSuccess;
}

nn::Result
PrivateTaskSetting_SetLifeTimeSec(virt_ptr<TaskSetting> self,
                                  uint64_t lifeTimeSec)
{
   self->lifeTimeSec = lifeTimeSec;
   return ResultSuccess;
}

nn::Result
PrivateTaskSetting_SetPermission(virt_ptr<TaskSetting> self,
                                 uint8_t permission)
{
   self->permission = permission;
   return ResultSuccess;
}

nn::Result
PrivateTaskSetting_SetPriority(virt_ptr<TaskSetting> self,
                               TaskPriority priority)
{
   self->priority = priority;
   return ResultSuccess;
}

void
Library::registerTaskSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss11TaskSettingFv",
                              TaskSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss11TaskSettingFv",
                              TaskSetting_Destructor);
   RegisterFunctionExportName("InitializeSetting__Q3_2nn4boss11TaskSettingFv",
                              TaskSetting_InitializeSetting);
   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
                              TaskSetting_RegisterPreprocess);
   RegisterFunctionExportName("RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
                              TaskSetting_RegisterPostprocess);
   RegisterFunctionExportName("SetRunPermissionInParentalControlRestriction__Q3_2nn4boss11TaskSettingFb",
                              TaskSetting_SetRunPermissionInParentalControlRestriction);

   RegisterFunctionExportName("SetIntervalSecForShortSpanRetry__Q3_2nn4boss18PrivateTaskSettingSFRQ3_2nn4boss11TaskSettingUs",
                              PrivateTaskSetting_SetIntervalSecForShortSpanRetry);
   RegisterFunctionExportName("SetIntervalSec__Q3_2nn4boss18PrivateTaskSettingSFRQ3_2nn4boss11TaskSettingUi",
                              PrivateTaskSetting_SetIntervalSec);
   RegisterFunctionExportName("SetLifeTimeSec__Q3_2nn4boss18PrivateTaskSettingSFRQ3_2nn4boss11TaskSettingUL",
                              PrivateTaskSetting_SetLifeTimeSec);
   RegisterFunctionExportName("SetPermission__Q3_2nn4boss18PrivateTaskSettingSFRQ3_2nn4boss11TaskSettingUc",
                              PrivateTaskSetting_SetPermission);
   RegisterFunctionExportName("SetPriority__Q3_2nn4boss18PrivateTaskSettingSFRQ3_2nn4boss11TaskSettingQ3_2nn4boss12TaskPriority",
                              PrivateTaskSetting_SetPriority);

   registerTypeInfo<TaskSetting>(
      "nn::boss::TaskSetting",
      {
         "__dt__Q3_2nn4boss11TaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      });
}

}  // namespace namespace cafe::nn_boss
