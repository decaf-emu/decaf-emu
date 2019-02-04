#include "nn_boss.h"
#include "nn_boss_tasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_boss
{

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
   self->unk0x2A = uint8_t { 125 };
   self->unk0x30 = 28800u;
   self->unk0x38 = 0u;
   self->unk0x3C = 7776000u;
}

void
TaskSetting_SetRunPermissionInParentalControlRestriction(virt_ptr<TaskSetting> self,
                                                         bool value)
{
   if (value) {
      self->unk0x2C |= 2u;
   } else {
      self->unk0x2C &= ~2u;
   }
}

nn::Result
TaskSetting_RegisterPreprocess(virt_ptr<TaskSetting> self,
                               uint32_t a1,
                               virt_ptr<TitleID> a2,
                               virt_ptr<const char> a3)
{
   return 2097280u;
}

void
TaskSetting_RegisterPostprocess(virt_ptr<TaskSetting> self,
                                uint32_t a1,
                                virt_ptr<TitleID> a2,
                                virt_ptr<const char> a3,
                                virt_ptr<nn::Result> a4)
{
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

   registerTypeInfo<TaskSetting>(
      "nn::boss::TaskSetting",
      {
         "__dt__Q3_2nn4boss11TaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      });
}

}  // namespace namespace cafe::nn_boss
