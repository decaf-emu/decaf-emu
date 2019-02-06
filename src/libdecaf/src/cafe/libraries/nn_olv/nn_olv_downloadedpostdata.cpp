#include "nn_olv.h"
#include "nn_olv_downloadeddatabase.h"
#include "nn_olv_downloadedpostdata.h"

#include "cafe/libraries/ghs/cafe_ghs_malloc.h"

namespace cafe::nn_olv
{

virt_ptr<ghs::VirtualTable> DownloadedPostData::VirtualTable = nullptr;
virt_ptr<ghs::TypeDescriptor> DownloadedPostData::TypeDescriptor = nullptr;

virt_ptr<DownloadedPostData>
DownloadedPostData_Constructor(virt_ptr<DownloadedPostData> self)
{
   if (!self) {
      self = virt_cast<DownloadedPostData *>(ghs::malloc(sizeof(DownloadedPostData)));
      if (!self) {
         return nullptr;
      }
   }

   DownloadedDataBase_Constructor(virt_cast<DownloadedDataBase *>(self));
   self->virtualTable = DownloadedPostData::VirtualTable;
   self->communityId = 0u;
   self->empathyCount = 0u;
   self->commentCount = 0u;
   return self;
}

void
DownloadedPostData_Destructor(virt_ptr<DownloadedPostData> self,
                              ghs::DestructorFlags flags)
{
   if (!self) {
      return;
   }

   if (flags & ghs::DestructorFlags::FreeMemory) {
      ghs::free(self);
   }
}

uint32_t
DownloadedPostData_GetCommentCount(virt_ptr<const DownloadedPostData> self)
{
   return self->commentCount;
}

uint32_t
DownloadedPostData_GetCommunityId(virt_ptr<const DownloadedPostData> self)
{
   return self->communityId;
}

uint32_t
DownloadedPostData_GetEmpathyCount(virt_ptr<const DownloadedPostData> self)
{
   return self->empathyCount;
}

virt_ptr<const uint8_t>
DownloadedPostData_GetPostId(virt_ptr<const DownloadedPostData> self)
{
   return DownloadedDataBase_GetPostId(virt_cast<const DownloadedDataBase *>(self));
}

void
Library::registerDownloadedPostDataSymbols()
{
   RegisterFunctionExportName("__ct__Q3_2nn3olv18DownloadedPostDataFv",
                              DownloadedPostData_Constructor);
   RegisterFunctionExportName("__dt__Q3_2nn3olv18DownloadedPostDataFv",
                              DownloadedPostData_Destructor);
   RegisterFunctionExportName("GetCommentCount__Q3_2nn3olv18DownloadedPostDataCFv",
                              DownloadedPostData_GetCommentCount);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv18DownloadedPostDataCFv",
                              DownloadedPostData_GetCommunityId);
   RegisterFunctionExportName("GetEmpathyCount__Q3_2nn3olv18DownloadedPostDataCFv",
                              DownloadedPostData_GetEmpathyCount);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv18DownloadedPostDataCFv",
                              DownloadedPostData_GetPostId);

   registerTypeInfo<DownloadedPostData>(
      "nn::olv::DownloadedPostData",
      {
         "__dt__Q3_2nn3olv18DownloadedPostDataFv",
      });
}

}  // namespace cafe::nn_olv
