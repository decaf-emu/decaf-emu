#include "stackwidget.h"

#include <qendian.h>
#include <QPainter>
#include <QScrollBar>

#include <libdecaf/decaf_debug_api.h>

StackWidget::StackWidget(QWidget *parent) :
   AddressTextDocumentWidget(parent)
{
   setBytesPerLine(4);
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

   mTextFormats.invalid = QTextCharFormat { };
   mTextFormats.invalid.setForeground(Qt::gray);

   mTextFormats.lineAddress = QTextCharFormat { };
   mTextFormats.lineAddress.setForeground(Qt::black);

   mTextFormats.data = QTextCharFormat { };
   mTextFormats.data.setForeground(Qt::gray);

   mTextFormats.symbolName = QTextCharFormat { };
   mTextFormats.symbolName.setForeground(Qt::blue);

   mTextFormats.referencedAscii = QTextCharFormat { };
   mTextFormats.referencedAscii.setForeground(Qt::gray);

   mTextFormats.activeLineAddress = QTextCharFormat { };
   mTextFormats.activeLineAddress.setBackground(Qt::green);

   mTextFormats.stackOutline = Qt::darkBlue;
   mTextFormats.backchainOutline = Qt::black;
}

void
StackWidget::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
   connect(mDebugData, &DebugData::dataChanged, this, &StackWidget::dataChanged);
   connect(mDebugData, &DebugData::activeThreadIndexChanged, this, &StackWidget::activeThreadChanged);
}

void
StackWidget::dataChanged()
{
   AddressTextDocumentWidget::updateTextDocument();
}

void
StackWidget::activeThreadChanged()
{
   if (auto activeThread = mDebugData->activeThread()) {
      setAddressRange(activeThread->stackEnd, activeThread->stackStart);
      navigateToAddress(activeThread->gpr[1]);
      AddressTextDocumentWidget::updateTextDocument();
   }
}

void
StackWidget::paintEvent(QPaintEvent *e)
{
   AddressTextDocumentWidget::paintEvent(e);
   auto painter = QPainter { viewport() };

   if (mVisibleColumns.outline) {
      auto offset = documentMargin();
      if (mVisibleColumns.lineAddress) {
         offset += 9 * characterWidth();
      }

      auto startAddress = getStartAddress();
      auto firstVisibleLine = verticalScrollBar()->value();
      auto lastVisibleLine = firstVisibleLine + (verticalScrollBar()->pageStep() - 1);

      auto currentLineStartY = -1;
      auto currentLineIsBackchain = false;
      auto lineX1 = offset + (characterWidth() / 2);
      auto lineX2 = lineX1 + characterWidth();
      auto lineY = documentMargin() + lineHeight() / 2;

      auto itr = mStackFrames.lower_bound(startAddress + firstVisibleLine * 4);

      for (auto i = firstVisibleLine; i < lastVisibleLine; ++i) {
         auto address = startAddress + i * 4;

         if (address >= itr->second.end) {
            ++itr;
         }

         if (itr == mStackFrames.end()) {
            break;
         }

         if (address == itr->second.start) {
            // Start of frame
            painter.setPen(mTextFormats.stackOutline);
            painter.drawLine(lineX1, lineY, lineX2, lineY);
            currentLineStartY = lineY;
            currentLineIsBackchain = false;
         } else if (address == itr->second.end - 12) {
            // End of frame
            painter.setPen(mTextFormats.stackOutline);
            painter.drawLine(lineX1, currentLineStartY, lineX1, lineY);
            painter.drawLine(lineX1, lineY, lineX2, lineY);
            currentLineStartY = lineY;
            currentLineIsBackchain = true;
         } else if (address == itr->second.end - 4) {
            // End of back chain
            painter.setPen(mTextFormats.backchainOutline);
            painter.drawLine(lineX1, currentLineStartY, lineX1, lineY);
            painter.drawLine(lineX1, lineY, lineX2, lineY);
            currentLineStartY = -1;
         } else if (address > itr->second.start && address < itr->second.end) {
            // Inside stack frame
            if (currentLineStartY == -1) {
               currentLineStartY = 0;
               currentLineIsBackchain = false;
            }
         }

         lineY += lineHeight();
      }

      if (currentLineStartY != -1) {
         painter.setPen(QPen {
            currentLineIsBackchain ?
               mTextFormats.backchainOutline :
               mTextFormats.stackOutline });
         painter.drawLine(lineX1, currentLineStartY, lineX1, viewport()->height());
      }
   }
}

void
StackWidget::updateStackFrames()
{
   if (auto activeThread = mDebugData->activeThread()) {
      mStackCurrentAddress = activeThread->gpr[1];
   }

   auto frame = StackFrame { };
   auto address = mStackCurrentAddress + 8;
   auto startAddress = getEndAddress();
   auto frameEnd = std::array<uint8_t, 4> { };
   mStackFrames.clear();

   while (true) {
      if (address < mStackCurrentAddress ||
         !decaf::debug::isValidVirtualAddress(address)) {
         // If we somehow jump outside of our valid range, lets stop
         break;
      }

      decaf::debug::readMemory(address - 8, frameEnd.data(), 4);

      frame.start = address;
      frame.end = qFromBigEndian<uint32_t>(frameEnd.data()) + 8;

      if (mStackFrames.find(frame.end) != mStackFrames.end()) {
         // Prevent an infinite loop with a buggy stack!
         break;
      }

      if (frame.end > startAddress) {
         // Stop when we go outside of the stack
         break;
      }

      mStackFrames.emplace(frame.end, frame);
      address = frame.end;
   }
}

void
StackWidget::updateTextDocument(QTextCursor cursor,
                                VirtualAddress firstLineAddress,
                                VirtualAddress lastLineAddress,
                                int bytesPerLine)
{
   auto data = std::array<uint8_t, 4> { };
   auto symbolNameBuffer = std::array<char, 256> { };
   auto moduleNameBuffer = std::array<char, 256> { };

   updateStackFrames();

   for (auto address = static_cast<int64_t>(firstLineAddress);
        address <= lastLineAddress; address += bytesPerLine) {
      auto valid = decaf::debug::readMemory(address, data.data(), data.size()) == data.size();

      if (address != firstLineAddress) {
         cursor.insertBlock();
      }

      if (mVisibleColumns.lineAddress) {
         cursor.insertText(
            QString { "%1" }.arg(address, 8, 16, QLatin1Char{ '0' }),
            valid ? mTextFormats.lineAddress : mTextFormats.invalid);
         cursor.insertText(mPunctuation.afterLineAddress, mTextFormats.punctuation);
      }

      if (mVisibleColumns.outline) {
         cursor.insertText(mPunctuation.outline, mTextFormats.punctuation);
      }

      if (mVisibleColumns.data) {
         cursor.insertText(QString { "%1 %2 %3 %4" }
            .arg(data[0], 2, 16, QLatin1Char { '0' })
            .arg(data[1], 2, 16, QLatin1Char { '0' })
            .arg(data[2], 2, 16, QLatin1Char { '0' })
            .arg(data[3], 2, 16, QLatin1Char { '0' })
            .toUpper(), mTextFormats.data);
         cursor.insertText(mPunctuation.afterData, mTextFormats.punctuation);
      }

      if (mVisibleColumns.dataReference) {
         auto value = qFromBigEndian<VirtualAddress>(data.data());
         if (value > 0 && value < 0x10000000) {
            auto symbolDistance = uint32_t { 0 };
            auto symbolFound =
               decaf::debug::findClosestSymbol(
                  value,
                  &symbolDistance,
                  symbolNameBuffer.data(),
                  static_cast<uint32_t>(symbolNameBuffer.size()),
                  moduleNameBuffer.data(),
                  static_cast<uint32_t>(moduleNameBuffer.size()));

            if (symbolFound && moduleNameBuffer[0]) {
               cursor.insertText(
                  QString { "%1+0x%2 %3" }
                  .arg(moduleNameBuffer.data())
                  .arg(symbolDistance, 0, 16)
                  .arg(symbolNameBuffer.data()), mTextFormats.symbolName);
            }
         } else if (value >= 0x10000000) {
            // TODO: Try read ASCII ?
         }
      }
   }
}
