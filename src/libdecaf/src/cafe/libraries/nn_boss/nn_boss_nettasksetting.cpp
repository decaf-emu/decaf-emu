#include "nn_boss.h"
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> NetTaskSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> NetTaskSetting::TypeDescriptor = nullptr;

virt_ptr<NetTaskSetting>
NetTaskSetting_Constructor(virt_ptr<NetTaskSetting> self)
{
   if (!self) {
      self = virt_cast<NetTaskSetting *>(ghs::malloc(sizeof(NetTaskSetting)));
      if (!self) {
         return nullptr;
      }
   }

   TaskSetting_Constructor(virt_cast<TaskSetting *>(self));
   self->virtualTable = NetTaskSetting::VirtualTable;
   self->unk0x18C = 120u;
   return self;
}

void
NetTaskSetting_Destructor(virt_ptr<NetTaskSetting> self,
                          ghs::DestructorFlags flags)
{
   TaskSetting_Destructor(virt_cast<TaskSetting *>(self),
                          ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

void
Library::registerNetTaskSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss14NetTaskSettingFv",
                              NetTaskSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss14NetTaskSettingFv",
                              NetTaskSetting_Destructor);

   registerTypeInfo<NetTaskSetting>(
      "nn::boss::NetTaskSetting",
      {
         "__dt__Q3_2nn4boss14NetTaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      },
      {
         "nn::boss::TaskSetting",
      });
}

}  // namespace cafe::nn_boss
