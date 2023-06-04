// IR Chat

main() {
   int ret;

   clear();
   puts("IR Chat\n");
   //ret = seropen(57600, "8N1C", 50);
   ret = seropenx(0x8001, 57600);
   puts("seropenx() - " + ret + "\n");
   while (1) {
      if (event(0)==1)
         // grafitti written
         sersend(key());
      if (serdata())
         // data waiting
         puts((char)serrecv());
      // save batteries
      sleep(10);
   }
   // This line included for completeness
   serclose();
}
