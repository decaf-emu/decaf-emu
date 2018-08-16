#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::avm
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::avm, "avm.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::avm
