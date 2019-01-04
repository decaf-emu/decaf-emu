#pragma once
#include <libdecaf/decaf_softwarekeyboard.h>
#include <QObject>

class QInputDialog;
class QWidget;

class SoftwareKeyboardDriver : public QObject, public decaf::SoftwareKeyboardDriver
{
   Q_OBJECT

public:
   SoftwareKeyboardDriver(QObject *parent);

   void acceptInput(QString text);
   void rejectInput();

signals:
   void open(QString text);
   void close();
   void inputStringChanged(QString text);

private:
   void onOpen(std::u16string defaultText) override;
   void onClose() override;
   void onInputStringChanged(std::u16string text) override;
};
