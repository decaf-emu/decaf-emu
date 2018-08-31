#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::uac
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::uac, "uac.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::uac
