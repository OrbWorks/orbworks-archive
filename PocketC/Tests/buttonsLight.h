/$ buttonsLight.h
// offsets for our pseudo button struct
#define func 0
#define next 1
#define left 2
#define right 3
#define top 4
#define bottom 5
#define btext 6

// button struct template
pointer buttonTemp;
pointer _1;
int _2, _3, _4, _5;
string _6;

// global pointer to our linked list
pointer buttons;

// set this to true to exit
int exitflag;

// global timer, there can only be one
pointer ptfunc;
int idelay;
int _buttonData[70];
pointer _lastButton;

buttonInit() {
  _lastButton = _buttonData;
}

drawButton(string bt, int l, int t, int r, int b) {
  pushdraw();
  setbgi(getuicolor(1));
  rect(0, l, t, r, b, 4);
  setfgi(getuicolor(0));
  frame(1, l, t, r, b, 4);
  settextcolori(getuicolor(2));
  textalign(11);
  text((l+r)/2, (t+b-3)/2, bt);
  popdraw();
}

// add a button to our linked list
@doc "add a button"; 
addButton(string bt, pointer f, int l, int t, int r, int b) {
  pointer ptr;
  // create the new block
  ptr = _lastButton;
  _lastButton = _lastButton+7;
  if (ptr) {
    // copy out template to the new block
    memcpy(ptr, &buttonTemp, 7);
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
    drawButton(bt, l, t, r, b);
  }
}

refreshAll() {
  pointer list;
  list = buttons;
  clearg();
  while (list) {
    drawButton(list[btext], list[left], list[top], list[right], list[bottom]);
    list = list[next];
  }
}

bLevent() {
  int e, x, y;
  pointer f, list;

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
    return 1;
  }
}
