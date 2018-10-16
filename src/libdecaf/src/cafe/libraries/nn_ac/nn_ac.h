#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_ac
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_ac, "nn_ac.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerCApiFunctions();
   void registerLibFunctions();
};

} // namespace cafe::nn_ac
