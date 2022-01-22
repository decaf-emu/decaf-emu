#pragma once
#include <string>

namespace decaf
{

class ErrEulaDriver
{
public:
   void buttonClicked();
   void button1Clicked();
   void button2Clicked();

   virtual void onOpenErrorCode(int32_t errorCode) { };
   virtual void onOpenErrorMessage(std::u16string message,
                                   std::u16string button1 = {},
                                   std::u16string button2 = {}) { };
   virtual void onClose() { };
};

void setErrEulaDriver(ErrEulaDriver *driver);
ErrEulaDriver *errEulaDriver();

} // namespace decaf
