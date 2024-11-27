#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"
#include "symbol_table.h"

AST_Node *Compile(const char *filename, const char *source, SymbolTable **st);

#endif
