NativeDict::NativeDict() { }
NativeDict::~NativeDict() {
    clear();
}

long NativeDict::count() {
    return keys.size();
}

void NativeDict::clear() {
    long i;
    for (i=0;i<keys.size();i++) {
        free(keys[i]);
        free(vals[i]);
    }
    keys.clear();
    vals.clear();
}

long NativeDict::add(char* key, char* val) {
    char* p;
    keys.add(p = strdup(key));
    vals.add(p = strdup(val));
    return keys.size();
}

long NativeDict::del(char* key) {
    long i = findIndex(key);
    if (i >= 0) {
        free(keys[i]);
        free(vals[i]);
        keys.erase(i);
        vals.erase(i);
    }
    return keys.size();
}

char* NativeDict::find(char* key) {
    long i = findIndex(key);
    if (i >= 0)
        return vals[i];
    else
        return NULL;
}

long NativeDict::findIndex(char* key) {
    for (long i=0;i<keys.size();i++) {
        if (!strcmp(keys[i], key))
            return i;
    }
    return -1;
}

char* NativeDict::key(int index) {
    if (index < keys.size()) {
        return keys[index];
    }
    return "";
}

char* NativeDict::value(int index) {
    if (index < vals.size()) {
        return vals[index];
    }
    return "";
}

NATIVE_INIT(lib_dict_init, NativeDict)

void get_dict_count(VM* vm, int) {
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    vm->retVal.iVal = pdict->count();
}

void lib_dict_clear(VM* vm, int) {
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    pdict->clear();
}

void lib_dict_add(VM* vm, int) {
    Value* vKey, *vVal;
    vVal = vm->pop();
    vKey = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    pdict->add(vKey->lock(), vVal->lock());
    vKey->unlock(); vVal->unlock();
    vm->cleanUp(vKey); vm->cleanUp(vVal);
    vm->retVal.iVal = pdict->count();
}

void lib_dict_del(VM* vm, int) {
    Value* vKey;
    vKey = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    pdict->del(vKey->lock());
    vKey->unlock();
    vm->cleanUp(vKey);
    vm->retVal.iVal = pdict->count();
}

void lib_dict_find(VM* vm, int) {
    Value* vKey;
    vKey = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    vm->retVal.copyString(pdict->find(vKey->lock()));
    vKey->unlock();
    vm->cleanUp(vKey);
}

void lib_dict_has(VM* vm, int) {
    Value* vKey;
    vKey = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    vm->retVal.iVal = (pdict->find(vKey->lock())) != NULL;
    vKey->unlock();
    vm->cleanUp(vKey);
}

void lib_dict_key(VM* vm, int) {
    Value* vIndex;
    vIndex = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    vm->retVal.copyString(pdict->key(vIndex->iVal));
}

void lib_dict_value(VM* vm, int) {
    Value* vIndex;
    vIndex = vm->pop();
    NativeDict* pdict = (NativeDict*)PopNO(vm);
    vm->retVal.copyString(pdict->value(vIndex->iVal));
}

NativeStringList::NativeStringList() { }

NativeStringList::~NativeStringList() {
    clear();
}

long NativeStringList::count() {
    return strs.size();
}

void NativeStringList::clear() {
    for (long i=0;i<strs.size();i++) {
        free(strs[i]);
    }
    strs.clear();
}

long NativeStringList::insert(long index, char* val) {
    char* p;
    if (index == -1 || index > strs.size()) {
        strs.add(p = strdup(val));
    } else {
        strs.insert(index, p = strdup(val));
    }
    return strs.size();
}

long NativeStringList::del(long index) {
    if (index >= 0 && index < strs.size()) {
        free(strs[index]);
        strs.erase(index);
    }
    return strs.size();
}

long NativeStringList::find(char* val) {
    long res = -1;
    for (long i=0;i<strs.size();i++) {
        if (0 == strcmp(val, strs[i])) {
            res = i; break;
        }
    }
    return res;
}

char* NativeStringList::item(long index) {
    if (index >= 0 && index < strs.size()) {
        return strs[index];
    }
    return NULL;
}

char* strNextTok(char* src, char* toks) {
    while (*src) {
        char* tokiter = toks;
        while (*tokiter) {
            if (*src == *tokiter) return src;
            tokiter++;
        }
        src++;
    }
    return src;
}

long NativeStringList::tokens(char* src, char* toks) {
    char* next = NULL;
    char* last = src;
    int found = 0;
    
    while (true) {
        next = strNextTok(last, toks);
        if (next > last) {
            // not empty, so add it
            char* str = (char*)malloc(next - last + 1);
            strncpy(str, last, next - last);
            str[next-last] = 0;
            found++;
            strs.add(str);
        }
        if (*next == '\0') break;
        last = next + 1;
    }
    return found;
}

short MyStringCompare(void* A, void* B, long other) {
    return strcmp(*(char**)A, *(char**)B);
}

short MyStringCompareI(void* A, void* B, long other) {
    return stricmp(*(char**)A, *(char**)B);
}

void NativeStringList::sort(bool caseSensitive) {
    if (strs.size() > 1) {
        char** pdata = &strs[0];
        SysQSort(pdata, strs.size(), sizeof(char*),
            caseSensitive ? MyStringCompare : MyStringCompareI, 0);
    }
}

NATIVE_INIT(lib_stringlist_init, NativeStringList)

void get_stringlist_count(VM* vm, int) {
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.iVal = slist->count();
}

void lib_stringlist_clear(VM* vm, int) {
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    slist->clear();
}

void lib_stringlist_insert(VM* vm, int) {
    Value* vStr = vm->pop();
    Value* vIndex = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.iVal = slist->insert(vIndex->iVal, vStr->lock());
    vStr->unlockRelease();
}

void lib_stringlist_del(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.iVal = slist->del(vIndex->iVal);
}

void lib_stringlist_find(VM* vm, int) {
    Value* vStr = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.iVal = slist->find(vStr->lock());
    vStr->unlockRelease();
}

void lib_stringlist_item(VM* vm, int) {
    Value* vIndex = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.copyString(slist->item(vIndex->iVal));
}

void lib_stringlist_tokens(VM* vm, int) {
    Value* vToks = vm->pop();
    Value* vStr = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    vm->retVal.iVal = slist->tokens(vStr->lock(), vToks->lock());
    vToks->unlockRelease();
    vStr->unlockRelease();
}

void lib_stringlist_sort(VM* vm, int) {
    Value* vCS = vm->pop();
    NativeStringList* slist = (NativeStringList*)PopNO(vm);
    slist->sort(vCS->iVal);
}
