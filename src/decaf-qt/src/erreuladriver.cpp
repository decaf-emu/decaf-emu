#include "erreuladriver.h"

ErrEulaDriver::ErrEulaDriver(QObject *parent) :
   QObject(parent)
{
}

void
ErrEulaDriver::onOpenErrorCode(int32_t errorCode)
{
   emit openWithErrorCode(errorCode);
}

void
ErrEulaDriver::onOpenErrorMessage(std::u16string message,
                                  std::u16string button1,
                                  std::u16string button2)
{
   emit openWithMessage(QString::fromStdU16String(message),
                        QString::fromStdU16String(button1),
                        QString::fromStdU16String(button2));
}

void
ErrEulaDriver::onClose()
{
   emit close();
}
