/$ array2d
// array2d(int dim1, int dim2, char atype)
// create a two dimensional array (array[dim1][dim2])
// of type described by atype (i, p, c, s, f)

array2d(int dim1, int dim2, char atype) {
    pointer base_array, index_array;
    int i;

    base_array = malloct(dim1 * dim2, atype);
    if (!base_array)
        return 0;
    index_array = malloct(dim1, "p");
    if (!index_array) {
        free(base_array);
        return 0;
    }
    for (;i<dim1;i++)
        index_array[i] = &base_array[dim2 * i];
    return index_array;
} 
