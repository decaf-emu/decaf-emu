#include "nn_olv.h"
#include "nn_olv_downloadedtopicdata.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_olv
{

virt_ptr<DownloadedTopicData>
DownloadedTopicData_Constructor(virt_ptr<DownloadedTopicData> self)
{
   if (!self) {
      self = virt_cast<DownloadedTopicData *>(ghs::malloc(sizeof(DownloadedTopicData)));
      if (!self) {
         return nullptr;
      }
   }

   self->unk1 = 0u;
   self->communityId = 0u;
   return self;
}

uint32_t
DownloadedTopicData_GetCommunityId(virt_ptr<const DownloadedTopicData> self)
{
   return self->communityId;
}

uint32_t
DownloadedTopicData_GetUserCount(virt_ptr<const DownloadedTopicData> self)
{
   return 0;
}

void
Library::registerDownloadedTopicDataSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv19DownloadedTopicDataFv",
                              DownloadedTopicData_Constructor);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv19DownloadedTopicDataCFv",
                              DownloadedTopicData_GetCommunityId);
   RegisterFunctionExportName("GetUserCount__Q3_2nn3olv19DownloadedTopicDataCFv",
                              DownloadedTopicData_GetUserCount);
}

}  // namespace namespace cafe::nn_olv
