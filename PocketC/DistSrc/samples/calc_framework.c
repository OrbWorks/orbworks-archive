/$ calc_framework.c
// offets for our pseudo button struct
#define func 0
#define next 1
#define left 2
#define right 3
#define top 4
#define bottom 5
#define btext 6

// global pointer to our linked list
pointer buttons;

// set this to true to exit
int exitflag;

// add a button to our linked list
addButton(string bt, pointer f, int l, int t, int r, int b) {
  pointer ptr;
  // create the new block
  ptr = malloct(1, "ppiiiis");
  if (ptr) {
    // initialize the block
    ptr[func] = f;
    ptr[left] = l;
    ptr[right] = r;
    ptr[top] = t;
    ptr[bottom] = b;
    ptr[btext] = bt;
    // insert this block in the list
    ptr[next] = buttons;
    buttons = ptr;
    // draw the button
    frame(1, l, t, r, b, 2);
    textalign(11);
    text((l+r)/2, (t+b)/2, bt);
  }
}

eventLoop() {
  int e, x, y;
  pointer f, list;

  while (1) {
    // is it time to exit?
    if (exitflag) return;
    e = event(1);
    // respond only to the pen down
    if (e==2) {
      // set list to the start of the button list
      list = buttons;
      // get the coords of the key press
      x = penx();
      y = peny();
      while (list) {
        if (x >= list[left] && x <= list[right]
          && y >= list[top] && y <= list[bottom]) {
          // we found the button, get out of the loop
          break;
        }
        // check the next button in the list
        list = list[next];
      }
      // did we find a button?
      if (list) {
        // call the specified function
        (*list[func])();
      }
    }
  }
}
