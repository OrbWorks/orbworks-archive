/$ Ccontrols4.c
Cdraw(pointer p) {
    int c;
    c=p[0];
    if (c<0)
        c=-c;
    if (c>100)
        return;
    switch (c) {
        case 1:
        case 2:
        case 3: Cdb(p); break;
        case 9:
        case 13: Cdt(p); break;
        case 10: Cdd(p); break;
        case 5: Cdc(p); break;
        case 4:
        case 15: Cde(p); break;
        case 7: Cdr(p); break;
        case 12: Cdl(p); break;
        case 14: Cdf(p); break;
    }
}

Cerase(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    if (p[0]==10)
        rect(0,p[1]-1,p[2]-1,p[1]+12,p[2]+12,0);
    else
        rect(0,p[1]-1,p[2]-1,p[1]+p[3]+1,p[2]+p[4]+1,0);
    popdraw();
}

Chide(pointer p) {
    int c;
    c=p[0];
    if (c<0) c=-c;
    if (c>100) return;
    p[0]=p[0]*100;
    Cerase(p);
}

Cshow(pointer p) {
    int c;
    c=p[0];
    if (c<0) c=-c;
    if (c<100) return;
    p[0]=p[0]/100;
    Cdraw(p);
}

Cactivate(pointer p) {
    if (p[0]>0) return;
    p[0]=-(p[0]);
    if (p[0]==1) Cdraw(p);
}

Cdeactivate(pointer p) {
    if (p[0]<0) return;
    p[0]=-(p[0]);
    if (p[0]==-1) Cdraw(p);
}

Csetstate(pointer p, int s) {
    if (p[0]==3) {
        p[8]=s;
        return 1;
    }
    else if (p[0]==5) {
        p[6]=s;
        return 1;
    }
    else
        return 0;
}

Cgetstate(pointer p) {
    if (p[0]==3) return p[8];
    else if (p[0]==5) return p[6];
    else return 0;
}

Csetcursel(pointer p, int s) {
    if ((p[0]>6)&&(p[0]<14)) {
        p[6]=s;
        return 1;
    }
    else return 0;
}

Cgetcursel(pointer p) {
    if ((p[0]>6)&&(p[0]<14)) return p[6];
    else return 0;
}

Cdestroy(pointer p) {
    Cerase(p);
    if ((p[0]<=5)||(p[0]>=14))
        free(p[5]);
    else if (p[0]>=9)
        free(p[7]);
    if ((p[0]==8)||(p[0]==9))
        free(p[10]);
    if (p[0]==7)
        free(p[6]);
    if (p[0]==6) {
        free(p[1]);
        free(p[2]);
    }
    free(p);
}

int Cpopupevent(pointer p) {
    int r;
    saveg();
    p[6]=-1;
    memcpy(&curFrame, &clrObjFrame, 5);
    r=Cdrp(p);
    restoreg();
    return r;
}

Csetsize(pointer p,int n) {
    int m;
    pointer c;
    if (p[0]==9)
        m=p[12];
    else
        m=1;
    c=malloct(n*m,"s");
    if (c==0)
        return 0;
    if (n<p[9])
        p[9]=n;
    memcpy(c,p[7],p[9]*m);
    p[9]=n;
    free(p[7]);
    p[7]=c;
    return 1;
}

Cadditem(pointer p,string s) {
    int m;
    if (p[0]==9)
        m=p[12];
    else
        m=1;
    p[8]=p[8]+1;
    if (p[9]<p[8])
        if (!Csetsize(p,p[8]))
            return 0;
    if (p[0]==8)
        p[4]=p[8]*11;
    *(p[7]+(p[8]*m)-m)=s;
    Csetcursel(p,p[8]-1);
    return 1;
}

Cremoveitem(pointer p) {
    int m,i;
    if (p[0]==9)
        m=p[12];
    else
        m=1;
    p[8]=p[8]-1;
    i=p[6]*m;
    memcpy(p[7]+i,p[7]+i+m,(p[8]*m)-i);
    Csetsize(p,p[8]);
    if (p[0]==8)
        p[4]=p[8]*11;
    Csetcursel(p,-1);
    return 1;
}

Cmenu(int x,int w,int wt) {
    pointer p,c;
    p=Cip(12,x,1,w,0);
    c=malloct(1,"s");
    p[0]=8;p[4]=0;p[5]=0;p[6]=-1;p[7]=0;p[8]=0;p[9]=0;p[10]=c;p[11]=wt;*p[10]="";
    return p;
}

Cmenubar() {
    pointer p,c,m;
    p=malloct(4,"i");
    c=malloct(1,"s");
    m=malloct(1,"i");
    p[0]=6;p[1]=c;p[2]=m;p[3]=0;
    hookmenu(1);
    return p;
}

Caddmenu(pointer p,pointer m) {
    pointer c;
    c=malloct(p[3]+1,"i");
    if (c==0)
        return 0;
    memcpy(c,p[2],p[3]);
    free(p[2]);
    p[2]=c; p[3]=p[3]+1;
    *(p[2]+p[3]-1)=m;
    return 1;
}

Cinit() {
    clrObjFrame = getuicolor(0);
    clrObjFill = getuicolor(1);
    clrObjFG = getuicolor(2);
    clrObjSelFill = getuicolor(3);
    clrObjSelFG = getuicolor(4);
    
    clrFormFrame = getuicolor(21);
    clrFormFill = getuicolor(22);

    clrMenuFrame = getuicolor(5);
    clrMenuFill = getuicolor(6);
    clrMenuFG = getuicolor(7);
    clrMenuSelFill = getuicolor(8);
    clrMenuSelFG = getuicolor(9);
    
    if (getcolordepth() >= 8)
        useColor = true;
}
