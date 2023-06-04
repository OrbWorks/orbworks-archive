#if 0
void _____Graphics_____() {}
#endif
///////////////////////////////////////////////////////////////////////////////////////
//
// Graphics routines
//
void lib_graph_on(short) {
    openOutput = true;
    FrmGotoForm(GraphForm);
    UIYield(false);
};

void lib_graph_off(short) {
    FrmGotoForm(OutputForm);
    UIYield(false);
};

void lib_clearg(short) {
    if (FrmGetActiveFormID()==GraphForm) {
        RectangleType rect;
        // todo: high res
        RctSetRectangle(&rect, 0, 0, 160,160);
        WinEraseRectangle(&rect, 0);
        FrmDrawForm(FrmGetActiveForm());
    }
}

short textColor=1, textUL=0, textAlign=0;

void lib_textalign(short) {
    Value arg;
    pop(arg);
    textAlign = arg.cVal;
}	

void lib_textwidth(short) {
    Value text;
    pop(text);
    char* str = LockString(&text);
    short len = strlen(str);

    retVal.iVal = FntCharsWidth(str, len);	
    
    UnlockReleaseString(&text);
}

void lib_text(short) {
    Value text, x, y;
    pop(text);
    pop(y);
    pop(x);
    char* str = LockString(&text);
    short len = strlen(str);
    
    // Adjust for alignment
    if (textAlign % 10)
        x.iVal -= FntCharsWidth(str, len) / (textAlign % 10 == 2 ? 1 : 2);
    if (textAlign / 10)
        y.iVal -= FntBaseLine() / (textAlign / 10 == 2 ? 1 : 2);

    WinSetUnderlineMode((UnderlineModeType)textUL);
    if (textColor==0) WinEraseChars(str, len, x.iVal, y.iVal);
    else if (textColor==2) WinDrawInvertedChars(str, len, x.iVal, y.iVal);
    else if (textColor==3) WinInvertChars(str, len, x.iVal, y.iVal);
    else WinDrawChars(str, len, x.iVal, y.iVal);
    WinSetUnderlineMode(noUnderline);	

    UnlockReleaseString(&text);
};

void lib_line(short) {
    Value col, x1, x2, y1, y2;
    pop(y2);
    pop(x2);
    pop(y1);
    pop(x1);
    pop(col);
    
    if (!bHighDensity) {
        if (y1.iVal >= 160 || y2.iVal >= 160) {
            if (y1.iVal >= 160 && y2.iVal >= 160) return;
            short swap;
            if (y1.iVal >= 160) {
                swap = x1.iVal;
                x1.iVal = x2.iVal;
                x2.iVal = swap;
                swap = y1.iVal;
                y1.iVal = y2.iVal;
                y2.iVal = swap;
            }
            x2.iVal = x1.iVal + (160 - y1.iVal) * (x2.iVal - x1.iVal) / (y2.iVal - y1.iVal);
            y2.iVal = 159;
        }
    }
    if (col.iVal==0) WinEraseLine(x1.iVal, y1.iVal, x2.iVal, y2.iVal);
    else if (col.iVal==2) WinDrawGrayLine(x1.iVal, y1.iVal, x2.iVal, y2.iVal);
    else if (col.iVal==3) WinInvertLine(x1.iVal, y1.iVal, x2.iVal, y2.iVal);
    else WinDrawLine(x1.iVal, y1.iVal, x2.iVal, y2.iVal);
    
};

char tit[80];
void lib_title(short) {
    Value title;
    pop(title);
    
    if (!openOutput) {
        FrmGotoForm(OutputForm);
        openOutput = true;
        UIYield(false);
    }

    char* titlestr = LockString(&title);
    //if (strlen(titlestr) > 79) titlestr[79] = '\0'; // Ensure that the title is <= 79 chars
    strncpy(tit, titlestr, 79);
    tit[79] = '\0';
    FrmSetTitle(FrmGetActiveForm(), tit);
    UnlockReleaseString(&title);
}

void lib_rect(short) {
    Value col, x1, x2, y1, y2, d;
    pop(d);
    pop(y2);
    pop(x2);
    pop(y1);
    pop(x1);
    pop(col);

    RectangleType rect;
    RctSetRectangle(&rect, x1.iVal, y1.iVal, x2.iVal-x1.iVal, y2.iVal-y1.iVal);

    if (col.iVal==0) WinEraseRectangle(&rect, d.iVal);
    //else if (col.iVal==2) 
    else if (col.iVal==3) WinInvertRectangle(&rect, d.iVal);
    else WinDrawRectangle(&rect, d.iVal);
}

void lib_frame(short index) {
    Value col, x1, x2, y1, y2, d, thick;
    if (index > 50) pop(thick);
    else thick.iVal = 1;
    pop(d);
    pop(y2);
    pop(x2);
    pop(y1);
    pop(x1);
    pop(col);

    RectangleType rect;
    RctSetRectangle(&rect, x1.iVal, y1.iVal, x2.iVal-x1.iVal, y2.iVal-y1.iVal);

    FrameBitsType fb;
    fb.word = 0;
    fb.bits.cornerDiam = d.iVal;
    fb.bits.width = thick.iVal;
    
    if (col.iVal==0) WinEraseRectangleFrame(fb.word, &rect);
    else if (col.iVal==2) WinDrawGrayRectangleFrame(fb.word, &rect);
    else if (col.iVal==3) WinInvertRectangleFrame(fb.word, &rect);
    else WinDrawRectangleFrame(fb.word, &rect);
}


void lib_textattr(short) {
    Value font, col, ul;
    pop(ul);
    pop(col);
    pop(font);
    
    if (col.iVal < 0 || col.iVal > 2) col.iVal = 0;
    textColor = col.iVal;

    if (font.iVal < 0) font.iVal = 0;
    if (romVersion < 0x03000000 && font.iVal > 6) font.iVal = 0;
    
    FntSetFont((FontID)font.iVal);

    if (ul.iVal < 0 || ul.iVal > 2) ul.iVal = 0;
    textUL = ul.iVal;
}

char hval(char);
#define hval2 hval

typedef struct MyBitmapFlagsType
{
    UInt16 	compressed:1;  				// Data format:  0=raw; 1=compressed
    UInt16 	hasColorTable:1;				// if true, color table stored before bits[]
    UInt16 	hasTransparency:1;			// true if transparency is used
    UInt16 	indirect:1;						// true if bits are stored indirectly
    UInt16 	forScreen:1;					// system use only
    UInt16	directColor:1;					// direct color bitmap
    UInt16 	reserved:10;
}
MyBitmapFlagsType;
    
// this definition correspond to the 'Tbmp' and 'tAIB' resource types
typedef struct MyBitmapType
{
    Int16				width;
    Int16				height;
    UInt16				rowBytes;
    MyBitmapFlagsType	flags;
    UInt8				pixelSize;			// bits/pixel
    UInt8				version;			// version of bitmap. This is vers 2
    UInt16				nextDepthOffset;	// # of DWords to next BitmapType
                                            //  from beginnning of this one
    UInt8				transparentIndex;	// v2 only, if flags.hasTransparency is true,
                                            // index number of transparent color
    UInt8				compressionType;	// v2 only, if flags.compressed is true, this is
                                            // the type, see BitmapCompressionType
                                                        
    UInt16	 			reserved;			// for future use, must be zero!
    
    // if (flags.hasColorTable)
    //	  ColorTableType	colorTable		// NOTE: Could have 0 entries (2 bytes long)
    //
    // if (flags.directColor)
    //	  BitmapDirectInfoType	directInfo;
    // 
    // if (flags.indirect)
    //	  void*	  bitsP;						// pointer to actual bits
    // else
    //    UInt8	  bits[];					// or actual bits
    //
}
MyBitmapType;

#define BitmapType MyBitmapType
#define BitmapPtr MyBitmapType*

void lib_bitmap(short) {
    Value hbits, x, y;
    pop(hbits);
    pop(y);
    pop(x);
    
    char* bits = LockString(&hbits);
    char* data;
    short w,h,ci;
    
    if (strlen(bits) < 3) goto error;
    w = (hval2(*bits) * 16 + hval2(bits[1])); // Bits
    if (!w) goto error;
    h = (strlen(bits) - 2) / ((w+3)/4);
    
    short rowBytes = (((w+15) >> 4) << 1);
    short size = sizeof(BitmapType) + (h+2) * rowBytes;
    
    BitmapPtr bp = (BitmapPtr)malloc(size);
    if (!bp) {
        vm_error("Not enough memory to draw bitmap", pc);
        goto cleanup;
    }
    MemSet(bp, size, 0);
    bp->width = w;
    bp->height = h;
    bp->rowBytes = rowBytes;
    bp->flags.compressed = false;
    bp->pixelSize = 1;
    bp->version = 1;
    data = (char*)bp + sizeof(BitmapType);
    
    bits+=2; ci=0;
    for (short i=0;i<bp->rowBytes * h;i++) {
        data[i] = hval2(*bits++) << 4; ci++;
        if (ci < (w+3)/4) data[i] |= hval2(*bits++); ci++;
        if (ci >= (w+3)/4) {
            ci=0;
            if (!(i%2)) i++; // word-aligning
        }
    }
#undef BitmapPtr
    WinDrawBitmap((BitmapPtr)bp, x.iVal, y.iVal);
    free(bp);
cleanup:
    UnlockReleaseString(&hbits);
    return;
error:
    vm_error("Invalid bitmap", pc);
    goto cleanup;
}

#define MAX_SAVE_BITS 3
WinHandle saveBits[MAX_SAVE_BITS];
short nSaveBits;

void lib_saveg(short) {
    Word err;
    RectangleType rect;
    
    if (nSaveBits == MAX_SAVE_BITS) {
        WinDeleteWindow(saveBits[0], 0);
        for (short i=0;i<MAX_SAVE_BITS-1;i++)
            saveBits[i] = saveBits[i+1];
        nSaveBits--;
    }

    RctSetRectangle(&rect, 0, 0, 160, 160);

    saveBits[nSaveBits] = WinSaveBits(&rect, &err);
    if (saveBits[nSaveBits]) retVal.iVal = 1;
    nSaveBits++;
}

void lib_restoreg(short) {
    if (!nSaveBits) return;

    WinRestoreBits(saveBits[--nSaveBits], 0, 0);
}

//
// color support
//

void lib_setfgi(short) {
    Value vIndex;
    pop(vIndex);
    if (romVersion >= ver35) {
        retVal.iVal = WinSetForeColor(vIndex.iVal);
    }
}

void lib_setbgi(short) {
    Value vIndex;
    pop(vIndex);
    if (romVersion >= ver35) {
        retVal.iVal = WinSetBackColor(vIndex.iVal);
    }
}

void lib_settextcolori(short) {
    Value vIndex;
    pop(vIndex);
    if (romVersion >= ver35) {
        retVal.iVal = WinSetTextColor(vIndex.iVal);
    }
}

void lib_setfg(short) {
    Value vR, vG, vB;
    pop(vB);
    pop(vG);
    pop(vR);
    
    if (romVersion >= ver35) {
        RGBColorType rgb;
        rgb.r = vR.iVal; rgb.g = vG.iVal; rgb.b = vB.iVal;
        if (romVersion >= ver40) {
            WinSetForeColorRGB(&rgb, NULL);
        } else {
            WinSetForeColor(WinRGBToIndex(&rgb));
        }
    }
}

void lib_setbg(short) {
    Value vR, vG, vB;
    pop(vB);
    pop(vG);
    pop(vR);
    
    if (romVersion >= ver35) {
        RGBColorType rgb;
        rgb.r = vR.iVal; rgb.g = vG.iVal; rgb.b = vB.iVal;
        if (romVersion >= ver40) {
            WinSetBackColorRGB(&rgb, NULL);
        } else {
            WinSetBackColor(WinRGBToIndex(&rgb));
        }
    }
}

void lib_settextcolor(short) {
    Value vR, vG, vB;
    pop(vB);
    pop(vG);
    pop(vR);
    
    if (romVersion >= ver35) {
        RGBColorType rgb;
        rgb.r = vR.iVal; rgb.g = vG.iVal; rgb.b = vB.iVal;
        if (romVersion >= ver40) {
            WinSetTextColorRGB(&rgb, NULL);
        } else {
            WinSetTextColor(WinRGBToIndex(&rgb));
        }
    }
}

void lib_rgbtoi(short) {
    Value vR, vG, vB;
    pop(vB);
    pop(vG);
    pop(vR);
    
    if (romVersion >= ver35) {
        RGBColorType rgb;
        rgb.r = vR.iVal; rgb.g = vG.iVal; rgb.b = vB.iVal;
        retVal.iVal = WinRGBToIndex(&rgb);
    }
}

void lib_getcolordepth(short) {
    if (romVersion >= ver35) {
        UInt32 depth = 0;
        Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
        retVal.iVal = depth;
    } else {
        retVal.iVal = 1;
    }
}

void lib_setcolordepth(short) {
    Value vd;
    pop(vd);
    if (romVersion >= ver35) {
        UInt32 depth = vd.iVal;
        Err err = WinScreenMode(winScreenModeSet, 0, 0, &depth, 0);
        retVal.iVal = (err == 0);
    } // else return 0
}

void lib_getuicolor(short) {
    Value type;
    pop(type);
    if (romVersion >= ver35) {
        retVal.iVal =  UIColorGetTableEntryIndex((UIColorTableEntries)type.iVal);
    } // else return 0
}

void lib_pixel(short) {
    Value c1, x1, y1;
    pop(y1);
    pop(x1);
    pop(c1);
    short c = c1.iVal, x = x1.iVal, y = y1.iVal;
    if (romVersion >= ver35) {
        if (c==0) WinErasePixel(x, y);
        else if (c==3) WinInvertPixel(x, y);
        else WinDrawPixel(x, y);
    } else {
        if (c==0) WinEraseLine(x, y, x, y);
        else if (c==3) WinInvertLine(x, y, x, y);
        else WinDrawLine(x, y, x, y);
    }
}
short nPushDraw;
void lib_pushdraw(short) {
    if (romVersion >= ver35) WinPushDrawState();
    nPushDraw++;
}

void lib_popdraw(short) {
    if (romVersion >= ver35) WinPopDrawState();
    nPushDraw--;
}

void lib_choosecolori(short) {
    Value vpIndex;
    Value* pvIndex;
    Value vTitle;
    pop(vpIndex);
    pop(vTitle);
    pvIndex = deref(vpIndex.iVal);
    char* strTitle = LockString(&vTitle);

    if (romVersion >= ver35) {
        unsigned char index = pvIndex->iVal;
        if ((retVal.iVal = UIPickColor(&index, NULL, 0, strTitle, NULL))) {
            pvIndex->iVal = index;
        }
    }
    UnlockReleaseString(&vTitle);
}

//
// density support
//

void lib_setdrawdensity(short) {
    Value v;
    pop(v);
    if (bHighDensity) {
        WinSetCoordinateSystem(v.iVal ? 0 : kCoordinatesStandard);
    }
}

void lib_getscreenattrib(short) {
    Value v;
    pop(v);
    UInt32 attrib = v.iVal;
    if (bHighDensity) {
        UInt32 value;
        WinScreenGetAttribute((WinScreenAttrTag)attrib, &value);
        retVal.iVal = value;
    } else {
        if (attrib == 0 || attrib == 1)
            retVal.iVal = 160;
        else if (attrib == 5)
            retVal.iVal = 72;
        else
            retVal.iVal = 0;
    }
}


//
// offscreen buffer support
//

#define MAX_OFFSCREEN 8
WinHandle offScreen[MAX_OFFSCREEN];

// buffer 0 implies screen
// buffer -1 returned on error
// buffer 1-MAX_OFFSCREEN is offScreen[i-1]
void lib_bucreate(short) {
    Value w, h;
    pop(h); pop(w);
    
    // find an empty slot
    short i;
    for (i=0;i<MAX_OFFSCREEN;i++) {
        if (offScreen[i] == NULL) break;
    }
    if (i < MAX_OFFSCREEN) {
        UInt16 err;
        offScreen[i] = WinCreateOffscreenWindow(w.iVal, h.iVal, nativeFormat, &err);
        if (offScreen[i]) {
            if (romVersion < ver35) {
                RectangleType rect;
                RctSetRectangle(&rect, 0, 0, w.iVal, h.iVal);
                WinHandle hPrevWnd = WinSetDrawWindow(offScreen[i]);
                WinEraseRectangle(&rect, 0);
                WinSetDrawWindow(hPrevWnd);
            }
            retVal.iVal = i + 1;
        } else
            retVal.iVal = 0;
        
    } else {
        // no free buffers
        retVal.iVal = 0;
    }
}

WinHandle getBufferHandle(long id, bool allowDisplay) {
    if (id > 0 && id <= MAX_OFFSCREEN) {
        return offScreen[id - 1];
    } else if (id == 0 && allowDisplay) {
        return WinGetDisplayWindow();
    }
    return NULL;
}

void lib_budelete(short) {
    Value id;
    pop(id);
    
    WinHandle win = getBufferHandle(id.iVal, false);
    if (win) {
        WinDeleteWindow(win, false);
        offScreen[id.iVal - 1] = NULL;
    }
}

void lib_buset(short) {
    Value id;
    pop(id);
    
    WinHandle win = getBufferHandle(id.iVal, false);
    if (win == NULL) win = WinGetDisplayWindow();
    WinSetDrawWindow(win);
}

void lib_bucopyrect(short) {
    Value vmode, vyd, vxd, vdid, vh, vw, vys, vxs, vsid;
    pop(vmode); pop(vyd); pop(vxd); pop(vdid);
    pop(vh); pop(vw); pop(vys); pop(vxs); pop(vsid);
    
    WinHandle src = getBufferHandle(vsid.iVal, true);
    WinHandle dst = getBufferHandle(vdid.iVal, true);
    if (src == NULL || dst == NULL) return;
    
    RectangleType rect;
    RctSetRectangle(&rect, vxs.iVal, vys.iVal, vw.iVal, vh.iVal);
    WinCopyRectangle(src, dst, &rect, vxd.iVal, vyd.iVal, (WinDrawOperation)vmode.iVal);
}

void lib_bucopy(short) {
    Value vmode, vy, vx, vsid, vdid;
    pop(vmode); pop(vy); pop(vx); pop(vdid); pop(vsid);
    
    WinHandle src = getBufferHandle(vsid.iVal, true);
    WinHandle dst = getBufferHandle(vdid.iVal, true);
    if (src == NULL || dst == NULL) return;
    
    Coord w, h;
    WinHandle prev = WinSetDrawWindow(dst);
    WinGetWindowExtent(&w, &h);
    RectangleType rect;
    RctSetRectangle(&rect, 0, 0, w, h);
    WinCopyRectangle(src, dst, &rect, vx.iVal, vy.iVal, (WinDrawOperation)vmode.iVal);
}

//
// resource bitmap support
//
#define MAX_RESDBS 4
DmOpenRef openResDBs[MAX_RESDBS];

void lib_resopen(short) {
    Value vname; pop(vname);
    short i = 0;
    for (;i<MAX_RESDBS;i++) {
        if (openResDBs[i] == NULL) break;
    }
    if (i == MAX_RESDBS) {
        // no slot found
        ReleaseString(&vname);
        return;
    }
    LocalID lid = DmFindDatabase(0, LockString(&vname));
    if (lid) {
        openResDBs[i] = DmOpenDatabase(0, lid, dmModeReadOnly);
        if (openResDBs[i]) retVal.iVal = i + 1;
    }
    
    UnlockReleaseString(&vname);
}

void lib_resclose(short) {
    Value id; pop(id);
    if (id.iVal > 0 && id.iVal <= MAX_RESDBS && openResDBs[id.iVal - 1]) {
        DmCloseDatabase(openResDBs[id.iVal - 1]);
        openResDBs[id.iVal - 1] = NULL;
    }
}

void lib_bitmapr(short) {
    Value y, x, id;
    pop(y); pop(x); pop(id);
    MemHandle hMem = DmGetResource('Tbmp', id.iVal);
    if (hMem) {
        WinDrawBitmap((BitmapPtr)MemHandleLock(hMem), x.iVal, y.iVal);
        MemHandleUnlock(hMem);
    }
}

void lib_bitmaprm(short) {
    Value y, x, id, mode;
    pop(mode); pop(y); pop(x); pop(id);
    if (romVersion >= ver35) {
        MemHandle hMem = DmGetResource('Tbmp', id.iVal);
        if (hMem) {
            WinDrawOperation dop = WinSetDrawMode((WinDrawOperation)mode.iVal);
            WinPaintBitmap((BitmapPtr)MemHandleLock(hMem), x.iVal, y.iVal);
            WinSetDrawMode(dop);
            MemHandleUnlock(hMem);
        }
    }
}

