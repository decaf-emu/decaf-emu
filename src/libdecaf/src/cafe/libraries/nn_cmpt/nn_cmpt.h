#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_cmpt
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_cmpt, "nn_cmpt.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerLibSymbols();
};

} // namespace cafe::nn_cmpt
