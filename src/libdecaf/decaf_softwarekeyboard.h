#pragma once
#include <string>
#include <string_view>

namespace decaf
{

class SoftwareKeyboardDriver
{
public:
   void accept();
   void reject();
   void setInputString(std::u16string_view text);

   virtual void onOpen(std::u16string defaultText) { };
   virtual void onClose() { };
   virtual void onInputStringChanged(std::u16string text) { };
};

void setSoftwareKeyboardDriver(SoftwareKeyboardDriver *driver);
SoftwareKeyboardDriver *softwareKeyboardDriver();

} // namespace decaf
