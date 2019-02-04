#pragma once
#include "nn_boss_rawultasksetting.h"

#include "cafe/libraries/cafe_hle_library_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::PlayReportSetting::FindValueHead(char const *) const
nn::boss::PlayReportSetting::GetValue(char const *, unsigned int *) const
nn::boss::PlayReportSetting::GetValue(unsigned int, unsigned int *) const
nn::boss::PlayReportSetting::RegisterPreprocess(unsigned int, nn::boss::TitleID, char const *)
*/

namespace cafe::nn_boss
{

struct PlayReportSetting : public RawUlTaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> playReportUnk1;
   be2_val<uint32_t> playReportUnk2;
   be2_val<uint32_t> playReportUnk3;
   be2_val<uint32_t> playReportUnk4;
};
CHECK_OFFSET(PlayReportSetting, 0x1210, playReportUnk1);
CHECK_OFFSET(PlayReportSetting, 0x1214, playReportUnk2);
CHECK_OFFSET(PlayReportSetting, 0x1218, playReportUnk3);
CHECK_OFFSET(PlayReportSetting, 0x121C, playReportUnk4);
CHECK_SIZE(PlayReportSetting, 0x1220);

virt_ptr<PlayReportSetting>
PlayReportSetting_Constructor(virt_ptr<PlayReportSetting> self);

void
PlayReportSetting_Destructor(virt_ptr<PlayReportSetting> self,
                             ghs::DestructorFlags flags);

void
PlayReportSetting_Initialize(virt_ptr<PlayReportSetting> self,
                             virt_ptr<void> a1,
                             uint32_t a2);

nn::Result
PlayReportSetting_RegisterPreprocess(virt_ptr<PlayReportSetting> self,
                                     uint32_t a1,
                                     virt_ptr<TitleID> a2,
                                     virt_ptr<const char> a3);

bool
PlayReportSetting_Set(virt_ptr<PlayReportSetting> self,
                      virt_ptr<const char> key,
                      uint32_t value);

bool
PlayReportSetting_Set(virt_ptr<PlayReportSetting> self,
                      uint32_t key,
                      uint32_t value);

}  // namespace cafe::nn_boss
