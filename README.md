# The Crom Compiler

This repo houses a compiler implementation for the Crom programming
language.

**Crom is under-construction and actively being developed.**

---

## What is Crom?
> Crom is a statically-typed programming language that borrows heavily from C. Crom aims to improve certain areas of C and add QOL features, while maintaining a familiar syntax.

Some existing and planned features include:
  - Terse and explicitly-sized numeric types (i8, u32, f64)
  - Binary literals for easier bitwise manipulation and/or expressiveness
  - Out-of-the-box support for strings (e.g. concatenation, length)
  - First class functions
  - Tail call optimization
  - No need for header files (as found in C)
  - No array-to-pointer decay (as found in C)
  - Generics
  - Multiline string support
  - Default function arguments

---

### Roadmap
- [x] Lexer
- [x] Parser
- [x] Type Checker
- [ ] Interpreter (in progress)
- [ ] IR generation
- [ ] Code generation
- [ ] Optimization


