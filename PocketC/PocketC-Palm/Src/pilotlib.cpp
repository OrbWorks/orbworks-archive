#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#include <Bitmap.h>

#ifndef POCKETC_FAT
#include "PocketC.h"
#endif
#include "PocketC_res.h"

#include <DLServer.h> // Needed to get user name
#include <SysEvtMgr.h> // Needed for EvtResetAutoOffTimer
#include <SerialMgrOld.h> // Needed for serial I/O

#ifdef POCKETC_SCANNER
extern "C" {
#include <ScanMgrStruct.h>
#include <ScanMgr.h>
}
#endif


#define RetInt(x) { retVal.iVal = (x); }
extern bool openOutput;

VoidPtr GetObjectPtr(Word objectID);
short FindMemo(char*, bool);
void Alert(char*);
void rt_oom();
void CallFunc(unsigned short loc);
extern EventType lastEvent;

#define firstMath 10
#define firstMath2 26
bool serOpen;
long atexit_func;
extern DWord romVersion;

inline void emptyString(Value* v) {
    NewConstString(v, "");
}


#if 0
void _____Time_Date_____() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// Time/Date routines
// 
void lib_ticks(short) {
    RetInt(TimGetTicks() % 2147483648UL);
}


void lib_time_core(bool now) {
    Value arg;
    DateTimeType dt;
    ULong sec;
    SystemPreferencesType sysPref;
    
    pop(arg);
    if (now) {
        sec = TimGetSeconds();
    } else {
        Value vsec;
        pop(vsec);
        sec = vsec.iVal + 2147483648UL;
    }
    TimSecondsToDateTime(sec, &dt);
    if (arg.iVal==0) {
        RetInt(dt.hour * 100 + dt.minute);
        return;
    }
    if (arg.iVal==2) {
        RetInt(dt.hour * 10000UL + dt.minute * 100UL + dt.second);
        return;
    }

    PrefGetPreferences(&sysPref);
    char* timeStr = NewString(&retVal, timeStringLength);
    if (!timeStr) rt_oom();
    TimeToAscii(dt.hour, dt.minute, sysPref.timeFormat, timeStr);
    UnlockString(&retVal);
}

void lib_time(short) {
    lib_time_core(true);
}

void lib_timex(short) {
    lib_time_core(false);
}

void lib_date_core(bool now) {
    Value arg;
    DateTimeType dt;
    ULong sec;
    SystemPreferencesType sysPref;
    
    pop(arg);
    if (now) {
        sec = TimGetSeconds();
    } else {
        Value vsec;
        pop(vsec);
        sec = vsec.iVal + 2147483648UL;
    }
    TimSecondsToDateTime(sec, &dt);
    if (arg.iVal==0) {
        RetInt(dt.year * 10000UL + dt.month * 100UL + dt.day);
        return;
    }

    PrefGetPreferences(&sysPref);
    char* dateStr = NewString(&retVal, arg.iVal == 1 ? dateStringLength : longDateStrLength);
    if (!dateStr) rt_oom();
    DateToAscii(dt.month, dt.day, dt.year, (arg.iVal==1 ? sysPref.dateFormat : sysPref.longDateFormat), dateStr);
    UnlockString(&retVal);
}

void lib_date(short) {
    lib_date_core(true);
}

void lib_datex(short) {
    lib_date_core(false);
}

void lib_seconds(short) {
    RetInt(TimGetSeconds()-2147483648UL);
}

void lib_date_selectdate(short) {
    Value title, selectBy, date;
    pop(title);
    pop(selectBy);
    pop(date);
    
    DateTimeType dt;
    TimSecondsToDateTime(date.iVal + 2147483648UL, &dt);
    retVal.iVal = 0;
    
    if (SelectDay((SelectDayType)selectBy.iVal, &dt.month, &dt.day, &dt.year, LockString(&title))) {
        retVal.iVal = TimDateTimeToSeconds(&dt) - 2147483648UL;
    }
    UnlockReleaseString(&title);
    killVM = false;
}

void lib_date_selecttime(short) {
    Value title, date;
    pop(title);
    pop(date);
    
    DateTimeType dt;
    TimSecondsToDateTime(date.iVal + 2147483648UL, &dt);
    retVal.iVal = 0;

    if (SelectOneTime(&dt.hour, &dt.minute, LockString(&title))) {
        retVal.iVal = TimDateTimeToSeconds(&dt)-2147483648UL;
    }
    UnlockReleaseString(&title);
    killVM = false;
}

void lib_secondsx(short) {
    Value time, date;
    pop(time);
    pop(date);
    
    DateTimeType dt;
    dt.year = date.iVal / 10000;
    dt.month = date.iVal / 100 % 100;
    dt.day = date.iVal % 100;
    dt.hour = time.iVal / 10000;
    dt.minute = time.iVal / 100 % 100;
    dt.second = time.iVal % 100;
    retVal.iVal = TimDateTimeToSeconds(&dt) - 2147483648UL;
}

#if 0
void _______Math______() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// Math routines
//

void lib_mathlib(short) {
    RetInt(mathLibLoaded);
}

void lib_hex(short) {
    Value arg;
    pop(arg);
    
    char* res = NewString(&retVal, 16);
    if (!res) rt_oom();
    MemSet(res, 16, 0);
    
    short index = 0;
    unsigned long uval = arg.iVal;
    char c;
    while (uval) {
        c = '0' + uval % 16;
        if (c > '9') c= c -'0' - 10 + 'a';
        res[index++] = c;
        uval /= 16;
    }
    if (index==0) res[index++] = '0';
    res[index++] = 'x';
    res[index] = '0';
    char t;
    for (short r=0; r<=index/2; r++) t=res[r], res[r] = res[index-r], res[index-r]=t;
    UnlockString(&retVal);
}

void lib_math(short fID) {
    Value fArg;
    pop(fArg);
    double x, result;
    if (!mathLibLoaded) return;

    x = (double)fArg.fVal;
    
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
    retVal.type = vtFloat;
    retVal.fVal = (float)result;
}
        
    
void lib_math2(short fID) {
    Value arg1, arg2;
    double x, y, res;
    pop(arg2);
    pop(arg1);
    if (!mathLibLoaded) return;
    
    y = (double)arg1.fVal;
    x = (double)arg2.fVal;
    if (fID==firstMath2) MathLibPow(MathLibRef, y, x, &res);
    else MathLibATan2(MathLibRef, y, x, &res);
    retVal.type = vtFloat;
    retVal.fVal = (float)res;
}

#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836
#define NTAB 32
#define NDIV (1+(IM-1)/NTAB)
#define EPS 1.2e-7
#define RNMX (1.0-EPS)

// Global state for the random number generator
void randomize(long s);
long seed, idum, iy = 0, iv[NTAB];

void lib_rand(short) {
    retVal.type = vtFloat;
    if (!iy) randomize(1);
    long k = idum/IQ;
    idum = IA*(idum-k*IQ)-IR*k;
    if (idum < 0) idum += IM;
    short j = iy/NDIV;
    iy = iv[j];
    iv[j] = idum;
    float temp = AM * iy;
    if (temp > RNMX) retVal.fVal = RNMX;
    else retVal.fVal = temp;	
}

void lib_random(short) {
    Value arg;
    pop(arg);
    lib_rand(0);
    retVal.type = vtInt;
    retVal.iVal = retVal.fVal * arg.iVal;
}

void randomize(long s) {
    idum = seed = s;
    if (idum == 0) idum = 1;
    for (short j = NTAB+7; j >= 0; j--) {
        long k = (idum)/IQ;
        idum = IA*(idum-k*IQ)-IR*k;
        if (idum < 0) idum += IM;
        if (j < NTAB) iv[j] = idum;
    }
    iy = iv[0];
}

extern char msg[];
#define abs(x) (x < 0 ? -x : x)
void lib_format(short) {
    Value fArg, sig;
    pop(sig);
    pop(fArg);
    
    FlpCompDouble fd;
    fd.d = (double)fArg.fVal;
    SWord sign, expn;
    DWord mant, round;
    FlpBase10Info(fd.fd, &mant, &expn, &sign);

    short iter;
    if (-expn - sig.iVal > 8)
        mant = 0;
    if (mant == 0) {
        expn = -1;
    } else {
        expn+=7;
        sig.iVal = abs(sig.iVal);
        if (sig.iVal > 8) sig.iVal = 8;
        iter = -expn - sig.iVal + 7;
        if (iter > 0) {
            round = 5; while (--iter) round *= 10UL;
            mant+=round;
        }
        if (mant > 99999999) expn++;

        if (expn > 7) { // Use scientific notation
            retVal.fVal = fArg.fVal;
            retVal.type = vtFloat;
            typeCast(retVal, vtFloat);
            return;
        }
    }
    char* buff = msg;
    if (mant == 0) {
     	strcpy(buff, "00000000");
    } else {
        StrIToA(buff, mant);
    }
    retVal.type = vtString;
    char* ret = NewString(&retVal, 16);
    if (!ret) rt_oom();
    iter = 0;
    if (fArg.fVal != 0 && sign && -expn <= sig.iVal) ret[iter++]='-';
    if (expn < 0) {
        ret[iter++]='0';
        if (sig.iVal) {
            ret[iter++]='.';
            for (short i=1;i<-expn;i++) ret[iter++]='0';
            ret[sig.iVal+2]='\0';
        } else {
            ret[1] = '\0';
        }
    }
    while (-expn <= sig.iVal) {
        if (*buff) ret[iter++]=*buff++;
        else ret[iter++]='0';
        if (expn==0 && sig.iVal) ret[iter++]='.';
        expn--;
    }
    ret[iter]='\0';
    UnlockString(&retVal);
}

void lib_strstr(short) {
    Value str, sub, first;
    pop(first);
    pop(sub);
    pop(str);
    
    char *strp, *subp;
    strp = LockString(&str);
    subp = LockString(&sub);
    if (strlen(strp) - first.iVal >= strlen(subp)) {
        char* res = StrStr(&strp[first.iVal], subp);
        if (res) retVal.iVal = res - strp;
        else retVal.iVal = -1;
    } else retVal.iVal = -1;
    
clean:
    UnlockReleaseString(&str);
    UnlockReleaseString(&sub);
}

#if 0
void _____Basic_IO_____() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// Basic I/O routines
//
void lib_msg(short) {
    Value arg;
    pop(arg);
    char* str = LockString(&arg);
    Alert(str);
    UnlockString(&arg);
    cleanup(arg);
    UIYield(false);   
    eventQueue.RemoveInput();
}

void lib_confirm(short) {
    Value arg;
    pop(arg);
    char* str = LockString(&arg);
    retVal.iVal = !(FrmCustomAlert(ConfirmAlert, str, "", ""));
    UnlockString(&arg);
    cleanup(arg);
    UIYield(false);
    eventQueue.RemoveInput();
}

void lib_puts(short) {
    Value arg;
    volatile short lc;
    pop(arg);
    if (!openOutput) {
        FrmGotoForm(OutputForm);
        openOutput = true;
        UIYield(false);
    }
    
    if ((lc=MemHandleLockCount(output))>1)
        MemHandleUnlock(output);
    if (FrmGetActiveFormID()==OutputForm) {
        char* str = LockString(&arg);
        FieldAttrType fAttr;
        FieldPtr fp = (FieldPtr)GetObjectPtr(OutputOutputField);
        
        FldSetInsPtPosition(fp, -1);
        FldGetAttributes(fp, &fAttr);
        fAttr.editable=true;
        FldSetAttributes(fp, &fAttr);
        FldInsert(fp, str, strlen(str));
        fAttr.editable=false;
        FldSetAttributes(fp, &fAttr);
    } else {
        char* str = LockString(&arg);
        short size1 = h_strlen(output);
        short size2 = strlen(str);
        if (0 == MemHandleResize(output, size1 + size2 + 1)) {
            DmStrCopy(MemHandleLock(output), size1, str);
            MemHandleUnlock(output);
        }
    }
    UnlockReleaseString(&arg);
    UIYield(false);
}

void lib_clear(short) {
    if (FrmGetActiveFormID()==OutputForm) {
        FieldPtr fp = (FieldPtr)GetObjectPtr(OutputOutputField);
        FldDelete(fp, 0, -1);
    } else {
        MemHandleResize(output, 1);
        DmSet(MemHandleLock(output), 0, 1, 0);
        MemHandleUnlock(output);
    }
}	

extern char* defValue;
void lib_gets(short id) {
    Value arg;
    Value def;

    if (!openOutput) {
        FrmGotoForm(OutputForm);
        openOutput = true;
    }

    if (id > 3) {
        pop(def);
        defValue = LockString(&def);
    } else {
        defValue = NULL;
    }

    pop(arg);
    
    inputDone = false;
    inputRet = NULL; // This should be unnecessary
    FrmPopupForm(InputForm);
    UIYield(false); // Allow form to be set up
    if (inputDone || killVM) goto gets_done;
    FormPtr frmP = FrmGetFormPtr(InputForm);
    if (!frmP) goto gets_done;
    FrmCopyLabel(frmP, InputPromptLabel, LockString(&arg));
    UnlockString(&arg);
    while (!inputDone && !killVM) {
        UIYield(true);
    }

gets_done:
    if (defValue)
        UnlockString(&def);
    retVal.type = vtString;
    if (inputRet) {
        char* pir = (char*)MemHandleLock(inputRet);
        NewStringFromConst(&retVal, pir);
        MemHandleUnlock(inputRet);
        MemHandleFree(inputRet);
        inputRet = NULL;
    } else
        emptyString(&retVal);
    UIYield(false);
    eventQueue.RemoveInput();
    cleanup(arg);
    if (id > 3) cleanup(def);
}

extern short FInputx, FInputy, FInputw, FInputh;
void lib_getsi(short hasHeight) {
    Value def;
    Value x, y, w, h;

    if (!openOutput) {
        FrmGotoForm(OutputForm);
        openOutput = true;
    }

    pop(def);
    defValue = LockString(&def);
    
    if (hasHeight == -1) {
        pop(h);
        FInputh = h.iVal;
    } else {
        FInputh = 1;
    }
    pop(w);
    pop(y);
    pop(x);
    FInputx = x.iVal;
    FInputy = y.iVal;
    FInputw = w.iVal;

    inputDone = false;
    inputRet = NULL; // This should be unnecessary
    
    FrmPopupForm(FInputForm);
    UIYield(false); // Allow form to be set up
    if (inputDone || killVM) goto gets_done;
    
    FormPtr frmP = FrmGetFormPtr(FInputForm);
    if (!frmP) goto gets_done;
    while (!inputDone && !killVM) {
        UIYield(true);
    }

gets_done:
    if (defValue)
        UnlockString(&def);
    retVal.type = vtString;
    if (inputRet) {
        char* pir = (char*)MemHandleLock(inputRet);
        NewStringFromConst(&retVal, pir);
        MemHandleUnlock(inputRet);
        MemHandleFree(inputRet);
        inputRet = NULL;
    } else
        emptyString(&retVal);

    UIYield(false);
    eventQueue.RemoveInput();
    cleanup(def);
}

void lib_getsm(short) {
    lib_getsi(-1);
}

#if 0
void ______String______() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// String routines
//

void lib_strtoc(short) {
    Value strV, ptrV;
    pop(ptrV);
    pop(strV);
    
    short ptr = ptrV.iVal;
    char* str = LockString(&strV);
    Value* v;
    while (*str) {
        v = deref(ptr);
        if (v->type != vtChar)
            vm_error("Incorrect memory type in strtoc", pc);
        v->cVal = *str;
        str++;
        ++ptr;
    }
    v = deref(ptr);
    if (v->type != vtChar)
        vm_error("Incorrect memory type in strtoc", pc);
    v->cVal = 0;	
    UnlockReleaseString(&strV);
}

void lib_ctostr(short) {
    Value ptrV;
    pop(ptrV);
    
    long ptr = ptrV.iVal;
    short len = 0;
    
    Value* v;
    do {
        v = deref(ptr);
        if (v->type != vtChar)
            vm_error("Incorrect memory type in ctostr", pc);
        len++;
        ptr++;
    } while (v->cVal);
    
    char* str = NewString(&retVal, len);
    if (!str) rt_oom();
    
    short i = 0;
    ptr = ptrV.iVal;
    while (i < len) {
        v = deref(ptr);
        str[i] = v->cVal;
        i++;
        ptr++;
    }
    retVal.type = vtString;
    UnlockString(&retVal);	
}

void lib_strlen(short) {
    Value arg;
    pop(arg);
    char* data = LockString(&arg);
    short len = strlen(data);
    UnlockReleaseString(&arg);
    retVal.iVal = len;
}

void lib2_substr(Value* s, short i, short n) {
    char* src = LockString(s);
    short len = strlen(src);
    retVal.type = vtString;
    if (n < 0) n = len - i + n;
    if (i < 0 || n <= 0 || i >= len)
        emptyString(&retVal);
    else {
        if (i + n > len) n = len - i;
        char* dst = NewString(&retVal, n+1);
        if (!dst) rt_oom();
        for (short j=0;j<n;j++)
            dst[j] = src[j+i];
        dst[n] = '\0';
        UnlockString(&retVal);
    }
    UnlockString(s);
}
        
void lib_substr(short) {
    Value sArg, iArg, nArg;
    pop(nArg);
    pop(iArg);
    pop(sArg);
    lib2_substr(&sArg, iArg.iVal, nArg.iVal);
    cleanup(sArg);
}

void lib_right(short) {
    Value sArg, nArg;
    pop(nArg);
    pop(sArg);
    short n = nArg.iVal;
    short len = strlen(LockString(&sArg));
    UnlockString(&sArg);
    if (n > len) n = len;
    if (n < 0) n = len + n;
    if (n < 0) n = 0;
    lib2_substr(&sArg, len - n, n);
    cleanup(sArg);
}

void lib_left(short) {
    Value sArg, nArg;
    pop(nArg);
    pop(sArg);
    short n = nArg.iVal;
    short len = strlen(LockString(&sArg));
    UnlockString(&sArg);
    if (n > len) n = len;
    if (n < 0) n = len + n;
    if (n < 0) n = 0;
    lib2_substr(&sArg, 0, n);
    cleanup(sArg);
}

void lib_strupr(short) {
    Value sArg;
    pop(sArg);
    EnsureMalleable(&sArg);
    char* s = LockString(&sArg);
    while (*s) {
        if (*s >= 'a' && *s <= 'z') *s-=('a' - 'A');
        s++;
    }
    UnlockString(&sArg);
    retVal = sArg;
}
    
void lib_strlwr(short) {
    Value sArg;
    pop(sArg);
    EnsureMalleable(&sArg);
    char* s = LockString(&sArg);
    while (*s) {
        if (*s >= 'A' && *s <= 'Z') *s+=('a' - 'A');
        s++;
    }
    UnlockString(&sArg);
    retVal = sArg;
}

// short strtok(sting src, pointer res, string delim, short first)
void lib_strtok(short) {
    Value first, delim, res, src;
    pop(first);
    pop(delim);
    pop(res);
    pop(src);
    
    char* d = LockString(&delim);
    short dlen = strlen(d);
    char* s = LockString(&src);
    short slen = strlen(s);
    
    if (res.iVal == 0) {
        short tok = 1;
        // return count
        for (short i=first.iVal;i<slen;i++) {
            for (short j=0;j<dlen;j++) {
                if (s[i]==d[j]) {
                    tok++;
                    break;
                }
            }
        }
        retVal.iVal = tok;
    } else {
        Value* pRes = deref(res.iVal);
        short last = first.iVal;
        ReleaseString(pRes);
        emptyString(pRes);
        if (last > slen) { // this points AFTER the null
            retVal.iVal = -1;
        } else {
            for (short i=first.iVal;i<slen;i++) {
                for (short j=0;j<dlen;j++) {
                    if (s[i]==d[j]) {
                        last = i;
                        goto foundit;
                    }
                }
            }
            last = slen;
foundit:
            short n = last-first.iVal;
            char* dst = NewString(pRes, n);
            if (!dst) rt_oom();
            for (short j=0;j<n;j++)
                dst[j] = s[j+first.iVal];
            dst[n] = '\0';
            UnlockString(pRes);
            retVal.iVal = last+1;
        }
    }
    UnlockReleaseString(&delim);
    UnlockReleaseString(&src);
}

#if 0
void ______Sound______() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// Sound routines
//
void lib_beep(short) {
    Value arg;
    pop(arg);
    if (arg.iVal < 1 || arg.iVal > 7) arg.iVal = 1;
    SndPlaySystemSound((SndSysBeepType)arg.iVal);
}

void lib_tone(short) {
    Value freq, dur;
    pop(dur);
    pop(freq);
    SndCommandType cmd;
    cmd.cmd = sndCmdFreqDurationAmp;
    cmd.param1 = freq.iVal;
    cmd.param2 = dur.iVal;
    cmd.param3 = sndMaxAmp;
    if (freq.iVal > 0 && dur.iVal > 0) SndDoCmd(0, &cmd, false);
}

void lib_getvol(short) {
    Value v;
    pop(v);
    short id = prefSysSoundVolume;
    if (v.iVal == 1)
        id = prefGameSoundVolume;
    else if (v.iVal == 2)
        id = prefAlarmSoundVolume;
        
    if (romVersion >= ver30)
        retVal.iVal = PrefGetPreference((SystemPreferencesChoice)id);
    else
        retVal.iVal = sndMaxAmp;
}

void lib_tonea(short) {
    Value freq, dur, vol;
    pop(vol);
    pop(dur);
    pop(freq);
    SndCommandType cmd;
    cmd.cmd = sndCmdFrqOn;
    cmd.param1 = freq.iVal;
    cmd.param2 = dur.iVal;
    cmd.param3 = vol.iVal;
    if (romVersion >= ver30) {
        if (freq.iVal > 0 && dur.iVal > 0 && vol.iVal > 0) SndDoCmd(0, &cmd, false);
        else { cmd.cmd = sndCmdQuiet; SndDoCmd(0, &cmd, false); }
    }
}


#if 0
void ______Event______() {}
#endif
////////////////////////////////////////////////////////////////////////
//
// Event routines
//
short penx=0, peny=0, npenx=0,npeny=0;
extern UInt32 density;

void lib_wait(short) {
    // Wait for either a pen or key message
    eventQueue.RemoveInput();
tryAgain:
    do {
        while (!eventQueue.PopEvent())
            UIYield(true);
    } while (eventQueue.type != EventQueue::eqPenUp && eventQueue.type != EventQueue::eqKey);
    if (eventQueue.type == EventQueue::eqPenUp) {
        RetInt(-1);
        if (lastEvent.screenY > 159) goto tryAgain;
        penx = eventQueue.v1;
        peny = eventQueue.v2;
        npenx = penx * density / 72;
        npeny = peny * density / 72;
    } else {
        retVal.type = vtChar;
        retVal.cVal = eventQueue.v1;
    }
    //eventQ = 0;
}

void lib_waitp(short) {
    // Wait for a pen message
    eventQueue.RemoveInput();
    do {
        while (!eventQueue.PopEvent())
            UIYield(true);
    } while (eventQueue.type != EventQueue::eqPenUp || eventQueue.v2 > 159);
    //eventQ = 0;
    penx = eventQueue.v1;
    peny = eventQueue.v2;
    npenx = penx * density / 72;
    npeny = peny * density / 72;
}

void lib_getc(short) {
    // Wait for a key message
    do {
        while (!eventQueue.PopEvent())
            UIYield(true);
    } while (eventQueue.type != EventQueue::eqKey);
    retVal.type = vtChar;
    retVal.cVal = eventQueue.v1;
    //eventQ = 0;
}

void lib_pstate(short) {
    SWord x, y;
    Boolean b;
    EvtGetPen(&x, &y, &b);
    penx=x; peny=y;
    npenx = penx * density / 72;
    npeny = peny * density / 72;
    RetInt(b);
}

#include <KeyMgr.h>
void lib_bstate(short) {
    DWord res = KeyCurrentState();
    if (res & keyBitPageUp) retVal.iVal = 1;
    if (res & keyBitPageDown) retVal.iVal = -1;
}

short lastkey;
void lib_event(short) {
    Value time;
    pop(time);
    if (!eventQueue.PopEvent()) {
        UIYield(time.iVal);
        if (!eventQueue.PopEvent()) {
            return;
        }
    }
    if (eventQueue.type == EventQueue::eqKey) lastkey = eventQueue.v1;
    else if (eventQueue.type >= EventQueue::eqPenDown && eventQueue.type <= EventQueue::eqPenMove) {
        penx = eventQueue.v1;
        peny = eventQueue.v2;
        npenx = penx * density / 72;
        npeny = peny * density / 72;
    } else if (eventQueue.type == EventQueue::eqResize) {
        penx = eventQueue.v1;
        peny = eventQueue.v2;
    }
    RetInt(eventQueue.type);
    //eventQ=0;
}

extern bool bHookHard, bHookMenu, bHookSync, bHookSilk;
void lib_hardkeys(short) {
    Value hook;
    pop(hook);
    bHookHard = hook.iVal;
}

void lib_hookmenu(short) {
    Value hook;
    pop(hook);
    bHookMenu = hook.iVal;
}

void lib_hooksilk(short) {
    Value hook;
    pop(hook);
    bHookSilk = hook.iVal;
}

void lib_hooksync(short) {
    Value hook;
    pop(hook);
    bHookSync = hook.iVal;
}

void lib_key(short) {
    retVal.type = vtChar;
    retVal.cVal = lastkey;
}

void lib_penx(short) {
    RetInt(penx); 
}

void lib_peny(short) {
    RetInt(peny);
}

void lib_npenx(short) {
    RetInt(npenx); 
}

void lib_npeny(short) {
    RetInt(npeny);
}

void lib_enableresize(short) {
    enableResize = true;
}

#include "lib_draw.h"
#include "lib_db.h"

#if 0
void _____System_____() {}
#endif
/////////////////////////////////////////////////////////
//
// System
//
void lib_sleep(short) {
    Value ms;
    pop(ms);
    // cast to short to avoid calls to _lmul_ _ldiv_
    ULong ntick = ms.iVal / (1000 / SysTicksPerSecond());
    ULong start = TimGetTicks();
    while (TimGetTicks() - start < ntick) {
        SysTaskDelay(5);
        UIYield(false);
    }
}

void lib_deepsleep(short) {
    Value sec;
    pop(sec);
    
    // TODO: this won't work for standalone apps
    // actually, this does seem to work. Since PocketC
    // ignores the launch code, the result is just that the
    // device wakes up
    LocalID lid = DmFindDatabase(0, "PocketC");
    AlmSetAlarm(0, lid, 0, TimGetSeconds() + sec.iVal, true);
    EventType event;
    event.eType=keyDownEvent;
    event.data.keyDown.chr=autoOffChr;
    event.data.keyDown.keyCode=autoOffChr;
    event.data.keyDown.modifiers=commandKeyMask;
    EvtAddEventToQueue(&event); // Put the message back in the Q
    UIYield(false);
}

void lib_resetaot(short) {
    EvtResetAutoOffTimer();
}

void lib_getsysval(short) {
    Value id;
    pop(id);
    switch (id.iVal) {
        case 0:	{
            // username
            DlkGetSyncInfo(NULL, NULL, NULL, msg, NULL,	NULL);
            if (!*msg) strcpy(msg, "");
            if (!NewStringFromConst(&retVal, msg))
                emptyString(&retVal);
            break;
        }
        case 1:	{
            // os version
            retVal.iVal	= romVersion;
            break;
        }
        case 2:	{
            // os version string
            char* ver =	SysGetOSVersionString();
            if (!NewStringFromConst(&retVal, ver))
                emptyString(&retVal);
            MemPtrFree(ver);
            break;
        }
        case 3:	{
            // serial number
            if (romVersion >= 0x03003000) {
                char* sn = NULL;
                UInt16 len = 0;
                Err	ret	= SysGetROMToken(0,	sysROMTokenSnum, (UInt8**)&sn, &len);
                if (ret	== 0 &&	sn != NULL && (UInt8)*sn !=	0xff) {
                    char* data = NewString(&retVal, len);
                    if (data) {
                        strncpy(data, sn, len);
                        data[len] =	0;
                        MemHandleUnlock(retVal.sVal);
                    } else {
                        emptyString(&retVal);
                    }
                } else {
                    emptyString(&retVal);
                }
            } else {
                emptyString(&retVal);
            }
            break;
        }
        
        case 4:
            RetInt(SysTicksPerSecond());
            break;

        default:
            vm_error("unsupported arg to getsysval()", -1);
    }
}

void lib_exit(short) {
    if (romVersion >= 0x03003000) {   
        EventType event;
        event.eType=keyDownEvent;
        event.data.keyDown.chr=launchChr;
        event.data.keyDown.keyCode=launchChr;
        event.data.keyDown.modifiers=commandKeyMask;
        EvtAddEventToQueue(&event); // Put the message back in the Q
    }
    bHookSilk = false; // don't let this get in the way!
    UIYield(false);
    killEvents = true;
    //if (atexit_func) {
    //	CallFunc(atexit_func);
    //}
    killVM = true;
}

void lib_atexit(short) {
    Value ptr;
    pop(ptr);
    
    if (ptr.iVal < 0 || ptr.iVal > codePtr)
        vm_error("Invalid function pointer in atexit()", pc);
    atexit_func = ptr.iVal;
}

void lib_version(short) {
    RetInt(714);
}

void lib_launch(short) {
    UInt					cardNo;
    LocalID				dbID;
    DmSearchStateType	searchState;
    Err err;
    
    ULong appCreator;
    Value id;
    
    pop(id);
    
    appCreator = getTC(&id);
    
    DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
        appCreator, true,	&cardNo, &dbID);

    err = 1;
    if (dbID) err = SysUIAppSwitch(cardNo, dbID, 0, 0);

    cleanup(id);
    if (!err) {
        killEvents = true;
        //if (atexit_func)
        //	CallFunc(atexit_func);
        killVM = true;
    }
}

void lib_clipget(short) {
    Word len;
    VoidHand clp = ClipboardGetItem(clipboardText, &len);
    if (clp && len) {
        // do strdup here, since the string isn't NULL terminated
        char* src = (char*)MemHandleLock(clp);
        char* dest = NewString(&retVal, len+1);
        if (!dest)
            rt_oom();
        for (Word i=0;i<len;i++)
            dest[i] = src[i];
        dest[len] = 0;
        UnlockString(&retVal);
        MemHandleUnlock(clp);
    }
    else emptyString(&retVal);
    retVal.type = vtString;
}

void lib_clipset(short) {
    Value text;
    pop(text);
    char* pt = LockString(&text);
    ClipboardAddItem(clipboardText, pt, strlen(pt));
    UnlockReleaseString(&text);
}

#if 0
void _____Serial_____() {}
#endif
/////////////////////////////////////////////////////////
//
// Serial I/O routines
//

/*
Nope, only if you want to do a hardware loopback.
See the srmCtlRxEnable & srmCtlIrDAEnable commands for the SrmControl() API.
There may be some additional stuff you need for the SrmOpen() call.  For the
old serial manager use serCtlIrDAEnable & serCtlRxEnable for SerControl().
*/
Long ctstimeout;
bool oldSerial = false;
bool serIr = false;
UInt16 portId = 0;

//seropenx(short port, short baud)

void lib_seropenx(short) {
    Value port, baud;
    pop(baud);
    pop(port);
    
    // if flags are present, parse them
    if (serOpen) {
        RetInt(-1);
        return;
    }
    
    Err err = SrmOpen(port.iVal, baud.iVal, &portId);
    RetInt(err);
    if (err == 0) {
        serOpen = true;
        oldSerial = false;
        serIr = false;
        ctstimeout = 200;
        if (port.iVal == serPortIrPort) {
            SrmControl(portId, srmCtlIrDAEnable, NULL, NULL);
            SrmControl(portId, srmCtlRxEnable, NULL, NULL);
            SrmReceiveFlush(portId, 1);
            serIr = true;
        }
    }
}

UInt32 parseFlags(char* flagstr) {
    char fstr[4] = {0,0,0,0};

    for (short i=0;i<4;i++)
        if (*flagstr)
            fstr[i] = lower(*flagstr++);
        else
            break;
            
    UInt32 f = 0;
    // bits per char
    if (fstr[0]=='6') f |= serSettingsFlagBitsPerChar6;
    else if (fstr[0]=='7') f |= serSettingsFlagBitsPerChar7;
    else f |= serSettingsFlagBitsPerChar8;
    // parity
    if (fstr[1]=='e') f |= (serSettingsFlagParityOnM | serSettingsFlagParityEvenM);
    else if (fstr[1]=='o') f |= serSettingsFlagParityOnM;
    // stop bits
    if (fstr[2]=='2') f |= serSettingsFlagStopBits2;
    // handshaking
    if (fstr[3]=='x') f |= serSettingsFlagXonXoffM;
    else if (fstr[3]=='r') f |= serSettingsFlagRTSAutoM;
    else if (fstr[3]=='c') f |= serSettingsFlagCTSAutoM;
    else if (fstr[3]=='h') f |= serSettingsFlagRTSAutoM | serSettingsFlagCTSAutoM;
    
    return f;
}

void lib_sersettings(short) {
    Value flags, timeout;
    pop(timeout);
    pop(flags);
    
    if (serOpen && !oldSerial) {
        UInt32 f = parseFlags(LockString(&flags));
        UnlockString(&flags);
        
        UInt16 size = sizeof(UInt32);
        SrmControl(portId, srmCtlSetFlags, &f, &size);
        SrmControl(portId, srmCtlSetCtsTimeout, &timeout.iVal, &size);
    }
    
    cleanup(flags);
}

void lib_seropen(short) {
    Value baud, flags, timeout;
    pop(timeout);
    pop(flags);
    pop(baud);

    char fstr[4] = {0,0,0,0};

    if (SerLibRef && !serOpen) {
        UInt32 f = parseFlags(LockString(&flags));
        UnlockString(&flags);

        retVal.iVal = SerOpen(SerLibRef, 0, baud.iVal);
        SerSettingsType settings;
        if (!retVal.iVal || retVal.iVal == serErrAlreadyOpen) {
            SerGetSettings(SerLibRef, &settings);
            settings.flags = f;
            ctstimeout = settings.ctsTimeout = timeout.iVal;
            SerSetSettings(SerLibRef, &settings);
            serOpen = true;
            oldSerial = true;
        }
    } else retVal.iVal = -1;
    cleanup(flags);
}

void* serBuffer;
void lib_serclose(short) {
    if (serOpen) {
        if (serBuffer) {
            MemPtrFree(serBuffer);
            serBuffer = NULL;
            if (oldSerial)
                SerSetReceiveBuffer(SerLibRef, 0, 0);
            else
                SrmSetReceiveBuffer(portId, 0, 0);
        }
        if (oldSerial)
            SerClose(SerLibRef);
        else
            SrmClose(portId);
    }
    serOpen=false;
}

/*
// pin 0 - DTR
// pin 1 - RTS
void lib_sersetpin(short) {
    Value pin, val;
    pop(val);
    pop(pin);
    
    if (serOpen && !oldSerial) {
        if (pin.iVal == 0) {
            // DTR
            UInt16 size = sizeof(Boolean);
            Boolean bVal = !!val.iVal;
            SrmControl(portId, srmCtlSetDTRAsserted, &bVal, &size);
        } else if (pen.iVal == 1) {
        }
    }
    UInt16 len = sizeof(Boolean);
    
}

void lib_sergetpin(short) {
    Value pin;
    pop(pin);
}
*/
// serbuffsize
void lib_serbuffsize(short) {
    Value size;
    pop(size);
    
    if (serOpen) {
        void* buff = MemPtrNew(size.iVal + 32);
        if (buff) {
            if (oldSerial)
                SerSetReceiveBuffer(SerLibRef, buff, size.iVal + 32);
            else
                SrmSetReceiveBuffer(portId, buff, size.iVal);
            if (serBuffer) MemPtrFree(serBuffer);
            serBuffer = buff;
            retVal.iVal = 1;
        }
    }
}

void lib_sersend(short) {
    Value byte;
    pop(byte);
    
    if (serOpen) {
        if (oldSerial) {
            retVal.iVal = SerSend10(SerLibRef, &byte.cVal, 1);
            SerClearErr(SerLibRef);
        } else {
            Err err = 0;
            if (serIr)
                SrmControl(portId, srmCtlRxDisable, NULL, NULL);
            SrmSend(portId, &byte.cVal, 1, &err);
            if (serIr) {
                SrmSendWait(portId);
                SrmControl(portId, srmCtlRxEnable, NULL, NULL);
            }
            retVal.iVal = err;
            SrmClearErr(portId);
        }
    } else retVal.iVal = -1;
}

void lib_serrecv(short) {
    unsigned char byte;
    if (serOpen) {
        Err err;
        if (oldSerial) {
            err = SerReceive10(SerLibRef, &byte, 1, ctstimeout);
            SerClearErr(SerLibRef);
        } else {
            SrmReceive(portId, &byte, 1, ctstimeout, &err);
            SrmClearErr(portId);
        }
        retVal.iVal = byte;
        if (err) retVal.iVal += 0x100 * err;
    } else retVal.iVal = 256;
}

void lib_serdata(short) {
    ULong size = 0;
    if (serOpen) {
        if (oldSerial)
            SerReceiveCheck(SerLibRef, &size);
        else
            SrmReceiveCheck(portId, &size);
        retVal.iVal = size;
    }
}

//void lib_sersends(short) {
    // send a string
//}

void lib_sersenda(short) {
    // send an array
    Value ptr, n;
    pop(n);
    pop(ptr);
    
    unsigned char* data = NULL;
    
    for (short i=0;i<n.iVal;i++) {
        VarType vt = deref(ptr.iVal + i)->type;
        if (vt != vtInt && vt != vtChar)
            vm_error("sersenda() requires an array of ints/chars", pc);
    }
            
    data = new unsigned char[n.iVal];
    if (!data)
        rt_oom();
        
    for (short i=0;i<n.iVal;i++) {
        Value* p = deref(ptr.iVal + i);
        if (p->type == vtInt)
            data[i] = p->iVal;
        else
            data[i] = p->cVal;
    }
    
    if (serOpen) {
        if (oldSerial) {
            retVal.iVal = SerSend10(SerLibRef, data, n.iVal);
            SerClearErr(SerLibRef);
        } else {
            Err err = 0;
            if (serIr)
                SrmControl(portId, srmCtlRxDisable, NULL, NULL);
            SrmSend(portId, data, n.iVal, &err);
            if (serIr) {
                SrmSendWait(portId);
                SrmControl(portId, srmCtlRxEnable, NULL, NULL);
            }
            retVal.iVal = err;
            SrmClearErr(portId);
        }
    } else retVal.iVal = -1;
    
    delete data;
}

void lib_serrecva(short) {
    // receive an array
    Value ptr, n;
    pop(n);
    pop(ptr);
    
    unsigned char* data = NULL;
    
    for (short i=0;i<n.iVal;i++) {
        VarType vt = deref(ptr.iVal + i)->type;
        if (vt != vtInt && vt != vtChar)
            vm_error("serrecva() requires an array of ints/chars", pc);
    }
            
    data = new unsigned char[n.iVal];
    if (!data)
        rt_oom();
        
    if (serOpen) {
        Err err;
        if (oldSerial) {
            err = SerReceive10(SerLibRef, data, n.iVal, ctstimeout);
            SerClearErr(SerLibRef);
        } else {
            SrmReceive(portId, data, n.iVal, ctstimeout * n.iVal, &err);
            SrmClearErr(portId);
        }
        retVal.iVal = err;
        for (short i=0;i<n.iVal;i++) {
            Value* p = deref(ptr.iVal + i);
            if (p->type == vtInt)
                p->iVal = data[i];
            else
                p->cVal = data[i];
        }
    } else retVal.iVal = -1;
    
    delete data;
}

void lib_serwait(short) {
    Value timeout, size;
    pop(timeout);
    pop(size);
    
    if (serOpen) {
        Err err;
        if (oldSerial)
            err = SerReceiveWait(SerLibRef, size.iVal, timeout.iVal);
        else
            err = SrmReceiveWait(portId, size.iVal, timeout.iVal);
        if (err == serErrTimeOut) retVal.iVal = 0;
        else if (err == serErrLineErr) {
            retVal.iVal = -1;
            if (oldSerial)
                SerClearErr(SerLibRef);
            else
                SrmClearErr(portId);
        }
        else retVal.iVal = 1;
    }
}

unsigned long cardinalVal(Value* pv) {
    if (pv->type == vtInt)
        return (unsigned long)pv->iVal;
    return (unsigned long)(unsigned char)pv->cVal;
}

void lib_unpack(short) {
    Value dst, src, types, count;
    pop(count);
    pop(types);
    pop(src);
    pop(dst);
    
    unsigned long val;
    
    Value* pSrc, *pDst;
    char* pTypes = LockString(&types);
    char* pOrigTypes = pTypes;
    
    while (count.iVal) {
        pDst = deref(dst.iVal);
        pSrc = deref(src.iVal);
        val = 0;
        bool be = true;
        if (*pTypes == '<') {
            be = false;
            pTypes++;
        }
        switch (*pTypes) {
            case '1':
                val = cardinalVal(pSrc);
                src.iVal++;
                count.iVal--;
                break;
            case '2':
                deref(src.iVal+1);
                if (be) {
                    val = (cardinalVal(pSrc) << 8) | cardinalVal(pSrc+1);
                } else {
                    val = (cardinalVal(pSrc + 1) << 8) | cardinalVal(pSrc);
                }
                src.iVal+=2;
                count.iVal-=2;
                break;
            case '4':
                deref(src.iVal+3);
                if (be) {
                    val = (cardinalVal(pSrc) << 24) | (cardinalVal(pSrc+1) << 16) | (cardinalVal(pSrc+2) << 8) | cardinalVal(pSrc+3);
                } else {
                    val = (cardinalVal(pSrc+3) << 24) | (cardinalVal(pSrc+2) << 16) | (cardinalVal(pSrc+1) << 8) | cardinalVal(pSrc);
                }
                src.iVal+=4;
                count.iVal-=4;
                break;
            default:
                UnlockReleaseString(&types);
                vm_error("error in unpack() types", pc);
        }
        pTypes++;
        if (*pTypes == NULL)
            pTypes = pOrigTypes;
            
        if (pDst->type == vtInt)
            pDst->iVal = val;
        else
            pDst->cVal = val;
        dst.iVal++;
    }

    UnlockReleaseString(&types);
    return;
}

#if 0
void _____Memory_____() {}
#endif

short nBlocks, _nBlocks;
Handle blocks_h;

void insertBlock(short** bpp, short after) {
    short* bp = *bpp;
    if (nBlocks + 1 > _nBlocks) {
        MemHandleUnlock(blocks_h);
        if (MemHandleResize(blocks_h, (_nBlocks+10) * sizeof(short)))
            rt_oom();
        bp = (short*)MemHandleLock(blocks_h);
        _nBlocks+=10;
    }
    if (after < nBlocks - 1)
        MemMove(&bp[after+2], &bp[after+1], (nBlocks - after - 1) * sizeof(short));
    nBlocks++;
    *bpp = bp;		
}

void removeBlock(short* bp, short index) {
    if (index < nBlocks - 1)
        MemMove(&bp[index], &bp[index+1], (nBlocks - index - 1) * sizeof(short));
    nBlocks--;
}

short vmmalloc(short size) {
    short* bp = (short*)MemHandleLock(blocks_h);
    short i=0, loc=0, len;
    while (i<nBlocks) {
        len = bp[i] & 0x7fff;
        if ((bp[i] & 0x8000) && len >= size) {
            if (len == size) {
                // It is the exact size
                bp[i] = len;
                goto foundIt;
            } else {
                // It's too big
                short dif = len - size;
                bp[i] = size;
                insertBlock(&bp, i);
                bp[i+1] = 0x8000 | dif;
                goto foundIt;
            }
        }
        loc += len;
        i++;
    }
    // No room, add more global space
    // Unlock stuff to ease reallocation
    MemHandleUnlock(global_h);
    MemHandleUnlock(stack_h);
    if (MemHandleResize(global_h, (globalOff + size) * sizeof(Value))) {
        // Out of space. Doh!!!
        loc = 0;
    } else {
        insertBlock(&bp, nBlocks - 1); // Add a block (this modifies nBlocks)
        bp[nBlocks - 1] = size;
        globalOff += size;
    }
    stack = (Value*)MemHandleLock(stack_h);
    globals = (Value*)MemHandleLock(global_h);

foundIt:
    MemHandleUnlock(blocks_h);
    return loc;	
}

void lib_malloc(short) {
    Value size;
    pop(size);
    
    short loc = vmmalloc(size.iVal);
    if (loc) {
        for (;size.iVal;size.iVal--)
            globals[loc + size.iVal-1].type = vtInt;
    }
    retVal.iVal = loc;
}

/*
void lib_malloct(short) {
    Value n, types;
    pop(types);
    pop(n);
    
    if (n.iVal == 0) {
        cleanup(types);
        return;
    }
    
    char* pt = LockString(types.sVal);
    short len = strlen(pt);
    if (len == 0) {
        goto its_a_type_error;
    }

    for (short i=0;i<len;i++) {
        switch (pt[i]) {
            case 'i':
            case 'p':
            case 's':
            case 'c':
            case 'f':
                break;
            default:
                if (!isdigit(pt[i]))
                    goto its_a_type_error;
        }
    }
    UnlockString(types.sVal);
    short cloc, loc = vmmalloc(n.iVal * len);
    cloc = loc;
    
    if (loc) {
        pt = LockString(types.sVal);
        for (short i=0;i<n.iVal;i++) {
            for (short j=0;j<len;j++) {
                switch (pt[j]) {
                    case 'p':
                    case 'i':
                        globals[cloc].type = vtInt;
                        globals[cloc].iVal = 0;
                        break;
                    case 'c':
                        globals[cloc].type = vtChar;
                        globals[cloc].cVal = 0;
                        break;
                    case 'f':
                        globals[cloc].type = vtFloat;
                        globals[cloc].fVal = 0;
                        break;
                    case 's':
                        globals[cloc].type = vtString;
                        globals[cloc].sVal = emptyString();
                        break;
                }
                cloc++;
            }
        }
        UnlockReleaseString(types.sVal);
        RetInt(loc);
    }
    return;

its_a_type_error:
    UnlockReleaseString(types.sVal);
    vm_error("error in malloct() type", pc);
}
*/

void lib_malloct(short) {
    Value n, types;
    pop(types);
    pop(n);
    
    if (n.iVal == 0) {
        cleanup(types);
        return;
    }
    
    char* pt = LockString(&types);
    short len = 0;
    if (!*pt)
        goto its_a_type_error;

    {
        Format format(pt, 1, false, "error in malloct() type");
        VarType vt; long l;
        while (format.Next(byref vt, byref l)) len++;
    }
    UnlockString(&types);
    short cloc, loc = vmmalloc(n.iVal * len);
    cloc = loc;
    
    if (loc) {
        pt = LockString(&types);
        Format format(pt, n.iVal, false, "error in malloct() type");
        VarType vt; long l;
        
        while (format.Next(byref vt, byref l)) {
            globals[cloc].type = vt;
            globals[cloc].iVal = 0;
            if (vt == vtString) {
                emptyString(&globals[cloc]);
            }
            cloc++;
        }
        UnlockReleaseString(&types);
        RetInt(loc);
    }
    return;

its_a_type_error:
    UnlockReleaseString(&types);
    vm_error("error in malloct() type", pc);
}

void lib_free(short) {
    Value ptr;
    pop(ptr);
    if (!ptr.iVal || ptr.iVal >= 0x6000) return;
    short* bp = (short*)MemHandleLock(blocks_h);
    
    short loc=0, len;
    for (short i=0;i<nBlocks && loc <= ptr.iVal;i++) {
        if (loc == ptr.iVal) {
            len = bp[i] & 0x7fff;
            if (bp[i] & 0x8000) goto done; // Already free'd
            bp[i] |= 0x8000;
            if (bp[i-1] & 0x8000) { // i-1 is always >= 0
                // Merge with previous block
                bp[i-1] = 0x8000 | ((bp[i-1] & 0x7fff) + len);
                removeBlock(bp, i);
                i--; // So that we can merge with the next block
            }
            if (i+1 < nBlocks && (bp[i+1] & 0x8000)) {
                // Merge with next block
                bp[i] = 0x8000 | (((bp[i] & 0x7fff) + (bp[i+1] & 0x7fff)));
                removeBlock(bp, i+1);
            }
            // Cleanup the data
            for (;len;len--) {
                cleanup(globals[loc+len-1]);
                globals[loc+len-1].type = vtInt;
            }
            goto done;
        }
        loc += (bp[i] & 0x7fff);
    }
done:
    MemHandleUnlock(blocks_h);
}

void lib_settype(short) {
    Value ptr, len, type;
    pop(type);
    pop(len);
    pop(ptr);
    VarType newType;
    if (len.iVal <= 0 || ptr.iVal <= 0) return; // Invalid pointers
    // Global pointer bad
    if (ptr.iVal < 0x6000 && ptr.iVal + len.iVal > globalOff) return;
    // Local pointer bad
    if (ptr.iVal >= 0x6000 && (ptr.iVal - 0x6000 + len.iVal > st)) return;
    switch (type.cVal) {
        case 'c': newType = vtChar; break;
        case 'i': newType = vtInt; break;
        case 'f': newType = vtFloat; break;
        case 's': newType = vtString; break;
        default:
            return;
    }
    long addr = ptr.iVal;
    for (short i=0;i<len.iVal;i++) {
        Value* v = deref(addr++);
        cleanup(*v);
        v->type = newType;
        if (newType == vtString)
            emptyString(v);
    }
    retVal.iVal = 1;
}

void lib_typeof(short) {
    Value ptr;
    pop(ptr);
    
    Value* v = deref(ptr.iVal);
    switch (v->type) {
        case vtInt: retVal.cVal = 'i'; break;
        case vtChar: retVal.cVal = 'c'; break;
        case vtFloat: retVal.cVal = 'f'; break;
        case vtString: retVal.cVal = 's'; break;
        default: retVal.cVal = 'o';
    }
    retVal.type = vtChar;
}

void lib_memcpy(short) {
    Value dest, src, num;
    pop(num);
    pop(src);
    pop(dest);
    
    for (short i=0;i<num.iVal;i++) {
        Value* pDest = deref(dest.iVal+i);
        cleanup(*pDest);
        copyVal(pDest, deref(src.iVal+i));
    }
    
}

#define alertMsgXPos			32
#define alertXMargin			8
#define alertYMargin			7
#define minButtonWidth			36
#define alertButtonMargin		4
#define alertMaxTextLines 		10
#define alertTextLinesForField	2
#define gsiWidth				12
#define frameWidth				0
#define maxTextInput			256

// System bitmaps
#define palmLogoBitmap					10000
#define InformationAlertBitmap 			10004
#define ConfirmationAlertBitmap			10005
#define WarningAlertBitmap				10006
#define ErrorAlertBitmap				10007

// Custom Alert Dialog
#define CustomAlertDialog					12000
#define CustomAlertTextEntryField		12001
#define CustomAlertBitmap					12002
#define CustomAlertField					12003
#define CustomAlertFirstButton			12004

#define max(a,b) ((a) > (b) ? (a) : (b))
#define min(a,b) ((a) < (b) ? (a) : (b))

FormPtr BuildAlert(const char* title, const char* text, 
    const char* buttonStr, bool bInput, int alertType)
{
    Int16	alertMaxMsgLines = alertMaxTextLines - (bInput ? alertTextLinesForField : 0);
    Coord x, y;
    Coord alertHeight;
    Coord displayWidth;
    Coord displayHeight;
    Coord alertMsgYPos;
    UInt16 i;
    Int16 msgHeight;
    FontID curFont;
    FormPtr pForm;
    UInt16 bitmapID;
    FieldPtr pField;
    ControlPtr pCtrl;
    Int16	spaceLeft;
    Boolean smallButtons;

    // Compute the height the message string.
    WinGetDisplayExtent(&displayWidth, &displayHeight);
    alertMsgYPos = FntLineHeight() + 1 + alertYMargin; // title + margin
    
    // create the form
    pForm = FrmNewForm(CustomAlertDialog, title, 2, 2,
        displayWidth - 4, displayHeight - 4, true, CustomAlertFirstButton, 0, 0);
    
    // add the alert icon
    switch (alertType) {
        case informationAlert: bitmapID = InformationAlertBitmap; break;
        case confirmationAlert: bitmapID = ConfirmationAlertBitmap; break;
        case warningAlert: bitmapID = WarningAlertBitmap; break;
        case errorAlert: bitmapID = ErrorAlertBitmap; break;
    }
    FrmNewBitmap (&pForm, CustomAlertBitmap, bitmapID, alertXMargin, alertMsgYPos);
    
    // add a field for the message
    curFont = FntSetFont(boldFont);
    pField = FldNewField((void**)&pForm, CustomAlertField, alertMsgXPos, alertMsgYPos, displayWidth - alertMsgXPos - 6, 
        FntLineHeight() * alertMaxMsgLines, boldFont, 0, false, false, 
        false, false, leftAlign, false, false, false);
    FldSetTextPtr(pField, (char*)text);
    FldRecalculateField(pField, true);
    msgHeight = max (FldGetTextHeight(pField), (FntLineHeight() * 2));
    RectangleType rect;
    FldGetBounds(pField, &rect);
    rect.extent.y = msgHeight;
    FldSetBounds(pField, &rect);
    
    // add the text entry field and graffiti indicator
    x = alertButtonMargin + 1;								  // 1 for the frame
    y = alertMsgYPos + msgHeight + alertYMargin + 2;
    FntSetFont (stdFont);

    if (bInput) {
        pField = FldNewField((void**)&pForm, CustomAlertTextEntryField, x, y,
            displayWidth - ((alertButtonMargin + 1) * 2) - 1, FntLineHeight(),
            stdFont, maxTextInput - 1, true, true, true, false,
            leftAlign, false, false, false);
        
        y += FntLineHeight() + alertYMargin;
        if (romVersion >= ver35)
            FrmNewGsi(&pForm, displayWidth - (gsiWidth + frameWidth + alertButtonMargin),y + alertButtonMargin);
    }
    
    // add the buttons
    // split the buttons out by : separators
    const int maxButtons = 6;
    const int maxButtonText = 64;
    const char buttonSep = ':';

    int nButtons = 0;
    char buttons[maxButtons][maxButtonText];
    char* iter = (char*)buttonStr;
    int ich = 0;
    while (*iter && nButtons < maxButtons) {
        if (*iter != ':') {
            if (ich < maxButtonText - 1) {
                buttons[nButtons][ich++] = *iter;
            }
        } else {
            buttons[nButtons][ich] = 0;
            ich = 0;
            nButtons++;
        }
        iter++;
    }
    
    if (nButtons < maxButtons) {
        buttons[nButtons][ich] = 0;
        nButtons++;
    }
    
    smallButtons = false;
    for (i = 0; i < nButtons; i++) {
        pCtrl = CtlNewControl((void**)&pForm, CustomAlertFirstButton + i, buttonCtl,
            buttons[i], x, y, 0, 0, stdFont, 0, true);
        
        UInt16 index = FrmGetObjectIndex(pForm, CustomAlertFirstButton + i);
        FrmGetObjectBounds(pForm, index, &rect);
        x += rect.extent.x + alertButtonMargin + 2;  // 2 for frames
        if (rect.extent.x < minButtonWidth)
            smallButtons = true; //we may have to come back and enlarge
    }

    //Add width to small buttons if there's room
    spaceLeft = displayWidth - (alertButtonMargin + x + frameWidth);
    if (bInput)
        spaceLeft -= gsiWidth;
    if (spaceLeft > 0 && smallButtons) {
        Int16	spaceAdded = 0;
        Int16	oldWidth;
        
        for (i = 0; i < nButtons; i++) {
            UInt16 index = FrmGetObjectIndex(pForm, CustomAlertFirstButton + i);
            FrmGetObjectBounds(pForm, index, &rect);
            if (spaceAdded)
                rect.topLeft.x += spaceAdded;
            if (spaceLeft && rect.extent.x < minButtonWidth) {
                oldWidth = rect.extent.x;
                rect.extent.x = min(oldWidth + spaceLeft, minButtonWidth);
                spaceAdded += rect.extent.x - oldWidth;
                spaceLeft -= rect.extent.x - oldWidth;
            }
            FrmSetObjectBounds(pForm, index, &rect);
        }
    }
    
    // Resize the form vertically now that we know the message height.
    alertHeight = y + 17;	// 17 = button height + 4 pixels of white space, that is,
                                    // FntLineHeight(stdFont) + 5, for proper spacing from bottom of form.
    WinHandle wh = FrmGetWindowHandle(pForm);
    if (romVersion < ver40) {
        WinHandle prev = WinSetDrawWindow(wh);
        WinGetDrawWindowBounds(&rect);
        WinSetDrawWindow(prev);
    } else {
        WinGetBounds(wh, &rect);
    }
    rect.topLeft.y = displayHeight - alertHeight - 2;
    rect.extent.y = alertHeight;
    WinSetBounds(wh, &rect);
    
    
    FntSetFont(curFont);						// restore font
    
    return pForm;
}

SndSysBeepType alertTypeToBeep[] = { sndInfo, sndConfirmation, sndWarning, sndError };

void lib_customBase(bool bInput) {
    long addr;
    Value* res;
    if (bInput) {
        Value vAddr;
        pop(vAddr);		
        addr = vAddr.iVal;
        res = deref(addr);
    }
    Value vType; pop(vType);
    int type = vType.iVal;
    Value vButtons; pop(vButtons);
    Value vMessage; pop(vMessage);
    Value vTitle; pop(vTitle);
    char* buttons = LockString(&vButtons);
    char* message = LockString(&vMessage);
    char* title = LockString(&vTitle);
    
    FormPtr pForm = BuildAlert(title, message, buttons, bInput, type);
    UnlockReleaseString(&vButtons);
    UnlockReleaseString(&vTitle);
    UnlockReleaseString(&vMessage);

    FormActiveStateType	curFrmState;
    MenuEraseStatus(0);
    FrmSaveActiveState(&curFrmState);
    FrmSetActiveForm (pForm);
    FrmDrawForm(pForm);
    FieldPtr pField;
    if (bInput) {
        pField = (FieldPtr)FrmGetObjectPtr(pForm, FrmGetObjectIndex(pForm, CustomAlertTextEntryField));
        FrmSetFocus(pForm, FrmGetObjectIndex (pForm, CustomAlertTextEntryField));
    }
    if (type >= 0 && type <= 3)
        SndPlaySystemSound(alertTypeToBeep[type]);
    UInt16 buttonHit = FrmDoDialog(pForm);
    if (bInput) {
        char* str = FldGetTextPtr(pField);
        cleanup(*res);
        if (str)
            NewStringFromConst(res, str);
        else
            emptyString(res);
    }
    FrmDeleteForm (pForm);
    FrmRestoreActiveState(&curFrmState);				// restore active form/window state
    retVal.iVal = (buttonHit - CustomAlertFirstButton);
    killVM = false;
}

void lib_customAlert(short) {
    lib_customBase(false);
}

void lib_customPrompt(short) {
    lib_customBase(true);
}

#ifdef POCKETC_SCANNER
bool bScanOpen;
bool bScanEnabled;
#include "scanner.cpp"
#else
void lib_scanrequired(short) {
    vm_error("Scanner-enabled PocketC runtime required for this app", pc);
}
#endif


/////////////////////////////////////////////////////////
//
// Library initializer
//
const FuncInfo funcTable[] = {
    // Basic I/O routines
    {"alert", 1, 0, lib_msg, vtString}, //, vtVoid, vtVoid, vtVoid, vtVoid, vtVoid, vtVoid, vtVoid, vtVoid, vtVoid}
    {"puts", 1, 0, lib_puts, vtString},
    {"gets", 1, 0, lib_gets, vtString},
    {"clear", 0, 0, lib_clear},

    // String routines
    {"strlen", 1, 0, lib_strlen, vtString},
    {"substr", 3, 0, lib_substr, vtString, vtInt, vtInt},
    {"strleft", 2, 0, lib_left, vtString, vtInt},
    {"strright", 2, 0, lib_right, vtString, vtInt},
    {"strupr", 1, 0, lib_strupr, vtString},
    {"strlwr", 1, 0, lib_strlwr, vtString},
    
    // Math routines
    {"cos", 1, 0, lib_math, vtFloat},
    {"sin", 1, 0, lib_math, vtFloat},
    {"tan", 1, 0, lib_math, vtFloat},
    {"acos", 1, 0, lib_math, vtFloat},
    {"asin", 1, 0, lib_math, vtFloat},
    {"atan", 1, 0, lib_math, vtFloat},
    {"cosh", 1, 0, lib_math, vtFloat},
    {"sinh", 1, 0, lib_math, vtFloat},
    {"tanh", 1, 0, lib_math, vtFloat},
    {"acosh", 1, 0, lib_math, vtFloat},
    {"asinh", 1, 0, lib_math, vtFloat},
    {"atanh", 1, 0, lib_math, vtFloat},
    {"exp", 1, 0, lib_math, vtFloat},
    {"log", 1, 0, lib_math, vtFloat},
    {"log10", 1, 0, lib_math, vtFloat},
    {"sqrt", 1, 0, lib_math, vtFloat},
    {"pow", 2, 0, lib_math2, vtFloat, vtFloat},
    {"atan2", 2, 0, lib_math2, vtFloat, vtFloat},
    {"rand", 0, 0, lib_rand},
    {"random", 1, 0, lib_random, vtInt},
    
    // Sound routines
    {"beep", 1, 0, lib_beep, vtInt},
    {"tone", 2, 0, lib_tone, vtInt, vtInt},
    
    // Graphics routines
    {"graph_on", 0, 0, lib_graph_on},
    {"graph_off", 0, 0, lib_graph_off},
    {"clearg", 0, 0, lib_clearg},
    {"text", 3, 0, lib_text, vtInt, vtInt, vtString},
    {"line", 5, 0, lib_line, vtInt, vtInt, vtInt, vtInt, vtInt},
    {"rect", 6, 0, lib_rect, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt},
    {"title", 1, 0, lib_title, vtString},
    {"textattr", 3, 0, lib_textattr, vtInt, vtInt, vtInt},
    
    // Advanced I/O routines
    {"wait", 0, 0, lib_wait},
    {"waitp", 0, 0, lib_waitp},
    {"getc", 0, 0, lib_getc},
    {"penx", 0, 0, lib_penx},
    {"peny", 0, 0, lib_peny},
    
    // Late additions
    {"hex", 1, 0, lib_hex, vtInt},
    {"frame", 6, 0, lib_frame, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt},

    // Time/Date routines
    {"time", 1, 0, lib_time, vtInt},
    {"date", 1, 0, lib_date, vtInt},
    {"seconds", 0, 0, lib_seconds},
    {"ticks", 0, 0, lib_ticks},
    
    // More late additions
    {"confirm", 1, 0, lib_confirm, vtString},
    {"mathlib", 0, 0, lib_mathlib},
    {"frame2", 7, 0, lib_frame, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt},
    
    // Database I/O
    {"dbopen", 1, 0, lib_dbopen, vtString},
    {"dbcreate", 1, 0, lib_dbcreate, vtString},
    {"dbclose", 0, 0, lib_dbclose},
    {"dbwrite", 1, 0, lib_dbwrite, vtVoid},
    {"dbread", 1, 0, lib_dbread, vtChar},
    {"dbpos", 0, 0, lib_dbpos},
    {"dbseek", 1, 0, lib_dbseek, vtInt},
    {"dbbackup", 1, 0, lib_dbbackup, vtInt},
    {"dbdelete", 0, 0, lib_dbdelete},
    
    // New event routines
    {"event", 1, 0, lib_event, vtInt},
    {"key", 0, 0, lib_key},
    {"pstate", 0, 0, lib_pstate},
    {"bstate", 0, 0, lib_bstate},
    
    // MemoDB I/O
    {"mmnew", 0, 0, lib_mmnew},
    {"mmfind", 1, 0, lib_mmfind, vtString},
    {"mmopen", 1, 0, lib_mmopen, vtInt},
    {"mmclose", 0, 0, lib_mmclose},
    {"mmputs", 1, 0, lib_mmputs, vtString},
    {"mmgetl", 0, 0, lib_mmgetl},
    {"mmeof", 0, 0, lib_mmeor},
    {"mmrewind", 0, 0, lib_mmrewind},
    {"mmdelete", 0, 0, lib_mmdelete},
    
    {"strstr", 3, 0, lib_strstr, vtString, vtString, vtInt},
    {"bitmap", 3, 0, lib_bitmap, vtInt, vtInt, vtString},
    {"sleep", 1, 0, lib_sleep, vtInt},
    {"resetaot", 0, 0, lib_resetaot},
    {"getsysval", 1, 0, lib_getsysval, vtInt},
    {"format", 2, 0, lib_format, vtFloat, vtInt},
    
    // Serial I/O
    {"seropen", 3, 0, lib_seropen, vtInt, vtString, vtInt},
    {"serclose", 0, 0, lib_serclose},
    {"sersend", 1, 0, lib_sersend, vtChar},
    {"serrecv", 0, 0, lib_serrecv},
    {"serdata", 0, 0, lib_serdata},

    {"textalign", 1, 0, lib_textalign, vtChar},
    {"launch", 1, 0, lib_launch, vtString},
    {"saveg", 0, 0, lib_saveg},
    {"restoreg", 0, 0, lib_restoreg},
    
    {"serbuffsize", 1, 0, lib_serbuffsize, vtInt},
    
    {"malloc", 1, 0, lib_malloc, vtInt},
    {"free", 1, 0, lib_free, vtInt},
    {"settype", 3, 0, lib_settype, vtInt, vtInt, vtChar},
    {"typeof", 1, 0, lib_typeof, vtInt},
    
    {"clipget", 0, 0, lib_clipget},
    {"clipset", 1, 0, lib_clipset, vtString},
    {"mmcount", 0, 0, lib_mmcount},
    
    // New database API
    {"dbenum", 3, 0, lib_dbenum, vtInt, vtString, vtString},
    {"dbrec", 1, 0, lib_dbrec, vtInt},
    {"dbnrecs", 0, 0, lib_dbnrecs},
    {"dbreadx", 2, 0, lib_dbreadx, vtInt, vtString},
    {"dbwritex", 2, 0, lib_dbwritex, vtInt, vtString},
    {"dbsize", 0, 0, lib_dbsize},
    {"dbdelrec", 1, 0, lib_dbdelrec, vtInt},
    {"dbremrec", 1, 0, lib_dbremrec, vtInt},
    {"dbarcrec", 1, 0, lib_dbarcrec, vtInt},
    {"dberase", 0, 0, lib_dberase, vtInt},
    
    {"memcpy", 3, 0, lib_memcpy, vtInt, vtInt, vtInt},
    {"hookhard", 1, 0, lib_hardkeys, vtInt},
    {"hookmenu", 1, 0, lib_hookmenu, vtInt},
    {"exit", 0, 0, lib_exit},
    {"strtoc", 2, 0, lib_strtoc, vtString, vtInt},
    {"ctostr", 1, 0, lib_ctostr, vtInt},
    
    {"getsd", 2, 0, lib_gets, vtString, vtString},
    {"atexit", 1, 0, lib_atexit, vtInt},
    {"textwidth", 1, 0, lib_textwidth, vtString},
    {"version", 0, 0, lib_version},
    {"getsi", 4, 0, lib_getsi, vtInt, vtInt, vtInt, vtString},
    
    {"sersenda", 2, 0, lib_sersenda, vtInt, vtInt},
    {"serrecva", 2, 0, lib_serrecva, vtInt, vtInt},
    {"unpack", 4, 0, lib_unpack, vtInt, vtInt, vtString, vtInt},
    {"malloct", 2, 0, lib_malloct, vtInt, vtString},
    {"getsm", 5, 0, lib_getsm, vtInt, vtInt, vtInt, vtInt, vtString},
    {"deepsleep", 1, 0, lib_deepsleep, vtInt},
    {"dbgetcat", 0, 0, lib_dbgetcat},
    {"dbsetcat", 1, 0, lib_dbsetcat, vtInt},
    {"dbcatname", 1, 0, lib_dbcatname, vtInt},
    {"dbcreatex", 3, 0, lib_dbcreatex, vtString, vtString, vtString},
    {"dbmoverec", 2, 0, lib_dbmoverec, vtInt, vtInt },
    {"dbinfo", 3, 0, lib_dbinfo, vtString, vtInt, vtInt },
    {"dbtotalsize", 1, 0, lib_dbtotalsize, vtString },
    {"hooksilk", 1, 0, lib_hooksilk, vtInt},
    {"hooksync", 1, 0, lib_hooksync, vtInt},
    {"serwait", 2, 0, lib_serwait, vtInt, vtInt},
    {"dbrename", 1, 0, lib_dbrename, vtString},
    {"dbsetcatname", 2, 0, lib_dbsetcatname, vtInt, vtString},
    {"dbwritexc", 3, 0, lib_dbwritex2, vtInt, vtString, vtInt},
    {"dbgetappinfo", 0, 0, lib_dbgetappinfo},
    {"dbsetappinfo", 0, 0, lib_dbsetappinfo},
    {"dbreadxc", 3, 0, lib_dbreadx2, vtInt, vtString, vtInt},
    {"dbmovecat", 3, 0, lib_dbmovecat, vtInt, vtInt, vtInt},
    {"mmfindx", 1, 0, lib_mmfindx, vtString},
    {"strtok", 4, 0, lib_strtok, vtString, vtInt, vtString, vtInt},
    {"seropenx", 2, 0, lib_seropenx, vtInt, vtInt},
    {"sersettings", 2, 0, lib_sersettings, vtString, vtInt},

    {"npenx", 0, 0, lib_npenx},
    {"npeny", 0, 0, lib_npeny},
    {"setfgi", 1, 0, lib_setfgi, vtInt},
    {"setbgi", 1, 0, lib_setbgi, vtInt},
    {"settextcolori", 1, 0, lib_settextcolori, vtInt},
    {"setfg", 3, 0, lib_setfg, vtInt, vtInt, vtInt},
    {"setbg", 3, 0, lib_setbg, vtInt, vtInt, vtInt},
    {"settextcolor", 3, 0, lib_settextcolor, vtInt, vtInt, vtInt},
    {"rgbtoi", 3, 0, lib_rgbtoi, vtInt, vtInt, vtInt},
    {"getcolordepth", 0, 0, lib_getcolordepth},
    {"setcolordepth", 1, 0, lib_setcolordepth, vtInt},
    {"getuicolor", 1, 0, lib_getuicolor, vtInt},
    {"pixel", 3, 0, lib_pixel, vtInt, vtInt, vtInt},
    {"pushdraw", 0, 0, lib_pushdraw},
    {"popdraw", 0, 0, lib_popdraw},
    {"choosecolori", 2, 0, lib_choosecolori, vtString, vtInt},
    {"drawnative", 1, 0, lib_setdrawdensity, vtInt},
    {"getscreenattrib", 1, 0, lib_getscreenattrib, vtInt},
    {"getvol", 1, 0, lib_getvol, vtInt},
    {"tonea", 3, 0, lib_tonea, vtInt, vtInt, vtInt},
    {"bucreate", 2, 0, lib_bucreate, vtInt, vtInt},
    {"budelete", 1, 0, lib_budelete, vtInt},
    {"buset", 1, 0, lib_buset, vtInt},
    {"bucopyrect", 9, 0, lib_bucopyrect, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt, vtInt},
    {"bucopy", 5, 0, lib_bucopy, vtInt, vtInt, vtInt, vtInt, vtInt},
    {"resopen", 1, 0, lib_resopen, vtString},
    {"resclose", 1, 0, lib_resclose, vtInt},
    {"bitmapr", 3, 0, lib_bitmapr, vtInt, vtInt, vtInt},
    {"bitmaprm", 4, 0, lib_bitmaprm, vtInt, vtInt, vtInt, vtInt},
    {"timex", 2, 0, lib_timex, vtInt, vtInt},
    {"datex", 2, 0, lib_datex, vtInt, vtInt},
    {"selectdate", 3, 0, lib_date_selectdate, vtInt, vtInt, vtString},
    {"selecttime", 2, 0, lib_date_selecttime, vtInt, vtString},
    {"alertc", 4, 0, lib_customAlert, vtString, vtString, vtString, vtInt},
    {"promptc", 5, 0, lib_customPrompt, vtString, vtString, vtString, vtInt, vtInt},
    {"secondsx", 2, 0, lib_secondsx, vtInt, vtInt},
    {"enableresize", 0, 0, lib_enableresize},

#ifdef POCKETC_SCANNER
    // general scanner functions
    {"scanIsSymbol", 0, 0, lib_issymbol},
    {"scanOpen", 0, 0, lib_scanopen},
    {"scanClose", 0, 0, lib_scanclose},
    {"scanAim", 1, 0, lib_scanaim, vtInt},
    {"scanGetAim", 0, 0, lib_scangetaim},
    {"scanLed", 1, 0, lib_scanled, vtInt},
    {"scanGetLed", 0, 0, lib_scangetled},
    {"scanEnable", 1, 0, lib_scanenable, vtInt},
    {"scanGetEnabled", 0, 0, lib_scangetenabled},
    {"scanDecode", 1, 0, lib_scandecode, vtInt},
    {"scanGetParams", 0, 0, lib_scangetparams},
    {"scanGetVer", 1, 0, lib_scangetver, vtInt},
    {"scanBeep", 1, 0, lib_scanbeep, vtInt},
    {"scanDefaults", 0, 0, lib_scandefaults},
    {"scanSendParams", 0, 0, lib_scansendparams},
    {"scanGetData", 0, 0, lib_scangetdata},

    // barcode specific functions
    {"scanEnableType", 2, 0, lib_scanenabletype, vtInt, vtInt},
    {"scanOptCode39", 2, 0, lib_scanoptcode39, vtInt, vtInt},
    
    // scanner hardware functions
    {"scanGetAimDur", 0, 0, lib_scangetaimdur},
    {"scanAimDur", 1, 0, lib_scanaimdur, vtInt},
    {"scanGetBeepAfterDecode", 0, 0, lib_scangetbeepafterdecode},
    {"scanBeepAfterDecode", 1, 0, lib_scanbeepafterdecode, vtInt},
    {"scanGetBiDi", 0, 0, lib_scangetbidi},
    {"scanBiDi", 1, 0, lib_scanbidi, vtInt},
    {"scanGetLedOnTime", 0, 0, lib_scangetledontime},
    {"scanLedOnTime", 1, 0, lib_scanledontime, vtInt},
    {"scanGetLaserOnTime", 0, 0, lib_scangetlaserontime},
    {"scanLaserOnTime", 1, 0, lib_scanlaserontime, vtInt},
    {"scanGetAngle", 0, 0, lib_scangetangle},
    {"scanAngle", 1, 0, lib_scanangle, vtInt},
    {"scanGetCount", 0, 0, lib_scangetcount},
    {"scanCount", 1, 0, lib_scancount, vtInt},
    {"scanGetTriggerMode", 0, 0, lib_scangettriggermode},
    {"scanTriggerMode", 1, 0, lib_scantriggermode, vtInt},
#else
    {"*", 0, 0, lib_scanrequired},
#endif
};

unsigned short nBIFuncs = sizeof(funcTable)/sizeof(FuncInfo);

void initLib() {
    randomize(TimGetSeconds());
    currdb = 0;
    dbloc = 0;
    eventQueue.Clear();
    memorec = memoloc = -1;
    mmDB = DmFindDatabase(0, "MemosDB-PMem");
    if (!mmDB) 
        mmDB = DmFindDatabase(0, "MemoDB");
    serOpen = false;
    textAlign = 0;
    textUL = noUnderline;
    nSaveBits = 0;
    serBuffer = NULL;
    nSaveBits = 0;
    blocks_h = (Handle)h_malloc(10 * sizeof(short));
    short* bp = (short*)MemHandleLock(blocks_h);
    MemSet(bp, 10 * sizeof(short), 0);
    bp[0] = globalOff; // Mark this as used
    MemHandleUnlock(blocks_h);
    MemSet(offScreen, MAX_OFFSCREEN * sizeof(WinHandle), 0);
    MemSet(openResDBs, MAX_RESDBS * sizeof(DmOpenRef), 0);
    _nBlocks = 10;
    nBlocks = 1;
    atexit_func = 0;
    nPushDraw = 0;
#ifdef POCKETC_SCANNER
    bScanOpen = false;
    bScanEnabled = false;
#endif	
}

void closeLib() {
    bHookHard = bHookMenu = false;
    FntSetFont(stdFont);
    lib_serclose(0);
        
    for (short i=0;i<nSaveBits;i++)
        WinDeleteWindow(saveBits[i], 0);
    for (short i=0;i<MAX_OFFSCREEN;i++)
        if (offScreen[i]) WinDeleteWindow(offScreen[i], false);
    for (short i=0;i<MAX_RESDBS;i++)
        if (openResDBs[i]) DmCloseDatabase(openResDBs[i]);
    h_free(blocks_h); // Actual memory is freed in Execute()
    if (romVersion >= ver35 && nPushDraw)
        while (nPushDraw--)
            WinPopDrawState();

#ifdef POCKETC_SCANNER
    if (bScanEnabled)
        ScanCmdScanDisable();
    if (bScanOpen)
        ScanCloseDecoder();
#endif	
}
