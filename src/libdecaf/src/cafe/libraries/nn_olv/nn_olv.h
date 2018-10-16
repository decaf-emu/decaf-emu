#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_olv
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_olv, "nn_olv.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerDownloadedCommunityDataSymbols();
   void registerDownloadedDataBaseSymbols();
   void registerDownloadedPostDataSymbols();
   void registerDownloadedTopicDataSymbols();
   void registerInitSymbols();
   void registerInitializeParamSymbols();
   void registerUploadedDataBaseSymbols();
   void registerUploadedPostDataSymbols();
};

} // namespace cafe::nn_olv
