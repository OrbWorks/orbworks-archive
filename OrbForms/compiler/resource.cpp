#include "compiler.h"

char* resNames[rtLastResType] = {
    "form", "label", "field", "button", "pushbutton", "repeatbutton", "checkbox", "popup",
    "list", "selector", "bitmap", "graffiti", "scroll", "gadget", "menu", "app",
    "resstring", "menuitem", "icon", "menubar", "slider", "project", "language", "resource"
};

char* resTypes[rtLastResType] = {
    "UIForm", "UILabel", "UIField", "UIButton", "UIPushButton", "UIRepeatButton",
    "UICheckbox", "UIPopup", "UIList", "UISelector", "UIFBitmap", "", "UIScroll", "UIGadget",
    "UIMenu", "UIApp", "UIString", "UIMenuItem", "", "UIMenuBar", "UISlider", "", "", ""
};

char* propNames[ptLastPropType] = {
    "name", "text", "x", "y", "w", "h", "version", "creator", "file", "id",
    "helpid", "bitmapid", "sbitmapid", "menuid", "fontid", "leftanchor",
    "modal", "defbuttonid",	"graphic", "group", "frame", "dbname",
    "menu", "menuitem", "shortcut",
    "maxchars", "editable", "underline", "singleline", "dynamicsize", 
    "autoshift", "hasscroll", "numeric",
    "min", "max", "page", "numvisible", "triggerid", "listid",
    "image1", "image2", "image4", "image8", "image16",
    "imageh1", "imageh2", "imageh4", "imageh8", "imageh16",
    "small1", "small2", "small4", "small8", "small16",
    "smallh1", "smallh2", "smallh4", "smallh8", "smallh16",
    "large1", "large2", "large4", "large8", "large16",
    "largeh1", "largeh2", "largeh4", "largeh8", "largeh16",
    "sysid", "resizable", "hresize", "vresize", "locked", "hidden",
    "imageres", "smallres", "largeres", "resonly",
    "res", "type"
};


/* resource file structure

    All data  for a given element is separated by whitespace or comma

@app UIApp appName {
    name = "My App"
    DBname = "MyApp"
    file = "app.prc"
    cid = "MyaP" // 4 chars, at least one must be non-lowercase (uppercase/number/punc/etc.)
    version = "4.7"

    icon {
        small1="file.bmp", large8="file2.bmp"
        // smallx/largex where x=1,2,4,8. small1 and large1 MUST exist
    }

    bitmap Bitmap bmName {
        w=10, h=10
        id=13
        image8="file3.bmp" // imagex where x=1,2,4,8
        image1="file4.bmp"
    }
    
    resstring {
        text = "string"
        id = 87;
    }

    form UIForm formName {
        text="Form Title" // title
        x=2, y=2, w=160, h=160, id=1000
        modal=false // dialog?
        menuid=1002 // id of the default menu defined below
        helpid=4001 // id of a string, or 0. only if modal
        defbuttonid=1020 // id of button that is default, or 0 (i think)
        
        button UIButton okButton {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
            graphic=true, bitmapid=4500, sbitmapid=4501
            // bitmapids only for graphic buttons
            // w and h can be 0 for Palm to do the sizing based on text size
            // true/false are converted to 1/0 by lex, so use true/false
        }
        pushbutton UIPushButton optionA {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
            graphic=true, bitmapid=4500, sbitmapid=4501
        }
        repeatbutton UIRepeatButton repeatButton {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
            graphic=true, bitmapid=4500, sbitmapid=4501
        }
        checkbox UICheckbox myCheck {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
        }
        popup UIPopup popFruit {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
        }
        selector UISelector mysel {
            x=10, y=20, w=12, h=12, text="OK"
            id=1020, leftanchor=true
            fontid=1
        }
        label UILabel myLabel {
            x=10, y=20, text="My Label is Cool!"
            fontid=1
        }
        bitmap UIFBitmap formBitmap {
            x=20, y=30, id=13, bitmapid=4500
        }
        gadget UIGadget_type myGadget {
            // the UIGadget_type is the user provided gadget struct name
            // this struct must have UIGadget as the first member because
            // we use aggregation since we don't allow inheritance
            x=20, y=30, w=100, h=100, id=2003
        }
        graffiti {
            x=120, y=120
        }
        field UIField myField {
            x=10, y=20, w=100, h=3, id=8002, fontID=1,
            // field only properties
            maxchars=20, editable=true, underline=true
            singleline=false, dynamicsize=false, leftanchor=true, autoshift=true,
            hasscroll=false, numeric=false
        }
        list UIList liFruit {
            x=10, y=30, w=40 // height seems to be ignored
            numvisible=4, popupid=1020
            id=1080
        }
        slider UISlider slide {
            x=10, y=30, w=60, h=12
            min=0, max=100, page=20
            id=1081
        }
        scroll UIScroll myscroll {
            x=10, y=30, w=60, h=12
            min=0, max=100, page=20
            id=1082
        }
        menubar UIMenuBar menuMain {
            id=3000
            menu UIMenu menuMainFile {
                text="File"
                menuitem UIMenuItem mainFileAbout {
                    id=3001, text="About", shortcut="A"
                }
                menuitem UIMenuItem mainFileSave {
                    id=3002, text="Save", shortcut="S"
                }
            }
        }
    }
}
*/

ResType Compiler::getResType() {
    if (tok.id != tiIdent)
        c_error("identifier expected");

    int i;
    for (i=0;i<rtLastResType;i++) {
        if (strcmp(tok.value, resNames[i])==0) {
            break;
        }
    }

    return (ResType)i;
}

PropType Compiler::getPropType() {
    if (tok.id != tiIdent)
        c_error("identifier expected");

    int i;
    for (i=0;i<ptLastPropType;i++) {
        if (strcmp(tok.value, propNames[i])==0) {
            break;
        }
    }

    return (PropType)i;
}

int Compiler::addResSymbol(ResType rt, int sid, string name, int id) {
    if (bProjectRead)
        return 0;

    // is this a nameless res symbol?
    if (name != "*") {
        int rid = findRes(name);
        if (rid != -1)
            c_error("all resources must have unique names");
    }

    ResSymbol rs;
    rs.name = name;
    rs.structID = sid;
    rs.type = rt;
    rs.loc = globTypes.size();

    // global init/decl
    GlobalInit gi;
    gi.val.type = vtInt;
    gi.val.iVal = id;
    gi.offset = rs.loc;
    globInits.push_back(gi);

    if (rt == rtApp) {
        gi.val.type = vtString;
        if (app.version.empty())
            gi.val.iVal = addString("");
        else
            gi.val.iVal = addString(app.version.c_str());
        gi.offset = rs.loc + 1;
        globInits.push_back(gi);

        gi.val.iVal = addString(app.creatorID.c_str());
        gi.offset = rs.loc + 2;
        globInits.push_back(gi);
    }

    Type type;
    type.vtype = vtStruct;
    type.structID = sid;
    doStructInit(false, false, type);

    // if init, add init call
    if (structs[sid].hasInit) {
        ExprNode* addr = NULL;
        addr = new ExprNode(opCWord);
        addr->type = type;
        addr->val.type = vtInt;
        addr->val.iVal = rs.loc;
        ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_init"), 0, addr);
        globInitTree.AddExprStmt(expr);
    }
    // if destroy, add destroy call
    if (structs[sid].hasDestroy) {
        ExprNode* addr = NULL;
        addr = new ExprNode(opCWord);
        addr->type = type;
        addr->val.type = vtInt;
        addr->val.iVal = rs.loc;
        ExprNode* expr = buildSpecial(type.structID, findStructFunc(type.structID, "_destroy"), 0, addr);
        globDestTree.AddExprStmt(expr);
    }

    assert(structs[sid].size > 0);

    globOffset += structs[sid].size;
    resSyms.push_back(rs);

    return rs.loc;
}

bool Compiler::doRes() {
    ResType type = getResType();
    nextToken();

    if (type == rtApp) {
        return doResApp();
    } else if (type == rtProject) {
        match(tiLBrace);
        while (tok.id == tiConstString) {
            nextToken();
        }
        match(tiRBrace);
        return true;
    } else if (type == rtBitmap) {
        return doResBitmap(true);
    } else if (type == rtString) {
        return doResString();
    } else if (type == rtLanguage) {
        check(tiConstString, "language translation file expected");
        nextToken();
        return true;
    } else if (type == rtForm) {
        return doResForm();
    } else if (type == rtResource) {
        return doResResource();
    } else {
        c_error("expected resource type");
    }
    
    return false;
}

void Compiler::getResNameType(ResType rt, string& name, int& sid) {
    string type;
    getResNameType(rt, name, type, sid);
}

void Compiler::getResNameType(ResType rt, string& name, string& type, int& sid) {
    string ident1, ident2;

    if (tok.id == tiIdent) {
        ident1 = tok.value;
        nextToken();
    }

    if (tok.id == tiIdent) {
        ident2 = tok.value;
        nextToken();
    }

    if (bProjectRead) {
        sid = 0;
    } else {
        if (ident2.empty()) {
            sid = findStruct(resTypes[rt]);
            assert(sid != -1);
        } else {
            sid = findStruct(ident1);
            if (sid == -1)
                c_error("undeclared resource type");
        }
    }

    if (ident1.empty()) {
        name = "*";
    } else {
        name = ident2.empty() ? ident1 : ident2;
        type = ident2.empty() ? resTypes[rt] : ident1;
    }
}

bool Compiler::doResApp() {
    ResType rt;
    PropType pt;

    if (app.parsed)
        c_error("only one app allowed");
    app.parsed = true;

    int sid = 0;
    getResNameType(rtApp, app.name, sid);

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        rt = getResType();
        if (rt != rtLastResType) {
            nextToken();
            switch (rt) {
                case rtIcon:
                    doResIcon();
                    break;
                case rtBitmap:
                    doResBitmap(false);
                    break;
                case rtForm:
                    doResForm();
                    break;
                case rtString:
                    doResString();
                    break;
                default:
                    c_error("resource type not allowed here");
            }

        } else if ((pt = getPropType()) != ptLastPropType) {
            nextToken();
            match(tiAssign, "property must be followed by '='");
            switch (pt) {
                case ptName:
                    check(tiConstString, "name must be a string");
                    app.dname = translateString(tok.value, "app display name");
                    nextToken();
                    break;
                case ptDBName:
                    check(tiConstString, "dbname must be a string");
                    app.dbname = translateString(tok.value, "app database name");
                    nextToken();
                    break;
                case ptVersion:
                    check(tiConstString, "version must be a string");
                    app.version = translateString(tok.value, "app version");
                    nextToken();
                    break;
                case ptCreator:
                    check(tiConstString, "cid must be a string");
                    app.creatorID = tok.value;
                    nextToken();
                    break;
                case ptFile:
                    check(tiConstString, "file must be a string");
                    app.file = tok.value;
                    nextToken();
                    break;
                case ptLocked:
                    check(tiConstInt, "locked must be true/false");
                    app.locked = tok.intVal;
                    nextToken();
                    break;
                case ptHidden:
                    check(tiConstInt, "hidden must be true/false");
                    app.hidden = tok.intVal;
                    nextToken();
                    break;
                case ptResOnly:
                    check(tiConstInt, "resonly must be true/false");
                    app.resOnly = tok.intVal != 0;
                    nextToken();
                    break;
                default:
                    c_error("property type not allowed here");
            }

        } else {
            c_error("expected property or resource type");
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }

    match(tiRBrace);
    if (app.creatorID.empty())
        c_error("creator property must be specified");
    if (app.dname.empty())
        c_error("name property must be specified");
    if (app.dbname.empty())
        c_error("dbname property must be specified");

    app.addr = addResSymbol(rtApp, sid, app.name, 0);
    return true;
}

bool Compiler::doResForm() {
    PalmForm form;
    ResType rt;
    PropType pt;

#ifdef DEMO
    if (app.forms.size() > 1 && !bProjectRead) {
        c_error("OrbForms Designer DEMO supports a maximum of 2 forms");
        return false;
    }
#endif

    int sid = 0;
    getResNameType(rtForm, form.name, form.typeName, sid);

    match(tiLBrace);

    while (tok.id != tiRBrace) {
        rt = getResType();
        if (rt != rtLastResType) {
            nextToken();
            switch (rt) {
                case rtButton:
                case rtPushButton:
                case rtRepeatButton:
                case rtCheckbox:
                case rtLabel:
                case rtField:
                case rtList:
                case rtSelector:
                case rtBitmap:
                case rtGraffiti:
                case rtScroll:
                case rtSlider:
                case rtGadget:
                case rtPopup:
                    doResControl(rt, form);
                    break;
                case rtMenuBar:
                    doResMenuBar(form);
                    break;
                default:
                    c_error("resource type not allowed here");
            }

        } else if ((pt=getPropType()) != ptLastPropType) {
            nextToken();
            match(tiAssign, "property must be followed by '='");
            switch (pt) {
                case ptID:
                    check(tiConstInt, "id must be an int");
                    if (tok.intVal < 0 || tok.intVal > 8999)
                        c_error("id must be > 0 and < 9000");
                    form.id = tok.intVal;
                    nextToken();
                    break;
                case ptText:
                    check(tiConstString, "text must be a string");
                    form.text = translateString(tok.value, form.name.c_str(), "form title");
                    nextToken();
                    break;
                case ptX:
                    checkInt("x must be an int");
                    if (tok.intVal < 0 || tok.intVal > 159)
                        c_error("x must be >= 0 and <= 159");
                    form.x = tok.intVal;
                    nextToken();
                    break;
                case ptY:
                    checkInt("y must be an int");
                    if (tok.intVal < 0 || tok.intVal > 159)
                        c_error("y must be >= 0 and <= 159");
                    form.y = tok.intVal;
                    nextToken();
                    break;
                case ptW:
                    checkInt("w must be an int");
                    if (tok.intVal <= 0 || tok.intVal > 160)
                        c_error("w must be > 0 and <= 160");
                    form.w = tok.intVal;
                    nextToken();
                    break;
                case ptH:
                    checkInt("h must be an int");
                    if (tok.intVal <= 0 || tok.intVal > 160)
                        c_error("h must be > 0 and <= 160");
                    form.h = tok.intVal;
                    nextToken();
                    break;
                case ptMenuID:
                    check(tiConstInt, "menuid must be an int");
                    form.menuID = tok.intVal;
                    nextToken();
                    break;
                case ptHelpID:
                    check(tiConstInt, "helpid must be an int");
                    form.helpID = tok.intVal;
                    nextToken();
                    break;
                case ptDefButtonID:
                    check(tiConstInt, "defbuttonid must be an int");
                    form.defbuttonID = tok.intVal;
                    nextToken();
                    break;
                case ptModal:
                    check(tiConstInt, "modal must be true/false");
                    form.modal = tok.intVal;
                    nextToken();
                    break;
                case ptResizable:
                    check(tiConstInt, "modal must be true/false");
                    form.resizable = tok.intVal;
                    nextToken();
                    break;
                case ptHResize:
                    check(tiConstInt, "hresize must be 0,1,2");
                    form.hresize = tok.intVal % 3;
                    nextToken();
                    break;
                case ptVResize:
                    check(tiConstInt, "vresize must be 0,1,2");
                    form.vresize = tok.intVal % 3;
                    nextToken();
                    break;
                default:
                    c_error("unexpected property");
                    break;
            }

        } else {
            c_error("resource type or property expected");
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();

    }

    match(tiRBrace);
    form.addr = addResSymbol(rtForm, sid, form.name, form.id); 
    app.forms.push_back(form);
    return true;
}

bool Compiler::doResControl(ResType rt, PalmForm& form) {
    PropType pt;
    PalmControl ctrl;
    int sid = -1;

    if (rt != rtGraffiti) {
        getResNameType(rt, ctrl.name, ctrl.typeName, sid);

        if (bProjectRead && ctrl.typeName == "UIGadget") {
            // an unspecified gadget will be saved as a UIGadget, but
            // we don't want this shown in the IDE
            ctrl.typeName = "";
        }

        if (!bProjectRead && rt == rtGadget) {
            // ensure that the gadget is a legal type
            if (ctrl.typeName != "UIGadget") {
                if (structs[sid].isObject) {
                    string err = "gadget type '";
                    err += ctrl.typeName + "' must be declared as a struct, not an object";
                    c_error(err.c_str());
                }
                if (structs[sid].vars.size() < 1 ||
                    structs[sid].vars[0].type.vtype != vtStruct ||
                    structs[sid].vars[0].type.structID != findStruct("UIGadget"))
                {
                    string err = "gadget type '";
                    err += ctrl.typeName + "' must have UIGadget as its first member";
                    c_error(err.c_str());
                }
            }
        }
    }
    ctrl.type = rt;
    match(tiLBrace);

    while (tok.id != tiRBrace) {
        pt=getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");

        nextToken();
        match(tiAssign, "property must be followed by '='");
        switch (pt) {
            // standard control properties
            case ptID:
                check(tiConstInt, "id must be an int");
                if (tok.intVal < 0 || tok.intVal > 8999)
                    c_error("id must be > 0 and < 9000");
                ctrl.id = tok.intVal;
                nextToken();
                break;
            case ptText:
                check(tiConstString, "text must be a string");
                ctrl.text = translateString(tok.value, ctrl.name.c_str(), "control text");
                nextToken();
                break;
            case ptX:
                checkInt("x must be an int");
                ctrl.x = tok.intVal;
                nextToken();
                break;
            case ptY:
                checkInt("y must be an int");
                ctrl.y = tok.intVal;
                nextToken();
                break;
            case ptW:
                checkInt("w must be an int");
                ctrl.w = tok.intVal;
                nextToken();
                break;
            case ptH:
                checkInt("h must be an int");
                ctrl.h = tok.intVal;
                nextToken();
                break;
            case ptFontID:
                check(tiConstInt, "fontid must be an int");
                ctrl.fontID = tok.intVal;
                nextToken();
                break;
            case ptLeftAnchor:
                check(tiConstInt, "leftanchor must be true/false");
                ctrl.leftAnchor = !!tok.intVal;
                nextToken();
                break;
            case ptGroup:
                check(tiConstInt, "group must be an int");
                ctrl.group= tok.intVal;
                nextToken();
                break;
            case ptFrame:
                check(tiConstInt, "frame must be 0,1,2,3");
                ctrl.frame = tok.intVal % 4;
                nextToken();
                break;
            case ptHResize:
                check(tiConstInt, "hresize must be 0,1,2");
                ctrl.hresize = tok.intVal % 3;
                nextToken();
                break;
            case ptVResize:
                check(tiConstInt, "vresize must be 0,1,2");
                ctrl.vresize = tok.intVal % 3;
                nextToken();
                break;

            // graphic controls
            case ptGraphic:
                check(tiConstInt, "graphic must be true/false");
                ctrl.graph = !!tok.intVal;
                nextToken();
                break;
            case ptBitmapID:
                check(tiConstInt, "bitmapid must be an int");
                ctrl.bitmapID = tok.intVal;
                nextToken();
                break;
            case ptSBitmapID:
                check(tiConstInt, "sbitmapid must be an int");
                ctrl.sbitmapID = tok.intVal;
                nextToken();
                break;

            // field stuff
            case ptMaxChars:
                check(tiConstInt, "maxchars must be an int");
                ctrl.maxChars = tok.intVal;
                nextToken();
                break;
            case ptEditable:
                check(tiConstInt, "editable must be an int");
                ctrl.editable = !!tok.intVal;
                nextToken();
                break;
            case ptUnderline:
                check(tiConstInt, "underline must be an int");
                ctrl.underline = tok.intVal;
                nextToken();
                break;
            case ptSingleLine:
                check(tiConstInt, "singeline must be an int");
                ctrl.singleLine = !!tok.intVal;
                nextToken();
                break;
            case ptDynamicSize:
                check(tiConstInt, "dynamicsize must be an int");
                ctrl.dynSize = !!tok.intVal;
                nextToken();
                break;
            case ptAutoShift:
                check(tiConstInt, "autoshift must be an int");
                ctrl.autoShift = !!tok.intVal;
                nextToken();
                break;
            case ptHasScroll:
                check(tiConstInt, "hasscroll must be an int");
                ctrl.hasScroll = !!tok.intVal;
                nextToken();
                break;
            case ptNumeric:
                check(tiConstInt, "numeric must be an int");
                ctrl.numeric = !!tok.intVal;
                nextToken();
                break;

            // scroll/slider
            case ptMin:
                checkInt("min must be an int");
                ctrl.iMin = tok.intVal;
                nextToken();
                break;
            case ptMax:
                checkInt("max must be an int");
                ctrl.iMax = tok.intVal;
                nextToken();
                break;
            case ptPage:
                checkInt("page must be an int");
                ctrl.page = tok.intVal;
                nextToken();
                break;

            // list
            case ptNumVisible:
                check(tiConstInt, "numvisible must be an int");
                ctrl.nVisible = tok.intVal;
                nextToken();
                break;
            case ptTriggerID:
                check(tiConstInt, "triggerid must be an int");
                ctrl.triggerID = tok.intVal;
                nextToken();
                break;

            // popup
            case ptListID:
                check(tiConstInt, "listid must be an int");
                ctrl.listID = tok.intVal;
                nextToken();
                break;
            default:
                c_error("unexpected property");
                break;
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();

    }

    match(tiRBrace);
    if (rt == rtGraffiti) {
        // nothing
    } else if (rt == rtBitmap) {
        ctrl.addr = addResSymbol(rt, sid, ctrl.name, ctrl.bitmapID);
    } else {
        ctrl.addr = addResSymbol(rt, sid, ctrl.name, ctrl.id);
    }
    form.controls.push_back(ctrl);
    return true;
}

bool Compiler::doResMenuBar(PalmForm& form) {
    PropType pt;
    PalmMenuBar bar;

    int sid = 0;
    getResNameType(rtMenuBar, bar.name, sid);

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");
        nextToken();
        if (pt != ptMenu)
            match(tiAssign, "property must be followed by '='");
        switch (pt) {
            case ptID:
                check(tiConstInt, "id must be an int");
                if (tok.intVal < 0 || tok.intVal > 8999)
                    c_error("id must be > 0 and < 9000");
                bar.id = tok.intVal;
                nextToken();
                break;
            case ptMenu:
                doResMenu(bar);
                break;
            default:
                c_error("unexpected property");
                break;
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    addResSymbol(rtMenuBar, sid, bar.name, bar.id);
    form.menubars.push_back(bar);
    return true;
}

bool Compiler::doResMenu(PalmMenuBar& bar) {
    PropType pt;
    PalmMenu menu;

    int sid = 0;
    getResNameType(rtMenu, menu.name, sid);

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");
        nextToken();
        if (pt != ptMenuItem)
            match(tiAssign, "property must be followed by '='");
        switch (pt) {
            case ptText:
                check(tiConstString, "text must be a string");
                menu.text = translateString(tok.value, menu.name.c_str(), "menu name");
                nextToken();
                break;
            case ptMenuItem:
                doResMenuItem(menu);
                break;
            default:
                c_error("unexpected property");
                break;
        }				

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    addResSymbol(rtMenu, sid, menu.name, 0);
    bar.menus.push_back(menu);
    return true;
}

bool Compiler::doResMenuItem(PalmMenu& menu) {
    PropType pt;
    PalmMenuItem item;

    int sid = 0;
    getResNameType(rtMenuItem, item.name, sid);

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");
        nextToken();
        match(tiAssign, "property must be followed by '='");
        switch (pt) {
            case ptID:
                check(tiConstInt, "id must be an int");
                if (tok.intVal < 0 || tok.intVal > 8999)
                    c_error("id must be > 0 and < 9000");
                item.id = tok.intVal;
                nextToken();
                break;
            case ptSysID:
                check(tiConstInt, "sysid must be an int");
                item.sysid = tok.intVal;
                nextToken();
                break;
            case ptText:
                check(tiConstString, "text must be a string");
                item.text = translateString(tok.value, item.name.c_str(), "menu item name");
                nextToken();
                break;
            case ptShortcut:
                check(tiConstString, "shortcut must be a string");
                item.shortcut = tok.value[0];
                nextToken();
                break;
            default:
                c_error("unexpected property");
                break;
        }				

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    item.addr = addResSymbol(rtMenuItem, sid, item.name, item.id);
    menu.items.push_back(item);
    return true;
}

bool Compiler::doResBitmap(bool external) {
    PropType pt;
    PalmBitmap bmp;

    int sid = 0;
    getResNameType(rtBitmap, bmp.name, sid);
    // if the type was unspecified, replace it with Bitmap (since rtBitmap is used for both Bitmap and
    // UIFBitmap
    if (!bProjectRead && structs[sid].name == resTypes[rtBitmap])
        sid = findStruct("Bitmap");

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");
        nextToken();
        match(tiAssign, "property must be followed by '='");
        switch (pt) {
            case ptW:
                checkInt("w must be an int");
                bmp.w = tok.intVal;
                nextToken();
                break;
            case ptH:
                checkInt("h must be an int");
                bmp.h = tok.intVal;
                nextToken();
                break;
            case ptImage1:
            case ptImage2:
            case ptImage4:
            case ptImage8:
            case ptImage16:
            case ptImageH1:
            case ptImageH2:
            case ptImageH4:
            case ptImageH8:
            case ptImageH16:
                check(tiConstString, "filename must be a string");
                bmp.file[pt-ptImage1] = tok.value;// lex.FindFile(tok.value);
                nextToken();
                break;
            case ptImageRes:
                check(tiConstString, "image resource must be a string");
                bmp.res = tok.value;
                nextToken();
                break;
            case ptID:
                check(tiConstInt, "id must be an int");
                if (tok.intVal < 0 || tok.intVal > 8999)
                    c_error("id must be > 0 and < 9000");
                bmp.id = tok.intVal;
                nextToken();
                break;
            default:
                c_error("unexpected property");
                break;
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    addResSymbol(rtBitmap, sid, bmp.name, bmp.id);
    if (external) {
        app.externalBitmaps.push_back(bmp);
    } else {
        app.bitmaps.push_back(bmp);
    }
    return true;
}

bool Compiler::doResIcon() {
    PropType pt;

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        nextToken();
        match(tiAssign, "property must be followed by '='");
        check(tiConstString, "icon file name expected");
        if (pt >= ptSmall1 && pt <= ptSmallH16) {
            app.icon.sfile[pt-ptSmall1] = tok.value; //lex.FindFile(tok.value);
        } else if (pt >= ptLarge1 && pt <= ptLargeH16) {
            app.icon.file[pt-ptLarge1] = tok.value; //lex.FindFile(tok.value);
        } else if (pt == ptLargeRes) {
            app.icon.res = tok.value;
        } else if (pt == ptSmallRes) {
            app.icon.sres = tok.value;
        } 
        else
            c_error("expected icon format");
        nextToken();
        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    return true;
}

bool Compiler::doResString() {
    PropType pt;
    string text, name;
    int id = -1, sid = -1;

    getResNameType(rtString, name, sid);

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        nextToken();
        match(tiAssign, "property must be followed by '='");
        if (pt == ptID) {
            check(tiConstInt, "id must be an int");
            id = tok.intVal;
            if (tok.intVal < 0 || tok.intVal > 8999)
                c_error("id must be > 0 and < 9000");
            if (name.empty()) {
                name = "str";
                char buff[16];
                name += _itoa(id, buff, 10);
            }
            nextToken();
        } else if (pt == ptText) {
            check(tiConstString, "text must be a string");
            text = translateString(tok.value, name.c_str(), "resource string");
            nextToken();
        } else
            c_error("id/text expected");
        
        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);

    PalmString pstr;
    pstr.id = id;
    pstr.str = text;
    pstr.name = name;
    app.strings.push_back(pstr);
    addResSymbol(rtString, sid, pstr.name, pstr.id);
    return true;
}

bool Compiler::doResResource() {
    PropType pt;
    PalmResource res;

    match(tiLBrace);
    while (tok.id != tiRBrace) {
        pt = getPropType();
        if (pt == ptLastPropType)
            c_error("expected property type");
        nextToken();
        match(tiAssign, "property must be followed by '='");
        switch (pt) {
            case ptRes:
                check(tiConstString, "res must be a string");
                res.res = tok.value;
                nextToken();
                break;
            case ptFile:
                check(tiConstString, "file must be a string");
                res.file = tok.value;
                nextToken();
                break;
            case ptType:
                check(tiConstString, "type must be a string");
                if (strlen(tok.value) != 4)
                    c_error("type must be 4 characters");
                res.type = tok.value;
                nextToken();
                break;
            case ptID:
                check(tiConstInt, "id must be an int");
                res.id = tok.intVal;
                nextToken();
                break;
            default:
                c_error("unexpected property");
                break;
        }

        if (tok.id == tiComma || tok.id == tiSemiColon)
            nextToken();
    }
    match(tiRBrace);
    if (res.id == -1) {
        if (res.res.empty()) {
            c_error("id must be specified when not importing an entire PRC");
        }
    } else {
        if (res.type.empty()) {
            c_error("type must be specified when adding a single resource");
        }
    }
    app.resources.push_back(res);
    return true;
}
