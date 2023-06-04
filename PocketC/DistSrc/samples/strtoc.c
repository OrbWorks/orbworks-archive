// Hack it up

hackitup(string str) {
  pointer ptr;
  string ret;
  int i, len;

  // determine the required length
  len = strlen(str) + 1;
  // allocate enough space
  ptr = malloct(len, "c");
  // fill the block with the chars from the string
  strtoc(str, ptr);

  // process each char
  for (i=0;i<len;i++) {
    if (ptr[i]=='A') ptr[i] = '4';
    else if (ptr[i]=='E') ptr[i] = '3';
  }

  // rebuild the return string
  ret = ctostr(ptr);
  // release the memory block
  free(ptr);
  return ret;
}

main() {
  string str;

  str = gets("Enter a string:");
  if (str) {
    str = strupr(str);
    puts(hackitup(str) + "\n");
  } else {
    puts("User cancelled\n");
  }
}