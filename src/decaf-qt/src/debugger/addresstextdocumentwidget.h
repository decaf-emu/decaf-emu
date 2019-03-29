#pragma once
#include <QAbstractScrollArea>
#include <QAbstractTextDocumentLayout>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QVector>
#include <optional>

#include <libdecaf/decaf_debug_api.h>

class QTextDocument;

class AddressTextDocumentWidget : public QAbstractScrollArea
{
protected:
   using VirtualAddress = decaf::debug::VirtualAddress;

   struct DocumentCursor
   {
      VirtualAddress address;
      int cursorPosition;

      bool operator <(const DocumentCursor &rhs) const
      {
         if (address < rhs.address) {
            return true;
         } else if (address > rhs.address) {
            return false;
         }

         return cursorPosition < rhs.cursorPosition;
      }

      bool operator !=(const DocumentCursor &rhs) const
      {
         return address != rhs.address || cursorPosition != rhs.cursorPosition;
      }
   };

   struct MouseHitTest
   {
      VirtualAddress lineAddress;
      QTextCursor textCursor;
   };

public:
   AddressTextDocumentWidget(QWidget *parent = nullptr);

   void setAddressRange(VirtualAddress start, VirtualAddress end);
   void setBytesPerLine(int bytesPerLine);

   int getBytesPerLine() { return mBytesPerLine; };
   VirtualAddress getStartAddress() { return mStartAddress; }
   VirtualAddress getEndAddress() { return mEndAddress; }

   void navigateToAddress(VirtualAddress address);
   void navigateBackward();
   void navigateForward();

protected:
   std::optional<MouseHitTest> mouseEventHitTest(QMouseEvent *e);
   int characterWidth() { return mCharacterWidth; }
   int documentMargin() { return mDocumentMargin; }
   int lineHeight() { return mLineHeight; }
   void updateTextDocument();

protected:
   void paintEvent(QPaintEvent *e) override;
   void resizeEvent(QResizeEvent *) override;

   void focusInEvent(QFocusEvent *e) override;
   void focusOutEvent(QFocusEvent *e) override;

   void mouseMoveEvent(QMouseEvent *e) override;
   void mousePressEvent(QMouseEvent *e) override;
   void mouseReleaseEvent(QMouseEvent *e) override;
   void mouseDoubleClickEvent(QMouseEvent *e) override;

   void keyPressEvent(QKeyEvent *e) override;

   virtual void updateTextDocument(QTextCursor cursor, VirtualAddress firstLineAddress, VirtualAddress lastLineAddress, int bytePerLine) = 0;


   // We provide a default implementation for cursor / selection tracking which
   // works perfectly fine as long as bytesPerLine does not change.

   virtual DocumentCursor cursorFromAddress(VirtualAddress address)
   {
      return DocumentCursor { address, 0 };
   }

   virtual VirtualAddress cursorToAddress(DocumentCursor cursor)
   {
      return cursor.address;
   }

   virtual DocumentCursor getDocumentCursor()
   {
      return mDefaultCursor;
   }

   virtual DocumentCursor getDocumentSelectionBegin()
   {
      return mDefaultSelectionBegin;
   }

   virtual DocumentCursor getDocumentSelectionEnd()
   {
      return mDefaultSelectionEnd;
   }

   virtual void setDocumentCursor(DocumentCursor cursor)
   {
      mDefaultCursor = cursor;
   }

   virtual void setDocumentSelectionBegin(DocumentCursor cursor)
   {
      mDefaultSelectionBegin = cursor;
   }

   virtual void setDocumentSelectionEnd(DocumentCursor cursor)
   {
      mDefaultSelectionEnd = cursor;
   }

private:
   void showAddress(VirtualAddress address);
   bool ensureCursorVisible(bool centerOnCursor);
   void updateHighlightedWord();
   void updateHorizontalScrollBar();
   void updateVerticalScrollBar();

private:
   int mLineHeight = 16;
   int mCharacterWidth = 16;
   int mBytesPerLine = 16;
   int mDocumentMargin = 0;
   int mNumLines = 0;

   // Address range
   VirtualAddress mStartAddress = 0;
   VirtualAddress mEndAddress = 0;

   // Text document
   QTextDocument *mTextDocument = nullptr;
   VirtualAddress mTextDocumentFirstLineAddress = 0;
   VirtualAddress mTextDocumentLastLineAddress = 0;
   qreal mMaxDocumentWidth = 0.0;

   // Text formats
   QTextCharFormat mTextFormatSelection;
   QTextCharFormat mTextFormatHighlightedWord;

   // Cursor
   bool mBlinkCursorVisible = false;
   QTimer *mBlinkTimer = nullptr;

   // Highlighted word
   QString mHighlightedWord;
   QVector<QAbstractTextDocumentLayout::Selection> mHighlightedWordSelections;

   // Navigation
   int mNavigationHistoryIndex = 0;
   size_t mNavigationHistoryMaxSize = 128;
   std::vector<VirtualAddress> mNavigationBackwardStack;
   std::vector<VirtualAddress> mNavigationForwardStack;

   // Default cursor implementation
   DocumentCursor mDefaultCursor = { };
   DocumentCursor mDefaultSelectionBegin = { };
   DocumentCursor mDefaultSelectionEnd = { };
};
