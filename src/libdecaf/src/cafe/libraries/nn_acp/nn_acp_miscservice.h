#pragma once
#include "nn_acp_acpresult.h"
#include "nn/acp/nn_acp_miscservice.h"

#include <libcpu/be2_struct.h>

namespace cafe::coreinit
{
struct OSCalendarTime;
} // namespace cafe::coreinit

namespace cafe::nn_acp
{

using ACPMetaXml = nn::acp::ACPMetaXml;
using ACPTitleId = nn::acp::ACPTitleId;

ACPResult
ACPGetNetworkTime(virt_ptr<int64_t> outTime,
                  virt_ptr<uint32_t> outUnknown);

void
ACPConvertNetworkTimeToOSCalendarTime(int64_t networkTime,
                                      virt_ptr<coreinit::OSCalendarTime> calendarTime);

ACPResult
ACPGetTitleIdOfMainApplication(virt_ptr<ACPTitleId> outTitleId);

ACPResult
ACPGetTitleMetaXml(ACPTitleId titleId,
                   virt_ptr<ACPMetaXml> outData);

}  // namespace cafe::nn_acp
