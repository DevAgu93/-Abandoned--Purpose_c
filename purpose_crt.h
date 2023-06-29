//Note(Agu):Should I put this on another file?
int _fltused = 0x9875;
#pragma function(memset)
void* __cdecl memset(void* _Dst,int _Val, size_t _Size)
{
    unsigned char *at = (unsigned char *)_Dst;
    unsigned char  v = *(unsigned char *)&_Val;
    while(_Size--) 
    {
        *at++ = v;
    }
    return _Dst;

}
