#include "softwarekeyboarddriver.h"

SoftwareKeyboardDriver::SoftwareKeyboardDriver(QObject *parent) :
   QObject(parent)
{
}

void
SoftwareKeyboardDriver::acceptInput(QString text)
{
   decaf::SoftwareKeyboardDriver::setInputString(text.toStdU16String());
   decaf::SoftwareKeyboardDriver::accept();
}

void
SoftwareKeyboardDriver::rejectInput()
{
   decaf::SoftwareKeyboardDriver::reject();
}

void
SoftwareKeyboardDriver::onOpen(std::u16string defaultText)
{
   emit open(QString::fromStdU16String(defaultText));
}

void
SoftwareKeyboardDriver::onClose()
{
   emit close();
}

void
SoftwareKeyboardDriver::onInputStringChanged(std::u16string text)
{
   emit inputStringChanged(QString::fromStdU16String(text));
}
