#include "decaf_erreula.h"

#include "cafe/libraries/erreula/erreula_errorviewer.h"

namespace decaf
{

static ErrEulaDriver *sErrEulaDriver = nullptr;

void
setErrEulaDriver(ErrEulaDriver *driver)
{
   sErrEulaDriver = driver;
}

ErrEulaDriver *
errEulaDriver()
{
   return sErrEulaDriver;
}

void
ErrEulaDriver::buttonClicked()
{
   cafe::nn_erreula::internal::buttonClicked();
}

void
ErrEulaDriver::button1Clicked()
{
   cafe::nn_erreula::internal::button1Clicked();
}

void
ErrEulaDriver::button2Clicked()
{
   cafe::nn_erreula::internal::button2Clicked();
}

} // namespace decaf
