#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_temp
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::nn_temp, "nn_temp.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerTempDirSymbols();
};

} // namespace cafe::nn_temp
