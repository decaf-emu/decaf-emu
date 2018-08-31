#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::uvc
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::uvc, "uvc.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::uvc
