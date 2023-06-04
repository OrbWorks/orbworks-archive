/$ Ccontrols1.c
Cc(pointer p) {
    int x,y,h;
    x=penx(); y=peny();
    
    if ((p[0]==10)||(p[0]==14))
        h=11;
    else
        h=p[4];
    return ((x>p[1]) && (x<(p[1]+p[3])) && (y>p[2]) && (y<(p[2]+h)));
}

Ct(int x,int y,int w,int h,int a,string s) {
    int t;
    textalign(a);
    rect(0,x,y,x+w,y+h,0);
    if (a==2)
        t=x+w;
    else if (a==1)
        t=x+(w/2);
    else t=x+1;
    text(t,y,s);
    textalign(0);
}

Cet(pointer p) {
    textattr(0,1,0);
    Ct(p[1],p[2],p[3],p[4],p[8],*p[5]);
}

Cib(pointer p) {
    rect(3,p[1],p[2],p[1]+p[3],p[2]+p[4],p[7]);
}

Chb(pointer p) {
    int h;
    h=1;
    Cib(p);
    do {
        if (!Cc(p)&&h) {
            h=0;
            Cib(p);
        } else if (Cc(p)&&!h) {
            h=1;
            Cib(p);
        }
    } while (event(1)!=3);
    
    if (h==1) {
        if (p[0]==3) {
            p[8]=!(p[8]);
            h=1;
        }
        else
            Cib(p);
    }
    return h;
}

Cir(pointer p) {
    int x,w;
    w=p[3]/p[8];
    x=p[1]+(p[6]*w);
    rect(3,x,p[2],x+w-1,p[2]+p[4],0);
}

Cdr(pointer p);
Chr(pointer p) {
    int ic;
    do {
        ic=(penx()-p[1])/(p[3]/p[8]);
        if ((ic!=p[6])&&(ic<p[8])&&Cc(p)) {
            if (!useColor && p[6]!=-1)
                Cir(p);
            p[6]=ic;
            if (useColor) Cdr(p);
            else Cir(p);
        }
    } while (event(1)!=3);

    return (p[6]!=-1);
}

Che(pointer p) {
    int r;
    string s,m;
    r=Chb(p);
    if (strlen(*p[10])>0)
        m=*p[10];
    else
        m=*p[5];
    if (r) {
        s=getsi(p[1]-1, p[2]-1, p[3], *p[5]);
        if (s)*p[5]=s;
            Cet(p);
    }
    return r;
}

Cck(pointer p) {
    if (p[6])
        bitmap(p[1],p[2]+1,"0c0067ee41c5b85f04e84484087f8");
    else
        bitmap(p[1],p[2]+1,"0c0007f84084084084084084087f8");
}

Chc(pointer p) {
    if (Cc(p)) {
        p[6]=!(p[6]);
        Cck(p);
        return 1;
    }
    else
        return 0;
}

Cilbi(pointer p) {
    int y,n;
    n=p[4]/11;
    if ((p[6]>=p[5]) && (p[6]<(p[5]+n))) {
        y=p[2]+((p[6]-p[5])*11);
        rect(3,p[1],y,p[1]+p[3],y+11,0);
    }
}

Cdlbi(pointer p) {
    int i,l,x,y,n,r;
    n=p[4]/11;
    if (p[0]==13)
        p[8]=dbnrecs();
    pushdraw();
    setbgi(curFill);
    setfgi(curFrame);
    settextcolori(curFG);
    for (i=p[5];(i<p[5]+n)&&(i<p[8]);i++) {
        y=p[2]+((i-p[5])*11);
        if (p[6]==i) {
            pushdraw();
            setbgi(curSelFill);
            settextcolori(curSelFG);
        }
        if ((p[0]==9)||(p[0]==13)) {
            x=p[1];
            for (r=0;r<p[12];r++) {
                if (p[0]==9)
                    Ct(x,y,*(p[10]+r)-1,11,*(p[11]+r),*(p[7]+(p[12]*i)+r));
                else {
                    dbrec(i);
                    dbreadx(p[7],*p[13]);
                    Ct(x,y,*(p[10]+r)-1,11,*(p[11]+r),(string)*(p[7]+r));
                }
                x=x+*(p[10]+r);
            }
            x--;
            rect(0,x,y,p[3]+p[1],y+11,0);
        } else
            Ct(p[1],y,p[3],11,0,*(p[7]+i));
        if (p[6]==i)
            popdraw();
    }
    for (l=(i-p[5]);l<n;l++) {
        y=p[2]+(l*11);
        if (p[0]==9) {
            x=p[1];
            for (r=0;r<p[12];r++) {
                rect(0,x,y,x+*(p[10]+r)-1,y+11,0);
                x=x+*(p[10]+r);
            }
        } else
            rect(0,p[1],y,p[1]+p[3],y+11,0);
    }
    setfgi(curFG);
    if (p[6]==p[5]) {
        pushdraw();
        setbgi(curSelFill);
        setfgi(curSelFG);
    }
    if (p[5]>0)
        bitmap(p[1]+p[3]-8,p[2]+1,"0710387cfe383838");
    else
        rect(0,p[1]+p[3]-8,p[2]+1,p[1]+p[3]-1,p[2]+10,0);
    if (p[6]==p[5])
        popdraw();
    if (p[6]==p[5]+n-1) {
        setbgi(curSelFill);
        setfgi(curSelFG);
    }
    if ((p[5]+n)<p[8])
        bitmap(p[1]+p[3]-8,p[2]+p[4]-8,"07383838fe7c3810");
    else
        rect(0,p[1]+p[3]-8,p[2]+p[4]-8,p[1]+p[3]-1,p[2]+p[4],0);
    popdraw();
    if (!useColor) Cilbi(p);
}

Csi(pointer p, pointer s, int d) {
    int n;
    n=(p[4]/11)*d;
    if (((d>0)&&((p[5]+n)<p[8])) || ((d<0)&&(p[5]>0))) {
        if (Chb(s)) {
            if (!useColor) Cilbi(p);
            p[5]=p[5]+n;
            if (p[5]>(p[8]-n))
                p[5]=p[8]-n;
            if (p[5]<0)
                p[5]=0;
            Cdlbi(p);
        }
    }
}
