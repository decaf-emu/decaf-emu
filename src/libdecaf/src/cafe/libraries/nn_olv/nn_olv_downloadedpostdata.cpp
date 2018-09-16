#include "nn_olv.h"
#include "nn_olv_downloadeddatabase.h"
#include "nn_olv_downloadedpostdata.h"
#include "nn_olv_result.h"

namespace cafe::nn::olv
{

virt_ptr<hle::VirtualTable>
DownloadedPostData::VirtualTable = nullptr;

virt_ptr<hle::TypeDescriptor>
DownloadedPostData::TypeDescriptor = nullptr;

DownloadedPostData::DownloadedPostData()
{
   mVirtualTable = VirtualTable;
   mCommunityId = 0u;
   mEmpathyCount = 0u;
   mCommentCount = 0u;
}

DownloadedPostData::~DownloadedPostData()
{
}

uint32_t
DownloadedPostData::GetCommentCount() const
{
   return mCommentCount;
}

uint32_t
DownloadedPostData::GetCommunityId() const
{
   return mCommunityId;
}

uint32_t
DownloadedPostData::GetEmpathyCount() const
{
   return mEmpathyCount;
}

virt_ptr<const uint8_t>
DownloadedPostData::GetPostId() const
{
   return DownloadedDataBase::GetPostId();
}

void
Library::registerDownloadedPostDataSymbols()
{
   RegisterConstructorExport("__ct__Q3_2nn3olv18DownloadedPostDataFv",
                             DownloadedPostData);
   RegisterDestructorExport("__dt__Q3_2nn3olv18DownloadedPostDataFv",
                            DownloadedPostData);
   RegisterFunctionExportName("GetCommentCount__Q3_2nn3olv18DownloadedPostDataCFv",
                              &DownloadedPostData::GetCommentCount);
   RegisterFunctionExportName("GetCommunityId__Q3_2nn3olv18DownloadedPostDataCFv",
                              &DownloadedPostData::GetCommunityId);
   RegisterFunctionExportName("GetEmpathyCount__Q3_2nn3olv18DownloadedPostDataCFv",
                              &DownloadedPostData::GetEmpathyCount);
   RegisterFunctionExportName("GetPostId__Q3_2nn3olv18DownloadedPostDataCFv",
                              &DownloadedPostData::GetPostId);

   registerTypeInfo<DownloadedPostData>(
      "nn::olv::DownloadedPostData",
      {
         "__dt__Q3_2nn3olv18DownloadedPostDataFv",
      });
}

}  // namespace cafe::nn::olv
