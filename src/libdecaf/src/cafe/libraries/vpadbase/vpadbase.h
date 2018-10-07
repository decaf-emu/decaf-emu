#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::vpadbase
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::vpadbase, "vpadbase.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerControllerSymbols();
};

} // namespace cafe::vpadbase
