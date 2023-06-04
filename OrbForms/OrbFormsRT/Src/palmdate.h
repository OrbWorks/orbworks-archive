static void GetDate(VM* vm, Value*& pVal, DateTimeType& dt) {
    Value* vPtr = vm->pop();
    pVal = vm->deref(vPtr->iVal);
    TimSecondsToDateTime(pVal->iVal, &dt);
}

static void SetDate(Value* pVal, DateTimeType& dt) {
    pVal->iVal = TimDateTimeToSeconds(&dt);
}

#define DATE_PROP_GET(prop) \
void get_date_##prop(VM* vm, int) { \
    DateTimeType dt; \
    Value* vDate; \
    GetDate(vm, byref vDate, byref dt); \
    vm->retVal.iVal = dt.prop; \
} \

#define DATE_PROP_SET(prop) \
void set_date_##prop(VM* vm, int) { \
    DateTimeType dt; \
    Value* vDate; \
    Value* vVal = vm->pop(); \
    GetDate(vm, byref vDate, byref dt); \
    dt.prop = vVal->iVal; \
    SetDate(vDate, byref dt); \
    vm->retVal.iVal = dt.prop; \
} \

DATE_PROP_GET(year)
DATE_PROP_GET(month)
DATE_PROP_GET(day)
DATE_PROP_GET(hour)
DATE_PROP_GET(minute)
DATE_PROP_GET(second)
DATE_PROP_GET(weekDay)
DATE_PROP_SET(year)
DATE_PROP_SET(month)
DATE_PROP_SET(day)
DATE_PROP_SET(hour)
DATE_PROP_SET(minute)
DATE_PROP_SET(second)

void get_date_ymd(VM* vm, int) {
    DateTimeType dt;
    Value* vDate;
    GetDate(vm, byref vDate, byref dt);
    vm->retVal.iVal = dt.year * 10000UL + dt.month * 100UL + dt.day;
}

void set_date_ymd(VM* vm, int) {
    DateTimeType dt;
    Value* vDate;
    Value* vVal = vm->pop();
    GetDate(vm, byref vDate, byref dt);
    dword dwVal = vVal->iVal;
    dt.year = dwVal / 10000UL;
    dt.month = (dwVal % 10000UL) / 100UL;
    dt.day = dwVal % 100UL;
    SetDate(vDate, byref dt);
    vm->retVal.iVal = dt.year * 10000UL + dt.month * 100UL + dt.day;
}

void lib_date_now(VM* vm, int) {
    Value* vPtr = vm->pop();
    Value* pVal = vm->deref(vPtr->iVal);
    pVal->iVal = TimGetSeconds();
}

void lib_date_selectdate(VM* vm, int) {
    Value* vTitle = vm->pop();
    char* strTitle = strdup(vTitle->lock());
    vTitle->unlockRelease();
    Value* vSelectBy = vm->pop();
    DateTimeType dt;
    Value* vDate;
    GetDate(vm, byref vDate, byref dt);
    
    Int16 year = dt.year, month = dt.month, day = dt.day;
    if ((vm->retVal.iVal = SelectDay((SelectDayType)vSelectBy->iVal, &month, &day, &year, strTitle))) {
        DateTimeType dt = { 0, };
        dt.year = year;
        dt.month = month;
        dt.day = day;
        SetDate(vDate, byref dt);
    }
    free(strTitle);
    vm->killVM = false;
}

void lib_date_selecttime(VM* vm, int) {
    Value* vTitle = vm->pop();
    char* strTitle = strdup(vTitle->lock());
    vTitle->unlockRelease();
    DateTimeType dt;
    Value* vDate;
    GetDate(vm, byref vDate, byref dt);
    
    Int16 hour = dt.hour, minute = dt.minute;
    if ((vm->retVal.iVal = SelectOneTime(&hour, &minute, strTitle))) {
        dt.hour = hour;
        dt.minute = minute;
        SetDate(vDate, byref dt);
    }
    free(strTitle);
    vm->killVM = false;
}

void lib_date_time(VM* vm, int) {
    DateTimeType dt;
    Value* vDate;
    GetDate(vm, byref vDate, byref dt);

    SystemPreferencesType sysPref;
    PrefGetPreferences(&sysPref);

    char* strTime = vm->retVal.newString(timeStringLength);
    if (strTime) {
        TimeToAscii(dt.hour, dt.minute, sysPref.timeFormat, strTime);
        vm->retVal.unlock();
    }
}

const int fi_short_date = 237; // BEWARE FUNC INDEX

void lib_date_date(VM* vm, int iFunc) {
    DateTimeType dt;
    Value* vDate;
    GetDate(vm, byref vDate, byref dt);

    SystemPreferencesType sysPref;
    PrefGetPreferences(&sysPref);

    char* strDate = vm->retVal.newString(iFunc == fi_short_date ? dateStringLength : longDateStrLength);
    if (strDate) {
        DateToAscii(dt.month, dt.day, dt.year, (iFunc == fi_short_date ? sysPref.dateFormat : sysPref.longDateFormat), strDate);
        vm->retVal.unlock();
    }
}

void lib_date_diffdays(VM* vm, int) {
    Value* vPtrB = vm->pop();
    dword dwB = vm->deref(vPtrB->iVal)->iVal;
    Value* vPtrA = vm->pop();
    dword dwA = vm->deref(vPtrA->iVal)->iVal;
    vm->retVal.iVal = (dwA / 86400) - (dwB / 86400);
}
