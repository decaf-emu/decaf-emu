#include "nn_boss.h"
#include "nn_boss_nettasksetting.h"

namespace nn
{

namespace boss
{

ghs::VirtualTableEntry *
NetTaskSetting::VirtualTable = nullptr;

ghs::TypeDescriptor *
NetTaskSetting::TypeInfo = nullptr;

NetTaskSetting::NetTaskSetting()
{
   mVirtualTable = NetTaskSetting::VirtualTable;
}

NetTaskSetting::~NetTaskSetting()
{
}

void Module::registerNetTaskSettingFunctions()
{
   RegisterKernelFunctionConstructor("__ct__Q3_2nn4boss14NetTaskSettingFv", NetTaskSetting);
   RegisterKernelFunctionDestructor("__dt__Q3_2nn4boss14NetTaskSettingFv", NetTaskSetting);
}

void Module::initialiseNetTaskSetting()
{
   NetTaskSetting::TypeInfo = ghs::internal::makeTypeDescriptor("nn::boss::NetTaskSetting", {
      { TaskSetting::TypeInfo, 0x1600 },
   });

   NetTaskSetting::VirtualTable = ghs::internal::makeVirtualTable({
      { 0, NetTaskSetting::TypeInfo },
      { 0, findExportAddress("__dt__Q3_2nn4boss14NetTaskSettingFv") },
      TaskSetting::VirtualTable[2],
      TaskSetting::VirtualTable[3],
   });
}

}  // namespace boss

}  // namespace nn
