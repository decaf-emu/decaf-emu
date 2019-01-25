#pragma once
#include "ui_about.h"

#include <QDialog>
#include <common/scm_rev.h>

class AboutDialog : public QDialog
{
public:
   AboutDialog(QWidget *parent = nullptr) :
      QDialog(parent)
   {
      mUi.setupUi(this);
      mUi.iconWidget->load(QString(":/images/logo"));
      mUi.labelBuildInfo->setText(
         mUi.labelBuildInfo->text().arg(Common::g_build_fullname, Common::g_scm_branch,
               Common::g_scm_desc, QString(Common::g_build_date).left(10)));
   }

private:
   Ui::AboutDialog mUi;
};
