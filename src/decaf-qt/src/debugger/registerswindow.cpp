#include "registerswindow.h"
#include "kcollapsiblegroupbox.h"

#include <QFontDatabase>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDataWidgetMapper>

RegistersWindow::RegistersWindow(QWidget *parent) :
   QWidget(parent)
{
   // Set to fixed width font
   auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
   if (!(font.styleHint() & QFont::Monospace)) {
      font.setFamily("Monospace");
      font.setStyleHint(QFont::TypeWriter);
   }
   setFont(font);

   auto baseLayout = new QVBoxLayout { };
   setLayout(baseLayout);

   // TODO: Fpscr?

   // Text colour to red when changed
   mChangedPalette.setColor(QPalette::Text, Qt::red);

   {
      auto group = new KCollapsibleGroupBox { };
      auto layout = new QGridLayout { };
      group->setTitle(tr("General Purpose Registers"));
      group->setExpanded(true);
      group->setLayout(layout);

      for (auto i = 0; i < 32; ++i) {
         auto row = i % 16;
         auto column = 2 * (i / 16);
         auto label = new QLabel { QString { "r%1" }.arg(i) };
         label->setToolTip(tr("General Purpose Register %1").arg(i));

         auto value = new QLineEdit { "00000000" };
         value->setInputMask("HHHHHHHH");
         value->setReadOnly(true);

         layout->addWidget(label, row, column + 0);
         layout->addWidget(value, row, column + 1);
         mRegisterWidgets.gpr[i] = value;
      }

      baseLayout->addWidget(group);
      mGroups.gpr = group;
   }

   {
      auto group = new KCollapsibleGroupBox { };
      auto layout = new QGridLayout { };
      group->setTitle(tr("Misc Registers"));
      group->setExpanded(true);
      group->setLayout(layout);

      {
         auto label = new QLabel { "LR" };
         label->setToolTip(tr("Link Register"));

         auto value = new QLineEdit { "00000000" };
         value->setInputMask("HHHHHHHH");
         value->setReadOnly(true);

         layout->addWidget(label, 0, 0);
         layout->addWidget(value, 0, 1);
         mRegisterWidgets.lr = value;
      }

      {
         auto label = new QLabel { "CTR" };
         label->setToolTip(tr("Count Register"));

         auto value = new QLineEdit { "00000000" };
         value->setInputMask("HHHHHHHH");
         value->setReadOnly(true);

         layout->addWidget(label, 0, 2);
         layout->addWidget(value, 0, 3);
         mRegisterWidgets.ctr = value;
      }

      {
         auto label = new QLabel { "XER" };
         label->setToolTip(tr("Fixed Point Exception Register"));

         auto value = new QLineEdit { "00000000" };
         value->setInputMask("HHHHHHHH");
         value->setReadOnly(true);

         layout->addWidget(label, 1, 0);
         layout->addWidget(value, 1, 1);
         mRegisterWidgets.xer = value;
      }

      {
         auto label = new QLabel { "MSR" };
         label->setToolTip(tr("Machine State Register"));

         auto value = new QLineEdit { "00000000" };
         value->setInputMask("HHHHHHHH");
         value->setReadOnly(true);

         layout->addWidget(label, 1, 2);
         layout->addWidget(value, 1, 3);
         mRegisterWidgets.msr = value;
      }

      baseLayout->addWidget(group);
      mGroups.misc = group;
   }

   {
      auto group = new KCollapsibleGroupBox { };
      auto layout = new QGridLayout { };
      group->setTitle(tr("Condition Registers"));
      group->setExpanded(true);
      group->setLayout(layout);

      for (auto i = 0; i < 2; ++i) {
         auto header = new QLineEdit { "O Z + -" };
         header->setStyleSheet("background: transparent; border: 0px;");
         header->setFont(font);
         header->setReadOnly(true);
         header->setToolTip(tr("Overflow Zero Positive Negative"));
         layout->addWidget(header, 0, 1 + i * 2);
      }

      for (auto i = 0; i < 8; ++i) {
         auto row = 1 + i % 4;
         auto column = 2 * (i / 4);
         auto label = new QLabel { QString { "crf%1" }.arg(i) };
         label->setToolTip(tr("Condition Register Field %1").arg(i));

         auto value = new QLineEdit { "0 0 0 0" };
         value->setInputMask("B B B B");
         value->setReadOnly(true);

         layout->addWidget(label, row, column + 0);
         layout->addWidget(value, row, column + 1);

         mRegisterWidgets.cr[i] = value;
      }

      baseLayout->addWidget(group);
      mGroups.cr = group;
   }

   {
      auto group = new KCollapsibleGroupBox { };
      auto layout = new QGridLayout { };
      group->setTitle(tr("Floating Point Registers"));
      group->setExpanded(true);
      group->setLayout(layout);

      for (auto i = 0; i < 32; ++i) {
         auto label = new QLabel { QString { "f%1" }.arg(i) };

         auto hexValue = new QLineEdit { "0000000000000000" };
         hexValue->setInputMask("HHHHHHHHHHHHHHHH");
         hexValue->setReadOnly(true);

         auto floatValue = new QLineEdit { "0.0" };
         floatValue->setReadOnly(true);

         layout->addWidget(label, i, 0);
         layout->addWidget(hexValue, i, 1);
         layout->addWidget(floatValue, i, 2);

         mRegisterWidgets.fprFloat[i] = floatValue;
         mRegisterWidgets.fprHex[i] = hexValue;
      }

      baseLayout->addWidget(group);
      mGroups.fpr = group;
   }

   {
      auto group = new KCollapsibleGroupBox { };
      auto layout = new QGridLayout { };
      group->setTitle(tr("Paired Single 1 Registers"));
      group->setExpanded(true);
      group->setLayout(layout);

      for (auto i = 0; i < 32; ++i) {
         auto label = new QLabel { QString { "p%1" }.arg(i) };

         auto hexValue = new QLineEdit { "0000000000000000" };
         hexValue->setInputMask("HHHHHHHHHHHHHHHH");
         hexValue->setReadOnly(true);

         auto floatValue = new QLineEdit { "0.0" };
         floatValue->setReadOnly(true);

         layout->addWidget(label, i, 0);
         layout->addWidget(hexValue, i, 1);
         layout->addWidget(floatValue, i, 2);

         mRegisterWidgets.ps1Float[i] = floatValue;
         mRegisterWidgets.ps1Hex[i] = hexValue;
      }

      baseLayout->addWidget(group);
      mGroups.ps1 = group;
   }

   baseLayout->addStretch(1);
}

void
RegistersWindow::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   connect(mDebugData, &DebugData::dataChanged, this, &RegistersWindow::debugDataChanged);
}

template<typename T>
static QString toHexString(T value, int width)
{
   return QString { "%1" }.arg(value, width / 4, 16, QLatin1Char{ '0' }).toUpper();
}

void
RegistersWindow::updateRegisterValue(QLineEdit *lineEdit,
                                     uint32_t value)
{
   auto text = toHexString(value, 32);
   if (text != lineEdit->text()) {
      lineEdit->setText(text);
      lineEdit->setPalette(mChangedPalette);
   } else if (!mDebugPaused) {
      lineEdit->setPalette({ });
   }
}

void
RegistersWindow::updateRegisterValue(QLineEdit *lineEditFloat,
                                   QLineEdit *lineEditHex,
                                   double value)
{
   auto hexText = toHexString(*reinterpret_cast<uint64_t *>(&value), 64);

   if (hexText != lineEditHex->text()) {
      lineEditFloat->setText(QString { "%1" }.arg(value));
      lineEditFloat->setPalette(mChangedPalette);

      lineEditHex->setText(hexText);
      lineEditHex->setPalette(mChangedPalette);
   } else if (!mDebugPaused) {
      lineEditFloat->setPalette({ });
      lineEditHex->setPalette({ });
   }
}

void
RegistersWindow::updateConditionRegisterValue(QLineEdit *lineEdit, uint32_t value)
{
   auto text = QString { "%1 %2 %3 %4" }
      .arg((value >> 0) & 1)
      .arg((value >> 1) & 1)
      .arg((value >> 2) & 1)
      .arg((value >> 3) & 1);

   if (text != lineEdit->text()) {
      lineEdit->setText(text);
      lineEdit->setPalette(mChangedPalette);
   } else if (!mDebugPaused) {
      lineEdit->setPalette({ });
   }
}

void
RegistersWindow::debugDataChanged()
{
   auto thread = mDebugData->activeThread();
   if (!thread) {
      return;
   }

   for (auto i = 0; i < mRegisterWidgets.gpr.size(); ++i) {
      updateRegisterValue(mRegisterWidgets.gpr[i], thread->gpr[i]);
   }

   for (auto i = 0; i < mRegisterWidgets.cr.size(); ++i) {
      auto value = (thread->cr >> ((7 - i) * 4)) & 0xF;
      updateConditionRegisterValue(mRegisterWidgets.cr[i], value);
   }

   updateRegisterValue(mRegisterWidgets.lr, thread->lr);
   updateRegisterValue(mRegisterWidgets.ctr, thread->ctr);
   updateRegisterValue(mRegisterWidgets.xer, thread->xer);
   updateRegisterValue(mRegisterWidgets.msr, thread->msr);

   for (auto i = 0; i < mRegisterWidgets.fprFloat.size(); ++i) {
      updateRegisterValue(mRegisterWidgets.fprFloat[i],
                          mRegisterWidgets.fprHex[i],
                          thread->fpr[i]);
   }

   for (auto i = 0; i < mRegisterWidgets.ps1Float.size(); ++i) {
      updateRegisterValue(mRegisterWidgets.ps1Float[i],
                          mRegisterWidgets.ps1Hex[i],
                          thread->ps1[i]);
   }
}
