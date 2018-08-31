#include "nn_boss.h"
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/cafe_hle_stub.h"

namespace cafe::nn::boss
{

virt_ptr<hle::VirtualTable> NetTaskSetting::VirtualTable = nullptr;
virt_ptr<hle::TypeDescriptor> NetTaskSetting::TypeDescriptor = nullptr;

NetTaskSetting::NetTaskSetting()
{
   mVirtualTable = NetTaskSetting::VirtualTable;
   mTaskSettingData.unk0x18C = 120u;
}

NetTaskSetting::~NetTaskSetting()
{
}

void
Library::registerNetTaskSettingSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn4boss14NetTaskSettingFv",
                             NetTaskSetting);
   RegisterDestructorExport("__dt__Q3_2nn4boss14NetTaskSettingFv",
                            NetTaskSetting);

   registerTypeInfo<TaskSetting>(
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

}  // namespace cafe::nn::boss
