#include "memorywidget.h"

#include <cctype>
#include <libdecaf/decaf_debug_api.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QMenu>

MemoryWidget::MemoryWidget(QWidget *parent) :
   AddressTextDocumentWidget(parent)
{
   setBytesPerLine(16);
   setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
   setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

   mTextFormats.lineAddress = QTextCharFormat { };
   mTextFormats.lineAddress.setForeground(Qt::darkGray);

   mTextFormats.hexData = QTextCharFormat { };
   mTextFormats.hexData.setForeground(QColor { 0, 0x80, 0x40 });

   mTextFormats.textData = QTextCharFormat { };
   mTextFormats.textData.setForeground(Qt::blue);

   mTextFormats.punctuation = QTextCharFormat { };
   mTextFormats.punctuation.setForeground(Qt::darkBlue);

   mCopyAction = new QAction(tr("&Copy"), this);
   mCopyHexAction = new QAction(tr("Copy data as hex"), this);
   mCopyTextAction = new QAction(tr("Copy data as text"), this);
   connect(mCopyAction, &QAction::triggered, this, &MemoryWidget::copySelection);
   connect(mCopyHexAction, &QAction::triggered, this, &MemoryWidget::copySelectionAsHex);
   connect(mCopyTextAction, &QAction::triggered, this, &MemoryWidget::copySelectionAsText);
}

void
MemoryWidget::setBytesPerLine(int bytesPerLine)
{
   mAutoBytesPerLine = false;
   AddressTextDocumentWidget::setBytesPerLine(bytesPerLine);
}

void
MemoryWidget::setAutoBytesPerLine(bool enabled)
{
   mAutoBytesPerLine = enabled;
   updateAutoBytesPerLine();
}

bool
MemoryWidget::autoBytesPerLine()
{
   return mAutoBytesPerLine;
}

void
MemoryWidget::updateAutoBytesPerLine()
{
   auto maxWidth = viewport()->width() - documentMargin() * 2;
   auto charactersPerByte = 0;
   auto charactersTotalOffset = 0;
   auto charWidth = characterWidth();

   if (mVisibleColumns.lineAddress) {
      maxWidth -= 8 * charWidth;
      maxWidth -= mPunctuation.afterLineAddress.size() * charWidth;
   }

   if (mVisibleColumns.hexData) {
      charactersTotalOffset -= 1;
      charactersPerByte += 3;
      maxWidth -= mPunctuation.afterHexData.size() * charWidth;
   }

   if (mVisibleColumns.textData) {
      charactersPerByte++;
   }

   auto maxCharacters = maxWidth / characterWidth();
   auto bytesPerLine = (maxCharacters - charactersTotalOffset) / charactersPerByte;
   AddressTextDocumentWidget::setBytesPerLine(std::max(4, bytesPerLine));
}

void
MemoryWidget::resizeEvent(QResizeEvent *e)
{
   if (mAutoBytesPerLine) {
      updateAutoBytesPerLine();
   }

   AddressTextDocumentWidget::resizeEvent(e);
}

void
MemoryWidget::copySelectionAsHex()
{
   if (mSelectionBegin != mSelectionEnd) {
      auto startAddress = std::min(mSelectionBegin.address, mSelectionEnd.address);
      auto endAddress = std::max(mSelectionBegin.address, mSelectionEnd.address);
      auto count = endAddress - startAddress;

      // Limit copy to 4 kb
      if (count <= 4 * 1024) {
         auto buffer = QByteArray { };
         buffer.resize(count);
         buffer.resize(static_cast<int>(
            decaf::debug::readMemory(startAddress, buffer.data(),
               buffer.size())));
         qApp->clipboard()->setText(QString { buffer.toHex() });
      }
   }
}

void
MemoryWidget::copySelectionAsText()
{
   if (mSelectionBegin != mSelectionEnd) {
      auto startAddress = std::min(mSelectionBegin.address, mSelectionEnd.address);
      auto endAddress = std::max(mSelectionBegin.address, mSelectionEnd.address);
      auto count = endAddress - startAddress;

      // Limit copy to 4 kb
      if (count <= 4 * 1024) {
         auto buffer = QByteArray { };
         buffer.resize(count);
         buffer.resize(static_cast<int>(
            decaf::debug::readMemory(startAddress, buffer.data(),
               buffer.size())));

         auto textString = QString { };
         for (auto c : buffer) {
            if (std::isprint(c)) {
               textString += c;
            } else {
               textString += '?';
            }
         }

         qApp->clipboard()->setText(textString);
      }
   }
}

void
MemoryWidget::keyPressEvent(QKeyEvent *e)
{
   return AddressTextDocumentWidget::keyPressEvent(e);
}

void
MemoryWidget::showContextMenu(QMouseEvent *e)
{
   auto menu = QMenu { this };
   menu.addAction(mCopyAction);
   menu.addAction(mCopyHexAction);
   menu.addAction(mCopyTextAction);
   menu.exec(e->globalPos());
}

MemoryWidget::MemoryCursor
MemoryWidget::convertCursor(const DocumentCursor documentCursor)
{
   auto memoryCursor = MemoryCursor { };
   auto bytesPerLine = getBytesPerLine();
   auto endAddress = getEndAddress();

   // Convert document cursor into a non-document dependent memory cursor
   if (mVisibleColumns.hexData) {
      if (documentCursor.cursorPosition < mHexDataStartPosition) {
         memoryCursor.address = documentCursor.address;
         memoryCursor.type = MemoryCursor::Hex;
         memoryCursor.nibble = 0;
      } else if (documentCursor.cursorPosition >= mHexDataStartPosition &&
                 documentCursor.cursorPosition < mHexDataEndPosition) {
         auto byte = (documentCursor.cursorPosition - mHexDataStartPosition) / 3;
         if (byte >= bytesPerLine) {
            byte = bytesPerLine - 1;
            memoryCursor.nibble = 2;
         } else {
            memoryCursor.nibble = (documentCursor.cursorPosition - mHexDataStartPosition) % 3;
         }

         memoryCursor.address = documentCursor.address + byte;
         memoryCursor.type = MemoryCursor::Hex;
      } else if (documentCursor.cursorPosition >= mHexDataEndPosition &&
                 documentCursor.cursorPosition < mTextDataStartPosition) {
         memoryCursor.address = documentCursor.address + (bytesPerLine - 1);
         memoryCursor.type = MemoryCursor::Hex;
         memoryCursor.nibble = 2;
      }
   }

   if (mVisibleColumns.textData) {
      if (documentCursor.cursorPosition >= mTextDataStartPosition) {
         auto byte = documentCursor.cursorPosition - mTextDataStartPosition;
         if (byte >= bytesPerLine) {
            byte = bytesPerLine - 1;
            memoryCursor.nibble = 2;
         }

         if (static_cast<int64_t>(endAddress) - static_cast<int64_t>(documentCursor.address) < byte) {
            memoryCursor.address = endAddress;
            memoryCursor.nibble = 2;
         } else {
            memoryCursor.address = documentCursor.address + byte;
         }

         memoryCursor.type = MemoryCursor::Text;
      }
   }

   return memoryCursor;
}

MemoryWidget::DocumentCursor
MemoryWidget::convertCursor(const MemoryWidget::MemoryCursor memoryCursor)
{
   auto documentCursor = DocumentCursor { };
   auto bytesPerLine = getBytesPerLine();
   auto startAddress = getStartAddress();
   auto lineStartAddress = startAddress + ((memoryCursor.address - startAddress) / bytesPerLine) * bytesPerLine;
   auto lineByte = memoryCursor.address - lineStartAddress;
   documentCursor.address = lineStartAddress;

   if (memoryCursor.type == MemoryCursor::Hex) {
      documentCursor.cursorPosition = mHexDataStartPosition + 3 * lineByte + memoryCursor.nibble;
   } else if (memoryCursor.type == MemoryCursor::Text) {
      documentCursor.cursorPosition = mTextDataStartPosition + lineByte;
      if (memoryCursor.nibble == 2) {
         documentCursor.cursorPosition++;
      }
   }

   return documentCursor;
}

MemoryWidget::MemoryCursor
MemoryWidget::moveMemoryCursor(MemoryCursor memoryCursor, int byteOffset)
{
   auto startAddress = getStartAddress();
   auto endAddress = getEndAddress();

   if (byteOffset < 0 && mCursor.address < startAddress - byteOffset) {
      memoryCursor.address = startAddress;
      memoryCursor.nibble = 0;
   } else if (byteOffset > 0 && mCursor.address > endAddress - byteOffset) {
      memoryCursor.address = endAddress;
      memoryCursor.nibble = 2;
   } else {
      memoryCursor.address += byteOffset;
   }

   return memoryCursor;
}

MemoryWidget::DocumentCursor
MemoryWidget::cursorFromAddress(VirtualAddress address)
{
   auto memoryCursor = MemoryCursor { };
   if (mVisibleColumns.hexData) {
      memoryCursor.type = MemoryCursor::Hex;
   } else if (mVisibleColumns.textData) {
      memoryCursor.type = MemoryCursor::Text;
   }

   memoryCursor.address = address;
   return convertCursor(memoryCursor);
}

MemoryWidget::VirtualAddress
MemoryWidget::cursorToAddress(DocumentCursor cursor)
{
   return convertCursor(cursor).address;
}

MemoryWidget::DocumentCursor
MemoryWidget::getDocumentCursor()
{
   return convertCursor(mCursor);
}

MemoryWidget::DocumentCursor
MemoryWidget::getDocumentSelectionBegin()
{
   return convertCursor(mSelectionBegin);
}

MemoryWidget::DocumentCursor
MemoryWidget::getDocumentSelectionEnd()
{
   return convertCursor(mSelectionEnd);
}

void
MemoryWidget::setDocumentCursor(DocumentCursor cursor)
{
   mCursor = convertCursor(cursor);
}

void
MemoryWidget::setDocumentSelectionBegin(DocumentCursor cursor)
{
   mSelectionBegin = convertCursor(cursor);
}

void
MemoryWidget::setDocumentSelectionEnd(DocumentCursor cursor)
{
   mSelectionEnd = convertCursor(cursor);
}

static constexpr inline QChar
toHexUpper(uint value)
{
   return "0123456789ABCDEF"[value & 0xF];
}

void
MemoryWidget::updateTextDocument(QTextCursor cursor,
                                  VirtualAddress firstLineAddress,
                                  VirtualAddress lastLineAddress,
                                  int bytesPerLine,
                                  bool forDisplay)
{
   // TODO: Cache line cursor position for Address, Hex, Text
   auto buffer = std::vector<uint8_t> { };
   buffer.resize(bytesPerLine, 0);

   auto hexString = QString { };
   hexString.resize(bytesPerLine * 3 - 1, ' ');

   auto textString = QString { };
   textString.resize(bytesPerLine, '.');

   auto pageSize = decaf::debug::getMemoryPageSize();
   auto pageMask = ~(pageSize - 1);

   auto beginAddress = static_cast<int64_t>(firstLineAddress) ;
   auto endAddress = static_cast<int64_t>(lastLineAddress) + bytesPerLine;
   auto address = beginAddress;

   auto currentPage = beginAddress & pageMask;
   auto nextPage = currentPage + pageSize;
   auto currentPageValid = decaf::debug::isValidVirtualAddress(static_cast<VirtualAddress>(currentPage));
   auto nextPageValid = decaf::debug::isValidVirtualAddress(static_cast<VirtualAddress>(nextPage));

   // This code assumes you could only ever have 2 pages on a single line
   assert(pageSize > bytesPerLine);

   for (auto address = beginAddress; address < endAddress; address += bytesPerLine) {
      auto firstValidByte = -1;
      auto lastValidByte = -1;
      auto lineCrossesPage = (nextPage - address) < bytesPerLine;

      if (address != beginAddress) {
         cursor.insertBlock();
      }

      if (mVisibleColumns.lineAddress) {
         mAddressStartPosition = cursor.positionInBlock();
         cursor.insertText(
            QString { "%1" }.arg(address, 8, 16, QLatin1Char { '0' }),
            mTextFormats.lineAddress);
         mAddressEndPosition = cursor.positionInBlock();
         cursor.insertText(mPunctuation.afterLineAddress, mTextFormats.punctuation);
      }

      if (currentPageValid) {
         // Current page is valid, so first byte is valid
         firstValidByte = 0;
      } else if (lineCrossesPage && nextPageValid) {
         // Current page invalid, next page valid
         firstValidByte = static_cast<int>(nextPage - address);
      }

      if (!lineCrossesPage) {
         if (currentPageValid) {
            // Whole line is on current valid page
            lastValidByte = bytesPerLine;
         }
      } else {
         if (nextPageValid) {
            // Next page is valid so assume we can finish the current line
            lastValidByte = bytesPerLine;
         } else if (currentPageValid) {
            // Current page is valid, next page is invalid
            lastValidByte = std::min(static_cast<int>(nextPage - address), bytesPerLine);
         }
      }

      if (lineCrossesPage) {
         currentPage = nextPage;
         currentPageValid = nextPageValid;

         nextPage = currentPage + pageSize;
         nextPageValid = decaf::debug::isValidVirtualAddress(static_cast<VirtualAddress>(nextPage));
      }

      if (firstValidByte < 0 || lastValidByte < 0) {
         // Whole line is invalid
         hexString.fill(' ');
         textString.fill(' ');
      } else {
         decaf::debug::readMemory(address + firstValidByte, buffer.data() + firstValidByte, lastValidByte - firstValidByte);

         for (auto i = 0; i < firstValidByte; ++i) {
            hexString[i * 3 + 0] = ' ';
            hexString[i * 3 + 1] = ' ';
            textString[i] = ' ';
         }

         for (auto i = firstValidByte; i < lastValidByte; ++i) {
            hexString[i * 3 + 0] = toHexUpper(buffer[i] >> 4);
            hexString[i * 3 + 1] = toHexUpper(buffer[i] & 0xf);

            if (std::isprint(buffer[i])) {
               textString[i] = QChar { buffer[i] };
            } else {
               textString[i] = '.';
            }
         }

         for (auto i = lastValidByte; i < bytesPerLine; ++i) {
            hexString[i * 3 + 0] = ' ';
            hexString[i * 3 + 1] = ' ';
            textString[i] = ' ';
         }
      }

      if (mVisibleColumns.hexData) {
         mHexDataStartPosition = cursor.positionInBlock();
         cursor.insertText(hexString, mTextFormats.hexData);
         mHexDataEndPosition = cursor.positionInBlock();
         cursor.insertText(mPunctuation.afterLineAddress, mTextFormats.punctuation);
      }

      if (mVisibleColumns.textData) {
         mTextDataStartPosition = cursor.positionInBlock();
         cursor.insertText(textString, mTextFormats.textData);
         mTextDataEndPosition = cursor.positionInBlock();
      }
   }
}
