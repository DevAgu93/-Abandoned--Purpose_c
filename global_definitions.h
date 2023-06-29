#define Assert(Condition) \
    if(!(Condition)) {*(int*)0 = 0;}
#define Assertm(condition, msg) Assert((condition && msg))
#define Assertc(condition) typedef char __C_ASSERT__[(condition) ? 1 : -1]
#define ARRAYCOUNT(a) (sizeof(a) / sizeof(a[0]))
#define NotImplemented Assert(0)

#define KILOBYTES(Value) ((Value)*1024LL)
#define MEGABYTES(Value) (KILOBYTES(Value)*1024LL)
#define GIGABYTES(Value) (MEGABYTES(Value)*1024LL)
#define TERABYTES(Value) (GIGABYTES(Value)*1024LL)
#define U32MAX 4294967295 //(1 << 32)
#define U16MAX 65535
//Maximum normal of a floating point number
#define F32MAX (3.402823E+38f) //340,282,300,000,000,000,000,000,000,000,000,000,000
//Minimum normal
#define F32MIN (-1.175494351e-38f)
//smallest relative difference between two adyacent numbers
#define F32_EPSILON (-1.19209290e-7) //0x7FC00000
#define F32_SIGN_MASK 0x80000000
//mask for the exponent of a floating point number (the 8 bits following the sign bit)
#define F32_EXPONENT_MASK 0x7f800000
#define F32_EXPONENT_SHIFT 23
//mask for the rest of the bits after the exponent
#define F32_SIGNIFICAND_MASK 0x007fffff
#define F32_EXPONENT_BIAS 127

#define I32MAX 2147483647
#define I32MIN (-2147483647 - 1)
#define I16MAX (32767)
#define I16MIN (-32768)

#define F32_NAN 0x7FC00000

//Args 
#if 1
//Variable arg lists.
//these macros are used to pass an unspecified amount of arguments
//to a function and reading them from the stack memory.
#define _INC_STDARG
typedef char* va_list;

//round the size of the parameter to the nearest integer boundary
#define _INTSIZEOF(n)          ((sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1))
#define _ADDRESSOF(src) (&(src))


//make args point to where the stack would start
//src is used to know where the next variables will be
//on the stack from its direction
#define _crt_va_start(args, src) ((void)(args = (va_list)_ADDRESSOF(src) + _INTSIZEOF(src)))
//advance args by the size of type rounded to the nearest integer boundary and return
//the variable.
#define _crt_va_arg(args, type) (*(type*)((args += _INTSIZEOF(type)) - _INTSIZEOF(type)))
#define _crt_va_end(args) ((void)(args = (va_list)0))

//#define __crt_va_start(args, x) __crt_va_start_a(args, x)

#define va_start_m _crt_va_start
#define va_arg_m   _crt_va_arg
#define va_end_m   _crt_va_end
#define va_copy(destination, source) ((destination) = (source))

#else
#include <stdarg.h>
#include <vadefs.h>
#endif


/*Notes:


IEEE 754's floating point representation
========================
= 1 =  8  =     23     =
========================
The first bit represents the sign

value exponent (8 bits):
from i = 0 to 7, 
E = (b(23+i) * 2^i + ...), 
r = (-1)^(b31) * 2^(E - 127)
where E is the resulting exponent, r is the resulting value,
(-1)^(b31) is the sign bit.

The mantissa is calculated as follows:
1 + (from i = 1 to 23 (b23-i * 2^-i) + ...) where b is the bit value from 1 to 0 and i an index from 0 to 23

Infinity:
is represented as s 11111111 000000000000
the two types or NaN are represented as: 
s 11111111 1xxxxxxxxxxx "quiet"
s 11111111 0xxxxxxxxxxx "signaling"

 */

typedef unsigned long long uint64;
typedef unsigned int       uint32;
typedef unsigned short     uint16; 
typedef unsigned char      uint8;

typedef long long          int64;
typedef short              int16;
typedef int                int32;
typedef char               int8;

typedef float              real32;
typedef double             real64;

typedef unsigned int       bool32;
typedef unsigned short     bool16; 
typedef unsigned char      bool8;


typedef unsigned long long u64;
typedef unsigned int       u32;
typedef unsigned short     u16; 
typedef unsigned char      u8;

typedef long long          i64;
typedef short              i16;
typedef int                i32;
typedef char               i8;

typedef float              f32;
typedef double             f64;

typedef unsigned int       b32;
typedef unsigned short     b16; 
typedef unsigned char      b8;

#define global_variable static

#define enum32(en) u32
#define enum16(en) u16
#define enum8(en) u8

typedef union ieee_binary{
	f32 f;
	i32 i;
}ieee_binary;

typedef union ieee_binary64{
	f64 f;
	i64 i;
}ieee_binary64;

static inline i32
f32_to_16_16(f32 v)
{
	i32 result = (i32)(v * (f32)(1 << 16) + (v >= 0 ? 0.5f : -0.5f));
	return(result);
}

static inline i64
f64_to_16_16(f64 v)
{
	i64 result = (i64)(v * (f64)((i64)1 << (i64)32) + (v >= 0 ? 0.5f : -0.5f));
	return(result);
}

static inline f32
f32_from_16_16(i64 v)
{
	f32 result = v / (f32)(1 << 8);

	return(result);
}

static inline f64
f64_from_16_16(i64 v)
{
	f64 result = v / (f64)(1 << 16);

	return(result);
}

inline f32
f32_positive_infinity()
{
	ieee_binary result = {0};
	result.i |= F32_EXPONENT_MASK;
	return(result.f);
}

inline f32
f32_nevative_infinity()
{
	ieee_binary result = {0};
	result.i |= F32_EXPONENT_MASK;
	result.i |= F32_SIGN_MASK;
	return(result.f);
}

inline u32
f32_is_infinite(f32 n)
{
	ieee_binary n_b;
	n_b.f = n;
	i32 n_exponent = n_b.i & F32_EXPONENT_MASK;
	u32 result = (n_exponent != F32_EXPONENT_MASK);
	return(result);
}

inline i32
f32_extract_exponent(f32 n)
{
	ieee_binary n_b;
	n_b.f = n;
	i32 n_exponent = n_b.i & F32_EXPONENT_MASK;
	n_exponent = (n_exponent >> F32_EXPONENT_SHIFT) - F32_EXPONENT_BIAS;
	return(n_exponent);
}

inline f32
f32_extract_mantissa(f32 n)
{
	ieee_binary n_b;
	n_b.f = n;
	//set exponent to zero
	n_b.i &= ~F32_EXPONENT_MASK;
	n_b.i |= F32_EXPONENT_BIAS << F32_EXPONENT_SHIFT;
	return(n_b.f);
}

static f32
f32_from_i32(i32 v)
{
	ieee_binary result;
	result.i = v;
	return(result.f);
}

static f64
f64_from_i64(i64 v)
{
	ieee_binary64 result;
	result.i = v;
	return(result.f);
}

#ifdef Bx64
    typedef u64 memory_size;
#else
    typedef u32 memory_size;
#endif

typedef struct 
{
    union
    {
        real32  v[2];
        struct { real32 x, y;
        };
    };
}vec2;
typedef struct 
{
    union
    {
        real32  v[3];
        struct {
            real32 x, y, z;
        };
    };
}vec3;

typedef struct 
{
    union
    {
        real32  v[4];
        struct {
            real32 x, y, z, w;
        };
		struct{
			f32 r, g, b, a;
		};
    };
}vec4;

typedef struct{
	union{
		i32 r[4];
      struct{
		  int32 x, y, w, h;
	  };
      struct{
		  int32 x0, y0, x1, y1;
	  };

	};
}rectangle32s;

typedef union{
	u32 r[4];
	struct{
		u32 x, y, w, h;
	};
}rectangle32u;

static rectangle32u
R32U(u32 x, u32 y, u32 w, u32 h)
{
	rectangle32u result = {x, y, w, h};
	return(result);
}

typedef struct{
    real32 x, y, w, h;
}rectangle32f;
