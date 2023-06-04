void lib_math(VM* vm, int fID) {
    Value* fArg = vm->pop();
    
    if (MathLibRef == 0xffff) {
        vm->vm_error("This application requires MathLib");
    }
    double x, result;

    x = (double)fArg->fVal;
    
    switch (fID - firstMath) {
        case  0: MathLibCos(MathLibRef, x, &result); break;
        case  1: MathLibSin(MathLibRef, x, &result); break;
        case  2: MathLibTan(MathLibRef, x, &result); break;
        case  3: MathLibACos(MathLibRef, x, &result); break;
        case  4: MathLibASin(MathLibRef, x, &result); break;
        case  5: MathLibATan(MathLibRef, x, &result); break;
        case  6: MathLibCosH(MathLibRef, x, &result); break;
        case  7: MathLibSinH(MathLibRef, x, &result); break;
        case  8: MathLibTanH(MathLibRef, x, &result); break;
        case  9: MathLibACosH(MathLibRef, x, &result); break;
        case 10: MathLibASinH(MathLibRef, x, &result); break;
        case 11: MathLibATanH(MathLibRef, x, &result); break;
        case 12: MathLibExp(MathLibRef, x, &result); break;
        case 13: MathLibLog(MathLibRef, x, &result); break;
        case 14: MathLibLog10(MathLibRef, x, &result); break;
        case 15: MathLibSqrt(MathLibRef, x, &result); break;
    }
    vm->retVal.type = vtFloat;
    vm->retVal.fVal = (float)result;
}

void lib_atan2(VM* vm, int) {
    Value* vy = vm->pop();
    Value* vx = vm->pop();
    double x, y, res;

    x = (double)vx->fVal;
    y = (double)vy->fVal;
    MathLibATan2(MathLibRef, x, y, &res);
    vm->retVal.type = vtFloat;
    vm->retVal.fVal = (float)res;
}

void lib_pow(VM* vm, int) {
    Value* vy = vm->pop();
    Value* vx = vm->pop();
    double x, y, res;

    x = (double)vx->fVal;
    y = (double)vy->fVal;
    MathLibPow(MathLibRef, x, y, &res);
    vm->retVal.type = vtFloat;
    vm->retVal.fVal = (float)res;
}

void lib_random(VM* vm, int) {
    Value* n = vm->pop();
    word r = SysRandom(0);
    vm->retVal.iVal = r % n->iVal;
}

void lib_rand(VM* vm, int) {
    word r = SysRandom(0);
    vm->retVal.type = vtFloat;
    vm->retVal.fVal = (float)r / sysRandomMax;
}

void lib_srand(VM* vm, int) {
    SysRandom(vm->pop()->iVal);
}
