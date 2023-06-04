// test routines

int passed = true;

test(int val, string testname) {
    if (!val) {
        passed = false;
        puts("FAIL: " + testname + "\n");
    } else {
        puts("PASS: " + testname + "\n");
    }
}

testErr(int err, string testname) {
    if (err != 0) {
        passed = false;
        puts("FAIL: (" + err + ") " + testname + "\n");
    } else {
        puts("PASS: " + testname + "\n");
    }
}

final() {
    if (passed)
        puts("\n## PASSED\n");
    else
        puts("\n## FAILED\n");
}
