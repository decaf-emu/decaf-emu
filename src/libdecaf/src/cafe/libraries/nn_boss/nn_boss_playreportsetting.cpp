#include "nn_boss.h"
#include "nn_boss_playreportsetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<hle::VirtualTable> PlayReportSetting::VirtualTable = nullptr;
virt_ptr<hle::TypeDescriptor> PlayReportSetting::TypeDescriptor = nullptr;

PlayReportSetting::PlayReportSetting() :
   mPlayReportUnk1(0u),
   mPlayReportUnk2(0u),
   mPlayReportUnk3(0u),
   mPlayReportUnk4(0u)
{
   mVirtualTable = PlayReportSetting::VirtualTable;
}

PlayReportSetting::~PlayReportSetting()
{
}

void
PlayReportSetting::Initialize(virt_ptr<void> a1,
                              uint32_t a2)
{
   decaf_warn_stub();
}

nn::Result
PlayReportSetting::RegisterPreprocess(uint32_t a1,
                                      virt_ptr<TitleID> a2,
                                      virt_ptr<const char> a3)
{
   decaf_warn_stub();
   return ResultInvalidParameter;
}

bool
PlayReportSetting::Set(virt_ptr<const char> key,
                       uint32_t value)
{
   decaf_warn_stub();
   return true;
}

bool
PlayReportSetting::Set(uint32_t key,
                       uint32_t value)
{
   decaf_warn_stub();
   return true;
}

void
Library::registerPlayReportSettingSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss17PlayReportSettingFv",
                             PlayReportSetting);
   RegisterDestructorExport("__dt__Q3_2nn4boss17PlayReportSettingFv",
                            PlayReportSetting);

   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss17PlayReportSettingFUiQ3_2nn4boss7TitleIDPCc",
                              &PlayReportSetting::RegisterPreprocess);
   RegisterFunctionExportName("Initialize__Q3_2nn4boss17PlayReportSettingFPvUi",
                              &PlayReportSetting::Initialize);
   RegisterFunctionExportName("Set__Q3_2nn4boss17PlayReportSettingFPCcUi",
                              static_cast<bool (PlayReportSetting::*)(virt_ptr<const char>, uint32_t)>(&PlayReportSetting::Set));
   RegisterFunctionExportName("Set__Q3_2nn4boss17PlayReportSettingFUiT1",
                              static_cast<bool (PlayReportSetting::*)(uint32_t, uint32_t)>(&PlayReportSetting::Set));
}

}  // namespace cafe::nn_boss
