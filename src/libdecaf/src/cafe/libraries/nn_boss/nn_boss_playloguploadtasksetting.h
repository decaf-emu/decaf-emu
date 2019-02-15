#pragma once
#include "nn_boss_rawultasksetting.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct PlayLogUploadTaskSetting : public RawUlTaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(PlayLogUploadTaskSetting, 0x1210);

virt_ptr<PlayLogUploadTaskSetting>
PlayLogUploadTaskSetting_Constructor(virt_ptr<PlayLogUploadTaskSetting> self);

void
PlayLogUploadTaskSetting_Destructor(virt_ptr<PlayLogUploadTaskSetting> self,
                                    ghs::DestructorFlags flags);

nn::Result
PlayLogUploadTaskSetting_Initialize(virt_ptr<PlayLogUploadTaskSetting> self);

nn::Result
PlayLogUploadTaskSetting_RegisterPreprocess(virt_ptr<PlayLogUploadTaskSetting> self,
                                            uint32_t a1,
                                            virt_ptr<TitleID> a2,
                                            virt_ptr<const char> a3);

void
PlayLogUploadTaskSetting_SetPlayLogUploadTaskSettingToRecord(virt_ptr<PlayLogUploadTaskSetting> self);

}  // namespace cafe::nn_boss
