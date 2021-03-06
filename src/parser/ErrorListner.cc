#include "ErrorListner.h"

#include "exception/exceptions.h"

namespace antlr4 {

void SyntaxErrorListner::syntaxError(Recognizer *recognizer,
                                     Token *offendingSymbol, size_t line,
                                     size_t charPositionInLine,
                                     const std::string &msg,
                                     std::exception_ptr e) {
  throw thdb::ParserException(msg);
}

}  // namespace antlr4
