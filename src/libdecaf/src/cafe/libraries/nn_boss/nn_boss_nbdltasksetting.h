#pragma once
#include "nn_boss_nettasksetting.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

namespace cafe::nn_boss
{

struct NbdlTaskSetting : NetTaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(NbdlTaskSetting, 0x1004);

virt_ptr<NbdlTaskSetting>
NbdlTaskSetting_Constructor(virt_ptr<NbdlTaskSetting> self);

void
NbdlTaskSetting_Destructor(virt_ptr<NbdlTaskSetting> self,
                           ghs::DestructorFlags flags);

nn::Result
NbdlTaskSetting_Initialize(virt_ptr<NbdlTaskSetting> self,
                           virt_ptr<const char> bossCode,
                           int64_t a3,
                           virt_ptr<const char> directoryName);

nn::Result
NbdlTaskSetting_SetFileName(virt_ptr<NbdlTaskSetting> self,
                            virt_ptr<const char> fileName);

nn::Result
PrivateNbdlTaskSetting_SetCountryCode(virt_ptr<NbdlTaskSetting> self,
                                      virt_ptr<const char> countryCode);

nn::Result
PrivateNbdlTaskSetting_SetLanguageCode(virt_ptr<NbdlTaskSetting> self,
                                       virt_ptr<const char> languageCode);

nn::Result
PrivateNbdlTaskSetting_SetOption(virt_ptr<NbdlTaskSetting> self,
                                 uint8_t option);

} // namespace cafe::nn_boss