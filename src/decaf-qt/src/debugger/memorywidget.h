#pragma once
#include "addresstextdocumentwidget.h"
#include <QAbstractScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QTextCursor>
#include <QTextDocument>

#include <cstddef>
#include <vector>
#include <optional>

class QTextDocument;

class MemoryWidget : public AddressTextDocumentWidget
{
   using VirtualAddress = uint32_t;

   struct MemoryCursor
   {
      enum Type
      {
         Invalid,
         Hex,
         Text,
      };

      Type type = Invalid;
      VirtualAddress address = 0;
      int nibble = 0;

      bool operator !=(const MemoryCursor &rhs) const
      {
         return type != rhs.type || address != rhs.address || nibble != rhs.nibble;
      }

      bool operator <(const MemoryCursor &rhs) const
      {
         if (address > rhs.address) {
            return false;
         } else if (address < rhs.address) {
            return true;
         } else {
            return nibble < rhs.nibble;
         }
      }
   };

public:
   MemoryWidget(QWidget *parent = nullptr);

   void setBytesPerLine(int bytesPerLine);

   void setAutoBytesPerLine(bool enabled);
   bool autoBytesPerLine();

private:
   void updateAutoBytesPerLine();

   void resizeEvent(QResizeEvent *e) override;
   void keyPressEvent(QKeyEvent *e) override;

   MemoryCursor convertCursor(const DocumentCursor documentCursor);
   DocumentCursor convertCursor(const MemoryCursor memoryCursor);
   MemoryCursor moveMemoryCursor(MemoryCursor memoryCursor, int byteOffset);

   DocumentCursor cursorFromAddress(VirtualAddress address) override;
   VirtualAddress cursorToAddress(DocumentCursor cursor) override;
   DocumentCursor getDocumentCursor() override;
   DocumentCursor getDocumentSelectionBegin() override;
   DocumentCursor getDocumentSelectionEnd() override;
   void setDocumentCursor(DocumentCursor cursor) override;
   void setDocumentSelectionBegin(DocumentCursor cursor) override;
   void setDocumentSelectionEnd(DocumentCursor cursor) override;

   void updateTextDocument(QTextCursor cursor, VirtualAddress startAddress, VirtualAddress endAddress, int bytesPerLine) override;

private:
   bool mAutoBytesPerLine = false;

   MemoryCursor mCursor = { };
   MemoryCursor mSelectionBegin = { };
   MemoryCursor mSelectionEnd = { };

   int mAddressStartPosition = 0;
   int mAddressEndPosition = 0;

   int mHexDataStartPosition = 0;
   int mHexDataEndPosition = 0;

   int mTextDataStartPosition = 0;
   int mTextDataEndPosition = 0;

   struct {
      QTextCharFormat lineAddress;
      QTextCharFormat punctuation;
      QTextCharFormat hexData;
      QTextCharFormat textData;
   } mTextFormats;

   struct {
      QString afterLineAddress = "  ";
      QString afterHexData = "  ";
   } mPunctuation;

   struct {
      bool lineAddress = true;
      bool hexData = true;
      bool textData = true;
   } mVisibleColumns;
};
