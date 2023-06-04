// Memo Pad Sample
string data[10];
int ndata;

read() {
  int i;

  puts("Open succeeded, reading values\n");

  // read title
  mmgetl();

  // read a line at a time
  while (!mmeof())
    data[ndata++] = mmgetl();

  // output the data
  puts("Number entries: "+ndata+"\n");
  for (i=0;i<ndata;i++)
    puts("\x95 "+data[i]+"\n");

  // delete the memo
  puts("Deleting memo 'TestMemo'\n");
  mmdelete();
}

write() {
  puts("Open failed, creating new memo\n");

  // attempt to create a new memo
  if (!mmnew()) {
    puts("Memo creation failed!\n");
    return;
  }

  // write title
  mmputs("TestMemo\n");

  // input data
  puts("Enter up to 10 values\n");
  while (data[ndata]=gets("Value #" + (ndata+1))) {
    // write data
    mmputs(data[ndata] + "\n");
    puts("\x95 Wrote '"+data[ndata]+"'\n");
    ndata++;
  }

  // close the memo
  mmclose();
}

main() {
  title("Memo Pad Sample");
  clear();
  puts("Opening Memo 'TestMemo'\n");
  if (mmfind("TestMemo")) read();
  else write();
}