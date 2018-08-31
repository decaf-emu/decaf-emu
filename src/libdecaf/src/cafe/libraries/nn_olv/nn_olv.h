#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn::olv
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
   void registerDownloadedTopicDataSymbols();
   void registerInitSymbols();
   void registerUploadedDataBaseSymbols();
   void registerUploadedPostDataSymbols();
};

} // namespace cafe::nn::olv
