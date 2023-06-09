# OrbC Virtual Machine

Both the PocketC and OrbC virtual machines are [stack machines](https://en.wikipedia.org/wiki/Stack_machine), meaning most operations push and pop data to a stack rather than using registers.

The OrbC virtual machine adds many features to PocketC.

## Notable Differences

The following is a summary of notable differences between the PocketC and OrbC VMs:

- OrbC uses a different stack frame than PocketC (fp and pc are swapped)

- SetRet0 is an error in the OrbC VM
- OrbC VM uses 32-bit addresses and includes a frame-pointer-based addressing mode
- OrbC VM uses type-specific instructions
- OrbC VM has extended instructions allowing many instructions to specify constants or memory locations as operands. This was inspired by PocketC VM's AddExtI instruction.
- OrbC source code cannot be embedded in the binary

## State

The OrbC VM has nearly the same state as the PocketC VM with the addition of registers and object info.

- `globals` - an array of global values. This array consists of global variables declared at compile time, but can be extended using APIs and VM instructions.
- `stack` - an array of values forming a stack. This array is extended as needed while the program is running. After a function returns, the stack may be reduced.
- `strings` - an array of characters representing the literal strings found in source code.
- `floats` - an array of floats representing the literal floats found in source code.
- `registers` - an array of 10 ints. Although the VM supports 10 registers, the compiler only generates code to use register 0. The register is used to pass a value from one part of a complex expression to another when using the stack would add complexity. This is only used to generate a vector initializer calls.
- `objinfo` - object info generated by the compiler and used to implement several object-based instructions such as virtual and interface method calls.
- `code` - a byte array of instructions. In practice, this is divided into code segments in order to allow for more than 64K of code.
- `pc` - the current program counter.
- `st` - the current stack offset. `stack[st - 1]` is the top of the stack.
- `fp` - the current frame pointer. This is the stack offset when the current function was called and is used to access parameters and local variables.
- `fb` - the current function base. This is the first instruction in a given function. Jump instructions specify offsets from this base.
- `arraySize` - the array size register. This is set by the Array instruction and reset after each other instruction. Constant instructions (e.g. CInt) will use this register to determine if they should push an array of values rather than just one.
- `ret` - the return value register. This temporary register is set by a return statement (resulting in a SetRet instruction) and pushed on the stack by the Ret instruction.

## Memory

Memory is accessed via 32-bit addresses. Every value in the virtual machine is a variant which can be an int, char, float, or string. Many instruction are specific to globals or locals. However, some instructions use memory addresses.

- `0x00000000-0x7fffffff`: represents global values. An address of `addr` refers to `globals[addr]`.
- `0x80000000-0xbfffffff`: represents stack values. An address of `addr` refers to `stack[addr & 0x3fffffff]`.
- `0x80000000-0xbfffffff`: aliases for stack values based on the current `fp`. An address of `addr` refers to `stack[(addr & 0x3fffffff) + fp]`. This addressing mode exists to support extended instructions, allowing an operand for an instruction to be retrieved from a frame-pointer-relative position on the stack.

Unlike in PocketC, type of a value in memory cannot change at runtime. Attempts to write the wrong type of value in a memory location would originally result in a vm error. However, the addition of PocketC compatibility relaxed this constraint.

Some instructions embed a word-sized compressed address. Compressed addresses represent the address ranges above using `0x0000-0x7fff`, `0x8000-0xbfff`, and `0xc0000-0xffff`.

There is no mechanism for the app to access `code`.

## Lifecycle

Before executing code:

- `globals` array entries are initialized based on the global type table and the global init table.
- Native libraries are initialized.

For apps with UI:

- Initializers are called for global objects
- App.onstart is called
- The UI drives calls into handlers until the app is ready to exit
- App.onstop is called
- Destructors are called for global objects

For apps without UI:

- Initializers are called for global objects
- Execution starts at `code[0]`and runs until the VM halts
- Destructors are called for global objects

## Object Info

The compiler generates a table of object info which is used by the VM to perform virtual function calls, interface calls, and initialize memory when allocating objects. Each object type has an object ID, which is the offset within the object table where its info starts. For each object type, the following information is encoded:

- Name (embedded in the object info table, not the string table)
- Object ID of base object type if this object is a subclass, otherwise `0xffff`. Objects only support single-inheritance.
- Object size
- Virtual methods
  - Number of virtual methods
  - Array of addresses for the methods. Some may be built-in or library methods, in which case the address of a compiler-generated stub is specified.
- Interfaces
  - Number of interfaces (including inherited interfaces)
  - Number of total interface methods
  - Array of interface method table offsets. Each entry is 6 bytes containing a 16-bit offset into the object table where the interface is defined and a 32-bit offset into the table where the list of interface methods starts. Note that interfaces support multiple-inheritance, so it is possible that a single interface may be represented multiple times in this array.
  - Array of interface method addresses. Because of multiple-inheritance, it is possible for this array to have multiple references to the same interface methods.
- Sub-objects
  - Number of sub-objects in this object. This is not recursive.
  - Array of sub-objects. Each entry is the memory offset within the object where the sub-object is located followed by the object ID of sub-object.

The description above is a bit complex. To understand better, use the `bc.exe` tool to print the object info from a binary. See [Readme](Tools/Readme.md) in Tools.

## Instructions

The following table describes all the instructions supported by the OrbC VM.

Notes:

- The OrbC VM has many more instructions than the PocketC VM, mostly due to the typed memory layout requiring type-specific instructions. Where PocketC had ToInt, OrbC has FtoI, CtoI, StoI, and VtoI.
- Unlike the PocketC VM, value types are only coerced into compatible types by specific instructions which support variants. These instructions were added for PocketC Architect to support compatibility with existing source code.
- Most instructions can operate on the stack, a constant, or an dereferenced address of memory. This is controlled by extended instructions, detailed below. For this reason, many instructions below are defined in terms of `op1` and `op2`. For unary operators, `op2` is the only operand.
- Some unimplemented instructions (e.g. IncAS) exist to make code generation easier and allow variant operators to be reinterpreted more easily. See VOp1 and VOp2.
- Most instructions are described in terms of the state described in the first section (e.g. the `globals` array), and a set of operations: 
  - `push(v)` - pushes a value on the stack, incrementing `st`
  - `v = pop()` - pop a value from the stack, decrementing `st`
  - `deref(addr)` - read a value from a memory address, which refers to either `globals` or `stack`, depending on the address
  - `expand(addr)` - converts a 16-bit address into a 32-bit address
- The descriptions below are simplified. The actual implementations often have optimizations.

| Byte | Inst | Size | Arg             | Description                |
| ---- | ---- | ---- | -------------------------- | -------------------------- |
| 0x00 | CInt | 4 | int value | `push(value)`<br/>See Array. |
| 0x01 | CChar | 1 | char value | `push(value)`<br/>See Array. |
| 0x02 | CFloat | 4 | float value | `push(value)`<br/>See Array. |
| 0x03 | CStr | 2 | index | `v = strings[index]; push(v)`<br/>See Array. |
| 0x04 | ItoC | 0 |  | Convert `op2` from an int to a char and push the result. The following XtoX instructions behave as you would expect based on their names. |
| 0x05 | ItoF | 0 |  | See above |
| 0x06 | ItoS | 0 |  | See above |
| 0x07 | FtoI | 0 |  | See above |
| 0x08 | FtoC | 0 |  | See above |
| 0x09 | FtoS | 0 |  | See above |
| 0x0a | CtoI | 0 |  | See above |
| 0x0b | CtoF | 0 |  | See above |
| 0x0c | CtoS | 0 |  | See above |
| 0x0d | StoI | 0 |  | See above |
| 0x0e | StoF | 0 |  | See above |
| 0x0f | StoC | 0 |  | See above |
| 0x10 | StoB | 0 |  | Convert `op2` from a string to a boolean (0, 1) and push the result. |
| 0x11 | Array | 2 | size | `arraySize = size`<br/>If the next instruction is CInt, CChar, CFloat, CString, CWord, or CWordPFP, it will push `arraySize` copies of the supplied constant instead of just one. |
| 0x12 | AddI | 0 |  | `v = op1 + op2; push(v)`<br/>This instruction operates on int values. The following AddX instructions operate on different types based on their name (C = char, F = float, S = string). |
| 0x13 | AddC | 0 |  | See above |
| 0x14 | AddF | 0 |  | See above |
| 0x15 | AddS | 0 |  | See above |
| 0x16 | SubI | 0 |  | `v = op1 - op2; push(v)`<br/>The following SubX instructions behave as you would expect based on their names. |
| 0x17 | SubC | 0 |  | See above |
| 0x18 | SubF | 0 |  | See above |
| 0x19 | MultI | 0 |  | `v = op1 * op2; push(v)`<br/>The following MultX instructions behave as you would expect based on their names. |
| 0x1a | MultC | 0 |  | See above |
| 0x1b | MultF | 0 |  | See above |
| 0x1c | DivI | 0 |  | `v = op1 / op2; push(v)`<br/>The following DivX instructions behave as you would expect based on their names. |
| 0x1d | DivC | 0 |  | See above |
| 0x1e | DivF | 0 |  | See above |
| 0x1f | ModI | 0 |  | `v = op1 % op2; push(v)`<br/>The following ModX instructions behave as you would expect based on their names. |
| 0x20 | ModC | 0 |  | See above |
| 0x21 | ModF | 0 |  | Generates a vm error. |
| 0x22 | NegI | 0 |  | `v = -op2; push(v)`<br/>The following NegX instructions behave as you would expect based on their names. |
| 0x23 | NegC | 0 |  | See above |
| 0x24 | NegF | 0 |  | See above |
| 0x25 | Not | 0 |  | `v = !op2; push(v)`<br/>The operand must be an int |
| 0x26 | And | 0 |  | `v = op1 && op2; push(v)`<br/>The operands must be ints |
| 0x27 | Or | 0 |  | `v = op1 || op2; push(v)`<br/>The operands must be ints |
| 0x28 | EqI | 0 |  | `v = op1 == op2; push(v)`<br/>The following EqX instructions behave as you would expect based on their names. |
| 0x29 | EqC | 0 |  | See above |
| 0x2a | EqF | 0 |  | See above |
| 0x2b | EqS | 0 |  | See above |
| 0x2c | NEqI | 0 |  | `v = op1 != op2; push(v)`<br/>The following NEqX instructions behave as you would expect based on their names. |
| 0x2d | NEqC | 0 |  | See above |
| 0x2e | NEqF | 0 |  | See above |
| 0x2f | NEqS | 0 |  | See above |
| 0x30 | LTI | 0 |  | `v = op1 < op2; push(v)`<br/>The following LTX instructions behave as you would expect based on their names. |
| 0x31 | LTC | 0 |  | See above |
| 0x32 | LTF | 0 |  | See above |
| 0x33 | LTS | 0 |  | See above |
| 0x34 | LTEI | 0 |  | `v = op1 <= op2; push(v)`<br/>The following LTEX instructions behave as you would expect based on their names. |
| 0x35 | LTEC | 0 |  | See above |
| 0x36 | LTEF | 0 |  | See above |
| 0x37 | LTES | 0 |  | See above |
| 0x38 | GTI | 0 |  | `v = op1 > op2; push(v)`<br/>The following GTX instructions behave as you would expect based on their names. |
| 0x39 | GTC | 0 |  | See above |
| 0x3a | GTF | 0 |  | See above |
| 0x3b | GTS | 0 |  | See above |
| 0x3c | GTEI | 0 |  | `v = op1 >= op2; push(v)`<br/>The following GTEX instructions behave as you would expect based on their names. |
| 0x3d | GTEC | 0 |  | See above |
| 0x3e | GTEF | 0 |  | See above |
| 0x3f | GTES | 0 |  | See above |
| 0x40 | Jmp | 2 | offset | `pc = fb + offset` |
| 0x41 | JmpZ | 2 | offset | `if (op2) pc = fb + offset`<br/>This instruction leaves the operand on the stack. |
| 0x42 | JmpNZ | 2 |  | `if (!op2) pc = fb + offset`<br/>This instruction leaves the operand on the stack. |
| 0x43 | JmpPopZ | 2 |  | `if (op2) pc = fb + offset`<br/>This instruction leaves the operand on the stack if the branch is taken, but pops it if the branch is not taken. |
| 0x44 | JmpPopNZ | 2 |  | `if (!op2) pc = fb + offset`<br/>This instruction leaves the operand on the stack if the branch is taken, but pops it if the branch is not taken. |
| 0x45 | Call | 4 | addr | `push(fb); push(pc); pc = addr; fb = pc` |
| 0x46 | CallI | 0 |  | `addr = pop(); push(fb); push(pc); pc = addr; fb = pc` |
| 0x47 | CallBI | 2 | funcID | Call a built-in function |
| 0x48 | CallH | 0 |  | Call a handler. Same as Call but if the popped address is -1 the call is skipped. This allows safely calling an unimplemented handler. |
| 0x49 | Ret | 1 | count | `fb = pop(); pc = pop();` Call `pop()` count times to discard the function args. `push(ret)`<br/>`if (pc == -1) halt` |
| 0x4a | RetN | 1 | count | Same as Ret but does not push a return value. |
| 0x4b | SetRet | 0 |  | `ret = op2` |
| 0x4c | SetRet0 | 0 |  | Generates a vm error. Executing this instruction means that a non-void function didn't return a value. This instruction exists because the compiler is not smart enough to ensure that all code paths return a value. |
| 0x4d | Pop | 0 |  | `pop()` |
| 0x4e | PopN | 2 | count | Call `pop()` count times |
| 0x4f | Swap | 0 |  | `a = pop(); b = pop(); push(a); push(b)`<br/>This instruction is not generated. |
| 0x50 | StDup | 0 |  | `a = pop(); push(a); push(a)` |
| 0x51 | Alloc | 2 | count | `addr = st`, push count int values, `push(addr | 0x80000000)`<br/>Allocates values on the stack and then pushes the address of the start of the allocation. No longer generated. |
| 0x52 | AllocT | 2 | index | `addr = st`, push values of the types described by `strings[index]`, `push(addr |0x80000000)`<br/>Allocates values on the stack and then pushes the address of the start of the allocation. No longer generated. |
| 0x53 | New | 0 |  | `ts = pop(); count = pop();` allocate heap memory using the types specified in `strings[ts]`, push the address of the heap allocation. |
| 0x54 | Delete | 0 |  | `addr = pop();` free heap memory at `addr` |
| 0x55 | Link | 1 | count | `push(fp); fp = st - count - 3`<br/>Setup the new frame pointer accounting for `count` parameters. |
| 0x56 | UnLink | 0 |  | `fp = pop()` |
| 0x57 | Halt | 0 |  | Halt the virtual machine |
| 0x58 | LibCall | 3 | libID, funcID | Call the specified function `funcID` in library `libID`.<br/>libID is a byte and funcID is a word. |
| 0x59 | CWord | 2 | word value | `push(value)`<br/>See Array. |
| 0x5a | CWordPFP | 2 |word value | `push((value + fp) | 0x80000000)`<br/>This sets up the address of a local. See Array. |
| 0x5b | Load | 0 |  | `addr = pop(); v = deref(addr); push(v)` |
| 0x5c | Store | 0 |  | `v = pop(); addr = pop(); deref(addr) = v; push(v)` |
| 0x5d | Copy | 2 | copy | `src = pop(); dest = pop()`, copy `count` values from `src` to `dest` |
| 0x5e | Set | 1 |  | Not implemented |
| 0x5f | Get | 1 |  | Not implemented |
| 0x60 | LoadI | 2 | index | `v = globals[index]; push(v)` |
| 0x61 | LoadFP | 2 | offset | `v = stack[offset + fp]; push(v)` |
| 0x62 | LoadF0 | 2 | offset | `this = stack[fp]; v = deref(fp + offset); push(v)`<br/>This instruction in generated in object methods to load fields of the current object. The `this` pointer is the first parameter to the method. |
| 0x63 | LoadF1 | 2 | offset | `this = stack[fp + 1]; v = deref(fp + offset); push(v)`<br/>Same as LoadF0 but used in methods that return structs, because those methods have the struct return address as the first parameter and `this ` as the second parameter. |
| 0x64 | StoreI | 2 | index | `v = pop(); globals[index] = v; push(v)` |
| 0x65 | StorFP | 2 | offset | `v = pop(); stack[fp + offset] = v; push(v)` |
| 0x66 | StorF0 | 2 | offset | `this = stack[fp]; v = pop(); deref(fp + offset) = v; push(v)`<br/>This instruction in generated in object methods to store fields of the current object. The `this` pointer is the first parameter to the method. |
| 0x67 | StorF1 | 2 | offset | `this = stack[fp + 1]; v = pop(); deref(fp + offset) = v; push(v)`<br/>Same as StorF0 but used in methods that return structs, because those methods have the struct return address as the first parameter and `this ` as the second parameter. |
| 0x68 | IncAI | 0 |  | `push(deref(op2)++)`<br/>Post-increment the value pointed to by `op2`. The following IncAX instructions behave as you would expect based on their names. |
| 0x69 | IncAC | 0 |  | See above |
| 0x6a | IncAF | 0 |  | See above |
| 0x6b | IncAS | 0 |  | Generates a vm error. |
| 0x6c | DecAI | 0 |  | `push(deref(op2)--)`<br/>Post-decrement the value pointed to by `op2`. The following DecAX instructions behave as you would expect based on their names. |
| 0x6d | DecAC | 0 |  | See above |
| 0x6e | DecAF | 0 |  | See above |
| 0x6f | DecAS | 0 |  | Generates a vm error. |
| 0x70 | IncBI | 0 |  | `push(++deref(op2))`<br/>Pre-increment the value pointed to by `op2`. The following IncBX instructions behave as you would expect based on their names. |
| 0x71 | IncBC | 0 |  | See above |
| 0x72 | IncBF | 0 |  | See above |
| 0x73 | IncBS | 0 |  | Generates a vm error. |
| 0x74 | DecBI | 0 |  | `push(--deref(op2))`<br/>Pre-decrement the value pointed to by `op2`. The following DecBX instructions behave as you would expect based on their names. |
| 0x75 | DecBC | 0 |  | See above |
| 0x76 | DecBF | 0 |  | See above |
| 0x77 | DecBS | 0 |  | Generates a vm error. |
| 0x78 | IncAII | 2 | value | `v = &deref(op2); push(*v); *v += value`<br/>Post-increment by `value` the value point to by `op2` |
| 0x79 | DecAII | 2 | value | `v = &deref(op2); push(*v); *v -= value`<br/>Post-decrement by `value` the value point to by `op2` |
| 0x7a | IncBII | 2 | value | `v = &deref(op2); *v += value; push(*v)`<br/>Pre-increment by `value` the value point to by `op2` |
| 0x7b | DecBII | 2 | value | `v = &deref(op2); *v -= value; push(*v)`<br/>Pre-decrement by `value` the value point to by `op2` |
| 0x7c | BAndI | 0 |  | `v = op1 & op2; push(v)` |
| 0x7d | BAndC | 0 |  | `v = op1 & op2; push(v)` |
| 0x7e | BOrI | 0 |  | `v = op1 | op2; push(v)` |
| 0x7f | BOrC | 0 |  | `v = op1 | op2; push(v)` |
| 0x80 | BNotI | 0 |  | `v = ~op2; push(v)` |
| 0x81 | BNotC | 0 |  | `v = ~op2; push(v)` |
| 0x82 | SLI | 0 |  | `v = op1 << op2; push(v)` |
| 0x83 | SLC | 0 |  | `v = op1 << op2; push(v)` |
| 0x84 | SRI | 0 |  | `v = op1 >> op2; push(v)` |
| 0x85 | SRC | 0 |  | `v = op1 >> op2; push(v)` |
| 0x86 | XorI | 0 |  | `v = op1 ^ op2; push(v)` |
| 0x87 | XorC | 0 |  | `v = op1 ^ op2; push(v)` |
| 0x88 | NoOp | 0 |  | No-op |
| 0x89 | NoOp2 | 2 | index | No-op with a word index. This instruction is used to embed function names into the binary,  with `strings[index]` being the embedded string. These values are used by the bytecode printer as well as generating callstacks for errors. |
| 0x8a | FuncA | 4 | index | Intermediate instruction that never appears in a compiled app. The instruction is generated during compilation as a placeholder for a function address before the final location of the function is known. After dead function removal, the target function address is determined and this is replaced with the CInt instruction. |
| 0x8b | SwitchI | 1 | count | Starts a switch statement which is followed by `count` Case instructions and an optional Default instruction. `v = pop()`. Each case statement value is compared with `v` until a match is found, at which point `pc = fb + offset`. If no match is found and the next instruction is Default, the instruction's offset is used. |
| 0x8c | SwitchC | 1 | count | See above |
| 0x8d | SwitchF | 1 | count | See above |
| 0x8e | SwitchS | 1 | count | See above |
| 0x8f | Case | 6 | value, offset | `value` is interpreted based on the SwitchX instruction. For SwitchI and SwitchC it is treated as a literal value. For SwitchF it is reinterpreted as a float. For SwitchS it is treated as an index into `strings`. |
| 0x90 | Default | 2 | offset | See SwitchX |
| 0x91 | GetAt | 0 |  | `index = pop(); addr = pop(); c = deref(addr)[index]; push(c)`<br/>Get a character from a string |
| 0x92 | SetAt | 0 |  | `c = pop(); index = pop(); addr = pop(); deref(addr)[index] = c`<br/>Set a character in a string |
| 0x93 | StackInit | 2 | index | Initialize the stack of a function using the type string `strings[index]`. Pushes a series of values of the correct type onto the stack. |
| 0x94 | SCInt | 6 | addr, value | `v = &deref(expand(addr)); *v = value`<br/>Set constant int. `addr` is a word-sized compressed address. |
| 0x95 | SCChar | 3 | addr, value | `v = &deref(expand(addr)); *v = value`<br/>Set constant int. `addr` is a word-sized compressed address. |
| 0x96 | SCFloat | 6 | addr, value | `v = &deref(expand(addr)); *v = value`<br/>Set constant int. `addr` is a word-sized compressed address. |
| 0x97 | SCString | 4 | addr, index | `v = &deref(expand(addr)); *v = strings[index]`<br/>Set constant int. `addr` is a word-sized compressed address. |
| 0x98 | SCWord | 4 | addr, value | `v = &deref(expand(addr)); *v = value`<br/>Set constant int. `addr` is a word-sized compressed address. |
| 0x99 | SetReg | 1 | index | `registers[index] = stack[st - 1]`<br/>Copies the top stack value to the specified register without popping. The value must be an int. |
| 0x9a | GetReg | 1 | index | `push(registers[index])` |
| 0x9b | Move | 4 | src, dest | `deref(expand(dest)) = deref(expand(src))`<br/>Copy the value at `src` to the value at `dest`. `src` and `dest` and word-sized compressed addresses. |
| 0x9c | CallStubBI | 2 | funcID | `pc = pop(); fb = pop()`, call the bulit-in function `funcID`. When code takes the address of a built-in function, the compiler generates a stub which uses this instruction to remove the normal stack frame leaving only the parameters needed by the built-in function. |
| 0x9d | LibStubCall | 3 | libID, funcID | Same as CallStubBI but for library functions |
| 0x9e | CallVirt | 2 | offset, funcID | `this = stack[st - 1 - offset]`, look up the address of the virtual method `funcID` in `objinfo` based on the type of the object `this`, `push(fb); push(pc); pc = addr; fb = pc` |
| 0x9f | CallIface | 4 | offset, interfaceID, funcID | `this = stack[st - 1 - offset]`, look up the address of the interface method `funcID` on interface `interfaceID` in `objinfo` based on the type of the object `this`, `push(fb); push(pc); pc = addr; fb = pc` |
| 0xa0 | NewObj | 2 | objectID | `types = pop(); count = pop()`, `addr` = allocate an array of `count` objects, write the `objectID` as the first value of each allocated object, and recursively do the same for each embedded sub-object, `push(addr)` |
| 0xa1 | SObjInfo | 4 | addr, objectID | Not implemented                                              |
| 0xa2 | VtoI | 0 |  | Convert `op2` from a variant to a char and push the result. The following VtoX instructions behave as you would expect based on their names. |
| 0xa3 | VtoC | 0 |  | See above |
| 0xa4 | VtoF | 0 |  | See above |
| 0xa5 | VtoS | 0 |  | See above |
| 0xa6 | VtoB | 0 |  | See above |
| 0xa7 | VOp1 | 0 |  | Read the next instruction and execute the type-correct version which matches the type of the `op2`. For example, if `op2` is a float and the following instruction is NegI then execute NegF. This was added to support PocketC compatibility. |
| 0xa8 | VOp2 | 0 |  | Same as VOp1 but for binary operators. Type coercion occurs first. |
| 0xa9 | StoreV | 0 |  | `v = pop(); addr = pop()` , typecast `v` to match the type of memory at `addr`, `deref(addr) = v` |
| 0xaa | CmpAdr | 0 |  | Compress the address on the top of the stack |
| 0xc0+ |  |  |  | See Extended Instructions below |

### Extended Instructions

Any instruction with the top 2 bits (bits 6-7) set is an extended instruction which modifies the operands of the following instruction. Bits 4-5 define where `op2` comes from, and bits 2-3 define where `op1` comes from. Bits 0-1 were designed to define where the result of an operation ends up, but only the stack is supported (other options are not generated by the compiler nor implemented by the VM). The operand sources are as follows:

| Value  | Source    | Description                                                  |
| ------ | --------- | ------------------------------------------------------------ |
| `0b00` | None      | There is no operand. This is used for `op1` when the instruction is a unary operator. |
| `0b01` | Stack     | The operand is popped from the stack                         |
| `0b10` | Immediate | The operand is an immediate (constant) value stored as a word in the next 2 instruction bytes |
| `0b11` | Address   | The operand is a memory location at the compressed address stored as a word in the next 2 instruction bytes. |
