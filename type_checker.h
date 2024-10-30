#ifndef TYPE_CHECKER_H
#define TYPE_CHECKER_H

#include "ast.h"
#include "symbol_table.h"

void CheckTypes(AST_Node *ast_root, SymbolTable *symbol_table);

#endif
