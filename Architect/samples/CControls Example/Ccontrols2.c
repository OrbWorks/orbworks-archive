/$ Ccontrols2.c
Chl2(pointer p) {
    int ic,s1[8],s2[8];
    s1[0]=12; s1[1]=p[1]+p[3]-10; s1[2]=p[2]+p[4]-10; s1[3]=10; s1[4]=10; s1[7]=0;
    s2[0]=12; s2[1]=p[1]+p[3]-10; s2[2]=p[2]; s2[3]=10; s2[4]=10; s2[7]=0;
    
    do {
        if (Cc(s1)) {
            Csi(p,s1,1);
            return 0;
        }
        else if (Cc(s2)) {
            Csi(p,s2,-1);
            return 0;
        } else {
            ic=((peny()-p[2])/11)+p[5];
            if ((ic!=p[6])&&(ic<p[8])&& Cc(p)) {
                if (!useColor && p[6]!=-1)
                    Cilbi(p);
                p[6]=ic;
                if (useColor) Cdlbi(p);
                else Cilbi(p);
            }
        }
    } while (pcevent(1)!=3);
    return (p[6]!=-1);
}

Chl(pointer p) {
    memcpy(&curFrame, &clrObjFrame, 5);
    Chl2(p);
}

Cdrp(pointer p) {
    int t,x,y,r;
    t=0;r=0;x=p[1];y=p[2];
    
    if (p[0]==8)
        p[2]=14;
    if (x+p[3]>158)
        p[1]=158-p[3];
    if (y+p[4]>158)
        p[2]=158-p[4];
    if (p[0]==8)
        rect(0,p[1]-2,p[2]+1,p[1]+p[3]+3,p[2]+p[4]+3,0);
    else
        rect(0,p[1]-2,p[2]-2,p[1]+p[3]+3,p[2]+p[4]+3,0);
    frame(1,p[1],p[2],p[1]+p[3],p[2]+p[4],1);
    line(1,p[1]+1,p[2]+p[4]+1,p[1]+p[3]-2,p[2]+p[4]+1);
    line(1,p[1]+p[3]+1,p[2]+1,p[1]+p[3]+1,p[2]+p[4]-2);
    p[5]=0;
    Cdlbi(p);
    if (p[0]==10) {
        t=1;
        p[0]=11;
    }
    while (1) {
        if (pcevent(1)==2)
            if (Cc(p)) {
                if (Chl2(p)) {
                    r=1;
                    break;
                }
            } else {
                r=0;
                break;
            }
    }
    if (t)
        p[0]=10;
    p[1]=x;p[2]=y;
    return r;
}

Cdrt(pointer p) {
    pushdraw();
    setbgi(clrFormFill);
    settextcolori(clrObjFG);
    rect(0,p[1]+11,p[2],p[1]+p[3],p[2]+11,0);
    textattr(0,1,0);
    textalign(0);
    text(p[1]+11,p[2],*(p[7]+p[6]));
    popdraw();
}

Chd(pointer p) {
    int r,i,t[8];
    r=0;
    saveg();
    for (i=0;i<8;i++)
        t[i]=p[i];
    t[4]=11;
    if (Cc(t))
        if (Chb(t))
            r=Cdrp(p);
    restoreg();
    if (r)
        Cdrt(p);
    return r;
}

Csettopic(pointer p, string s) {
    *p[10]=s;
}

Cgettopic(pointer p) {
    return *p[10];
}

Cdm(pointer p) {
    int i;
    pointer t;
    pushdraw();
    setbgi(clrMenuFill);
    setfgi(clrMenuFrame);
    settextcolori(clrMenuFG);
    rect(0,0,0,160,15,0);
    frame(1,1,1,158,13,1);
    line(1,2,14,156,14);
    line(1,159,2,159,11);
    for (i=0;i<p[3];i++) {
        t=*(p[2]+i);
        text(t[1]+3,1,*t[10]);
    }
    popdraw();
}

Cmevent(pointer p) {
    int i,e,c,r,t[8],m[8];
    pointer z;
    c=1;r=0;e=0; t[2]=0;t[4]=12;
    m[1]=0;m[2]=0;m[3]=160;m[4]=12;
    saveg();
    textattr(1,1,0);
    Cdm(p);
    while (c) {
        e=pcevent(1);
        if (e==2) {
            for (i=0;i<p[3];i++) {
                z=*(p[2]+i);z[6]=-1;
                t[1]=z[1];t[3]=z[11];
                if (Cc(t)) {
                    if (useColor) {
                        pushdraw();
                        setfgi(clrMenuFrame);
                        setbgi(clrMenuSelFill);
                        settextcolori(clrMenuSelFG);
                        rect(0,t[1],1,t[1]+t[3],13,0);
                        text(z[1]+3,1,*z[10]);
                        popdraw();
                        memcpy(&curFrame, &clrMenuFrame, 5);
                    } else {
                        rect(3,t[1],1,t[1]+t[3],13,0);
                    }
                    r=Cdrp(z); c=!r; i=p[3];
                    if (!r) {
                        if (!Cc(m))
                            c=0;
                        else {
                            restoreg();
                            saveg();
                            Cdm(p);
                            i=-1;
                        }
                    }
                    else {
                        *p[1]=*(z[7]+z[6]);
                        r=1;
                        c=0;
                    }
                } else if (!Cc(m))
                c=0;
            }
        } else if (e==11)
            c=0;
    }
    restoreg();
    textattr(0,1,0);
    return r;
}

int Cevent(pointer p,int e) {
    if (p<=0)
        return 0;
    if (e==2) {
        while (Cc(p)) {
            if (((p[0]>0)&& (p[0]<4))||(p[0]==15)) return Chb(p);
            else if ((p[0]==12)||(p[0]==9)||(p[0]==13)) return Chl(p);
            else if (p[0]==10) return Chd(p);
            else if (p[0]==5) return Chc(p);
            else if (p[0]==4) return Che(p);
            else if (p[0]==7) return Chr(p);
            else return 0;
        }
    } else if (e==11)
        if (p[0]==6)
            return Cmevent(p);
    return 0;
}
