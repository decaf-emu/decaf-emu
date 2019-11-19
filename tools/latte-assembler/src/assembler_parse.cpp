#include "shader_assembler.h"
#include "grammar.h"

bool
assembleShaderCode(Shader &shader,
                   std::string_view code)
{
   peg::parser parser;
   parser.log = [&](size_t ln, size_t col, const std::string &msg) {
      std::cout << "Error parsing grammar:" << ln << ":" << col << ": " << msg << std::endl;
   };

   if (!parser.load_grammar(LatteGrammar)) {
      return false;
   }

   parser.log = [&](size_t ln, size_t col, const std::string &msg) {
      std::cout << shader.path << ":" << ln << ":" << col << ": " << msg << std::endl;
   };

   parser.enable_ast();

   std::shared_ptr<peg::Ast> ast;
   if (!parser.parse_n(code.data(), code.size(), ast)) {
      return false;
   }

   ast = peg::AstOptimizer(false).optimize(ast);
   assembleAST(shader, ast);
   return true;
}
