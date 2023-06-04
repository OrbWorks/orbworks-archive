// atexit
// This is an example of using the atexit() function to save the applet
// state when the user switches apps
int x1, x2, y1, y2;

savestuff() {
  if (dbcreate("atexitSaveDB")) {
    dbwrite(x1);
    dbwrite(y1);
    dbwrite(x2);
    dbwrite(y2);
  }
}

loadstuff() {
  if (dbopen("atexitSaveDB")) {
    x1 = dbread('i');
    y1 = dbread('i');
    x2 = dbread('i');
    y2 = dbread('i');
  }
}

main() {
  int e;

  loadstuff();
  atexit(savestuff); // pass the pointer to savestuff()
  graph_on();
  title("Drag the pen");
  line(1, x1, y1, x2, y2);
  
  do {
    e = event(1);
    if (e == 2) {
      x1 = penx();
      y1 = peny();
    }
    if (e == 3) {
      x2 = penx();
      y2 = peny();
      clearg();
      line(1, x1, y1, x2, y2);
    }
  } while (1);
}