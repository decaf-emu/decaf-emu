#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_ccr
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_ccr, "nn_ccr.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nn_ccr
