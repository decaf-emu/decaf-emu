#include "nn_boss.h"
#include "nn_boss_rawultasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"
#include "nn/boss/nn_boss_result.h"

using namespace nn::boss;

namespace cafe::nn_boss
{

virt_ptr<ghs::VirtualTable> RawUlTaskSetting::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> RawUlTaskSetting::TypeDescriptor = nullptr;

virt_ptr<RawUlTaskSetting>
RawUlTaskSetting_Constructor(virt_ptr<RawUlTaskSetting> self)
{
   if (!self) {
      self = virt_cast<RawUlTaskSetting *>(ghs::malloc(sizeof(RawUlTaskSetting)));
      if (!self) {
         return nullptr;
      }
   }

   NetTaskSetting_Constructor(virt_cast<NetTaskSetting *>(self));
   self->virtualTable = RawUlTaskSetting::VirtualTable;
   self->rawUlUnk1 = 0u;
   self->rawUlUnk2 = 0u;
   self->rawUlUnk3 = 0u;
   std::memset(virt_addrof(self->rawUlData).get(), 0, self->rawUlData.size());
   return self;
}

void
RawUlTaskSetting_Destructor(virt_ptr<RawUlTaskSetting> self,
                            ghs::DestructorFlags flags)
{
   NetTaskSetting_Destructor(virt_cast<NetTaskSetting *>(self),
                             ghs::DestructorFlags::None);

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

nn::Result
RawUlTaskSetting_RegisterPreprocess(virt_ptr<RawUlTaskSetting> self,
                                    uint32_t a1,
                                    virt_ptr<TitleID> a2,
                                    virt_ptr<const char> a3)
{
   decaf_warn_stub();
   return ResultInvalidParameter;
}

void
RawUlTaskSetting_RegisterPostprocess(virt_ptr<RawUlTaskSetting> self,
                                     uint32_t a1,
                                     virt_ptr<TitleID> a2,
                                     virt_ptr<const char> a3,
                                     virt_ptr<nn::Result> a4)
{
   decaf_warn_stub();
}

void
Library::registerRawUlTaskSettingSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn4boss16RawUlTaskSettingFv",
                              RawUlTaskSetting_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn4boss16RawUlTaskSettingFv",
                              RawUlTaskSetting_Destructor);

   RegisterFunctionExportName("RegisterPreprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCc",
                              RawUlTaskSetting_RegisterPreprocess);
   RegisterFunctionExportName("RegisterPostprocess__Q3_2nn4boss16RawUlTaskSettingFUiQ3_2nn4boss7TitleIDPCcQ2_2nn6Result",
                              RawUlTaskSetting_RegisterPostprocess);

   registerTypeInfo<RawUlTaskSetting>(
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
