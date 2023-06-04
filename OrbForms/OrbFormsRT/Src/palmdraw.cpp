#include "OrbFormsRT.h"
#include "OrbFormsRTRsc.h"

NativeDraw* pDefaultDraw;

// NativeDraw object
NativeDraw::NativeDraw() {
    x = y = w = h = 0;
    bOffscreen = false;
    hWnd = hPrevWnd = NULL;
    alignMode = 0;
    ulMode = 0;
    nBegin = 0;
    iGadget = -1;
    if (!pDefaultDraw)
        pDefaultDraw = this;
}

NativeDraw::~NativeDraw() {
    if (nBegin > 0) {
        nBegin = 1;
        end();
    }
    release();
    if (pDefaultDraw == this)
        pDefaultDraw = NULL;
}

void NativeDraw::attachGadget(word id) {
    int index = FindCtrl(id);
    assert(index != -1);
    iGadget = index;
    //x = ctrls[index].x;
    //y = ctrls[index].y;
    //w = ctrls[index].w;
    //h = ctrls[index].h;
}

void NativeDraw::attachForm(word id) {
    int index = FindForm(id);
    assert(index != -1);
    // since the form is the draw window, everything is relative to it, so x,y = 0
    x = 0;
    y = 0;
    w = forms[index].w;
    h = forms[index].h;
}

void NativeDraw::begin(bool _bNative, bool freeDraw) {
#ifdef DEBUG
    if (bInOpen) {
        vm->vm_error("Cannot draw in an onopen handler");
    }
#endif

    if (nBegin == 0) {
        if (iGadget != -1) {
            x = ctrls[iGadget].x;
            y = ctrls[iGadget].y;
            w = ctrls[iGadget].w;
            h = ctrls[iGadget].h;
        }
        if (romVersion >= ver35) {
            WinPushDrawState();
        }
        if (_bNative && bHighDensity) {
            WinSetCoordinateSystem(0);
            bNative = _bNative;
            RectangleType rect = {x, y, x+w, y+h};
            WinScaleRectangle(&rect);
            x = rect.topLeft.x;
            y = rect.topLeft.y;
            w = rect.extent.x - x;
            h = rect.extent.y - y;
        }
        if (bOffscreen) {
            // set draw window
            if (hWnd) {
                hPrevWnd = WinGetDrawWindow();
                WinSetDrawWindow(hWnd);
            }
        } else {
            hWnd = WinGetDrawWindow();
            if (freeDraw) {
                x = 0; y = 0; w = 160; h = 160;
            } else {
                RectangleType clip;
                WinGetClip(&old_clip);
                RctSetRectangle(&clip, x, y, w, h);
                WinClipRectangle(&clip);
                WinSetClip(&clip);
            }
        }
        if (romVersion < ver35) {
            WinSetColors(0, &old_rgb_fg, 0, &old_rgb_bg);
            rgb_text.r = rgb_text.g = rgb_text.b = 0;
        }
    }
    nBegin++;
}

void NativeDraw::end() {	
    if (nBegin > 0) {
        if (--nBegin == 0) {
            if (bOffscreen) {
                // reset draw window
                if (hPrevWnd) {
                    WinSetDrawWindow(hPrevWnd);
                    hPrevWnd = NULL;
                }
            } else {
                if (hWnd) {
                    WinSetClip(&old_clip);
                    hWnd = NULL;
                }
            }
            if (romVersion < ver35) {
                WinSetColors(&old_rgb_fg, 0, &old_rgb_bg, 0);
            } else {
                if (bHighDensity) {
                    RectangleType rect = {x,y,x+w,y+h};
                    WinUnscaleRectangle(&rect);
                    x = rect.topLeft.x;
                    y = rect.topLeft.y;
                    w = rect.extent.x - x;
                    h = rect.extent.y - y;
                }
                WinPopDrawState();
            }
        }
    }
}

void NativeDraw::release() {
    if (bOffscreen && hWnd) {
        WinDeleteWindow(hWnd, false);
        bOffscreen = false;
        hWnd = hPrevWnd = NULL;
    }
}

bool NativeDraw::createOffscreen(int _w, int _h) {
    release();
    bOffscreen = true;
    UInt16 err;
    //hWnd = WinCreateOffscreenWindow(_w, _h, screenFormat, &err);
    hWnd = WinCreateOffscreenWindow(_w, _h, nativeFormat, &err);
    if (hWnd) {
        w = _w;
        h = _h;
    }
    return hWnd != NULL;
}

bool NativeDraw::create(int _w, int _h) {
    if (createOffscreen(_w, _h)) {
        if (romVersion < ver35) {
            RectangleType rect;
            RctSetRectangle(&rect, 0, 0, _w, _h);
            hPrevWnd = WinSetDrawWindow(hWnd);
            WinEraseRectangle(&rect, 0);
            WinSetDrawWindow(hPrevWnd);
            hPrevWnd = NULL;
        }
        return true;
    }
    return false;
}

bool NativeDraw::copyForm(word id) {
    int index = FindForm(id);
    assert(index != -1);
    if (createOffscreen(forms[index].w, forms[index].h)) {
        RectangleType rect;
        RctSetRectangle(&rect, 0, 0, w, h);
        WinCopyRectangle(WinGetDrawWindow(), hWnd, &rect, 0, 0, winPaint);
        return true;
    }
    return false;
}

bool NativeDraw::copyGadget(word id) {
    int index = FindCtrl(id);
    assert(index != -1);
    if (createOffscreen(ctrls[index].w, ctrls[index].h)) {
        RectangleType rect;
        RctSetRectangle(&rect, ctrls[index].x, ctrls[index].y, w, h);
        WinCopyRectangle(WinGetDrawWindow(), hWnd, &rect, 0, 0, winPaint);
        return true;
    }
    return false;
}

void NativeDraw::line(int c, int x1, int y1, int x2, int y2) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    x1 += x; x2 += x;
    y1 += y; y2 += y;
    if (c==1) WinEraseLine(x1, y1, x2, y2);
    else if (c==2) WinDrawGrayLine(x1, y1, x2, y2);
    else if (c==3) WinInvertLine(x1, y1, x2, y2);
    else WinDrawLine(x1, y1, x2, y2);
}

void NativeDraw::pixel(int c, int x1, int y1) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    x1 += x;
    y1 += y;
    if (romVersion >= ver35) {
        if (c==1) WinErasePixel(x1, y1);
        else if (c==3) WinInvertPixel(x1, y1);
        else WinDrawPixel(x1, y1);
    } else {
        if (c==1) WinEraseLine(x1, y1, x1, y1);
        else if (c==3) WinInvertLine(x1, y1, x1, y1);
        else WinDrawLine(x1, y1, x1, y1);
    }
}

void NativeDraw::rect(int c, int x1, int y1, int x2, int y2, int rad) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    RectangleType rect;
    RctSetRectangle(&rect, x1 + x, y1 + y, x2-x1, y2-y1);

    if (c==1) WinEraseRectangle(&rect, rad);
    //else if (color->iVal==2)
    else if (c==3) WinInvertRectangle(&rect, rad);
    else WinDrawRectangle(&rect, rad);
}

void NativeDraw::frame(int c, int x1, int y1, int x2, int y2, int rad, int thick) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    RectangleType rect;
    RctSetRectangle(&rect, x1 + x, y1 + y, x2-x1, y2-y1);

    FrameBitsType fb;
    fb.word = 0;
    fb.bits.cornerDiam = rad;
    fb.bits.width = thick;
    
    if (c==1) WinEraseRectangleFrame(fb.word, &rect);
    else if (c==2) WinDrawGrayRectangleFrame(fb.word, &rect);
    else if (c==3) WinInvertRectangleFrame(fb.word, &rect);
    else WinDrawRectangleFrame(fb.word, &rect);
}

void NativeDraw::bitmap(int id, int dx, int dy, int mode) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    
    MemHandle hMem = DmGetResource('Tbmp', id);
    if (hMem) {
        WinDrawOperation dop = WinSetDrawMode((WinDrawOperation)mode);
        WinPaintBitmap((BitmapPtr)MemHandleLock(hMem), dx + x, dy + y);
        WinSetDrawMode(dop);
        MemHandleUnlock(hMem);
    }
}

int NativeDraw::textAlign(int ta) {
    int ret = alignMode;
    alignMode = ta;
    return ret;
}

int NativeDraw::underline(int ul) {
    int ret = ulMode;
    ulMode = ul;
    if (ulMode < 0 || ulMode > 2) ulMode = 0;
    return ret;
}

int NativeDraw::textWidth(char* str) {
    int len = strlen(str);
    return FntCharsWidth(str, len);
}

int NativeDraw::textHeight() {
    return FntCharHeight();
}

void NativeDraw::text(int c, int x1, int y1, char* str, bool bPocketC) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    int len = strlen(str);
    
    x1 = x + x1;
    y1 = y + y1;
    
    // Adjust for alignment
    if (alignMode % 10)
        x1 -= FntCharsWidth(str, len) / (alignMode % 10 == 2 ? 1 : 2);
    if (alignMode / 10)
        y1 -= FntBaseLine() / (alignMode / 10 == 2 ? 1 : 2);

    RGBColorType rgb;
    if (romVersion < ver35) {
        WinSetColors(&rgb_text, &rgb, 0, 0);
    }
    WinSetUnderlineMode((UnderlineModeType)ulMode);

    if (!bPocketC) {
        if (c==1) WinDrawInvertedChars(str, len, x1, y1);
        else WinDrawChars(str, len, x1, y1);
    } else {
        if (c==0) WinEraseChars(str, len, x1, y1);
        else if (c==2) WinDrawInvertedChars(str, len, x1, y1);
        else if (c==3) WinInvertChars(str, len, x1, y1);
        else WinDrawChars(str, len, x1, y1);
    }
    WinSetUnderlineMode(noUnderline);	
    if (romVersion < ver35) {
        WinSetColors(&rgb, 0, 0, 0);
    }
}

void NativeDraw::textTrunc(int x1, int y1, int w, char* str) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    int len = strlen(str);
    
    x1 = x + x1;
    y1 = y + y1;
    
    RGBColorType rgb;
    if (romVersion < ver35) {
        WinSetColors(&rgb_text, &rgb, 0, 0);
    }
    WinSetUnderlineMode((UnderlineModeType)ulMode);
    WinDrawTruncChars(str, len, x1, y1, w);
    WinSetUnderlineMode(noUnderline);	
    if (romVersion < ver35) {
        WinSetColors(&rgb, 0, 0, 0);
    }
}

void NativeDraw::draw(NativeDraw* draw, int dx, int dy, int mode) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (!hWnd) return;
    if (draw->bOffscreen && draw->hWnd) {
        RectangleType rect;
        RctSetRectangle(&rect, draw->x, draw->y, draw->w, draw->h);
        WinCopyRectangle(draw->hWnd, hWnd, &rect, dx + x, dy + y, (WinDrawOperation)mode);
    }
}

static const byte pal2[4] = { 255, 170, 85, 0 };
static const byte pal4[16] = { 255, 238, 221, 204, 187, 170, 153, 136, 119,
    102, 85, 68, 51, 34, 17, 0 };

int NativeDraw::indexFromColor(int r, int g, int b) {
    int clr = 0;
    if (romVersion < ver35) {
        // do it ourself
        UInt32 depth = 0;
        Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
        
        if (depth == 1) {
            clr = r*r+g*g+b*b < 128*128*3;
        } else {
            const byte* pal = pal2;
            int n = 4;
            if (depth == 4)  { pal = pal4; n = 16; }
            long diff, prevDiff = 255 * 255;
            clr = n-1;
            for (int i=0;i<n;i++) {
                diff = (r-pal[i])*(r-pal[i]) + (g-pal[i])*(g-pal[i]) + (b-pal[i])*(b-pal[i]);
                if (diff > prevDiff) { clr = i-1; break; }
                prevDiff = diff;
            }
        }
    } else {
        RGBColorType rgb;
        rgb.r = r;
        rgb.g = g;
        rgb.b = b;
        clr = WinRGBToIndex(&rgb);
    }

    return clr;
}

static void getRGB(RGBColorType* rgb, int index, int depth) {
    if (depth == 1) {
        rgb->r = rgb->g = rgb->b = (1 - index) * 255;
    } else if (depth == 2) {
        rgb->r = rgb->g = rgb->b = pal2[index];
    } else if (depth == 4) {
        rgb->r = rgb->g = rgb->b = pal4[index];
    }
}

int NativeDraw::fg(int clr) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (romVersion < ver35) {
        RGBColorType rgb, oldrgb;
        UInt32 depth = 0;
        Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
        if (clr >= (1 << depth)) clr = (1 << depth) - 1;
        getRGB(&rgb, clr, depth);
        WinSetColors(&rgb, &oldrgb, 0, 0);
        int prev = indexFromColor(oldrgb.r, oldrgb.g, oldrgb.b);
        return prev;
    } else {
        int old = WinSetForeColor(clr);
        //if (old_fg == -1) old_fg = old;
        return old;
    }
}

void NativeDraw::fgRGB(int r, int g, int b) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    RGBColorType rgb;
    rgb.r = r; rgb.g = g; rgb.b = b;
    if (romVersion < ver35) {
        WinSetColors(&rgb, 0, 0, 0);
    } else if (romVersion >= ver40) {
        WinSetForeColorRGB(&rgb, NULL);
    } else {
        WinSetForeColor(WinRGBToIndex(&rgb));
    }
}

int NativeDraw::bg(int clr) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (romVersion < ver35) {
        RGBColorType rgb, oldrgb;
        UInt32 depth = 0;
        Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
        if (clr >= (1 << depth)) clr = (1 << depth) - 1;
        getRGB(&rgb, clr, depth);
        WinSetColors(0, 0, &rgb, &oldrgb);
        int prev = indexFromColor(oldrgb.r, oldrgb.g, oldrgb.b);
        return prev;
    } else {
        int old = WinSetBackColor(clr);
        //if (old_bg == -1) old_bg = old;
        return old;
    }
}

void NativeDraw::bgRGB(int r, int g, int b) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    RGBColorType rgb;
    rgb.r = r; rgb.g = g; rgb.b = b;
    if (romVersion < ver35) {
        WinSetColors(0, 0, &rgb, 0);
    } else if (romVersion >= ver40) {
        WinSetBackColorRGB(&rgb, NULL);
    } else {
        WinSetBackColor(WinRGBToIndex(&rgb));
    }
}

int NativeDraw::textColor(int clr) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    if (romVersion < ver35) {
        UInt32 depth = 0;
        Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
        if (clr >= (1 << depth)) clr = (1 << depth) - 1;
        int prev = indexFromColor(rgb_text.r, rgb_text.g, rgb_text.b);
        getRGB(&rgb_text, clr, depth);
        return prev;
    } else {
        int old = WinSetTextColor(clr);
        //if (old_text == -1) old_text = old;
        return old;
    }
}

void NativeDraw::textRGB(int r, int g, int b) {
#ifdef DEBUG
    if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    RGBColorType rgb;
    rgb.r = r; rgb.g = g; rgb.b = b;
    if (romVersion < ver35) {
        rgb_text = rgb;
    } else if (romVersion >= ver40) {
        WinSetTextColorRGB(&rgb, NULL);
    } else {
        WinSetTextColor(WinRGBToIndex(&rgb));
    }
}

int NativeDraw::font(int f) {
#ifdef DEBUG
    //if (!nBegin) vm->vm_error("Attempt to draw without calling 'begin/nbegin'");
#endif
    return FntSetFont((FontID)f);
}

int NativeDraw::uiColorIndex(int type) {
    if (romVersion >= ver35) {
        return UIColorGetTableEntryIndex((UIColorTableEntries)type);
    } else return 0;
}

bool NativeDraw::read(NativeDBRecord* pRec, int iRec) {
    // TODO: read
    return false;
}

bool NativeDraw::write(NativeDBRecord* pRec, int iRec) {
    // TODO: write
    return false;
}
