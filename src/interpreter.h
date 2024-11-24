#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "symbol_table.h"

void Interpret(AST_Node *root, SymbolTable *st);

#endif
