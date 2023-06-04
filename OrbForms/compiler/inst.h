#ifdef DEBUG
char* instNames[] = {
    // constants
    "CInt", "CChar", "CFloat", "CStr",
    // typecasts
    "ItoC", "ItoF", "ItoS", "FtoI", "FtoC", "FtoS",
    "CtoI", "CtoF", "CtoS", "StoI", "StoF", "StoC",
    "StoB",
    // variables
    "Array",
    // operators
    "AddI", "AddC", "AddF", "AddS", "SubI", "SubC", "SubF",
    "MultI", "MultC", "MultF", "DivI", "DivC", "DivF",
    "ModI", "ModC", "ModF", "NegI", "NegC", "NegF",
    "Not", "And", "Or",
    // relational operators
    "EqI", "EqC", "EqF", "EqS", "NEqI", "NEqC", "NEqF", "NEqS", 
    "LTI", "LTC", "LTF", "LTS", "LTEI", "LTEC", "LTEF", "LTES",
    "GTI", "GTC", "GTF", "GTS", "GTEI", "GTEC", "GTEF", "GTES",
    // flow control
    "Jmp", "JmpZ", "JmpNZ",
    "JmpPopZ", "JmpPopNZ",
    "Call", "CallI", "CallBI", "CallH",
    "Ret", "RetN", "SetRet", "StRet0",
    "Pop", "PopN", "Swap", "StDup", "Alloc", "AllocT", "New", "Delete",
    "Link", "UnLink", "Halt", "LCall",

    "CWord", "CWdPFP",
    "Load", "Store", "Copy", "Set", "Get",
    "LoadI", "LoadFP", "LoadF0", "LoadF1",
    "StoreI", "StorFP", "StorF0", "StorF1",
    "IncAI", "IncAC", "IncAF", "IncAS", "DecAI", "DecAC", "DecAF", "DecAS", 
    "IncBI", "IncBC", "IncBF", "IncBS", "DecBI", "DecBC", "DecBF", "DecBS",
    "IncAII", "DecAII", "IncBII", "DecBII",
    // bitwise
    "BAndI", "BAndC", "BOrI", "BOrC", "BNotI", "BNotC", "SLI", "SLC", "SRI", "SRC", "XorI", "XorC",
    "NoOp", "NoOp2", /*"CallF",*/ "FuncA",
    "SwchI", "SwchC", "SwchF", "SwchS", "Case", "Deflt",
    "GetAt", "SetAt",

    // Stack init
    "StInit", "SCInt", "SCChar", "SCFloat", "SCString", "SCWord",
    "SetReg", "GetReg", "Move",
    "CallSBI", "LSCall",

    // inheritance
    "CallVirt", "CallIface", "NewObj", "SObjInfo",

    // variant types
    "VtoI", "VtoC", "VtoF", "VtoS", "VtoB", "VOp1", "VOp2",
    "StoreV",
    "CmpAdr"
};
#endif

int bcsize[vmNumInstr] = {
    // CInt
    4, 1, 4, 2,
    // typecasts
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0,
    // array
    2,
    // operators
    0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0,
    0, 0, 0,
    // relational operators
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    // flow control
    2, 2, 2,
    2, 2,
    4, 0, 2, 0,
    1, 1, 0, 0,
    0, 2, 0, 0, 2, 2, 0, 0,
    1, 0, 0, 3,
    // CWord
    2, 2,
    // Load/Store
    0, 0, 2, 1, 1,
    2, 2, 2, 2,
    2, 2, 2, 2,
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
    2, 2, 2, 2,
    // bitwise
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 2, 4,
    1, 1, 1, 1, 6, 2,
    0, 0,
    // stack init
    2, 6, 3, 6, 4, 4,
    // reg/move
    1, 1, 4,
    // call stubs
    2, 3,
    // inheritance
    2, 4, 2, 4,
    // variant
    0, 0, 0, 0, 0, 0, 0,
    0,
    0
};
