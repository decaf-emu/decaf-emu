#pragma once
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/cafe_hle_library_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

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

namespace cafe::nn_boss
{

class RawUlTaskSetting : public NetTaskSetting
{
public:
   static virt_ptr<hle::VirtualTable> VirtualTable;
   static virt_ptr<hle::TypeDescriptor> TypeDescriptor;

public:
   RawUlTaskSetting();
   ~RawUlTaskSetting();

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
   be2_val<uint32_t> mRawUlUnk1;
   be2_val<uint32_t> mRawUlUnk2;
   be2_val<uint32_t> mRawUlUnk3;
   be2_array<char, 0x200> mRawUlData;

protected:
   CHECK_MEMBER_OFFSET_BEG
   CHECK_OFFSET(RawUlTaskSetting, 0x1004, mRawUlUnk1);
   CHECK_OFFSET(RawUlTaskSetting, 0x1008, mRawUlUnk2);
   CHECK_OFFSET(RawUlTaskSetting, 0x100C, mRawUlUnk3);
   CHECK_OFFSET(RawUlTaskSetting, 0x1010, mRawUlData);
   CHECK_MEMBER_OFFSET_END
};
CHECK_SIZE(RawUlTaskSetting, 0x1210);

}  // namespace cafe::nn_boss
