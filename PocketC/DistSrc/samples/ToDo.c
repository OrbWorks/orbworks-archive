// ToDo Reader

#define Description  0
#define Note    1
#define Priority  2
#define Day      3
#define Month    4
#define Year    5

// memory for a todo structure
string todoStruct, _1;
int _2, _3, _4, _5;

readRecord(pointer pTodo, int id) {
  int packedDate;
  
  dbrec(id);
  if (dbreadx(&packedDate, "i2") != 1) {
    return 0; // failed to read
  }
  
  if (packedDate == 0xffff) {
    // date is unspecified
    pTodo[Day] = 0;
    pTodo[Month] = 0;
    pTodo[Year] = 0;
  } else {
    // unpack the date structure
    pTodo[Day] = packedDate & 0x1f;
    pTodo[Month] = (packedDate >> 5) & 0xf;
    pTodo[Year] = ((packedDate >> 9) & 0x7f) + 1904;
  }
    
  pTodo[Priority] = dbread('c');
  dbreadx(pTodo, "szsz");
  return 1;
}

writeRecord(pointer pTodo, int id) { // set id = -1 to add
  int packedDate;
  char priority;
  string desc;
  string note;
  
  dbrec(id);
  if (pTodo[Year] == 0)
    packedDate = 0xffff;
  else
    packedDate = (((pTodo[Year] - 1904) & 0x7f) << 9) | ((pTodo[Month] & 0xf) << 5) |
      (pTodo[Day] & 0x1f);
  
  priority = pTodo[Priority];
  desc = pTodo[Description];
  note = pTodo[Note];
  if (dbwritex(&packedDate, "i2cszsz") != 4) {
    return 0;
  }
  return 1;
}
  

main() {
  int nRecs, i, d;
  pointer pTodo;
  
  pTodo = &todoStruct; // so that we can access our items by array index
  
  dbopen("ToDoDB");
  nRecs = dbnrecs();
  puts("NumRecs: " + nRecs + "\n");
  for (i=0;i<nRecs;i++) {
    if (readRecord(pTodo, i)) {
      if (pTodo[Year]) {
        puts(pTodo[Priority] + ": " + pTodo[Month] + "/" + pTodo[Day] + "/" +
          pTodo[Year] + ": " + pTodo[Description] + "\n");
      } else {
        puts(pTodo[Priority] + ": <No Date>: " + pTodo[Description] + "\n");
      }
    }
  }
  
  // create a todo item
  d = date(0);
  pTodo[Year] = d / 10000;
  pTodo[Month] = (d % 10000) / 100;
  pTodo[Day] = (d % 100);
  pTodo[Priority] = 1;
  pTodo[Description] = "Register PocketC";
  pTodo[Note] = "Only 18.50";
  if (confirm("Add PocketC reminder?")) {
    if (!writeRecord(pTodo, -1))
      puts("Failed to write Todo");
  }
  
  dbclose();
}
