// Draw
int x,y;

draw() {
   int e;

   // Get initial position
   x = penx();
   y = peny();

   // Draw lines until we stop receiving penMoves
   do {
      line(1, x, y, x=penx(), y=peny());
   } while (event(1)==4);
}

main() {
   int e;

   graph_on();
   title("Draw");
   while (true) {
      e = event(1);
      if (e==2) draw();
   }
}

