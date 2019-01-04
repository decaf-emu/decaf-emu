#include "decaf_softwarekeyboard.h"

#include "cafe/libraries/swkbd/swkbd_keyboard.h"

namespace decaf
{

static SoftwareKeyboardDriver *sSoftwareKeyboardDriver = nullptr;

void
setSoftwareKeyboardDriver(SoftwareKeyboardDriver *driver)
{
   sSoftwareKeyboardDriver = driver;
}

SoftwareKeyboardDriver *
softwareKeyboardDriver()
{
   return sSoftwareKeyboardDriver;
}

void
SoftwareKeyboardDriver::accept()
{
   cafe::swkbd::internal::inputAccept();
}

void
SoftwareKeyboardDriver::reject()
{
   cafe::swkbd::internal::inputReject();
}

void
SoftwareKeyboardDriver::setInputString(std::u16string_view text)
{
   cafe::swkbd::internal::setInputString(text);
}

} // namespace decaf
