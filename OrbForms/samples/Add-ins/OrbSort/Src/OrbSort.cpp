#include <PalmOS.h>
#include "OrbNative.h"

extern "C" {
void* __Startup__(OrbFormsInterface*, void*, UInt16);
}

// The following five functions would normally be defined
// in the runtime library that an application links with.
// However, since this is just a code resource, we cannot
// link to it, so must define these ourselves
void* operator new(unsigned long size) {
    return MemPtrNew(size);
}

void operator delete(void* p) {
    MemPtrFree(p);
}

void* operator new[](unsigned long size) {
    return MemPtrNew(size);
}

void operator delete[](void* p) {
    MemPtrFree(p);
}
// the compiler generates a call to this to copy a structure
extern "C" 
void *__copy(char *to,char *from,unsigned long size) {
    char *f,*t;

    if(to) for(f=(char *)from,t=(char *)to; size>0; size--) *t++=*f++;
    return to;
}


// context information for the the sort
struct CompareContext {
    CompareContext(OrbFormsInterface* _ofi) {
        ofi = _ofi;
        si1 = si2 = so1 = so2 = 0;
    }
    OrbFormsInterface* ofi;
    int si1, so1; // sort index, sort order
    int si2, so2; // a sort order of 1 means descending
};

// given two struct/object addresses, compare them
int compareValue(UInt32 a, UInt32 b, int si, int so, OrbFormsInterface* ofi) {
    // dereference the values
    Value* va = ofi->vm_deref(a + si);
    Value* vb = ofi->vm_deref(b + si);
    // this should only occur if someone passes an invalid size/count parameter
    if (va->type != vb->type) ofi->vm_error("compare types are not the same");
    int res = 0;
    switch (va->type) {
        case vtInt: res = va->iVal < vb->iVal ? -1 : va->iVal > vb->iVal; break;
        //res = va->iVal - vb->iVal; break;
        case vtChar: res = va->cVal - vb->cVal; break;
        case vtFloat: res = va->fVal < vb->fVal ? -1 : va->fVal > vb->fVal; break;
        //res = va->fVal - vb->fVal; break;
        case vtString: 
            res = StrCompare(ofi->val_lock(va), ofi->val_lock(vb));
            ofi->val_unlock(va);
            ofi->val_unlock(vb);
            break;
    }
    
    if (so) res = -res;
    return res;
}

// compare to values, given the sort indices and ordering
int compare(UInt32 a, UInt32 b, CompareContext* cc) {
    int res = compareValue(a, b, cc->si1, cc->so1, cc->ofi);
    if (res == 0 && cc->si2 >= 0) {
        res = compareValue(a, b, cc->si2, cc->so2, cc->ofi);
    }
    return res;
}

// get an int from a string, and advance the pointer
int parseInt(char** str) {
    int res = 0;
    while (**str >= '0' && **str <= '9') {
        res *= 10;
        res += **str - '0';
        (*str)++;
    }
    return res;
}

void sort(OrbFormsInterface* ofi) {
    // pop off all the arguments in reverse order
    Value* order = ofi->vm_pop();
    int size = ofi->vm_pop()->iVal;
    int count = ofi->vm_pop()->iVal;
    int ptr = ofi->vm_pop()->iVal;
    
    // no need to sort if fewer than 2 elements
    if (count < 2) {
        ofi->val_release(order); // must always release a string parameter
        return;
    }
    
    // allocate the compare context and parse the "order" parameter into it
    CompareContext* cc = new CompareContext(ofi);
    char* orderStr = ofi->val_lock(order);
    if (*orderStr == '<') cc->so1 = 1;
    orderStr++;
    cc->si1 = parseInt(&orderStr);
    if (*orderStr) {
        if (*orderStr == '<') cc->so2 = 1;
        orderStr++;
        cc->si2 = parseInt(&orderStr);
    }
    ofi->val_unlockRelease(order); // unlock AND release the string

    Value* tempSpace = new Value[size];
    int left = ptr;
    int pivot = ptr + size;
    int right = ptr + size * count;
    
    int left_b, right_b, middle_b;
    
    while (pivot < right) {
        // is the next item >= than the last?
        if (compare(left, pivot, cc) > 0) {
            // binary insert into place
            left_b = ptr;
            right_b = left;
            
            while (left_b < right_b) {
                middle_b = left_b + ((right_b - left_b)/size)/2*size;
                int res = compare(pivot, middle_b, cc);
                if (res < 0) {
                    right_b = middle_b - size;
                } else {
                    left_b = middle_b + size;
                }
            }
                
            if (left_b >= right_b) {
                middle_b = left_b;
                if (compare(pivot, middle_b, cc) >= 0) {
                    middle_b += size;
                }
            }
            
            // the following lines copy OrbForms memory around. It is not safe
            // to copy just any value on top of another. If the types do not
            // match, the memory will be corrupted. If one of the Values is a
            // string, ref-counting must be handled properly
            //
            // given the above concerns, it is safe to do this here because:
            // 1. we are assured that the types are the same, since we are copying
            //    whole structures at a time
            // 2. we are just moving the Values about, so the string ref-counts
            //    need not ever change.
            
            // save to temp
            for (int i=0;i<size;i++) tempSpace[i] = *ofi->vm_deref(pivot + i);
            // make  room
            for (int i=pivot-middle_b; i>0; i--)
                *ofi->vm_deref(middle_b + size + i - 1) = *ofi->vm_deref(middle_b + i - 1);
            // copy from temp
            for (int i=0;i<size;i++) *ofi->vm_deref(middle_b + i) = tempSpace[i];
            
            left = pivot;
            pivot += size;
        } else {
            left = pivot;
            pivot += size;
        }
    }
    
    delete [] tempSpace;
    delete cc;
    
}

void* __Startup__(OrbFormsInterface* ofi, void* data, UInt16 iFunc)
{
    switch (iFunc) {
        case NATIVE_STARTUP:
            return NULL; // we do not need to setup any global state
        case NATIVE_SHUTDOWN:
            return NULL;
        case 0:
            sort(ofi);
            break;
    }
    return NULL;
}
