//#define _CMP_EQ_OQ     0x00  /* Equal (ordered, nonsignaling)               */
//#define _CMP_LT_OS     0x01  /* Less-than (ordered, signaling)              */
//#define _CMP_LE_OS     0x02  /* Less-than-or-equal (ordered, signaling)     */
//#define _CMP_UNORD_Q   0x03  /* Unordered (nonsignaling)                    */
//#define _CMP_NEQ_UQ    0x04  /* Not-equal (unordered, nonsignaling)         */
//#define _CMP_NLT_US    0x05  /* Not-less-than (unordered, signaling)        */
//#define _CMP_NLE_US    0x06  /* Not-less-than-or-equal (unordered,
//                                                        signaling)          */
//#define _CMP_ORD_Q     0x07  /* Ordered (nonsignaling)                      */
//#define _CMP_EQ_UQ     0x08  /* Equal (unordered, non-signaling)            */
//#define _CMP_NGE_US    0x09  /* Not-greater-than-or-equal (unordered,
//                                                           signaling)       */
//#define _CMP_NGT_US    0x0A  /* Not-greater-than (unordered, signaling)     */
//#define _CMP_FALSE_OQ  0x0B  /* False (ordered, nonsignaling)               */
//#define _CMP_NEQ_OQ    0x0C  /* Not-equal (ordered, non-signaling)          */
//#define _CMP_GE_OS     0x0D  /* Greater-than-or-equal (ordered, signaling)  */
//#define _CMP_GT_OS     0x0E  /* Greater-than (ordered, signaling)           */
//#define _CMP_TRUE_UQ   0x0F  /* True (unordered, non-signaling)             */
//#define _CMP_EQ_OS     0x10  /* Equal (ordered, signaling)                  */
//#define _CMP_LT_OQ     0x11  /* Less-than (ordered, nonsignaling)           */
//#define _CMP_LE_OQ     0x12  /* Less-than-or-equal (ordered, nonsignaling)  */
//#define _CMP_UNORD_S   0x13  /* Unordered (signaling)                       */
//#define _CMP_NEQ_US    0x14  /* Not-equal (unordered, signaling)            */
//#define _CMP_NLT_UQ    0x15  /* Not-less-than (unordered, nonsignaling)     */
//#define _CMP_NLE_UQ    0x16  /* Not-less-than-or-equal (unordered,
//                                                        nonsignaling)       */
//#define _CMP_ORD_S     0x17  /* Ordered (signaling)                         */
//#define _CMP_EQ_US     0x18  /* Equal (unordered, signaling)                */
//#define _CMP_NGE_UQ    0x19  /* Not-greater-than-or-equal (unordered,
//                                                           nonsignaling)    */
//#define _CMP_NGT_UQ    0x1A  /* Not-greater-than (unordered, nonsignaling)  */
//#define _CMP_FALSE_OS  0x1B  /* False (ordered, signaling)                  */
//#define _CMP_NEQ_OS    0x1C  /* Not-equal (ordered, signaling)              */
//#define _CMP_GE_OQ     0x1D  /* Greater-than-or-equal (ordered,
//                                                       nonsignaling)        */
//#define _CMP_GT_OQ     0x1E  /* Greater-than (ordered, nonsignaling)        */
//#define _CMP_TRUE_US   0x1F  /* True (unordered, signaling)                 */

#include <intrin.h>
typedef struct
{
    __m256i v;

}lane_uint32;

typedef struct
{

    __m256 v;
}lane_real32;

typedef struct
{
    lane_real32 x;
    lane_real32 y;
    lane_real32 z;

}lane_vec3;

static lane_uint32
lane_U32(uint32 v1, uint32 v2, uint32 v3, uint32 v4,uint32 v5, uint32 v6, uint32 v7, uint32 v8 )
{
    lane_uint32 result = {0};
    result.v = _mm256_set_epi32(v1, v2, v3, v4, v5, v6, v7, v8);
    return(result);
}

static lane_uint32
lane_U32_FROM_1U32(uint32 v)
{
    lane_uint32 result = {0};
    result.v = _mm256_set1_epi32(v);
    return(result);
}

static lane_real32
lane_F32_FROM_1U32(uint32 v)
{
    lane_real32 result = {0};
    result.v = _mm256_set1_ps((real32)v);
    return(result);
}

static lane_real32
lane_F32_FROM_1F32(real32 v)
{
    lane_real32 result = {0};
    result.v = _mm256_set1_ps(v);
    return(result);
}

//Convertions
static lane_real32
lane_U32_CAST_F32(lane_uint32 l)
{
    lane_real32 result = {0};
    result.v = _mm256_castsi256_ps(l.v);
    return(result);
}

static lane_uint32
lane_F32_CAST_U32(lane_real32 l)
{
    lane_uint32 result = {0};
    result.v = _mm256_castps_si256(l.v);
    return(result);
}

static lane_real32
lane_U32_CVT_F32(lane_uint32 l)
{
    lane_real32 result = {0};
    result.v = _mm256_cvtepi32_ps(l.v);
    return(result);
}

static lane_uint32
lane_F32_CVT_U32(lane_real32 l)
{
    lane_uint32 result = {0};
    result.v = _mm256_cvtps_epi32(l.v);
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
lane_U32_GT(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm256_cmpgt_epi32(l1.v, l2.v);
   return(result);
}

static lane_uint32
lane_U32_LT(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   //Note(Agu):Unchecked.
   result.v = _mm256_xor_si256(_mm256_cmpgt_epi32(l1.v, l2.v),
                               _mm256_set1_epi32(0xffffffff));
   return(result);
}
static lane_uint32
lane_U32_EQ(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm256_cmpeq_epi32(l1.v, l2.v);
   return(result);
}

static lane_uint32
lane_U32_NEQ(lane_uint32 l1, lane_uint32 l2)
{
   lane_uint32 result = {0};
   //Note(Agu):Unchecked.
   result.v = _mm256_xor_si256(_mm256_cmpeq_epi32(l1.v, l2.v),
                               _mm256_set1_epi32(0xffffffff));
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
   result.v = _mm256_castps_si256(_mm256_cmp_ps(l1.v, l2.v, _CMP_EQ_OQ));
   return(result);
}

static lane_uint32
lane_F32_NEQ_U32M(lane_real32 l1, lane_real32 l2)
{
    //This one actually has a neq, not like U32
   lane_uint32 result = {0};
   result.v = _mm256_castps_si256(_mm256_cmp_ps(l1.v, l2.v, _CMP_NEQ_OQ));
   return(result);
}
//TODO(Agu): Do a non-cast version of this to keep a real32 mask
static lane_uint32
lane_F32_LT(lane_real32 l1, lane_real32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm256_castps_si256(_mm256_cmp_ps(l1.v, l2.v, _CMP_LT_OQ));
   return(result);
}

static lane_uint32
lane_F32_GT(lane_real32 l1, lane_real32 l2)
{
   lane_uint32 result = {0};
   result.v = _mm256_castps_si256(_mm256_cmp_ps(l1.v, l2.v, _CMP_GT_OQ));
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
    l1.v = _mm256_min_epi32(l1.v, l2.v);
    return(l1);
}

static lane_uint32
lane_U32_Max(lane_uint32 l1, lane_uint32 l2)
{
    l1.v = _mm256_max_epi32(l1.v, l2.v);
    return(l1);
}

static lane_uint32
lane_U32_Or(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm256_or_si256(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_And(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm256_and_si256(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_Xor(lane_uint32 l1, lane_uint32 l2)
{
   l1.v = _mm256_xor_si256(l1.v, l2.v);

   return(l1);
}

static lane_uint32
lane_U32_ShiftL(lane_uint32 l, uint32 amount)
{
   lane_uint32 result = {0};
   result.v = _mm256_slli_epi32(l.v, amount);
   return(result);
}

static lane_uint32
lane_U32_ShiftR(lane_uint32 l, uint32 amount)
{
   lane_uint32 result = {0};
   result.v = _mm256_srli_epi32(l.v, amount);
   return(result);
}

static lane_uint32
lane_U32_Add(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm256_add_epi32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Sub(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm256_sub_epi32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Mul(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm256_mul_epu32(l1.v, l2.v);
    return(result);
}

static lane_uint32
lane_U32_Div(lane_uint32 l1, lane_uint32 l2)
{
    lane_uint32 result = {0};
    result.v = _mm256_div_epi32(l1.v, l2.v);
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
    l1.v = _mm256_min_ps(l1.v, l2.v);
    return(l1);
}

static lane_real32
lane_F32_Max(lane_real32 l1, lane_real32 l2)
{
    l1.v = _mm256_max_ps(l1.v, l2.v);
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
   l1.v = _mm256_or_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_And(lane_real32 l1, lane_real32 l2)
{
   l1.v = _mm256_and_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_Xor(lane_real32 l1, lane_real32 l2)
{
   l1.v = _mm256_xor_ps(l1.v, l2.v);
   return(l1);
}

static lane_real32
lane_F32_Neg(lane_real32 l)
{
    l.v = _mm256_sub_ps(_mm256_set1_ps(0), l.v);
    return(l);
}
static lane_real32
lane_F32_Add(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm256_add_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Sub(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm256_sub_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Mul(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm256_mul_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Div(lane_real32 l1, lane_real32 l2)
{
    lane_real32 result = {0};
    result.v = _mm256_div_ps(l1.v, l2.v);
    return(result);
}

static lane_real32
lane_F32_Sqrt(lane_real32 l)
{
    lane_real32 result = {0};
    result.v = _mm256_sqrt_ps(l.v);
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
    lane_real32 l2 = {_mm256_set1_ps(divisor)};
    lane_real32 result = lane_F32_Div(l1, l2);  
    return(result);
}

static lane_real32
lane_F32_1F32_DivR(real32 divisor, lane_real32 l1)
{
    lane_real32 l2 = {_mm256_set1_ps(divisor)};
    lane_real32 result = lane_F32_Div(l2, l1);
    return(result);
}


static lane_real32
lane_F32_U32_And(lane_real32 l1, lane_uint32 l2)
{
   //return(lane_U32_CAST_F32(lane_U32_And(lane_F32_CAST_U32(l1), l2)));
    lane_real32 result = {0};
    result.v = _mm256_and_ps(l1.v, _mm256_castsi256_ps(l2.v));
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
    result.v = _mm256_andnot_si256(mask.v, l.v);
    return(result);
}

static lane_real32
lane_F32_AndNot(lane_real32 mask, lane_real32 l)
{
    lane_real32 result = {0};
    result.v = _mm256_andnot_ps(mask.v, l.v);
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
    result.v = _mm256_setr_epi32(*(uint32 *)((uint8 *)basePtr + vI[0] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[1] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[2] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[3] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[4] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[5] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[6] * stride),
                                 *(uint32 *)((uint8 *)basePtr + vI[7] * stride));
    return(result);
}

static lane_real32
lane_Gather_F32_(void *basePtr, uint32 stride, lane_uint32 indices)
{
    uint32 *vI = (uint32 *)&indices.v;
    lane_real32 result = {0};
    result.v = _mm256_setr_ps(*(real32 *)((uint8 *)basePtr + vI[0] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[1] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[2] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[3] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[4] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[5] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[6] * stride),
                              *(real32 *)((uint8 *)basePtr + vI[7] * stride));
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
    uint32 result = _mm256_movemask_epi8(mask.v);
    return(result == 0);
}

//Note(Agu): these will not be as precise, use carefully.
static real32
lane_HorizontalAdd_F32(lane_real32 l)
{
  real32 *values = (real32 *)&l.v;
  real32 result = (values[0] + values[1] + values[2] + values[3] +
                   values[4] + values[5] + values[6] + values[7]);
  return(result);

}

static uint32
lane_HorizontalAdd_U32(lane_uint32 l)
{
  uint32 *values = (uint32 *)&l.v;
  uint32 result = (values[0] + values[1] + values[2] + values[3] +
                   values[4] + values[5] + values[6] + values[7]);
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
