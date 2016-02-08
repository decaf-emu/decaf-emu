#pragma once
#include "modules/nn_result.h"
#include "modules/coreinit/coreinit_ghs_typeinfo.h"
#include "nn_boss_titleid.h"
#include "types.h"
#include "utils/structsize.h"

namespace nn
{

namespace boss
{

class TaskSetting
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   TaskSetting();
   ~TaskSetting();

   void
   InitializeSetting();

   void
   SetRunPermissionInParentalControlRestriction(bool value);

   nn::Result
   RegisterPreprocess(uint32_t, nn::boss::TitleID *id, const char *);

   void
   RegisterPostprocess(uint32_t, nn::boss::TitleID *id, const char *, nn::Result *);

protected:
   char mTaskSettingData[0x1000];
   be_ptr<ghs::VirtualTableEntry> mVirtualTable;
};
CHECK_SIZE(TaskSetting, 0x1004);

}  // namespace boss

}  // namespace nn
