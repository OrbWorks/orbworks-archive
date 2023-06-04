/$ PocketGui
// Copyright ©1998, Klaus Rindtorff

GFrame(int i,int x,int y,int w,int h) {GCreate(i,0,"",x,y,w,h);}
GText(int i,string s,int x,int y,int w) {GCreate(i,1,s,x,y,w,2);}
GLabel(int i,string s,int x,int y,int w) {GCreate(i,2,s,x,y,w,2);}
GEntry(int i,string s,int x,int y,int w) {GCreate(i,3,s,x,y,w,2);}
GButton(int i,string s,int x,int y,int w) {GCreate(i,4,s,x,y,w,2);}
GCheck(int i,string s,int x,int y,int w) {GCreate(i,5,s,x,y,w,2);}
GCount(int i,string s,int x,int y,int w) {GCreate(i,6,s,x,y,w,2);}
GBits(int i,string s,int x,int y,int w, int h) {GCreate(i,7,s,x,y,w,h);}
GRing(int i,string s,int x,int y,int w) {GCreate(i,8,s,x,y,w,2);}
GList(int i,string s,int x,int y,int w,int n) {GCreate(i,9,s,x,y,w,n*2);}
GRadio(int i,string s,int x,int y,int w) {GCreate(i,10,s,x,y,w,2);}
GDrop(int i,string s,int x,int y,int w) {GCreate(i,11,s,x,y,w,2);}
GMenu(int i,string s,int x,int w) {GCreate(i,12,s,x,-1,w,2);}
GPopUp(int i,string s) {GCreate(i,13,s,0,0,0,0);}
GState(int i) {return GS[i];}
GItems(int i) {return GI[i];}
GFlash(int x,int y,int X,int Y,int r) {rect(3,x,y,X,Y,r); sleep(100); rect(3,x,y,X,Y,r);}

GRefresh()
{int i; clearg(); title(Gn);
 if(GGrid) for(i=15;i<160;i=i+6) bitmap(0,i,"A00410410410410410410410410410410410410410");
 for(i=0;i<Gm;i++) if(Gt[i]>=0) if(Gt[i]<=12) GPaint(i);
}

GInit(string t)
{graph_on(); clearg(); title(t);
 Gn=t; Go=-1; GGrid=0; GDebug=0;
 for(Gm=0; Gm<50; Gm++) Gt[Gm]=-1; Gm=50;
 Gb[0]="0c0007f84084084084084084087f8"; Gb[1]="0c0067ee41c5b85f04e84484087f8";
}

GSelect(int i,int j)
{if(j<GI[i] || (Gt[i]>=4&&Gt[i]<=6)) {GS[i]=j; if(Gt[i]<12) GPaint(i);}}

GScroll(int i,int px, int py,int x,int y, int X, int o, int r)
{if(X-px<8&&((py-y<12&&o>0) || (y+r*12-1-py<12&&o+r<GI[i])))
 {if(y+r*12-1-py<12) {o=o+r; if(o+r>GI[i]) o=GI[i]-r;}
  else {o=o-r; if(o<0) o=0;}
  GO[i]=o; GPaint(i); return 1; }
 return 0;
}

GFind(int px,int py)
{int i;
 for(i=0;i<Gm;i++)
  if(Gt[i]>0)
  {if(px>=Gx[i]) if(px<=GX[i]) if(py>=Gy[i]) if(py<=GY[i]) break;}
  else if(Gt[i]==0)
  {if(px>=Gx[i]) if(px<Gx[i]+6) if(py>=Gy[i]) if(py<Gy[i]+6) break;}
 return i;
}

GEvent()
{int i,t,e,px,py,x,y,X,Y,r,o; string s;
 e=event(1);
 if(e==2)
 {px=penx(); py=peny();
  if(Go>=0) i=Go; else i=GFind(px,py);
  if(i<Gm){t=Gt[i]; x=Gx[i]; y=Gy[i]; X=GX[i]; Y=GY[i];}
  if(Go>=0)
  {o=GO[i]; r=GI[i]; while(y+r*12>160) r--;
   if(px>=x&&px<=X&&py>=y&&py<=y+r*12-1)
   {if(GScroll(i,px,py,x,y,X,o,r)) return -1;
    r=y+12*((py-y)/12); rect(3,x,r,X,r+11,0);
    beep(7); GS[i]=o+(py-y)/12;
   } else {beep(2); i=-1;}
   Go=-1; restoreg(); if(i>=0) if(t!=13) GPaint(i); return i;
  }
  else if(i<Gm)
  {if(GDebug) {beep(7); GFlash(x,y,X,Y,0); return i;}
   if(t>2) if(t!=9) beep(7);
   if(t==3) {s=gets(GT[i]); if(s) GSet(i,s);}
   else if(t==4) {if(GS[i]) GFlash(x,y,X,Y,4); else i=-1;}
   else if(t==5) {GS[i]=!GS[i]; GPaint(i);}
   else if(t==6)
   {if(X-px<9)
    {if(py-y<6) GS[i]++; else {GS[i]--; y=y+6;}
     GFlash(X-9,y,X,y+5,0);
    } else {s=gets(GT[i]); if(s)GS[i]=(int)s;}
    GPaint(i);
   }
   else if(t==7) GFlash(x,y,X,Y,0);
   else if(t==8)
   {if(X-px<11)
    {if(X-px<6) GS[i]++; else {GS[i]--; X=X-6;}
     if(GS[i]<0) GS[i]=GI[i]-1; else if(GS[i]>=GI[i]) GS[i]=0;
     GFlash(X-5,y,X,Y,0); GPaint(i);
    }
   }
   else if(t==9)
   {r=(Y-y+1)/12; o=GO[i];
    if(GScroll(i,px,py,x,y,X,o,r) || (py-y)/12>=GI[i]-o) return -1;
    beep(7); if(GS[i]>=o) if(GS[i]<o+r) {r=y+(GS[i]-o)*12; rect(3,x,r,X,r+11,0);}
    GS[i]=o+(py-y)/12; r=y+(GS[i]-o)*12; rect(3,x,r,X,r+11,0);
   }
   else if(t==10)
   {r=(X-x+1)*GS[i]; o=GI[i]; rect(3,x+r/o,y,x+(r+(X-x+1))/o-1,Y,0);
    GS[i]=((px-x)*o)/(X-x+1); r=(X-x+1)*GS[i]; rect(3,x+r/o,y,x+(r+(X-x+1))/o-1,Y,0);
   }
   else if(t==11||t==12) {saveg(); Go=i; GPaint(i); i=-1;}
   return i;
  }
 }
 return -1;
}

GPop(int i, int x, int y, int w)
{int e;
 if(Gt[i]==13)
 {saveg(); Go=i; GS[i]=0; GO[i]=0; Gx[i]=x*6; GX[i]=(x+w)*6-1;
  if(y<0) Gy[i]=2; else Gy[i]=y*6+16; textattr(0,1,0); GShow(i);
  do {e=GEvent();} while(Go>=0); if(e==i) return GS[i];
 }
 return -1;
}
