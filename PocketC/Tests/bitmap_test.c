//bitmap_test.c
library "PToolboxLib"
main() {

int w, x, y, z;
string a,b,c;
string d[16] =
{"0","1","2","3","4","5","6","7","8","9","A","B","C","D","E","F"};

SetDepth(2);
graph_on();
SetFore(2);
clear();
for(x=1;x<=160;x++) {
 if(x%4) w=(x/4)+1;
 else w=(x/4);
 b=d[x/16]+d[x%16];
 a=b;

 for(y=1;y<=160;y++) {
   for(z=0;z<w;z++) a=a+"f";
   bitmap(0,0,a);
 }
}
event(1);
graph_off();
SetDepth(0);
}
