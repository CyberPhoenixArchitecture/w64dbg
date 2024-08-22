#include <windows.h>

struct Example
{
    //Infinite recursion
    void RootException(void)
    {
        DebugBreak();
    }
    //Start the recursion
    void CauseException(void)
    {
        RootException();
    }
};

int main(void)
{
    Example example;
    example.CauseException();
}
