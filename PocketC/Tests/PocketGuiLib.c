/$ PocketGuiLib
// Copyright ©1998, Klaus Rindtorff

string GT[50],Gb[2],Gn;
int Gt[50],GS[50],GI[50],GO[50],Gx[50],Gy[50],GX[50],GY[50],Gm,Go,GDebug,GGrid;

GItem(int i,int j)
{int b,e;
 b=strstr(GT[i],(char)j,0); e=strstr(GT[i],(char)(j+1),0);
 if (b!=e) return substr(GT[i],b+1,e-b-1);
 return "";
}

GRemove(int i,int j)
{int c,n; string s;
 if (j>=GI[i]) return; s=""; c=1;
 for (n=0;n<GI[i];n++) if (n!=j) s=s+GItem(i,n)+(char)c++;
 GT[i]=s; if (j<GS[i] || GS[i]==GI[i]-1) {GS[i]--;} GI[i]--;
}

GClear(int i) {GT[i]=""; GI[i]=0; GO[i]=0; GS[i]=0;}

GAdd(int i,string s)
{int b,e,n; string t;
 s=s+","; b=0; e=strstr(s,",",0);
 t=GT[i]; n=GI[i];
 while (b<e&&n<31)
 {t=t+substr(s,b,e-b)+(char)(++n); b=e+1; e=strstr(s,",",b);}
 GT[i]=t; GI[i]=n;
}

GShow(int i)
{int j,r,o,t,x,y,X,Y,xl,xr,yc; string s;
 t=Gt[i]; o=GO[i]; r=GI[i]; x=Gx[i]; y=Gy[i]; X=GX[i];
 if (t>10) {Y=y+r*12; while (Y>160) {Y=Y-12; r--;} } else Y=GY[i];
 rect(0,x,y,X,Y,0); frame(1,x,y,X,Y,(t>10));
 if (t>10) {line(1,X+1,y+1,X+1,Y-2); line(1,x+1,Y+1,X-2,Y+1);}
 if (t==10) {textalign(11); xl=x; yc=(y+Y)/2-1;}
 else {r=(Y-y+1)/12; textalign(0); yc=y;}
 for (j=o;j<o+r;j++)
 {s=GItem(i,j); if (s=="") break;
  if (t==10)
  {xr=x+((X-x+1)*(j+1))/r-1;
   if (j+1<GI[i]) line(1,xr,y,xr,Y);
   text((xr+xl+1)/2,yc,s); xl=xr;
  }
  else {text(x+1,yc,s); yc=yc+12;}
 }
 if (t==10)
 {o=(X-x+1)*GS[i]; rect(3,x+o/r,y,x+(o+(X-x+1))/r-1,Y,0);}
 else
 {if (o>0) bitmap(X-8,y+1,"0810387cfe383838");
  if (o+r<GI[i]) bitmap(X-8,Y-8,"08383838fe7c3810");
  if (GS[i]>=o) if(GS[i]<o+r) {y=y+(GS[i]-o)*12; rect(3,x,y,X,y+11,0);}
 }
}

GPaint(int i)
{int t,x,y,X,Y,c,r; string s;
 t=Gt[i]; x=Gx[i]; y=Gy[i]; X=GX[i]; Y=GY[i]; c=(y+Y)/2-1;
 textattr(0,1,0); textalign(10);
 if(t==0) frame2(1,x+3,y+3,X-3,Y-3,4,1); else rect(0,x,y,X,Y,0);
 if(t==1) text(x,c,GT[i]);
 else if(t==2) {textalign(12); text(X,c,GT[i]);}
 else if(t==3) {text(x+1,c,GT[i]); line(2,x,Y,X,Y);}
 else if(t==4) {frame(2-(GS[i]!=0),x,y,X,Y,4); textalign(11); text((x+X)/2,c,GT[i]);}
 else if(t==5) {bitmap(x,y,Gb[GS[i]!=0]); text(x+15,c,GT[i]);}
 else if(t==6)
 {GT[i]=(string)GS[i];
  textalign(12); text(X-11,c,GT[i]); line(2,x,Y,X-11,Y);
  bitmap(X-9,y,"090801c03e07f0ff8000ff87f03e01c0080");
 }
 else if(t==7) bitmap(x,y,GT[i]);
 else if(t==8)
 {frame(2,x,y,X,Y,0);
  text(x+1,c,GItem(i,GS[i]));
  bitmap(X-11,y+1,"0b0a01b03b87bcfbe7bc3b81b00a0");
 }
 else if(t==9 || t==10) GShow(i);
 else if(t==11)
 {if (Go<0) {bitmap(x,y,"0800000000ff7e3c18"); text(x+12,c,GItem(i,GS[i]));}
  else {GShow(i);}
 }
 else if(t==12)
 {textattr(1,1,0);
  if (Go<0) {text(x+1,y+4,GItem(i,0));}
  else {GS[i]=0; GO[i]=0; GShow(i);}
 }
}

GCreate(int i,int t,string s,int x,int y,int w,int h)
{if (i<0 || i>=Gm) return;
 Gt[i]=t; GI[i]=0; GO[i]=0; GT[i]="";
 Gx[i]=x*6; GX[i]=(x+w)*6-1; Gy[i]=16+y*6;
 if (y<0) {Gy[i]=2; GY[i]=13;} else GY[i]=15+(y+h)*6;
 if (t==6) GS[i]=(int)s; else GS[i]=(t==4);
 if (t>=8&&t<=13) GAdd(i,s); else GT[i]=s;
}

GSave(string name,int b,int e)
{int i,c;
 name="PocketGui "+name; if (!dbcreate(name)) return 0; c=0;
 for (i=b;i<=e;i++)
 {if (Gt[i]<0) continue;
  c++; dbwrite((int)i); dbwrite((int)Gt[i]); dbwrite((string)GT[i]);
  dbwrite((int)GI[i]); dbwrite((int)GS[i]); dbwrite((int)GO[i]);
  dbwrite((int)Gx[i]); dbwrite((int)Gy[i]); dbwrite((int)GX[i]); dbwrite((int)GY[i]);
 }
 dbclose(); return c;
}

GLoad(string name,int b,int e)
{string T; int i,t,I,S,O,x,y,X,Y,c;
 if (!dbopen("PocketGui "+name)) return 0; c=0;
 while (dbpos()>=0)
 {i=dbread('i'); t=dbread('i'); T=dbread('s'); I=dbread('i'); S=dbread('i'); O=dbread('i');
  x=dbread('i'); y=dbread('i'); X=dbread('i'); Y=dbread('i');
  if(i>=0) if(i<Gm) if(b<=i) if(i<=e)
  {Gt[i]=t; GT[i]=T; GI[i]=I; GS[i]=S; GO[i]=O; Gx[i]=x; Gy[i]=y; GX[i]=X; GY[i]=Y; c++;}
 } dbclose(); return c;
}

GSet(int i,string t) {if(Gt[i]<8) {GT[i]=t; GPaint(i);}}
GGet(int i) {if(Gt[i]<8) return GT[i]; return GItem(i,GS[i]);}
