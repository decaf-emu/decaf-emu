#pragma once
#include "cafe/libraries/cafe_hle_library.h"

namespace cafe::proc_ui
{

class Library : public hle::Library
{
public:
   Library() :
      hle::Library(hle::LibraryId::proc_ui, "proc_ui.rpl")
   {
   }

protected:
   virtual void registerSymbols() override;

private:
   void registerMessagesFunctions();
};

} // namespace cafe::proc_ui
