#pragma once
#include <QColorDialog>
#include <QEvent>
#include <QLineEdit>

class ColourLineEdit : public QLineEdit
{
public:
   ColourLineEdit(QWidget *parent = nullptr) :
      QLineEdit(parent)
   {
      installEventFilter(this);
      setReadOnly(true);
      setColour(QColor { Qt::black });
   }

   bool eventFilter(QObject*, QEvent* ev)
   {
      if (ev->type() == QEvent::MouseButtonPress) {
         auto color = QColorDialog::getColor(mColour, this, tr("Select color"));

         if (color.isValid()) {
            setColour(color);
         }

         return false;
      }

      return false;
   }

   void setColour(const QColor &colour)
   {
      mColour = colour;

      auto palette = QPalette { };
      palette.setColor(QPalette::Base, mColour);
      palette.setColor(QPalette::Text, getTextColor());
      setPalette(palette);
      setText(mColour.name());
   }

   QColor getColour() const
   {
      return mColour;
   }

private:
   QColor getTextColor()
   {
      double luminance = 1 - (0.299 * mColour.red() + 0.587 * mColour.green() + 0.114 * mColour.blue()) / 255;

      if (luminance < 0.5) {
         return QColor { Qt::black };
      } else {
         return QColor { Qt::white };
      }
   }

private:
   QColor mColour;
};
