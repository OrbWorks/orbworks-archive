#include "compiler.h"

int stringWidth(const char* str);

//
// generate DB records
//
//

bool Compiler::copyResFromPRC(PalmDB& db, string type, int id, string res, string& error) {
    int i = res.find_first_of(',');
    if (i == string::npos) {
        error = "invalid resource string";
        return false;
    }
    string file = res.substr(0, i);
    res = res.substr(i+1);
    i = res.find_first_of(',');
    if (i == string::npos) {
        error = "invalid resource string";
        return false;
    }
    string srctype = res.substr(0, i);
    int srcid = atoi(res.substr(i+1).c_str());

#ifdef WIN32
    if (db.GetResRec(type.c_str(), id)) {
        error = "conflicting resource already present";
        return false;
    }

    file += ".prc";
    file = lex.FindFile(file.c_str());
    PalmDB srcdb;
    if (!srcdb.Read(file.c_str())) {
        error = "unable to read PRC file";
        return false;
    }

    PalmResRecord* pResRec = srcdb.GetResRec(srctype.c_str(), srcid);
    if (!pResRec) {
        error = "unable to read resource from PRC file";
        return false;
    }

    PalmResRecord resRec;
    resRec = *pResRec;
    resRec.type = type;
    resRec.id = id;
    db.AddResRec(resRec);
    return true;
#else
    char dtype[4];
    for (int i=0;i<4 && i<type.length();i++)
        dtype[i] = type[i];

    if (DmFindResource(db.GetDmRef(), *((dword*)&dtype[0]), id, NULL) != 0xffff) {
        error = "conflicting resource already present";
        return false;
    }

    LocalID lid = DmFindDatabase(0, file.c_str());
    if (!lid) {
        error = "unable to find resource database";
        return false;
    }
    DmOpenRef dbRef = DmOpenDatabase(0, lid, dmModeReadOnly);
    if (!dbRef) {
        error = "unable to find resource database";
        return false;
    }
    char atype[4];
    for (int i=0;i<4 && i<srctype.length();i++)
        atype[i] = srctype[i];

    MemHandle hMem = DmGet1Resource(*((dword*)&atype[0]), srcid);
    if (!hMem) {
        error = "unable to find resource in database";
        return false;
    }
    
    PalmResRecord resRec;
    resRec.id = id;
    resRec.type = type;
    resRec.data = (byte*)MemHandleLock(hMem);
    resRec.len = MemHandleSize(hMem);
    db.AddResRec(resRec);
    MemHandleUnlock(hMem);
    DmCloseDatabase(dbRef);
    return true;
#endif
}


bool Compiler::copyAllResFromPRC(PalmDB& db, string file, string& error) {
#ifdef WIN32
    file += ".prc";
    file = lex.FindFile(file.c_str());
    PalmDB srcdb;
    if (!srcdb.Read(file.c_str())) {
        error = "unable to read PRC file";
        return false;
    }

    for (int i=0;i<srcdb.GetCount();i++) {
        PalmResRecord* pResRec = srcdb.GetResRec(i);
        if (!pResRec) {
            error = "unable to read resource from PRC file";
            return false;
        }

        if (db.GetResRec(pResRec->type.c_str(), pResRec->id)) {
            error = "conflicting resource already present";
            return false;
        }

        PalmResRecord resRec;
        resRec = *pResRec;
        db.AddResRec(resRec);
    }
    return true;
#else
    int i;
    LocalID lid = DmFindDatabase(0, file.c_str());
    if (!lid) {
        error = "unable to find resource database";
        return false;
    }
    DmOpenRef dbRef = DmOpenDatabase(0, lid, dmModeReadOnly);
    if (!dbRef) {
        error = "unable to find resource database";
        return false;
    }
    word nRes = DmNumResources(dbRef);
    for (i=0;i<nRes;i++) {
        MemHandle hMem = DmGetResourceIndex(dbRef, i);
        if (!hMem) {
            error = "unable to find resource in database";
            DmCloseDatabase(dbRef);
            return false;
        }
        
        char dtype[5] = {0};
        word id;
        DmResourceInfo(dbRef, i, (dword*)&dtype[0], &id, NULL);
        if (DmFindResource(db.GetDmRef(), *((dword*)&dtype[0]), id, NULL) != 0xffff) {
            error = "conflicting resource already present";
            DmCloseDatabase(dbRef);
            return false;
        }

        PalmResRecord resRec;
        resRec.id = id;
        resRec.type = string(dtype);
        resRec.data = (byte*)MemHandleLock(hMem);
        resRec.len = MemHandleSize(hMem);
        db.AddResRec(resRec);
        MemHandleUnlock(hMem);
    }
    DmCloseDatabase(dbRef);
    return true;
#endif
}


bool Compiler::genMenus(PalmDB& db) {
    int i,j,k,m;
    // gen menus
    const int cbMenuBar = 32;
    const int cbMenu = 34;
    const int cbItem = 8;

    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        for (j=0;j<app.forms[i].menubars.size();j++) {
            PalmMenuBar& bar = app.forms[i].menubars[j];

            int nItems = 0, nChars = 0;
            int iItems = 0, iChars = 0;
            int size = 0;

            // precalc sizes
            for (k=0;k<bar.menus.size();k++) {
                PalmMenu& menu = bar.menus[k];
                nItems += menu.items.size();
                nChars += menu.text.length() + 1;
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    nChars += item.text.length() + 1;
                }
            }

            iItems = cbMenuBar + bar.menus.size() * cbMenu;
            iChars = iItems + cbItem * nItems;
            size = iChars + nChars;

            PalmDBStream dbstr(size);

            // menubar
            dbstr.addLong(0);
            dbstr.addLong(0);
            dbstr.addLong(0);
            dbstr.addLong(0);
            dbstr.addByte(1); // visible
            dbstr.addByte(0);
            dbstr.addWord(0);
            dbstr.addWord(0); // curItem
            dbstr.addLong(0);
            dbstr.addWord(bar.menus.size());
            dbstr.addLong(0);
            
            // menus
            int xTitle = 4;
            for (k=0;k<bar.menus.size();k++) {
                PalmMenu& menu = bar.menus[k];
                int wTitle = stringWidth(menu.text.c_str()) + 7;
                int xMenu = xTitle + 2;
                int wMenu = 0, hMenu = 0;
                for (m=0;m<menu.items.size();m++) {
                    int wItem = stringWidth(menu.items[m].text.c_str()) + 6;
                    if (menu.items[m].shortcut) {
                        wItem += 12 + 6;
                        char sc[2] = {0};
                        sc[0] = menu.items[m].shortcut;
                        wItem += stringWidth(sc);
                    }
                    wMenu = wMenu > wItem ? wMenu : wItem;
                    if (menu.items[m].text == "-")
                        hMenu += 5;
                    else
                        hMenu += 11;
                }
                if (wMenu > 160)
                    wMenu = 160;
                if (xMenu + wMenu >= 160)
                    xMenu = 160 - wMenu - 2;
                dbstr.addLong(0);
                dbstr.addWord(xMenu); // x - bounds
                dbstr.addWord(14); // y
                dbstr.addWord(wMenu); // w
                dbstr.addWord(hMenu); // h
                dbstr.addLong(0);
                dbstr.addWord(xTitle); // x - title bounds
                dbstr.addWord(0); // y
                dbstr.addWord(wTitle); // w
                dbstr.addWord(12); // h
                dbstr.addLong(iChars);
                iChars += menu.text.length() + 1;
                dbstr.addWord(menu.items.size());
                dbstr.addLong(iItems);
                iItems += cbItem * menu.items.size();
                xTitle += wTitle;
            }

            // items
            for (k=0;k<bar.menus.size();k++) {
                PalmMenu& menu = bar.menus[k];
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    if (item.sysid)
                        dbstr.addWord(item.sysid);
                    else
                        dbstr.addWord(item.id);
                    dbstr.addByte(item.shortcut);
                    dbstr.addByte(0);
                    dbstr.addLong(iChars);
                    iChars += item.text.length() + 1;
                }
            }

            // strings
            for (k=0;k<bar.menus.size();k++) {
                PalmMenu& menu = bar.menus[k];
                dbstr.addString(menu.text.c_str());
            }
            for (k=0;k<bar.menus.size();k++) {
                PalmMenu& menu = bar.menus[k];
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    dbstr.addString(item.text.c_str());
                }
            }

            PalmResRecord rec;
            rec.id = bar.id;
            rec.type = "MBAR";
            rec.len = size;
            rec.data = dbstr.data;
            db.AddResRec(rec);
        }
    }

    return true;
}

const int nResHandlers[rtLastResType] = {
    10, // rtForm
    0, // rtLabel
    2, // rtField
    1, // rtButton
    1, // rtPushButton
    1, // rtRepeatButton
    1, // rtCheckbox
    2, // rtPopup
    1, // rtList
    1, // rtSelector
    0, // rtBitmap
    0, // rtGraffiti
    2, // rtScroll
    6, // rtGadget
    0, // rtMenu
    4, // rtApp
    0, // rtString
    1, // rtMenuItem
    0, // rtIcon
    0, // rtMenuBar
    2, // slider
};

#define HANDLERS_SIZE(rt) (4 * nResHandlers[rt])
#define CB_RECT 8
#define CB_ORIGIN 4
#define CB_COUNT 2
#define CB_LEN 2
#define CB_ADDR_ID 4
#define CB_ADDR 2
#define CB_CTYPE 1
#define CB_FLAGS 2

const int cbRes[rtLastResType] = {
    CB_ADDR_ID + CB_RECT + CB_COUNT + CB_LEN + CB_FLAGS + HANDLERS_SIZE(rtForm), // rtForm
    CB_CTYPE + CB_ORIGIN + CB_FLAGS + CB_ADDR_ID + CB_LEN + HANDLERS_SIZE(rtLabel), // rtLabel
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtField), // rtField
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtButton), // rtButton
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtPushButton), // rtPushButton
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtRepeatButton), // rtRepeatButton
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtCheckbox), // rtCheckbox
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtPopup), // rtPopup
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtList), // rtList
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtSelector), // rtSelector
    CB_CTYPE + CB_ORIGIN + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtBitmap), // rtBitmap
    CB_CTYPE + CB_ORIGIN + CB_FLAGS + HANDLERS_SIZE(rtGraffiti), // rtGraffiti
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtScroll), // rtScroll
    CB_CTYPE + CB_FLAGS + CB_ADDR_ID + CB_RECT + HANDLERS_SIZE(rtGadget), // rtGadget
    HANDLERS_SIZE(rtMenu), // rtMenu
    CB_ADDR + CB_COUNT + CB_COUNT + CB_COUNT + HANDLERS_SIZE(rtApp), // rtApp
    HANDLERS_SIZE(rtString), // rtString
    CB_ADDR_ID + HANDLERS_SIZE(rtMenuItem), // rtMenuItem
    HANDLERS_SIZE(rtIcon), // rtIcon
    HANDLERS_SIZE(rtMenuBar), // rtMenuBar
    CB_CTYPE + CB_RECT + CB_FLAGS + CB_ADDR_ID + HANDLERS_SIZE(rtSlider), // rtSlider
};

/*
const int cbRes[rtLastResType] = {
    24 + (4 * nResHandlers[rtForm]), // rtForm
    9 + 6 + (4 * nResHandlers[rtLabel]), // rtLabel
    9 + 10 + (4 * nResHandlers[rtField]), // rtField
    9 + 17 + (4 * nResHandlers[rtButton]), // rtButton
    9 + 17 + (4 * nResHandlers[rtPushButton]), // rtPushButton
    9 + 17 + (4 * nResHandlers[rtRepeatButton]), // rtRepeatButton
    9 + 17 + (4 * nResHandlers[rtCheckbox]), // rtCheckbox
    9 + 13 + (4 * nResHandlers[rtPopup]), // rtPopup
    9 + 9 + (4 * nResHandlers[rtList]), // rtList
    9 + 11 + (4 * nResHandlers[rtSelector]), // rtSelector
    9 + 2 + (4 * nResHandlers[rtBitmap]), // rtBitmap
    9 + 0 + (4 * nResHandlers[rtGraffiti]), // rtGraffiti
    9 + 10 + (4 * nResHandlers[rtScroll]), // rtScroll
    9 + 4 + (4 * nResHandlers[rtGadget]), // rtGadget
    0 + (4 * nResHandlers[rtMenu]), // rtMenu
    2 + (4 * nResHandlers[rtApp]), // rtApp
    0 + (4 * nResHandlers[rtString]), // rtString
    4 + (4 * nResHandlers[rtMenuItem]), // rtMenuItem
    0 + (4 * nResHandlers[rtIcon]), // rtIcon
    0 + (4 * nResHandlers[rtMenuBar]), // rtMenuBar
    9 + 15 + (4 * nResHandlers[rtSlider]), // rtSlider
};
*/
const int cbRes2[rtLastResType] = {
    40 + 28, // rtForm
    14, // rtLabel
    40, // rtField
    20, // rtButton
    20, // rtPushButton
    20, // rtRepeatButton
    20, // rtCheckbox
    20, // rtPopup
    32, // rtList
    20, // rtSelector
    8, // rtBitmap
    4, // rtGraffiti
    24, // rtScroll
    16, // rtGadget
    -1, // rtMenu - objects 
    -1, // rtApp
    -1, // rtString
    -1, // rtMenuItem
    -1, // rtIcon
    -1, // rtMenuBar
    30, // rtSlider
};

byte mapResType[rtLastResType] = {
    -1, // rtForm
    8, // rtLabel
    0, // rtField
    1, // rtButton
    1, // rtPushButton
    1, // rtRepeatButton
    1, // rtCheckbox
    1, // rtPopup
    2, // rtList
    1, // rtSelector
    4, // rtBitmap
    11, // rtGraffiti
    13, // rtScroll
    12, // rtGadget
    -1, // rtMenu - objects 
    -1, // rtApp
    -1, // rtString
    -1, // rtMenuItem
    -1, // rtIcon
    -1, // rtMenuBar
    1, // rtSlider //14
};

byte mapResControlStyle[rtLastResType] = {
    -1, // rtForm
    -1, // rtLabel
    -1, // rtField
    0, // rtButton
    1, // rtPushButton
    5, // rtRepeatButton
    2, // rtCheckbox
    3, // rtPopup
    -1, // rtList
    4, // rtSelector
    -1, // rtBitmap
    -1, // rtGraffiti
    -1, // rtScroll
    -1, // rtGadget
    -1, // rtMenu - objects 
    -1, // rtApp
    -1, // rtString
    -1, // rtMenuItem
    -1, // rtIcon
    -1, // rtMenuBar
    7, // rtSlider
};

const char* handlerNames[rtLastResType][MAX_HANDLERS] = {
    { "onchar", "onhkey", "onopen", "onclose", "ondraw", "onpendown", "onpenup", "onpenmove", "ontimer", "onresize" }, // rtForm
    { NULL }, // rtLabel
    { "onhchange", "onchange" }, // rtField
    { "onselect" }, // rtButton
    { "onselect" }, // rtPushButton
    { "onselect" }, // rtRepeatButton
    { "onselect" }, // rtCheckbox
    { "onselect", "onlistselect" }, // rtPopup
    { "onselect" }, // rtList
    { "onselect" }, // rtSelector
    { NULL }, // rtBitmap
    { NULL }, // rtGraffiti
    { "onmove", "ondone" }, // rtScroll
    { "onopen", "onclose", "ondraw", "onpendown", "onpenup", "onpenmove" }, // rtGadget
    { NULL }, // rtMenu
    { "onstart", "onstop", "onbeam", "onalarm" }, // rtApp
    { NULL }, // rtString
    { "onselect" }, // rtMenuItem
    { NULL }, // rtIcon
    { NULL }, // rtMenuBar
    { "onmove", "ondone" }, // rtSlider
};

void Compiler::addHandler(PalmDBStream& db, long h) {
    unsigned long ul = h;
    if (ul & 0x80000000) {
        db.addLong(ul);
    } else {
        if (ul > funcs.size())
            c_error("handler out of range!", -1);
        else
            db.addLong(funcs[ul].loc);
    }
}

static int CtrlSize(PalmControl& ctrl) {
    int size = 0;
    ResType rt = ctrl.type;
    size = cbRes2[rt];
    switch (rt) {
        case rtLabel:
        case rtCheckbox:
        case rtPopup:
        case rtSelector:
            size += ctrl.text.length() + 1;
            break;
        case rtButton:
        case rtPushButton:
        case rtRepeatButton:
            if (!ctrl.graph) {
                size += ctrl.text.length() + 1;
            }
            break;
        case rtList:
            //size += 6 * ctrl.nVisible;
            break;
    }
    return size + (size & 0x1); // pad length to word
}

bool Compiler::genForms(PalmDB& db, int& osver) {
    int i, j;
    int size = 0;
    int nControls, nPopups;
    const int cbTitle = 12;
    const int cbListEntry = 6;
    const int cbPopup = 4;
    bool bHasTitle;

    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        nControls = nPopups = 0;

        bHasTitle = form.text.length() != 0;
        size = cbRes2[rtForm];
        if (bHasTitle) {
            size += cbTitle + form.text.length() + 1;
            if (size % 2) size++;
            nControls = 1;
        }

        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            nControls++;
            size += CtrlSize(ctrl);
            if (rt == rtList && ctrl.triggerID) {
                nPopups++;
                size += cbPopup;
            }
        }

        size += cbListEntry * (nControls + nPopups);

        PalmDBStream dbstr(size);

        // write form's window structure
        dbstr.addWord(0); // window.displayWidth
        dbstr.addWord(0); // h
        dbstr.addLong(0); 
        byte wndFlags = 0x12; // focusable/dialog
        if (form.modal)
            wndFlags |= 0x20;
        dbstr.addByte(wndFlags);
        dbstr.addByte(0); // reserved part of flags
        dbstr.addWord(form.x);
        dbstr.addWord(form.y);
        dbstr.addWord(form.w);
        dbstr.addWord(form.h);
        dbstr.addLong(0); //
        dbstr.addLong(0); //
        dbstr.addLong(0); //
        if (form.modal) {
            dbstr.addByte(3);
            dbstr.addByte(2);
        } else {
            dbstr.addByte(0); // TODO: corner diameter
            dbstr.addByte(0); // shadow w | w
        }
        dbstr.addLong(0);
        dbstr.addLong(0);

        // write form data
        dbstr.addWord(form.id);
        byte formAttr = 0x80; // usable
        if (form.modal && !form.resizable) {
            formAttr |= 0x08; // savebehind
        }
        // TODO: grafittiShift?
        dbstr.addByte(formAttr);
        dbstr.addByte(0);
        dbstr.addWord(0); // remainder of formattr
        dbstr.addLong(0);
        dbstr.addLong(0);
        dbstr.addWord(0); // focus
        dbstr.addWord(form.defbuttonID);
        dbstr.addWord(form.helpID);
        dbstr.addWord(form.menuID);
        dbstr.addWord(nControls + nPopups); // 1 for title
        dbstr.addLong(0);

        // write control list
        int offset = cbRes2[rtForm] + cbListEntry * (nControls + nPopups);
        if (bHasTitle) {
            dbstr.addByte(9); // title = 9
            dbstr.addByte(0);
            dbstr.addLong(offset);
            offset += cbTitle + form.text.length() + 1;
            if (offset % 2) offset++;
        }
        // add actual controls
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            dbstr.addByte(mapResType[rt]); // TODO: is graphic control different?
            dbstr.addByte(0);
            dbstr.addLong(offset);
            offset += CtrlSize(ctrl);
        }
        // add popups
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            if (rt == rtList && ctrl.triggerID) {
                dbstr.addByte(10); // popup = 10
                dbstr.addByte(0);
                dbstr.addLong(offset);
                offset += cbPopup;
            }
        }

        // write the title
        if (bHasTitle) {
            dbstr.addLong(0);
            dbstr.addLong(0);
            dbstr.addLong(0);
            dbstr.addStringPad(form.text.c_str());
        }

        // write the controls
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;

            switch (ctrl.type) {
                case rtLabel:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addByte(0x80); // usable
                    dbstr.addByte(0);
                    dbstr.addByte(ctrl.fontID);
                    dbstr.addByte(0);
                    dbstr.addLong(0);
                    dbstr.addStringPad(ctrl.text.c_str());
                    break;
                case rtBitmap:
                    dbstr.addByte(0x80); // usable
                    dbstr.addByte(0);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.bitmapID);
                    break;
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtPopup:
                case rtSelector:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    if (ctrl.graph) {
                        dbstr.addWord(ctrl.bitmapID);
                        dbstr.addWord(ctrl.sbitmapID);
                    } else {
                        dbstr.addLong(0); // text
                    }
                    {
                        byte attrib = 0xc0; // usable/enabled
                        if (ctrl.type == rtButton || ctrl.type == rtRepeatButton) {
                            attrib |= ctrl.frame;
                        }
                        if (ctrl.leftAnchor)
                            attrib |= 0x08;
                        dbstr.addByte(attrib);
                        if (ctrl.graph) {
                            dbstr.addByte(0x40);
                            if (osver < 0x350)
                                osver = 0x350;
                        } else
                            dbstr.addByte(0);
                    }
                    dbstr.addByte(mapResControlStyle[rt]);
                    dbstr.addByte(ctrl.fontID);
                    dbstr.addByte(ctrl.group);
                    dbstr.addByte(0);
                    if (!ctrl.graph)
                        dbstr.addStringPad(ctrl.text.c_str());
                    break;
                case rtField: 
                {
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);

                    byte attrib = 0x82; // usable, insPtVisible
                    if (ctrl.editable) attrib |= 0x20;
                    if (ctrl.singleLine) attrib |= 0x10;
                    if (ctrl.dynSize) attrib |= 0x04;
                    dbstr.addByte(attrib);

                    attrib = 0;
                    if (ctrl.numeric) attrib = 0x02;
                    if (ctrl.hasScroll) attrib |= 0x04;
                    if (ctrl.autoShift) attrib |= 0x08;
                    attrib |= (ctrl.underline << 6);
                    if (!ctrl.leftAnchor) attrib |= 0x20;
                    dbstr.addByte(attrib);

                    dbstr.addLong(0); // text
                    dbstr.addLong(0);
                    dbstr.addLong(0);
                    dbstr.addLong(0);
                    dbstr.addWord(ctrl.maxChars);
                    dbstr.addLong(0);
                    dbstr.addLong(0);
                    dbstr.addByte(ctrl.fontID);
                    dbstr.addByte(0);
                    break;
                }
                case rtGraffiti:
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    break;
                case rtGadget:
                    dbstr.addWord(ctrl.id);
                    dbstr.addByte(0x80); // usable
                    dbstr.addByte(0);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addLong(0);
                    break;
                case rtScroll:
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.id);
                    dbstr.addByte(0x80); // usable
                    dbstr.addByte(0);
                    dbstr.addWord(0);
                    dbstr.addWord(ctrl.iMin);
                    dbstr.addWord(ctrl.iMax);
                    dbstr.addWord(ctrl.page);
                    dbstr.addLong(0);
                    break;
                case rtSlider:
                    if (osver < 0x350)
                        osver = 0x350;
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.bitmapID);
                    dbstr.addWord(ctrl.sbitmapID);
                    {
                        byte attrib = 0xc0; // usable/enabled
                        dbstr.addByte(attrib);
                        dbstr.addByte(0x40); // always graphical
                    }
                    dbstr.addByte(7); // feedbackSlider = 7
                    dbstr.addByte(0);
                    dbstr.addWord(ctrl.iMin);
                    dbstr.addWord(ctrl.iMax);
                    dbstr.addWord(ctrl.page);
                    dbstr.addWord(0);
                    dbstr.addLong(0);
                    break;
                case rtList:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addByte(ctrl.triggerID ? 0 : 0x80); // usable
                    dbstr.addByte(0);
                    dbstr.addLong(0);
                    dbstr.addWord(0);
                    dbstr.addLong(0);
                    dbstr.addByte(ctrl.fontID);
                    dbstr.addByte(0);
                    dbstr.addLong(0);
                    dbstr.addLong(0);
                    break;
            }
        }

        // write the popups
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            if (rt == rtList && ctrl.triggerID) {
                dbstr.addWord(ctrl.triggerID);
                dbstr.addWord(ctrl.id);
            }
        }

        PalmResRecord rec;
        rec.type = "tFRM";
        rec.id = form.id;
        rec.len = size;
        rec.data = dbstr.data;
        db.AddResRec(rec);
    }

    return true;
}

bool Compiler::genUI(PalmDB& db) {
    int i,j,k,m;
    int size = 0;
    int nMenuItems = 0, nControls = 0;

    // precalc size
    size = cbRes[rtApp];

    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        size += cbRes[rtForm];
        nControls += form.controls.size();
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            size += cbRes[rt];
        }
        for (k=0;k<form.menubars.size();k++) {
            PalmMenuBar& bar = form.menubars[k];
            for (j=0;j<bar.menus.size();j++) {
                PalmMenu& menu = bar.menus[j];
                nMenuItems += menu.items.size();
                size += menu.items.size() * cbRes[rtMenuItem];
            }
        }
    }

    PalmDBStream dbstr(size);

    // app
    dbstr.addWord(app.addr);
    for (i=0;i<nResHandlers[rtApp];i++)
        addHandler(dbstr, app.handlers[i]);

    dbstr.addWord(app.forms.size());
    dbstr.addWord(nControls);
    dbstr.addWord(nMenuItems);

    // menuitems 
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        for (k=0;k<form.menubars.size();k++) {
            PalmMenuBar& bar = form.menubars[k];
            for (j=0;j<bar.menus.size();j++) {
                PalmMenu& menu = bar.menus[j];
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    if (item.sysid)
                        dbstr.addWord(item.sysid);
                    else
                        dbstr.addWord(item.id);
                    dbstr.addWord(item.addr);
                    addHandler(dbstr, item.onselect);
                }
            }
        }
    }
    
    // forms
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        dbstr.addWord(form.id);
        dbstr.addWord(form.addr);
        dbstr.addWord(form.x);
        dbstr.addWord(form.y);
        dbstr.addWord(form.w);
        dbstr.addWord(form.h);
        dbstr.addWord(form.controls.size());
        dbstr.addWord(form.text.length() + 1);
        if (form.resizable) { // && !form.modal) { // for now, disable resizable modal forms
            word flags = form.hresize | (form.vresize << 2);
            flags |= 0x10; // resizable
            dbstr.addWord(flags);
        } else {
            dbstr.addWord(0); // flags
        }

        for (k=0;k<nResHandlers[rtForm];k++)
            addHandler(dbstr, form.handler[k]);

        // gen controls
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            dbstr.addByte(ctrl.type);
            word flags;
            flags = ctrl.hresize | (ctrl.vresize << 2);
            dbstr.addWord(flags);
            switch (ctrl.type) {
                case rtLabel:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.addr);
                    dbstr.addWord(ctrl.text.length() + 1);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    break;
                case rtBitmap:
                    dbstr.addWord(ctrl.bitmapID);
                    dbstr.addWord(ctrl.addr);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    break;
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtField: 
                case rtPopup:
                case rtSelector:
                case rtScroll:
                case rtSlider:
                case rtList:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.addr);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    break;
                case rtGraffiti:
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    break;
                case rtGadget:
                    dbstr.addWord(ctrl.id);
                    dbstr.addWord(ctrl.addr);
                    dbstr.addWord(ctrl.x);
                    dbstr.addWord(ctrl.y);
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    break;
            }
            for (k=0;k<nResHandlers[ctrl.type];k++)
                addHandler(dbstr, ctrl.handler[k]);
        }
    }

    PalmResRecord rec;
    rec.type = "ofUI";
    rec.id = 0;
    rec.len = size;
    rec.data = dbstr.data;
    db.AddResRec(rec);
    return true;
}

/*
bool Compiler::genForms(PalmDB& db) {
    int i,j,k,m;
    int size = 0;
    int iChar, nChar = 0, nMenuItems = 0, nControls = 0;

    // precalc size
    size = cbRes[rtApp];
    size += 2; // nMenuItems
    size += 2; // # of forms
    size += 2; // # of controls

    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        size += cbRes[rtForm];
        nChar += form.text.length() + 1;
        nControls += form.controls.size();
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            size += cbRes[rt];
            switch (rt) {
                case rtLabel:
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtPopup:
                case rtSelector:
                    nChar += ctrl.text.length() + 1;
                    break;
            }
        }
        for (k=0;k<form.menubars.size();k++) {
            PalmMenuBar& bar = form.menubars[k];
            for (j=0;j<bar.menus.size();j++) {
                PalmMenu& menu = bar.menus[j];
                nMenuItems += menu.items.size();
                size += menu.items.size() * cbRes[rtMenuItem];
            }
        }
    }

    iChar = size;
    size += nChar;

    PalmDBStream dbstr(size);

    // app
    dbstr.addWord(app.addr);
    for (i=0;i<nResHandlers[rtApp];i++)
        addHandler(dbstr, app.handlers[i]);

    dbstr.addWord(app.forms.size());
    dbstr.addWord(nControls);
    dbstr.addWord(nMenuItems);

    // menuitems 
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        for (k=0;k<form.menubars.size();k++) {
            PalmMenuBar& bar = form.menubars[k];
            for (j=0;j<bar.menus.size();j++) {
                PalmMenu& menu = bar.menus[j];
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    dbstr.addWord(item.id);
                    dbstr.addWord(item.addr);
                    addHandler(dbstr, item.onselect);
                }
            }
        }
    }
    
    // forms
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        dbstr.addWord(form.id);
        dbstr.addWord(form.addr);
        dbstr.addWord(form.x);
        dbstr.addWord(form.y);
        dbstr.addWord(form.w);
        dbstr.addWord(form.h);
        dbstr.addLong(iChar);
        iChar += form.text.length() + 1;
        dbstr.addByte(form.modal);
        dbstr.addWord(form.defbuttonID);
        dbstr.addWord(form.helpID);
        dbstr.addWord(form.menuID);
        dbstr.addByte(form.controls.size());

        for (k=0;k<nResHandlers[rtForm];k++)
            addHandler(dbstr, form.handler[k]);

        // gen controls
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            dbstr.addByte(ctrl.type);
            dbstr.addWord(ctrl.id);
            dbstr.addWord(ctrl.addr);
            dbstr.addWord(ctrl.x);
            dbstr.addWord(ctrl.y);
            switch (ctrl.type) {
                case rtLabel:
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addLong(iChar);
                    iChar += ctrl.text.length() + 1;
                    break;
                case rtBitmap:
                    dbstr.addWord(ctrl.bitmapID);
                    break;
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addByte(ctrl.group);
                    dbstr.addByte(ctrl.leftAnchor);
                    dbstr.addByte(ctrl.graph);
                    dbstr.addWord(ctrl.bitmapID);
                    dbstr.addWord(ctrl.sbitmapID);
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addLong(iChar);
                    iChar += ctrl.text.length() + 1;
                    break;
                case rtField: 
                {
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addByte(ctrl.leftAnchor);
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addWord(ctrl.maxChars);
                    byte flags;
                    flags = ctrl.autoShift << 6;
                    flags |= ctrl.dynSize << 5;
                    flags |= ctrl.editable << 4;
                    flags |= ctrl.hasScroll << 3;
                    flags |= ctrl.numeric << 2;
                    flags |= ctrl.singleLine << 1;
                    flags |= ctrl.underline;
                    dbstr.addByte(flags);
                    break;
                }
                case rtGraffiti:
                    break;
                case rtGadget:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    break;
                case rtPopup:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.listID);
                    dbstr.addByte(ctrl.leftAnchor);
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addLong(iChar);
                    iChar += ctrl.text.length() + 1;
                    break;
                case rtSelector:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addByte(ctrl.leftAnchor);
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addLong(iChar);
                    iChar += ctrl.text.length() + 1;
                    break;
                case rtScroll:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.iMin);
                    dbstr.addWord(ctrl.iMax);
                    dbstr.addWord(ctrl.page);
                    break;
                case rtSlider:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.iMin);
                    dbstr.addWord(ctrl.iMax);
                    dbstr.addWord(ctrl.page);
                    dbstr.addByte(ctrl.graph);
                    dbstr.addWord(ctrl.bitmapID);
                    dbstr.addWord(ctrl.sbitmapID);
                    break;
                case rtList:
                    dbstr.addWord(ctrl.w);
                    dbstr.addWord(ctrl.h);
                    dbstr.addWord(ctrl.fontID);
                    dbstr.addByte(ctrl.nVisible);
                    dbstr.addWord(ctrl.triggerID);
                    break;
            }
            for (k=0;k<nResHandlers[ctrl.type];k++)
                addHandler(dbstr, ctrl.handler[k]);
        }
    }

    // strings
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        dbstr.addString(form.text.c_str());
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            ResType rt = ctrl.type;
            switch (rt) {
                case rtLabel:
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtPopup:
                case rtSelector:
                    if (!ctrl.graph)
                        dbstr.addString(ctrl.text.c_str());
                    break;
            }
        }
    }

    PalmResRecord rec;
    rec.type = "ofUI";
    rec.id = 0;
    rec.len = size;
    rec.data = dbstr.data;
    db.AddResRec(rec);
    return true;
}
*/

bool Compiler::genApp(PalmDB& db, int& osver) {
    if (app.name.empty()) {
        c_error("application properties not set - @app directive not found", -1);
    }

    PalmResRecord rec;
    int i;

    genForms(db, osver);
    if (!app.resOnly)
        genUI(db);
    genMenus(db);

    // gen icon
#ifdef WIN32
    bool bNoIcon = true;
    for (i=0;i<10;i++) {
        if (!app.icon.file[i].empty() || !app.icon.sfile[i].empty()) {
            bNoIcon = false;
            break;
        }
    }
    if (!app.icon.res.empty() || !app.icon.sres.empty())
        bNoIcon = false;

    if (!bNoIcon) {
        if (app.icon.res.empty()) {
            string err;
            string files[10];
            for (i=0;i<10;i++) {
                files[i] = lex.FindFile(app.icon.file[i].c_str());
            }
            PalmBitmapFamily family = { NULL };
            for (i=0;i<5;i++) {
                family.images[i] = (char*)files[i].c_str();
                family.imagesHigh[i] = (char*)files[i+5].c_str();
            }
            if (!db.CreateBitmapFamily(1000, true, &family, err)) {
                c_error(("unable to create large icon: " + err).c_str(), -1);
            }
        } else {
            string err;
            if (!copyResFromPRC(db, "tAIB", 1000, app.icon.res, err))
                c_error(("unable to create large icon: " + err).c_str(), -1);
        }
        if (app.icon.sres.empty()) {
            string err;
            string files[10];
            for (i=0;i<10;i++) {
                files[i] = lex.FindFile(app.icon.sfile[i].c_str());
            }
            PalmBitmapFamily family = { NULL };
            for (i=0;i<5;i++) {
                family.images[i] = (char*)files[i].c_str();
                family.imagesHigh[i] = (char*)files[i+5].c_str();
            }
            if (!db.CreateBitmapFamily(1001, true, &family, err)) {
                c_error(("unable to create small icon: " + err).c_str(), -1);
            }
        } else {
            string err;
            if (!copyResFromPRC(db, "tAIB", 1001, app.icon.sres, err))
                c_error(("unable to create small icon: " + err).c_str(), -1);
        }
    }
#else
    if (!app.icon.res.empty()) {
        string err;
        if (!copyResFromPRC(db, "tAIB", 1000, app.icon.res, err))
            c_error(("unable to create large icon: " + err).c_str(), -1);
    }
    if (!app.icon.sres.empty()) {
        string err;
        if (!copyResFromPRC(db, "tAIB", 1001, app.icon.sres, err))
            c_error(("unable to create large icon: " + err).c_str(), -1);
    }
#endif

#ifdef WIN32
    // gen external bitmaps
    for (i=0;i<app.externalBitmaps.size();i++) {
        if (app.externalBitmaps[i].res.empty()) {
            string err;
            string files[10];
            for (int j=0;j<10;j++) {
                files[j] = lex.FindFile(app.externalBitmaps[i].file[j].c_str());
            }
            PalmBitmapFamily family = { NULL };
            for (int k=0;k<5;k++) {
                family.images[k] = (char*)files[k].c_str();
                family.imagesHigh[k] = (char*)files[k+5].c_str();
            }
            if (!db.CreateBitmapFamily(app.externalBitmaps[i].id, false, &family, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.externalBitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        } else {
            string err;
            if (!copyResFromPRC(db, "Tbmp", app.externalBitmaps[i].id, app.externalBitmaps[i].res, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.externalBitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        }
    }

    // gen bitmaps
    for (i=0;i<app.bitmaps.size();i++) {
        if (app.bitmaps[i].res.empty()) {
            string err;
            string files[10];
            for (int j=0;j<10;j++) {
                files[j] = lex.FindFile(app.bitmaps[i].file[j].c_str());
            }
            PalmBitmapFamily family = { NULL };
            for (int k=0;k<5;k++) {
                family.images[k] = (char*)files[k].c_str();
                family.imagesHigh[k] = (char*)files[k+5].c_str();
            }
            if (!db.CreateBitmapFamily(app.bitmaps[i].id, false, &family, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.bitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        } else {
            string err;
            if (!copyResFromPRC(db, "Tbmp", app.bitmaps[i].id, app.bitmaps[i].res, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.bitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        }
    }
#else
    // gen external bitmaps
    for (i=0;i<app.externalBitmaps.size();i++) {
        if (!app.externalBitmaps[i].res.empty()) {
            string err;
            if (!copyResFromPRC(db, "Tbmp", app.externalBitmaps[i].id, app.externalBitmaps[i].res, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.externalBitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        }
    }

    // gen bitmaps
    for (i=0;i<app.bitmaps.size();i++) {
        if (!app.bitmaps[i].res.empty()) {
            string err;
            if (!copyResFromPRC(db, "Tbmp", app.bitmaps[i].id, app.bitmaps[i].res, err)) {
                string err2 = "unable to create bitmap '";
                err2 += app.bitmaps[i].name + "': ";
                err2 += err;
                c_error(err2.c_str(), -1);
            }
        }
    }
#endif

    // merge resources
#ifdef WIN32
    for (i=0;i<app.resources.size();i++) {
        if (!app.resources[i].res.empty()) {
            // add from a PRC
            string err;
            if (app.resources[i].id != -1) {
                if (!copyResFromPRC(db, app.resources[i].type, app.resources[i].id, app.resources[i].res, err)) {
                    string err2 = "unable to merge resource '";
                    err2 += app.resources[i].res + "': ";
                    err2 += err;
                    c_error(err2.c_str(), -1);
                }
            } else {
                if (!copyAllResFromPRC(db, app.resources[i].res, err)) {
                    string err2 = "unable to merge all resources '";
                    err2 += app.resources[i].res + "': ";
                    err2 += err;
                    c_error(err2.c_str(), -1);
                }
            }
        } else {
            // add from a file
            PalmResRecord rrec;
            rrec.type = app.resources[i].type;
            rrec.id = app.resources[i].id;
            string file = lex.FindFile(app.resources[i].file.c_str(), false, true);
            Source* source = GetFile(file.c_str(), false, &rrec.len);
            if (!source) {
                c_error((string("unable to open file '") + app.resources[i].file + "'").c_str(), -1);
            }
            rrec.data = (byte*)source->GetSrc();
            db.AddResRec(rrec);
            delete source;
        }
    }
#else
    for (i=0;i<app.resources.size();i++) {
        if (!app.resources[i].res.empty()) {
            string err;
            if (app.resources[i].id != -1) {
                if (!copyResFromPRC(db, app.resources[i].type, app.resources[i].id, app.resources[i].res, err)) {
                    string err2 = "unable to merge resource '";
                    err2 += app.resources[i].res + "': ";
                    err2 += err;
                    c_error(err2.c_str(), -1);
                }
            } else {
                if (!copyAllResFromPRC(db, app.resources[i].res, err)) {
                    string err2 = "unable to merge all resources '";
                    err2 += app.resources[i].res + "': ";
                    err2 += err;
                    c_error(err2.c_str(), -1);
                }
            }
        }
    }
#endif

    if (!app.resOnly) {
        // gen version
        rec.id = 1000;
        rec.data = (byte*)app.version.c_str();
        rec.len = app.version.length() + 1;
        rec.type = "tver";
        db.AddResRec(rec);

        // gen display name
        rec.id = 1000;
        rec.data = (byte*)app.dname.c_str();      
        rec.len = app.dname.length() + 1;
        rec.type = "tAIN";
        db.AddResRec(rec);
    }

    // gen strings
    for (i=0;i<app.strings.size(); i++) {
        rec.id = app.strings[i].id;
        rec.data = (byte*)app.strings[i].str.c_str();
        rec.len = app.strings[i].str.length() + 1;
        rec.type = "tSTR";
        db.AddResRec(rec);
    }
    return true;
}

// handler order
// 1. instance handler
// 2. gadget handler
// 3. type handler

void Compiler::setupHandler(const char* objname, const char* eventname,
                            int sid, int gsid, bool isGadget, long* pHandler) {
    string hname = objname;
    hname += ".";
    hname += eventname;
    int funcID = findFunc(hname);
    if (funcID >= 0) {
        *pHandler = funcID;
        return;
    }

    funcID = findStructFunc(sid, eventname);
    if (funcID >= 0) {
        if (funcs[funcID].builtin) {
            *pHandler = 0x80000000 | ((funcs[funcID].lib & 0xff) << 16) | (funcs[funcID].loc);
        } else {
            *pHandler = funcID;
        }
        return;
    }
    if (isGadget) {
        // TODO: this is never called!
        funcID = findStructFunc(gsid, eventname);
        if (funcID >= 0) {
            if (funcs[funcID].builtin) {
                *pHandler = 0x80000000 | ((funcs[funcID].lib & 0xff) << 16) | (funcs[funcID].loc);
            } else {
                *pHandler = funcID;
            }
            return;
        }
    }
    *pHandler = 0xffffffff;
}

bool Compiler::HookUpHandlers() {
    if (app.name.empty())
        return true; // no handlers

    string hname;
    int i, j, k, m, sid, gsid, rid;
    gsid = findStruct("UIGadget");
    if (gsid == -1)
        c_error("UIGadget must be defined", -1);

    // app handlers
    rid = findRes(app.name);
    sid = resSyms[rid].structID;
    for (i=0;i<nResHandlers[rtApp];i++)
        setupHandler(app.name.c_str(), handlerNames[rtApp][i], sid, -1, false, &app.handlers[i]);

    // menu handlers
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        for (k=0;k<form.menubars.size();k++) {
            PalmMenuBar& bar = form.menubars[k];
            for (j=0;j<bar.menus.size();j++) {
                PalmMenu& menu = bar.menus[j];
                for (m=0;m<menu.items.size();m++) {
                    PalmMenuItem& item = menu.items[m];
                    rid = findRes(item.name);
                    sid = resSyms[rid].structID;
                    setupHandler(item.name.c_str(), handlerNames[rtMenuItem][0], sid, -1, false, &item.onselect);
                }
            }
        }
    }

    // form handlers
    for (i=0;i<app.forms.size();i++) {
        PalmForm& form = app.forms[i];
        rid = findRes(form.name);
        sid = resSyms[rid].structID;
        for (k=0;k<nResHandlers[rtForm];k++)
            setupHandler(form.name.c_str(), handlerNames[rtForm][k], sid, -1, false, &form.handler[k]);

        // gen controls
        for (j=0;j<form.controls.size();j++) {
            PalmControl& ctrl = form.controls[j];
            if (ctrl.type == rtGraffiti) continue;
            rid = findRes(ctrl.name);
            sid = resSyms[rid].structID;
            for (k=0;k<nResHandlers[ctrl.type];k++)
                setupHandler(ctrl.name.c_str(), handlerNames[ctrl.type][k], sid, -1, false, &ctrl.handler[k]);
            if (ctrl.type == rtGadget) {
                for (k=0;k<structs[sid].vars.size();k++) {
                    Variable& var = structs[sid].vars[k];
                    if (var.type.vtype == vtHandler) {
                        long h = -1;
                        setupHandler(ctrl.name.c_str(), var.name.c_str(), sid, -1, false, &h);
                        // add global data
                        GlobalInit gi;
                        gi.offset = ctrl.addr + var.offset;
                        gi.val.type = vtFuncPtr;
                        gi.val.iVal = h;
                        globInits.push_back(gi);
                    }
                }
            }
        }
    }

    return true;
}

const int fontWidth[] = {
    5,5,5,5,5,5,5,5,5,2,5,5,5,5,5,5,
    5,5,5,5,11,5,10,8,6,6,5,5,5,5,5,5,
    2,3,6,10,6,13,9,3,5,5,6,6,3,5,3,6,
    6,6,6,6,6,6,6,6,6,6,3,3,6,6,6,6,
    10,7,7,6,7,5,5,8,8,3,5,7,6,10,7,8,
    7,8,8,6,7,7,8,11,7,7,7,4,6,4,6,5,
    4,6,6,5,6,6,5,6,6,3,4,6,3,9,6,6,
    6,6,5,5,6,6,6,9,6,6,5,5,3,5,7,5,
    6,5,3,9,5,6,5,5,4,17,6,5,10,10,10,10,
    10,3,3,5,5,4,4,6,7,10,5,5,10,5,5,7,
    2,3,6,7,8,7,3,6,4,8,6,8,6,5,8,6,
    5,6,4,4,4,6,7,2,4,2,6,8,9,9,9,6,
    7,7,7,7,7,7,9,6,5,5,5,5,3,3,3,3,
    8,7,8,8,8,8,8,6,8,7,7,7,7,7,7,8,
    6,6,6,6,6,6,9,5,6,6,6,6,3,3,3,3,
    6,6,6,6,6,6,6,7,8,6,6,6,6,6,6,6
};


int stringWidth(const char* str) {
    int w = 0;
    while (*str) {
        w += fontWidth[*((unsigned char*)str)];
        //w += fontWidth[*str];
        str++;
    }
    return w;
}
