#include "nn_boss.h"
#include "nn_boss_playloguploadtasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> PlayLogUploadTaskSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> PlayLogUploadTaskSetting::TypeDescriptor = nullptr;

virt_ptr<PlayLogUploadTaskSetting>
PlayLogUploadTaskSetting_Constructor(virt_ptr<PlayLogUploadTaskSetting> self)
{
   if (!self) {
      self = virt_cast<PlayLogUploadTaskSetting *>(ghs::malloc(sizeof(PlayLogUploadTaskSetting)));
      if (!self) {
         return nullptr;
      }
   }

   RawUlTaskSetting_Constructor(virt_cast<RawUlTaskSetting *>(self));
   self->virtualTable = PlayLogUploadTaskSetting::VirtualTable;
   return self;
}

void
PlayLogUploadTaskSetting_Destructor(virt_ptr<PlayLogUploadTaskSetting> self,
                                    ghs::DestructorFlags flags)
{
   RawUlTaskSetting_Destructor(virt_cast<RawUlTaskSetting *>(self),
                               ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
PlayLogUploadTaskSetting_Initialize(virt_ptr<PlayLogUploadTaskSetting> self)
{
   PlayLogUploadTaskSetting_SetPlayLogUploadTaskSettingToRecord(self);
   return ResultSuccess;
}

nn::Result
PlayLogUploadTaskSetting_RegisterPreprocess(virt_ptr<PlayLogUploadTaskSetting> self,
                                            uint32_t a1,
                                            virt_ptr<TitleID> a2,
                                            virt_ptr<const char> a3)
{
   return ResultSuccess;
}

void
PlayLogUploadTaskSetting_SetPlayLogUploadTaskSettingToRecord(virt_ptr<PlayLogUploadTaskSetting> self)
{
   self->unk0x28 = uint16_t { 5 };
   self->permission |= 0x1A;
}

void
Library::registerPlayLogUploadTaskSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss24PlayLogUploadTaskSettingFv",
                              PlayLogUploadTaskSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss24PlayLogUploadTaskSettingFv",
                              PlayLogUploadTaskSetting_Destructor);

   RegisterFunctionExportName("Initialize__Q3_2nn4boss24PlayLogUploadTaskSettingFv",
                              PlayLogUploadTaskSetting_Initialize);
   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss24PlayLogUploadTaskSettingFUiQ3_2nn4boss7TitleIDPCc",
                              PlayLogUploadTaskSetting_RegisterPreprocess);
   RegisterFunctionExportName("SetPlayLogUploadTaskSettingToRecord__Q3_2nn4boss24PlayLogUploadTaskSettingFv",
                              PlayLogUploadTaskSetting_SetPlayLogUploadTaskSettingToRecord);

   registerTypeInfo<PlayLogUploadTaskSetting>(
      "nn::boss::PlayLogUploadTaskSetting",
      {
         "__dt__Q3_2nn4boss24PlayLogUploadTaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss24PlayLogUploadTaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      },
      {
         "nn::boss::PlayLogUploadTaskSetting",
      });
}

}  // namespace cafe::nn_boss
