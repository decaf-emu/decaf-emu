#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_nets2
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_nets2, "nn_nets2.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
};

} // namespace cafe::nn_nets2
