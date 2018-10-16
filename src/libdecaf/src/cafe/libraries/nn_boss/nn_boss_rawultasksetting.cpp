#include "nn_boss.h"
#include "nn_boss_rawultasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<hle::VirtualTable> RawUlTaskSetting::VirtualTable = nullptr;
virt_ptr<hle::TypeDescriptor> RawUlTaskSetting::TypeDescriptor = nullptr;

RawUlTaskSetting::RawUlTaskSetting() :
   mRawUlUnk1(0u),
   mRawUlUnk2(0u),
   mRawUlUnk3(0u)
{
   mVirtualTable = RawUlTaskSetting::VirtualTable;
   std::memset(virt_addrof(mRawUlData).get(), 0, mRawUlData.size());
}

RawUlTaskSetting::~RawUlTaskSetting()
{
}

nn::Result
RawUlTaskSetting::RegisterPreprocess(uint32_t a1,
                                     virt_ptr<TitleID> a2,
                                     virt_ptr<const char> a3)
{
   decaf_warn_stub();
   return ResultInvalidParameter;
}

void
RawUlTaskSetting::RegisterPostprocess(uint32_t a1,
                                      virt_ptr<TitleID> a2,
                                      virt_ptr<const char> a3,
                                      virt_ptr<nn::Result> a4)
{
   decaf_warn_stub();
}

void
Library::registerRawUlTaskSettingSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss16RawUlTaskSettingFv",
                             RawUlTaskSetting);
   RegisterDestructorExport("__dt__Q3_2nn4boss16RawUlTaskSettingFv",
                            RawUlTaskSetting);

   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCc",
                              &RawUlTaskSetting::RegisterPreprocess);
   RegisterFunctionExportName("RegisterPostprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
                              &RawUlTaskSetting::RegisterPostprocess);

   registerTypeInfo<TaskSetting>(
      "nn::boss::RawUlTaskSetting",
      {
         "__dt__Q3_2nn4boss16RawUlTaskSettingFv",
         "RegisterPreprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCc",
         "RegisterPostprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
      },
      {
         "nn::boss::NetTaskSetting",
      });
}

}  // namespace cafe::nn_boss
