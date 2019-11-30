#include "registerswindow.h"

#include <QFontDatabase>
#include <QTextBlock>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QScrollBar>

RegistersWindow::RegistersWindow(QWidget *parent) :
   QPlainTextEdit(parent)
{
   // Set to fixed width font
   auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
   if (!(font.styleHint() & QFont::Monospace)) {
      font.setFamily("Monospace");
      font.setStyleHint(QFont::TypeWriter);
   }
   setFont(font);
   setReadOnly(true);
   setWordWrapMode(QTextOption::NoWrap);

   mTextFormats.registerName = QTextCharFormat { };
   mTextFormats.registerName.setForeground(Qt::darkBlue);

   mTextFormats.punctuation = QTextCharFormat { };
   mTextFormats.punctuation.setForeground(Qt::darkBlue);

   mTextFormats.changedValue = QTextCharFormat { };
   mTextFormats.changedValue.setForeground(Qt::red);

   generateDocument();
   verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
   horizontalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
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
   return QString { "%1" }.arg(value, width / 4, 16, QLatin1Char { '0' }).toUpper();
}

void
RegistersWindow::generateDocument()
{
   auto cursor = QTextCursor { document() };
   cursor.beginEditBlock();
   document()->clear();

   auto registerNameWidth = 10;
   for (auto i = 0; i < 32; ++i) {
      if (i != 0) {
         cursor.insertBlock();
      }

      cursor.insertText(QString { "R%1" }.arg(i, 2, 10, QLatin1Char { '0' }), mTextFormats.registerName);
      cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));

      mRegisterCursors.gpr[i] = { cursor.block(), cursor.positionInBlock() };
      cursor.insertText(toHexString(0, 32), mTextFormats.value);
      mRegisterCursors.gpr[i].end = cursor.positionInBlock();
   }

   cursor.insertBlock();

   cursor.insertBlock();
   cursor.insertText(QString { "LR" }, mTextFormats.registerName);
   cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
   mRegisterCursors.lr = { cursor.block(), cursor.positionInBlock() };
   cursor.insertText(toHexString(0, 32), mTextFormats.value);
   mRegisterCursors.lr.end = cursor.positionInBlock();

   cursor.insertBlock();
   cursor.insertText(QString { "CTR" }, mTextFormats.registerName);
   cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
   mRegisterCursors.ctr = { cursor.block(), cursor.positionInBlock() };
   cursor.insertText(toHexString(0, 32), mTextFormats.value);
   mRegisterCursors.ctr.end = cursor.positionInBlock();

   cursor.insertBlock();
   cursor.insertText(QString { "XER" }, mTextFormats.registerName);
   cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
   mRegisterCursors.xer = { cursor.block(), cursor.positionInBlock() };
   cursor.insertText(toHexString(0, 32), mTextFormats.value);
   mRegisterCursors.xer.end = cursor.positionInBlock();

   cursor.insertBlock();
   cursor.insertText(QString { "MSR" }, mTextFormats.registerName);
   cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
   mRegisterCursors.msr = { cursor.block(), cursor.positionInBlock() };
   cursor.insertText(toHexString(0, 32), mTextFormats.value);
   mRegisterCursors.msr.end = cursor.positionInBlock();

   cursor.insertBlock();

   cursor.insertBlock();
   cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
   cursor.insertText(QString { "O Z + -" });

   for (auto i = 0; i < 8; ++i) {
      cursor.insertBlock();
      cursor.insertText(QString { "CRF%1" }.arg(i), mTextFormats.registerName);
      cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));
      mRegisterCursors.crf[i] = { cursor.block(), cursor.positionInBlock() };
      cursor.insertText("0 0 0 0", mTextFormats.value);
      mRegisterCursors.crf[i].end = cursor.positionInBlock();
   }

   cursor.insertBlock();

   for (auto i = 0; i < 32; ++i) {
      cursor.insertBlock();
      cursor.insertText(QString { "F%1" }.arg(i, 2, 10, QLatin1Char { '0' }), mTextFormats.registerName);
      cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));

      mRegisterCursors.fpr[i] = { cursor.block(), cursor.positionInBlock() };
      cursor.insertText(toHexString(0, 64) + QString { " 0" }, mTextFormats.value);
      mRegisterCursors.fpr[i].end = cursor.positionInBlock();
   }

   for (auto i = 0; i < 32; ++i) {
      cursor.insertBlock();
      cursor.insertText(QString { "P%1" }.arg(i, 2, 10, QLatin1Char { '0' }), mTextFormats.registerName);
      cursor.insertText(QString { ' ' }.repeated(registerNameWidth - cursor.positionInBlock()));

      mRegisterCursors.psf[i] = { cursor.block(), cursor.positionInBlock() };
      cursor.insertText(toHexString(0, 64) + QString { " 0" }, mTextFormats.value);
      mRegisterCursors.psf[i].end = cursor.positionInBlock();
   }

   cursor.endEditBlock();
}

void
RegistersWindow::debugDataChanged()
{
   auto thread = mDebugData->activeThread();
   if (!thread) {
      return;
   }

   auto clearChangedHighlight = !mDebugData->paused() || mLastThreadState.nia != thread->nia;
   auto cursor = QTextCursor { document() };
   cursor.beginEditBlock();

   auto updateRegisterText =
      [&](RegisterCursor &reg, auto lastValue, auto currentValue, auto toString) {
         cursor.setPosition(reg.block.position() + reg.start);
         cursor.setPosition(reg.block.position() + reg.end, QTextCursor::KeepAnchor);

         auto wasChangedValue = cursor.charFormat().foreground().color() == mTextFormats.changedValue.foreground().color();

         if (lastValue != currentValue) {
            cursor.insertText(toString(currentValue), mTextFormats.changedValue);
            reg.end = cursor.selectionEnd() - reg.block.position();
         } else if (clearChangedHighlight) {
            if (cursor.charFormat().foreground().color() == mTextFormats.changedValue.foreground().color()) {
               cursor.setCharFormat(mTextFormats.value);
            }
         }
      };

   for (auto i = 0u; i < 32; ++i) {
      updateRegisterText(mRegisterCursors.gpr[i], mLastThreadState.gpr[i], thread->gpr[i],
                         [](auto value) { return toHexString(value, 32); });
   }

   updateRegisterText(mRegisterCursors.lr, mLastThreadState.lr, thread->lr,
                      [](auto value) { return toHexString(value, 32); });
   updateRegisterText(mRegisterCursors.ctr, mLastThreadState.ctr, thread->ctr,
                      [](auto value) { return toHexString(value, 32); });
   updateRegisterText(mRegisterCursors.xer, mLastThreadState.xer, thread->xer,
                      [](auto value) { return toHexString(value, 32); });
   updateRegisterText(mRegisterCursors.msr, mLastThreadState.msr, thread->msr,
                      [](auto value) { return toHexString(value, 32); });

   for (auto i = 0; i < 8; ++i) {
      auto lastValue = (mLastThreadState.cr >> ((7 - i) * 4)) & 0xF;
      auto value = (thread->cr >> ((7 - i) * 4)) & 0xF;
      updateRegisterText(mRegisterCursors.crf[i], lastValue, value,
         [](auto value) {
            return QString { "%1 %2 %3 %4" }
               .arg((value >> 0) & 1)
               .arg((value >> 1) & 1)
               .arg((value >> 2) & 1)
               .arg((value >> 3) & 1);
         });
   }

   for (auto i = 0u; i < 32; ++i) {
      updateRegisterText(mRegisterCursors.fpr[i], mLastThreadState.fpr[i], thread->fpr[i],
         [](auto value) {
            return QString { "%1 %2" }
               .arg(toHexString(*reinterpret_cast<uint64_t *>(&value), 64).toUpper())
               .arg(value);
         });
   }

   for (auto i = 0u; i < 32; ++i) {
      updateRegisterText(mRegisterCursors.psf[i], mLastThreadState.ps1[i], thread->ps1[i],
         [](auto value) {
            return QString { "%1 %2" }
               .arg(toHexString(*reinterpret_cast<uint64_t *>(&value), 64).toUpper())
               .arg(value);
         });
   }

   cursor.endEditBlock();
   mLastThreadState = *thread;
}
