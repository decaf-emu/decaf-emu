#include "shader_compiler.h"
#include "grammar.h"

bool
compileShaderCode(Shader &shader,
                  std::vector<char> &src)
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
   if (!parser.parse_n(src.data(), src.size(), ast)) {
      return false;
   }

   ast = peg::AstOptimizer(false).optimize(ast);
   std::cout << peg::ast_to_s(ast) << std::endl;
   return compileAST(shader, ast);
}
