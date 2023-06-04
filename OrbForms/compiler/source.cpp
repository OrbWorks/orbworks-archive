#include "compiler.h"

BufferSource::BufferSource(const char* buffer, bool bFree) {
    this->buffer = (char*)buffer;
    this->bFree = bFree;
}

BufferSource::~BufferSource() {
    if (bFree)
        delete buffer;
}

char* BufferSource::GetSrc() {
    return buffer;
}

SourceType BufferSource::GetType() {
    return srcNone;
}


MacroSource::MacroSource(char* str) : BufferSource(str, false) {
}

SourceType MacroSource::GetType() {
    return srcMacro;
}


#ifndef WIN32
extern DmOpenRef memoDB, PcDB;

MemoSource::MemoSource(short id) {
    this->hMem = DmQueryRecord(memoDB, id);
}

MemoSource::~MemoSource() {
    MemHandleUnlock(hMem);
}

char* MemoSource::GetSrc() {
    return (char*)MemHandleLock(hMem);
}

SourceType MemoSource::GetType() {
    return srcMemo;
}


HandleSource::HandleSource(MemHandle hMem) {
    this->hMem = hMem;
}

HandleSource::~HandleSource() {
    MemHandleUnlock(hMem);
}

char* HandleSource::GetSrc() {
    return (char*)MemHandleLock(hMem);
}

SourceType HandleSource::GetType() {
    return srcRes;
}


DocSource::DocSource(short id) {
    this->hMem = DmQueryRecord(PcDB, id);
}

DocSource::~DocSource() {
    MemHandleUnlock(hMem);
    // this doc record will be cleaned up at the end of compile
}

char* DocSource::GetSrc() {
    return (char*)MemHandleLock(hMem);
}

SourceType DocSource::GetType() {
    return srcDoc;
}


#endif
