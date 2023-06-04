#include "OrbFormsRT.h"
#include "OrbFormsRTRsc.h"
#include <DLServer.h> // Needed to get user name

void lib_assert(VM* vm, int) {
    Value* cond = vm->pop();
    Value* vstr = &vm->stack[vm->st-1]; // leave it on the stack, because Alert
    char* str = vstr->lock();
    
    if (cond->iVal == 0) {
        vm->vm_error(str, true); 
    }
    vstr = vm->pop();
    vstr->unlockRelease();
}

void lib_error(VM* vm, int) {
    Value* vstr = &vm->stack[vm->st-1]; // leave it on the stack, because Alert
    char* str = vstr->lock();
    vm->vm_error(str); 
    vstr = vm->pop();
    vstr->unlockRelease();
}

void lib_debugstack(VM* vm, int) {
    vm->retVal.copyString(vm->stackDump(25));
}

void lib_alert(VM* vm, int) {
    Value* p = &vm->stack[vm->st-1]; // leave it on the stack, because Alert
    assert(p->type == vtString); // can cause another handler to run
    Alert(p->lock());
    p = vm->pop(); // stack may have been moved if handlers ran
    p->unlock();
    vm->cleanUp(p);
    vm->killVM = false;
}

NativeObject::NativeObject() { nRefs = nXRefs = 0; }
NativeObject::~NativeObject() { }
int NativeObject::AddRef(bool ex) {
    if (ex)
        nXRefs++;
    return ++nRefs;
}

int NativeObject::Release(bool ex) {
    int ret = --nRefs;
    if (ex)
        nXRefs--;
    if (!nRefs) {
        ppile->Remove(id);
        delete this;
    }
    return ret;
}

void lib_nobject_destroy(VM* vm, int) {
    Value* vObjref = vm->pop();
    assert(vObjref->type == vtInt);
    Value* vID = vm->deref(vObjref->iVal);
    assert(vID->type == vtInt);
    word id = vID->iVal;
    NativeObject* pno = ppile->Find(id);
    pno->Release(false);
    vID->iVal = 0;
}

void lib_nobject_copy(VM* vm, int) {
    NativeObject* pSrc = PopNO(vm);

    Value* vDst = vm->pop();
    assert(vDst->type == vtInt);
    Value* vDstID = vm->deref(vDst->iVal);
    assert(vDstID->type == vtInt);
    word dstID = vDstID->iVal;
    NativeObject* pDst = ppile->Find(dstID);
    
    pDst->Release(false);
    vDstID->iVal = pSrc->id;
    pSrc->AddRef(false);
    
    vm->retVal.iVal = vDst->iVal;
}

void nobject_init(VM* vm, NativeObject* pno) {
    Value* vObjref = vm->pop();
    assert(vObjref->type == vtInt);
    Value* vID = vm->deref(vObjref->iVal);
    assert(vID->type == vtInt);
    pno->id = vID->iVal = ppile->Add(pno);
    pno->AddRef(false);
}

NativePile::NativePile() : nEmpty(0) { }
NativePile::~NativePile() {
    for (short i=0;i<nops.size();i++) {
        NativeObject* pno = nops[i];
        if (pno) {
            int nRefs = pno->nRefs - pno->nXRefs;
            while (nRefs--)
                pno->Release(false); // release the language refs
            // this solves the problem of dependencies
        }
    }
    for (short i=0;i<nops.size();i++) {
        if (nops[i]) {
            assert(!"NativeObject leaked");
            delete nops[i];
        }
    }
}

short NativePile::Add(NativeObject* pno) {
    // TODO: optimize finding an empty spot
    if (nEmpty) {
        for (short i=0;i<nops.size();i++) {
            if (nops[i] == NULL) {
                nops[i] = pno;
                nEmpty--;
                return i;
            }
        }
    }
    nops.add(pno);
    return nops.size() - 1;
}

void NativePile::Remove(short id) {
    nEmpty++;
    nops[id] = 0;
}

NativeObject* NativePile::Find(short id) {
    assert(nops[id]);
    return nops[id];
}

NativePile* ppile;

extern int nPushDraw;
void LibStart() {
    ppile = new NativePile();
    nPushDraw = 0;
}

void LibStop() {
    delete ppile;
    while (nPushDraw--)
        if (romVersion >= ver35) WinPopDrawState();
}

void lib_NYI(VM* vm, int) {
    vm->vm_error("Not yet implemented");
}

enum VectorCallType { vcCall, vcCallBI, vcCallLib, vcVirtCall };

void vector_call(VM* vm, int, VectorCallType vc) {
    Value *f, *a, *s, *c, *l = NULL;
    c = vm->pop();
    s = vm->pop();
    f = vm->pop();
    a = vm->pop();
    if (vc == vcCallLib)
        l = vm->pop();

    long addr = a->iVal;
    long count = c->iVal;
    long size = s->iVal;
    long func = f->iVal;
    int lib = 0;
    if (vc == vcCallLib) {
        lib = l->iVal;
        assert(lib < addIns.size());
    }
    
    long pc = vm->GetFullPC();
    long fb = vm->fb;
    long fp = vm->fp;

    if (addr == 0) {
        // this was not allocated to begin with.
        // @TODO: must do this is compiler, too!
        return;
    }
    
    if (size == -1 && vc == vcVirtCall) {
        // size will only be unknown for an array of the same type of
        // object - no mixed objects in the array
        int objID = vm->deref(addr)->iVal;
        size = vm->getObjectSize(objID);
    }
    
    if (count == -1) {
        // get the count from heap info
        word bsize = vm->globs.getsize(addr);
        if (bsize == 0xffff)
            vm->vm_error("attempt to delete bad pointer");
        else if (bsize % size != 0)
            vm->vm_error("heap looks broken");
        if (vm->killVM)
            return;
        count = bsize / size;
    }
    
    while (count--) {
        Value t;
        t.type = vtInt;
        t.iVal = addr;
        vm->push(&t);
        if (vc == vcCall) {
            vm->Call(func, true, false);
        } else if (vc == vcCallBI) {
            (*bifuncs[func].func)(vm, func);
        } else if (vc == vcCallLib) {
            (*addIns[lib].func)(&ofi, addIns[lib].data, func);
        } else {
            long objID = vm->deref(addr)->iVal;
            long funcloc = vm->getVirtFunc(objID, func);
            vm->Call(funcloc, true, false);
        }
        
        addr += size;
    }
    
    vm->killVM = false;
    vm->SetPC(pc);
    vm->fp = fp;
    vm->fb = fb;
}

void lib_vector_call(VM* vm, int i) {
    vector_call(vm, i, vcCall);
}

void lib_vector_bicall(VM* vm, int i) {
    vector_call(vm, i, vcCallBI);
}

void lib_vector_libcall(VM* vm, int i) {
    vector_call(vm, i, vcCallLib);
}

void lib_vector_virtcall(VM* vm, int i) {
    vector_call(vm, i, vcVirtCall);
}

void lib_puts(VM* vm, int) {
    if (hasUI)
        vm->vm_error("puts not allowed in UI app");

    Value* p = vm->pop();
    assert(p->type == vtString);
    if (FrmGetActiveFormID() == OutputForm) {
        FieldPtr fp = (FieldPtr)GetObjectPtr(OutputOutputField);
        FldSetInsPtPosition(fp, -1);
        char* text = p->lock();
        FldInsert(fp, text, strlen(text));
    } else {
        char* text = p->lock();
        char* output = (char*)MemHandleLock(hOutput);
        int olen = strlen(output);
        int tlen = strlen(text);
        int rem = OutputFieldLen - olen - tlen - 1;
        if (rem < tlen)
            tlen = rem;
        strncpy(output + olen, text, tlen + 1);
        MemHandleUnlock(hOutput);
    }
    p->unlock();
    vm->cleanUp(p);
}

void lib_clear(VM* vm, int) {
    if (hasUI)
        vm->vm_error("clear not allowed in UI app");
    if (FrmGetActiveFormID() == OutputForm) {
        FieldPtr fp = (FieldPtr)GetObjectPtr(OutputOutputField);
        FldDelete(fp, 0, -1);
    } else {
        char* output = (char*)MemHandleLock(hOutput);
        *output = '\0';
        MemHandleUnlock(hOutput);
    }
}

void lib_debuglog(VM* vm, int) {
    Value* p = vm->pop();
    char* str = p->lock();
    if (vm->hostFile == NULL) {
        vm->hostFile = HostFOpen("\\orbforms_log.txt", "at");
    }
    if (vm->hostFile) {
        HostFPrintF(vm->hostFile, str);
        HostFPrintF(vm->hostFile, "\n");
    }
    p->unlockRelease();
}

void lib_prompt(VM* vm, int) {
    Value* p = &vm->stack[vm->st-1]; // leave it on the stack, because Alert
    assert(p->type == vtString);
    char msg[512];
    if (FrmCustomResponseAlert(InputAlert, p->lock(), NULL, NULL, msg, 512, NULL) == 1) {
        msg[0] = 0; // cancel => empty string
    }
    p = vm->pop(); // stack may have been moved if handlers ran
    p->unlock();
    vm->cleanUp(p);
    vm->retVal.copyString(msg);
    vm->killVM = false;
}

void lib_confirm(VM* vm, int) {
    Value* p = &vm->stack[vm->st-1]; // leave it on the stack, because Alert
    assert(p->type == vtString); // can cause another handler to run
    vm->retVal.iVal = !(FrmCustomAlert(ConfirmAlert, p->lock(), "", ""));
    p = vm->pop(); // stack may have been moved if handlers ran
    p->unlock();
    vm->cleanUp(p);
    vm->killVM = false;
}

void lib_ticks(VM* vm, int) {
    vm->retVal.iVal = TimGetTicks() % 2147483648UL;
}

void lib_tickspersec(VM* vm, int) {
    vm->retVal.iVal = SysTicksPerSecond();
}

bool AppEventLoop(bool bRuntime, bool bBlocking);

void lib_events(VM* vm, int) {
    bool bBlocking = vm->pop()->iVal;
    long callDepth = vm->callDepth;
    do {
        if (AppEventLoop(true, bBlocking))
            vm->killVM = true; // ensure that the VM is stopped
        else
            vm->killVM = false;
    } while (vm->callDepth > callDepth && vm->killVM == false);
}

void get_TestOnlyDouble_dbl(VM* vm, int) {
    Value* st = vm->pop();
    Value* single = vm->deref(st->iVal);
    assert(single->type == vtInt);
    vm->retVal.iVal = single->iVal * 2;
    vm->cleanUp(st);
}

void set_TestOnlyDouble_dbl(VM* vm, int) {
    Value* val = vm->pop();
    Value* st = vm->pop();
    Value* single = vm->deref(st->iVal);
    assert(single->type == vtInt);
    single->iVal = val->iVal / 2;
    vm->retVal.iVal = single->iVal * 2;
    vm->cleanUp(st);
}

bool isEmulator = false;
void lib_is_emulator(VM* vm, int) {
    HostID hID = HostGetHostID();
    isEmulator = (hID == hostIDPalmOSEmulator || hID == hostIDPalmOSSimulator);
}

void lib_display_no_run(VM* vm, int) {
    if (!isEmulator)
        Alert("This application was built with the DEMO version of"
        " OrbForms Designer and may only be run on an emulator.");
}

void lib_throw_no_run(VM* vm, int) {
    if (!isEmulator)
        ErrThrow(4);
}

word popID(VM* vm) {
    Value* vObjref = vm->pop();
    assert(vObjref->type == vtInt);
    Value* vID = vm->deref(vObjref->iVal);
    assert(vID->type == vtInt);
    return vID->iVal;
}

void lib_expected_count(VM* vm, int) {
    Value* vType = vm->pop();
    char* type = vType->lock();
    long count = 0;
    long total = 0;
    
    while (*type) {
        count = 0;
        while (*type >= '0' && *type <= '9') {
            count *= 10;
            count += *type + '0';
            type++;
        }
        if (*type == 'l') {
            while (*type && *type != '.')
                type++;
        }
        total += count ? count : 1;
        type++;
    }
    vm->retVal.type = vtInt;
    vm->retVal.iVal = total;
    
    vType->unlockRelease();
}

#include "palmstring.h"
#include "palmuilib.h"

long IterateValues(long addr, char* strFormat, int count, IterFunc func, void* pContext) {
    long res = 0;
    bool isObject;
    while (count--) {
        char* strIter = strFormat;
        int i;
        while (*strIter) {
            int count = 1, len = 0;
            VarType type;
getType:
            isObject = false;
            switch (*strIter) {
                case 'o': isObject = true; break;
                case 'i':
                case 'p': type = vtInt; len = 4; break;
                case 'f': type = vtFloat; len = 4; break;
                case 'w': type = vtInt; len = 2; break;
                case 'b': type = vtInt; len = 1; break;
                case 'c': type = vtChar; len = 1; break;
                case 's': type = vtString; break;
                case 'l': { // fixed-length string
                    strIter++;
                    while (*strIter >= '0' && *strIter <= '9') {
                        len *= 10;
                        len += *strIter - '0';
                        strIter++;
                        
                    }
                    assert(*strIter == '.');
                    type = vtString;
                    }
                    break;
                default:
                    count = 0;
                    while (*strIter >= '0' && *strIter <= '9') {
                        count *=10;
                        count += *strIter - '0';
                        strIter++;
                    }
                    if (*strIter)
                        goto getType;
            }
            for (i=0;i<count;i++) {
                Value* pVal = vm->deref(addr);
                if (!isObject) {
                    if (type != pVal->type) {
                        if (type == vtChar && pVal->type == vtInt) {
                            // it's ok
                        } else {
                            vm->vm_error("Type mismatch");
                        }
                    }
                    if (!(*func)(pVal, type, len, pContext)) {
                        goto done;
                    }
                }
                res++;
                addr++;
            }
            strIter++;
        }
    }
done:
    return res;
}

bool TypeSizeIterFunc(Value* pVal, VarType type, int len, void* pContext) {
    word* pRes = (word*)pContext;
    if (len) {
        (*pRes) += len;
    } else {
        assert(type == vtString);
        char* pStr = pVal->lock();
        (*pRes) += strlen(pStr) + 1;
        pVal->unlock();
    }
    return true;
}

word TypeDataSize(long addr, char* strFormat, int count) {
    word res = 0;
    IterateValues(addr, strFormat, count, &TypeSizeIterFunc, &res);
    return res;
}

word TypeSize(char* str, bool bMin) {
    word size = 0;
    // returns 0 for indeterminate
    while (*str) {
        word count = 1;
getType:
        switch (*str) {
            case 'i':
            case 'p':
            case 'f': size += count * 4; break;
            case 'w': size += count * 2; break;
            case 'b':
            case 'c': size += count; break;
            case 's': if (bMin) { size += count; break; } return 0;
            case 'l': { // fixed-length string
                word len = 0;
                while (*str >= '0' && *str <= '9') {
                    len *= 10;
                    len += *str - '0';
                    str++;
                }
                assert(*str == '.');
                size += count * len;
                }
                break;
            default:
                count = 0;
                while (*str >= '0' && *str <= '9') {
                    count *=10;
                    count += *str - '0';
                    str++;
                }
                goto getType;
        }
        str++;
    }
    return size;
}

const int firstMath = 110; // BEWARE FUNC INDEX

#include "palmlist.h"
#include "palmdata.h"

NativeObject* PopNO(VM* vm) {
    word id = popID(vm);
    return ppile->Find(id);
}

#include "palmdraw.h"
#include "palmdbmemo.h"
#include "palmsys.h"
#include "palmfield.h"
#include "palmmath.h"
#include "palmsound.h"
#include "palmdate.h"
#include "palmpocketc.h"

BuiltinFunc bifuncs[] = {
    { lib_NYI, false },
    { lib_vector_call, false },
    { lib_vector_bicall, false },
    { lib_vector_libcall, false },
    { get_TestOnlyDouble_dbl, true },
    { set_TestOnlyDouble_dbl, true },
    { lib_puts, false },
    { lib_alert, false },
    { lib_ticks, true },
    { get_UILabel_text, true },
    { set_UILabel_text, true }, // 10
    { get_control_text, true },
    { set_control_text, true },
    { get_control_bounds, true }, // x - 13 (const in palmuilib.cpp)
    { get_control_bounds, true }, // y
    { get_control_bounds, true }, // w
    { get_control_bounds, true }, // h
    { get_form_bounds, true }, // x - 17 (const in palmuilib.cpp)
    { get_form_bounds, true }, // y
    { get_form_bounds, true }, // w
    { get_form_bounds, true }, // h
    { get_UIForm_title, true }, // 21
    { set_UIForm_title, true },
    { get_control_value, true }, // 23
    { set_control_value, true },
    { set_UIFBitmap_bmpid, true }, // 25
    { set_control_visible, true },
    { get_field_text, true }, // 27
    { set_field_text, true },
    { get_field_inspt, true },
    { set_field_inspt, true },
    { get_field_seltext, true },
    { set_field_seltext, true },
    { get_field_fontid, true },
    { set_field_fontid, true },
    { get_field_scrollpos, true },
    { set_field_scrollpos, true },
    { get_field_dirty, true },
    { set_field_dirty, true }, // 38
    { lib_field_insert, false },
    { lib_field_select, false },
    { lib_field_cut, false },
    { lib_field_copy, false },
    { lib_field_paste, false },
    { lib_field_undo, false },
    { lib_field_del, false },
    { get_Event_x, true }, // 46
    { get_Event_y, true },
    /*
    { lib_UIDraw_pixel, false }, // 48
    { lib_UIDraw_frame, false },
    { lib_UIDraw_rect, false },
    { lib_UIDraw_line, false },
    */
    { lib_draw_bitmap, false },
    { lib_draw_text, false },
    { lib_draw_draw, false },
    { lib_draw_indexFromColor, true },
    
    { get_Event_inside, true }, // 52
    { get_Slider_val, true }, // 53 value
    { get_Slider_val, true }, // 54 min
    { get_Slider_val, true }, // 55 max
    { get_Slider_val, true }, // 56 page
    { set_Slider_val, true }, // 57 value
    { set_Slider_val, true }, // 58 min
    { set_Slider_val, true }, // 59 max
    { set_Slider_val, true }, // 60 page
    { get_Event_value, true },
    { get_Event_prev, true },
    { get_Event_key, true },
    { get_UIList_selitem, true }, // 64
    { set_UIList_selitem, true }, //
    { lib_UIList_gettext, true }, // 66
    { set_UIList_topitem, true }, // 67
    { get_UIList_count, true }, // 68
    { lib_UIList_makevisible, false }, // 69
    { lib_UIList_scroll, true }, // 70
    { lib_UIList_popup, true }, // 71
    { lib_UIList_setitems, true }, // 72
    { lib_UIForm_select, false },
    { lib_UIList_redraw, false }, // 74
    { lib_field_grabfocus, false },
    { lib_field_relfocus, false },
    { lib_UIForm_timer, false }, // 77
    { get_dict_count, true },
    { lib_dict_init, false },
    { lib_nobject_destroy, false }, // 80 - used for all refcounted NativeObjects
    { lib_dict_clear, false },
    { lib_dict_add, true },
    { lib_dict_del, true },
    { lib_dict_find, true },
    { lib_dict_has, true }, // 85
    { lib_UIForm_load, false },
    { lib_UIForm_domodal, false },
    { lib_UIForm_close, false }, // 88
    { lib_nobject_copy, true }, // 89 - used for all refcounted NativeObjects
    { lib_draw_init, false }, // 90
    { lib_draw_begin, false },
    { lib_draw_end, false },
    { lib_draw_line, false }, // 93
    { lib_draw_attachGadget, false },
    { lib_draw_attachForm, false },
    { lib_draw_pixel, false }, // 96
    { lib_draw_rect, false },
    { lib_draw_frame, false },
    { lib_strlen, true }, // 99
    { lib_substr, true },
    { lib_left, true },
    { lib_right, true },
    { lib_strupper, true },
    { lib_strlower, true },
    { lib_strstr, true },
    { lib_hex, true },
    { lib_format, true },
    { lib_strctos, true },
    { lib_strstoc, true }, // 109
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true },
    { lib_math, true }, // 125
    { lib_atan2, true },
    { lib_pow, true },
    { lib_random, true },
    { lib_rand, true }, // 129
    { lib_memo_init, false }, // 130
    { get_memo_text, true },
    { set_memo_text, true },
    { get_memo_nrecs, true },
    { lib_memo_find, true },
    { lib_memo_create, true },
    { lib_memo_open, true },
    { lib_memo_puts, false },
    { lib_memo_line, true },
    { lib_memo_end, true },
    { lib_memo_rewind, false }, // 140
    { lib_memo_close, false },
    { lib_memo_remove, false },
    { lib_memo_erase, false },
    { lib_draw_fg, true },
    { lib_draw_bg, true },
    { lib_draw_textColor, true },
    { lib_draw_font, true },
    { lib_draw_underline, true },
    { lib_draw_textAlign, true },
    { lib_draw_read, true }, // 150
    { lib_draw_write, true },
    { lib_draw_create, true },
    { lib_draw_copyForm, true },
    { lib_draw_copyGadget, true },
    { lib_draw_release, false },
    { lib_app_getdepth, true }, // 156
    { lib_app_setdepth, true },
    { lib_database_init, false }, // 158
    { lib_database_open, true },
    { lib_database_opentc, true }, // 160
    { lib_database_create, true },
    { lib_database_close, false },
    { lib_database_delete, false },
    { lib_database_setcatname, false },
    { lib_database_getcatname, true }, // 165
    { lib_database_moverec, false },
    { lib_database_removerec, false },
    { lib_database_delrec, false },
    { lib_database_archiverec, false },
    { lib_database_getrec, true }, // 170
    { lib_database_newrec, true },
    { get_database_nrecs, true },
    { get_database_backup, true },
    { set_database_backup, true },
    { get_database_size, true }, // 175
    { get_database_type, true },
    { get_database_cid, true },
    { get_database_name, true }, // 178
    { lib_dbrecord_read, true },
    { lib_dbrecord_write, true }, // 180
    { lib_dbrecord_erase, false },
    { lib_dbrecord_close, false },
    { get_dbrecord_offset, true },
    { set_dbrecord_offset, true },
    { get_dbrecord_id, true }, // 185
    { get_dbrecord_cat, true },
    { set_dbrecord_cat, true },
    { get_dbrecord_size, true }, // 188
    { lib_dbrecord_init, false }, // 189
    { lib_tone, false },
    { lib_tonea, false },
    { lib_tonema, false },
    { lib_beep, false }, // 193
    { lib_resetaot, false },
    { lib_launch, false }, // 195
    { lib_username, true },
    { lib_serialnum, true },
    { lib_osver, true },
    { lib_osvers, true },
    { lib_orbver, true }, // 200
    { lib_getclip, true },
    { lib_setclip, false },
    { lib_memcpy, false },
    { lib_malloct, true }, // 204 
    { lib_dbmanager_dbenum, true }, // 205
    { lib_stringlist_init, false },
    { get_stringlist_count, true },
    { lib_stringlist_clear, false },
    { lib_stringlist_insert, true },
    { lib_stringlist_del, true }, // 210
    { lib_stringlist_find, true },
    { lib_stringlist_tokens, true },
    { lib_stringlist_item, true }, // 213
    { lib_prefs_write, true },
    { lib_prefs_read, true },
    { lib_prefs_del, false }, // 216
    { lib_UIList_setitemslist, true },
    { lib_app_hookhard, false },
    { lib_free, false }, // 219
    { get_date_year, true }, // 220
    { get_date_month, true },
    { get_date_day, true },
    { get_date_hour, true },
    { get_date_minute, true },
    { get_date_second, true },
    { get_date_ymd, true },
    { set_date_year, true }, // 227
    { set_date_month, true },
    { set_date_day, true },
    { set_date_hour, true },
    { set_date_minute, true },
    { set_date_second, true },
    { set_date_ymd, true },
    { lib_date_now, false }, // 234
    { lib_date_selectdate, true },
    { lib_date_time, true },
    { lib_date_date, true }, // short
    { lib_date_date, true }, // 238 long
    { lib_draw_textWidth, true },
    { lib_draw_textHeight, true },
    { lib_is_emulator, false }, // 241
    { lib_display_no_run, false }, // 242
    { lib_throw_no_run, false }, // 243
    { lib_field_scroll, false },
    { lib_scroll_update, false },
    { get_scroll_val, true }, // 246 value
    { get_scroll_val, true }, // min
    { get_scroll_val, true }, // max
    { get_scroll_val, true }, // page
    { set_scroll_val, true }, // value
    { set_scroll_val, true }, // min
    { set_scroll_val, true }, // max
    { set_scroll_val, true }, // page
    { set_UIForm_menu, true }, // 254
    { lib_database_getres, true },
    { lib_database_newres, true },
    { lib_database_removeres, false },
    { get_database_res, true }, // 258
    { lib_database_getappinfo, true },
    { lib_database_setappinfo, true },
    { lib_dict_key, true }, // 261
    { lib_dict_value, true }, // 262
    { lib_UIForm_dodialog, true }, // 263
    { lib_confirm, true },
    { lib_app_exit, false },
    { lib_sleep, false }, // 266
    { lib_prompt, true }, // 267
    { lib_date_selecttime, true },
    { lib_draw_selectIndex, true }, // 269
    { lib_UIForm_help, false }, // 270
    { set_control_bounds, true }, // 271 x
    { set_control_bounds, true }, // 272 y
    { set_control_bounds, true }, // 273 w
    { set_control_bounds, true }, // 274 h
    { get_Event_code, true }, // 275
    { lib_UIForm_redraw, false },
    { lib_date_diffdays, true },
    { lib_database_createappinfo, true }, // 278
    { lib_database_hasappinfo, true },
    { get_database_dbref, true }, // 280
    { get_dbrecord_handle, true }, // 281
    { lib_lformat, true },
    { lib_lparse, true },
    { lib_database_getresinfo, true }, // 284
    { lib_memo_delete, false },
    { lib_memo_archive, false },
    { get_date_weekDay, true }, // 287
    { get_draw_handle, true },
    { lib_UIForm_activefield, true },
    { lib_field_phonelookup, false }, // 290
    { lib_UIForm_keyboard, false },
    { lib_keystate, true },
    { lib_penstate, true },
    { set_control_bmp, true }, // 294
    { set_control_sbmp, true },
    { get_string_text, true }, // 296
    { get_Event_nx, true },
    { get_Event_ny, true },
    { lib_app_getscreenattrib, true },
    { get_draw_nw, true }, // 300
    { get_draw_nh, true },
    { lib_draw_nbegin, false },
    { lib_draw_fgRGB, false },
    { lib_draw_bgRGB, false },
    { lib_draw_textRGB, false }, // 305
    { set_field_editable, true },
    { lib_app_getSysPref, true },
    { lib_draw_uiColorIndex, true },
    { set_database_name, true }, // 309
    { set_field_numeric, true },
    { lib_draw_textTrunc, false },
    { get_database_version, true }, // 312
    { set_database_version, true },
    { lib_customAlert, true },
    { lib_customPrompt, true },
    { lib_events, false },
    { lib_UIList_setdrawfunc, false },
    { lib_vector_virtcall, false },
    { lib_expected_count, true },
    { get_database_nobeam, true }, // 320
    { set_database_nobeam, true },
    { get_dbrecord_uniqueid, true },
    { lib_database_findrec, true },
    { lib_tickspersec, true },
    { lib_assert, false }, // 325
    { lib_debuglog, false },
    { lib_debugstack, true },
    { lib_error, false },
    { lib_stringlist_sort, false }, // 329
    { get_UIForm_obscured, true }, // 330
    { lib_database_getdate, true }, // 331
    { get_database_inrom, true },
    { lib_dbrecord_fromfile, true },
    { lib_dbrecord_resfromfile, true },
    
    { get_memo_index, true }, // 335
    { lib_customPromptDefault, true },
    { lib_clear, false },
    { lib_getsm, true },
    { lib_graph_on, false }, // 339
    { lib_graph_off, false }, // 340
    { lib_saveg, true },
    { lib_restoreg, false },
    { lib_title, false },
    { lib_pushdraw, false },
    { lib_popdraw, false }, // 345
    { lib_wait, true },
    { lib_waitp, false },
    { lib_getc, true },
    { lib_pstate, true },
    { lib_pcevent, true }, // 350
    { lib_hookmenu, false },
    { lib_hooksilk, false },
    { lib_hooksync, false },
    { lib_key, true },
    { lib_penx, true }, // 355
    { lib_peny, true },
    { lib_npenx, true },
    { lib_npeny, true },
    { lib_pctext, false },
    { lib_clearg, false }, // 360
    { lib_bitmap, false },
    { lib_atexit, false },
    { lib_drawnative, false },
    { lib_bucreate, true },
    { lib_budelete, false }, // 365
    { lib_buset, false },
    { lib_bucopy, false },
    { lib_bucopyrect, false },
    { lib_bitmaprm, false },
    { lib_enableresize, false }, // 370
    { lib_strinsert, true },
    { lib_strreplace, true },
    { lib_freemem, true },
    { lib_battery, true },
    { lib_srand, false }, // 375
    { lib_draw_bitmap, false },
    { lib_draw_draw, false },
    // 4.1
    { lib_launch_goto, false }, // 378
    { lib_launch_string, false }, // 379
    { get_database_dbid, true },
    { get_database_card, true },
    { get_app_launchArgs, true },
    { set_field_underline, true }, // 383
    { get_control_visible, true },
    { get_control_enabled, true },
    { set_control_enabled, true },
};

int nbifuncs = sizeof(bifuncs) / sizeof(bifuncs[0]);
