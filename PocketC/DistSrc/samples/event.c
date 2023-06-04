// Event sample

string prevString;
out(string str) {
   textattr(0,0,0);
   text(80,1,prevString);
   textattr(0,1,0);
   text(80,1,str);
   prevString = str;
}

// Handle a key event
handlekey() {
   char k;

   k = key();
   out("KeyEvent: '"+k+"' + [" + (int)k + "]");
}

int x, y;
// Handle a pen event
handlepen(int e) {
   if (e==2) {
      x = penx();
      y = peny();
   } else if (e==4)
      line(1, x, y, x=penx(), y=peny());
   else out("PenUp: ("+penx()+", "+peny()+")");
}

// Handle a button event
handleb(int e) {
   if (e==5) out("PageUpEvent");
   else if (e==6) out("PageDnEvent");
   else if (e==11) out("MenuEvent");
   else out("HardButton" + (e-6));
   if (e==11) exit();
}

main() {
   int e;

   clear();
   graph_on();
   title("Event Sample");
   hookhard(1);  // intercept hard buttons
   hookmenu(1);  // intercept the menu silkscreen button
   while (1) {
      e = event(1);
      if (e==1) handlekey();
      else if (e>=2 && e<=4) handlepen(e);
      else handleb(e);
   }
}
