#include "shader_compiler.h"
#include <regex>

/*
Matches:
; $Something = true
; $attribVars[1].type = "Float4"
; $VGT_HOS_REUSE_DEPTH.REUSE_DEPTH = 16
; $SQ_VTX_SEMANTIC_CLEAR.CLEAR = 0xFFFFFFFC
*/
static std::regex
sCommentKeyValueRegex
{
   ";[[:space:]]*\\$([_[:alnum:]]+)(?:\\[([[:digit:]]+)\\])?(?:\\.([_[:alnum:]]+))?[[:space:]]*=[[:space:]]*(\"[^\"]+\"|[0-9]+|0x[0-9a-fA-F]+|true|false|TRUE|FALSE)"
};

bool
parseComment(const std::string &comment,
             CommentKeyValue &out)
{
   std::smatch match;
   if (!std::regex_match(comment, match, sCommentKeyValueRegex)) {
      return false;
   }

   out.obj = match[1];
   out.index = match[2];
   out.member = match[3];
   out.value = match[4];

   if (out.value.size() >= 2 && out.value[0] == '"') {
      // Erase quotes from string value
      out.value.erase(out.value.begin());
      out.value.erase(out.value.end() - 1);
   }

   return true;
}

unsigned long
parseNumber(peg::Ast &node)
{
   assert(node.is_token);
   return std::stoul(node.token);
}

float
parseFloat(peg::Ast &node)
{
   return std::stof(node.token);
}

uint32_t
parseHexNumber(peg::Ast &node)
{
   return static_cast<uint32_t>(std::stoul(node.token, 0, 0));
}

LiteralValue
parseLiteral(peg::Ast &node)
{
   auto literal = LiteralValue { 0 };

   for (auto child : node.nodes) {
      if (child->name == "HexNumber") {
         literal.flags |= LiteralValue::ReadHex;
         literal.hexValue = parseHexNumber(*child);
      } else if (child->name == "Float") {
         literal.flags |= LiteralValue::ReadFloat;
         literal.floatValue = parseFloat(*child);
      }
   }

   return literal;
}

bool
parseValueBool(const std::string &v)
{
   auto value = v;
   std::transform(value.begin(), value.end(), value.begin(), ::toupper);

   if (value == "TRUE") {
      return true;
   } else if (value == "FALSE") {
      return true;
   } else {
      throw parse_exception { fmt::format("Expected boolean value, found {}", value) };
   }
}

uint32_t
parseValueNumber(const std::string &value)
{
   return static_cast<uint32_t>(std::stoul(value, 0, 0));
}
