#ifdef WIN32
#include "compiler.h"
//#include "shlwapi.h"

char hval(char c) {
    if (isdigit(c)) return c-'0';
    if (c >='A' && c <= 'F') c+=32;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

string EscapeString(string str) {
    string ret;
    for (int i=0;i<str.length();i++) {
        if (str[i] == '"')
            ret += "\\\"";
        else if (str[i] == '\t')
            ret += "\\t";
        else if (str[i] == '\n')
            ret += "\\n";
        else if (str[i] == '\r')
            ret += "\\r";
        else if (str[i] == '\\')
            ret += "\\\\";
        else
            ret += str[i];
    }
    return ret;
}

string UnescapeString(string str) {
    string ret;
    for (int i=0;i<str.length();i++) {
        if (str[i] == '\\') {
            i++;
            if (str[i] == 't') ret += '\t';
            else if (str[i] == 'n') ret += '\n';
            else if (str[i] == 'r') ret += '\r';
            else if (str[i] == '\\') ret += '\\';
            else if (str[i] == '"') ret += '"';
            else if (str[i] == '\'') ret += '\'';
            else if (str[i] == '0') ret += '\0'; // This may not be useful
            else if (str[i] == 'x' || str[i] =='X') {
                i++;
                int intversion = 0;
                intversion |= hval(str[i++]);
                intversion *= 16;
                intversion |= hval(str[i]);
                ret += (char)intversion;
            }
            continue;
        }
        ret += str[i];
    }
    return ret;
}

/*
static string RelativePath(string file, string projname) {
    char buff[MAX_PATH];
    if (PathRelativePathTo(buff, projname.c_str(), FILE_ATTRIBUTE_NORMAL,
        file.c_str(), FILE_ATTRIBUTE_NORMAL))
    {
        return buff;
    }
    else
    {
        return file;
    }
}
*/

class projectfile
{
public:
    projectfile() :m_fhandle(0) {
    }

    ~projectfile() {
        if (m_fhandle)	{
            fclose(m_fhandle);
            m_fhandle=0;
        }
    }

    void attach(FILE* fhandle) { 
        assert(fhandle);
        m_fhandle = fhandle; 
    }
    
    FILE* detach() { 
        FILE* t = m_fhandle; 
        assert(t);
        m_fhandle=0; 
        return t; 
    }

    bool open(string file, const char* opt) {
        m_fhandle=fopen(file.c_str(),opt);
        if (m_fhandle) return true;
        return false;
    }

    FILE* handle() { 
        return m_fhandle; 
    }
    
    class exception_write {
    };

    exception_write bad_write;

    void write(const char * fmt, ...) {
        
        enum {TBUF_SIZE = 3000};
        char tbuf[TBUF_SIZE];

        try {
            va_list argptr;
            va_start(argptr, fmt);
            _vsnprintf(tbuf, TBUF_SIZE, fmt, argptr);
            va_end(argptr);
        }
        catch (...)
        {
            assert(0); // should never get here 
            throw bad_write;
        }
        assert(m_fhandle);
        if (0==m_fhandle)
            throw bad_write;
        int n = fputs(tbuf, m_fhandle);
        if (n<0)	   
           throw bad_write;
    }

protected:
    FILE* m_fhandle;
};

class projectfile_autoattach {
protected:
    projectfile& m_projectfile;
public:
    projectfile_autoattach(projectfile &orb, FILE * f) : m_projectfile(orb) { 
        assert(f);
        m_projectfile.attach(f);
    }
    ~projectfile_autoattach() {
        m_projectfile.detach();
    }
};

static void WriteStrProperty(FILE* f, int indent, string name, string val) {
    string esc = EscapeString(val);
    char spaces[] = "              ";
    spaces[indent] = '\0';
    
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    orbfile.write( "%s%s = \"%s\"\n", spaces, name.c_str(), esc.c_str());
}

static void WriteBigStr(FILE* f, string str) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    string line;
    
    while (str.size()) {
        int len = min(str.size(), 64);
        line = str.substr(0, len);
        orbfile.write( "  \"%s\"\n", EscapeString(line).c_str());
        if (len < str.size()) {
            str = str.substr(len, str.length() - len);
        } else {
            str = "";
        }
    }
}

template <typename T>
int GetMaxId(int maxId, vector<T>& v) {
    for (int i=0; i<v.size(); i++) {
        if (v[i].id > maxId)
            maxId = v[i].id;
    }
    return maxId;
}

int FixId(int id, PalmProject* proj = NULL) {
    static int maxId;
    if (proj != NULL) {
        // find the maxId
        maxId = GetMaxId(0, proj->app.bitmaps);
        maxId = GetMaxId(maxId, proj->app.externalBitmaps);
        maxId = GetMaxId(maxId, proj->app.strings);
        maxId = GetMaxId(maxId, proj->app.forms);
        for (int i=0; i<proj->app.forms.size(); i++) {
            maxId = GetMaxId(maxId, proj->app.forms[i].controls);
        }
        return 0;
    } else {
        if (id == -1)
            return ++maxId;
        else
            return id;
    }
}

bool Compiler::WriteProject(char* file, PalmProject& proj) {
#ifdef DEMO
    if (proj.app.forms.size() > 2) {
        return false;
    }
#endif

    // ensure that no ids are -1
    FixId(0, &proj);

    projectfile orbfile;
    string fpath = file;
    if (false==orbfile.open(fpath,"wt"))
        return false;

    try {
        int i;
        orbfile.write("// This file is automatically generated by the OrbForms compiler\n"
                   "// DO NOT EDIT\n\n");

        orbfile.write("// language translation files\n");
        for (i=0;i<proj.translationFiles.size();i++) {
            orbfile.write("@language \"%s\"\n", EscapeString(proj.translationFiles[i]).c_str());
        }
        orbfile.write("\n");
        
        orbfile.write("// gadget files\n");
        for (i=0;i<proj.gadgets.size();i++) {
            orbfile.write( "#include \"%s\"\n", EscapeString(proj.gadgets[i]).c_str());
        }

        orbfile.write("\n// IDE data\n");
        orbfile.write( "@project {\n");
        WriteBigStr(orbfile.handle(), proj.data);
        orbfile.write( "}\n");

        orbfile.write( "\n// app definition\n");
        WriteApp(orbfile.handle(), proj.app);
        orbfile.write( "\n// source files\n");

        for (i=0;i<proj.files.size();i++) {
            orbfile.write( "#include \"%s\"\n", EscapeString(proj.files[i]).c_str());
        }
    }
    catch (projectfile::exception_write &e)	{
        (void)e; // avoid compiler warning 
        return false;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

bool Compiler::WriteApp(FILE* f, PalmApp& app) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);


    static char* idxToBits[10] = { "1", "2", "4", "8", "16", "h1", "h2", "h4", "h8", "h16"};
    // properties

    orbfile.write( "@app UIApp %s {\n", app.name.c_str());
    WriteStrProperty(orbfile.handle(), 2, "name", app.dname);
    WriteStrProperty(orbfile.handle(), 2, "dbname", app.dbname);
    WriteStrProperty(orbfile.handle(), 2, "file", app.file);
    WriteStrProperty(orbfile.handle(), 2, "creator", app.creatorID);
    WriteStrProperty(orbfile.handle(), 2, "version", app.version);
    orbfile.write( "  locked = %s, hidden = %s\n", app.locked ? "true" : "false", app.hidden ? "true" : "false");

    // icon
    orbfile.write( "  icon {\n");
    int i,j;
    for (i=0;i<10;i++) {
        if (!app.icon.file[i].empty())
            orbfile.write( "    large%s = \"%s\"\n", idxToBits[i], EscapeString(app.icon.file[i]).c_str());
        if (!app.icon.sfile[i].empty())
            orbfile.write( "    small%s = \"%s\"\n", idxToBits[i], EscapeString(app.icon.sfile[i]).c_str());
    }
    if (!app.icon.res.empty())
        orbfile.write( "    largeres = \"%s\"\n", EscapeString(app.icon.res).c_str());
    if (!app.icon.sres.empty())
        orbfile.write( "    smallres = \"%s\"\n", EscapeString(app.icon.sres).c_str());
    orbfile.write( "  }\n");

    // bitmaps
    for (i=0;i<app.bitmaps.size();i++) {
        orbfile.write( "  bitmap Bitmap %s {\n", app.bitmaps[i].name.c_str());
        orbfile.write( "    id = %d\n", FixId(app.bitmaps[i].id));
        for (j=0;j<10;j++) {
            if (!app.bitmaps[i].file[j].empty())
                orbfile.write( "    image%s = \"%s\"\n", idxToBits[j], EscapeString(app.bitmaps[i].file[j]).c_str());
        }
        if (!app.bitmaps[i].res.empty())
            orbfile.write( "    imageres = \"%s\"\n", EscapeString(app.bitmaps[i].res).c_str());
        orbfile.write( "  }\n");
    }

    // strings
    for (i=0;i<app.strings.size();i++) {
        if (app.strings[i].name.empty()) {
            app.strings[i].name = "string";
            char buff[16];
            app.strings[i].name += _itoa(app.strings[i].id, buff, 10);
        }

        orbfile.write( "  resstring UIString %s {\n    id = %d, text = \"%s\"\n  }\n", app.strings[i].name.c_str(), FixId(app.strings[i].id), EscapeString(app.strings[i].str).c_str());
    }

    // forms
    for (i=0;i<app.forms.size();i++)
        WriteForm(orbfile.handle(), app.forms[i]);

    orbfile.write( "}\n");

    return true;
}

bool Compiler::WriteForm(FILE* f, PalmForm& form) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    int i;
    orbfile.write( "  form UIForm %s {\n", form.name.c_str());
    // write properties
    orbfile.write( "    id = %d\n", FixId(form.id));
    WriteStrProperty(orbfile.handle(), 4, "text", form.text);
    orbfile.write( "    x = %d, y = %d, w = %d, h = %d\n", form.x, form.y, form.w, form.h);
    orbfile.write( "    menuid = %d, helpid = %d, defbuttonid = %d\n", form.menuID, form.helpID, form.defbuttonID);
    orbfile.write( "    modal = %s, resizable = %s\n", form.modal ? "true" : "false", form.resizable ? "true" : "false");
    orbfile.write( "    hresize = %d, vresize = %d\n", form.hresize, form.vresize);

    // write menubars
    for (i=0;i<form.menubars.size();i++)
        WriteMenuBar(orbfile.handle(), form.menubars[i]);

    // write controls
    for (i=0;i<form.controls.size();i++)
        WriteControl(orbfile.handle(), form.controls[i]);

 	orbfile.write( "  }\n");
    return true;
}

bool Compiler::WriteControl(FILE* f, PalmControl& ctrl) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    if (ctrl.type == rtGraffiti) {
        orbfile.write( "    graffiti { x = %d, y = %d, hresize = %d, vresize = %d }\n", ctrl.x, ctrl.y, ctrl.hresize, ctrl.vresize);
        return true;
    }
    if (ctrl.type == rtGadget && ctrl.typeName.empty()) {
        ctrl.typeName = "UIGadget";
    }
    // write properties
    orbfile.write( "    %s %s %s {\n", resNames[ctrl.type], ctrl.typeName.c_str(), ctrl.name.c_str());
    orbfile.write( "      id = %d\n", FixId(ctrl.id));
    if (ctrl.type == rtLabel || ctrl.type == rtBitmap)
        orbfile.write( "      x = %d, y = %d\n", ctrl.x, ctrl.y);
    else
        orbfile.write( "      x = %d, y = %d, w = %d, h = %d\n", ctrl.x, ctrl.y, ctrl.w, ctrl.h);

    switch (ctrl.type) {
        case rtButton:
        case rtPushButton:
        case rtRepeatButton:
        case rtCheckbox:
        case rtSelector:
        case rtPopup:
            WriteStrProperty(orbfile.handle(), 6, "text", ctrl.text);
            orbfile.write( "      leftanchor = %s, group = %d, fontid = %d\n", ctrl.leftAnchor ? "true" : "false", ctrl.group, ctrl.fontID);
            if (ctrl.graph && (ctrl.type >= rtButton && ctrl.type <= rtRepeatButton))
                orbfile.write("      graphic = true, bitmapid = %d, sbitmapid = %d\n", ctrl.bitmapID, ctrl.sbitmapID);
            if (ctrl.type == rtButton || ctrl.type == rtRepeatButton)
                orbfile.write("      frame = %d\n", ctrl.frame);
            break;
        case rtList:
            orbfile.write( "      numvisible = %d, triggerid = %d, fontid = %d\n", ctrl.nVisible, ctrl.triggerID, ctrl.fontID);
            break;
        case rtSlider:
            if (ctrl.graph)
                orbfile.write( "      graphic = true, bitmapid = %d, sbitmapid = %d\n", ctrl.bitmapID, ctrl.sbitmapID);
            // fall-through
        case rtScroll:
            orbfile.write( "      min = %d, max = %d, page = %d\n", ctrl.iMin, ctrl.iMax, ctrl.page);
            break;
        case rtGadget:
            break;
        case rtLabel:
            orbfile.write( "      fontid = %d\n", ctrl.fontID);
            WriteStrProperty(orbfile.handle(), 6, "text", ctrl.text);
            break;
        case rtBitmap:
            orbfile.write( "      bitmapid = %d\n", ctrl.bitmapID);
            break;
        case rtField:
            orbfile.write( "      fontid = %d, leftanchor = %s, maxchars = %d\n", ctrl.fontID, ctrl.leftAnchor ? "true" : "false", ctrl.maxChars);
            orbfile.write( "      underline = %s, singleline = %s, dynamicsize = %s, hasscroll = %s\n",
                ctrl.underline ? "true" : "false", ctrl.singleLine ? "true" : "false",
                ctrl.dynSize ? "true" : "false", ctrl.hasScroll ? "true" : "false");
            orbfile.write( "      editable = %s, autoshift = %s, numeric = %s\n", ctrl.editable ? "true" : "false",
                ctrl.autoShift ? "true" : "false", ctrl.numeric ? "true" : "false");
            break;
        default:
            assert(!"what was that!");
    }
    orbfile.write( "      hresize = %d, vresize = %d\n", ctrl.hresize, ctrl.vresize);

    orbfile.write( "    }\n");
    return true;
}

bool Compiler::WriteMenuBar(FILE* f, PalmMenuBar& bar) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    orbfile.write( "    menubar UIMenuBar %s {\n      id = %d\n", bar.name.c_str(), bar.id);
    for (int i=0;i<bar.menus.size();i++) {
        WriteMenu(orbfile.handle(), bar.menus[i]);
    }
    orbfile.write( "    }\n");
    return true;
}

bool Compiler::WriteMenu(FILE* f, PalmMenu& menu) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    orbfile.write( "      menu UIMenu %s {\n", menu.name.c_str());
    WriteStrProperty(orbfile.handle(), 8, "text", menu.text);
    for (int i=0;i<menu.items.size();i++) {
        WriteMenuItem(orbfile.handle(), menu.items[i]);
    }
    orbfile.write( "      }\n");
    return true;
}

bool Compiler::WriteMenuItem(FILE* f, PalmMenuItem& item) {
    projectfile orbfile;
    projectfile_autoattach a(orbfile, f);

    orbfile.write( "        menuitem UIMenuItem %s {\n          id = %d, sysid = %d, text = \"%s\"",
        item.name.c_str(), item.id, item.sysid, EscapeString(item.text).c_str());
    if (item.shortcut)
        orbfile.write( ", shortcut = \"%c\"", item.shortcut);
    orbfile.write( "\n        }\n");
    return true;
}

// TODO: return line number
bool Compiler::ReadProject(char* file, PalmProject& proj, ErrorInfo& info) {

    enum State { stTop, stBody, stIdent };
    State state;
    int nBraces = 0;
    bool bGadget = true, ret = true, skip = false;

    errorInfo.desc = "Internal compiler error";
    errorInfo.file = "";
    errorInfo.line = 0;
    errorInfo.ch = 0;

    Source* source = GetFile(file, false);
    if (!source) {
        errorInfo.desc = "Unable to open file";
        return false;
    }

    try {
        // setup the lexer
        lex.push(file, source);
        state = stTop;
        bProjectRead = true; // turn on some stuff

        while (tok.id != tiEnd) {
            if (!skip) nextToken();
            else skip = false;
            switch (state) {
                case stTop:
                    if (tok.id == tiInclude) {
                        nextToken();
                        if (tok.id != tiConstString) {
                            state = stTop;
                            c_error("include requires a file name");
                            ret = false;
                            break;
                        }
                        if (bGadget)
                            proj.gadgets.push_back(string(tok.value));
                        else
                            proj.files.push_back(string(tok.value));

                    } else if (tok.id == tiAt) {
                        nextToken();
                        if (!strcmp(tok.value, "app")) {
                            nextToken();
                            doResApp();
                            skip = true;
                        } else if (!strcmp(tok.value, "project")) {
                            nextToken();
                            match(tiLBrace);
                            proj.data = "";
                            while (tok.id == tiConstString) {
                                proj.data += tok.value;
                                nextToken();
                            }
                            match(tiRBrace);
                            skip = true;
                        } else if (!strcmp(tok.value, "language")) {
                            nextToken();
                            check(tiConstString, "language translation file expected");
                            proj.translationFiles.push_back(tok.value);
                            break; // don't reset bGadget
                        }
                        bGadget = false;
                    } else {
                        state = stIdent;
                    }
                    break;
                case stIdent:
                    if (tok.id == tiSemiColon) {
                        state = stTop;
                    } else if (tok.id == tiLBrace) {
                        state = stBody;
                        nBraces = 1;
                    }
                    break;
                case stBody:
                    if (tok.id == tiLBrace)
                        nBraces++;
                    else if (tok.id == tiRBrace) {
                        nBraces--;
                        if (!nBraces) {
                            state = stTop;
                        }
                    }
                    break;
            }
        }
        lex.pop();

    } catch (...) {
        info = errorInfo;
        ret = false;
    }

    proj.app = app;
    return ret;
}
#endif
