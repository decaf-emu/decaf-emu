#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_nfp
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_nfp, "nn_nfp.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerLibSymbols();
};

} // namespace cafe::nn_nfp
