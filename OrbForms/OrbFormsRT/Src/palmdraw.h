// NativeDraw interface
NATIVE_INIT(lib_draw_init, NativeDraw);

void lib_draw_attachGadget(VM* vm, int) {
    word id = popID(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->attachGadget(id);
}

void lib_draw_attachForm(VM* vm, int) {
    word id = popID(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->attachForm(id);
}

void lib_draw_begin(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->begin(false);
}

void lib_draw_nbegin(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->begin(true);
}

void lib_draw_end(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->end();
}

void lib_draw_line(VM* vm, int) {
    Value* y2 = vm->pop(); assert(y2->type == vtInt);
    Value* x2 = vm->pop(); assert(x2->type == vtInt);
    Value* y1 = vm->pop(); assert(y1->type == vtInt);
    Value* x1 = vm->pop(); assert(x1->type == vtInt);
    Value* color = vm->pop(); assert(color->type == vtInt);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->line(color->iVal, x1->iVal, y1->iVal, x2->iVal, y2->iVal);
}

void lib_draw_pixel(VM* vm, int) {
    Value* y1 = vm->pop(); assert(y1->type == vtInt);
    Value* x1 = vm->pop(); assert(x1->type == vtInt);
    Value* color = vm->pop(); assert(color->type == vtInt);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->pixel(color->iVal, x1->iVal, y1->iVal);
}

void lib_draw_rect(VM* vm, int) {
    Value* rad = vm->pop(); assert(rad->type == vtInt);
    Value* y2 = vm->pop(); assert(y2->type == vtInt);
    Value* x2 = vm->pop(); assert(x2->type == vtInt);
    Value* y1 = vm->pop(); assert(y1->type == vtInt);
    Value* x1 = vm->pop(); assert(x1->type == vtInt);
    Value* color = vm->pop(); assert(color->type == vtInt);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->rect(color->iVal, x1->iVal, y1->iVal, x2->iVal, y2->iVal, rad->iVal);
}

void lib_draw_frame(VM* vm, int) {
    Value* thick = vm->pop(); assert(thick->type == vtInt);
    Value* rad = vm->pop(); assert(rad->type == vtInt);
    Value* y2 = vm->pop(); assert(y2->type == vtInt);
    Value* x2 = vm->pop(); assert(x2->type == vtInt);
    Value* y1 = vm->pop(); assert(y1->type == vtInt);
    Value* x1 = vm->pop(); assert(x1->type == vtInt);
    Value* color = vm->pop(); assert(color->type == vtInt);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->frame(color->iVal, x1->iVal, y1->iVal, x2->iVal, y2->iVal, rad->iVal, thick->iVal);
}

void lib_draw_bitmap(VM* vm, int index) {
    int mode = winPaint;
    if (index > 375)
        mode = vm->pop()->iVal;
    Value* y = vm->pop();
    Value* x = vm->pop();
    Value* bmpaddr = vm->pop();
    Value* id = vm->deref(bmpaddr->iVal);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->bitmap(id->iVal, x->iVal, y->iVal, mode);
}

void lib_draw_text(VM* vm, int) {
    Value* str = vm->pop();
    Value* y = vm->pop();
    Value* x = vm->pop();
    Value* c = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->text(c->iVal, x->iVal, y->iVal, str->lock(), false);
    str->unlockRelease();
}

void lib_draw_textTrunc(VM* vm, int) {
    Value* str = vm->pop();
    Value* w = vm->pop();
    Value* y = vm->pop();
    Value* x = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    draw->textTrunc(x->iVal, y->iVal, w->iVal, str->lock());
    str->unlockRelease();
}

void lib_draw_draw(VM* vm, int index) {
    int mode = winPaint;
    if (index > 375)
        mode = vm->pop()->iVal;
    Value* y = vm->pop();
    Value* x = vm->pop();
    NativeDraw* srcDraw = (NativeDraw*)PopNO(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->draw(srcDraw, x->iVal, y->iVal, mode);
}

void lib_draw_indexFromColor(VM* vm, int) {
    Value* b = vm->pop();
    Value* g = vm->pop();
    Value* r = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->indexFromColor(r->iVal, g->iVal, b->iVal);
}

void lib_draw_fg(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->fg(v->iVal);
}

void lib_draw_fgRGB(VM* vm, int) {
    Value* b = vm->pop();
    Value* g = vm->pop();
    Value* r = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->fgRGB(r->iVal, g->iVal, b->iVal);
}

void lib_draw_bg(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->bg(v->iVal);
}

void lib_draw_bgRGB(VM* vm, int) {
    Value* b = vm->pop();
    Value* g = vm->pop();
    Value* r = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->bgRGB(r->iVal, g->iVal, b->iVal);
}

void lib_draw_textColor(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->textColor(v->iVal);
}

void lib_draw_textRGB(VM* vm, int) {
    Value* b = vm->pop();
    Value* g = vm->pop();
    Value* r = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->textRGB(r->iVal, g->iVal, b->iVal);
}

void lib_draw_textWidth(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->textWidth(v->lock());
    v->unlockRelease();
}

void lib_draw_textHeight(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->textHeight();
}

void lib_draw_font(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->font(v->iVal);
}

void lib_draw_underline(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->underline(v->iVal);
}

void lib_draw_textAlign(VM* vm, int) {
    Value* v = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->textAlign(v->iVal);
}

void lib_draw_read(VM* vm, int) {
    Value* v = vm->pop();
    NativeDBRecord* rec = (NativeDBRecord*)PopNO(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->read(rec, v->iVal);
}

void lib_draw_write(VM* vm, int) {
    Value* v = vm->pop();
    NativeDBRecord* rec = (NativeDBRecord*)PopNO(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->write(rec, v->iVal);
}

void lib_draw_create(VM* vm, int) {
    Value* h = vm->pop();
    Value* w = vm->pop();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->create(w->iVal, h->iVal);
}

void lib_draw_copyForm(VM* vm, int) {
    word id = popID(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->copyForm(id);
}

void lib_draw_copyGadget(VM* vm, int) {
    word id = popID(vm);
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->copyGadget(id);
}

void lib_draw_release(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    draw->release();
}

void get_draw_handle(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->handle();
}

void get_draw_nw(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->nw();
}

void get_draw_nh(VM* vm, int) {
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->nh();
}

void lib_draw_selectIndex(VM* vm, int) {
    Value* vpIndex = vm->pop();
    Value* vIndex = vm->deref(vpIndex->iVal);
    Value* vTitle = vm->pop();
    char* strTitle = strdup(vTitle->lock());
    vTitle->unlockRelease();
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    
    byte index = vIndex->iVal;
    
    if ((vm->retVal.iVal = UIPickColor(&index, NULL, 0, strTitle, NULL))) {
        vIndex->iVal = index;
    }
    free(strTitle);
    vm->killVM = false;
}

void lib_draw_uiColorIndex(VM* vm, int) {
    int type = vm->pop()->iVal;
    NativeDraw* draw = (NativeDraw*)PopNO(vm);
    vm->retVal.iVal = draw->uiColorIndex(type);
}

void lib_app_getdepth(VM* vm, int) {
    vm->pop(); // ignore app
    UInt32 depth = 0;
    Err err = WinScreenMode(winScreenModeGet, 0, 0, &depth, 0);
    vm->retVal.iVal = depth;
}

void lib_app_setdepth(VM* vm, int) {
    Value* vd = vm->pop();
    vm->pop(); // ignore app
    UInt32 depth = vd->iVal;
    Err err = WinScreenMode(winScreenModeSet, 0, 0, &depth, 0);
    vm->retVal.iVal = (err == 0);
}

void lib_app_getscreenattrib(VM* vm, int) {
    int attrib = vm->pop()->iVal;
    vm->pop(); // ignore app
    if (bHighDensity) {
        UInt32 value;
        WinScreenGetAttribute((WinScreenAttrTag)attrib, &value);
        vm->retVal.iVal = value;
    } else {
        vm->retVal.iVal = 0;
    }
}
