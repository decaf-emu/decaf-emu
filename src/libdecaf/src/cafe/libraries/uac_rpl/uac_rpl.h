#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::uac_rpl
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::uac_rpl, "uac_rpl.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::uac_rpl
