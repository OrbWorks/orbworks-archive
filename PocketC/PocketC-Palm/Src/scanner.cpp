void lib_issymbol(short) {
    RetInt(ScanIsPalmSymbolUnit());

}

void lib_scanopen(short) {
    RetInt(ScanOpenDecoder());
    bScanOpen = true;
}

void lib_scanclose(short) {
    RetInt(ScanCloseDecoder());
    bScanOpen = false;
}

void lib_scanaim(short) {
    Value v;
    pop(v);
    
    if (v.iVal) {
        RetInt(ScanCmdAimOn());
    } else {
        RetInt(ScanCmdAimOff());
    }
}

void lib_scangetaim(short) {
    RetInt(ScanGetAimMode());
}

void lib_scanled(short) {
    Value v;
    pop(v);
    
    if (v.iVal) {
        RetInt(ScanCmdLedOn());
    } else {
        RetInt(ScanCmdLedOff());
    }
}

void lib_scangetled(short) {
    RetInt(ScanGetLedState());
}

void lib_scanenable(short) {
    Value v;
    pop(v);
    
    if (v.iVal) {
        RetInt(ScanCmdScanEnable());
        bScanEnabled = true;
    } else {
        RetInt(ScanCmdScanDisable());
        bScanEnabled = false;
    }
}

void lib_scangetenabled(short) {
    RetInt(ScanGetScanEnabled());
}

void lib_scandecode(short) {
    Value v;
    pop(v);
    
    if (v.iVal) {
        RetInt(ScanCmdStartDecode());
    } else {
        RetInt(ScanCmdStopDecode());
    }
}

void lib_scangetparams(short) {
    char* buff = NewString(&retVal, 256);
    if (!buff) rt_oom();
    buff[0] = 0;
    
    ScanCmdGetAllParams((unsigned char*)buff, 256);
    UnlockString(&retVal);
}

void lib_scangetver(short) {
    Value v;
    pop(v);
    
    char* buff = NewString(&retVal, 64);
    if (!buff) rt_oom();
    buff[0] = 0;
    
    if (v.iVal == 0)
        ScanGetDecoderVersion(buff, 64);
    else if (v.iVal == 1)
        ScanGetScanManagerVersion(buff, 64);
    else
        ScanGetScanPortDriverVersion(buff, 64);
    
    UnlockString(&retVal);
}

void lib_scanbeep(short) {
    Value v;
    pop(v);
    
    ScanCmdBeep((BeepType)v.iVal);
}

void lib_scandefaults(short) {
    ScanCmdParamDefaults();
}

void lib_scansendparams(short) {
    Value v;
    pop(v);
    
    ScanCmdSendParams((BeepType)v.iVal);
}
    
long bcType;
void lib_scangetdata(short) {
    char* buff;
    MESSAGE msg;
    msg.data[0] = 0;
    
    ScanGetDecodedData(&msg);
    buff = NewString(&retVal, msg.length);
    if (!buff) rt_oom();
    strcpy(buff, (const char*)msg.data);
    UnlockString(&retVal);
    bcType = msg.type;
    
    //Alert("This software for evaluation only.");
}

void lib_scangetdatatype(short) {
    RetInt(bcType);
}
    

//
//
// Barcode type functions
//

void lib_scanenabletype(short) {
    Value bt, en;
    pop(en);
    pop(bt);
    
    ScanSetBarcodeEnabled((BarType)bt.iVal, en.iVal);
}
    
void lib_scanoptcode39(short) {
    Value cdv, fa;
    pop(fa);
    pop(cdv);
    
    ScanSetCode39CheckDigitVerification(cdv.iVal);
    ScanSetCode39FullAscii(fa.iVal);
}


//
//
// Scanner hardware functions
//

void lib_scangetaimdur(short) {
    RetInt(ScanGetAimDuration());
}

void lib_scanaimdur(short) {
    Value ad;
    pop(ad);
    
    ScanSetAimDuration(ad.iVal);
}

void lib_scangetbeepafterdecode(short) {
    RetInt(ScanGetBeepAfterGoodDecode());
}

void lib_scanbeepafterdecode(short) {
    Value v;
    pop(v);
    
    ScanSetBeepAfterGoodDecode(v.iVal);
}

void lib_scangetbidi(short) {
    RetInt(ScanGetBidirectionalRedundancy());
}

void lib_scanbidi(short) {
    Value v;
    pop(v);
    
    ScanSetBidirectionalRedundancy(v.iVal);
}

void lib_scangetledontime(short) {
    RetInt(ScanGetDecodeLedOnTime());
}

void lib_scanledontime(short) {
    Value v;
    pop(v);
    
    ScanSetDecodeLedOnTime(v.iVal);
}

void lib_scangetlaserontime(short) {
    RetInt(ScanGetLaserOnTime());
}

void lib_scanlaserontime(short) {
    Value v;
    pop(v);
    
    ScanSetLaserOnTime(v.iVal);
}

void lib_scangetcount(short) {
    RetInt(ScanGetLinearCodeTypeSecurityLevel());
}

void lib_scancount(short) {
    Value v;
    pop(v);
    
    ScanSetLinearCodeTypeSecurityLevel(v.iVal);
}

// TODO: consider transmission format and prefix/suffix functions
// TODO: consider code id functions

void lib_scangetangle(short) {
    RetInt(ScanGetAngle());
}

void lib_scanangle(short) {
    Value v;
    pop(v);
    
    ScanSetAngle(v.iVal);
}

void lib_scangettriggermode(short) {
    RetInt(ScanGetTriggeringModes());
}

void lib_scantriggermode(short) {
    Value v;
    pop(v);
    
    ScanSetTriggeringModes(v.iVal);
}



