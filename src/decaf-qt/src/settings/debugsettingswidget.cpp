#include "debugsettingswidget.h"

#include <QIntValidator>

DebugSettingsWidget::DebugSettingsWidget(QWidget *parent,
                                         Qt::WindowFlags f) :
   SettingsWidget(parent, f)
{
   mUi.setupUi(this);

   mUi.lineEditGdbServerPort->setValidator(new QIntValidator(0, 65535));
}

void
DebugSettingsWidget::loadSettings(const Settings &settings)
{
   mUi.checkBoxBreakEntry->setChecked(settings.decaf.debugger.break_on_entry);
   mUi.checkBoxGdbServerEnabled->setChecked(settings.decaf.debugger.gdb_stub);
   mUi.lineEditGdbServerPort->setText(QString("%1").arg(settings.decaf.debugger.gdb_stub_port));
}

void
DebugSettingsWidget::saveSettings(Settings &settings)
{
   settings.decaf.debugger.break_on_entry = mUi.checkBoxBreakEntry->isChecked();
   settings.decaf.debugger.gdb_stub = mUi.checkBoxGdbServerEnabled->isChecked();
   settings.decaf.debugger.gdb_stub_port = mUi.lineEditGdbServerPort->text().toInt();
}
