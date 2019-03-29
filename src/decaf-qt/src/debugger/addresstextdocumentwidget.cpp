#include "addresstextdocumentwidget.h"

#include <QApplication>
#include <QAbstractTextDocumentLayout>
#include <QFontDatabase>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QMouseEvent>
#include <QPainter>

#include <libdecaf/decaf_debug_api.h>

AddressTextDocumentWidget::AddressTextDocumentWidget(QWidget *parent) :
   QAbstractScrollArea(parent)
{
   // Set to fixed width font
   auto font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
   if (!(font.styleHint() & QFont::Monospace)) {
      font.setFamily("Monospace");
      font.setStyleHint(QFont::TypeWriter);
   }

   setFont(font);
   setMouseTracking(true);
   setCursor(Qt::IBeamCursor);

   mLineHeight = fontMetrics().height();
   mCharacterWidth = fontMetrics().horizontalAdvance(' ');

   // Setup text document
   mTextDocument = new QTextDocument { this };
   mTextDocument->setDefaultFont(font);
   mTextDocument->setUndoRedoEnabled(false);
   mTextDocument->setPlainText({});
   mDocumentMargin = mTextDocument->documentMargin();

   // Setup text formats
   mTextFormatSelection = QTextCharFormat { };
   mTextFormatSelection.setBackground(Qt::darkGray);
   mTextFormatSelection.setForeground(Qt::white);

   mTextFormatHighlightedWord = QTextCharFormat { };
   mTextFormatHighlightedWord.setBackground(Qt::yellow);

   // Setup cursor blink timer
   mBlinkTimer = new QTimer { this };
   mBlinkTimer->setInterval(qApp->cursorFlashTime());
   connect(mBlinkTimer, &QTimer::timeout, this, [this] {
      mBlinkCursorVisible = !mBlinkCursorVisible;
      viewport()->update();
   });
}

void
AddressTextDocumentWidget::setBytesPerLine(int bytesPerLine)
{
   mBytesPerLine = bytesPerLine;
   mNumLines = static_cast<int>((static_cast<int64_t>(mEndAddress - mStartAddress) + 1) / mBytesPerLine);
   updateVerticalScrollBar();
   viewport()->update();
}

void
AddressTextDocumentWidget::setAddressRange(VirtualAddress start, VirtualAddress end)
{
   mStartAddress = start;
   mEndAddress = end;
   mNumLines = static_cast<int>((static_cast<int64_t>(mEndAddress - mStartAddress) + 1) / mBytesPerLine);
   updateVerticalScrollBar();
   viewport()->update();
}

void
AddressTextDocumentWidget::navigateToAddress(VirtualAddress address)
{
   auto cursorAddress = cursorToAddress(getDocumentCursor());
   mNavigationForwardStack = { };
   if (mNavigationBackwardStack.empty() ||
       mNavigationBackwardStack.back() != cursorAddress) {
      mNavigationBackwardStack.push_back(cursorAddress);
   }
   showAddress(address);
}

void
AddressTextDocumentWidget::navigateBackward()
{
   if (!mNavigationBackwardStack.empty()) {
      auto address = mNavigationBackwardStack.back();
      auto cursorAddress = cursorToAddress(getDocumentCursor());
      mNavigationBackwardStack.pop_back();
      mNavigationForwardStack.push_back(cursorAddress);
      showAddress(address);
   }
}

void
AddressTextDocumentWidget::navigateForward()
{
   if (!mNavigationForwardStack.empty()) {
      auto address = mNavigationForwardStack.back();
      auto cursorAddress = cursorToAddress(getDocumentCursor());
      mNavigationForwardStack.pop_back();
      mNavigationBackwardStack.push_back(cursorAddress);
      showAddress(address);
   }
}

void
AddressTextDocumentWidget::paintEvent(QPaintEvent *e)
{
   auto palette = qApp->palette();
   auto painter = QPainter { viewport() };
   painter.setFont(font());

   if (mNumLines == 0) {
      painter.fillRect(e->rect(), palette.brush(QPalette::Base));
      return;
   }

   auto firstVisibleLine = verticalScrollBar()->value();
   auto lastVisibleLine = firstVisibleLine + verticalScrollBar()->pageStep() - 1;

   // Ensure text document is up to date
   updateTextDocument();

   // Setup text document painting
   auto textDocumentRect = viewport()->rect();
   auto textDocumentOffsetX = horizontalScrollBar()->value();
   painter.translate(-textDocumentOffsetX, 0.0);
   textDocumentRect.moveTo({ textDocumentOffsetX, 0 });

   auto paintContext = QAbstractTextDocumentLayout::PaintContext { };
   paintContext.clip = textDocumentRect;
   paintContext.selections = mHighlightedWordSelections;
   painter.setClipRect(textDocumentRect);

   // Process text selection
   auto selectionBegin = getDocumentSelectionBegin();
   auto selectionEnd = getDocumentSelectionEnd();

   if (selectionBegin != selectionEnd) {
      auto selectionFirst = std::min(selectionBegin, selectionEnd);
      auto selectionLast = std::max(selectionBegin, selectionEnd);

      auto selectionFirstLine = static_cast<int>((selectionFirst.address - mStartAddress) / mBytesPerLine);
      auto selectionLastLine = static_cast<int>((selectionLast.address - mStartAddress) / mBytesPerLine);

      if (selectionLastLine >= firstVisibleLine && lastVisibleLine >= selectionFirstLine) {
         QTextCursor selectionFirstCursor;
         QTextCursor selectionLastCursor;

         if (selectionFirstLine < firstVisibleLine) {
            selectionFirstCursor = QTextCursor { mTextDocument->firstBlock() };
            selectionFirstCursor.movePosition(QTextCursor::StartOfLine, QTextCursor::MoveAnchor);
         } else {
            selectionFirstCursor = QTextCursor { mTextDocument->findBlockByLineNumber(selectionFirstLine - firstVisibleLine) };
            selectionFirstCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, selectionFirst.cursorPosition);
         }

         if (selectionLastLine > lastVisibleLine) {
            selectionLastCursor = QTextCursor { mTextDocument->lastBlock() };
            selectionLastCursor.movePosition(QTextCursor::EndOfLine, QTextCursor::MoveAnchor);
         } else {
            selectionLastCursor = QTextCursor { mTextDocument->findBlockByLineNumber(selectionLastLine - firstVisibleLine) };
            selectionLastCursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, selectionLast.cursorPosition);
         }

         auto selection = std::move(selectionFirstCursor);
         selection.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor,
            selectionLastCursor.position() - selection.position());
         paintContext.selections.push_back({ selection, mTextFormatSelection });
      }
   }

   mTextDocument->documentLayout()->draw(&painter, paintContext);

   // Draw cursor on top if there is no selection
   if (mBlinkCursorVisible) {
      auto cursor = getDocumentCursor();
      auto cursorLine = static_cast<int>((cursor.address - mStartAddress) / mBytesPerLine);
      if (cursorLine >= firstVisibleLine && cursorLine <= lastVisibleLine) {
         auto cursorX = mDocumentMargin + cursor.cursorPosition * mCharacterWidth;
         auto cursorY = mDocumentMargin + (cursorLine - firstVisibleLine) * mLineHeight;
         painter.setCompositionMode(QPainter::CompositionMode_Difference);
         painter.setPen(QPen{ Qt::GlobalColor::white, 2 });
         painter.drawLine(cursorX, cursorY, cursorX, cursorY + mLineHeight);
      }
   }
}

void
AddressTextDocumentWidget::resizeEvent(QResizeEvent *e)
{
   updateHorizontalScrollBar();
   updateVerticalScrollBar();
   QAbstractScrollArea::resizeEvent(e);
}

void
AddressTextDocumentWidget::focusInEvent(QFocusEvent *e)
{
   QAbstractScrollArea::focusInEvent(e);
   mBlinkTimer->start();
   mBlinkCursorVisible = true;
   viewport()->update();
}

void
AddressTextDocumentWidget::focusOutEvent(QFocusEvent *e)
{
   QAbstractScrollArea::focusOutEvent(e);
   mBlinkTimer->stop();
   mBlinkCursorVisible = false;
   viewport()->update();
}

std::optional<AddressTextDocumentWidget::MouseHitTest>
AddressTextDocumentWidget::mouseEventHitTest(QMouseEvent *e)
{
   // Translate mouse position to document cursor
   auto documentLayout = mTextDocument->documentLayout();
   auto documentOffset = QPoint { horizontalScrollBar()->value(), 0 };
   auto cursorPos = documentLayout->hitTest(e->pos() + documentOffset, Qt::FuzzyHit);
   if (cursorPos < 0) {
      return { };
   }

   auto cursor = QTextCursor { mTextDocument };
   cursor.setPosition(cursorPos);

   // Translate mouse position to address
   auto firstVisibleLine = verticalScrollBar()->value();
   auto viewportLine = static_cast<int>(e->pos().y() - mDocumentMargin) / mLineHeight;
   if (viewportLine >= mNumLines - firstVisibleLine) {
      viewportLine = mNumLines - firstVisibleLine - 1;
   } else if (firstVisibleLine + viewportLine < 0) {
      viewportLine = -firstVisibleLine;
   }

   auto firstVisibleAddress = mStartAddress + static_cast<uint32_t>(firstVisibleLine) * mBytesPerLine;
   auto address = firstVisibleAddress + static_cast<uint32_t>(viewportLine * mBytesPerLine);
   return MouseHitTest { address, cursor };
}

void
AddressTextDocumentWidget::mousePressEvent(QMouseEvent *e)
{
   auto handled = false;

   if (e->buttons() & (Qt::LeftButton | Qt::RightButton)) {
      if (auto hit = mouseEventHitTest(e)) {
         auto cursor = DocumentCursor { };
         cursor.address = hit->lineAddress;
         cursor.cursorPosition = hit->textCursor.positionInBlock();

         // Update cursors
         setDocumentCursor(cursor);
         setDocumentSelectionBegin(cursor);
         setDocumentSelectionEnd(cursor);

         // Start cursor blink timer
         mBlinkTimer->start();
         mBlinkCursorVisible = true;

         // Update the currently highlighted word
         auto character = mTextDocument->characterAt(hit->textCursor.position());
         if (character.isSpace()) {
            mHighlightedWord.clear();
            updateHighlightedWord();
         } else {
            hit->textCursor.select(QTextCursor::WordUnderCursor);
            mHighlightedWord = hit->textCursor.selectedText();
            updateHighlightedWord();
         }

         viewport()->update();
         handled = true;
      }
   }

   if (handled) {
      e->accept();
   } else {
      QAbstractScrollArea::mousePressEvent(e);
   }
}

void
AddressTextDocumentWidget::mouseMoveEvent(QMouseEvent *e)
{
   auto handled = false;

   if (e->buttons() & Qt::LeftButton) {
      if (auto hit = mouseEventHitTest(e)) {
         auto cursor = DocumentCursor { };
         cursor.address = hit->lineAddress;
         cursor.cursorPosition = hit->textCursor.positionInBlock();

         // Update cursors
         setDocumentCursor(cursor);
         setDocumentSelectionEnd(cursor);

         // Start cursor blink timer
         mBlinkTimer->start();
         mBlinkCursorVisible = true;

         // Update the currently highlighted word
         auto character = mTextDocument->characterAt(hit->textCursor.position());
         if (character.isSpace()) {
            mHighlightedWord.clear();
            updateHighlightedWord();
         } else {
            hit->textCursor.select(QTextCursor::WordUnderCursor);
            mHighlightedWord = hit->textCursor.selectedText();
            updateHighlightedWord();
         }

         // Ensure cursor is visible
         // TODO: Use a QTimer to rate limit this auto scrolling
         auto line = static_cast<int>((cursor.address - mStartAddress) / mBytesPerLine);
         if (line > verticalScrollBar()->maximum()) {
            verticalScrollBar()->setValue(verticalScrollBar()->maximum());
         } else if (line < 0) {
            verticalScrollBar()->setValue(0);
         } else {
            auto firstLine = verticalScrollBar()->value();
            auto lastLine = firstLine + verticalScrollBar()->pageStep();
            if (line < firstLine) {
               verticalScrollBar()->setValue(line);
            } else if (line > lastLine) {
               verticalScrollBar()->setValue(line - verticalScrollBar()->pageStep() + 1);
            }
         }

         viewport()->update();
         handled = true;
      }
   }

   if (handled) {
      e->accept();
   } else {
      QAbstractScrollArea::mouseMoveEvent(e);
   }
}

void
AddressTextDocumentWidget::mouseReleaseEvent(QMouseEvent *e)
{
   auto handled = false;
   if (e->button() == Qt::BackButton) {
      navigateBackward();
      handled = true;
   } else if (e->button() == Qt::ForwardButton) {
      navigateForward();
      handled = true;
   }

   if (handled) {
      e->accept();
   } else {
      QAbstractScrollArea::mouseReleaseEvent(e);
   }
}

void
AddressTextDocumentWidget::mouseDoubleClickEvent(QMouseEvent *e)
{
   auto handled = false;
   if (e->buttons() & Qt::LeftButton) {
      if (auto hit = mouseEventHitTest(e)) {
         auto blockPosition = hit->textCursor.block().position();
         hit->textCursor.select(QTextCursor::WordUnderCursor);

         setDocumentSelectionBegin({
            hit->lineAddress,
            hit->textCursor.selectionStart() - blockPosition
         });
         setDocumentSelectionEnd({
            hit->lineAddress,
            hit->textCursor.selectionEnd() - blockPosition
         });
         setDocumentCursor({
            hit->lineAddress,
            hit->textCursor.selectionEnd() - blockPosition
         });

         viewport()->update();
         handled = true;
      }
   }

   if (handled) {
      e->accept();
   } else {
      QAbstractScrollArea::mouseDoubleClickEvent(e);
   }
}

void
AddressTextDocumentWidget::keyPressEvent(QKeyEvent *e)
{
   auto cursor = getDocumentCursor();
   auto cursorLine = static_cast<int>((cursor.address - mStartAddress) / mBytesPerLine);
   auto firstVisibleLine = verticalScrollBar()->value();
   auto lastVisibleLine = firstVisibleLine + verticalScrollBar()->pageStep() - 1;

   if (cursorLine < firstVisibleLine || cursorLine > lastVisibleLine) {
      ensureCursorVisible(false);
   }

   auto moveToEndOfNewCursorLine = false;
   auto moveAnchor = true;
   auto moveAddressOffset = 0;
   auto textCursor = QTextCursor { mTextDocument };
   auto textBlock = mTextDocument->findBlockByLineNumber(cursorLine - firstVisibleLine);
   auto endOfCurrentLine = textBlock.length();
   textCursor.setPosition(textBlock.position() + cursor.cursorPosition);

   if (e->matches(QKeySequence::MoveToPreviousChar) || e->matches(QKeySequence::SelectPreviousChar)) {
      if (cursor.cursorPosition == 0) {
         moveAddressOffset = -mBytesPerLine;
         moveToEndOfNewCursorLine = true;
      } else {
         cursor.cursorPosition -= 1;
      }

      moveAnchor = e->matches(QKeySequence::MoveToPreviousChar);
   } else if (e->matches(QKeySequence::MoveToNextChar) || e->matches(QKeySequence::SelectNextChar)) {
      if (cursor.cursorPosition >= endOfCurrentLine) {
         moveAddressOffset = mBytesPerLine;
         cursor.cursorPosition = 0;
      } else {
         cursor.cursorPosition += 1;
      }

      moveAnchor = e->matches(QKeySequence::MoveToNextChar);
   } else if (e->matches(QKeySequence::MoveToStartOfLine) || e->matches(QKeySequence::SelectStartOfLine)) {
      cursor.cursorPosition = 0;
      moveAnchor = e->matches(QKeySequence::MoveToStartOfLine);
   } else if (e->matches(QKeySequence::MoveToEndOfLine) || e->matches(QKeySequence::SelectEndOfLine)) {
      cursor.cursorPosition = endOfCurrentLine;
      moveAnchor = e->matches(QKeySequence::MoveToEndOfLine);
   } else if (e->matches(QKeySequence::MoveToPreviousLine) || e->matches(QKeySequence::SelectPreviousLine)) {
      moveAddressOffset = -mBytesPerLine;
      moveAnchor = e->matches(QKeySequence::MoveToPreviousLine);
   } else if (e->matches(QKeySequence::MoveToNextLine) || e->matches(QKeySequence::SelectNextLine)) {
      moveAddressOffset = mBytesPerLine;
      moveAnchor = e->matches(QKeySequence::MoveToNextLine);
   } else if (e->matches(QKeySequence::MoveToPreviousPage) || e->matches(QKeySequence::SelectPreviousPage)) {
      moveAddressOffset = verticalScrollBar()->pageStep() * -mBytesPerLine;
      moveAnchor = e->matches(QKeySequence::MoveToPreviousPage);
   } else if (e->matches(QKeySequence::MoveToNextPage) || e->matches(QKeySequence::SelectNextPage)) {
      moveAddressOffset = verticalScrollBar()->pageStep() * mBytesPerLine;
      moveAnchor = e->matches(QKeySequence::MoveToNextPage);
   } else if (e->matches(QKeySequence::MoveToStartOfDocument) || e->matches(QKeySequence::SelectStartOfDocument)) {
      cursor.address = mStartAddress;
      cursor.cursorPosition = 0;
      moveAnchor = e->matches(QKeySequence::MoveToStartOfDocument);
   } else if (e->matches(QKeySequence::MoveToEndOfDocument) || e->matches(QKeySequence::SelectEndOfDocument)) {
      cursor.address = mEndAddress;
      moveToEndOfNewCursorLine = true;
      moveAnchor = e->matches(QKeySequence::MoveToEndOfDocument);
   } else if (e->matches(QKeySequence::MoveToPreviousWord) || e->matches(QKeySequence::SelectPreviousWord)) {
      if (!textCursor.movePosition(QTextCursor::PreviousWord)) {
         cursor.cursorPosition = 0;
      } else {
         cursor.cursorPosition = textCursor.positionInBlock();
      }
      moveAnchor = e->matches(QKeySequence::MoveToPreviousWord);
   } else if (e->matches(QKeySequence::MoveToNextWord) || e->matches(QKeySequence::SelectNextWord)) {
      if (!textCursor.movePosition(QTextCursor::NextWord)) {
         cursor.cursorPosition = endOfCurrentLine;
      } else {
         cursor.cursorPosition = textCursor.positionInBlock();
      }
      moveAnchor = e->matches(QKeySequence::MoveToNextWord);
   } else if (e->matches(QKeySequence::SelectAll)) {
      setDocumentSelectionBegin({ mStartAddress, 0 });
      cursor.address = mEndAddress;
      moveToEndOfNewCursorLine = true;
      moveAnchor = false;
   } else if (e->matches(QKeySequence::Deselect)) {
      moveAnchor = true;
   } else if (e->matches(QKeySequence::Back)) {
      navigateBackward();
      e->accept();
      return;
   } else if (e->matches(QKeySequence::Forward)) {
      navigateForward();
      e->accept();
      return;
   } else {
      return QAbstractScrollArea::keyPressEvent(e);
   }

   if (moveAddressOffset) {
      if (moveAddressOffset < 0) {
         if (cursor.address - mStartAddress < static_cast<uint32_t>(-moveAddressOffset)) {
            cursor.address = mStartAddress;
            cursor.cursorPosition = 0;
         } else {
            cursor.address += moveAddressOffset;
         }
      } else {
         if (mEndAddress - cursor.address < static_cast<uint32_t>(moveAddressOffset)) {
            cursor.address = mEndAddress;
            moveToEndOfNewCursorLine = true;
         } else {
            cursor.address += moveAddressOffset;
         }
      }
   }

   if (moveAnchor) {
      setDocumentCursor(cursor);
      setDocumentSelectionBegin(cursor);
      setDocumentSelectionEnd(cursor);
   } else {
      setDocumentCursor(cursor);
      setDocumentSelectionEnd(cursor);
   }

   ensureCursorVisible(false);

   if (moveToEndOfNewCursorLine) {
      cursorLine = static_cast<int>((cursor.address - mStartAddress) / mBytesPerLine);
      firstVisibleLine = verticalScrollBar()->value();
      textBlock = mTextDocument->findBlockByLineNumber(cursorLine - firstVisibleLine);
      cursor.cursorPosition = textBlock.length();
      setDocumentCursor(cursor);
   }

   mBlinkTimer->start();
   mBlinkCursorVisible = true;
   viewport()->update();
   e->accept();
}

void
AddressTextDocumentWidget::showAddress(VirtualAddress address)
{
   setDocumentCursor(cursorFromAddress(address));
   setDocumentSelectionBegin({});
   setDocumentSelectionEnd({});

   mHighlightedWord = QString { "%1" }.arg(address, 8, 16, QLatin1Char{ '0' });
   if (!ensureCursorVisible(true)) {
      updateHighlightedWord();
      viewport()->update();
   }
}

bool
AddressTextDocumentWidget::ensureCursorVisible(bool centerOnCursor)
{
   auto cursor = getDocumentCursor();
   auto address = cursor.address;
   auto targetLine = static_cast<int>((address - mStartAddress) / mBytesPerLine);
   auto firstVisibleLine = verticalScrollBar()->value();
   auto lastVisibleLine = firstVisibleLine + verticalScrollBar()->pageStep() - 1;

   if (targetLine >= firstVisibleLine && targetLine <= lastVisibleLine) {
      // Already visible
      return false;
   }

   // Scroll so that the target line is 1/3rd of way down screen
   if (centerOnCursor) {
      targetLine -= verticalScrollBar()->pageStep() / 3;
   } else if (targetLine > lastVisibleLine) {
      targetLine -= verticalScrollBar()->pageStep() - 1;
   }

   if (targetLine < 0) {
      verticalScrollBar()->setValue(0);
   } else if (targetLine > verticalScrollBar()->maximum()) {
      verticalScrollBar()->setValue(verticalScrollBar()->maximum());
   } else {
      verticalScrollBar()->setValue(targetLine);
   }

   updateTextDocument();
   viewport()->update();
   return true;
}

void
AddressTextDocumentWidget::updateHorizontalScrollBar()
{
   auto viewportWidth = viewport()->width();
   if (mMaxDocumentWidth < viewportWidth) {
      horizontalScrollBar()->setMinimum(0);
      horizontalScrollBar()->setMaximum(0);
      horizontalScrollBar()->setValue(0);
   } else {
      horizontalScrollBar()->setMinimum(0);
      horizontalScrollBar()->setMaximum(mMaxDocumentWidth - viewportWidth);
      horizontalScrollBar()->setSingleStep(mCharacterWidth);
      horizontalScrollBar()->setPageStep(viewportWidth);
   }
}

void
AddressTextDocumentWidget::updateVerticalScrollBar()
{
   if (mNumLines == 0) {
      verticalScrollBar()->setMinimum(0);
      verticalScrollBar()->setMaximum(0);
      verticalScrollBar()->setValue(0);
   } else {
      auto margin = static_cast<int>(mDocumentMargin * 2);
      auto pageLines = (viewport()->height() - margin) / mLineHeight;
      verticalScrollBar()->setMinimum(0);
      verticalScrollBar()->setMaximum(mNumLines - pageLines);
      verticalScrollBar()->setSingleStep(1);
      verticalScrollBar()->setPageStep(pageLines);
   }
}

void
AddressTextDocumentWidget::updateHighlightedWord()
{
   mHighlightedWordSelections.clear();

   auto findCursor = QTextCursor { mTextDocument };
   while (!findCursor.isNull() && !findCursor.atEnd()) {
      findCursor = mTextDocument->find(mHighlightedWord, findCursor, QTextDocument::FindWholeWords);
      if (!findCursor.isNull()) {
         auto selection = QAbstractTextDocumentLayout::Selection { };
         selection.cursor = findCursor;
         selection.format = mTextFormatHighlightedWord;
         mHighlightedWordSelections.push_back(selection);
      }
   }
}

void
AddressTextDocumentWidget::updateTextDocument()
{
   // Check if text document is representing current view
   auto firstVisibleLine = verticalScrollBar()->value();
   auto lastVisibleLine = firstVisibleLine + verticalScrollBar()->pageStep() - 1;
   auto firstVisibleLineAddress = static_cast<VirtualAddress>(mStartAddress + (firstVisibleLine * mBytesPerLine));
   auto lastVisibleLineAddress = static_cast<VirtualAddress>(mStartAddress + (lastVisibleLine * mBytesPerLine));

   if (firstVisibleLineAddress == mTextDocumentFirstLineAddress &&
       lastVisibleLineAddress == mTextDocumentLastLineAddress) {
      return;
   }

   // Generate new text document
   mTextDocument->clear();

   auto cursor = QTextCursor { mTextDocument };
   cursor.beginEditBlock();
   updateTextDocument(cursor, firstVisibleLineAddress, lastVisibleLineAddress, mBytesPerLine);
   cursor.endEditBlock();

   // Update horizontal scroll
   mMaxDocumentWidth = std::max(mMaxDocumentWidth, mTextDocument->documentLayout()->documentSize().width());
   updateHorizontalScrollBar();

   // Search text for highlighted word
   updateHighlightedWord();
}
