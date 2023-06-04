// String Sample
string str;
main() {
   puts("String Library\n");
   while (str=gets("Enter a string:")) {
      puts("\nString\"" + str + "\"");
      puts("\nLength = " + strlen(str));
      puts("\nLeft3 = " + strleft(str,3));
      puts("\nRight3 = " + strright(str,3));
      puts("\n2-6 = " + substr(str, 1, 5));
      puts("\nLower = " + strlwr(str));
      puts("\nUpper = " + strupr(str));
      puts("\n");
   }
}