#define BITN1 0x68E31DA4
#define BITN2 0xB5298A4D
#define BITN3 0x1B56C4E9
#define MULTF1 (1.0f / 27416821);

#define U32MAX 4294967296 //(1 << 32)
#define U16MAX 65536
#define F32MAX 3.402823E+38f //340,282,300,000,000,000,000,000,000,000,000,000,000

typedef struct{
    uint32 x;
    uint32 seed;
}random_series;

inline uint32
Noise1D_U32(uint32 x, uint32 seed)
{
     uint32 bit = x;
     bit *= BITN1;
     bit += seed;
     bit ^= bit >> 8;
     bit += BITN2;
     bit ^= bit << 8;
     bit *= BITN3;
     bit ^= bit >> 8;

     return bit;
}
inline real32
Noise1D_F32(uint32 x, uint32 seed)
{
     uint32 bit = Noise1D_U32(x, seed);
     return (real32)bit / U32MAX;
}

inline uint32 
random_GetU32(random_series *random)
{
    return(Noise1D_U32(random->x++, random->seed));
}
inline real32
random_GetF32(random_series *random)
{
    return(Noise1D_F32(random->x++, random->seed));
}
#if 0

const uint BITN1     = 0x68E31DA4;
        const uint BITN2     = 0xB5298A4D;
        const uint BITN3     = 0x1B56C4E9;
        const float MULTF1 = 1f / 27416821;

        public static uint Get1dUintNoise(in int x, in uint seed)
        {
            uint bit = (uint)x;

            bit *= BITN1;
            bit += seed;
            bit ^= bit >> 8;
            bit += BITN2;
            bit ^= bit << 8;
            bit *= BITN3;
            bit ^= bit >> 8;

            return bit;
        }
#endif
