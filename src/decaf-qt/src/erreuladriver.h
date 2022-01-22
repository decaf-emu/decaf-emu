#pragma once
#include <libdecaf/decaf_erreula.h>
#include <QObject>

class QInputDialog;
class QWidget;

class ErrEulaDriver : public QObject, public decaf::ErrEulaDriver
{
   Q_OBJECT

public:
   ErrEulaDriver(QObject *parent);

signals:
   void openWithErrorCode(int32_t errorCode);
   void openWithMessage(QString message, QString button1, QString button2);
   void close();

private:
   void onOpenErrorCode(int32_t errorCode) override;
   void onOpenErrorMessage(std::u16string message,
                           std::u16string button1,
                           std::u16string button2) override;
   void onClose() override;
};
