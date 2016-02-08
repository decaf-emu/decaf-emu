#pragma once
#include "nn_boss_tasksetting.h"

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

namespace nn
{

namespace boss
{

class NetTaskSetting : public TaskSetting
{
public:
   static ghs::VirtualTableEntry *VirtualTable;
   static ghs::TypeDescriptor *TypeInfo;

public:
   NetTaskSetting();
   ~NetTaskSetting();

protected:
};
CHECK_SIZE(NetTaskSetting, 0x1004);

}  // namespace boss

}  // namespace nn
