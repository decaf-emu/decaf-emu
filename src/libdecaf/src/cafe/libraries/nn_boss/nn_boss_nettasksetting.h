#pragma once
#include "nn_boss_tasksetting.h"

#include "cafe/libraries/ghs/cafe_ghs_typeinfo.h"
#include "nn/nn_result.h"

#include <libcpu/be2_struct.h>

/*
Unimplemented functions:
nn::boss::NetTaskSetting::AddCaCert(char const *)
nn::boss::NetTaskSetting::AddHttpHeader(char const *, char const *)
nn::boss::NetTaskSetting::AddHttpQueryString(char const *, nn::boss::QueryKind)
nn::boss::NetTaskSetting::AddInternalCaCert(signed char)
nn::boss::NetTaskSetting::ClearCaCertSetting(void)
nn::boss::NetTaskSetting::ClearClientCertSetting(void)
nn::boss::NetTaskSetting::ClearHttpHeaders(void)
nn::boss::NetTaskSetting::ClearHttpQueryStrings(void)
nn::boss::NetTaskSetting::SetClientCert(char const *, char const *)
nn::boss::NetTaskSetting::SetConnectionSetting(nn::boss::HttpProtocol, char const *, unsigned short)
nn::boss::NetTaskSetting::SetFirstLastModifiedTime(char const *)
nn::boss::NetTaskSetting::SetHttpOption(unsigned short)
nn::boss::NetTaskSetting::SetHttpTimeout(unsigned int)
nn::boss::NetTaskSetting::SetInternalClientCert(signed char)
nn::boss::NetTaskSetting::SetServiceToken(unsigned char const *)
*/

namespace cafe::nn_boss
{

struct NetTaskSetting : public TaskSetting
{
   static virt_ptr<ghs::VirtualTable> VirtualTable;
   static virt_ptr<ghs::TypeDescriptor> TypeDescriptor;
};
CHECK_SIZE(NetTaskSetting, 0x1004);

virt_ptr<NetTaskSetting>
NetTaskSetting_Constructor(virt_ptr<NetTaskSetting> self);

void
NetTaskSetting_Destructor(virt_ptr<NetTaskSetting> self,
                          ghs::DestructorFlags flags);

}  // namespace cafe::nn_boss
