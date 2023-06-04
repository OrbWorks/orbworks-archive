# PocketC and OrbC Command-line Tools

Command-line tools which support development and analysis of the compiler are documented below.

## PocketC

`bytecode.exe` is PocketC's bytecode printer. To use it, compile a source file using PocketC Desktop Edition, such as this `hello_pocketc.pc` file:

```c
// hello_pocketc.pc
hello(string name) {
    return "Hello, " + name + "!";
}

main() {
    puts(hello("world"));
}
```

Then run `bytecode.exe hello_pocketc.pdb` to generate the following output.

```
   Name: hello_pocketc.pc
Version: 7.10
 Header: 12 bytes
   Libs: 0 (@ 64)

   17   CInt            46
   22   Call
   23   Pop
   24   Halt

Func 0: hello()
   25   Link            (1 args)
   27   CString         1       "Hello, "
   30   GetL            0
   33   Add
   34   CString         9       "!"
   37   Add
   38   SetRet
   39   Jmp             43
   42   SetRet0
   43   UnLink
   44   Ret             1

Func 1: main()
   46   Link            (0 args)
   48   CString         11      "world"
   51   OldCall         hello()
   56   ToString
   57   CallBI          puts
   59   Pop
   60   SetRet0
   61   UnLink
   62   Ret             0
```

## OrbC

OrbC has a more complete suite of command-line tools including a compiler, bytecode printer, and minimal VM. The Windows-based VM is a slightly older snapshot of the Palm OS VM, so some features are incomplete.

### Compiler

The compiler is included and documented in the official release. To show some of the other tools, we will compile the following `hello_orbc.oc` file:

```c
// hello_orbc.oc
string hello(string name) {
    return "Hello, " + name + "!";
}

void main() {
    puts(hello("world"));
}
```

Use the following command line when compiling. Notice that a `.vm` extension is used for the output file. This allows the file to be built without needing to specify app properties:

`oc.exe hello_orbc.oc hello_orbc.vm`.

### Virtual Machine

The Windows VM can execute either `.vm` files or `.prc` files, however only a handful of APIs are actually implemented such at `puts` and some internal compiler helpers. Execute the app using the command line:

`vm.exe hello_orbc.vm`

You should see the "Hello, world!" printed to the console.

### Bytecode Printer

The OrbC bytecode printer can print all the information compiled into the app. The command takes the following arguments:

- `-c` - Print the code. This is the default selection if no parameter is passed.
- `-g` - Print the global initializer table
- `-t` - Print the global types table
- `-s` - Print the string table
- `-a` - Don't print code addresses in the output
- `-l` - Print the native library table
- `-o` - Print the object info table
- `-m` - Print the built-in method table. This option cannot be specified with any other option.

The command-line parser only allows a single parameter, so if you want to print code all the info about a binary, use the command line `bc -cgtslo file.vm`.

Here is the output of `bc hello_orbc.vm`:

```
======  Code ======
    0   Call (35) "main"
    5   Halt
: "hello"
    9   Link 1
   11   CStr (11) "Hello, "
   14   AddS SP <= SP,[0xc000]
   18   CStr (19) "!"
   21   AddS
   22    Jmp 28
   25   CStr (0) ""
   28 SetRet
   29 UnLink
   30    Ret 1
: "main"
   35   Link 0
   37   CStr (21) "world"
   40   Call (9) "hello"
   45 CallBI puts
   48 UnLink
   49   RetN 0
```

For a slightly more interesting example, compile the following `object.oc` file:

```c
interface ISpeak
{
    string Speak();
};

object Animal : ISpeak
{
    virtual string Speak();
};
string Animal.Speak() { return "?"; }

object Dog : Animal
{
    virtual string Speak();
};
string Dog.Speak() { return "woof"; }

void main()
{
    ISpeak* is;
    Animal* a;
    Dog dog;
    a = &dog;
    is = &dog;
    // call through base class pointer to generate a virtual call
    puts(a->Speak());
    // call through interface pointer to generate a virtual call
    puts(is->Speak());
}
```

And print the code and object info table using `bc -co object.vm`:

```
======  Object info  ======
Name: ISpeak (no base, size: D)
Name: Animal (no base, size: D)
  Func: 9 "Animal.Speak"
  Ifaces: [ISpeak: 0]
     Func: 9 "Animal.Speak"
Name: Dog (base: Animal, size: 1)
  Func: 27 "Dog.Speak"
  Ifaces: [ISpeak: 0]
     Func: 27 "Dog.Speak"
======  Code ======
    0   Call (45) "main"
    5   Halt
: "Animal.Speak"
    9   Link 1
   11   CStr (11) "?"
   14    Jmp 20
   17   CStr (0) ""
   20 SetRet
   21 UnLink
   22    Ret 1
: "Dog.Speak"
   27   Link 1
   29   CStr (13) "woof"
   32    Jmp 38
   35   CStr (0) ""
   38 SetRet
   39 UnLink
   40    Ret 1
: "main"
   45   Link 0
   47 StInit (18) "3i"
   50 SCWord [0xc005] <- 55
   55 CWdPFP 5
   58 StorFP 4
   61    Pop
   62 CWdPFP 5
   65 StorFP 3
   68    Pop
   69 LoadFP 4
   72 CallVi 0 (st:0)
   75 CallBI puts
   78 LoadFP 3
   81 CallIf ISpeak.0 (st:0)
   86 CallBI puts
   89   PopN 3
   92 UnLink
   93   RetN 0
```



