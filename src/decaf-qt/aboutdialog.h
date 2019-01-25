#pragma once
#include "ui_about.h"

#include <QDialog>
#include <decaf_buildinfo.h>

class AboutDialog : public QDialog
{
public:
   AboutDialog(QWidget *parent = nullptr) :
      QDialog(parent)
   {
      mUi.setupUi(this);
      mUi.iconWidget->load(QString(":/images/logo"));
      mUi.labelBuildInfo->setText(
         mUi.labelBuildInfo->text().arg(BUILD_FULLNAME, GIT_BRANCH, GIT_DESC,
                                        QString(BUILD_DATE).left(10)));
   }

private:
   Ui::AboutDialog mUi;
};
