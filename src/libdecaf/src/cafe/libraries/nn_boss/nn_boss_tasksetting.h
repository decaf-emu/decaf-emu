#pragma once
#include "nn_boss_titleid.h"

#include "cafe/libraries/nn_result.h"
#include "cafe/libraries/cafe_hle_library_typeinfo.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn::boss
{

class TaskSetting
{
   struct TaskSettingData
   {
      be2_val<uint32_t> unk0x00;
      UNKNOWN(4);
      be2_val<uint32_t> unk0x08;
      be2_val<uint32_t> unk0x0C;
      UNKNOWN(0x2A - 0x10);
      be2_val<uint8_t> unk0x2A;
      UNKNOWN(1);
      be2_val<uint8_t> unk0x2C;
      UNKNOWN(3);
      be2_val<uint32_t> unk0x30;
      UNKNOWN(4);
      be2_val<uint32_t> unk0x38;
      be2_val<uint32_t> unk0x3C;
      UNKNOWN(0x18C - 0x40);
      be2_val<uint32_t> unk0x18C;
      UNKNOWN(0x1000 - 0x190);
   };

public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   TaskSetting();
   ~TaskSetting();

   void
   InitializeSetting();

   void
   SetRunPermissionInParentalControlRestriction(bool value);

   nn::Result
   RegisterPreprocess(uint32_t a1,
                      virt_ptr<TitleID> a2,
                      virt_ptr<const char> a3);

   void
   RegisterPostprocess(uint32_t a1,
                       virt_ptr<TitleID> a2,
                       virt_ptr<const char> a3,
                       virt_ptr<nn::Result> a4);

protected:
   be2_struct<TaskSettingData> mTaskSettingData;
   be2_virt_ptr<hle::VirtualTable> mVirtualTable;

protected:
   CHECK_MEMBER_OFFSET_BEG
      CHECK_OFFSET(TaskSetting, 0x00, mTaskSettingData);
      CHECK_OFFSET(TaskSetting, 0x1000, mVirtualTable);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(TaskSetting, 0x1004);

}  // namespace namespace cafe::nn::boss
