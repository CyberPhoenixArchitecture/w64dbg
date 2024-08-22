#include <stdio.h>

static inline void Exception(int i, double j, const char * str)
{
    //Cause access violation
    sscanf("12345", "%d", (int *) 1);
}

struct Example
{
    static void RootException(int i, float j)
    {
        Exception(i * 2, j, "Hello");
    }

    void CauseException(void)
    {
        RootException(4, 5.6f);
    }
};

int main(void)
{
    Example example;
    example.CauseException();
}
