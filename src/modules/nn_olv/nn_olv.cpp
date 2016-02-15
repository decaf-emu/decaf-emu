#include "nn_olv.h"

namespace nn
{

namespace olv
{

void
Module::initialise()
{
   initialiseUploadedDataBase();
   initialiseUploadedPostData();
}

void
Module::RegisterFunctions()
{
   registerDownloadedCommunityData();
   registerDownloadedTopicData();
   registerUploadedDataBase();
   registerUploadedPostData();
}

} // namespace olv

} // namespace nn
