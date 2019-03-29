#pragma once
#include "addresstextdocumentwidget.h"
#include "debugdata.h"

#include <libcpu/espresso/espresso_disassembler.h>

#include <array>
#include <vector>
#include <QColor>
#include <QPainterPath>
#include <QStack>
#include <QString>
#include <QTextCharFormat>

class QTextDocument;

class DisassemblyWidget : public AddressTextDocumentWidget
{
   Q_OBJECT

   using VirtualAddress = DebugData::VirtualAddress;

public:
   DisassemblyWidget(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

public slots:
   void followSymbolUnderCursor();
   void toggleBreakpointUnderCursor();

protected:
   void followSymbolAtCursor(DocumentCursor cursor);

   void paintEvent(QPaintEvent *e) override;
   void mouseReleaseEvent(QMouseEvent *e) override;
   void keyPressEvent(QKeyEvent *e) override;

   void updateTextDocument(QTextCursor cursor, VirtualAddress firstLineAddress,
                           VirtualAddress lastLineAddress,
                           int bytePerLine) override;

private:
   DebugData *mDebugData;

   VirtualAddress mCacheStartAddress;

   // Cached disassembly information of current visible instructions
   struct DisassemblyCacheItem
   {
      bool valid = false;
      VirtualAddress address;
      std::array<uint8_t, 4> data;
      espresso::Disassembly disassembly;
      DebugData::AnalyseDatabase::Lookup addressLookup;
      const DebugData::AnalyseDatabase::Function *referenceLookup;
   };
   std::vector<DisassemblyCacheItem> mDisassemblyCache;

   // Cache of the cursor positions of current visible instructions
   struct TextCursorPositionCache
   {
      std::pair<int, int> lineAddress;
      std::pair<int, int> instructionData;
      std::pair<int, int> instructionName;
      std::vector<std::pair<int, int>> instructionArgs;
      std::pair<int, int> referencedSymbol;
      std::pair<int, int> commentFunctionName;
   };
   std::vector<TextCursorPositionCache> mTextCursorPositionCache;

   // Visible columns in disassembly
   struct {
      bool lineAddress = true;
      bool functionOutline = true;
      bool instructionData = true;
      bool branchOutline = true;
      bool instructionName = true;
      bool instructionArgs = true;
      bool referencedSymbol = true;
      bool functionName = true;
   } mVisibleColumns;

   // Punctuation between columns
   struct {
      QString afterLineAddress = "  ";
      QString functionOutline = "  ";
      QString afterInstructionData = "  ";
      QString branchOutline = "  ";
      int instructionNameWidth = 12;
      int instructionArgsWidth = 32;
      QString afterReferencedSymbol = "  ";
      QString beforeComment = "# ";
   } mPunctuation;

   // Formatting for each data type
   struct {
      QTextCharFormat breakpoint;
      QTextCharFormat lineAddress;
      QTextCharFormat instructionData;
      QTextCharFormat instructionName;
      QTextCharFormat registerName;
      QTextCharFormat punctuation;
      QTextCharFormat branchAddress;
      QTextCharFormat symbolName;
      QTextCharFormat numericValue;
      QTextCharFormat invalid;
      QTextCharFormat comment;
      QColor functionOutline;
      QColor branchOutlineTrue;
      QColor branchOutlineFalse;
      QColor branchDirectionArrow;
      QPainterPath branchUpArrowPath;
      QPainterPath branchDownArrowPath;
   } mTextFormats;
};
