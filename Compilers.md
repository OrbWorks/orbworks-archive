# Compilers

A very brief look at the PocketC and OrbC compilers.

## PocketC

The PocketC compiler was inspired by [Small-C](https://en.wikipedia.org/wiki/Small-C) as well as [Herbert Schildt's Little C interpreter](https://en.wikipedia.org/wiki/Herbert_Schildt#Little_C). In fact, the first prototype was an interpreter rather than a compiler. PocketC is a [one-pass compiler](https://en.wikipedia.org/wiki/One-pass_compiler), generating the output bytecode as it parses the source code. One-pass compilers are faster and use less memory than multi-pass compilers, which were critical attributes for early Palm OS devices. 

The compiler is written in C++, but looks mostly like C. It is not object-oriented, and almost all compiler state is stored in globals. The compiler is made up of a lexer, parser, symbol table manager, and code manager.

### Lexer

Implemented in lex.cpp.

The [lexer](https://en.wikipedia.org/wiki/Lexical_analysis) (aka lexical analyzer) is responsible for reading source code and converting it to tokens. For example, `bob = 45;` would generate 4 tokens: identifier (bob), assign, constant int (45), semicolon. The lexer has no understanding of semantics.

The pre-processor is implemented within the lexer.

### Parser

Implemented in compile.cpp.

PocketC uses a [recursive descent parser](https://en.wikipedia.org/wiki/Recursive_descent_parser). The parser drives the lexer, calling `nextToken()` to read through the source code. At each level of the parser, the current token is evaluated to see if the source code matches the pattern it is looking for. For example:

```c
// addition/subtraction
void expr4() {
    TokenID id;
    expr5();
    while (tok.id == tiPlus || tok.id == tiMinus) {
        id = tok.id;
        nextToken();
        expr5();
        addByte((id==tiPlus) ? vmAdd : vmSub);
    }
}
```

`expr4()` implements the portion of the parser for addition and subtraction. `expr4()` always calls `expr5()` first to generate the lower-level expression code, and then looks for either `+` or `-` in the source code. If found, that means the first call to `expr5()` produced the left side of the expression and we need to call `expr5()` again to generate the right side. After the right side expression has been generated, we generated the Add or Sub instruction.

As the parser executes, it generates the bytecode for expressions, statements, and functions. However, some instructions need to reference bytecode that hasn't been generated. For example, Jmp* instructions are often used to jump forward. All references to code locations are generated with placeholder code and is fixed up by the code manager after parsing:

- All possible jump targets are given label numbers. Jmp* instructions are generated with the target label number as the argument rather than code offset.
- Function calls generate a temporary CallFunc instruction with the function ID as the argument rather than absolute code location.
- Function addresses generate a temporary FuncA instruction with the function ID as the argument rather than absolute code location.

### Symbol Table Manager

Implemented in symbols.cpp.

This code manages the global symbols, local symbols, and macro definitions.

### Code Manager

Implemented in code.cpp.

This manages adding bytecode, string constants, and float constants to the compiled output. It also handles address fixups, include adding and setting labels (see `newLabel()` and `setLabel()`).

On Palm OS, there is a small in-memory code buffer which is flushed to a Palm database as it fills. On Windows, this is all held in RAM until compilation is complete.

On Windows, the code manager implements a [peephole optimizer](https://en.wikipedia.org/wiki/Peephole_optimization). The optimizer evaluates the 4 most recently generated instructions and applies the following transformations:

- Removes `+ 0`, since it has no effect
- Evaluates constant compile-time expressions in the form `a + b` (e.g. `2 + 3` becomes `5`)
- Converts Add instructions to the AddExtI extended instruction if possible
- Converts Load instructions into GetG or GetL if possible

Address fixups are handled byte `relocate()`, which is the last part of the compilation. It does the following:

- Replaces the arguments to all Jmp* instructions with code offsets based on the label that was originally written to the bytecode
- Replaces FuncCall instructions with OldCall instructions, translating the instruction argument from function ID to absolute code address
- Replaces FuncA instructions with CInt instructions, translating the instruction argument from function ID to absolute code address

The Windows version of `relocate()` performs unused function removal before address fixups. First, the `main()` function is marked as used. Then we iteratively look at all marked functions, marking any functions that they reference as used. Once the complete set is found, the code for all unused functions is removed. 

## OrbC

The OrbC compiler was based on the PocketC compiler, but rewritten in C++ as a two-pass compiler. The compiler is made up of a lexer, parser, symbol table manager, optimizer, and code generator.

### Lexer

Implemented in lex.cpp.

The lexer is very similar to the PocketC lexer, but manages the pre-processor macros itself rather than relying on the symbol table manager. Because there is no longer a direct link between the lexer and the code generation, embedding the source code in the binary is not implemented.

### Parser

The OrbC parser is also a recursive descent parser. It has several major differences from the PocketC parser:

- Compile-time types rather than all types being variants at runtime. Type coercion operations are generated at compile time.
- Support for structs, objects, and interfaces. This adds a lot of complexity to the parser, especially the declaration parsing.
- Strongly-type pointers. Where PocketC pointers were always variants, and therefore mostly indistinguishable from integers in the compiler, the above differences also require tracking pointer types.

The most important difference is that the OrbC parser generates parse trees as its output, rather than directly generating bytecode. For each function/method parse, the following is done:

- A set of parse trees are created. One for the function body, one for the local initializers, and one for the local destructors.
- The function body is parsed, building up the trees as appropriate.
- The trees are optimized by the optimizer.
- Code for the trees is generated by the code generator. Like in PocketC, this code is not yet final - it will need address fixups at the end.

The parser is implemented in the following files:

- parse.cpp - parses top-level elements including functions, enums, consts. Contains the `Parse()` entrypoint.
- decl.cpp - parses struct and variable declarations and initializers
- expr.cpp - parses expressions
- primary.cpp - parses the primary (lowest-level) expression types, such as variables, literals, and function calls
- sinit.cpp - generates implementations of special functions, such as an object initializer if one is required by not defined
- tree.cpp - implements the parse tree structures

### Symbol Table Manager

Implemented in symbol.cpp.

### Optimizer

Implemented in optimize.cpp.

The OrbC optimizer is more comprehensive than PocketC, but is still fairly basic. It focuses primarily on generating more efficient instructions and constant folding. The following high-level transformations are supported:

- Minimizing the size of constants (e.g. using CWord instead of CInt to save space)
- Identity operation removal (`+ 0`, `- 0`, `* 1`, and `/ 1`)
- Constant folding for `+`, `-`, and `*` 
- Addressing mode, such as turning Load into LoadF0 or LoadI
- Converting Load -> Store into Move if possible

The additional addressing modes and constant folding are very beneficial to struct field access.

### Code Generator

Implemented in codegen.cpp.

The code generator is responsible for converting parse trees into bytecode after each function is parsed. There is a lot of code involved, but it is fairly straightforward. Although the optimizer does some transformations to use efficient addressing modes, the code generator is the component that generates extended instructions.

When all compilation is completed, the code generator handles removing unused functions/objects, generating object info, and doing address fixups.
