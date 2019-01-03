#pragma once
#include "ui_about.h"

#include <QDialog>

class AboutDialog : public QDialog
{
public:
   AboutDialog(QWidget *parent = nullptr) :
      QDialog(parent)
   {
      mUi.setupUi(this);
      mUi.iconWidget->load(QString(":/images/logo"));
   }

private:
   Ui::AboutDialog mUi;
};
