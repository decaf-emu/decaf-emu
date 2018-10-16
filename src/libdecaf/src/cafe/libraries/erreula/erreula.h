#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::nn_erreula
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::erreula, "erreula.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerErrorViewerSymbols();
};

} // namespace cafe::nn_erreula
