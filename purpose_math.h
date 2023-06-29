//========Math========
//#include <math.h>
/*
The __m128 data type is used to represent the contents of a Streaming SIMD Extensions registers used by the Streaming SIMD Extension intrinsics. This is either four packed single-precision floating-point values or one scalar single-precision number.

The __m128d data type holds two 64-bit floating point (double-precision) values.

The __m128i data type can hold sixteen 8-bit, eight 16-bit, or four 32-bit, or two 64-bit integer values.


*/

#include <xmmintrin.h>

#define PI 3.14159265359f
#define TWOPI 6.2831854f
#define PI_HALF 1.570796326794f

#define vec3_compare(v0, v1) ((v0.x == v1.x) && (v0.y == v1.y) && (v0.z == v1.z))
#define vec2_compare(v0, v1) ((v0.x == v1.x) && (v0.y == v1.y))

#define MIN(one, two)   ((one) < (two) ? (one) : (two))
#define MINEQ(one, two) ((one) <= (two) ? (one) : (two))
#define MAX(one, two)   ((one) > (two) ? (one) : (two))
#define MAXEQ(one, two) ((one) >= (two) ? (one) : (two))
#define CLAMP(v, min, max) ((((v)) < (min)) ? (min) : (((v)) > (max)) ? (max) : ((v)))

#define SIGN(v) (((v) >= 0) - ((v) < 0))
#define SIGN_OR_ZERO(v) (((v) > 0) - ((v) < 0))
#define ABS(v) ((v) * ((v) < 0 ? -1 : 1))

typedef struct{
	f32 w;
	union{
		vec3 vector;
		struct{
	f32 x;
	f32 y;
	f32 z;
		};
	};
}quaternion;

typedef struct{
	union{
		quaternion qr;
		struct{
			f32 w;
			f32 x;
			f32 y;
			f32 z;
		};
	};
	union{
		quaternion qd;
		struct{
			f32 wd;
			f32 xd;
			f32 yd;
			f32 zd;
		};
	};
}quaternion_dual;

typedef struct {
  union {
    vec4 v[4];
    struct {
      float _11;
      float _12;
      float _13;
      float _14;

      float _21;
      float _22;
      float _23;
      float _24;

      float _31;
      float _32;
      float _33;
      float _34;

      float _41;
      float _42;
      float _43;
      float _44;
    };
    float    m[4][4];
  };
} matrix4x4;

typedef struct {
  union {
    vec4 v[3];
    struct {
      f32 _11;
      f32 _12;
      f32 _13;
      f32 _14;
      f32 _21;
      f32 _22;
      f32 _23;
      f32 _24;
      f32 _31;
      f32 _32;
      f32 _33;
      f32 _34;
    };
    float    m[3][3];
  };
} matrix3x3;

typedef struct{
    matrix4x4 foward;
    matrix4x4 inverse;
}matrix4x4_data;

inline f32
degrees_to_radians_f32(f32 degrees)
{
	f32 pi_over_180 = PI / 180.0f;
	f32 result = degrees * pi_over_180;

	return(result);
}

inline f32
radians_to_degrees_f32(f32 radians)
{
	f32 pi_over_180 = PI / 180.0f;
	f32 result = radians / pi_over_180;
	return(result);
}

inline f32
f32_ceil(f32 v)
{
	f32 decimals = v - ((i32)v);

	if(decimals)
	{
	    v = v + (1.0f - decimals);
	}
	return(v);
}

inline f32
f32_floor(f32 v)
{
	if(v > 0)
	{
		v = (f32)(i32)v;
	}
	else
	{
	    f32 decimals = v - ((i32)v);
		if(decimals)
		{
			v = (f32)(((i32)v) - 1);
		}
	}
	return(v);
}

inline u32
u32_pow(u32 a, u32 b)
{
	u32 result = 1;
	while(b)
	{
		if((b % 2) == 1)
		{
			result *= a;
		}
		a *= a;
		b >>= 1;
	}
	return(result);
}
inline f32
f32_pow(f32 a, u32 b)
{
	f32 result = 1;
	while(b)
	{
		if((b % 2) == 1)
		{
			result *= a;
		}
		a *= a;
		b >>= 1;
	}
	return(result);
}

inline u32
factorial(u32 a)
{
	if(a == 0)
	{
		return(1);
	}
	u32 result = a;
	while(a > 1)
	{
		result = result * (a - 1);
		--a;
	}
	return(result);
}

inline f32
f32_mod(f32 a, f32 b)
{
	f32 result = a;
	result = a - (b * (i32)(a / b));
	return(result);
}

inline f64
f64_mod(f64 a, f64 b)
{
	f64 result = a;
	result = a - (b * (i64)(a / b));
	return(result);
}

inline f32
taylor_cosf(f32 x)
{
	// ((1 / 1!) * x^0) -
	// ((1 / 1!) * x^2) +
	// ((1 / 1!) * x^4) -
	// ((1 / 1!) * x^6) 
	//x = fmod(x + PI, PI * 2)

	//factorials of 0, 2, 4, 6, 8, 10, 12, 14, 16
	u64 fac[9] = 
	{
		0,
		2,
		24,
		720,
		40320,
		3628800,
		479001600,
		87178291200,
		20922789888000,
	};

#if 1
	x = f32_mod(ABS(x) + PI, PI * 2) - PI;

	f32 result = 
		1 -
		(f32_pow(x, 2)  / fac[1]) +
		(f32_pow(x, 4)  / fac[2]) -
		(f32_pow(x, 6)  / fac[3]) +
		(f32_pow(x, 8)  / fac[4]) -
		(f32_pow(x, 10) / fac[5]) +
		(f32_pow(x, 12) / fac[6]) -
		(f32_pow(x, 14) / fac[7])
		;
#else
	result = cos32(x);

#endif
	return(result);
}

inline f32
taylor_sinf(f32 x)
{
	// ((1 / 1!) * x^1) -
	// ((1 / 1!) * x^3) +
	// ((1 / 1!) * x^5) -
	// ((1 / 1!) * x^7) 
	//x = fmod(x + PI, PI * 2)

	//factorials of 1, 3, 5, 7, 9, 11, 13, 15, 17
	u64 fac[9] = 
	{
		1, 
		6,
		120, 
		5040,

		362880,
		39916800,
		6227020800,
		1307674368000,

		355687428096000,
	};

#if 1
	i32 term_sign = SIGN(x);
	x = f32_mod(ABS(x) + PI, PI * 2) - PI;

	f32 result = 
		(1 * x) - 
		(f32_pow(x, 3)  / fac[1]) +
		(f32_pow(x, 5)  / fac[2]) -
		(f32_pow(x, 7)  / fac[3]) +
		(f32_pow(x, 9)  / fac[4]) -
		(f32_pow(x, 11) / fac[5]) +
		(f32_pow(x, 13) / fac[6]) -
		(f32_pow(x, 15) / fac[7])
		;
	result *= term_sign;
#else
	f32 result = 0;
	u32 power = 1;
	f32 x_past = x;
	i32 term_sign = SIGN(x);
	//x = fmodf(ABS(x) + PI, PI * 2) - PI;
	x = f32_mod(ABS(x_past) + PI, PI * 2) - PI;
	u32 i = 0;
	while(i < 8)
	{
		//result += (f32_pow(x, power) / factorial(power)) * term_sign;
		f32 p = f32_pow(x, power);
		u64 f = fac[i]; 
		result += (p / f) * term_sign;
		power += 2;
		term_sign *= -1;
		i++;
	}

#endif
	return(result);
}

inline f32
f32_sin(f32 x)
{
	f32 result = taylor_sinf(x);
	return(result);
}
/*
   These coefficients were computed using the tool Sollya on WSL and
   rounded to 32-bit floating point using float toy (https://evanw.github.io/float-toy/)
   the calculations are bounded from [0, pi / 4]
*/
static f64
_sin64_calculation(f64 x)
{
	f64 x_s = x * x;
    f64 result = 
		    f64_from_i64(0x3CDC000000000000) + x * 
		    (f64_from_i64(0x3FEFFFFFFFFFFE75) + x_s * 
		    (f64_from_i64(0XBFC55555555485D0) + x_s * 
		    (f64_from_i64(0x3F811111107528C0) + x_s * 
		    (f64_from_i64(0xBF2A019F48CEF000) + x_s * 
		    (f64_from_i64(0x3EC71D5917540000) + x_s *
		     f64_from_i64(0xBE5A8D74CE000000))))));

	return(result);
}

static f32
_sin32_calculation(f32 x)
{
    f32 c0 = f32_from_i32(0x35000000);
    f32 c1 = f32_from_i32(0x3F7FFF83);
    f32 c2 = f32_from_i32(0xBE2A97B0);
    f32 c3 = f32_from_i32(0x3C04F1C0);

	f32 x_squared = x * x;
	f32 result = c0 + x * (c1 + x_squared * (c2 + x_squared * c3));
//

    return(result);
}

static f64
_cos64_calculation(f64 x)
{
	f64 x_s = x * x;
    f64 result = 
	        f64_from_i64(0x3ff0000000000000) + x_s * 
		    (f64_from_i64(0xbfdfffffffffffa0) + x_s * 
		    (f64_from_i64(0x3fa555555554f7d0) + x_s * 
		    (f64_from_i64(0xbf56c16c16477200) + x_s * 
		    (f64_from_i64(0x3efa019f875c0000) + x_s * 
		    (f64_from_i64(0xbe927df671400000) + x_s *
		     f64_from_i64(0x3e21b94b30000000))))));

    //f64 result = 
	//        0x3ff0000000000000 + x_s * 
	//	    (0xbfdfffffffffffa0 + x_s * 
	//	    (0x3fa555555554f7d0 + x_s * 
	//	    (0xbf56c16c16477200 + x_s * 
	//	    (0x3efa019f875c0000 + x_s * 
	//	    (0xbe927df671400000 + x_s *
	//	     0x3e21b94b30000000)))));
    //0x3ff0000000000000 + x^2 * 
	//(0xbfdfffffffffffa0 + x^2 * 
	//(0x3fa555555554f7d0 + x^2 * 
	//(0xbf56c16c16477200 + x^2 * 
	//(0x3efa019f875c0000 + x^2 * 
	//(0xbe927df671400000 + x^2 *
	// 0x3e21b94b30000000)))))

	return(result);
}
static f32
_cos32_calculation(f32 x)
{

	//error = 0.000000059
    f32 c0= f32_from_i32(0x3F800000);
    f32 c1= f32_from_i32(0xBEFFFFDC);
    f32 c2= f32_from_i32(0x3D2A9FB1);
    f32 c3= f32_from_i32(0xBAB2374F);

	f32 x_squared = x * x;
	f32 result = c0 + x_squared * (c1 + x_squared * (c2 + c3 * x_squared));
    result = c0 + x_squared * (c1 + x_squared * (c2 + x_squared * c3));

	return(result);
}

static inline f64
_tan64_calculation(f64 x)
{
	f64 x_s = x * x;
    f64 result = 
		x * (f64_from_i64(0x3ff0000020000000) + x_s *
			(f64_from_i64(0x3fd5553540000000) + x_s * 
			(f64_from_i64(0x3fc1157500000000) + x_s * 
			(f64_from_i64(0x3fab1d9200000000) + x_s * 
			(f64_from_i64(0x3f9a490800000000) + x_s * 
			(f64_from_i64(0x3f58794000000000) + x_s * 
			 f64_from_i64(0x3f84e5e000000000)))))));
	return(result);
}

static inline f32
_tan32_calculation(f32 x)
{
	f32 x_s = x * x;

    f32 result = 
		x * (f32_from_i32(0x3F800001) + x_s *
			(f32_from_i32(0x3EAAA9AA) + x_s * 
			(f32_from_i32(0x3E08ABA8) + x_s * 
			(f32_from_i32(0x3D58EC90) + x_s * 
			(f32_from_i32(0x3CD24840) + x_s * 
			(f32_from_i32(0x3AC3CA00) + x_s * 
			 f32_from_i32(0x3C272F00)))))));
	return(result);
}

//this goes from [0, 1]
static inline f32
_arctan32_calculation(f32 x)
{
	f32 x_s = x * x;
	/*
    0x3e90000000000000 + x * 
	(0x3feffff320000000 + x^2 * 
	(0xbfd5524dc0000000 + x^2 * 
	(0x3fc9560f80000000 + x^2 * 
	(0xbfc0e3c880000000 + x^2 * 
	(0x3fb43e8100000000 + x^2 * 
	(0xbfa101ce00000000 + x^2 * 
	0x3f7b751000000000))))))
	*/

    f32 result = 
		    f32_from_i32(0x34800000) + x *
			(f32_from_i32(0x3f7fff99) + x_s * 
			(f32_from_i32(0xbeaa926e) + x_s * 
			(f32_from_i32(0x3e4ab07c) + x_s * 
			(f32_from_i32(0xbe071e44) + x_s * 
			(f32_from_i32(0x3da1f408) + x_s *
			(f32_from_i32(0xbd080e70) + x_s *
			f32_from_i32(0x3bdba880) )))))); 
	return(result);
}

static inline f32
_arcsin32_calculation(f32 x)
{

	f32 x_s = x * x;
    f32 result = 
		x * (f32_from_i32(0x3f7fffff) + x_s *
			(f32_from_i32(0x3e2aabf8) + x_s * 
			(f32_from_i32(0x3d995428) + x_s * 
			(f32_from_i32(0x3d3d4dd0) + x_s * 
			(f32_from_i32(0x3cae5c80) + x_s * 
			f32_from_i32(0x3d3d94d0) )))));

    //0x3f127c0000000000 + x * 
	//(0x3feff587c0000000 + x^2 * 
	//(0x3fc79b8900000000 + x^2 * 
	//(0xbf763fd000000000 + x^2 * 
	//0x3fc5937d80000000)))
	return(result);
}

static inline f64
_arcsin64_calculation(f64 x)
{

	return(0);
}


static f64
cos64(f64 x)
{

	x = f64_mod(x, PI * 2);
	x = x < 0 ? -x : x;
	f64 result = x;
	u32 n = 0;
	if(x < PI / 2)
	{
	}
	else if(x < PI)
	{
		x = PI - x;
		n = 1;
	}
	else if(x < PI + (PI / 2))
	{
		x = x - PI;
		n = 1;
	}
	else
	{
		x = PI * 2 - x;
	}

	if(x < PI / 4)
	{
		result = _cos64_calculation(x);
	}
	else
	{
		x = PI / 2 - x;
		result = _sin64_calculation(x);
	}
	result = n ? -result : result;

	return(result);
}

static f32
cos32(f32 x)
{

	x = f32_mod(x, PI * 2);
	x = x < 0 ? -x : x;
	f32 result = x;
	u32 n = 0;
	if(x < PI / 2)
	{
	}
	else if(x < PI)
	{
		x = PI - x;
		n = 1;
	}
	else if(x < PI + (PI / 2))
	{
		x = x - PI;
		n = 1;
	}
	else
	{
		x = PI * 2 - x;
	}

	if(x < PI / 4)
	{
		result = _cos32_calculation(x);
	}
	else
	{
		x = PI / 2 - x;
		result = _sin32_calculation(x);
	}
	result = n ? -result : result;

	return(result);
}

static f32
sin32(f32 x)
{

	if(!x) return(0);
	//reduce x to [0, pi * 2]
	x = f32_mod(x, PI * 2);
	f32 result = x;
	u32 n = 0;
	if(x < 0)
	{
		x = x < 0 ? -x : x;
		n = !n;
	}

	if(x < PI / 2)
	{
	}
	else if(x < PI)
	{
		x = PI - x;
	}
	else if(x < PI + (PI / 2))
	{
		//sine is negative at this point
		x = x - PI;
		n = !n;
	}
	else
	{
		x = PI * 2 - x;
		n = !n;
	}

	if(x < PI / 4)
	{
		result = _sin32_calculation(x);
	}
	else
	{
		x = PI / 2 - x;
		result = _cos32_calculation(x);
	}
	result = n ? -result : result;

	return(result);
}

static f64
sin64(f64 x)
{

	//reduce x to [0, pi * 2]
	x = f64_mod(x, PI * 2);
	f64 result = x;
	u32 n = 0;
	if(x < 0)
	{
		x = x < 0 ? -x : x;
		n = !n;
	}

	if(x < PI / 2)
	{
	}
	else if(x < PI)
	{
		x = PI - x;
	}
	else if(x < PI + (PI / 2))
	{
		//sine is negative at this point
		x = x - PI;
		n = !n;
	}
	else
	{
		x = PI * 2 - x;
		n = !n;
	}

	if(x < PI / 4)
	{
		result = _sin64_calculation(x);
	}
	else
	{
		x = PI / 2 - x;
		result = _cos64_calculation(x);
	}
	result = n ? -result : result;

	return(result);
}

//TODO: Minimize error when x goes beyond 1.45
static inline f32
tan32(f32 x)
{

	u32 n = 0;
	if(x < 0)
	{
		x = -x;
		n = !n;
	}
	x = f32_mod(x, PI);
	if(x > PI / 2)
	{
		//go from pi / 2 to 0
		x = PI  - x;
		n = !n;
	}

	//if x > 90º, go from 0 to pi / 2 and invert the tangent
	f32 result = x < (PI / 4) ?
		         _tan32_calculation(x) :
				 (1.0f / _tan32_calculation((PI / 2) - x));

	result = n ? -result : result;
	return(result);
}

static inline f32
arctan32(f32 x)
{
	u32 n = 0;
	if(x < 0)
	{
		x = -x;
		n = !n;
	}
	//Since the range of the calculation is bounded by
	//[0, 1], inverting x and taking the distance from pi/2
	u32 inverse = x > 1.0f;

	f32 result = 0;
	if(inverse)
	{
		x = 1.0f / x;
		result = PI / 2 - _arctan32_calculation(x);
	}
	else 
	{
		result = _arctan32_calculation(x);
	}
	result = n ? -result : result;
	return(result);
}

static inline f32
arctan232(f32 x, f32 y)
{
	f32 result = 0;
	if(x > 0)
	{
	    f32 yox = x/y;
		result = arctan32(yox);
		if(y < 0)
		{
			result = PI + result;
		}
	}
	else if(x < 0 && y >= 0)
	{
	    f32 yox = x/y;
		result = arctan32(yox);
	}
	else if(x < 0 && y < 0)
	{
	    f32 yox = x/y;
		result = arctan32(yox) - (PI);
	}
	else if(x == 0)
	{
		result = PI;

		if(y > 0)
		{
			result = 0;
		}
		else if(y == 0)
		{
			result = 0; 
		}
	}

	return(result);
}

static inline f64
sqrt64(f64 x)
{

	if(x == 0)
	{
		return(0);
	}
	f64 x_copy = x;
	u64 *xp = (u64 *)&x;
	*xp = (*xp >> 1) + ((u64)1023 << 51);
	
	u32 r = 0;
	while(r++ <= 3)
	{
		x = x - (x * x - x_copy) / (2 * x);
	}
	return(x);
}

static inline f32
sqrt32(f32 x)
{
#if 0
	if(x < 0)
	{
		return(0);
	}
	i32 fixed_x = f32_to_16_16(x);
	u32 leading_zeroes_count = 0;
	for(u32 bit = 0;
			bit < 16;
			bit++)
	{
		u32 b = ((fixed_x << bit) & 0x80000000) != 0;
		if(b)
		{
			break;
		}
		leading_zeroes_count++;
	}
	u32 shift_count = 15 - leading_zeroes_count;
	ieee_binary64 y;
	y.i = fixed_x >> shift_count;
	i32 table_index = (y.i >> 12) & 0xf;
	f64 table[ ] = 
	{ 1.0000000000000000f, 0.9411764705882350f, 0.8888888888888889f, 0.8421052631578946f,
      0.8000000000000000f, 0.7619047619047619f, 0.7272727272727273f, 0.6956521739130435f,
      0.6666666666666666f, 0.6400000000000000f, 0.6153846153846154f, 0.5925925925925926f,
      0.5714285714285714f, 0.5517241379310345f, 0.5333333333333333f, 0.5161290322580645f };
	y.f = y.i / 65536.0;
	f64 r = table[table_index];
	i32 a = f32_to_16_16(y.f * r - 1.0f);
	return(0);
	//count leading zeroes of the number
	//max number of bits I need - zeroes counted
#endif
#if 0
    f32 y = x;
    // Approximation
    u32 *i = (u32 *)&x;
    *i = (*i >> 1) + (127 << 22);
    // Newton-Raphson
	//f(x)  = x^2 - i
	//f(x)' = 2x
	//y = f(x) + f(x)'
	//i is the value to look the square root of
	//y = x - (x^2 - i) / 2x
    x = (x + y/x) / 2;
    x = (x + y/x) / 2;
    x = (x + y/x) / 2;
    return x;
#else
	if(x <= 0)
	{
		return(0);
	}
	u32 r = 0;
	f32 x_copy = x;
	u32 *xp = (u32 *)&x;
	//from wikipedia
	/*
     for a 32-bit single precision floating point number in IEEE
	 format you can get the approximate logarithm by interpreting its binary representation as
	 a 32-bit integer, scaling it by 2^-23, and removing a bias of 127.

     To get the square root, divide the logarithm by 2 and convert the value back (*xp >> 1).
	*/
	*xp = (*xp >> 1) + (127 << 22);
//	*xp = (1 << 29) + (*xp >> 1) - (1 << 22);
	
	while(r++ <= 3)
	{
		x = x - (x * x - x_copy) / (2 * x);
	}
	return(x);
#endif
}

static inline i32
sqrt32_int(u32 v)
{

	i8 rds[] = {
		v & 0x3,
		(v >> 2) & 0x3,
		(v >> 4) & 0x3,
		(v >> 6) & 0x3,
		(v >> 8) & 0x3,
		(v >> 10) & 0x3,
		(v >> 12) & 0x3,
		(v >> 14) & 0x3,
		(v >> 16) & 0x3,
		(v >> 18) & 0x3,
		(v >> 20) & 0x3,
		(v >> 22) & 0x3,
		(v >> 24) & 0x3,
		(v >> 26) & 0x3,
		(v >> 28) & 0x3,
		(v >> 30) & 0x3,
	};
	u32 rds_count = ARRAYCOUNT(rds);
	i32 res = 0;
	i32 Q = 0;
	for(u32 r = 0;
			r < rds_count;
			r++)
	{

		//start with the most significant bit
		i8 c = rds[rds_count - 1 - r];
		//res = A
		res |= c;

		u8 digit = c >= 0 ? 1 : 0;
		//D = T
		i8 d = res - (Q << 2) - 1;
		Q <<= 1;

		if(d >= 0)
		{
			res = d;
			c = 1;
			Q |= 1;
		}
		else
		{
			c = 0;
		}
		res <<= 2;
	}
	return(Q);
}

static inline f32
sqrt32_16_16(f32 val)
{

	i32 v = f32_to_16_16(val);

	i32 res = 0;
	i32 Q = 0;
	i32 remainder = 0;
	for(u32 r = 0;
			r < 16;
			r++)
	{

		//start with the most significant bit
		i8 c = ((v & 0xC0000000) >> 30);
		v <<= 2;
		//res = A
		//left shift val two places into res (the current msb, c)
		res <<= 2;
		res |= c;

		u8 digit = c >= 0 ? 1 : 0;
		//D = T. set d = res - Q shifted by two and with 01 on its msb
		i8 d = res - ((Q << 2) | 1);
		//left shift Q
		Q <<= 1;

		//is the result positive? then set the lsb of Q to 1
		if(d >= 0)
		{
			remainder = res;
			res = d;
			c = 1;
			if(r != 16)
			{
			    Q |= 1;
				int s = 0;
			}
		}
	}

	val = f32_from_16_16(Q);

	return(val);
}

static inline f64
sqrt64_new(f64 val)
{

	i64 v = f64_to_16_16(val);

	i32 res = 0;
	i64 Q = 0;
	i64 remainder = 0;
	for(u32 r = 0;
			r < 32;
			r++)
	{

		//start with the most significant bit
		i8 c = ((v & 0xC000000000000000) >> 62);
		v <<= 2;
		res <<= 2;
		res |= c;

		u8 digit = c >= 0 ? 1 : 0;
		//D = T. set d = res - Q shifted by two and with 01 on its msb
		i8 d = res - (i8)((Q << 2) | 1);
		//left shift Q
		Q <<= 1;

		//is the result positive? then set the lsb of Q to 1
		if(d >= 0)
		{
			remainder = res;
			res = d;
			c = 1;
			if(r != 31)
			{
			    Q |= 1;
				int s = 0;
			}
		}
	}
	if(!remainder)
	{
		Q |= 1;
	}

	val = f64_from_16_16(Q);
	return(val);
}

static f32
arcsin32(f32 x)
{
	u32 n = 0;
	if(x < 0)
	{
		x = -x;
		n = 1;
	}

	f32 result = 0;
	if(x < 0.5f)
	{
		result = _arcsin32_calculation(x);
	}
	else
	{
		result = PI * 0.5f - 2 * _arcsin32_calculation(sqrt32(0.5f - 0.5f * x));
	}

	result = n ? -result : result;
	return(result);
}

static inline f32
arccos32(f32 x)
{
	f32 result = (PI * 0.5f) + arcsin32(-x);
	return(result);
}


inline rectangle32s
REC(int32 x0, int32 y0, int32 x1, int32 y1)
{
	rectangle32s result = {x0, y0, x1, y1};
	return(result);
}

#define v2(x, y) V2(x, y)
inline vec2 
V2(real32 x, real32 y)
{
    vec2 result = {x, y};
    return result;
}

inline vec3 
V3(real32 x, real32 y, real32 z)
{
    vec3 result =  {x, y, z};
    return result;
}

inline vec4
V4(real32 x, real32 y, real32 z, real32 w)
{
    vec4 result = {x, y, z, w};
    return result;
}



inline int32
RoundToInt32(real32 v)
{
    return(int32)_mm_cvtss_si32(_mm_set_ss(v));
   // return(int32)(v);
}

inline f32
pow_i32(f32 x, i32 n)
{
	f32 ans = 1.0f;
	i32 n1 = n;

	if(n < 0)
	{
		n1 = -n1;
	}
	while(n1 > 0)
	{
		if(n1 % 2 == 1)
		{
			ans *= x;
			n1--;
		}
		else
		{
			x *= x;
			n1 /= 2;
		}
	}
	if(n < 0)
	{
		ans = 1.0f / ans;
	}
	return(ans);

}

#define f32_round_to_int(v) ((f32)(i32)(v))
inline f32
f32_round_up(f32 value)
{
    return((f32)((i32)(value + 0.9f)));
}

inline f32
f32_abs(f32 x)
{
	ieee_binary x_b;
	x_b.f = x;
	x_b.i &= ~F32_SIGN_MASK;
	return(x_b.f);
}

inline real32
Round1(real32 value)
{
    int32 valuei = (int32)value;
    int32 decimal = (int32)((value - valuei) * 10.0f);
    return decimal / 10.0f + valuei;
}

inline int32
rect_GetWidth(rectangle32s rect)
{
    return rect.w - rect.x;
}

inline int32
rect_GetHeight(rectangle32s rect)
{
    return rect.h - rect.y;
}

inline real32
real32_Pow(real32 v, real32 p)
{
    return((real32)pow(v, p));
}


inline real32 
real32_Squared(real32 val)
{
    return val * val;
}

inline real32
real32_DivSafe(real32 val, real32 divisor, real32 defaultval)
{
    real32 result = defaultval;
    if(divisor != 0)
    {
        result = val / divisor;
    }
    return result;
}

inline real32 
real32_V3_Mul(real32 scalar, vec3 vec)
{
    real32 result = scalar;
    result *= vec.x;
    result *= vec.y;
    result *= vec.z;
    return result;
}


inline vec4
SRGB1To255(vec4 c)
{
    vec4 result = { 255 * sqrt32(c.x),
                    255 * sqrt32(c.y),
                    255 * sqrt32(c.z),
                    255 * c.w
                  };
    return(result);
}

inline vec2
vec2_add(vec2 v1, vec2 v2)
{
    vec2 result = {
               v1.x + v2.x, 
               v1.y + v2.y
    };
    return(result);
}

inline vec2 
vec2_sub(vec2 v1, vec2 v2)
{
    vec2 result = {
               v1.x - v2.x, 
               v1.y - v2.y 
    };
    return(result);
}

inline vec2 
vec2_mul(vec2 v1, vec2 v2)
{
    vec2 result = {
               v1.x * v2.x, 
               v1.y * v2.y 
    };
    return(result);
}

inline vec2 
vec2_div(vec2 v1, vec2 v2)
{
    vec2 result = {
               v1.x / v2.x, 
               v1.y / v2.y 
    };
    return(result);
}

#define vec3_scale(v, f) vec3_f32_mul(v, (f32)f)
#define vec2_scale(v, f) vec2_f32_mul(v, (f32)f)
inline vec2
vec2_f32_mul(vec2 v, f32 f)
{
	v.x *= f;
	v.y *= f;
	return(v);
}

inline vec2
vec2_vec3_sub_xy(vec2 v2, vec3 v3)
{
	v2.x -= v3.x;
	v2.y -= v3.y;
	return(v2);
}

inline f32 
vec2_inner(vec2 v1, vec2 v2)
{
    return(v1.x * v2.x + 
           v1.y * v2.y);
}

inline f32
vec2_inner_squared(vec2 v)
{
	f32 result = vec2_inner(v, v);
	return(result);
}

inline vec2
vec2_Perpendicular(vec2 vec)
{
    vec2 result = {-vec.y, vec.x};
    return result;
}

inline vec3
vec2_ToV3(vec2 v2, f32 z)
{
	vec3 result = {v2.x, v2.y, z};
	return(result);
}

inline vec3
vec3_round_to_int(vec3 v)
{
	v.x = (f32)((i32)v.x);
	v.y = (f32)((i32)v.y);
	v.z = (f32)((i32)v.z);
	return(v);
}

inline vec2
vec2_round_to_int(vec2 v)
{
	v.x = (f32)((i32)v.x);
	v.y = (f32)((i32)v.y);
	return(v);
}

inline vec3
vec3_round_up(vec3 v)
{
	v.x = (f32)((i32)(v.x + 0.9f));
	v.y = (f32)((i32)(v.y + 0.9f));
	v.z = (f32)((i32)(v.z + 0.9f));
	return(v);
}

inline vec3 
vec3_add(vec3 v1, vec3 v2)
{
    vec3 result = {
               v1.x + v2.x, 
               v1.y + v2.y, 
               v1.z + v2.z 
    };
    return(result);
}

inline vec3 
vec3_add_xy(vec3 v, real32 xa, real32 ya)
{
	v.x += xa;
	v.y += ya;
    return(v);
}


inline vec3
vec3_add_x(vec3 v, real32 x)
{
	v.x += x;
	return(v);
}

inline vec3
vec3_add_y(vec3 v, real32 y)
{
	v.y += y;
	return(v);
}

inline vec3 
vec3_add_z(vec3 v, real32 z) 
{
	v.z += z;
    return(v);
}

inline vec3 
vec3_sub(vec3 v1, vec3 v2)
{
    vec3 result = {
               v1.x - v2.x, 
               v1.y - v2.y, 
               v1.z - v2.z 
    };
    return(result);
}

inline vec3
vec3_sub_x(vec3 v, real32 x)
{
	v.x -= x;
	return(v);
}

inline vec3
vec3_sub_y(vec3 v, real32 y)
{
	v.y -= y;
	return(v);
}

inline vec3
vec3_sub_z(vec3 v, real32 z)
{
	v.z -= z;
	return(v);
}

inline vec3 
vec3_mul(vec3 v1, vec3 v2)
{
    vec3 result = {
               v1.x * v2.x, 
               v1.y * v2.y, 
               v1.z * v2.z 
    };
    return(result);
}

inline vec3 
vec3_div(vec3 v1, vec3 v2)
{
    vec3 result = {
               v1.x / v2.x, 
               v1.y / v2.y, 
               v1.z / v2.z 
    };
    return(result);
}

inline vec3
vec3_div_safe(vec3 v1, vec3 v2)
{
	v2.x = v2.x == 0 ? 1.0f : v2.x;
	v2.y = v2.y == 0 ? 1.0f : v2.y;
	v2.z = v2.z == 0 ? 1.0f : v2.z;

	vec3 result = vec3_div(v1, v2);
	return(result);
}

inline vec3 
vec3_f32_add(vec3 v, real32 scalar)
{
    vec3 result; 
    result.x = v.x + scalar; 
    result.y = v.y + scalar; 
    result.z = v.z + scalar; 
    return(result);
}

inline vec3 
vec3_f32_sub(vec3 v, real32 scalar)
{
    vec3 result; 
    result.x = v.x - scalar; 
    result.y = v.y - scalar; 
    result.z = v.z - scalar; 
    return(result);
}

inline vec3 
vec3_f32_subR(real32 scalar, vec3 v)
{
    vec3 result; 
    result.x = scalar - v.x; 
    result.y = scalar - v.y; 
    result.z = scalar - v.z; 
    return(result);
}

#define vec3_Half(vec) vec3_f32_mul(vec, 0.5f);
inline vec3 
vec3_f32_mul(vec3 vec, real32 scalar)
{
    vec3 result;
    result.x = vec.x * scalar;
    result.y = vec.y * scalar;
    result.z = vec.z * scalar;
    return result;
}

inline vec3 
vec3_u32_Mul(vec3 vec, uint32 v)
{
    vec3 result;
    result.x = vec.x * v;
    result.y = vec.y * v;
    result.z = vec.z * v;
    return result;
}

inline vec3 
vec3_i32_Mul(vec3 vec, int32 v)
{
    vec3 result;
    result.x = vec.x * v;
    result.y = vec.y * v;
    result.z = vec.z * v;
    return result;
}

inline vec3 
vec3_f32_div(vec3 vec, real32 scalar)
{
    vec3 result;
    result.x = vec.x / scalar;
    result.y = vec.y / scalar;
    result.z = vec.z / scalar;
    return result;
}

inline vec3
vec3_vec2_add_xy(vec3 v3, vec2 v2)
{
	v3.x += v2.x;
	v3.y += v2.y;
	return(v3);
}

inline vec3
vec3_vec2_sub_xy(vec3 v3, vec2 v2)
{
	v3.x -= v2.x;
	v3.y -= v2.y;
	return(v3);
}

inline vec3
vec3_vec2_mul_xy(vec3 v3, vec2 v2)
{
	v3.x *= v2.x;
	v3.y *= v2.y;
	return(v3);
}

#define vec3_negated vec3_opposite
#define vec3_inverse vec3_opposite
inline vec3
vec3_opposite(vec3 v)
{
    v.x = -v.x;
    v.y = -v.y;
    v.z = -v.z;
    return v;
}

inline real32
vec3_inner(vec3 v1, vec3 v2)
{
    return(v1.x * v2.x + 
           v1.y * v2.y +
           v1.z * v2.z);
}

inline real32
vec3_inner_squared(vec3 v)
{
    return(vec3_inner(v, v));
}


inline vec3 
vec3_hadamard(vec3 v1, vec3 v2) 
{
    vec3 result = { v1.x * v2.x,
                    v1.y * v2.y,
                    v1.z * v2.z
    };
                    
    return result;
}

inline vec3 
vec3_cross(vec3 one, vec3 two)
{
    // [ V1.y*V2.z - V1.z*V2.y,
    //  V1.z*V2.x - V1.x*V2.z,
    //  V1.x*V2.y - V1.y*V2.x ] the result is a vector perpendicular to both a and b
    vec3 result = {
        one.y * two.z - one.z * two.y, //1y * 2z - 1z * 2y
        one.z * two.x - one.x * two.z, //1z * 3x - 1x * 2z
        one.x * two.y - one.y * two.x, //1x * 2y - 1y * 2x
    };
    return result;
}

inline real32
vec3_length(vec3 vec)
{
    return(sqrt32((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z)));
}

inline vec3
vec3_normalize(vec3 vec)
{
    real32 OneOverLenght = 1.0f / vec3_length(vec);
    vec.x *= OneOverLenght;
    vec.y *= OneOverLenght;
    vec.z *= OneOverLenght;
    return(vec);
}

inline vec3
vec3_normalize_safe(vec3 vec)
{
    real32 lengthSquared = vec3_inner(vec, vec);
    //lengthSquared = lengthSquared * lengthSquared;

    if(lengthSquared > (0.0001f * 0.0001f))
    {
       real32 o = 1.0f / sqrt32(lengthSquared);
       vec.x *= o; 
       vec.y *= o;
       vec.z *= o;
    }
    return(vec);
}

inline vec4 
vec4_add(vec4 v1, vec4 v2)
{
    vec4 result = {
               v1.x + v2.x, 
               v1.y + v2.y, 
               v1.z + v2.z, 
               v1.w + v2.w 
    };
    return(result);
}

inline vec4 
vec4_sub(vec4 v1, vec4 v2)
{
    vec4 result = {
               v1.x - v2.x, 
               v1.y - v2.y, 
               v1.z - v2.z, 
               v1.w - v2.w 
    };
    return(result);
}

inline vec4 
vec4_mul(vec4 v1, vec4 v2)
{
    vec4 result = {
               v1.x * v2.x, 
               v1.y * v2.y, 
               v1.z * v2.z, 
               v1.w * v2.w 
    };
    return(result);
}

inline vec4 
vec4_div(vec4 v1, vec4 v2)
{
    vec4 result = {
               v1.x / v2.x, 
               v1.y / v2.y, 
               v1.z / v2.z, 
               v1.w / v2.w, 
    };
    return(result);
}

inline vec4 
vec4_f32_add(vec4 v, real32 scalar)
{
    vec4 result; 
    result.x = v.x + scalar; 
    result.y = v.y + scalar; 
    result.z = v.z + scalar; 
    result.w = v.w + scalar; 
    return(result);
}

inline vec4 
vec4_f32_sub(vec4 v, real32 scalar)
{
    vec4 result; 
    result.x = v.x - scalar; 
    result.y = v.y - scalar; 
    result.z = v.z - scalar; 
    result.w = v.w - scalar; 
    return(result);
}

#define vec4_scale vec4_f32_mul
static vec4 
vec4_f32_mul(vec4 v, real32 scalar)
{
    vec4 result; 
    result.x = v.x * scalar; 
    result.y = v.y * scalar; 
    result.z = v.z * scalar; 
    result.w = v.w * scalar; 
    return(result);
}

inline vec4 
vec4_f32_div(vec4 v, real32 scalar)
{
    vec4 result; 
    result.x = v.x / scalar; 
    result.y = v.y / scalar; 
    result.z = v.z / scalar; 
    result.w = v.w / scalar; 
    return(result);
}

inline vec4
vec4_from_rec(rectangle32s r)
{
	vec4 result = {(real32)r.x, (real32)r.y, (real32)r.w, (real32)r.h};
	return(result);
}

inline matrix3x3
matrix3x3_from_vec_row(vec3 v0, vec3 v1, vec3 v2)
{
	matrix3x3 result = {
		v0.x, v0.y, v0.z,
		v1.x, v1.y, v1.z,
		v2.x, v2.y, v2.z,
	};
	return(result);
}

inline matrix3x3
matrix3x3_from_vec_col(vec3 v0, vec3 v1, vec3 v2)
{
	matrix3x3 result = {
		v0.x, v1.x, v2.x,
		v0.y, v1.y, v2.y,
		v0.z, v1.z, v2.z,
	};
	return(result);
}

#define matrix3x3_v3_mul_cols_l(m, vec) matrix3x3_v3_mul_cols(vec, m)
inline vec3
matrix3x3_v3_mul_rows(matrix3x3 m, vec3 vec) // Multiply rows of Matrix with the vector
{
    vec3 result;

    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[0][1]) + (vec.z * m.m[0][2]);
    result.y = (vec.x * m.m[1][0]) + (vec.y * m.m[1][1]) + (vec.z * m.m[1][2]);
    result.z = (vec.x * m.m[2][0]) + (vec.y * m.m[2][1]) + (vec.z * m.m[2][2]);
    return result;
}

inline vec3 
matrix3x3_v3_mul_cols(vec3 vec, matrix3x3 m) // Now with columns
{
    vec3 result;
    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[1][0]) + (vec.z * m.m[2][0]);
    result.y = (vec.x * m.m[0][1]) + (vec.y * m.m[1][1]) + (vec.z * m.m[2][1]);
    result.z = (vec.x * m.m[0][2]) + (vec.y * m.m[1][2]) + (vec.z * m.m[2][2]);
    return result;
}

inline vec3
matrix4x4_v3_mul_rows(matrix4x4 m, vec3 vec, float w) // Multiply rows of Matrix with the vector
{
    vec3 result;

    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[0][1]) + (vec.z * m.m[0][2]) + (w * m.m[0][3]);
    result.y = (vec.x * m.m[1][0]) + (vec.y * m.m[1][1]) + (vec.z * m.m[1][2]) + (w * m.m[1][3]);
    result.z = (vec.x * m.m[2][0]) + (vec.y * m.m[2][1]) + (vec.z * m.m[2][2]) + (w * m.m[2][3]);
    return result;
}
#define matrix4x4_v3_mul_cols_l(m, vec, w) matrix4x4_v3_mul_cols(vec, m, w)
inline vec3 
matrix4x4_v3_mul_cols(vec3 vec, matrix4x4 m, real32 w) // Now with columns
{
    vec3 result;
    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[1][0]) + (vec.z * m.m[2][0]) + (w * m.m[3][0]);
    result.y = (vec.x * m.m[0][1]) + (vec.y * m.m[1][1]) + (vec.z * m.m[2][1]) + (w * m.m[3][1]);
    result.z = (vec.x * m.m[0][2]) + (vec.y * m.m[1][2]) + (vec.z * m.m[2][2]) + (w * m.m[3][2]);
    return result;
}

inline vec4
matrix4x4_v4_mul_rows(matrix4x4 m, vec4 vec) // Multiply rows of Matrix with the vector
{
    vec4 result;

    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[0][1]) + (vec.z * m.m[0][2]) + (vec.w * m.m[0][3]); 
    result.y = (vec.x * m.m[1][0]) + (vec.y * m.m[1][1]) + (vec.z * m.m[1][2]) + (vec.w * m.m[1][3]); 
    result.z = (vec.x * m.m[2][0]) + (vec.y * m.m[2][1]) + (vec.z * m.m[2][2]) + (vec.w * m.m[2][3]);
    result.w = (vec.x * m.m[3][0]) + (vec.y * m.m[3][1]) + (vec.z * m.m[3][2]) + (vec.w * m.m[3][3]); 
    return result;
}

inline vec4 
matrix4x4_v4_mul_cols(vec4 vec, matrix4x4 m) // Now with columns
{
    vec4 result;
    result.x = (vec.x * m.m[0][0]) + (vec.y * m.m[1][0]) + (vec.z * m.m[2][0]) + (vec.w * m.m[3][0]); 
    result.y = (vec.x * m.m[0][1]) + (vec.y * m.m[1][1]) + (vec.z * m.m[2][1]) + (vec.w * m.m[3][1]); 
    result.z = (vec.x * m.m[0][2]) + (vec.y * m.m[1][2]) + (vec.z * m.m[2][2]) + (vec.w * m.m[3][2]);
    result.w = (vec.x * m.m[0][3]) + (vec.y * m.m[1][3]) + (vec.z * m.m[2][3]) + (vec.w * m.m[3][3]); 
    return result;
}

inline matrix4x4
matrix4x4_mul(matrix4x4 m1, matrix4x4 m2)
{
    matrix4x4 result;

    //Multiply the whole column of m2 with the row of m1
    float x = m1.m[0][0];
    float y = m1.m[0][1];
    float z = m1.m[0][2];
    float w = m1.m[0][3];
    result.m[0][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z) + (m2.m[3][0] * w);
    result.m[0][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z) + (m2.m[3][1] * w);
    result.m[0][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z) + (m2.m[3][2] * w);
    result.m[0][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z) + (m2.m[3][3] * w);
    //Repeat for the rest...
    x = m1.m[1][0];
    y = m1.m[1][1];
    z = m1.m[1][2];
    w = m1.m[1][3];
    result.m[1][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z) + (m2.m[3][0] * w);
    result.m[1][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z) + (m2.m[3][1] * w);
    result.m[1][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z) + (m2.m[3][2] * w);
    result.m[1][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z) + (m2.m[3][3] * w);

    x = m1.m[2][0];
    y = m1.m[2][1];
    z = m1.m[2][2];
    w = m1.m[2][3];
    result.m[2][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z) + (m2.m[3][0] * w);
    result.m[2][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z) + (m2.m[3][1] * w);
    result.m[2][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z) + (m2.m[3][2] * w);
    result.m[2][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z) + (m2.m[3][3] * w);

    x = m1.m[3][0];
    y = m1.m[3][1];
    z = m1.m[3][2];
    w = m1.m[3][3];
    result.m[3][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z) + (m2.m[3][0] * w);
    result.m[3][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z) + (m2.m[3][1] * w);
    result.m[3][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z) + (m2.m[3][2] * w);
    result.m[3][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z) + (m2.m[3][3] * w);

    return result;
}

inline matrix3x3
matrix3x3_mul(matrix3x3 m1, matrix3x3 m2)
{
    matrix3x3 result;

    //Multiply the whole column of m2 with the row of m1
    float x = m1.m[0][0];
    float y = m1.m[0][1];
    float z = m1.m[0][2];
    float w = m1.m[0][3];
    result.m[0][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z);
    result.m[0][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z);
    result.m[0][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z);
    result.m[0][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z);
    //Repeat for the rest...
    x = m1.m[1][0];
    y = m1.m[1][1];
    z = m1.m[1][2];
    w = m1.m[1][3];
    result.m[1][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z);
    result.m[1][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z);
    result.m[1][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z);
    result.m[1][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z);

    x = m1.m[2][0];
    y = m1.m[2][1];
    z = m1.m[2][2];
    w = m1.m[2][3];
    result.m[2][0] = (m2.m[0][0] * x) + (m2.m[1][0] * y) + (m2.m[2][0] * z);
    result.m[2][1] = (m2.m[0][1] * x) + (m2.m[1][1] * y) + (m2.m[2][1] * z);
    result.m[2][2] = (m2.m[0][2] * x) + (m2.m[1][2] * y) + (m2.m[2][2] * z);
    result.m[2][3] = (m2.m[0][3] * x) + (m2.m[1][3] * y) + (m2.m[2][3] * z);

    return(result);
}


inline vec3 
matrix3x3_v3_get_column(matrix3x3 m, uint8 c)
{
    vec3 result =
    {m.m[0][c], m.m[1][c], m.m[2][c] };
    return result;
}

inline vec3 
matrix3x3_v3_get_row(matrix3x3 m, uint8 r)
{
    vec3 result =
    {m.m[r][0], m.m[r][1], m.m[r][2] };
    return result;
}
//HELPER FUNCTIONS.
inline vec3 
matrix4x4_v3_get_column(matrix4x4 m, uint8 c)
{
    vec3 result =
    {m.m[0][c], m.m[1][c], m.m[2][c] };
    return result;
}

inline vec3 
matrix4x4_v3_get_row(matrix4x4 m, uint8 r)
{
    vec3 result =
    {m.m[r][0], m.m[r][1], m.m[r][2] };
    return result;
}

inline vec4 
matrix4x4_v4_get_column(matrix4x4 m, uint8 c)
{
    vec4 result =
    {m.m[0][c], m.m[1][c], m.m[2][c], m.m[3][c] };
    return result;
}

inline vec4 
matrix4x4_v4_get_row(matrix4x4 m, uint8 r)
{
    vec4 result =
    {m.m[r][0], m.m[r][1], m.m[r][2], m.m[r][3] };
    return result;
}

inline matrix4x4
VecToColumns(vec3 v1, vec3 v2, vec3 v3)
{
    matrix4x4 result = 
    {
       v1.x, v2.x, v3.x, 0,
       v1.y, v2.y, v3.y, 0,
       v1.z, v2.z, v3.z, 0,
       0,    0,    0,    1
    };
    return result;
}
inline matrix4x4
matrix4x4_rows_from_vectors(vec3 v1, vec3 v2, vec3 v3)
{
    matrix4x4 result = 
    {
       v1.x, v1.y, v1.z, 0,
       v2.x, v2.y, v2.z, 0,
       v3.x, v3.y, v3.z, 0,
       0,    0,    0,    1
    };
    return result;
}

static inline quaternion_dual
QUATD(f32 w, f32 x, f32 y, f32 z,f32 wd, f32 xd, f32 yd, f32 zd)
{
	quaternion_dual q = {w, x, y, z, wd, xd, yd, zd};
	return(q);
}

static inline quaternion
QUAT(f32 w, f32 x, f32 y, f32 z)
{
	quaternion q = {w, x, y, z};
	return(q);
}

static inline quaternion
quaternion_identity()
{
	quaternion q = {1, 0, 0, 0};
	return(q);
}

static quaternion_dual
quaternions_to_dual(quaternion qr, quaternion qd)
{
	quaternion_dual qd_result = {qr, qd};
	return(qd_result);
}


/*
   To get the angles back, you do the following
   a = arccos(q.w)
   v.x = q.x / sin(a)
   v.y = q.y / sin(a)
   v.z = q.z / sin(a)
   v.x *= a;
   */

static void
quaternion_fill_rotations_radians(
		quaternion q,
		f32 *angle,
		f32 *yaw,
		f32 *pitch,
		f32 *roll)
{
	f32 a = arccos32(q.w);
	f32 sinb = sin32(a);
   *angle = a;
#if 1
   if(sinb)
   {
	   f32 x = q.x / sinb;
	   f32 y = q.y / sinb;
	   f32 z = q.z / sinb;
	   a *= 2;
	   *yaw = a * x;
	   *pitch = a * y;
	   *roll = a * z;
   }
   else
   {
	   *yaw = 0;
	   *pitch = 0; 
	   *roll= 0;
   }
#else
   if(sinb)
   {
	   *yaw = q.x / sinb;
	   *pitch = q.y / sinb;
	   *roll = q.z / sinb;
   }
#endif
}


static void
quaternion_fill_rotations_degrees(
		quaternion q,
		f32 *angle,
		f32 *yaw,
		f32 *pitch,
		f32 *roll)
{
	quaternion_fill_rotations_radians(
			q,
			angle,
			yaw,
			pitch,
			roll);

	*angle = radians_to_degrees_f32(*angle);
	*yaw = radians_to_degrees_f32(*yaw);
	*pitch = radians_to_degrees_f32(*pitch);
	*roll = radians_to_degrees_f32(*roll);
}

//quaternion-like zyx matrix
static inline quaternion
quaternion_from_rotations_radians(f32 yaw, f32 pitch, f32 roll)
{
	//roll = -roll;

    f32 cos_yaw   = cos32(yaw * 0.5f);
    f32 sin_yaw   = sin32(yaw * 0.5f);
    f32 cos_pitch = cos32(pitch * 0.5f);
    f32 sin_pitch = sin32(pitch * 0.5f);
    f32 cos_roll  = cos32(roll * 0.5f);
    f32 sin_roll  = sin32(roll * 0.5f);

	quaternion q;
	q.w = cos_yaw * cos_pitch * cos_roll + sin_yaw * sin_pitch * sin_roll;
	q.z = cos_yaw * cos_pitch * sin_roll - sin_yaw * sin_pitch * cos_roll;
	q.y = sin_yaw * cos_pitch * sin_roll + cos_yaw * sin_pitch * cos_roll;
	q.x = sin_yaw * cos_pitch * cos_roll - cos_yaw * sin_pitch * sin_roll;
	/*
	   yaw (z, c), pitch (y, b)  roll (x, a)
cos(c/2) * cos(b/2) * cos(a/2) + (sin(c/2) * sin(b/2) * sin(a/2)) + 
(cos(c/2) * cos(b/2) * sin(a/2))i - (sin(c/2) * sin(b/2) * cos(a/2))i +
(sin(c/2) * cos(b/2) * sin(a/2))j + (cos(c/2) * sin(b/2)  * cos(a/2))j +
(sin(c/2) * cos(b/2) * cos(a/2))k - (cos(c/2) * sin(b/2) * sin(a/2))k +
*/
    return(q);
}

static inline quaternion
quaternion_from_rotations_degrees(f32 yaw, f32 pitch, f32 roll)
{
	//roll = -roll;
	yaw = degrees_to_radians_f32(yaw);
	pitch = degrees_to_radians_f32(pitch);
	roll = degrees_to_radians_f32(roll);

	quaternion q = quaternion_from_rotations_radians(yaw, pitch, roll);

    return(q);
}

static inline quaternion
quaternion_from_rotations_scale(f32 yaw, f32 pitch, f32 roll)
{
	yaw *= PI;
	pitch *= PI;
	roll *= PI;

	quaternion q = quaternion_from_rotations_radians(yaw, pitch, roll);
    return(q);
}

//the inverse of a quaternion is the conjugate q* divided by its length squared ql^2
//q^-1 = q* / ql^2
//for rotation quaternions (unit quaternions) the inverse is the conjugate
//q^-1 = q* if |q| == 1
/*
  A rotation of qa followed by a rotation of qb can be combined into the single rotation
  qc = qbqa. This can be extended to an arbitrary number of rotations.
  Note that the order matters.

  -La multiplicación de un quaternion con su conjugado da su magnitud al cuadrado:
  q * q* = q.w^2 + q.x^2 + q.y^2 + q.z^2
*/
static inline quaternion
quaternion_conjugate(quaternion q)
{
	quaternion q_result;
	q_result.w = q.w;
	q_result.x = -q.x;
	q_result.y = -q.y;
	q_result.z = -q.z;
	return(q_result);
}

static inline quaternion
quaternion_add(quaternion q0, quaternion q1)
{
	quaternion q_result;
	q_result.w = (q0.w + q1.w);
	q_result.x = (q0.x + q1.x);
	q_result.y = (q0.y + q1.y);
	q_result.z = (q0.z + q1.z);
	return(q_result);

}

static inline quaternion
quaternion_sub(quaternion q0, quaternion q1)
{
	quaternion q_result;
	q_result.w = (q0.w - q1.w);
	q_result.x = (q0.x - q1.x);
	q_result.y = (q0.y - q1.y);
	q_result.z = (q0.z - q1.z);
	return(q_result);

}

static inline quaternion
quaternion_mul(quaternion q0, quaternion q1)
{
	quaternion q_result;
	q_result.w = (q0.w * q1.w) - (q0.x * q1.x) - (q0.y * q1.y) - (q0.z * q1.z);
	q_result.x = (q0.w * q1.x) + (q0.x * q1.w) + (q0.y * q1.z) - (q0.z * q1.y);
	q_result.y = (q0.w * q1.y) - (q0.x * q1.z) + (q0.y * q1.w) + (q0.z * q1.x);
	q_result.z = (q0.w * q1.z) + (q0.x * q1.y) - (q0.y * q1.x) + (q0.z * q1.w);
	return(q_result);
}

//in this case is like multiplying the vector with the rows of a matrix
static inline vec3 
quaternion_v3_mul_foward_inverse(quaternion q, vec3 v)
{
	quaternion qv;
	quaternion q_inv;
	q_inv.w = q.w;
	q_inv.x = -q.x;
	q_inv.y = -q.y;
	q_inv.z = -q.z;

	qv.w = 0;
	qv.x = v.x;
	qv.y = v.y;
	qv.z = v.z;

	quaternion q_result = quaternion_mul(q, qv);
	q_result = quaternion_mul(q_result, q_inv);

	v.x = q_result.x;
	v.y = q_result.y;
	v.z = q_result.z;
	return(v);

}

//columns instead
static inline vec3 
quaternion_v3_mul_inverse_foward(quaternion q, vec3 v)
{
	quaternion qv;
	quaternion q_inv;
	q_inv.w = q.w;
	q_inv.x = -q.x;
	q_inv.y = -q.y;
	q_inv.z = -q.z;

	qv.w = 0;
	qv.x = v.x;
	qv.y = v.y;
	qv.z = v.z;

	quaternion q_result = quaternion_mul(q_inv, qv);
	q_result = quaternion_mul(q_result, q);

	v.x = q_result.x;
	v.y = q_result.y;
	v.z = q_result.z;
	return(v);

}

#define quaternion_dot quaternion_inner
static inline f32 
quaternion_inner(quaternion q0, quaternion q1)
{
	f32 result = (
			q0.w * q1.w +
			q0.x * q1.x +
			q0.y * q1.y +
			q0.z * q1.z
			);
	return(result);
}

static inline f32
quaternion_length(quaternion q)
{
	f32 result = sqrt32((q.w * q.w) + (q.x * q.x) +(q.y * q.y) +(q.z * q.z));
	return(result);
}

static inline quaternion
quaternion_normalize_safe(quaternion q)
{
	f32 q_dot = quaternion_inner(q, q);

	if(q_dot > 0.0001f * 0.0001f)
	{
		f32 q_dot_inv = 1.0f / sqrt32(q_dot);
		q.w *= q_dot_inv;
		q.x *= q_dot_inv;
		q.y *= q_dot_inv;
		q.z *= q_dot_inv;
	}
	return(q);
}

static inline quaternion 
quaternion_scale(quaternion q, f32 s)
{
	q.w *= s;
	q.x *= s;
	q.y *= s;
	q.z *= s;
	return(q);
}

static inline quaternion
quaternion_rotate_by_radian(quaternion q, f32 a)
{
	f32 a_cos = cos32(a * 0.5f);
	f32 a_sin = sin32(a * 0.5f);
	quaternion q_result;
	//q_result.w = (a_cos * q.w) - (a_sin * q.x); 
	//q_result.x = (a_cos * q.x) + (a_sin * q.w);
	//q_result.y = (a_cos * q.y) - (a_sin * q.z);
	//q_result.z = (a_cos * q.z) + (a_sin * q.y);
	q_result.w = a_cos - (a_sin * q.w); 
	q_result.x = (a_sin * q.x);
	q_result.z = (a_sin * q.z);
	q_result.y = (a_sin * q.y);
//	q = quaternion_normalize_safe(q);
	/*

cos(a) * w - sin(a) * x +
(cos(a) * x + sin(a) * w) * i +
(cos(a) * y - sin(a) * z) * j +
(cos(a) * z + sin(a) * y) * k
	*/
	return(q_result);
}


/*
   create quaternion from vector and radian. Given a vector v and an angle a
   q = cos(a) + sin(a) * (v.x, v.y, v.z)
   assuming the vector is of unit length

   To get the angles back, you do the following
   a = arccos(q.w)
   v.x = q.x / sin(a)
   v.y = q.y / sin(a)
   v.z = q.z / sin(a)
*/
static inline quaternion
quaternion_rotate_by_radian_and_maintain_length(quaternion q, f32 a)
{
	f32 a_cos = cos32(a);
	f32 a_sin = sin32(a);
	f32 a_sin_sq = a_sin * a_sin;

	f32 q_lenght = a_sin_sq * q.x + a_sin_sq * q.y + a_sin_sq * q.z;
	quaternion q_result;
	if(q_lenght < 1)
	{
		q_result = q;
		q_result.w = a_cos;
		if(!q.x && !q.y && !q.z)
		{
			q_result.x = a_sin;
		}
		else
		{
			q_result.x = sqrt32(1 - (q_result.y * q_result.y) - (q_result.z * q_result.z));
			q_result.y = sqrt32(1 - (q_result.x * q_result.x) - (q_result.z * q_result.z));
			q_result.z = sqrt32(1 - (q_result.x * q_result.x) - (q_result.y * q_result.y));

			//if not multiplying by a_sin, this would give me a good input vector
			//to do cos(a) + sin(a) * q.vector
			q_result.x *= a_sin;
			q_result.y *= a_sin;
			q_result.z *= a_sin;
		}
	}
	else
	{

	    q_result.w = a_cos;
	    q_result.x = a_sin * q.x;
	    q_result.y = a_sin * q.y;
	    q_result.z = a_sin * q.z;
		q_result = quaternion_normalize_safe(q_result);
	}
		return(q_result);

}

static inline quaternion
quaternion_rotated_at(f32 x, f32 y, f32 z, f32 angle)
{
	quaternion q = {0, x, y, z};
	if(x + y + z)
	{
		q.vector = vec3_normalize_safe(q.vector);
		q = quaternion_rotate_by_radian(q, angle);
	}
	else
	{
		q.w = 1;
	}
	return(q);
}

static inline quaternion
quaternion_from_vectors(vec3 v0, vec3 v1)
{
	quaternion q = {1, 0, 0, 0};
	f32 w  = vec3_inner(v0, v1);
	f32 v0_l = vec3_length(v0);
	f32 v1_l = vec3_length(v1);
	f32 length = sqrt32((v0_l * v0_l) * (v1_l * v1_l));
	if(w / length == -1)
	{
		//vectors are orthogonal
		f32 xa = ABS(v0.x);
		f32 ya = ABS(v0.y);
		f32 za = ABS(v0.z);
		vec3 v2 = xa < ya ? (xa < za ? V3(1, 0, 0) : V3(0, 0, 1)) :
			                  (ya < za ? V3(0, 1, 0) : V3(0, 0, 1));
		q.w = 0;
		q.vector = vec3_normalize(v2);
	}
	q.w = length + w;
	q.vector = vec3_cross(v0, v1);
	q = quaternion_normalize_safe(q);
	return(q);
}

static inline quaternion
quaternion_from_vector(vec3 v)
{
	quaternion q = {0, v.x, v.y, v.z};
	//q.w = 1.0f - (v.x + v.y + v.z);
	return(q);
}

static inline quaternion
quaternion_between_unit_vectors(vec3 v0, vec3 v1)
{
	quaternion q_r;
	f32 v0_length = vec3_length(v0);
	f32 v1_length = vec3_length(v1);
	f32 angle = arccos32(vec3_inner(v0, v1) / (v0_length * v1_length));
	f32 sin_angle = sin32(angle / 2);

	vec3 v0_v1_cross = vec3_normalize_safe(vec3_cross(v0, v1));

	q_r.w = cos32(angle * 0.5f);
	q_r.x = v0_v1_cross.x * sin_angle;
	q_r.y = v0_v1_cross.y * sin_angle;
	q_r.z = v0_v1_cross.z * sin_angle;

	return(q_r);
}

static inline quaternion
quaternion_unit_to_angle_and_vector(quaternion q)
{
	quaternion q_r;
	f32 angle = arccos32(q.w);

	f32 sin_angle = sin32(angle);
	q_r.w = angle;
	if(sin_angle != 0)
	{
		q_r.x = q.x / sin_angle;
		q_r.y = q.y / sin_angle;
		q_r.z = q.z / sin_angle;
	}
	return(q_r);
}

static inline quaternion
quaternion_angle_vector_to_unit(quaternion q)
{
	quaternion q_result;

	f32 sin_angle = sin32(q.w);
	q_result.w = cos32(q.w);
	if(q.w < 0)
	{
		sin_angle = -sin_angle;
	}
	q_result.x = sin_angle * q.x;
	q_result.y = sin_angle * q.y;
	q_result.z = sin_angle * q.z;

	return(q_result);
}

//assumes quaternions are unit quaternions
static inline quaternion
quaternion_unit_slerp(quaternion q0, quaternion q1, f32 t)
{
	//the multiplication of the real parts of q0 * q1^-1
	//a0 * a1 + (b0 * b1) + (c0 * c1) + (d0 * d1)
	f32 q0q1_cos_angle = (q0.w * q1.w) + (q0.x * q1.x) + (q0.y * q1.y) + (q0.z * q1.z);
	if(q0q1_cos_angle < 0)
	{
		q1.w = -q1.w;
		q1.x = -q1.x;
		q1.y = -q1.y;
		q1.z = -q1.z;
		q0q1_cos_angle = -q0q1_cos_angle;
	}
	//ONLY WORKS FOR UNIT QUATERNIONS!
	f32 angle = arccos32(q0q1_cos_angle);
	f32 sin_angle = sin32(angle);
	if(sin_angle < 0.0001f * 0.0001f)
	{
		quaternion result;
		result.w = (q0.w * (1 - t) + q1.w * t);
		result.x = (q0.x * (1 - t) + q1.x * t);
		result.y = (q0.y * (1 - t) + q1.y * t);
		result.z = (q0.z * (1 - t) + q1.z * t);
		return(result);
	}

	//(qa * sin((1 - t) * h) + qb * sin(t * h)) / sin(h)
	//qa * sin((1 - t) * h) / sin(h) + qb * sin(t * h) / sin(h)
	f32 t0 = sin32((1 - t) * angle) / sin_angle;
	f32 t1 = sin32((t * angle)) / sin_angle; 

	quaternion result;
	result.w = (q0.w * t0 + q1.w * t1);
	result.x = (q0.x * t0 + q1.x * t1);
	result.y = (q0.y * t0 + q1.y * t1);
	result.z = (q0.z * t0 + q1.z * t1);

	return(result);
}

static inline vec3
quaternion_v3_slerp(quaternion q, vec3 v, f32 t)
{
	quaternion q1 = {0, v};
	vec3 result = quaternion_unit_slerp(q, q1, t).vector;
	return(result);
}

static inline vec3 
_quaternion_v3_nlerp(quaternion q0, vec3 v, f32 t)
{
	quaternion result;
	result.w = 0;
	result.x = (q0.x * (1 - t) + v.x * t);
	result.y = (q0.y * (1 - t) + v.y * t);
	result.z = (q0.z * (1 - t) + v.z * t);
	result.vector = vec3_normalize_safe(result.vector);;
	return(result.vector);
}


static inline quaternion
_quaternion_nlerp(quaternion q0, quaternion q1, f32 t)
{
	quaternion result;
	result.w = (q0.w * (1 - t) + q1.w * t);
	result.x = (q0.x * (1 - t) + q1.x * t);
	result.y = (q0.y * (1 - t) + q1.y * t);
	result.z = (q0.z * (1 - t) + q1.z * t);
	result = quaternion_normalize_safe(result);
	return(result);
}

static inline quaternion
quaternion_nlerp(quaternion q0, quaternion q1, f32 t)
{
	//since nlerp at t = 0.5 is the same as slerp at that time, both functions become
	//closer as the get more in line, so this code will be more accurate than a 
	//normal nlerp and can be iterated though as many times as I want in case
	//of more accuracy needed
	quaternion mid = _quaternion_nlerp(q0, q1, 0.5f);
	quaternion result;
	if(t > 0.5f)
	{
		t = (t * 2) - 1;
	    result = (_quaternion_nlerp(mid, q1, t));
	}
	else
	{
		t *= 2;
	    result = (_quaternion_nlerp(q0, mid, t));
	}
	return(result);
}

static inline vec3 
quaternion_v3_nlerp(quaternion q0, vec3 v, f32 t)
{
	quaternion q1 = {0, v};
	return(quaternion_nlerp(q0, q1, t).vector);
}

static quaternion_dual
quaterniond_f32_mul(quaternion_dual qd, f32 s)
{
	qd.w *= s;
	qd.x *= s;
	qd.y *= s;
	qd.z *= s;

	qd.wd *= s;
	qd.xd *= s;
	qd.yd *= s;
	qd.zd *= s;
	return(qd);
}

//real with real, dual with dual
static quaternion_dual
quaterniond_add(quaternion_dual qd0, quaternion_dual qd1)
{
	quaternion_dual qd_result = {0};
	qd_result.qr = quaternion_add(qd0.qr, qd1.qr);
	qd_result.qd = quaternion_add(qd0.qd, qd1.qd);
	return(qd_result);
}

static quaternion_dual
quaterniond_sub(quaternion_dual qd0, quaternion_dual qd1)
{
	quaternion_dual qd_result = {0};
	qd_result.qr = quaternion_sub(qd0.qr, qd1.qr);
	qd_result.qd = quaternion_sub(qd0.qd, qd1.qd);
	return(qd_result);
}

static quaternion_dual
quaterniond_mul(quaternion_dual qd0, quaternion_dual qd1)
{
	quaternion_dual qd_result = {0};
	qd_result.qr = quaternion_mul(qd0.qr, qd1.qr);
	//real con dual, dual con real
	quaternion qd0_result = quaternion_mul(qd0.qr, qd1.qd);
	quaternion qd1_result = quaternion_mul(qd0.qd, qd1.qr);
	qd_result.qd = quaternion_add(qd0_result, qd1_result);
	return(qd_result);
}

static void
quaterniond_fill_rotation_translation(quaternion_dual qdual,
		vec3 *r_ptr, vec3 *t_ptr)
{
	vec3 r = {qdual.x, qdual.y, qdual.z};
	vec3 t = {qdual.xd, qdual.yd, qdual.zd};
	*r_ptr = r;
	*t_ptr = t;
}

static quaternion_dual
quaterniond_from_rotation_translation(f32 angle, vec3 r, vec3 t)
{
	//r + 1/2rt
	//for pure rotation qd_result = [r][0]
	//for pure translation qd_result = [unit_quaternion][0, t]
	quaternion_dual qd_result = {0};
	qd_result.w = angle;
	qd_result.x = r.x;
	qd_result.y = r.y;
	qd_result.z = r.z;
	qd_result.qr = quaternion_normalize_safe(qd_result.qr);

	qd_result.wd = 0;
	qd_result.xd = t.x;
	qd_result.yd = t.y;
	qd_result.zd = t.z;

	qd_result.qd = quaternion_mul(qd_result.qd, qd_result.qr);

	qd_result.xd *= 0.5f;
	qd_result.yd *= 0.5f;
	qd_result.zd *= 0.5f;
	return(qd_result);
}

static quaternion_dual
quaterniond_translation(
		vec3 t)
{
	quaternion_dual qd_result = {1, 0, 0, 0,
		0, t.x * 0.5f, t.y * 0.5f, t.z = 0.5f};
	return(qd_result);
}

static vec3
quaterniond_get_translation(quaternion_dual qdual)
{
	quaternion q_vec = {
		0,
		qdual.xd * 2,
		qdual.yd * 2,
		qdual.zd * 2
	};
	q_vec = quaternion_mul(qdual.qr, q_vec);
	vec3 result = {q_vec.x, q_vec.y, q_vec.z};
	return(result);
}

static quaternion_dual
quaterniond_conjugate(quaternion_dual qdual)
{
	qdual.qr = quaternion_conjugate(qdual.qr);
	qdual.qd = quaternion_conjugate(qdual.qd);
	return(qdual);
}

#define quaterniond_v3_mul_fi quaterniond_v3_mul_foward_inverse
static vec3
quaterniond_v3_mul_foward_inverse(quaternion_dual qdual, vec3 v)
{
	//does not need to be divided by 2
	quaternion_dual q_vec = {1, 0, 0, 0,
		0, v.x, v.y, v.z};
	//third conjugate (only the real part)
	quaternion_dual qdual_inv = qdual;
	qdual_inv.qr = quaternion_conjugate(qdual.qr);

	quaternion_dual q_result = quaterniond_mul(qdual, q_vec);
	q_result = quaterniond_mul(qdual_inv, q_vec);

	vec3 result = {
		q_result.xd,
		q_result.yd,
		q_result.zd
	};

	return(result);
}


inline matrix4x4
matrix4x4_scale(float scale)
{
    matrix4x4 result =
    {
        scale,   0,     0,     0,
        0,       scale, 0,     0,
        0,       0,     1.0f, 0,
        0,       0,     0,     1.0f
    };
    return result;
}
inline matrix4x4 
matrix4x4_Opposite(matrix4x4 matrix)
{
    matrix4x4 result = matrix;
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
          result.m[i][j] *= -1;
        }
    }
    return result;

}


inline matrix3x3 //Rotates around the Z axis
matrix3x3_rotation_scale_x(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix3x3 result = {
        1, 0, 0,
        0, scos, -ssin,
        0, ssin, scos,
    };
    return result;
}

inline matrix3x3
matrix3x3_rotation_scale_y(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix3x3 result = {
        scos, 0, ssin, 
        0,    1,   0,  
        -ssin, 0, scos,
    };

    return result;
}

inline matrix3x3 //Rotates around the Z axis
matrix3x3_rotation_scale_z(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix3x3 result = {
        scos, -ssin, 0,
        ssin,  scos, 0,
        0,     0,    1,
    };

    return result;
}

inline matrix4x4 //Rotates around the Z axis
matrix4x4_rotation_scale_x(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix4x4 result = {
        1, 0, 0, 0,
        0, scos, -ssin, 0,
        0, ssin, scos, 0,
        0, 0, 0, 1
    };
    return result;
}

inline matrix4x4
matrix4x4_rotation_scale_y(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix4x4 result = {
        scos, 0, ssin, 0,
        0,    1,   0,   0,
        -ssin, 0, scos,  0,
        0,    0,   0,   1
    };

    return result;
}

inline matrix4x4 //Rotates around the Z axis
matrix4x4_rotation_scale_z(float scale)
{
    float scos = cos32(scale);
    float ssin = sin32(scale);

    matrix4x4 result = {
        scos, -ssin, 0, 0,
        ssin,  scos, 0, 0,
        0,     0,    1, 0,
        0,     0,    0, 1
    };

    return result;
}

inline matrix4x4
matrix4x4_rotation_scale(float xAngle, float yAngle, float zAngle)
{
	xAngle *= PI;
	yAngle *= PI;
	zAngle *= PI;

    matrix4x4 result = matrix4x4_rotation_scale_z(zAngle);
              result = matrix4x4_mul(result, matrix4x4_rotation_scale_x(xAngle));
			  result = matrix4x4_mul(result, matrix4x4_rotation_scale_y(yAngle));
    return(result);
}


inline matrix4x4
matrix4x4_rotation_angles(float xAngle, float yAngle, float zAngle)
{
    matrix4x4 result = matrix4x4_rotation_scale_z(zAngle);
              result = matrix4x4_mul(result, matrix4x4_rotation_scale_x(xAngle));
			  result = matrix4x4_mul(result, matrix4x4_rotation_scale_y(yAngle));
    return(result);
}

inline matrix4x4
matrix4x4_rotation_degrees(i32 degree_x, i32 degree_y, i32 degree_z)
{
	f32 pi_over_180 = PI / 180.0f;
	f32 xAngle = degree_x * pi_over_180;
	f32 yAngle = degree_y * pi_over_180;
	f32 zAngle = degree_z * pi_over_180;

    matrix4x4 result = matrix4x4_rotation_scale_z(zAngle);
              result = matrix4x4_mul(result, matrix4x4_rotation_scale_x(xAngle));
			  result = matrix4x4_mul(result, matrix4x4_rotation_scale_y(yAngle));
    return(result);
}

inline matrix3x3
matrix3x3_rotation_scale(float xAngle, float yAngle, float zAngle)
{
	xAngle *= PI;
	yAngle *= PI;
	zAngle *= PI;

	//3, 2, 1
    matrix3x3 result = matrix3x3_rotation_scale_z(zAngle);
              result = matrix3x3_mul(result, matrix3x3_rotation_scale_y(yAngle));
			  result = matrix3x3_mul(result, matrix3x3_rotation_scale_x(xAngle));
    return(result);
}

inline matrix3x3
matrix3x3_rotation_degrees(i32 degree_x, i32 degree_y, i32 degree_z)
{
	f32 pi_over_180 = PI / 180.0f;
	f32 xAngle = degree_x * pi_over_180;
	f32 yAngle = degree_y * pi_over_180;
	f32 zAngle = degree_z * pi_over_180;

    matrix3x3 result = matrix3x3_rotation_scale_z(zAngle);
              result = matrix3x3_mul(result, matrix3x3_rotation_scale_y(yAngle));
			  result = matrix3x3_mul(result, matrix3x3_rotation_scale_x(xAngle));
    return(result);
}


inline matrix4x4
matrix4x4_Rotation_Axes_Rows(vec3 xAxis, vec3 yAxis, vec3 zAxis)
{
    matrix4x4 result =
	{
		xAxis.x, xAxis.y, xAxis.z, 0,
		yAxis.x, yAxis.y, yAxis.z, 0,
		zAxis.x, zAxis.y, zAxis.z, 0,
		      0,       0,       0, 0,
	};

    return(result);
}


inline matrix4x4
matrix4x4_Rotation_Axes_Columns(vec3 xAxis, vec3 yAxis, vec3 zAxis)
{
    matrix4x4 result =
	{
		xAxis.x, yAxis.x, zAxis.x, 0,
		xAxis.y, yAxis.y, zAxis.y, 0,
		xAxis.z, yAxis.z, zAxis.z, 0,
		      0,       0,       0, 0,
	};

    return(result);
}

inline matrix4x4
matrix4x4_Translation(float x, float y, float z)
{
    matrix4x4 result = 
    {
        1, 0, 0, x,
        0, 1, 0, y,
        0, 0, 1, z,
        0, 0, 0, 1
    };
    return result;
}

inline matrix4x4 
matrix4x4_Transpose(matrix4x4 m)
{
    //Finish this
    matrix4x4 t;
    t.m[0][0] = m.m[0][0];
    t.m[0][1] = m.m[2][0]; //get x and y
    t.m[0][2] = m.m[0][1];
    t.m[0][3] = m.m[2][1];

    t.m[1][0] = m.m[1][0];
    t.m[1][1] = m.m[3][0]; //get x and y
    t.m[1][2] = m.m[1][1];
    t.m[1][3] = m.m[3][1];

    t.m[2][0] = m.m[0][2];
    t.m[2][1] = m.m[2][2]; //get z and w
    t.m[2][2] = m.m[0][3];
    t.m[2][3] = m.m[2][3];

    t.m[3][0] = m.m[1][2];
    t.m[3][1] = m.m[3][2]; //get z and w
    t.m[3][2] = m.m[1][3];
    t.m[3][3] = m.m[3][3];

    matrix4x4 r;

    r.m[0][0] = t.m[0][0];
    r.m[0][1] = t.m[1][0]; //get x and y
    r.m[0][2] = t.m[0][1];
    r.m[0][3] = t.m[1][1];

    r.m[1][0] = t.m[0][2];
    r.m[1][1] = t.m[1][2]; //get x and y
    r.m[1][2] = t.m[0][3];
    r.m[1][3] = t.m[1][3];

    r.m[2][0] = t.m[2][0];
    r.m[2][1] = t.m[3][0]; //get z and w
    r.m[2][2] = t.m[2][1];
    r.m[2][3] = t.m[3][1];

    r.m[3][0] = t.m[2][2];
    r.m[3][1] = t.m[3][2]; //get z and w
    r.m[3][2] = t.m[2][3];
    r.m[3][3] = t.m[3][3];

    return r;

}
inline matrix3x3
matrix3x3_identity()
{
    matrix3x3 result = 
           {1, 0, 0,
            0, 1, 0,
            0, 0, 1};
    return result;
}

inline matrix4x4
matrix4x4_Identity()
{
    matrix4x4 result = 
           {1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 0,
            0, 0, 0, 1};
    return result;
}
#if 0
inline 
float Length4(vec4 vec)
{
    return sqrt32((vec.x * vec.x) + 
                 (vec.y * vec.y) + 
                 (vec.z * vec.z) + 
                 (vec.w * vec.w));
}
inline float 
Length3(vec3 vec)
{
    return sqrt32((vec.x * vec.x) + 
                 (vec.y * vec.y) + 
                 (vec.z * vec.z)  
                 );
}
#endif
inline real32 
vec4_inner(vec4 one, vec4 two)
{
    return((one.x * two.x) + 
           (one.y * two.y) + 
           (one.z * two.z) + 
           (one.w * two.w)); 
}
inline real32
vec4_inner_squared(vec4 one)
{
   return vec4_inner(one, one);
}
//Note: This probably shouldn't be a vector4.

inline vec3
vec3_all(f32 v)
{
	vec3 result = {v, v, v};
	return(result);
}

inline vec4
vec4_all(real32 v)
{
    vec4 result = {v, v, v, v};
    return(result);
}

inline matrix4x4
matrix4x4_TranslateCol(matrix4x4 m, vec3 position)
{
    matrix4x4 result = m;
    result.m[0][3] = position.x;
    result.m[1][3] = position.y;
    result.m[2][3] = position.z;
    return result;
}
inline matrix4x4
matrix4x4_TranslateRow(matrix4x4 m, vec3 position)
{
    matrix4x4 result = m;
    result.m[3][0] = position.x;
    result.m[3][1] = position.y;
    result.m[3][2] = position.z;
    return result;
}

inline matrix4x4_data
matrix4x4_projection_perspective(uint32 vx, uint32 vy, real32 n, real32 f)
{

    matrix4x4_data result = {0};

   real32 a = 1.0f;
   real32 b = (real32)vx / vy;
   real32 c = (n + f) / (n - f);
   real32 d = (2 * n * f) / (n - f);
   //origin axis = (vx / 2, vy / 2, n);
   matrix4x4 projection = 
   {a,  0,  0,  0,
    0,  b,  0,  0,
    0,  0,  c,  d,
    0,  0,  -1,  0};

   matrix4x4 projection_inverse = 
   {1.0f / a,  0,       0,    0,
    0,  1.0f / b,       0,    0,
    0,         0,       0,   -1,
    0,         0,  1.0f/d,  c/d};

   //0 * 0 + 0 

   result.foward = projection;
   result.inverse = projection_inverse;
#if 0
   matrix4x4 identity = matrix4x4_mul(projection, projection_inverse);
#endif

   return(result);
}

inline matrix4x4_data
matrix4x4_projection_perspective_fov_r(f32 fov, f32 aspect, real32 n, real32 f)
{

    matrix4x4_data result = {0};
	f32 tan_half = tan32(fov * 0.5f);

   real32 a = 1.0f / (aspect * tan_half);
   real32 b = 1 / tan_half;
   real32 c = (n + f) / (n - f);
   real32 d = (2 * n * f) / (n - f);
   //origin axis = (vx / 2, vy / 2, n);
   matrix4x4 projection = 
   {a,  0,  0,  0,
    0,  b,  0,  0,
    0,  0,  c,  d,
    0,  0,  -1,  0};

   matrix4x4 projection_inverse = 
   {1.0f / a,  0,       0,    0,
    0,  1.0f / b,       0,    0,
    0,         0,       0,   -1,
    0,         0,  1.0f/d,  c/d};

   //0 * 0 + 0 

   result.foward = projection;
   result.inverse = projection_inverse;
#if 0
   matrix4x4 identity = matrix4x4_mul(projection, projection_inverse);
#endif

   return(result);
}

inline matrix4x4_data
matrix4x4_projection_orthographic(f32 up, f32 down, f32 left, f32 right) 
{
   f32 a = 2.0f / (right - left);
   f32 b = 2.0f / (up - down);
   f32 d = (f32)-(right + left) / (right - left);
   f32 e = (f32)-(up + down) / (up - down);
   //a = 1.0f;
   //b = (real32)vx / vy;
   //origin axis = (vx / 2, vy / 2, n);
   matrix4x4 projection = 
   {a,  0,  0,  d, //This shouldn't be -1 if the viewport is smaller!
    0,  b,  0,  e,
    0,  0,  1,  0,
    0,  0,  0,  1}; //FOV near z , width and height translated

   matrix4x4 projection_inverse = 
   {1.0f / a,  0,  0,  -d, //This shouldn't be -1 if the viewport is smaller!
    0, 1.0f / b,  0,  -e,
    0,  0,  1, 0,
    0,  0,  0, 1}; //FOV near z , width and height translated
   matrix4x4_data r = {0};
   r.foward = projection;
   r.inverse = projection_inverse;
//   matrix4x4 i = matrix4x4_mul(
//		   r.inverse, r.foward);

   return(r);
}

inline matrix4x4_data
matrix4x4_projection_orthographic_depth(f32 up, f32 down, f32 left, f32 right) 
{
   f32 a = 2.0f / (right - left);
   f32 b = 2.0f / (up - down);
   f32 d = (f32)-(right + left) / (right - left);
   f32 e = (f32)-(up + down) / (up - down);

   f32 n = 0.01f;
   f32 f = 10000.0f;

   f32 fd = -(f + n)  / (f - n);
   f32 gd = -(2 * f * n) / (f - n);
   //f32 fd = (n + f) / (n - f);
   //f32 gd = (2 * n * f) / (n - f);
   //f32 f = 0.01f;
   //f32 n = 10000.0f;
   //f32 fd = 1 / (f - n); 
   //f32 gd = -n / (f - n);
   //a = 1.0f;
   //b = (real32)vx / vy;
   //origin axis = (vx / 2, vy / 2, n);
   matrix4x4 projection = 
   {a,  0,  0,  d, //This shouldn't be -1 if the viewport is smaller!
    0,  b,  0,  e,
    0,  0,  fd,  gd,
    0,  0,  -1,  0}; //FOV near z , width and height translated

   matrix4x4 projection_inverse = 
   {1.0f / a,  0,  0,  -d, //This shouldn't be -1 if the viewport is smaller!
    0, 1.0f / b,  0,  -e,
    0,  0,  0, -1,
    0,  0,  1.0f / gd,   fd / gd}; //FOV near z , width and height translated

   matrix4x4_data r = {0};
   r.foward = projection;
   r.inverse = projection_inverse;

//  matrix4x4 i = matrix4x4_mul(
//		   r.inverse, r.foward);
   return(r);
}


inline matrix4x4_data 
matrix4x4_camera_transform(vec3 xAxis, vec3 yAxis, vec3 zAxis, vec3 CameraPosition)
{
    //Rotates the translation vector before translating it to the matrix
    matrix4x4 rotation_matrix = matrix4x4_rows_from_vectors(xAxis, yAxis, zAxis);
    vec3 camera_translated = matrix4x4_v3_mul_rows(rotation_matrix, vec3_opposite(CameraPosition), 1);
    matrix4x4 transformationMatrix = matrix4x4_TranslateCol(rotation_matrix, camera_translated);

    //divide by the length squared to prevent a scaled rotation.
    vec3 iXA = vec3_f32_div(xAxis, vec3_inner(xAxis, xAxis));
    vec3 iYA = vec3_f32_div(yAxis, vec3_inner(yAxis, yAxis));
    vec3 iZA = vec3_f32_div(zAxis, vec3_inner(zAxis, zAxis));
    vec3 inverse_camera_transform = {
        (iXA.x * camera_translated.x) + (iYA.x * camera_translated.y) + (iZA.x * camera_translated.z),
        (iXA.y * camera_translated.x) + (iYA.y * camera_translated.y) + (iZA.y * camera_translated.z),
        (iXA.z * camera_translated.x) + (iYA.z * camera_translated.y) + (iZA.z * camera_translated.z),
    };

    matrix4x4 invTMatrix = 
   {iXA.x,  iYA.x,  iZA.x,  -inverse_camera_transform.x,
    iXA.y,  iYA.y,  iZA.y,  -inverse_camera_transform.y,
    iXA.z,  iYA.z,  iZA.z,  -inverse_camera_transform.z,
    0,          0,      0,  1};

//    matrix4x4 identity = matrix4x4_mul(invTMatrix, transformationMatrix);

    matrix4x4_data result;
    result.foward = transformationMatrix;
    result.inverse = invTMatrix;
    return(result); 
}

#define f32_lerp(l, delta, r) real32_Lerp(l, delta, r)
inline real32
real32_Lerp(real32 left, real32 delta, real32 right)
{
    return(left * (1 - delta) +  right * delta);
}

inline f32
f32_curve_lerp(f32 f0, f32 f1, f32 f2, f32 t)
{
    f32 a = (1 - t) * (1 - t);
	f32 b = 2 * (1 - t) * t;
	f32 c = t * t;
	f32 result = (a * f0) + (b * f1) + (c * f2);
	return(result);
}

inline vec3
vec3_Lerp(vec3 left, real32 delta, vec3 right)
{
    vec3 result;
    result.x = real32_Lerp(left.x, delta, right.x);
    result.y = real32_Lerp(left.y, delta, right.y);
    result.z = real32_Lerp(left.z, delta, right.z);
    return(result);
}

inline vec2
vec2_lerp(vec2 v0, f32 delta, vec2 v1)
{
	vec2 result = {0};
	result.x = real32_Lerp(v0.x, delta, v1.x);
	result.y = real32_Lerp(v0.y, delta, v1.y);
	return(result);
}

inline vec2
vec2_curve_lerp(vec2 v0, vec2 v1, vec2 v2, f32 t)
{
	vec2 result = {0};
	result.x = f32_curve_lerp(v0.x, v1.x, v2.x, t);
	result.y = f32_curve_lerp(v0.y, v1.y, v2.y, t);

	return(result);
}

inline vec2
vec2_rotate(vec2 v, f32 angle)
{
	f32 ca = cos32(angle);
	f32 sa = sin32(angle);
	f32 x = (v.x * ca) - (v.y * sa);
	f32 y = (v.x * sa) + (v.y * ca);
	v.x = x;
	v.y = y;
	return(v);
}

inline real32
real32_Clamp(real32 v, real32 min, real32 max)
{
    return(v < min ? min : v > max ? max : v);
}
inline real32
real32_Fract(real32 v)
{
    return v - (int32)v;
}
inline real32
vec2_length(vec2 vec)
{
    return(sqrt32((vec.x * vec.x) + (vec.y * vec.y)));
}

inline real32
vec4_length(vec4 vec)
{
    return(sqrt32((vec.x * vec.x) + (vec.y * vec.y) + (vec.z * vec.z) + (vec.w * vec.w)));
}

inline vec2
vec2_normalize(vec2 vec)
{
    real32 OneOverLenght = 1.0f / vec2_length(vec);
    vec.x *= OneOverLenght;
    vec.y *= OneOverLenght;
    return(vec);
}

inline vec2
vec2_normalize_zero(vec2 vec)
{
    f32 length_squared = vec2_inner(vec, vec);
    //lengthSquared = lengthSquared * lengthSquared;

    if(length_squared > (0.0001f * 0.0001f))
    {
       f32 o = 1.0f / sqrt32(length_squared);
       vec.x *= o; 
       vec.y *= o;
    }
    return(vec);
}


inline vec2
vec2_max(vec2 v1, vec2 v2)
{
	vec2 result;
	result.x = MAX(v1.x, v2.x);
	result.y = MAX(v1.y, v2.y);
	return(result);
}

inline vec2
vec2_min(vec2 v1, vec2 v2)
{
	vec2 result;
	result.x = MIN(v1.x, v2.x);
	result.y = MIN(v1.y, v2.y);
	return(result);
}

inline vec4
vec4_Normalize(vec4 vec)
{
    float l = 1.0f / vec4_length(vec);
   vec4 result = {
       vec.x * l, 
       vec.y * l, 
       vec.z * l, 
       vec.w * l, 
   };
   return result;
}
//Note(Agu): Camera planes for detecting objects inside them.
//Ej: object.x >= 0.5f if it's inside the left plane
inline vec4
vec4_PlaneLeft(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] + m.m[0][0],
   m.m[3][1] + m.m[0][1],
   m.m[3][2] + m.m[0][2],
   m.m[3][3] + m.m[0][3]};
  return(result);
}

inline vec4
vec4_PlaneRight(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] - m.m[0][0],
   m.m[3][1] - m.m[0][1],
   m.m[3][2] - m.m[0][2],
   m.m[3][3] - m.m[0][3]};
  return(result);
}

inline vec4
vec4_PlaneBottom(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] + m.m[1][0],
   m.m[3][1] + m.m[1][1],
   m.m[3][2] + m.m[1][2],
   m.m[3][3] + m.m[1][3]};
  return(result);
}

inline vec4
vec4_PlaneTop(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] - m.m[1][0],
   m.m[3][1] - m.m[1][1],
   m.m[3][2] - m.m[1][2],
   m.m[3][3] - m.m[1][3]};
  return(result);
}

inline vec4
vec4_PlaneNear(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] + m.m[2][0],
   m.m[3][1] + m.m[2][1],
   m.m[3][2] + m.m[2][2],
   m.m[3][3] + m.m[2][3]};
  return(result);
}

inline vec4
vec4_PlaneFar(matrix4x4 m)
{
  vec4 result =
  {m.m[3][0] - m.m[2][0],
   m.m[3][1] - m.m[2][1],
   m.m[3][2] - m.m[2][2],
   m.m[3][3] - m.m[2][3]};
  return(result);
}

#define vec3_cross_normalized(origin, xAxis, yAxis) vec3_normalize(vec3_GetZAxis(origin, xAxis, yAxis))

inline vec3
vec3_GetZAxis(vec3 origin, vec3 xAxis, vec3 yAxis)
{
  vec3 p0    = origin;
  vec3 p1    = vec3_add(p0, xAxis); 
  vec3 p2    = vec3_add(p0, yAxis);
  vec3 zAxis = vec3_cross(vec3_sub(p1, p0), vec3_sub(p2, p0));
  return(zAxis);
}
#if 0
//assumes you pass an already built and rotated matrix
inline matrix4x4
matrix4x4_rotate_keep_scale(matrix4x4 m)
{

		        vec3 grid_xAxis = matrix4x4_v3_get_column(gridRotation, 0);
		        vec3 grid_yAxis = matrix4x4_v3_get_column(gridRotation, 1);
		        vec3 grid_zAxis = matrix4x4_v3_get_column(gridRotation, 2);



				f32 dot_x = grid_yAxis.x;
				f32 dot_y = grid_yAxis.y;
				f32 dot_z = grid_yAxis.z;

				f32 dot_x1 = grid_xAxis.x;
				f32 dot_y1 = grid_xAxis.y;
				f32 dot_z1 = grid_xAxis.z;

				f32 xScale1 = 10000.0f;
				f32 yScale1 = 10000.0f;
				f32 zScale1 = 10000.0f;

				f32 xScale = 10000.0f;
				f32 yScale = 10000.0f;
				f32 zScale = 10000.0f;
				if(dot_x)
				{
				    xScale = ABS(1.0f / dot_x);
				}
				if(dot_y)
				{
				    yScale = ABS(1.0f / dot_y);
				}
				if(dot_z)
				{
				    zScale = ABS(1.0f / dot_z);
				}

				if(dot_x1)
				{
				    xScale1 = ABS(1.0f / dot_x1);
				}
				if(dot_y1)
				{
				    yScale1 = ABS(1.0f / dot_y1);
				}
				if(dot_z1)
				{
				    zScale1 = ABS(1.0f / dot_z1);
				}

				if(zScale < yScale)
				{
					yScale = zScale;
				}
				if(xScale < yScale)
				{
					yScale = xScale;
				}

				if(zScale1 < yScale1)
				{
					yScale1 = zScale1;
				}
				if(xScale1 < yScale1)
				{
					yScale1 = xScale1;
				}
				
				//grid_yAxis.x *= yScale;
				//grid_yAxis.y *= yScale;
				//grid_yAxis.z *= yScale;

				//grid_xAxis.x *= yScale1;
				//grid_xAxis.y *= yScale1;
				//grid_xAxis.z *= yScale1;

}
#endif
