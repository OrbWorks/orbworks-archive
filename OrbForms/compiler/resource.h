#pragma once

#ifdef WIN32
#ifndef ovector
#define ovector vector
#endif
#endif

enum ResType {
    rtForm=0, rtLabel, rtField, rtButton, rtPushButton, rtRepeatButton, rtCheckbox, rtPopup,
    rtList, rtSelector, rtBitmap, rtGraffiti, rtScroll, rtGadget, rtMenu, rtApp,
    rtString, rtMenuItem, rtIcon, rtMenuBar, rtSlider, rtProject, rtLanguage, rtResource,
    rtLastResType
};
extern char* resNames[rtLastResType];
extern char* resTypes[rtLastResType];

#define MAX_HANDLERS 10
extern const int nResHandlers[rtLastResType];
extern const char* handlerNames[rtLastResType][MAX_HANDLERS];

enum PropType {
    ptName, ptText, ptX, ptY, ptW, ptH, ptVersion, ptCreator, ptFile, ptID,
    ptHelpID, ptBitmapID, ptSBitmapID, ptMenuID, ptFontID, ptLeftAnchor,
    ptModal, ptDefButtonID, ptGraphic, ptGroup, ptFrame, ptDBName,
    ptMenu, ptMenuItem, ptShortcut,
    // field props
    ptMaxChars, ptEditable, ptUnderline, ptSingleLine, ptDynamicSize,
    ptAutoShift, ptHasScroll, ptNumeric,
    // list/scroll/slider
    ptMin, ptMax, ptPage, ptNumVisible, ptTriggerID, ptListID,
    // bitmap/icons
    ptImage1, ptImage2, ptImage4, ptImage8, ptImage16,
    ptImageH1, ptImageH2, ptImageH4, ptImageH8, ptImageH16,
    ptSmall1, ptSmall2, ptSmall4, ptSmall8, ptSmall16,
    ptSmallH1, ptSmallH2, ptSmallH4, ptSmallH8, ptSmallH16,
    ptLarge1, ptLarge2, ptLarge4, ptLarge8, ptLarge16,
    ptLargeH1, ptLargeH2, ptLargeH4, ptLargeH8, ptLargeH16,
    // system menu id
    ptSysID,
    // resize
    ptResizable, ptHResize, ptVResize, ptLocked, ptHidden,
    // images from PRCs
    ptImageRes, ptSmallRes, ptLargeRes, ptResOnly,
    // embedded resources
    ptRes, ptType,

    ptLastPropType
};
extern char* propNames[ptLastPropType];

struct PalmControl {
    PalmControl() {
        type = rtLastResType;
        id = x = y = w = h = 0;
        fontID = group = graph = bitmapID = sbitmapID = hresize = vresize = 0;
        leftAnchor = frame = 1;
        modal = menuID = helpID = defbuttonID = resizable = 0;
        editable = singleLine = underline = 1;
        maxChars = dynSize = autoShift = hasScroll = numeric = 0;
        iMin = 0;
        iMax = 99;
        page = 20;
        nVisible = 5;
        triggerID = listID = 0;
        addr = -1;
        for (int i=0;i<MAX_HANDLERS;i++)
            handler[i] = -1;
    }

    ResType type;
    string name, typeName;
    int id;
    int x, y, w, h;
    string text;

    int fontID, leftAnchor, group;
    int graph, bitmapID, sbitmapID;
    int frame;
    int hresize, vresize;

    // form stuff
    int modal, menuID, helpID, defbuttonID, resizable;

    // field stuff
    int maxChars, editable, underline, singleLine, dynSize,
        autoShift, hasScroll, numeric;

    // scroll/slider
    int iMin, iMax, page;

    // list
    int nVisible, triggerID, listID;

    int addr;
    long handler[MAX_HANDLERS];
};

struct PalmBitmap {
    PalmBitmap() : w(0), h(0), id(0) { }
    string name;
    string file[10];
    string res;
    int w,h;
    int id;
};

struct PalmIcon {
    string file[10];
    string sfile[10];
    string res, sres;
};

struct PalmMenuItem {
    PalmMenuItem() : shortcut(0), id(0), sysid(0), onselect(-1), addr(-1) { }
    string name;
    string text;
    char shortcut;
    int id;
    int addr;
    long onselect;
    int sysid;
};

struct PalmMenu {
    PalmMenu() : id(0) { }
    string name;
    string text;
    ovector<PalmMenuItem> items;
    int id;
};

struct PalmMenuBar {
    PalmMenuBar() : id(0) { }
    string name;
    ovector<PalmMenu> menus;
    int id;
};

struct PalmString {
    PalmString() : id(0) { }
    string str;
    string name;
    int id;
};

struct PalmResource {
    PalmResource() : id(-1) { }
    string res;
    string file;
    string type;
    int id;
};

struct PalmForm : public PalmControl {
    ovector<PalmControl> controls;
    ovector<PalmMenuBar> menubars;
};

struct PalmApp {
    PalmApp() : addr(-1), locked(false), hidden(false), parsed(false), resOnly(false) {
        for (int i=0;i<MAX_HANDLERS;i++)
            handlers[i] = -1;
    }
    string name, dname, dbname, version;
    string creatorID;
    string file;

    PalmIcon icon;
    ovector<PalmBitmap> bitmaps;
    ovector<PalmBitmap> externalBitmaps;
    ovector<PalmForm> forms;
    ovector<PalmString> strings;
    ovector<PalmResource> resources;
    int addr;
    int locked;
    int hidden;
    bool resOnly;
    long handlers[MAX_HANDLERS];
    bool parsed;
};

struct PalmProject {
    PalmApp app;
    ovector<string> gadgets;
    ovector<string> files;
    ovector<string> translationFiles;
    string data;
};

struct ResSymbol {
    ResSymbol() : type(rtLastResType), structID(-1), loc(0) { }
    string name;
    ResType type;
    int structID;
    int loc;
};

