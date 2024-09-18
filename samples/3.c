#include <stdio.h>

int main(void)
{
    int *p = NULL;
    //Dereference null pointer
    *p = 42;
    printf("Value: %d\n", *p);
}
