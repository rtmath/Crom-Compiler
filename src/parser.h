#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "symbol_table.h"

void InitParser(SymbolTable *symbol_table);
AST_Node *ParserBuildAST();

#endif
