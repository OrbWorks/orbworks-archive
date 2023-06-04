/$ array_st
// array_st(int dim, string stype)
// create an array of structures described by stype
// (as described by malloct())

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
