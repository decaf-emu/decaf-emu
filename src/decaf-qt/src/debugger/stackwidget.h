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

protected:
   void paintEvent(QPaintEvent *e) override;

   void updateStackFrames();
   void updateTextDocument(QTextCursor cursor, VirtualAddress firstLineAddress,
                           VirtualAddress lastLineAddress,
                           int bytePerLine) override;

private slots:
   void activeThreadChanged();
   void dataChanged();

private:
   DebugData *mDebugData;

   VirtualAddress mStackCurrentAddress;

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
      QTextCharFormat activeLineAddress;
      QTextCharFormat data;
      QTextCharFormat symbolName;
      QTextCharFormat referencedAscii;
      QColor stackOutline;
      QColor backchainOutline;
   } mTextFormats;
};
