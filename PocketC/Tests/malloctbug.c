//malloctbug.c
main() {
 pointer p;
 p=malloct(12,"i");
 p[0]="hello";
 puts(p[0]+"\n");
 text(23, 24);
}