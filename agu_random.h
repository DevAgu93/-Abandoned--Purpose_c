#define BITN1 0x68E31DA4
#define BITN2 0xB5298A4D
#define BITN3 0x1B56C4E9
#define MULTF1 (1.0f / 27416821);

#define BITN4 0x27B4421A
#define BITN5 0x8EA59122
#define BITN6 0x948AD264

typedef struct{
    uint32 x;
    uint32 seed;
}random_series;

inline uint32
noise_u32(uint32 x, uint32 seed)
{
     u32 bit = x;
     bit *= BITN1;
     bit += seed;
     bit ^= bit >> 8;
     bit += BITN2;
     bit ^= bit << 8;
     bit *= BITN3;
     bit ^= bit >> 8;

     return bit;
}


inline uint32
noise1D_U32_2(uint32 x, uint32 seed)
{
     u32 bit = x;
     bit *= BITN4;
     bit += seed;
     bit ^= bit >> 8;
     bit += BITN5;
     bit ^= bit << 8;
     bit *= BITN6;
     bit ^= bit >> 8;

     return bit;
}
//same as below
inline real32
noise_f32(uint32 x, uint32 seed)
{
     u32 bit = noise_u32(x, seed);
     return (real32)bit / U32MAX;
}

#define _random_f32_lerp(left, delta, right)\
	(left * (1 - delta) +  right * delta)
//returns a float between 0-1.0f
inline real32
noise1D_F32(uint32 x, uint32 seed)
{
     u32 bit = noise_u32(x, seed);
     return (real32)bit / U32MAX;
}

inline uint32 
random_u32(random_series *random)
{
    return(noise_u32(random->x++, random->seed));
}
inline real32
random_f32(random_series *random)
{
    return(noise1D_F32(random->x++, random->seed));
}

static inline u32
random_get_u32_between(random_series *random, u32 n0, u32 n1)
{
	f32 interpolation = noise1D_F32(random->x++, random->seed);
	f32 result = _random_f32_lerp(n0, interpolation, n1);
	return((u32)result);
}

static inline f32
random_get_f32_between(random_series *random, f32 n0, f32 n1)
{
	f32 interpolation = noise1D_F32(random->x++, random->seed);
	f32 result = _random_f32_lerp(n0, interpolation, n1);
	return(result);
}

static inline random_series
random_series_create(u32 seed)
{
	random_series result = {0};
	result.seed = seed;
	return(result);
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

//for weighted randoms
//sum all weight numbers
//select depending on range
