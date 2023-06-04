/$sort.h
#ifndef SORT_I
#define SORT_I

//(c)T.Frogley all rights reserved
//codemonkey_uk@hotmail.com
//reuse without limitation

int swap_size=0;
pointer swap_buffer;
swap(pointer a, pointer b, int width)
{
  if(swap_size<width){
    if (swap_size) free(swap_buffer);
    swap_buffer = malloc(width);
    swap_size=width;
  }
  memcpy(swap_buffer,a,width);
  memcpy(a,b,width);
  memcpy(b,swap_buffer,width);
}

//insersion sort
isort(
  pointer base, 
  int num,
  int width,
  pointer compare
)
{
  pointer i, j,end;
  pointer low;
  pointer swap;

  end = base+num*width;
  swap = malloc(width);
  for (i=base+width;i<end;i=i+width){

   j=i-width;
   if((*compare)(i,j)<0){
     memcpy(swap,i,width);
     memcpy(i,j,width);
     for(j=j-width;j>=base;j=j-width)
        if((*compare)(swap,j)<0)
          memcpy(j+width,j,width);
        else break;
      memcpy(j+width,swap,width);
    }
  }  
  free(swap);
}

//selection sort
ssort(
  pointer base, 
  int num,
  int width,
  pointer compare
)
{
  pointer i, j,end;
  pointer low;

  end = base+(num-1)*width;

  for (i=base;i<end; i=i+width) {
    low=i;
    for (j = end; j > i; j=j-width)
      if((*compare)(j,low)<0)
        low = j;

    if (low!=i){
      swap(low,i,width);
    }
  }
}

//double bubble sort
dbsort(
  pointer base, 
  int num,
  int width,
  pointer compare
)
{
  pointer i,end;
  int done;

  end = base+(num-1)*width;

  do{
    done=1;
    for(i=base;i<end;i=i+width)
      if((*compare)(i,i+width)>0){
        done=0;
        swap(i,i+width,width);
      }

    if (!done)
    for(done=1;i>base;i=i-width)
      if((*compare)(i,i-width)<0){
        done=0;
        swap(i,i-width,width);
      }

  }while(!done);
}

// general sort
sort(
  pointer base, 
  int num,
  int width,
  pointer compare
)
{
  isort( base, num,width,compare);
}

//for sorting ints, floats
int compare(
  pointer a, pointer b
)
{
  return *b-*a;
}
//for sorting ints, floats, reversed
int reverse_compare(
  pointer a, pointer b
)
{return *a-*b;}

//for sorting strings alphabeticaly
int comparestr(pointer a, pointer b)
{
  if (*a>*b) return 1;
  else  if (*a<*b) return -1;
  else return 0;
}
//for sorting strings, reversed
int reverse_comparestr(
  pointer a, 
  pointer b
)
{
  return -comparestr(a,b);
}

#endif 