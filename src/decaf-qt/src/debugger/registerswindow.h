#pragma once
#include <array>
#include <cstdint>
#include <QWidget>

#include "debugdata.h"

class KCollapsibleGroupBox;
class QLineEdit;

class RegistersWindow : public QWidget
{
   Q_OBJECT

   struct RegisterWidgets
   {
      std::array<QLineEdit *, 32> gpr;
      std::array<QLineEdit *, 32> fprFloat;
      std::array<QLineEdit *, 32> fprHex;
      std::array<QLineEdit *, 32> ps1Float;
      std::array<QLineEdit *, 32> ps1Hex;
      std::array<QLineEdit *, 8> cr;
      QLineEdit *xer;
      QLineEdit *lr;
      QLineEdit *ctr;
      QLineEdit *msr;
   };

   struct RegisterGroups
   {
      KCollapsibleGroupBox *gpr;
      KCollapsibleGroupBox *misc;
      KCollapsibleGroupBox *cr;
      KCollapsibleGroupBox *fpr;
      KCollapsibleGroupBox *ps1;
   };

public:
   RegistersWindow(QWidget *parent = nullptr);

   void setDebugData(DebugData *debugData);

private slots:
   void debugDataChanged();
   void updateRegisterValue(QLineEdit *lineEdit, uint32_t value);
   void updateRegisterValue(QLineEdit *lineEditFloat, QLineEdit *lineEditHex, double value);
   void updateConditionRegisterValue(QLineEdit *lineEdit, uint32_t value);

private:
   bool mDebugPaused = false;
   DebugData *mDebugData = nullptr;
   RegisterGroups mGroups = { };
   RegisterWidgets mRegisterWidgets = { };
   QPalette mChangedPalette = { };
};
