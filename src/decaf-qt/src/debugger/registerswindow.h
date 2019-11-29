#pragma once
#include <array>
#include <cstdint>
#include <QWidget>
#include <QPlainTextEdit>
#include <QTextBlock>

#include "debugdata.h"

class RegistersWindow : public QPlainTextEdit
{
   Q_OBJECT

   struct RegisterCursor
   {
      QTextBlock block;
      int start;
      int end;
   };

public:
   RegistersWindow(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

private slots:
   void debugDataChanged();

private:
   void generateDocument();

private:
   DebugData *mDebugData = nullptr;

   decaf::debug::CafeThread mLastThreadState = { };

   struct RegisterCursors
   {
      std::array<RegisterCursor, 32> gpr;
      RegisterCursor lr;
      RegisterCursor ctr;
      RegisterCursor xer;
      RegisterCursor msr;
      std::array<RegisterCursor, 8> crf;
      std::array<RegisterCursor, 32> fpr;
      std::array<RegisterCursor, 32> psf;
   } mRegisterCursors;

   struct {
      QTextCharFormat registerName;
      QTextCharFormat punctuation;
      QTextCharFormat value;
      QTextCharFormat changedValue;
   } mTextFormats;
};
