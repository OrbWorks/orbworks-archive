// Database Sample
string data[10];
int ndata;

read() {
   int i;

   puts("Open succeeded, reading values\n");

   // read the data
   while (dbpos() >= 0)
      data[ndata++] = dbread('s');

   // output the data
   puts("Number entries: "+ndata+"\n");
   for (i=0;i<ndata;i++)
      puts("\x95 "+data[i]+"\n");

   // check the backup status
   if (dbbackup(2))
      puts("Database backup bit set\n");
   else
      puts("Database backup bit NOT set\n");

   // delete the database
   puts("Deleting DB 'TestDB'\n");
   dbdelete();
}

write() {
   puts("Open failed, creating new database\n");

   // attempt to create a new database
   if (!dbcreate("TestDB")) {
      puts("Database creation failed!\n");
      return;
   }

   // get values
   puts("Enter up to 10 values\n");
   while (data[ndata]=gets("Value #"+(ndata+1))) {
      // write data
      dbwrite(data[ndata]);
      puts("\x95Wrote '"+data[ndata]+"'\n");
      ndata++;
   }

   if (confirm("Set backup bit?"))
      dbbackup(1);

   // close the database
   dbclose();
}

main() {
   title("Database Sample");
   clear();
   puts("Opening DB 'TestDB'\n");
   if (dbopen("TestDB"))
      read();
   else
      write();
}