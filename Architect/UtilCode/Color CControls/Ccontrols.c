/$ Ccontrols.c

int clrObjFrame, clrObjFill, clrObjFG, clrObjSelFill, clrObjSelFG;
int clrFormFrame, clrFormFill;
int clrMenuFrame, clrMenuFill, clrMenuFG, clrMenuSelFill, clrMenuSelFG;
int curFrame, curFill, curFG, curSelFill, curSelFG;
int useColor;

#define Chandle pointer

Cip(int s,int x,int y,int w,int h) {
    pointer p;
    p=malloct("i", s);
    if (!p)
        return 0;
    memcpy(&p[1], &x, 4);
    return p;
}

#include "Ccontrols1.c"
#include "Ccontrols2.c"
#include "Ccontrols3.c"
#include "Ccontrols4.c"

Cedit(int x,int y,int w,int l,int s,int a) {
    pointer p,c,t;
    p=Cip(11,x,y,w,11);
    p[5] = malloct("s", 1);
    p[10] = malloct("s", 1);
    p[0]=4; p[6]=l;p[7]=s;p[8]=a; *p[10]="";
    return p;
}

Clabel(int x,int y,int w,int l,int s,int a) {
    pointer p;
    p=Cedit(x,y,w,l,s,a);
    p[0]=15;
    return p;
}

Clistbox(int x,int y,int w,int h) {
    pointer p;
    p=Cip(10,x,y,w,h);
    p[0]=12;p[4]=h*11;p[5]=0;p[6]=-1; p[7]=0;p[8]=0;p[9]=0;
    return p;
}

Cdropdown(int x,int y,int w,int h) {
    pointer p;
    p=Clistbox(x,y,w,h);
    p[0]=10;p[6]=0;
    return p;
}

Cpopup(int x,int y,int w,int h) {
    pointer p;
    p=Clistbox(x,y,w,h);
    p[0]=11;
    return p;
}

Cbutton(int x,int y,int w,int h,int l,int b) {
    pointer p,c;
    p=Cip(8,x,y,w,h);
    c=malloct("s",1);
    p[0]=1;p[5]=c;p[6]=l;p[7]=b;
    if (h<12) p[4]=12;
    return p;
}

Cimage(int x,int y,int w,int h) {
    pointer p;
    p=Cbutton(x,y,w,h,0,0);
    p[0]=2;*p[5]="0";
    return p;
}

Cradio(int x,int y,int w,int h) {
    pointer p;
    p=Cip(10,x,y,w,h);
    if (h<12)
        p[4]=12;
    p[0]=7;p[5]=0;p[6]=0;p[7]=0;p[8]=0;p[9]=0;
    return p;
}

Cswitch(int x,int y,int w,int h,int l,int b) {
    pointer p,c;
    p=Cip(9,x,y,w,h);
    c=malloct("s",1);
    if (h<12)
        p[4]=12;
    p[0]=3;p[5]=c;p[6]=l; p[7]=b;p[8]=0;
    return p;
}

Ccheckbox(int x,int y,int w) {
    pointer p,c;
    p=Cip(7,x,y,w,12);
    c=malloct("s",1);
    p[0]=5;p[5]=c;p[6]=0;
    return p;
}

Csetrow(pointer p,int r,int w,int a) {
    if (r<=(p[12]-1)) {
        *(p[10]+r)=w;
        *(p[11]+r)=a;
    }
}

Ctable(int x,int y,int w,int h,int r) {
    int i;
    pointer p,c,a;
    p=Cip(13,x,y,w,h);
    c=malloct("i",r);
    a=malloct("i",r);
    p[0]=9;p[4]=h*11;p[5]=0;p[6]=-1;p[7]=0;p[8]=0;p[9]=0;p[10]=c;p[11]=a;p[12]=r;
    for (i=0;i<r;i++) {
        *(p[10]+i)=w/r;
        *(p[11]+i)=0;
    }
    return p;
}

CtableDB(int x,int y,int w,int h,int r,pointer g,string d) {
    int i;
    pointer p,c,f,a;
    p=Cip(14,x,y,w,h);
    c=malloct("i",r);
    f=malloct("s",1);
    a=malloct("i",r);
    p[0]=13;p[4]=h*11;p[5]=0;p[6]=-1;p[7]=g;p[8]=dbnrecs();
    p[9]=0;p[10]=c;p[11]=a; p[12]=r;p[13]=f;*p[13]=d;
    for (i=0;i<r;i++) {
        *(p[10]+i)=w/r;
        *(p[11]+i)=0;}
    return p;
}

Cframe(int x,int y,int w,int h) {
    pointer p,c;
    p=Cip(6,x,y,w,h);
    c=malloct("s",1);
    p[0]=14;p[5]=c;
    return p;
}

Csetfield(pointer p,int r,string s) {
    if (r<=(p[12]-1))
    *(p[7]+(p[12]*p[6])+r)=s;
}

Cgetfield(pointer p,int r) {
    if (r<=(p[12]-1))
    return *(p[7]+(p[12]*p[6])+r);
}

Csetcontent(pointer p,string s) {
    if (p[0]==9)
        *(p[7]+(p[6]*p[12]))=s;
    else if ((p[0]>6)&&(p[0]<14))
        *(p[7]+p[6])=s;
    else if (p[0]==6)
        *p[1]=s;
    else *p[5]=s;
}

Cgetcontent(pointer p) {
    if (p[0]==9)
        return *(p[7]+(p[6]*p[12]));
    else if ((p[0]>6)&&(p[0]<14))
        return *(p[7]+p[6]);
    else if (p[0]==6)
        return *p[1];
    else
        return *p[5];
}

Cgetcount(pointer p) {
    if ((p[0]>6)&&(p[0]<14))
        return p[8];
    else if (p[0]==6)
        return p[3];
    else
        return 0;
}
