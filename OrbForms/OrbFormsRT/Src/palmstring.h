void lib_strlen(VM* vm, int) {
    Value* s = vm->pop();
    vm->retVal.iVal = strlen(s->lock());
    s->unlockRelease();
}

void my_substr(VM* vm, char* src, int i, int n) {
    int len = strlen(src);
    if (n < 0) n = len - i + n;
    if (i < 0 || n <= 0 || i >= len)
        vm->retVal.newConstString("");
    else {
        if (i + n > len) n = len - i;
        char* dst = vm->retVal.newString(n);
        if (!dst) oom();
        for (int j=0;j<n;j++)
            dst[j] = src[j+i];
        dst[n] = '\0';
        vm->retVal.unlock();
    }
}
        
void lib_substr(VM* vm, int) {
    Value* nArg = vm->pop();
    Value* iArg = vm->pop();
    Value* sArg = vm->pop();
    my_substr(vm, sArg->lock(), iArg->iVal, nArg->iVal);
    sArg->unlockRelease();
}

void lib_right(VM* vm, int) {
    Value* nArg = vm->pop();
    Value* sArg = vm->pop();
    int n = nArg->iVal;
    char* s = sArg->lock();
    int len = strlen(s);
    if (n > len) n = len;
    if (n < 0) n = len + n;
    if (n < 0) n = 0;
    my_substr(vm, s, len - n, n);
    sArg->unlockRelease();
}

void lib_left(VM* vm, int) {
    Value* nArg = vm->pop();
    Value* sArg = vm->pop();
    int n = nArg->iVal;
    char* s = sArg->lock();
    int len = strlen(s);
    if (n > len) n = len;
    if (n < 0) n = len + n;
    if (n < 0) n = 0;
    my_substr(vm, s, 0, n);
    sArg->unlockRelease();
}

void lib_strupper(VM* vm, int) {
    Value* s = vm->pop();
    vm->retVal.copyString(s->lock());
    s->unlockRelease();
    char* dst = vm->retVal.lock();
    while (*dst) {
        if (*dst >= 'a' && *dst <= 'z')
            *dst -= 'a' - 'A';
        dst++;
    }
    vm->retVal.unlock();
}

void lib_strlower(VM* vm, int) {
    Value* s = vm->pop();
    vm->retVal.copyString(s->lock());
    s->unlockRelease();
    char* dst = vm->retVal.lock();
    while (*dst) {
        if (*dst >= 'A' && *dst <= 'Z')
            *dst += 'a' - 'A';
        dst++;
    }
    vm->retVal.unlock();
}

void lib_strstr(VM* vm, int) {
    Value* first = vm->pop();
    Value* sub = vm->pop();
    Value* str = vm->pop();
    
    char* strp = str->lock();
    char* subp = sub->lock();
    
    if (strlen(strp) - first->iVal >= strlen(subp)) {
        char* res = StrStr(&strp[first->iVal], subp);
        if (res) vm->retVal.iVal = res - strp;
        else vm->retVal.iVal = -1;
    } else vm->retVal.iVal = -1;
    
    sub->unlockRelease();
    str->unlockRelease();
}

void lib_strinsert(VM* vm, int) {
    Value* vins = vm->pop();
    int pos = vm->pop()->iVal;
    Value* vstr = vm->pop();

    char* ins = vins->lock();
    char* str = vstr->lock();
    int lins = strlen(ins);
    int lstr = strlen(str);
    char* res = vm->retVal.newString(lins + lstr);
    if (!res) oom();

    if (pos > lstr) pos = lstr;
    if (pos > 0)
        strncpy(res, str, pos);
    strcpy(res + pos, ins);
    if (pos < lstr)
        strcpy(res + pos + lins, str + pos);
    
    vstr->unlockRelease();
    vins->unlockRelease();
    vm->retVal.unlock();
}

void lib_strreplace(VM* vm, int) {
    Value* vreplace = vm->pop();
    Value* vsearch = vm->pop();
    Value* vstr = vm->pop();

    char* replace = vreplace->lock();
    char* search = vsearch->lock();
    char* str = vstr->lock();
    int lreplace = strlen(replace);
    int lsearch = strlen(search);
    int lstr = strlen(str);
    int nReplace = 0;
    char* iter = str;
    
    while (iter) {
        iter = StrStr(iter, search);
        if (iter) {
            nReplace++;
            iter += lsearch;
        }
    }

    char* res = vm->retVal.newString(lstr + (lreplace - lsearch) * nReplace);
    if (!res) oom();

    while (true) {
        iter = StrStr(str, search);
        if (iter) {
            while (str != iter) {
                *res++ = *str++;
            }
            strcpy(res, replace);
            res += lreplace;
            str += lsearch;
        } else {
            strcpy(res, str);
            break;
        }
    }
    
    vstr->unlockRelease();
    vsearch->unlockRelease();
    vreplace->unlockRelease();
    vm->retVal.unlock();
}

void lib_strstoc(VM* vm, int) {
    Value* vpc = vm->pop();
    Value* vs = vm->pop();
    
    char* str = vs->lock();
    dword pc = vpc->iVal;
    int len = 0;
    
    do {
        Value* dst = vm->deref(pc);
        if (dst->type != vtChar) vm->vm_error("strstoc() - ptr is not a char");
        dst->cVal = *str;
        pc++;
        len++;
    } while (*str++);
    
    vs->unlockRelease();
    vm->retVal.iVal = len;
}

void lib_strctos(VM* vm, int) {
    Value* vpc = vm->pop();
    
    int len = 0;
    dword ptr = vpc->iVal;
    Value* dst;
    
    do {
        dst = vm->deref(ptr++);
        if (dst->type != vtChar) vm->vm_error("strctos() - ptr is not a char");
        len++;
    } while (dst->cVal);
    
    char* str = vm->retVal.newString(len-1);
    if (!str) oom();
    
    ptr = vpc->iVal;
    do {
        dst = vm->deref(ptr++);
        *str++ = dst->cVal;
    } while (dst->cVal);
    
    vm->retVal.unlock();
}

void lib_hex(VM* vm, int) {
    static char hex[] = "0123456789abcdef";
    Value* val = vm->pop();
    
    char* res = vm->retVal.newString(16);
    if (!res) oom();
    
    int index = 0;
    unsigned long uval = val->iVal;
    res[index++] = '\0';

    while (uval) {
        res[index++] = hex[uval % 16];
        uval /= 16;
    }
    if (index==1
    ) res[index++] = '0';
    res[index++] = 'x';
    res[index] = '0';
    char t;
    for (int r=0; r<=index/2; r++) t=res[r], res[r] = res[index-r], res[index-r]=t;
    vm->retVal.unlock();
}

void format(VM* vm, bool bLoc) {
    Value& sig = *vm->pop();
    Value& fArg = *vm->pop();
    char dot = '.';
    
    if (bLoc) {
        NumberFormatType numFormat = (NumberFormatType)PrefGetPreference(prefNumberFormat);
        if (numFormat == nfPeriodComma || numFormat == nfSpaceComma || numFormat == nfApostropheComma)
            dot = ',';
    }
        
    FlpCompDouble fd;
    fd.d = (double)fArg.fVal;
    Int16 sign, expn;
    UInt32 mant, round;
    FlpBase10Info(fd.fd, &mant, &expn, &sign);
    
    int iter;
    if (-expn - sig.iVal > 8)
        mant = 0;
    if (mant == 0) {
        expn = -1;
    } else {
        expn+=7;
        sig.iVal = sig.iVal < 0 ? -sig.iVal : sig.iVal;
        if (sig.iVal > 8) sig.iVal = 8;
        iter = -expn - sig.iVal + 7;
        if (iter > 0) {
            round = 5; while (--iter) round *= 10UL;
            mant+=round;
        }
        if (mant > 99999999) expn++;

        if (expn > 7) { // Use scientific notation
            char* ret = vm->retVal.newString(16);
            if (!ret) oom();
            ftoa(fArg.fVal, ret);
            vm->retVal.unlock();
            return;
        }
    }
    char* buff = g_buff;
    if (mant == 0) {
     	strcpy(buff, "00000000");
    } else {
        StrIToA(buff, mant);
    }
    char* ret =	vm->retVal.newString(16);
    if (!ret) oom();
    iter = 0;
    
    if (fArg.fVal != 0 && sign && -expn <= sig.iVal) ret[iter++]='-';
    if (expn < 0) {
        ret[iter++]='0';
        if (sig.iVal) {
            ret[iter++]=dot;
            for (int i=1;i<-expn;i++) ret[iter++]='0';
            ret[sig.iVal+2]='\0';
        } else {
            ret[1] = '\0';
        }
    }
    while (-expn <= sig.iVal) {
        if (*buff) ret[iter++]=*buff++;
        else ret[iter++]='0';
        if (expn==0 && sig.iVal) ret[iter++]=dot;
        expn--;
    }
    ret[iter]='\0';
    vm->retVal.unlock();
}

void lib_format(VM* vm, int) {
    format(vm, false);
}

void lib_lformat(VM* vm, int) {
    format(vm, true);
}

void lib_lparse(VM* vm, int) {
    char buff[16];
    Value* str = vm->pop();
    strncpy(buff, str->lock(), 16);
    str->unlockRelease();
    buff[15] = 0;
    NumberFormatType numFormat = (NumberFormatType)PrefGetPreference(prefNumberFormat);
    if (numFormat == nfPeriodComma || numFormat == nfSpaceComma || numFormat == nfApostropheComma) {
        for (int i=0;i<16;i++) {
            if (buff[i] == '.') buff[i] = ',';
            else if (buff[i] == ',') buff[i] = '.';
        }
    }
    vm->retVal.type = vtFloat;
    vm->retVal.fVal = atof(buff);
}

// string list
