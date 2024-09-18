int main(void)
{
    //Generate an illegal CPU instruction
    __asm__("ud2");  //'ud2' is an undefined instruction
}
