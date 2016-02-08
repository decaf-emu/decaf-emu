#pragma once
#include "nn_boss_nettasksetting.h"

/*
Unimplemented functions:
nn::boss::RawUlTaskSetting::AddLargeHttpHeader(char const *, char const *)
nn::boss::RawUlTaskSetting::ClearLargeHttpHeader(void)
nn::boss::RawUlTaskSetting::CopyUploadFileToBossStorage(unsigned int, nn::boss::TitleID, char const *) const
nn::boss::RawUlTaskSetting::Initialize(char const *, unsigned char const *, long long)
nn::boss::RawUlTaskSetting::Initialize(char const *, char const *)
nn::boss::RawUlTaskSetting::Initialize(char const *, char const *, unsigned char *, long long)
nn::boss::RawUlTaskSetting::RegisterPreprocess(unsigned int, nn::boss::TitleID, char const *)
nn::boss::RawUlTaskSetting::SetOption(unsigned int)
nn::boss::RawUlTaskSetting::SetRawUlTaskSettingToRecord(char const *)
*/

namespace nn
{

namespace boss
{

class RawUlTaskSetting : public NetTaskSetting
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   RawUlTaskSetting();
   ~RawUlTaskSetting();

   nn::Result
   RegisterPreprocess(uint32_t, nn::boss::TitleID *id, const char *);

   void
   RegisterPostprocess(uint32_t, nn::boss::TitleID *id, const char *, nn::Result *);

protected:
   uint32_t mRawUlUnk1;
   uint32_t mRawUlUnk2;
   uint32_t mRawUlUnk3;
   char mRawUlData[0x200];
};
CHECK_SIZE(RawUlTaskSetting, 0x1210);

}  // namespace boss

}  // namespace nn
