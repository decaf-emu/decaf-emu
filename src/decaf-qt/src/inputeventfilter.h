#pragma once
#include <QObject>
#include <QEvent>
#include <QKeyEvent>

class InputEventFilter : public QObject
{
   Q_OBJECT

public:
   InputEventFilter(QObject *parent) :
      QObject(parent)
   {
   }

   bool enabled()
   {
      return mEnabled;
   }

   void enable()
   {
      mEnabled = true;
   }

   void disable()
   {
      mEnabled = false;
   }

signals:
   void caughtKeyPress(int key);

protected:
   bool eventFilter(QObject *obj, QEvent *event)
   {
      if (mEnabled) {
         if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            emit caughtKeyPress(keyEvent->key());
            return true;
         }
      }

      return QObject::eventFilter(obj, event);
   }

private:
   bool mEnabled = false;
};
