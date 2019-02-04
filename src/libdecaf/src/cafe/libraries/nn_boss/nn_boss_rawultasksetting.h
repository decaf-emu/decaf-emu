#pragma once
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
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

struct RawUlTaskSetting : public NetTaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;

   be2_val<uint32_t> rawUlUnk1;
   be2_val<uint32_t> rawUlUnk2;
   be2_val<uint32_t> rawUlUnk3;
   be2_array<char, 0x200> rawUlData;
};
CHECK_SIZE(RawUlTaskSetting, 0x1210);

virt_ptr<RawUlTaskSetting>
RawUlTaskSetting_Constructor(virt_ptr<RawUlTaskSetting> self);

void
RawUlTaskSetting_Destructor(virt_ptr<RawUlTaskSetting> self,
                            ghs::DestructorFlags flags);

nn::Result
RawUlTaskSetting_RegisterPreprocess(virt_ptr<RawUlTaskSetting> self,
                                    uint32_t a1,
                                    virt_ptr<TitleID> a2,
                                    virt_ptr<const char> a3);

void
RawUlTaskSetting_RegisterPostprocess(virt_ptr<RawUlTaskSetting> self,
                                     uint32_t a1,
                                     virt_ptr<TitleID> a2,
                                     virt_ptr<const char> a3,
                                     virt_ptr<nn::Result> a4);

}  // namespace cafe::nn_boss
