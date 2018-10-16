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

class PlayReportSetting : public RawUlTaskSetting
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   PlayReportSetting();
   ~PlayReportSetting();

   void
   Initialize(virt_ptr<void> a1,
              uint32_t a2);

   nn::Result
   RegisterPreprocess(uint32_t a1,
                      virt_ptr<TitleID> a2,
                      virt_ptr<const char> a3);

   bool
   Set(virt_ptr<const char> key,
       uint32_t value);

   bool
   Set(uint32_t key,
       uint32_t value);

protected:
   be2_val<uint32_t> mPlayReportUnk1;
   be2_val<uint32_t> mPlayReportUnk2;
   be2_val<uint32_t> mPlayReportUnk3;
   be2_val<uint32_t> mPlayReportUnk4;

protected:
   CHECK_MEMBER_OFFSET_BEG
      CHECK_OFFSET(PlayReportSetting, 0x1210, mPlayReportUnk1);
      CHECK_OFFSET(PlayReportSetting, 0x1214, mPlayReportUnk2);
      CHECK_OFFSET(PlayReportSetting, 0x1218, mPlayReportUnk3);
      CHECK_OFFSET(PlayReportSetting, 0x121C, mPlayReportUnk4);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(PlayReportSetting, 0x1220);

}  // namespace cafe::nn_boss
