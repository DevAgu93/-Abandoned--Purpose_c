

////////===============
#if 0
static lane_uint32 lane_U32(uint32 v1, uint32 v2, uint32 v3, uint32 v4);
static lane_uint32 lane_U32_FROM_1U32(uint32 v);
static lane_real32 lane_F32_FROM_1U32(uint32 v);
static lane_real32 lane_F32_FROM_1F32(real32 v);
//Assignments
static lane_vec3 lane_V3(lane_real32 x, lane_real32 y, lane_real32 z);
static lane_vec3 lane_V3_FROM_1V3(vec3 v);
static lane_vec3 lane_V3_FROM_1F32(real32 v);
//Comparisons
static lane_uint32 lane_U32_LT(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_GT(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_EQ(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_NEQ(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_1U32_LT(lane_uint32 l, uint32 v);
static lane_uint32 lane_U32_1U32_GT(lane_uint32 l, uint32 v);
static lane_uint32 lane_U32_1U32_NEQ(lane_uint32 l, uint32 v);
static lane_uint32 lane_F32_EQ(lane_real32 l1, lane_real32 l2);
static lane_uint32 lane_F32_NEQ(lane_real32 l1, lane_real32 l2);
static lane_uint32 lane_F32_LT(lane_real32 l1, lane_real32 l2);
static lane_uint32 lane_F32_GT(lane_real32 l1, lane_real32 l2);
static lane_uint32 lane_F32_1F32_LT(lane_real32 l, real32 v);
static lane_uint32 lane_F32_1F32_GT(lane_real32 l, real32 v);
//End Comparisons
static lane_uint32 lane_U32_Min(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Max(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Or(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_And(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Xor(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_ShiftL(lane_uint32 l, uint32 amount);
static lane_uint32 lane_U32_ShiftR(lane_uint32 l, uint32 amount);
static lane_uint32 lane_U32_Add(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Sub(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Mul(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_Div(lane_uint32 l1, lane_uint32 l2);
static lane_uint32 lane_U32_1U32_Add(lane_uint32 l1, uint32 v);
static lane_uint32 lane_U32_1U32_Sub(lane_uint32 l1, uint32 v);
static lane_uint32 lane_U32_1U32_Mul(lane_uint32 l1, uint32 v);
static lane_uint32 lane_U32_1U32_Div(lane_uint32 l1, uint32 v);
//Floating point
static lane_real32 lane_F32_Min(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Max(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Clamp(lane_real32 l);
static lane_real32 lane_F32_Or(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_And(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_U32_And(lane_real32 l1, lane_uint32 l2);
static lane_real32 lane_F32_Xor(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Neg(lane_real32 l);
static lane_real32 lane_F32_Add(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Sub(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Mul(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Div(lane_real32 l1, lane_real32 l2);
static lane_real32 lane_F32_Sqrt(lane_real32 l);
static lane_real32 lane_F32_1F32_Add(lane_real32 l1, real32 v);
static lane_real32 lane_F32_1F32_Sub(lane_real32 l1, real32 v);
static lane_real32 lane_F32_1F32_SubR(real32 v, lane_real32 l1);
static lane_real32 lane_F32_1F32_Mul(lane_real32 l1, real32 v);
static lane_real32 lane_F32_1F32_Div(lane_real32 l, real32 divisor);
static lane_real32 lane_F32_V3_Add(lane_real32 l1, lane_vec3 l2);
static lane_real32 lane_F32_V3_Mul(lane_real32 l1, lane_vec3 l2);
static lane_real32 lane_F32_V3_Div(lane_real32 l1, lane_vec3 l2);
static lane_real32 lane_F32_Lerp(lane_real32 left, lane_real32 delta, lane_real32 right);
//Floating point
static lane_vec3 lane_V3_Add(lane_vec3 l1, lane_vec3 l2);
static lane_vec3 lane_V3_Sub(lane_vec3 l1, lane_vec3 l2);
static lane_vec3 lane_V3_Mul(lane_vec3 l1, lane_vec3 l2);
static lane_vec3 lane_V3_Div(lane_vec3 l1, lane_vec3 l2);
static lane_vec3 lane_V3_F32_Add(lane_vec3 l, lane_real32 v);
static lane_vec3 lane_V3_F32_Sub(lane_vec3 l, lane_real32 v);
static lane_vec3 lane_V3_F32_SubR(lane_real32 l1, lane_vec3 l2);
static lane_vec3 lane_V3_F32_Mul(lane_vec3 l, lane_real32 v);
static lane_vec3 lane_V3_1F32_Mul(lane_vec3 l, real32 v);
static lane_real32 lane_V3_Inner(lane_vec3 l1, lane_vec3 l2);
static lane_vec3 lane_V3_NormalizeZero(lane_vec3 l1);
static lane_vec3 lane_V3_Lerp(lane_vec3 left, lane_real32 delta, lane_vec3 right);
//Convertions
static lane_real32 lane_U32_CAST_F32(lane_uint32 l);
static lane_uint32 lane_F32_CAST_U32(lane_real32 l);
//Comparison set
static lane_uint32lane_U32_AndNot(lane_uint32 mask, lane_uint32 l);
static lane_real32 lane_F32_AndNot(lane_real32 mask, lane_real32 l);
static void lane_U32_ConditionalSet(lane_uint32 mask, lane_uint32 *s, lane_uint32 v);
static void lane_F32_ConditionalSet(lane_real32 mask, lane_real32 *s, lane_real32 v);
static void lane_F32_ConditionalSet_U32M(lane_uint32 mask, lane_real32 *s, lane_real32 v);
static void lane_V3_ConditionalSet(lane_real32 mask, lane_vec3 *s, lane_vec3 v);
static void lane_V3_ConditionalSet_U32M(lane_uint32 mask, lane_vec3 *s, lane_vec3 v);

#define lane_Gather_U32(basePtr, index, member) lane_Gather_U32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_F32(basePtr, index, member) lane_Gather_F32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_V3(basePtr, index, member) lane_Gather_V3_(&(basePtr)->member, sizeof(*(basePtr)), index)
static lane_uint32lane_Gather_U32_(void *basePtr, uint32 stride, lane_uint32 indices);
static lane_real32 lane_Gather_F32_(void *basePtr, uint32 stride, lane_uint32 indices);
static lane_vec3 lane_Gather_V3_(void *basePtr, uint32 stride, lane_uint32 indices);
static int32 lane_MaskIsZero(lane_uint32 mask);
//Note(Agu): these will not be as precise, use carefully.
static real32 lane_HorizontalAdd_F32(lane_real32 l);
static uint32 lane_HorizontalAdd_U32(lane_uint32 l);
static vec3 lane_HorizontalAdd_V3(lane_vec3 l);
#endif

//////////////================================

#if LANEWIDTH == 4

#include <intrin.h>
typedef struct
{
    __m128i v;

}lane_uint32;

typedef struct
{

    __m128 v;
}lane_real32;

typedef struct
{
    lane_real32 x;
    lane_real32 y;
    lane_real32 z;

}lane_vec3;

static lane_uint32
lane_U32(uint32 v1, uint32 v2, uint32 v3, uint32 v4)
{
    lane_uint32 result = {0};
    result.v = _mm_set_epi32(v1, v2, v3, v4);
    return(result);
}

static lane_uint32
lane_U32_FROM_1U32(uint32 v)
{
    lane_uint32 result = {0};
    result.v = _mm_set1_epi32(v);
    return(result);
}

static lane_real32
lane_F32_FROM_1U32(uint32 v)
{
    lane_real32 result = {0};
    result.v = _mm_set1_ps((real32)v);
    return(result);
}

static lane_real32
lane_F32_FROM_1F32(real32 v)
{
    lane_real32 result = {0};
    result.v = _mm_set1_ps(v);
    return(result);
}

//Convertions
static lane_real32
lane_U32_CAST_F32(lane_uint32 l)
{
    lane_real32 result = {0};
    result.v = _mm_castsi128_ps(l.v);
    return(result);
}

static lane_uint32
lane_F32_CAST_U32(lane_real32 l)
{
    lane_uint32 result = {0};
    result.v = _mm_castps_si128(l.v);
    return(result);
}

static lane_real32
lane_U32_CVT_F32(lane_uint32 l)
{
    lane_real32 result = {0};
    result.v = _mm_cvtepi32_ps(l.v);
    return(result);
}

static lane_uint32
lane_F32_CVT_U32(lane_real32 l)
{
    lane_uint32 result = {0};
    result.v = _mm_cvtps_epi32(l.v);
    return(result);
}


//Assignments
static lane_vec3
lane_V3(lane_real32 x, lane_real32 y, lane_real32 z)
{
    lane_vec3 result = {x, y, z};
    return(result);
}

static lane_vec3
lane_V3_FROM_1V3(vec3 v)
{
    lane_vec3 result = {0};
    result.x = lane_F32_FROM_1F32(v.x);
    result.y = lane_F32_FROM_1F32(v.y);
    result.z = lane_F32_FROM_1F32(v.z);
    return (result);
}

static lane_vec3
lane_V3_FROM_1F32(real32 v)
{
    lane_vec3 result = {0};
    lane_real32 r = lane_F32_FROM_1F32(v);
    result.x = r;
    result.y = r;
    result.z = r;
    return (result);
}

//Comparisons

static lane_uint32
lane_U32_LT(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_cmplt_epi32(l1.v, l2.v);
   return(result);
}

static lane_uint32
lane_U32_GT(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_cmpgt_epi32(l1.v, l2.v);
   return(result);
}

static lane_uint32
lane_U32_EQ(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_cmpeq_epi32(l1.v, l2.v);
   return(result);
}

static lane_uint32
lane_U32_NEQ(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
  // result.v = _mm_andnot_si128(lane_U32_EQ(l1, l2).v, _mm_set1_epi32(0xffffffff)); 
   result.v = _mm_xor_si128(lane_U32_EQ(l1, l2).v, _mm_set1_epi32(0xffffffff)); 
   //asd
   return(result);
}

static lane_uint32
lane_U32_1U32_LT(lane_uint32 l, uint32 v)
{
   return(lane_U32_LT(l, lane_U32_FROM_1U32(v)));
}

static lane_uint32
lane_U32_1U32_GT(lane_uint32 l, uint32 v)
{
   return(lane_U32_GT(l, lane_U32_FROM_1U32(v)));
}

static lane_uint32
lane_U32_1U32_NEQ(lane_uint32 l, uint32 v)
{
   lane_uint32 result = lane_U32_NEQ(l, lane_U32_FROM_1U32(v)); 
   return(result);
}


static lane_uint32
lane_F32_EQ_U32M(lane_real32 l1, lane_real32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_castps_si128(_mm_cmpeq_ps(l1.v, l2.v));
   return(result);
}

static lane_uint32
lane_F32_NEQ_U32M(lane_real32 l1, lane_real32 l2)
{
    //This one actually has a neq, not like U32
   lane_uint32 result = {0};
   result.v = _mm_castps_si128(_mm_cmpneq_ps(l1.v, l2.v));
   return(result);
}
//TODO(Agu): Do a non-cast version of this to keep a real32 mask
static lane_uint32
lane_F32_LT(lane_real32 l1, lane_real32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_castps_si128(_mm_cmplt_ps(l1.v, l2.v));
   return(result);
}

static lane_uint32
lane_F32_GT(lane_real32 l1, lane_real32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm_castps_si128(_mm_cmpgt_ps(l1.v, l2.v));
   return(result);
}

static lane_uint32
lane_F32_1F32_LT(lane_real32 l, real32 v)
{
   return(lane_F32_LT(l, lane_F32_FROM_1F32(v)));
}

static lane_uint32
lane_F32_1F32_GT(lane_real32 l, real32 v)
{
   return(lane_F32_GT(l, lane_F32_FROM_1F32(v)));
}

//End Comparisons

static lane_uint32
lane_U32_Min(lane_uint32 l1, lane_uint32 l2)
{
    l1.v = _mm_min_epi32(l1.v, l2.v);
    return(l1);
}

static lane_uint32
lane_U32_Max(lane_uint32 l1, lane_uint32 l2)
{
    l1.v = _mm_max_epi32(l1.v, l2.v);
    return(l1);
}

static lane_uint32
lane_U32_Or(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm_or_si128(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_And(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm_and_si128(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_Xor(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm_xor_si128(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_ShiftL(lane_uint32 l, uint32 amount)
{
   lane_uint32 result = {0};
   result.v = _mm_slli_epi32(l.v, amount);
   return(result);
}

static lane_uint32
lane_U32_ShiftR(lane_uint32 l, uint32 amount)
{
   lane_uint32 result = {0};
   result.v = _mm_srli_epi32(l.v, amount);
   return(result);
}

static lane_uint32
lane_U32_Add(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm_add_epi32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Sub(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm_sub_epi32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Mul(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm_mul_epu32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Div(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm_div_epi32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_1U32_Add(lane_uint32 l1, uint32 v)
{
    lane_uint32 l2 = lane_U32_FROM_1U32(v);
    lane_uint32 result = lane_U32_Add(l1, l2);
    return(result);
}

static lane_uint32
lane_U32_1U32_Sub(lane_uint32 l1, uint32 v)
{
    lane_uint32 l2 = lane_U32_FROM_1U32(v);
    lane_uint32 result = lane_U32_Sub(l1, l2);
    return(result);
}

static lane_uint32
lane_U32_1U32_Mul(lane_uint32 l1, uint32 v)
{
    lane_uint32 l2 = lane_U32_FROM_1U32(v);
    lane_uint32 result = lane_U32_Mul(l1, l2);
    return(result);
}

static lane_uint32
lane_U32_1U32_Div(lane_uint32 l1, uint32 v)
{
    lane_uint32 l2 = lane_U32_FROM_1U32(v);
    lane_uint32 result = lane_U32_Div(l1, l2);
    return(result);
}

//Floating point
static lane_real32
lane_F32_Min(lane_real32 l1, lane_real32 l2)
{
    l1.v = _mm_min_ps(l1.v, l2.v);
    return(l1);
}

static lane_real32
lane_F32_Max(lane_real32 l1, lane_real32 l2)
{
    l1.v = _mm_max_ps(l1.v, l2.v);
    return(l1);
}

static lane_real32
lane_F32_Clamp(lane_real32 l)
{
    l = lane_F32_Min(lane_F32_Max(l, lane_F32_FROM_1F32(0)), lane_F32_FROM_1F32(1));
    return(l);
}

static lane_real32
lane_F32_Or(lane_real32 l1, lane_real32 l2)
{
   l1.v = _mm_or_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_And(lane_real32 l1, lane_real32 l2)
{
   l1.v = _mm_and_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_Xor(lane_real32 l1, lane_real32 l2)
{
   l1.v = _mm_xor_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_Neg(lane_real32 l)
{
    l.v = _mm_sub_ps(_mm_set1_ps(0), l.v);
    return(l);
}
static lane_real32
lane_F32_Add(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm_add_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Sub(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm_sub_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Mul(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm_mul_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Div(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm_div_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Sqrt(lane_real32 l)
{
    lane_real32 result = {0};
    result.v = _mm_sqrt_ps(l.v);
    return(result);
}

static lane_real32
lane_F32_1F32_Add(lane_real32 l1, real32 v)
{
    lane_real32 l2 = lane_F32_FROM_1F32(v);
    lane_real32 result = lane_F32_Add(l1, l2);
    return(result);
}

static lane_real32
lane_F32_1F32_Sub(lane_real32 l1, real32 v)
{
    lane_real32 l2 = lane_F32_FROM_1F32(v);
    lane_real32 result = lane_F32_Sub(l1, l2);
    return(result);
}

static lane_real32
lane_F32_1F32_SubR(real32 v, lane_real32 l1)
{
    lane_real32 l2 = lane_F32_FROM_1F32(v);
    lane_real32 result = lane_F32_Sub(l2, l1);
    return(result);
}

static lane_real32
lane_F32_1F32_Mul(lane_real32 l1, real32 v)
{
    lane_real32 l2 = lane_F32_FROM_1F32(v);
    lane_real32 result = lane_F32_Mul(l1, l2);
    return(result);
}

static lane_real32
lane_F32_1F32_Div(lane_real32 l1, real32 divisor)
{
    lane_real32 l2 = {_mm_set1_ps(divisor)};
    lane_real32 result = lane_F32_Div(l1, l2);  
    return(result);
}

static lane_real32
lane_F32_1F32_DivR(real32 divisor, lane_real32 l1)
{
    lane_real32 l2 = {_mm_set1_ps(divisor)};
    lane_real32 result = lane_F32_Div(l2, l1);
    return(result);
}


static lane_real32
lane_F32_U32_And(lane_real32 l1, lane_uint32 l2)
{
   //return(lane_U32_CAST_F32(lane_U32_And(lane_F32_CAST_U32(l1), l2)));
    lane_real32 result = {0};
    //result.v = _mm_castsi128_ps(_mm_and_si128(_mm_castps_si128(l1.v), l2.v));
    result.v = _mm_and_ps(l1.v, _mm_castsi128_ps(l2.v));
   return(result);
}

static lane_real32
lane_F32_Lerp(lane_real32 left, lane_real32 delta, lane_real32 right)
{
    lane_real32 result = lane_F32_Add(lane_F32_Mul(left, lane_F32_1F32_SubR(1.0f, delta)), lane_F32_Mul(right, delta));
    return(result);
}


//Floating point
static lane_vec3
lane_V3_Add(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Add(l1.x, l2.x);
    l1.y = lane_F32_Add(l1.y, l2.y);
    l1.z = lane_F32_Add(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Sub(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Sub(l1.x, l2.x);
    l1.y = lane_F32_Sub(l1.y, l2.y);
    l1.z = lane_F32_Sub(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Mul(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Mul(l1.x, l2.x);
    l1.y = lane_F32_Mul(l1.y, l2.y);
    l1.z = lane_F32_Mul(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Div(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Div(l1.x, l2.x);
    l1.y = lane_F32_Div(l1.y, l2.y);
    l1.z = lane_F32_Div(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_F32_Add(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Add(l.x, v);  
    l.y = lane_F32_Add(l.y, v);  
    l.z = lane_F32_Add(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_F32_Sub(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Sub(l.x, v);  
    l.y = lane_F32_Sub(l.y, v);  
    l.z = lane_F32_Sub(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_F32_SubR(lane_real32 v, lane_vec3 l)
{
    l.x = lane_F32_Sub(v, l.x);  
    l.y = lane_F32_Sub(v, l.y);  
    l.z = lane_F32_Sub(v, l.z);  
    return(l);
}

static lane_vec3
lane_V3_F32_Mul(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Mul(l.x, v);  
    l.y = lane_F32_Mul(l.y, v);  
    l.z = lane_F32_Mul(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_F32_And(lane_vec3 l1, lane_real32 l2)
{
   l1.x = lane_F32_And(l1.x, l2);
   l1.y = lane_F32_And(l1.y, l2);
   l1.z = lane_F32_And(l1.z, l2);
   return(l1);
}

static lane_vec3
lane_V3_U32_And(lane_vec3 l1, lane_uint32 l2)
{
   l1.x = lane_F32_U32_And(l1.x, l2);
   l1.y = lane_F32_U32_And(l1.y, l2);
   l1.z = lane_F32_U32_And(l1.z, l2);
   return(l1);
}

static lane_vec3
lane_V3_1F32_Mul(lane_vec3 l, real32 v)
{
    lane_real32 r = lane_F32_FROM_1F32(v);
    l = lane_V3_F32_Mul(l, r);  
    return(l);
}

static lane_real32
lane_V3_Inner(lane_vec3 l1, lane_vec3 l2)
{
    lane_real32 result = lane_F32_Add(lane_F32_Add(lane_F32_Mul(l1.x, l2.x), lane_F32_Mul(l1.y, l2.y)), lane_F32_Mul(l1.z, l2.z));
    return(result);
}

static void 
lane_V3_ConditionalSet_U32M(lane_uint32 mask, lane_vec3 *s, lane_vec3 v);
static lane_vec3
lane_V3_NormalizeZero(lane_vec3 l1)
{
  lane_real32 lengthSquared = lane_V3_Inner(l1, l1);
  //TODO(Agu): the if statement:!! if(lengthSquared > (0.0001f * 0.0001f))
  lane_uint32 assignMask = (lane_F32_GT(lengthSquared, lane_F32_FROM_1F32(0.0001f * 0.0001f)));
  lane_real32 o = lane_F32_1F32_DivR(1.0f, lane_F32_Sqrt(lengthSquared));

  lane_V3_ConditionalSet_U32M(assignMask , &l1, lane_V3_F32_Mul(l1, o));
  return(l1);
}

static lane_vec3
lane_V3_Lerp(lane_vec3 left, lane_real32 delta, lane_vec3 right)
{
    lane_vec3 result;
    result.x = lane_F32_Lerp(left.x, delta, right.x);
    result.y = lane_F32_Lerp(left.y, delta, right.y);
    result.z = lane_F32_Lerp(left.z, delta, right.z);
    return(result);
}
//Comparison set

static lane_uint32
lane_U32_AndNot(lane_uint32 mask, lane_uint32 l)
{
    lane_uint32 result = {0};
    result.v = _mm_andnot_si128(mask.v, l.v);
    return(result);
}

static lane_real32
lane_F32_AndNot(lane_real32 mask, lane_real32 l)
{
    lane_real32 result = {0};
    result.v = _mm_andnot_ps(mask.v, l.v);
    return(result);
}

static void 
lane_U32_ConditionalSet(lane_uint32 mask, lane_uint32 *s, lane_uint32 v)
{
    //*s = ((~mask & *s) | (mask & v));
    *s = lane_U32_Or(lane_U32_AndNot(mask, *s), lane_U32_And(mask, v));
}


static void 
lane_F32_ConditionalSet(lane_real32 mask, lane_real32 *s, lane_real32 v)
{
    *s = lane_F32_Or(lane_F32_AndNot(mask, *s), lane_F32_And(mask, v));
}

static void 
lane_F32_ConditionalSet_U32M(lane_uint32 mask, lane_real32 *s, lane_real32 v)
{
    lane_real32 maskPs = lane_U32_CAST_F32(mask); 
    lane_F32_ConditionalSet(maskPs, s, v);
}
     
static void 
lane_V3_ConditionalSet(lane_real32 mask, lane_vec3 *s, lane_vec3 v)
{
   lane_F32_ConditionalSet(mask, &s->x, v.x);
   lane_F32_ConditionalSet(mask, &s->y, v.y);
   lane_F32_ConditionalSet(mask, &s->z, v.z);
}

static void 
lane_V3_ConditionalSet_U32M(lane_uint32 mask, lane_vec3 *s, lane_vec3 v)
{
   lane_V3_ConditionalSet(lane_U32_CAST_F32(mask), s, v);
}

//Gather

#define lane_Gather_U32(basePtr, index, member) lane_Gather_U32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_F32(basePtr, index, member) lane_Gather_F32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_V3(basePtr, index, member) lane_Gather_V3_(&(basePtr)->member, sizeof(*(basePtr)), index)


static lane_uint32
lane_Gather_U32_(void *basePtr, uint32 stride, lane_uint32 indices)
{
    uint32 *vI = (uint32 *)&indices.v;
    lane_uint32 result = {0};
    result.v = _mm_setr_epi32(*(uint32 *)((uint8 *)basePtr + vI[0] * stride),
                              *(uint32 *)((uint8 *)basePtr + vI[1] * stride),
                              *(uint32 *)((uint8 *)basePtr + vI[2] * stride),
                              *(uint32 *)((uint8 *)basePtr + vI[3] * stride));
    return(result);
}

static lane_real32
lane_Gather_F32_(void *basePtr, uint32 stride, lane_uint32 indices)
{
    uint32 *vI = (uint32 *)&indices.v;
    lane_real32 result = {0};
    result.v = _mm_setr_ps(*(real32 *)((uint8 *)basePtr + vI[0] * stride),
                           *(real32 *)((uint8 *)basePtr + vI[1] * stride),
                           *(real32 *)((uint8 *)basePtr + vI[2] * stride),
                           *(real32 *)((uint8 *)basePtr + vI[3] * stride));
    return(result);
}

static lane_vec3
lane_Gather_V3_(void *basePtr, uint32 stride, lane_uint32 indices)
{
    lane_vec3 result = {0};
    result.x = lane_Gather_F32_((real32 *)basePtr + 0, stride, indices);
    result.y = lane_Gather_F32_((real32 *)basePtr + 1, stride, indices);
    result.z = lane_Gather_F32_((real32 *)basePtr + 2, stride, indices);
    return(result);
}


static uint32
lane_MaskIsZero(lane_uint32 mask)
{
    uint32 result = _mm_movemask_epi8(mask.v);
    return(result == 0);
}

//Note(Agu): these will not be as precise, use carefully.
static real32
lane_HorizontalAdd_F32(lane_real32 l)
{
  real32 *values = (real32 *)&l.v;
  real32 result = (values[0] + values[1] + values[2] + values[3]);
  return(result);

}

static uint32
lane_HorizontalAdd_U32(lane_uint32 l)
{
  uint32 *values = (uint32 *)&l.v;
  uint32 result = (values[0] + values[1] + values[2] + values[3]);
  return(result);

}

static vec3
lane_HorizontalAdd_V3(lane_vec3 l)
{
  vec3 result = {lane_HorizontalAdd_F32(l.x),
                  lane_HorizontalAdd_F32(l.y),  
                  lane_HorizontalAdd_F32(l.z)};
  return(result);

}

#elif LANEWIDTH == 1


typedef uint64 lane_uint64;
typedef uint32 lane_uint32;
typedef uint16 lane_uint16; 
typedef uint8  lane_uint8;
typedef int64  lane_int64;
typedef int16  lane_int16;
typedef int32  lane_int32;
typedef int8   lane_int8;
typedef real32 lane_real32;
typedef real64 lane_real64;

typedef vec2   lane_vec2;
typedef vec3   lane_vec3;
typedef vec4   lane_vec4;

#define FTOU32(v) *(uint32 *)v

static lane_uint32 lane_U32(uint32 v1, uint32 v2, uint32 v3, uint32 v4)
{
    uint32 m = 0xffff;
    lane_uint32 result = (v1         & m) |
                         ((v2 >> 8)  & m) | 
                         ((v3 >> 16) & m) | 
                         ((v4 >> 24) & m);
    return(result);
}

static lane_uint32
lane_U32_FROM_1U32(uint32 v)
{
    return(v);
}

static lane_real32
lane_F32_FROM_1U32(uint32 v)
{
    return((real32)v);
}

static lane_real32
lane_F32_FROM_1F32(real32 v)
{
    return(v);
}

//Assignments
static lane_vec3
lane_V3(lane_real32 x, lane_real32 y, lane_real32 z)
{
    lane_vec3 result = {x, y, z};
    return(result);
}

static lane_vec3
lane_V3_FROM_1V3(vec3 v)
{
    return (v);
}

static lane_vec3
lane_V3_FROM_1F32(real32 v)
{
    lane_vec3 result = {v, v, v};
    return(result);
}

//Comparisons

static lane_uint32
lane_U32_LT(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 < l2);
}

static lane_uint32
lane_U32_GT(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 > l2);
}

static lane_uint32
lane_U32_EQ(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 == l2);
}

static lane_uint32
lane_U32_NEQ(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 != l2);
}

static lane_uint32
lane_U32_1U32_LT(lane_uint32 l, uint32 v)
{
   return(l < v);
}

static lane_uint32
lane_U32_1U32_GT(lane_uint32 l, uint32 v)
{
   return(l > v);
}

static lane_uint32
lane_U32_1U32_NEQ(lane_uint32 l, uint32 v)
{
   return(l != v);
}


static lane_uint32
lane_F32_EQ(lane_real32 l1, lane_real32 l2)
{
   return(l1 == l2);
}

static lane_uint32
lane_F32_NEQ(lane_real32 l1, lane_real32 l2)
{
   return(l1 != l2);
}

static lane_uint32
lane_F32_LT(lane_real32 l1, lane_real32 l2)
{
   return(l1 < l2);
}

static lane_uint32
lane_F32_GT(lane_real32 l1, lane_real32 l2)
{
   return(l1 > l2);
}

static lane_uint32
lane_F32_1F32_LT(lane_real32 l, real32 v)
{
   return(l < v);
}

static lane_uint32
lane_F32_1F32_GT(lane_real32 l, real32 v)
{
   return(l > v);
}

//End Comparisons

static lane_uint32
lane_U32_Min(lane_uint32 l1, lane_uint32 l2)
{
    return((l1 < l2) ? l1 : l2);
}

static lane_uint32
lane_U32_Max(lane_uint32 l1, lane_uint32 l2)
{
    return((l1 > l2) ? l1 : l2);
}

static lane_uint32
lane_U32_Or(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 | l2);
}

static lane_uint32
lane_U32_And(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 & l2);
}

static lane_uint32
lane_U32_Xor(lane_uint32 l1, lane_uint32 l2)
{
   return(l1 ^ l2);
}

static lane_uint32
lane_U32_ShiftL(lane_uint32 l, uint32 amount)
{
   return(l << amount);
}

static lane_uint32
lane_U32_ShiftR(lane_uint32 l, uint32 amount)
{
   return(l >> amount);
}

static lane_uint32
lane_U32_Add(lane_uint32 l1, lane_uint32 l2)
{
    return(l1 + l2);
}

static lane_uint32
lane_U32_Sub(lane_uint32 l1, lane_uint32 l2)
{
    return(l1 - l2);
}

static lane_uint32
lane_U32_Mul(lane_uint32 l1, lane_uint32 l2)
{
    return(l1 * l2);
}

static lane_uint32
lane_U32_Div(lane_uint32 l1, lane_uint32 l2)
{
    return(l1 / l2);
}

static lane_uint32
lane_U32_1U32_Add(lane_uint32 l1, uint32 v)
{
    return(l1 + v);
}

static lane_uint32
lane_U32_1U32_Sub(lane_uint32 l1, uint32 v)
{
    return(l1 - v);
}

static lane_uint32
lane_U32_1U32_Mul(lane_uint32 l1, uint32 v)
{
    return(l1 * v);
}

static lane_uint32
lane_U32_1U32_Div(lane_uint32 l1, uint32 v)
{
    return(l1 / v);
}

//Floating point
static lane_real32
lane_F32_Min(lane_real32 l1, lane_real32 l2)
{
    return((l1 < l2) ? l1 : l2);
}

static lane_real32
lane_F32_Max(lane_real32 l1, lane_real32 l2)
{
    return((l1 > l2) ? l1 : l2);
}

static lane_real32
lane_F32_Clamp(lane_real32 l)
{
    l = lane_F32_Min(lane_F32_Max(l, lane_F32_FROM_1F32(0)), lane_F32_FROM_1F32(1));
}

static lane_real32
lane_F32_Or(lane_real32 l1, lane_real32 l2)
{
   return((real32)((*(uint32 *)&l1) | (*(uint32 *)&l2)));
}

static lane_real32
lane_F32_And(lane_real32 l1, lane_real32 l2)
{
   return((real32)((*(uint32 *)&l1) & (*(uint32 *)&l2)));
}

static lane_real32
lane_F32_U32_And(lane_real32 l1, lane_uint32 l2)
{
   lane_uint32 l3 = ((*(uint32 *)&l1) & l2);
   return(*(real32 *)&l3);
}

static lane_real32
lane_F32_Xor(lane_real32 l1, lane_real32 l2)
{
   return((real32)((*(uint32 *)&l1) ^ (*(uint32 *)&l2)));
}

static lane_real32
lane_F32_Neg(lane_real32 l)
{
    return(-l);
}
static lane_real32
lane_F32_Add(lane_real32 l1, lane_real32 l2)
{
    return(l1 + l2);
}

static lane_real32
lane_F32_Sub(lane_real32 l1, lane_real32 l2)
{
    return(l1 - l2);
}

static lane_real32
lane_F32_Mul(lane_real32 l1, lane_real32 l2)
{
    return(l1 * l2);
}

static lane_real32
lane_F32_Div(lane_real32 l1, lane_real32 l2)
{
    return(l1 / l2);
}

static lane_real32
lane_F32_Sqrt(lane_real32 l)
{
    return(SquareRoot(l));
}

static lane_real32
lane_F32_1F32_Add(lane_real32 l1, real32 v)
{
    return(l1 + v);
}

static lane_real32
lane_F32_1F32_Sub(lane_real32 l1, real32 v)
{
    return(l1 - v);
}

static lane_real32
lane_F32_1F32_SubR(real32 v, lane_real32 l1)
{
    return(v - l1);
}

static lane_real32
lane_F32_1F32_Mul(lane_real32 l1, real32 v)
{
    return(l1 * v);
}

static lane_real32
lane_F32_1F32_Div(lane_real32 l, real32 divisor)
{
    return(l / divisor);
}

static lane_real32
lane_F32_1F32_DivR(real32 divisor, lane_real32 l)
{
    return(divisor / l);
}

static lane_real32
lane_F32_V3_Add(lane_real32 l1, lane_vec3 l2);

static lane_real32
lane_F32_V3_Mul(lane_real32 l1, lane_vec3 l2);

static lane_real32
lane_F32_V3_Div(lane_real32 l1, lane_vec3 l2);

static lane_real32
lane_F32_Lerp(lane_real32 left, lane_real32 delta, lane_real32 right)
{
    return(Lerp(left, delta, right));
}


//Floating point
static lane_vec3
lane_V3_Add(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Add(l1.x, l2.x);
    l1.y = lane_F32_Add(l1.y, l2.y);
    l1.z = lane_F32_Add(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Sub(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Sub(l1.x, l2.x);
    l1.y = lane_F32_Sub(l1.y, l2.y);
    l1.z = lane_F32_Sub(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Mul(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Mul(l1.x, l2.x);
    l1.y = lane_F32_Mul(l1.y, l2.y);
    l1.z = lane_F32_Mul(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_Div(lane_vec3 l1, lane_vec3 l2)
{
    l1.x = lane_F32_Div(l1.x, l2.x);
    l1.y = lane_F32_Div(l1.y, l2.y);
    l1.z = lane_F32_Div(l1.z, l2.z);
    return(l1);
}

static lane_vec3
lane_V3_F32_Add(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Add(l.x, v);  
    l.y = lane_F32_Add(l.y, v);  
    l.z = lane_F32_Add(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_F32_Sub(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Sub(l.x, v);  
    l.y = lane_F32_Sub(l.y, v);  
    l.z = lane_F32_Sub(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_F32_SubR(lane_real32 l1, lane_vec3 l2)
{
    l2.x = lane_F32_Sub(l1, l2.x);  
    l2.y = lane_F32_Sub(l1, l2.y);  
    l2.z = lane_F32_Sub(l1, l2.z);  
    return(l2);
}

static lane_vec3
lane_V3_F32_Mul(lane_vec3 l, lane_real32 v)
{
    l.x = lane_F32_Mul(l.x, v);  
    l.y = lane_F32_Mul(l.y, v);  
    l.z = lane_F32_Mul(l.z, v);  
    return(l);
}

static lane_vec3
lane_V3_U32_And(lane_vec3 l1, lane_uint32 l2)
{
    //Note(Agu): this should be interpreted as MASK rather than the And operator.
   l2 = l2 ? 0xffffffff : 0;
#if 0
   vec3 result = {0};
   result.x = *(uint32 *)l1.x
#endif

   l1.x = lane_F32_U32_And(l1.x, l2);
   l1.y = lane_F32_U32_And(l1.y, l2);
   l1.z = lane_F32_U32_And(l1.z, l2);
   return(l1);
}

static lane_vec3
lane_V3_1F32_Mul(lane_vec3 l, real32 v)
{
    l = lane_V3_F32_Mul(l, v);  
    return(l);
}

static lane_real32
lane_V3_Inner(lane_vec3 l1, lane_vec3 l2)
{
    return(vec3_Inner(l1, l2));
}

static lane_vec3
lane_V3_NormalizeZero(lane_vec3 l1)
{
  return(vec3_NormalizeZero(l1));
}

static lane_vec3
lane_V3_Lerp(lane_vec3 left, lane_real32 delta, lane_vec3 right)
{
    lane_vec3 result;
    result.x = lane_F32_Lerp(left.x, delta, right.x);
    result.y = lane_F32_Lerp(left.y, delta, right.y);
    result.z = lane_F32_Lerp(left.z, delta, right.z);
    return(result);
}

//Convertions
static lane_real32
lane_U32_CAST_F32(lane_uint32 l)
{
    return((real32)l);
}

static lane_uint32
lane_F32_CAST_U32(lane_real32 l)
{
    return((uint32)l);
}
//Convertions
static lane_real32
lane_U32_CVT_F32(lane_uint32 l)
{
    //return(*(real32 *)&l);
    return(*(real32 *)&l);
}

static lane_uint32
lane_F32_CVT_U32(lane_real32 l)
{
    return(*(uint32 *)&l);
}


//Comparison set

static lane_uint32
lane_U32_AndNot(lane_uint32 mask, lane_uint32 l)
{
    return(~mask & l);
}

static lane_real32
lane_F32_AndNot(lane_real32 mask, lane_real32 l)
{
    return((real32)(~*(uint32 *)&mask & *(uint32 *)&l));
}

static void 
lane_U32_ConditionalSet(lane_uint32 mask, lane_uint32 *s, lane_uint32 v)
{
    mask = (mask ? 0xffffffff : 0);
    *s = ((~mask & *s) | (mask & v));
    //*s = lane_U32_Or(lane_U32_AndNot(mask, *s), lane_U32_And(mask, v));
}


static void 
lane_F32_ConditionalSet(lane_real32 mask, lane_real32 *s, lane_real32 v)
{
    lane_U32_ConditionalSet(*(uint32 *)&mask, (uint32 *)s, *(uint32 *)&v);
}

static void 
lane_F32_ConditionalSet_U32M(lane_uint32 mask, lane_real32 *s, lane_real32 v)
{
    lane_real32 maskPs = *(real32 *)&mask; 
    lane_F32_ConditionalSet(maskPs, s, v);
}
     
static void 
lane_V3_ConditionalSet(lane_real32 mask, lane_vec3 *s, lane_vec3 v)
{
   lane_F32_ConditionalSet(mask, &s->x, v.x);
   lane_F32_ConditionalSet(mask, &s->y, v.y);
   lane_F32_ConditionalSet(mask, &s->z, v.z);
}

static void 
lane_V3_ConditionalSet_U32M(lane_uint32 mask, lane_vec3 *s, lane_vec3 v)
{
   lane_real32 maskPs = *(real32 *)&mask; 
   lane_V3_ConditionalSet(maskPs, s, v);
}

//Gather

#define lane_Gather_U32(basePtr, index, member) lane_Gather_U32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_F32(basePtr, index, member) lane_Gather_F32_(&(basePtr)->member, sizeof(*(basePtr)), index)
#define lane_Gather_V3(basePtr, index, member) lane_Gather_V3_(&(basePtr)->member, sizeof(*(basePtr)), index)


static lane_uint32
lane_Gather_U32_(void *basePtr, uint32 stride, lane_uint32 index)
{
    lane_uint32 result = (*(uint32 *)((uint8 *)basePtr + stride * index));
    return(result);
}

static lane_real32
lane_Gather_F32_(void *basePtr, uint32 stride, lane_uint32 index)
{
    lane_real32 result = (*(real32 *)((uint8 *)basePtr + stride * index));
    return(result);
}

static lane_vec3
lane_Gather_V3_(void *basePtr, uint32 stride, lane_uint32 indices)
{
    lane_vec3 result = {0};
    result.x = lane_Gather_F32_((real32 *)basePtr + 0, stride, indices);
    result.y = lane_Gather_F32_((real32 *)basePtr + 1, stride, indices);
    result.z = lane_Gather_F32_((real32 *)basePtr + 2, stride, indices);
    return(result);
}

static int32
lane_MaskIsZero(lane_uint32 mask)
{
    return(mask == 0);
}

//Note(Agu): these will not be as precise, use carefully.
static real32
lane_HorizontalAdd_F32(lane_real32 l)
{
  return(l);
}

static uint32
lane_HorizontalAdd_U32(lane_uint32 l)
{
  return(l);
}

static vec3
lane_HorizontalAdd_V3(lane_vec3 l)
{
  return(l);
}

#else
#error Lane width must be: 1, 4... 
#endif
