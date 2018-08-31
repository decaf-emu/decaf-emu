#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nio_prof
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nio_prof, "nio_prof.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nio_prof
