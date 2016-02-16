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
   registerInitFunctions();
   registerDownloadedCommunityData();
   registerDownloadedTopicData();
   registerUploadedDataBase();
   registerUploadedPostData();
}

} // namespace olv

} // namespace nn
