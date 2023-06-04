# PocketC Virtual Machine

Both the PocketC and OrbC virtual machines are [stack machines](https://en.wikipedia.org/wiki/Stack_machine), meaning most operations push and pop data to a stack rather than using registers.

The PocketC virtual machine is simpler the the OrbC version that was later derived from it. Over the releases, the VM has added functionality via new instructions and new interpretations of old instructions.

## State

The PocketC VM has the following state:

- `globals` - an array of global values. This array initially consists of global variables declared at compile time, but can be extended using APIs.
- `stack` - an array of values forming a stack. This array is extended as needed while the program is running. After a function returns, the stack may be reduced.
- `strings` - an array of characters representing the literal strings found in source code.
- `floats` - an array of floats representing the literal floats found in source code.
- `code` - a byte array of instructions. In practice, this is divided into code segments in order to allow for more than 64K of code.
- `pc` - the current program counter.
- `st` - the current stack offset. `stack[st - 1]` is the top of the stack.
- `fp` - the current frame pointer. This is the stack offset when the current function was called and is used to access parameters and local variables.
- `fb` - the current function base. This is the first instruction in a given function. Jump instructions specify offsets from this base.
- `arraySize` - the array size register. This is set by the Array instruction and reset after each other instruction. Constant instructions (e.g. CInt) will use this register to determine if they should push an array of values rather than just one. Earlier versions of the compiler would also use this register to modify the behavior of some instructions that reference local and global values, which is still supported in the VM, but is not documented below.
- `ret` - the return value register. This temporary register is set by a return statement (resulting in a SetRet instruction) and pushed on the stack by the Ret instruction.

## Memory

Memory is accessed via 16-bit addresses. Every value in the virtual machine is a variant which can be an int, char, float, or string. Many instruction are specific to globals or locals. However, some instructions use memory addresses.

- `0x0000-0x5fff`: represents global values. An address of `addr` refers to `globals[addr]`.
- `0x6000-0xffff`: represents stack values. An address of `addr` refers to `stack[addr - 0x6000]`.

The type of a value in memory can change at runtime. For example, the Store instruction will change the type of the memory location to match the source value.

There is no mechanism for the app to access `code`.

## Startup

Before executing code:

- `globals` array entries are initialized based on the global init table.
- Native libraries are initialized.

In the initial version of PocketC, execution began at `code[0]`. In order to add more features, later versions set this instruction to Halt and added a header to the `code` array. 

## Instructions

The following table describes all the instructions supported by the PocketC VM.

Notes:

- When necessary, value types are coerced into compatible types.
- Many binary and unary operations are invalid for strings. Some are also invalid for floats.
- Most instructions are described in terms of the state described in the first section (e.g. the `globals` array), and a set of operations: 
  - `push(v)` - pushes a value on the stack, incrementing `st`
  - `v = pop()` - pop a value from the stack, decrementing `st`
  - `deref(addr)` - read a value from a memory address, which refers to either `globals` or `stack`, depending on the address
  - `truth(v)` - converts the value to a truth value (0 or 1)
- The descriptions below are simplified. The actual implementations often have optimizations.

| Byte | Inst | Size | Arg | Description                |
| ---- | ----------- | ----- | -------------------------- | -------------------------- |
| 0x00 | CInt | 4 | int value | `push(value)`<br/>See Array. |
| 0x01 | CChar | 1 | char value | `push(value)`<br/>See Array. |
| 0x02 | CFloat | 2 | index                | `v = floats[index]; push(v)`<br/>See Array. |
| 0x03 | CString | 2 | index | `v = strings[index]; push(v)`<br/>See Array. |
| 0x04 | ToInt | 0 |  | Convert the top stack value to an int |
| 0x05 | ToChar | 0 |  | Convert the top stack value to a char |
| 0x06 | ToFloat | 0 |  | Convert the top stack value to a float |
| 0x07 | ToString | 0 |  | Convert the top stack value to a string |
| 0x08 | GetG | 2 | index | `v = globals[index]; push(v)` |
| 0x09 | SetG | 2 | index | `globals[index] = stack[st-1]` |
| 0x0a | GetL | 2 | index | `v = stack[fp + index]; push(v)` |
| 0x0b | SetL | 2 | index | `stack[fp + index] = stack[st-1]` |
| 0x0c | Array | 0 |  | `arraySize = pop()`<br/>If the next instruction is CInt, CChar, CFloat, CString, or CWord, it will push `arraySize` copies of the supplied constant instead of just one. |
| 0x0d | IncGA | 2 | index | `v = globals[index]++; push(v)`<br/>Note: G in IncGA means Global and A means After (post-increment), as opposed to L (Local) and B (Before). |
| 0x0e | DecGA | 2 | index | `v = globals[index]--; push(v)` |
| 0x0f | IncLA | 2 | index | `v = stack[fp + index]++; push(v)` |
| 0x10 | DecLA | 2 | index | `v = stack[fp + index]--; push(v)` |
| 0x11 | IncGB | 2 | index | `v = ++globals[index]; push(v)` |
| 0x12 | DecGB | 2 | index | `v = --globals[index]; push(v)` |
| 0x13 | IncLB | 2 | index | `v = ++stack[fp + index]; push(v)` |
| 0x14 | DecLB | 2 | index | `v = --stack[fp + index]; push(v)` |
| 0x15 | Add | 0 |  | `b = pop(); a = pop(); push(a + b)` |
| 0x16 | Sub | 0 |  | `b = pop(); a = pop(); push(a - b)` |
| 0x17 | Mult | 0 |  | `b = pop(); a = pop(); push(a * b)` |
| 0x18 | Div | 0 |  | `b = pop(); a = pop(); push(a / b)` |
| 0x19 | Mod | 0 |  | `b = pop(); a = pop(); push(a % b)` |
| 0x1a | Neg | 0 |  | `stack[st - 1] = -stack[st - 1]` |
| 0x1b | Not | 0 |  | `stack[st - 1] = !stack[st - 1]` |
| 0x1c | And | 0 |  | `b = truth(pop()); a = truth(pop()); push(a && b)` |
| 0x1d | Or | 0 |  | `b = truth(pop()); a = truth(pop()); push(a || b)` |
| 0x1e | Eq | 0 |  | `b = pop(); a = pop(); push(a == b)` |
| 0x1f | NEq | 0 |  | `b = pop(); a = pop(); push(a != b)` |
| 0x20 | LT | 0 |  | `b = pop(); a = pop(); push(a < b)` |
| 0x21 | LTE | 0 |  | `b = pop(); a = pop(); push(a <= b)` |
| 0x22 | GT | 0 |  | `b = pop(); a = pop(); push(a > b)` |
| 0x23 | GTE | 0 |  | `b = pop(); a = pop(); push(a >= b)` |
| 0x24 | Jmp | 2 | offset | `pc = fb + offset` |
| 0x25 | JmpZ | 2 | offset | `a = truth(pop()); if (a) pc = fb + offset` |
| 0x26 | JmpNZ | 2 | offset | `a = truth(pop()); if (!a) pc = fb + offset` |
| 0x27 | OldCall | 4 | addr | `push(pc); push(fb); pc = addr; fb = pc`<br/>Older versions of the compiler used a word address. |
| 0x28 | CallBI | 1 | funcID | Call a built-in function |
| 0x29 | Ret | 1 | count | `fb = pop(); pc = pop()`, call `pop()` count times to discard the function args, `push(ret)` |
| 0x2a | SetRet | 0 |  | `ret = pop()` |
| 0x2b | SetRet0 | 0 |  | `ret = 0`<br/>This instruction is emitted at the end of every function to ensure that a return value has been set. A return statement in source code will jump past this instruction, so it will only be executed when the code reaches the end of a function without reaching a return statement. This instruction exists because the compiler isn't smart enough to know if all code paths ret |
| 0x2c | Pop | 0 |  | `pop()` |
| 0x2d | PopN | 2 | count | Call `pop()` count times |
| 0x2e | Link | 1 | count | `push(fp); fp = st - count - 3`<br/>Setup the new frame pointer accounting for `count` parameters. |
| 0x2f | UnLink | 0 |  | `fp = pop()` |
| 0x30 | Halt | 0 |  | Halt the virtual machine |
| 0x31 | Swap | 0 |  | `b = pop(); a = pop(); push(b); push(a)`<br/>This instruction is used to generate a call through a function pointer, since the function pointer is put on the stack before the parameters. |
| 0x32 | LibCall | 2 | libID, funcID | Call the specified function `funcID` in library `libID`<br/>libID and funcID are each a byte |
| 0x33 | CWord | 2 | word value | `push(value)`<br/>See Array. |
| 0x34 | CWordPFP | 2 | word value | `push(value + fp)`<br/>This sets up the address of a local. |
| 0x35 | Call | 0 |  | `addr = pop(); push(pc); push(fb); pc = addr; fb = pc` |
| 0x36 | Load | 0 |  | `addr = pop(); v = deref(addr); push(v)` |
| 0x37 | Save | 0 |  | `v = pop(); addr = pop(); deref(addr) = v` |
| 0x38 | IncA | 0 |  | `addr = pop(); deref(addr)++; push(deref(addr))`<br/>Post-increment the value pointed to by the address on top of the stack |
| 0x39 | DecA | 0 |  | `addr = pop(); deref(addr)--; push(deref(addr))` |
| 0x3a | IncB | 0 |  | `addr = pop(); ++deref(addr); push(deref(addr))` |
| 0x3b | DecB | 0 |  | `addr = pop(); --deref(addr); push(deref(addr))` |
| 0x3c | BAnd | 0 |  | `b = pop(); a = pop(); push(a & b)` |
| 0x3d | BOr | 0 |  | `b = pop(); a = pop(); push(a | b)` |
| 0x3e | BNot | 0 |  | `a = pop(); push(~a)` |
| 0x3f | SL | 0 |  | `b = pop(); a = pop(); push(a << b)` |
| 0x40 | SR | 0 |  | `b = pop(); a = pop(); push(a >> b)` |
| 0x41 | Xor | 0 |  | `b = pop(); a = pop(); push(a ^ b)` |
| 0x42 | NoOp | 0 |  | No-op |
| 0x43 | NoOp2 | 2 | index | No-op with a word index. This instruction is used to embed function names and source code into the binary,  with `strings[index]` being the embedded string. These values are used by the bytecode printer as well as generating callstacks for errors. |
| 0x44 | CallFunc | 4 | index | Intermediate instruction that never appears in a compiled app. The instruction is generated during compilation as a placeholder for a function call before the final location of the function is known. After dead function removal, the target function address is determined and this is replaced with the OldCall instruction. |
| 0x45 | FuncA | 4 | index | Intermediate instruction that never appears in a compiled app. The instruction is generated during compilation as a placeholder for a function address before the final location of the function is known. After dead function removal, the target function address is determined and this is replaced with the CInt instruction. |
| 0x46 | AddExtI | 5 | mask, index1, index2 | Add extended. This instruction is used to generate much more efficient code when computing addresses. `mask` is a byte which indicates the meaning of the word arguments `index1` and `index2`. The index values can either be constant values or global/local addresses which are dereferenced. See compiler and VM source code for details. |
| 0x47 | GetC | 0 |  | `index = pop(); addr = pop(); c = deref(addr)[index]; push(c)`<br/>Get a character from a string |
| 0x48 | SetC | 0 |  | `c = pop(); index = pop(); addr = pop(); deref(addr)[index] = c`<br/>Set a character in a string |
| 0x49 | Case | 7 | type, value, offset | Implements a case statement. The type specifies how to interpret value: an int, a char, or a index into `strings`. The value is then compared with the top of the stack, and if equal:  `pop(); pc = fb + offset`. |
