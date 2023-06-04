void lib_beep(VM* vm, int) {
    Value* arg = vm->pop();
    if (arg->iVal < 1 || arg->iVal > 7) arg->iVal = 1;
    SndPlaySystemSound((SndSysBeepType)arg->iVal);
}

void lib_tone(VM* vm, int) {
    Value* vol = vm->pop();
    Value* dur = vm->pop();
    Value* freq = vm->pop();
    
    SndCommandType cmd;
    cmd.cmd = sndCmdFreqDurationAmp;
    cmd.param1 = freq->iVal;
    cmd.param2 = dur->iVal;
    cmd.param3 = vol->iVal;
    if (freq->iVal > 0 && dur->iVal > 0) SndDoCmd(0, &cmd, false);
}

void lib_tonea(VM* vm, int) {
    Value* vol = vm->pop();
    Value* dur = vm->pop();
    Value* freq = vm->pop();
    
    SndCommandType cmd;
    cmd.cmd = sndCmdFrqOn;
    cmd.param1 = freq->iVal;
    cmd.param2 = dur->iVal;
    cmd.param3 = vol->iVal;
    if (freq->iVal > 0 && dur->iVal > 0) SndDoCmd(0, &cmd, false);
}

void lib_tonema(VM* vm, int) {
    Value* vol = vm->pop();
    Value* dur = vm->pop();
    Value* freq = vm->pop();
    
    SndCommandType cmd;
    cmd.cmd = sndCmdNoteOn;
    cmd.param1 = freq->iVal;
    cmd.param2 = dur->iVal;
    cmd.param3 = vol->iVal;
    if (dur->iVal > 0) SndDoCmd(0, &cmd, false);
}
