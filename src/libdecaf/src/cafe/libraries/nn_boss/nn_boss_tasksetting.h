#pragma once
#include "nn_boss_titleid.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"
#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

#pragma pack(push, 1)

using TaskPriority = uint8_t;

struct TaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> unk0x00;
   UNKNOWN(4);
   be2_val<uint32_t> unk0x08;
   be2_val<uint32_t> unk0x0C;
   UNKNOWN(0x28 - 0x10);
   be2_val<uint16_t> unk0x28;
   be2_val<TaskPriority> priority;
   UNKNOWN(1);
   be2_val<uint8_t> permission;
   UNKNOWN(1);
   be2_val<uint16_t> intervalSecForShortSpanRetry;
   be2_val<uint32_t> intervalSec;
   UNKNOWN(4);
   be2_val<uint64_t> lifeTimeSec;
   UNKNOWN(0x18C - 0x40);
   be2_val<uint32_t> unk0x18C;
   UNKNOWN(0x1000 - 0x190);
   be2_virt_ptr<ghs::VirtualTable> virtualTable;
};
CHECK_OFFSET(TaskSetting, 0x0, unk0x00);
CHECK_OFFSET(TaskSetting, 0x8, unk0x08);
CHECK_OFFSET(TaskSetting, 0xC, unk0x0C);
CHECK_OFFSET(TaskSetting, 0x28, unk0x28);
CHECK_OFFSET(TaskSetting, 0x2A, priority);
CHECK_OFFSET(TaskSetting, 0x2C, permission);
CHECK_OFFSET(TaskSetting, 0x2E, intervalSecForShortSpanRetry);
CHECK_OFFSET(TaskSetting, 0x30, intervalSec);
CHECK_OFFSET(TaskSetting, 0x38, lifeTimeSec);
CHECK_OFFSET(TaskSetting, 0x18C, unk0x18C);
CHECK_OFFSET(TaskSetting, 0x1000, virtualTable);
CHECK_SIZE(TaskSetting, 0x1004);

#pragma pack(pop)

virt_ptr<TaskSetting>
TaskSetting_Constructor(virt_ptr<TaskSetting> self);

void
TaskSetting_Destructor(virt_ptr<TaskSetting> self,
                       ghs::DestructorFlags flags);

void
TaskSetting_InitializeSetting(virt_ptr<TaskSetting> self);

void
TaskSetting_SetRunPermissionInParentalControlRestriction(virt_ptr<TaskSetting> self,
                                                         bool value);

nn::Result
TaskSetting_RegisterPreprocess(virt_ptr<TaskSetting> self,
                               uint32_t a1,
                               virt_ptr<TitleID> a2,
                               virt_ptr<const char> a3);

void
TaskSetting_RegisterPostprocess(virt_ptr<TaskSetting> self,
                                uint32_t a1,
                                virt_ptr<TitleID> a2,
                                virt_ptr<const char> a3,
                                virt_ptr<nn::Result> a4);

nn::Result
PrivateTaskSetting_SetIntervalSecForShortSpanRetry(virt_ptr<TaskSetting> self,
                                                   uint16_t sec);

nn::Result
PrivateTaskSetting_SetIntervalSec(virt_ptr<TaskSetting> self,
                                  uint32_t sec);

nn::Result
PrivateTaskSetting_SetLifeTimeSec(virt_ptr<TaskSetting> self,
                                  uint64_t lifeTimeSec);

nn::Result
PrivateTaskSetting_SetPermission(virt_ptr<TaskSetting> self,
                                 uint8_t permission);

nn::Result
PrivateTaskSetting_SetPriority(virt_ptr<TaskSetting> self,
                               TaskPriority priority);

}  // namespace namespace cafe::nn_boss
