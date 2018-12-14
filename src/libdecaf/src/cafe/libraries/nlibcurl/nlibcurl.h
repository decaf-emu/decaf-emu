#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nlibcurl
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nlibcurl, "nlibcurl.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerCurlSymbols();
   void registerEasySymbols();
};

} // namespace cafe::nlibcurl
