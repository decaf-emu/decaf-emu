#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn::ndm
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_ndm, "nn_ndm.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerNdmSymbols();
};

} // namespace cafe::nn::ndm
