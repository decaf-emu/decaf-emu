#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn::nim
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_nim, "nn_nim.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nn::nim
