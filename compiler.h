#ifndef COMPILER_H
#define COMPILER_H

#include "ast.h"

AST_Node *Compile(const char *filename, const char *source);

#endif
