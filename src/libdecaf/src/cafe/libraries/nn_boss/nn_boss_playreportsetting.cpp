#include "nn_boss.h"
#include "nn_boss_playreportsetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> PlayReportSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> PlayReportSetting::TypeDescriptor = nullptr;

virt_ptr<PlayReportSetting>
PlayReportSetting_Constructor(virt_ptr<PlayReportSetting> self)
{
   if (!self) {
      self = virt_cast<PlayReportSetting *>(ghs::malloc(sizeof(PlayReportSetting)));
      if (!self) {
         return nullptr;
      }
   }

   RawUlTaskSetting_Constructor(virt_cast<RawUlTaskSetting *>(self));
   self->virtualTable = PlayReportSetting::VirtualTable;
   self->playReportUnk1 = 0u;
   self->playReportUnk2 = 0u;
   self->playReportUnk3 = 0u;
   self->playReportUnk4 = 0u;
   return self;
}

void
PlayReportSetting_Destructor(virt_ptr<PlayReportSetting> self,
                             ghs::DestructorFlags flags)
{
   RawUlTaskSetting_Destructor(virt_cast<RawUlTaskSetting *>(self),
                               ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

void
PlayReportSetting_Initialize(virt_ptr<PlayReportSetting> self,
                             virt_ptr<void> a1,
                             uint32_t a2)
{
   decaf_warn_stub();
}

nn::Result
PlayReportSetting_RegisterPreprocess(virt_ptr<PlayReportSetting> self,
                                     uint32_t a1,
                                     virt_ptr<TitleID> a2,
                                     virt_ptr<const char> a3)
{
   decaf_warn_stub();
   return ResultInvalidParameter;
}

bool
PlayReportSetting_Set(virt_ptr<PlayReportSetting> self,
                      virt_ptr<const char> key,
                      uint32_t value)
{
   decaf_warn_stub();
   return true;
}

bool
PlayReportSetting_Set(virt_ptr<PlayReportSetting> self,
                      uint32_t key,
                      uint32_t value)
{
   decaf_warn_stub();
   return true;
}

void
Library::registerPlayReportSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss17PlayReportSettingFv",
                              PlayReportSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss17PlayReportSettingFv",
                              PlayReportSetting_Destructor);

   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss17PlayReportSettingFUiQ3_2nn4boss7TitleIDPCc",
                              PlayReportSetting_RegisterPreprocess);
   RegisterFunctionExportName("Initialize__Q3_2nn4boss17PlayReportSettingFPvUi",
                              PlayReportSetting_Initialize);
   RegisterFunctionExportName("Set__Q3_2nn4boss17PlayReportSettingFPCcUi",
                              static_cast<bool(*)(virt_ptr<PlayReportSetting>, virt_ptr<const char>, uint32_t)>(PlayReportSetting_Set));
   RegisterFunctionExportName("Set__Q3_2nn4boss17PlayReportSettingFUiT1",
                              static_cast<bool(*)(virt_ptr<PlayReportSetting>, uint32_t, uint32_t)>(PlayReportSetting_Set));
}

}  // namespace cafe::nn_boss
