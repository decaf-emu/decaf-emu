#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_sl
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_sl, "nn_sl.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nn_sl
