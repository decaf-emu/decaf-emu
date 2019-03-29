#include "disassemblywidget.h"

#include <QtEndian>
#include <QPainter>

#include <libcpu/cpu_breakpoints.h>
#include <libcpu/espresso/espresso_instructionset.h>
#include <libdecaf/decaf_debug_api.h>

DisassemblyWidget::DisassemblyWidget(QWidget *parent) :
   AddressTextDocumentWidget(parent)
{
   setBytesPerLine(4);
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
   setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

   mTextFormats.breakpoint = QTextCharFormat{ };
   mTextFormats.breakpoint.setBackground(Qt::red);

   mTextFormats.lineAddress = QTextCharFormat { };
   mTextFormats.lineAddress.setForeground(Qt::black);

   mTextFormats.instructionData = QTextCharFormat{ };
   mTextFormats.instructionData.setForeground(Qt::gray);

   mTextFormats.instructionName = QTextCharFormat { };
   mTextFormats.instructionName.setForeground(Qt::darkBlue);

   mTextFormats.registerName = QTextCharFormat { };
   mTextFormats.registerName.setForeground(Qt::darkBlue);

   mTextFormats.punctuation = QTextCharFormat { };
   mTextFormats.punctuation.setForeground(Qt::darkBlue);

   mTextFormats.branchAddress = QTextCharFormat { };
   mTextFormats.branchAddress.setForeground(Qt::blue);

   mTextFormats.symbolName = QTextCharFormat { };
   mTextFormats.symbolName.setForeground(Qt::blue);

   mTextFormats.numericValue = QTextCharFormat { };
   mTextFormats.numericValue.setForeground(Qt::darkGreen);

   mTextFormats.invalid = QTextCharFormat { };
   mTextFormats.invalid.setForeground(Qt::darkGray);

   mTextFormats.comment = QTextCharFormat{ };
   mTextFormats.comment.setForeground(Qt::gray);

   mTextFormats.functionOutline = Qt::black;

   mTextFormats.branchDirectionArrow = Qt::darkBlue;
   mTextFormats.branchOutlineTrue = Qt::darkGreen;
   mTextFormats.branchOutlineFalse = Qt::darkRed;

   auto arrowSize = characterWidth() - 1;
   mTextFormats.branchDownArrowPath = QPainterPath { };
   mTextFormats.branchDownArrowPath.moveTo(0, 0);
   mTextFormats.branchDownArrowPath.lineTo(arrowSize, 0);
   mTextFormats.branchDownArrowPath.lineTo(arrowSize / 2.0, arrowSize);
   mTextFormats.branchDownArrowPath.lineTo(0, 0);
   mTextFormats.branchDownArrowPath.translate(-arrowSize / 2.0, -arrowSize / 2.0);

   mTextFormats.branchUpArrowPath = QPainterPath { };
   mTextFormats.branchUpArrowPath.moveTo(0, arrowSize);
   mTextFormats.branchUpArrowPath.lineTo(arrowSize, arrowSize);
   mTextFormats.branchUpArrowPath.lineTo(arrowSize / 2.0, 0);
   mTextFormats.branchUpArrowPath.lineTo(0, arrowSize);
   mTextFormats.branchUpArrowPath.translate(-arrowSize / 2.0, -arrowSize / 2.0);
}

void
DisassemblyWidget::setDebugData(DebugData *debugData)
{
   mDebugData = debugData;
}

void
DisassemblyWidget::followSymbolUnderCursor()
{
   followSymbolAtCursor(getDocumentCursor());
}

void
DisassemblyWidget::toggleBreakpointUnderCursor()
{
   auto address = getDocumentCursor().address;
   if (decaf::debug::hasBreakpoint(address)) {
      decaf::debug::removeBreakpoint(address);
   } else {
      decaf::debug::addBreakpoint(address);
   }

   AddressTextDocumentWidget::updateTextDocument();
}

void
DisassemblyWidget::followSymbolAtCursor(DocumentCursor cursor)
{
   auto cacheIndex = (cursor.address - mCacheStartAddress) / 4;
   if (cacheIndex >= mTextCursorPositionCache.size()) {
      return;
   }

   auto &cursorPositionCache = mTextCursorPositionCache[cacheIndex];
   auto &disassemblyCache = mDisassemblyCache[cacheIndex];
   auto inRange = [](int cursorPosition, std::pair<int, int> range) {
      return cursorPosition >= range.first && cursorPosition < range.second;
   };

   for (auto i = 0u; i < cursorPositionCache.instructionArgs.size(); ++i) {
      auto argCursorPositions = cursorPositionCache.instructionArgs[i];
      if (!inRange(cursor.cursorPosition, argCursorPositions)) {
         continue;
      }

      auto arg = disassemblyCache.disassembly.args[i];
      if (arg.type == espresso::Disassembly::Argument::Address) {
         navigateToAddress(arg.address);
      }
   }

   if (inRange(cursor.cursorPosition, cursorPositionCache.referencedSymbol)) {
      navigateToAddress(disassemblyCache.referenceLookup->start);
   }
}

void
DisassemblyWidget::paintEvent(QPaintEvent *e)
{
   AddressTextDocumentWidget::paintEvent(e);

   auto painter = QPainter { viewport() };

   if (mVisibleColumns.functionOutline) {
      auto offset = documentMargin();
      if (mVisibleColumns.lineAddress) {
         offset += (mTextCursorPositionCache[0].lineAddress.second + 1) * characterWidth();
      }

      auto functionLineStartY = -1;
      auto functionLineX1 = offset + (characterWidth() / 2);
      auto functionLineX2 = functionLineX1 + characterWidth();
      auto lineY = documentMargin() + lineHeight() / 2;

      painter.setPen(QPen { mTextFormats.functionOutline });
      for (auto &item : mDisassemblyCache) {
         if (item.addressLookup.function) {
            if (item.addressLookup.function->start == item.address &&
                item.addressLookup.function->end != 0xFFFFFFFF) {
               // Start of function
               painter.drawLine(functionLineX1, lineY, functionLineX2, lineY);
               functionLineStartY = lineY;
            } else if (item.addressLookup.function->end == item.address + 4) {
               // End of function
               painter.drawLine(functionLineX1, functionLineStartY, functionLineX1, lineY);
               painter.drawLine(functionLineX1, lineY, functionLineX2, lineY);
               functionLineStartY = -1;
            } else {
               // Inside function
               if (functionLineStartY == -1) {
                  functionLineStartY = 0;
               }
            }
         }

         lineY += lineHeight();
      }

      if (functionLineStartY != -1) {
         painter.drawLine(functionLineX1, functionLineStartY, functionLineX1, viewport()->height());
      }
   }

   if (mVisibleColumns.branchOutline) {
      auto offset = documentMargin();
      if (mVisibleColumns.instructionData) {
         offset += (mTextCursorPositionCache[0].instructionData.second + 1) * characterWidth();
      } else {
         if (mVisibleColumns.lineAddress) {
            offset += (mTextCursorPositionCache[0].lineAddress.second + 1) * characterWidth();
         }

         if (mVisibleColumns.functionOutline) {
            offset += (mPunctuation.functionOutline.size() + 1) * characterWidth();
         }
      }

      auto lineY = documentMargin() + lineHeight() / 2;
      auto outlineX = offset + characterWidth() / 2;
      auto arrowLeftX = offset + characterWidth() * 2 + 2;

      auto cursor = getDocumentCursor();
      auto ctr = 0u;
      auto cr = 0u;
      auto lr = 0u;

      if (auto activeThread = mDebugData->activeThread()) {
         ctr = activeThread->ctr;
         cr = activeThread->cr;
         lr = activeThread->lr;
      }

      for (auto &item : mDisassemblyCache) {
         if (item.disassembly.instruction &&
             espresso::isBranchInstruction(item.disassembly.instruction->id)) {
            auto instr = qFromBigEndian<uint32_t>(item.data.data());
            auto info =
               espresso::disassembleBranchInfo(item.disassembly.instruction->id,
                                               instr, item.address, ctr, cr, lr);
            if (!info.isVariable && !info.isCall) {
               if (info.target > item.address) {
                  painter.fillPath(
                     mTextFormats.branchDownArrowPath.translated(arrowLeftX, lineY + 2),
                     mTextFormats.branchDirectionArrow);
               } else {
                  painter.fillPath(
                     mTextFormats.branchUpArrowPath.translated(arrowLeftX, lineY + 2),
                     mTextFormats.branchDirectionArrow);
               }

               if (cursor.address == item.address) {
                  painter.setPen(QPen {
                     info.conditionSatisfied ?
                        mTextFormats.branchOutlineTrue :
                        mTextFormats.branchOutlineFalse });
                  painter.drawLine(outlineX, lineY, outlineX + characterWidth(), lineY);
                  if (info.target < mCacheStartAddress) {
                     painter.drawLine(outlineX, 0, outlineX, lineY);
                  } else {
                     auto index = (info.target - mCacheStartAddress) / 4;
                     if (index >= mDisassemblyCache.size()) {
                        painter.drawLine(outlineX, lineY, outlineX, viewport()->height());
                     } else {
                        auto targetLineY = documentMargin() + lineHeight() / 2 + lineHeight() * index;
                        painter.drawLine(outlineX, lineY, outlineX, targetLineY);
                        painter.drawLine(outlineX, targetLineY, outlineX + characterWidth(), targetLineY);
                     }
                  }
               }
            }
         }

         lineY += lineHeight();
      }
   }
}

void
DisassemblyWidget::mouseReleaseEvent(QMouseEvent *e)
{
   auto handled = false;

   if ((e->modifiers() & Qt::ControlModifier) && e->button() == Qt::LeftButton) {
      if (auto hit = mouseEventHitTest(e)) {
         followSymbolAtCursor({ hit->lineAddress, hit->textCursor.positionInBlock() });
         handled = true;
      }
   }

   if (!handled) {
      AddressTextDocumentWidget::mouseReleaseEvent(e);
   }
}

void
DisassemblyWidget::keyPressEvent(QKeyEvent *e)
{
   auto handled = false;

   if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
      followSymbolUnderCursor();
      handled = true;
   }

   if (!handled) {
      AddressTextDocumentWidget::keyPressEvent(e);
   }
}

void
DisassemblyWidget::updateTextDocument(QTextCursor cursor, VirtualAddress firstLineAddress, VirtualAddress lastLineAddress, int bytesPerLine)
{
   auto breakpoints = cpu::getBreakpoints();
   auto getBreakPoint = [&](uint32_t address) -> cpu::Breakpoint * {
      if (breakpoints) {
         for (auto &breakpoint : *breakpoints) {
            if (breakpoint.address == address) {
               return &breakpoint;
            }
         }
      }

      return nullptr;
   };

   mCacheStartAddress = firstLineAddress;
   mTextCursorPositionCache.clear();
   mDisassemblyCache.clear();

   for (auto address = static_cast<int64_t>(firstLineAddress);
        address <= lastLineAddress; address += bytesPerLine) {
      auto &cursorPositionCache = mTextCursorPositionCache.emplace_back();
      auto &item = mDisassemblyCache.emplace_back();
      item.address = static_cast<VirtualAddress>(address);
      item.valid = decaf::debug::readMemory(item.address, item.data.data(), bytesPerLine) == bytesPerLine;

      auto breakpoint = getBreakPoint(item.address);
      if (breakpoint) {
         item.data[0] = (breakpoint->savedCode >> 0) & 0xFF;
         item.data[1] = (breakpoint->savedCode >> 8) & 0xFF;
         item.data[2] = (breakpoint->savedCode >> 16) & 0xFF;
         item.data[3] = (breakpoint->savedCode >> 24) & 0xFF;
      }

      if (item.address != firstLineAddress) {
         cursor.insertBlock();
      }

      if (mVisibleColumns.lineAddress) {
         cursorPositionCache.lineAddress.first = cursor.positionInBlock();
         cursor.insertText(
            QString { "%1" }.arg(item.address, 8, 16, QLatin1Char { '0' }),
            breakpoint ?
               mTextFormats.breakpoint :
               (!item.valid ?
                  mTextFormats.invalid :
                  mTextFormats.lineAddress));
         cursorPositionCache.lineAddress.second = cursor.positionInBlock();
         cursor.insertText(mPunctuation.afterLineAddress, mTextFormats.punctuation);
      }

      if (!item.valid) {
         continue;
      }

      if (mVisibleColumns.functionOutline) {
         cursor.insertText(mPunctuation.functionOutline, mTextFormats.punctuation);
      }

      if (mVisibleColumns.instructionData) {
         cursorPositionCache.instructionData.first = cursor.positionInBlock();
         cursor.insertText(QString { "%1 %2 %3 %4" }
            .arg(item.data[0], 2, 16, QLatin1Char { '0' })
            .arg(item.data[1], 2, 16, QLatin1Char { '0' })
            .arg(item.data[2], 2, 16, QLatin1Char { '0' })
            .arg(item.data[3], 2, 16, QLatin1Char { '0' })
            .toUpper(), mTextFormats.instructionData);
         cursorPositionCache.instructionData.second = cursor.positionInBlock();
         cursor.insertText(mPunctuation.afterInstructionData, mTextFormats.punctuation);
      }

      auto disassemblyValid = espresso::disassemble(qFromBigEndian<uint32_t>(item.data.data()), item.disassembly, item.address);

      if (mVisibleColumns.branchOutline) {
         cursor.insertText(mPunctuation.branchOutline, mTextFormats.punctuation);
      }

      if (mVisibleColumns.instructionName) {
         cursorPositionCache.instructionName.first = cursor.positionInBlock();
         if (!disassemblyValid) {
            cursor.insertText("???", mTextFormats.invalid);
         } else {
            cursor.insertText(QString::fromStdString(item.disassembly.name),
                              mTextFormats.instructionName);
         }
         cursorPositionCache.instructionName.second = cursor.positionInBlock();

         cursor.insertText(
            QString { ' ' }.repeated(
               std::max(0, mPunctuation.instructionNameWidth -
                           static_cast<int>(item.disassembly.name.size()))),
            mTextFormats.punctuation);
      }

      if (mVisibleColumns.instructionArgs) {
         auto beforeArgsPosition = cursor.position();
         auto firstArg = true;
         for (const auto &arg : item.disassembly.args) {
            if (!firstArg) {
               cursor.insertText(QString { ", " }, mTextFormats.punctuation);
            }
            firstArg = false;

            auto &argCursorPosition = cursorPositionCache.instructionArgs.emplace_back();
            argCursorPosition.first = cursor.positionInBlock();
            switch (arg.type) {
            case espresso::Disassembly::Argument::Address:
            {
               auto lookup =
                  decaf::debug::analyseLookupFunction(mDebugData->analyseDatabase(), arg.address);
               if (lookup && lookup->start == arg.address && !lookup->name.empty()) {
                  item.referenceLookup = lookup;
               }

               cursor.insertText(
                  QString { "@%1" }.arg(arg.address, 8, 16, QLatin1Char { '0' }),
                  mTextFormats.branchAddress);
               break;
            }
            case espresso::Disassembly::Argument::Register:
               cursor.insertText(
                  QString::fromStdString(arg.registerName),
                  mTextFormats.registerName);
               break;
            case espresso::Disassembly::Argument::ValueUnsigned:
               if (arg.valueUnsigned > 9) {
                  cursor.insertText(
                     QString { "0x%1" }.arg(arg.valueUnsigned, 0, 16),
                     mTextFormats.numericValue);
               } else {
                  cursor.insertText(
                     QString { "%1" }.arg(arg.valueUnsigned),
                     mTextFormats.numericValue);
               }
               break;
            case espresso::Disassembly::Argument::ValueSigned:
               if (arg.valueSigned < -9) {
                  cursor.insertText(
                     QString { "-0x%1" }.arg(-arg.valueSigned, 0, 16),
                     mTextFormats.numericValue);
               } else if (arg.valueSigned > 9) {
                  cursor.insertText(
                     QString { "0x%1" }.arg(arg.valueSigned, 0, 16),
                     mTextFormats.numericValue);
               } else {
                  cursor.insertText(
                     QString { "%1" }.arg(arg.valueSigned),
                     mTextFormats.numericValue);
               }
               break;
            case espresso::Disassembly::Argument::ConstantUnsigned:
               cursor.insertText(
                  QString { "%1" }.arg(arg.constantUnsigned),
                  mTextFormats.numericValue);
               break;
            case espresso::Disassembly::Argument::ConstantSigned:
               cursor.insertText(
                  QString { "%1" }.arg(arg.constantSigned),
                  mTextFormats.numericValue);
               break;
            default:
               cursor.insertText(
                  QString { '?' },
                  mTextFormats.invalid);
               break;
            }
            argCursorPosition.second = cursor.positionInBlock();
         }

         auto afterArgsPosition = cursor.position();
         cursor.insertText(
            QString { ' ' }.repeated(std::max(0, mPunctuation.instructionArgsWidth - (afterArgsPosition - beforeArgsPosition))),
            mTextFormats.punctuation);
      }

      if (mVisibleColumns.referencedSymbol && item.referenceLookup) {
         cursorPositionCache.referencedSymbol.first = cursor.positionInBlock();
         cursor.insertText(
            QString { "@%1" }.arg(QString::fromStdString(item.referenceLookup->name)),
            mTextFormats.symbolName);
         cursorPositionCache.referencedSymbol.second = cursor.positionInBlock();
         cursor.insertText(mPunctuation.afterReferencedSymbol, mTextFormats.punctuation);
      }

      item.addressLookup = decaf::debug::analyseLookupAddress(mDebugData->analyseDatabase(), item.address);
      if (item.addressLookup.function &&
          item.addressLookup.function->start == item.address &&
          !item.addressLookup.function->name.empty()) {
         cursor.setCharFormat(mTextFormats.comment);
         cursor.insertText(mPunctuation.beforeComment);
         cursorPositionCache.commentFunctionName.first = cursor.positionInBlock();
         cursor.insertText(QString::fromStdString(item.addressLookup.function->name));
         cursorPositionCache.commentFunctionName.second = cursor.positionInBlock();
      }
   }
}
