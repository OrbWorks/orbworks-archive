// CControls example
#include "Ccontrols.c"

// controls
Chandle hl1,he1,hlb1,ht1,hm1,hm2,hmf,hb1,hb2,hi1,hd1,hp1,hs1,hc1,hr1;
pointer DB;

string rotator[4]={
"0c0f033c43e43e83f83ffc1fc17c27c23cc0f0",
"0c0f030c402606f0ffdffbff0f60640230c0f0",
"0c0f03cc7c27c2fc1fc183f83f43e43e33c0f0",
"0c0f03fc5fa4f28618418218614f25fa3fc0f0"
};
int iRot;
int isRot;

initcontrols(){
int i,r;
// button 1
hb1=Cbutton(1,147,35,0,1,4);
Csetcontent(hb1,"Info");
// button 2
hb2=Cbutton(40,147,35,0,1,4);
Csetcontent(hb2,"Quit");
// image-button 1
hi1=Cimage(140,18,12,12);
Csetcontent(hi1,"0c1f83fc7fee67e67ffffffdfbef770e3fc1f8");
// listbox 1
hl1=Clistbox(1,18,90,3);
for(i=1;i<=10;i++) Cadditem(hl1,"Line number  "+i);
Csetcursel(hl1,1);
// dropdown 1
hd1=Cdropdown(95,36,60,5);
for(i=1;i<=10;i++) Cadditem(hd1,"Dropline "+i);
// popup 1
hp1=Cdropdown(14,23,70,5);
for(i=1;i<=8;i++) Cadditem(hp1,"Popline "+i);
// switch 1
hs1=Cswitch(95,18,20,12,1,0);
Csetcontent(hs1,"Off");
// checkbox 1
hc1=Ccheckbox(95,113,40);
Csetcontent(hc1,"active");
Csetstate(hc1,1);
// editfield 1
he1=Cedit(1,114,65,2,2,2);
Csettopic(he1,"Enter text:");
Csetcontent(he1,"edit me");
// label 1
hlb1=Clabel(1,129,65,0,2,1);
Csetcontent(hlb1,"click here!");
// radio 1
hr1=Cradio(95,128,60,0);
for(i=1;i<=3;i++) Cadditem(hr1,"Do"+i);
// menu-topic 1
hm1=Cmenu(5,60,40);
Csettopic(hm1,"Applet");
for(i=1;i<=5;i++) Cadditem(hm1,"Item"+i);
// menu-topic 2
hm2=Cmenu(45,60,45);
Csettopic(hm2,"Options");
for(i=1;i<=7;i++) Cadditem(hm2,"Item "+i);
Csetcursel(hm2,1);
Csetcontent(hm2,"TestX");
// menu-bar
hmf=Cmenubar();
Caddmenu(hmf,hm1);
Caddmenu(hmf,hm2);
}

drawscreen(){
clearg();
title("CControl Test");
// draw controls
Cdraw(hb1); Cdraw(hb2);
Cdraw(hl1); Cdraw(hi1);
Cdraw(hd1); Cdraw(hs1);
Cdraw(hc1); Cdraw(he1);
Cdraw(hlb1); Cdraw(hr1);
Cdraw(ht1);
}

dialog(){
Chandle hf1,he1,hb1; int e;
// dialog frame
hf1=Cframe(10,40,140,70);
Csetcontent(hf1,"Some Dialog");
// editfield 1
he1=Cedit(20,60,70,2,1,0);
Csetcontent(he1,"dialog field");
// button 1
hb1=Cbutton(65,90,30,12,1,4);
Csetcontent(hb1,"Ok");
// draw dialog
Cdraw(hf1); Cdraw(he1); Cdraw(hb1);
// message loop
while(1){
    e=event(1);
    if(Cevent(he1,e));
    else if(Cevent(hb1,e))
        break;
}
}

createDB(){
int i,r; pointer p;
p=malloct(3,"s");
if(!dbcreate("CTestDB")){alert("\nUnable to create CTestDB.");return;}
for(i=0;i<10;i++){
    for(r=0;r<3;r++) p[r]="DB "+(i+1)+"/"+(r+1);
    dbrec(-1); dbwritex(p,"szszsz");
}
}

initDB(){
DB=malloct(3,"s");
// create database
if(!dbopen("CTestDB")) createDB();
// create database-table
ht1=CtableDB(1,55,158,5,3,DB,"szszsz");
Csetrow(ht1,0,45,0);
Csetrow(ht1,1,55,2);
Csetrow(ht1,2,48,1);
}

// actions on controlevents:
onbutton1(){
alert("Listbox:"+Cgetcontent(hl1)+"\nSwitch:"+Cgetstate(hs1)+"\nRadio:"+Cgetcursel(hr1));
}
onlabel1(){
dialog(); drawscreen();
}
onimage1(){
if(Cpopupevent(hp1)) alert("Popup:\n"+Cgetcontent(hp1));
}
onswitch1(){
if(Cgetstate(hs1)){
    isRot=1;iRot=0;
    Csetcontent(hi1,*rotator);
    Csetcontent(hs1,"On");
}else{
    isRot=0;
    Csetcontent(hi1,"0c1f83fc7fee67e67ffffffdfbef770e3fc1f8");
    Csetcontent(hs1,"Off");
}
Cdraw(hi1);
Cdraw(hs1);
}
oncheckbox1(){
if(Cgetstate(hc1)) Cactivate(hb2);
else Cdeactivate(hb2);
}
onmenu(string s){
alert(s);
}

main(){
int e,t;
// initialize screen
graph_on();
title("Please wait....");
Cinit();
initDB();
initcontrols();
drawscreen();

// message loop
while(1){
e=event(10);
if(isRot){
    Csetcontent(hi1,rotator[iRot]);
    Cdraw(hi1);
    iRot=(iRot+1)%4;
}
if(e == 23) drawscreen();
else if(Cevent(hb1,e)) onbutton1();
else if(Cevent(hb2,e)) break;
else if(Cevent(hl1,e));
else if(Cevent(hd1,e));
else if(Cevent(hi1,e)) onimage1();
else if(Cevent(hs1,e)) onswitch1();
else if(Cevent(hc1,e)) oncheckbox1();
else if(Cevent(he1,e));
else if(Cevent(hlb1,e)) onlabel1();
else if(Cevent(hmf,e)) onmenu(Cgetcontent(hmf));
else if(Cevent(hr1,e));
else if(Cevent(ht1,e));
}
dbclose();
}
