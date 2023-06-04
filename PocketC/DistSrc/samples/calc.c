// Simple calc

// This sample demonstrate advanced PocketC concepts:
// simulating structures, indirect function calls.
//
// This sample sets up an event-based applet framework, and
// creates a integer calculator using it.
//
// Since malloc() returns an array of ints, and we need a
// more complex structure for our buttons, we create a
// template for the button structure out of globals. Since
// globals reside in memory in the same order that they are
// declared, we can use a block of globals to initialize our
// button list items. This works because memcpy() copies
// not only memory contents, but also memory types.

include "calc_framework.c"

updateDisp(int num) {
  rect(0, 0, 15, 160, 39, 0);
  textattr(6, 1, 0);
  textalign(12);
  text(159, 16, num);
  textattr(0, 1, 0);
}

int total;
int operand;
char operator;

doOp() {
  if (operator=='+') {
    total = total + operand;
  } else if (operator=='-') {
    total = total - operand;
  } else if (operator=='*') {
    total = total * operand;
  } else if (operator=='/') {
    total = total / operand;
  }
  operand = 0;
  operator = '+';
  updateDisp(total);
}


// common code for each button
#define NUM_BEG { operand = operand * 10 +
#define NUM_END ; updateDisp(operand); }

// button handlers
but0() NUM_BEG 0 NUM_END
but1() NUM_BEG 1 NUM_END
but2() NUM_BEG 2 NUM_END
but3() NUM_BEG 3 NUM_END
but4() NUM_BEG 4 NUM_END
but5() NUM_BEG 5 NUM_END
but6() NUM_BEG 6 NUM_END
but7() NUM_BEG 7 NUM_END
but8() NUM_BEG 8 NUM_END
but9() NUM_BEG 9 NUM_END
butC() { operator = '+'; operand = 0; total = 0; updateDisp(0); }
butPlus() { doOp(); operator = '+'; }
butMinus() { doOp(); operator = '-'; }
butMult() { doOp(); operator = '*'; }
butDiv() { doOp(); operator = '/'; }
butEq() { doOp(); }
exitfunc() { exitflag = 1; }

main() {
  // initialize graphics
  graph_on();
  title("Simple Calc");
  // create our buttons
  addButton("7", but7, 30, 40, 55, 65);
  addButton("8", but8, 55, 40, 80, 65);
  addButton("9", but9, 80, 40, 105, 65);
  addButton("/", butDiv, 105, 40, 130, 65);

  addButton("4", but4, 30, 65, 55, 90);
  addButton("5", but5, 55, 65, 80, 90);
  addButton("6", but6, 80, 65, 105, 90);
  addButton("*", butMult, 105, 65, 130, 90);

  addButton("1", but1, 30, 90, 55, 115);
  addButton("2", but2, 55, 90, 80, 115);
  addButton("3", but3, 80, 90, 105, 115);
  addButton("-", butMinus, 105, 90, 130, 115);

  addButton("0", but0, 30, 115, 55, 140);
  addButton("C", butC, 55, 115, 80, 140);
  addButton("=", butEq, 80, 115, 105, 140);
  addButton("+", butPlus, 105, 115, 130, 140);

  addButton("Exit", exitfunc, 30, 145, 130, 159);

  operator='+';
  updateDisp(0);
  // let's go!
  eventLoop();
}