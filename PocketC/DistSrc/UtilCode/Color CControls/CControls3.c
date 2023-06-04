/$ Ccontrols3.c
Cdb(pointer p) {
    int c;
    int sel;
    if (p[0]>0)
        c=p[6];
    else
        c=2;
    pushdraw();
    settextcolori(clrObjFG);
    if (p[0]==3)
        if (p[8])
            sel = true;
    if (sel) {
        setbgi(clrObjSelFill);
        settextcolori(clrObjSelFG);
    } else {
        setfgi(clrObjFrame);
        setbgi(clrObjFill);
    }
    if (p[0]==2)
        setbgi(clrFormFill);
    rect(0,p[1],p[2],p[1]+p[3],p[2]+p[4],p[7]);
    frame(c,p[1],p[2],p[1]+p[3],p[2]+p[4],p[7]);
    if (p[0]==2)
        bitmap(p[1],p[2],*p[5]);
    else {
        textattr(0,1,0);
        textalign(11);
        text(p[1]+(p[3]/2),p[2]+(p[4]/2)-2,*p[5]);
        textalign(0);
    }
    popdraw();
    if (!useColor && sel) Cib(p);
}

Cdr(pointer p) {
    int i,x,w;
    w=p[3]/p[8];
    pushdraw();
    setbgi(clrObjFill);
    setfgi(clrObjFrame);
    settextcolori(clrObjFG);
    rect(0,p[1],p[2],p[1]+(p[8]*w),p[2]+p[4],0);
    textattr(0,1,0);
    textalign(11); 
    for (i=0;i<p[8];i++) {
        x=p[1]+(i*w);
        if (p[6]==i) {
            pushdraw();
            setbgi(clrObjSelFill);
            settextcolori(clrObjSelFG);
            rect(0,x,p[2],x+w-1,p[2]+p[4],0);
        }			
        frame(1,x,p[2],x+w-1,p[2]+p[4],0);
        text(x+(w/2),p[2]+(p[4]/2)-2,*(p[7]+i));
        if (p[6]==i)
            popdraw();
    }
    textalign(0);
    if (!useColor) Cir(p);
    popdraw();
}

Cde(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    setfgi(clrObjFrame);
    settextcolori(clrObjFG);
    rect(0,p[1],p[2],p[1]+p[3],p[2]+p[4],0);
    if (p[7]==2)
        frame(p[6],p[1],p[2],p[1]+p[3],p[2]+p[4],0);
    else if (p[7]==1)
        line(p[6],p[1],p[2]+p[4],p[1]+p[3],p[2]+p[4]);
    Cet(p);
    popdraw();
}

Cdl(pointer p) {
    pushdraw();
    setbgi(clrObjFill);
    setfgi(clrObjFrame);
    settextcolori(clrObjFG);
    frame(1,p[1],p[2],p[1]+p[3],p[2]+p[4],0);
    memcpy(&curFrame, &clrObjFrame, 5);
    Cdlbi(p);
    popdraw();
}

Cdt(pointer p) {
    int x,r;
    x=p[1];
    pushdraw();
    setfgi(clrObjFrame);
    frame(1,p[1],p[2],p[1]+p[3],p[2]+p[4],0);
    for (r=0;r<p[12]-1;r++) {
        x=x+*(p[10]+r);
        line(2,x-1,p[2],x-1,p[2]+p[4]-1);
    }
    popdraw();
    memcpy(&curFrame, &clrObjFrame, 5);
    Cdlbi(p);
}

Cdd(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    setfgi(clrObjFG);
    settextcolori(clrObjFG);
    bitmap(p[1],p[2],"0800000000ff7e3c18");
    Cdrt(p);
    popdraw();
}

Cdc(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    setfgi(clrObjFG);
    settextcolori(clrObjFG);
    Cck(p);
    rect(0,p[1]+12,p[2],p[1]+p[3],p[2]+p[4],0);
    textattr(0,1,0);
    textalign(0);
    text(p[1]+12,p[2],*p[5]);
    popdraw();
}

Cdf(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    setfgi(clrFormFrame);
    rect(0,p[1],p[2],p[1]+p[3],p[2]+p[4],0);
    frame2(1,p[1]+2,p[2]+2,p[1]+p[3]-2,p[2]+p[4]-2,3,2);
    rect(1,p[1]+2,p[2]+2,p[1]+p[3],p[2]+14,0);
    setbgi(clrFormFrame);
    settextcolori(clrFormFill);
    textattr(1,1,0);
    textalign(1);
    text(p[1]+(p[3]/2),p[2]+2,*p[5]); 
    textattr(0,1,0);
    textalign(0);
    popdraw();
}
