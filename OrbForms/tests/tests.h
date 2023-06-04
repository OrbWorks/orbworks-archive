// test routines

bool passed = true;
bool quiet = false;
bool interactive = true;

void test(int val, string testname) {
    if (!val) {
        passed = false;
        puts("FAIL: " + testname + "\n");
        debuglog("FAIL: " + testname);
    } else {
        if (!quiet)
            puts("PASS: " + testname + "\n");
        debuglog("PASS: " + testname);
    }
}

void testErr(int err, string testname) {
    if (err != 0) {
        passed = false;
        puts("FAIL: (" + err + ") " + testname + "\n");
        debuglog("FAIL: (" + err + ") " + testname);
    } else {
        if (!quiet)
            puts("PASS: " + testname + "\n");
        debuglog("PASS: " + testname);
    }
}

void testErrQ(int err, string testname) {
    if (err != 0) {
        passed = false;
        puts("FAIL: (" + err + ") " + testname + "\n");
        debuglog("FAIL: (" + err + ") " + testname);
    }
}

void section(string sectName) {
    puts("*** " + sectName + " ***\n");
    debuglog("*** " + sectName + " ***");
}

void putsl(string l) {
    puts(l + "\n");
    debuglog(l);
}

void final() {
    if (passed) {
        puts("\n## PASSED\n");
        debuglog("\n## PASSED");
    } else {
        puts("\n## FAILED\n");
        debuglog("\n## FAILED");
    }
}
