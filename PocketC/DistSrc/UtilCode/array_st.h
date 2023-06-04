/$ array_st
// array_st(int dim, string stype)
// create an array of structures described by stype
// (as described by malloct())
// see example at bottom

array_st(int dim, string stype) {
    pointer base_array, index_array;
    int i, slen;

    slen = strlen(stype);
    base_array = malloct(dim, stype);
    if (!base_array)
        return 0;
    index_array = malloct(dim, "p");
    if (!index_array) {
        free(base_array);
        return 0;
    }
    for (;i<dim;i++)
        index_array[i] = &base_array[slen * i];
    return index_array;
}

/*
Example: create an array of structures, where each
  structure contains a string name, and an int age
  
#define H_NAME 0
#define H_AGE 1

main() {
  pointer humans;
  humans = array_st(5, "si");
  
  humans[0][H_NAME] = "Alex";
  humans[0][H_AGE] = 5;
  humans[1][H_NAME] = "Billy";
  humans[1][H_AGE] = 6;
  ...
}
*/
