#pragma optimize("", off)
#include "nn_boss.h"
#include "nn_boss_nbdltasksetting.h"
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "nn/boss/nn_boss_result.h"

#include <common/strutils.h>

namespace cafe::nn_boss
{

using namespace ::nn::boss;

virt_ptr<ghs::VirtualTable> NbdlTaskSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> NbdlTaskSetting::TypeDescriptor = nullptr;

virt_ptr<NbdlTaskSetting>
NbdlTaskSetting_Constructor(virt_ptr<NbdlTaskSetting> self)
{
   if (!self) {
      self = virt_cast<NbdlTaskSetting *>(ghs::malloc(sizeof(NbdlTaskSetting)));
      if (!self) {
         return nullptr;
      }
   }

   NetTaskSetting_Constructor(virt_cast<NetTaskSetting *>(self));
   self->virtualTable = NbdlTaskSetting::VirtualTable;
   return self;
}

void
NbdlTaskSetting_Destructor(virt_ptr<NbdlTaskSetting> self,
                           ghs::DestructorFlags flags)
{
   NetTaskSetting_Destructor(virt_cast<NetTaskSetting *>(self),
                             ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
NbdlTaskSetting_Initialize(virt_ptr<NbdlTaskSetting> self,
                           virt_ptr<const char> bossCode,
                           int64_t a3,
                           virt_ptr<const char> directoryName)
{
   if (!bossCode || strnlen(bossCode.get(), 32) == 32) {
      return ResultInvalidParameter;
   }

   if (!directoryName || strnlen(directoryName.get(), 8) == 8) {
      return ResultInvalidParameter;
   }

   // TODO: Add appropriate fields to TaskSetting structure
   string_copy(reinterpret_cast<char *>(self.get()) + 0x7C0, 32, bossCode.get(), 32);
   *(reinterpret_cast<char *>(self.get()) + 0x7C0 + 32 - 1) = 0;

   string_copy(reinterpret_cast<char *>(self.get()) + 0x7E0, 8, directoryName.get(), 8);
   *(reinterpret_cast<char *>(self.get()) + 0x7E0 + 8 - 1) = 0;

   *virt_cast<int64_t *>(virt_cast<char *>(self) + 0x7F0) = a3;
   return ResultSuccess;
}

nn::Result
NbdlTaskSetting_SetFileName(virt_ptr<NbdlTaskSetting> self,
                            virt_ptr<const char> fileName)
{
   if (!fileName || strnlen(fileName.get(), 8) == 8) {
      return ResultInvalidParameter;
   }

   // TODO: Add appropriate fields to TaskSetting structure
   string_copy(reinterpret_cast<char *>(self.get()) + 0x7F8, 32, fileName.get(), 32);
   *(reinterpret_cast<char *>(self.get()) + 0x7F8 + 32 - 1) = 0;
   return ResultSuccess;
}

nn::Result
PrivateNbdlTaskSetting_SetCountryCode(virt_ptr<NbdlTaskSetting> self,
                                      virt_ptr<const char> countryCode)
{
   if (!countryCode || strnlen(countryCode.get(), 3) != 2) {
      return ResultInvalidParameter;
   }

   // TODO: Add appropriate fields to TaskSetting structure
   string_copy(reinterpret_cast<char *>(self.get()) + 0x81C, 3, countryCode.get(), 3);
   *(reinterpret_cast<char *>(self.get()) + 0x81C + 3 - 1) = 0;
   return ResultSuccess;
}

nn::Result
PrivateNbdlTaskSetting_SetLanguageCode(virt_ptr<NbdlTaskSetting> self,
                                       virt_ptr<const char> languageCode)
{
   if (!languageCode || strnlen(languageCode.get(), 3) != 2) {
      return ResultInvalidParameter;
   }

   // TODO: Add appropriate fields to TaskSetting structure
   string_copy(reinterpret_cast<char *>(self.get()) + 0x818, 3, languageCode.get(), 3);
   *(reinterpret_cast<char *>(self.get()) + 0x818 + 3 - 1) = 0;
   return ResultSuccess;
}

nn::Result
PrivateNbdlTaskSetting_SetOption(virt_ptr<NbdlTaskSetting> self,
                                 uint8_t option)
{
   // TODO: Add appropriate fields to TaskSetting structure
   *(virt_cast<uint8_t *>(self) + 0x81F) = option;
   return ResultSuccess;
}

void
Library::registerNbdlTaskSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss15NbdlTaskSettingFv",
                              NbdlTaskSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss15NbdlTaskSettingFv",
                              NbdlTaskSetting_Destructor);
   RegisterFunctionExportName("Initialize__Q3_2nn4boss15NbdlTaskSettingFPCcLT1",
                              NbdlTaskSetting_Initialize);
   RegisterFunctionExportName("SetFileName__Q3_2nn4boss15NbdlTaskSettingFPCc",
                              NbdlTaskSetting_SetFileName);

   RegisterFunctionExportName(
      "SetCountryCodeA2__Q3_2nn4boss22PrivateNbdlTaskSettingSFRQ3_2nn4boss15NbdlTaskSettingPCc",
      PrivateNbdlTaskSetting_SetCountryCode);
   RegisterFunctionExportName(
      "SetLanguageCodeA2__Q3_2nn4boss22PrivateNbdlTaskSettingSFRQ3_2nn4boss15NbdlTaskSettingPCc",
      PrivateNbdlTaskSetting_SetLanguageCode);
   RegisterFunctionExportName(
      "SetOption__Q3_2nn4boss22PrivateNbdlTaskSettingSFRQ3_2nn4boss15NbdlTaskSettingUc",
      PrivateNbdlTaskSetting_SetOption);

   registerTypeInfo<NbdlTaskSetting>(
      "nn::boss::NbdlTaskSetting",
      {
         "__dt__Q3_2nn4boss15NbdlTaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss11TaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      },
      {
         "nn::boss::NetTaskSetting",
      });
}

}  // namespace cafe::nn_boss
