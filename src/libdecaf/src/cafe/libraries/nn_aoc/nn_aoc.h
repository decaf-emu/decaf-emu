#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_aoc
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_aoc, "nn_aoc.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerLibSymbols();
};

} // namespace cafe::nn_aoc
