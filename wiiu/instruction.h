#pragma once
#include <cstdint>

union Instruction
{
   Instruction() {}
   Instruction(uint32_t value) : value(value) {}

   uint32_t value;

   operator uint32_t() const
   {
      return value;
   }

   struct {
      uint32_t : 1;
      uint32_t aa : 1;
      uint32_t : 30;
   };

   struct {
      uint32_t : 2;
      uint32_t bd : 14;
      uint32_t : 16;
   };

   struct {
      uint32_t : 16;
      uint32_t bi : 5;
      uint32_t : 11;
   };

   struct {
      uint32_t : 21;
      uint32_t bo : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 16;
      uint32_t crbA : 5;
      uint32_t : 11;
   };

   struct {
      uint32_t : 11;
      uint32_t crbB : 5;
      uint32_t : 16;
   };

   struct {
      uint32_t : 21;
      uint32_t crbD : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 23;
      uint32_t crfD : 3;
      uint32_t : 6;
   };

   struct {
      uint32_t : 18;
      uint32_t crfS : 3;
      uint32_t : 11;
   };

   struct {
      uint32_t : 12;
      uint32_t crm : 8;
      uint32_t : 12;
   };

   struct {
      uint32_t d : 16;
      uint32_t : 16;
   };

   struct {
      uint32_t : 17;
      uint32_t fm : 8;
      uint32_t : 7;
   };

   struct {
      uint32_t : 16;
      uint32_t frA : 5;
      uint32_t : 11;
   };

   struct {
      uint32_t : 11;
      uint32_t frB : 5;
      uint32_t : 16;
   };

   struct {
      uint32_t : 6;
      uint32_t frC : 5;
      uint32_t : 21;
   };

   struct {
      uint32_t : 21;
      uint32_t frD : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 21;
      uint32_t frS : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 12;
      uint32_t i : 3;
      uint32_t : 17;
   };

   struct {
      uint32_t : 12;
      uint32_t imm : 4;
      uint32_t : 16;
   };

   struct {
      uint32_t : 2;
      uint32_t li : 24;
      uint32_t : 6;
   };
   

   struct {
      uint32_t lk : 1;
      uint32_t : 31;
   };
   

   struct {
      uint32_t : 6;
      uint32_t mb : 5;
      uint32_t : 21;
   };

   struct {
      uint32_t : 1;
      uint32_t me : 5;
      uint32_t : 26;
   };

   struct {
      uint32_t : 11;
      uint32_t nb : 5;
      uint32_t : 16;
   };

   struct {
      uint32_t : 10;
      uint32_t oe : 1;
      uint32_t : 21;
   };

   struct {
      uint32_t : 26;
      uint32_t opcd : 6;
   };

   struct {
      uint32_t : 16;
      uint32_t rA : 5;
      uint32_t : 11;
   };

   struct {
      uint32_t : 11;
      uint32_t rB : 5;
      uint32_t : 16;
   };

   struct {
      uint32_t rc : 1;
      uint32_t : 31;
   };

   struct {
      uint32_t : 21;
      uint32_t rD : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 21;
      uint32_t rS : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t : 11;
      uint32_t sh : 5;
      uint32_t : 16;
   };

   struct {
      int32_t simm : 16;
      int32_t : 16;
   };

   struct {
      uint32_t : 16;
      uint32_t sr : 4;
      uint32_t : 12;
   };

   struct {
      uint32_t : 11;
      uint32_t spr : 10;
      uint32_t : 11;
   };

   struct {
      uint32_t : 11;
      uint32_t spru : 5;
      uint32_t sprl : 5;
      uint32_t : 11;
   };

   struct {
      uint32_t : 21;
      uint32_t to : 5;
      uint32_t : 6;
   };

   struct {
      uint32_t uimm : 16;
      uint32_t : 16;
   };

   struct {
      uint32_t : 1;
      uint32_t xo1 : 10;
      uint32_t : 21;
   };

   struct {
      uint32_t : 1;
      uint32_t xo2 : 9;
      uint32_t : 22;
   };

   struct {
      uint32_t : 1;
      uint32_t xo3 : 6;
      uint32_t : 25;
   };

   struct {
      uint32_t : 1;
      uint32_t xo4 : 5;
      uint32_t : 26;
   };
};
