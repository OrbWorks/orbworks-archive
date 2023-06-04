// dbenum
// list all PocketC applets

main() {
   string name;

   name = dbenum(1, "PCvm", "PktC");
   do {
      puts(name + "\n");
      name = dbenum(0, "PCvm", "PktC");
   } while (name != 0);
}