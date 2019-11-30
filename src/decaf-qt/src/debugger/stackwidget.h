#pragma once
#include "addresstextdocumentwidget.h"
#include "debugdata.h"

#include <map>
#include <QString>
#include <QTextCharFormat>

class QTextDocument;

class StackWidget : public AddressTextDocumentWidget
{
   Q_OBJECT

   using VirtualAddress = DebugData::VirtualAddress;

   struct StackFrame
   {
      VirtualAddress start;
      VirtualAddress end;
   };

public:
   StackWidget(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

public slots:
   void navigateOperand();

signals:
   void navigateToTextAddress(uint32_t address);

protected:
   void paintEvent(QPaintEvent *e) override;
   void keyPressEvent(QKeyEvent *e) override;

   QVector<QAbstractTextDocumentLayout::Selection>
   getCustomSelections(QTextDocument *document) override;
   void updateStackFrames();
   void updateTextDocument(QTextCursor cursor, VirtualAddress firstLineAddress,
                           VirtualAddress lastLineAddress,
                           int bytePerLine,
                           bool forDisplay) override;

private slots:
   void activeThreadChanged();
   void dataChanged();

private:
   DebugData *mDebugData;

   VirtualAddress mStackCurrentAddress;
   VirtualAddress mStackFirstVisibleAddress;
   VirtualAddress mStackLastVisibleAddress;
   QVector<QAbstractTextDocumentLayout::Selection> mCustomSelectionsBuffer;

   // Cached stack frames
   std::map<VirtualAddress, StackFrame> mStackFrames;

   // Visible columns in disassembly
   struct {
      bool lineAddress = true;
      bool outline = true;
      bool data = true;
      bool dataReference = true;
   } mVisibleColumns;

   // Punctuation between columns
   struct {
      QString afterLineAddress = "  ";
      QString outline = "  ";
      QString afterData = "  ";
   } mPunctuation;

   // Formatting for each data type
   struct {
      QTextCharFormat invalid;
      QTextCharFormat punctuation;
      QTextCharFormat lineAddress;
      QTextCharFormat currentLine;
      QTextCharFormat data;
      QTextCharFormat symbolName;
      QTextCharFormat referencedAscii;
      QColor stackOutline;
      QColor backchainOutline;
   } mTextFormats;
};
