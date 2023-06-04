// Address Reader

// offsets of specific entries
#define LastName	0
#define FirstName	1
#define Company		2
#define Phone1		3
#define Phone2		4
#define Phone3		5
#define Phone4		6
#define Phone5		7
#define Address		8
#define City		9
#define State		10
#define ZipCode		11
#define Country		12
#define Title		13
#define Custom1		14
#define Custom2		15
#define Custom3		16
#define Custom4		17
#define Note		18
#define NumFields	19

readRecord(pointer pStrArray, int id) {
    int bitmask, i;
    
    dbrec(id);	
    dbread('i'); // read and throw away options
    bitmask = dbread('i');
    dbread('c'); // unused
    
    for (i=0;i<NumFields;i++) {
        if (bitmask & (1 << i))
            pStrArray[i] = dbread('s');
    }
}



writeRecord(pointer pStrArray, int id) {
    int i, j;
    string recFormat = "iic";
    
    // The T5 Contacts application will attempt to copy updates to
    // AddressDB to the new contacts database. If all writes are not
    // done in one call to dbwritex(), it will crash when attempting
    // to do the migration.
    
    // the following is an in memory representation of the record
    int phoneFlags;
    int bitmask; // which fields are used
    char companyOffset;
    string data[NumFields];
    
    dbrec(id); // use id = -1 to add a new entry
    
    // compute the bitmask of used fields
    for (i=0;i<NumFields;i++) {
        if (pStrArray[i])
            bitmask = bitmask | (1 << i);
    }
    
    // calculate the company offset
    if (pStrArray[Company]) {
        if (pStrArray[0]) companyOffset = strlen(pStrArray[0]) + 1;
        if (pStrArray[1]) companyOffset = companyOffset + strlen(pStrArray[1]) + 1;
        companyOffset++;
    }
    
    // for each used field, add it to the in-memory representation
    // and add an "s" to the format string
    for (i=0;i<NumFields;i++) {
        if (bitmask & (1 << i)) {
            recFormat = recFormat + "s";
            data[j++] = pStrArray[i];
        }
    }
    
    // write the whole thing in one shot
    dbwritex(&phoneFlags, recFormat);
}

string addr[NumFields];

main() {
    int i;
    
    // read and display the 0th address record
    dbopen("AddressDB");
    readRecord(addr, 0);
    dbclose();	

    puts("Name:\t" + addr[LastName] + ", " + addr[FirstName] + "\n");
    puts("Addr:\t" + addr[Address] + "\n\t\t" + addr[City] + ", " +
        addr[State] + " " + addr[ZipCode]);
    
    for (i=0;i<NumFields;i++)
        addr[i] = "";
    
    // Insert a new address at the end of the database
    addr[FirstName] = "Jon";
    addr[LastName] = "Doe";
    addr[Phone1] = "(428) 555-8989";
    addr[City] = "Wallawalla";
    addr[State] = "WA";
    addr[ZipCode] = "78251";
    addr[Address] = "1287 63rd Pl";
    addr[Title] = "IS Guy";
    addr[Company] = "Conquer Inc";
    dbopen("AddressDB");
    writeRecord(addr, -1);
    dbclose();	
}
