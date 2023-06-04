// Tic-Tac-Toe
int brd[9]; // The board

drawx(int x, int y) {
   // Draw an 'x' at grid location (x,y)
   x = x * 40 + 20;
   y = y * 40 + 27;
   line(1, x+3, y+3, x+37, y+37);
   line(1, x+3, y+37, x+37, y+3);
}

drawo(int x, int y) {
   // Draw an 'o' at grid location (x,y)
   x = x * 40 + 20;
   y = y * 40 + 27;
   frame(1, x+3, y+3, x+37, y+37, 17);
}

verify(int a, int b, int c) {
   // verify that locations a,b,c contain the same pieces
   char chk;

   chk = brd[a] + brd[b]*4 + brd[c]*16;
   if (chk == 21 || chk == 42) return 1;
   return 0;
}

winner() {
   // check along each line/diagonal
   if (verify(0,1,2)) return 1;
   if (verify(3,4,5)) return 2;
   if (verify(6,7,8)) return 3;
   if (verify(0,3,6)) return 4;
   if (verify(1,4,7)) return 5;
   if (verify(2,5,8)) return 6;
   if (verify(0,4,8)) return 7;
   if (verify(2,4,6)) return 8;
   return 0;
}

human() {
   int x, y;
   while(1) {
      // Wait for a pen up event
      waitp();
      x = penx();
      y = peny();
      // Did the user tap outside the grid?
      if (x<20 || x>140 || y<27 || y>147)
         return -1;

      // Get grid coordinates
      x = (x-20)/40;
      y = (y-27)/40;

      if (brd[x+3*y] == 0) {
         // mark the space as used
         brd[x+3*y] = 1;
         drawx(x,y);
         if (winner()) return 1;
         else return 0;
      }
   }
}

#ifndef DUMB
// "AI" provided by Thaddaeus Frogley <codemonkey_uk@hotmail.com>
cpu() {
  int loc;
  int a,b,c;

  loc=-1;
  b=1;
  c=0;
  for(a=0;a<9;a++){
    if ( brd[a]==0){

      if (loc==-1){
        loc=a;
      }
      else if (c==0 && (random(b)==0 || a==4)){
        loc=a;
        b++;
        if (a==4) c=1;
      }

      brd[a] = 2;
      if (winner()){
        loc=a;
        break;
      }
      else{
        brd[a] = 1;
        if (winner()){ 
          loc=a;
          c=1;
        }
      }
      brd[a] =0;

    }
  }

  drawo(loc%3, loc/3);
  brd[loc] = 2;

   if (winner()) return 1;
   else return 0;
}
#else

cpu() {
   int loc;

   // Find an empty space
   do {
      loc = random(9);
   } while (brd[loc] != 0);

   drawo(loc%3, loc/3);
   brd[loc] = 2;

   if (winner()) return 1;
   else return 0;
}
#endif

main() {
   int numMoves, win;

   numMoves = win = 0;
   clear();
   puts("Tic-Tac-Toe: A PocketC demo\n\nTap outside the grid to exit.\nTap anywhere to start...\n");
   wait();
   graph_on();
   title("Tic-Tac-Toe");

   // Draw the board
   line(1,60,27,60,147);
   line(1,100,27,100,147);
   line(1,20,67,140,67);
   line(1,20,107,140,107);

   // Draw the gray border
   frame(2, 20, 27, 140, 147, 0);

   while (1) {
      // we are done when either the human or the cpu wins
      // or when the board is full
      win=human();
      if (win==-1) return; // Exit
      if (win) {
         text(56,82,"Human wins!");
         break;
      }
      numMoves++;
      if (numMoves>8) {
         text(68,82,"Draw!");
         break;
      }
      if (cpu()) {
         text(51,82,"PalmPilot wins!");
         break;
      }
      numMoves++;
   }
   wait();
}
