#pragma once
#include "nn_boss_rawultasksetting.h"

/*
Unimplemented functions:
nn::boss::PlayReportSetting::FindValueHead(char const *) const
nn::boss::PlayReportSetting::GetValue(char const *, unsigned int *) const
nn::boss::PlayReportSetting::GetValue(unsigned int, unsigned int *) const
nn::boss::PlayReportSetting::RegisterPreprocess(unsigned int, nn::boss::TitleID, char const *)
*/

namespace nn
{

namespace boss
{

class PlayReportSetting : public RawUlTaskSetting
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   PlayReportSetting();
   ~PlayReportSetting();

   nn::Result
   RegisterPreprocess(uint32_t, nn::boss::TitleID *id, const char *);

   void
   Initialize(void *, uint32_t);

   bool
   Set(const char *key, uint32_t value);

   bool
   Set(uint32_t key, uint32_t value);

protected:
   uint32_t mPlayReportUnk1;
   uint32_t mPlayReportUnk2;
   uint32_t mPlayReportUnk3;
   uint32_t mPlayReportUnk4;

protected:
   CHECK_MEMBER_OFFSET_START
   CHECK_OFFSET(PlayReportSetting, 0x1210, mPlayReportUnk1);
   CHECK_OFFSET(PlayReportSetting, 0x1214, mPlayReportUnk2);
   CHECK_OFFSET(PlayReportSetting, 0x1218, mPlayReportUnk3);
   CHECK_OFFSET(PlayReportSetting, 0x121C, mPlayReportUnk4);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(PlayReportSetting, 0x1220);

}  // namespace boss

}  // namespace nn
