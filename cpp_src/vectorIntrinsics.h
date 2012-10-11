#ifndef __VECTINTRINSICS__
#define __VECTINTRINSICS__

inline int min(int x, int y)
{
  return (x<y) ? x : y;
}


#ifdef __SSE2__
#include<emmintrin.h>
#include<xmmintrin.h>



typedef float float4 __attribute__ ((vector_size (16)));
typedef double double2 __attribute__ ((vector_size (16)));

typedef float float64;

inline float4 vecSel4(float4 m, float4 t, float4 f)
{
  float4 t0 = _mm_and_ps(t, m);
  float4 t1 = _mm_andnot_ps(m, f);
  return _mm_or_ps(t0,t1);
}

inline float4 vecEq4(float4 a, float4 b)
{
  return _mm_cmpeq_ps(a,b);
}

inline float4 vecNeq4(float4 a, float4 b)
{
  return _mm_cmpneq_ps(a,b);
}

inline float4 vecLT4(float4 a, float4 b)
{
  return _mm_cmplt_ps(a,b);
}

inline float4 vecGT4(float4 a, float4 b)
{
  return _mm_cmpgt_ps(a,b);
}

inline float4 vecLTE4(float4 a, float4 b)
{
  return _mm_cmple_ps(a,b);
}

inline float4 vecGTE4(float4 a, float4 b)
{
  return _mm_cmpge_ps(a,b);
}

inline float4 vecLoad4(float *addr) 
{
  return _mm_load_ps(addr); 
}

inline double2 vecLoad2(double *addr) 
{
  return _mm_load_pd(addr); 
}

inline void vecStore4(float *addr, float4 x)
{
  _mm_store_ps( (float*) addr, x);
}

inline void vecStore2(double *addr, double2 x)
{
  _mm_store_pd( (double*) addr, x);
}

inline float4 vecLoadSplatter4(float *addr)
{
  float x = *addr;
  return _mm_set_ps(x,x,x,x);
}

inline double2 vecLoadSplatter2(double *addr)
{
  double x = *addr;
  return _mm_set_pd(x,x);
}

inline float4 vecSplatter4(float x)
{
  return _mm_set_ps(x,x,x,x);
}

inline double2 vecSplatter2(double x)
{
  return _mm_set_pd(x,x);
}


inline float4 vecAdd4(float4 a, float4 b)
{
  return _mm_add_ps(a,b);
}

inline double2 vecAdd2(double2 a, double2 b)
{
  return _mm_add_pd(a,b);
}

inline float4 vecSub4(float4 a, float4 b)
{
  return _mm_sub_ps(a,b);
}

inline double2 vecSub2(double2 a, double2 b)
{
  return _mm_sub_pd(a,b);
}

inline float4 vecMul4(float4 a, float4 b)
{
  return _mm_mul_ps(a,b);
}

inline double2 vecMul2(double2 a, double2 b)
{
  return _mm_mul_pd(a,b);
}

inline float4 vecDiv4(float4 a, float4 b)
{
  return _mm_div_ps(a,b);
}

inline double2 vecDiv2(double2 a, double2 b)
{
  return _mm_div_pd(a,b);
}


#elif defined __ARM_NEON__
#include <arm_neon.h>
typedef float32x4_t float4;
typedef struct 
{
  float4 lo;
  float4 hi;
} float8;

inline float4 vecLoad4(float *addr) 
{
  return vld1q_f32 ((const float32_t *) addr);
}
inline float8 vecLoad8(float *addr) 
{
  float8 t;
  t.lo = vld1q_f32 ((const float32_t *) addr);
  t.hi = vld1q_f32 ((const float32_t *) addr + 4); 
  return t;
}

inline void vecStore4(float *addr, float4 x)
{
  vst1q_f32 ((float32_t *) addr, x);
}

inline void vecStore8(float *addr, float8 x)
{
  vst1q_f32 ((float32_t *) addr, x.lo);
  vst1q_f32 ((float32_t *) addr+4, x.hi);
}

inline float4 vecLoadSplatter4(float *addr)
{
  float x = *addr;
  return vdupq_n_f32((float32_t)x);
}
inline float8 vecLoadSplatter8(float *addr)
{
  float8 t;
  float x = *addr;
  t.lo = vdupq_n_f32((float32_t)x);
  t.hi = t.lo;
  return t;
}
inline float4 vecSplatter4(float x)
{
  return vdupq_n_f32((float32_t)x);
}
inline float8 vecSplatter8(float x)
{
  float8 t;
  t.lo = vdupq_n_f32((float32_t)x);
  t.hi = t.lo;
  return t;
}
inline float4 vecAdd4(float4 a, float4 b)
{
  return vaddq_f32(a,b);
}

inline float4 vecSub4(float4 a, float4 b)
{
  return vsubq_f32(a,b);
}

inline float4 vecMul4(float4 a, float4 b)
{
  return vmulq_f32(a,b);
} 


inline float8 vecAdd8(float8 a, float8 b)
{
  float8 t;
  t.lo = vaddq_f32(a.lo,b.lo);
  t.hi = vaddq_f32(a.hi,b.hi);
  return t;
}

inline float8 vecSub8(float8 a, float8 b)
{
  float8 t;
  t.lo = vsubq_f32(a.lo,b.lo);
  t.hi = vsubq_f32(a.hi,b.hi);  
  return t;
}

inline float8 vecMul8(float8 a, float8 b)
{
  float8 t;
  t.lo = vmulq_f32(a.lo,b.lo);
  t.hi = vmulq_f32(a.hi,b.hi);
  return t;
} 


#elif defined __ALTIVEC__
#include <altivec.h>
typedef float float4 __attribute__ ((vector_size (16)));

inline float4 vecLoad4(float *addr) 
{
  return vec_ldl(0, (float*)addr);
}

inline void vecStore4(float *addr, float4 x)
{
  vec_st(x, 0, addr);
}

inline float4 vecLoadSplatter4(float *addr)
{
  float x = *addr;
  float4 t = {x,x,x,x};
  return t; 
}

inline float4 vecSplatter4(float x)
{
  float4 t = {x,x,x,x};
  return t; 
}

inline float4 vecAdd4(float4 a, float4 b)
{
  return vec_add(a,b);
}

inline float4 vecSub4(float4 a, float4 b)
{
  return vec_sub(a,b);
}

inline float4 vecMul4(float4 a, float4 b)
{
  float4 z = {0.0f, 0.0f, 0.0f, 0.0f};
  return vec_madd(a,b,z);
} 

/*
inline float4 vecDiv4(float4 a, float4 b)
{
  return vmulq_f32(a,b);
}
*/

#else
#define NO_VEC 1
typedef struct
{
  float x;
  float y;
  float z;
  float w;
} float4;


inline float4 vecLoadSplatter4(float *addr)
{
  float4 t;
  float v = *addr;
  t.x = t.y = t.z = t.w = v;
  return t;
}

inline float4 vecLoadSplatter4(float v)
{
  float4 t;
  t.x = t.y = t.z = t.w = v;
  return t;
}



inline float4 vecLoad4(float *addr)
{
  float4 t;
  t.x = addr[0];
  t.y = addr[1];  
  t.z = addr[2];
  t.w = addr[3];
  return t;
}

inline void vecStore4(float *addr, float4 v)
{
  addr[0] = v.x;
  addr[1] = v.y;
  addr[2] = v.z;
  addr[3] = v.w;
}


inline float4 vecAdd4(float4 a, float4 b)
{
  float4 t;
  t.x = a.x + b.x;
  t.y = a.y + b.y;
  t.z = a.z + b.z;
  t.w = a.w + b.w;
  return t;
}

inline float4 vecSub4(float4 a, float4 b)
{
  float4 t;
  t.x = a.x - b.x;
  t.y = a.y - b.y;
  t.z = a.z - b.z;
  t.w = a.w - b.w;
  return t;
}

inline float4 vecDiv4(float4 a, float4 b)
{
  float4 t;
  t.x = a.x / b.x;
  t.y = a.y / b.y;
  t.z = a.z / b.z;
  t.w = a.w / b.w;
  return t;
}

inline float4 vecMul4(float4 a, float4 b)
{
  float4 t;
  t.x = a.x * b.x;
  t.y = a.y * b.y;
  t.z = a.z * b.z;
  t.w = a.w * b.w;
  return t;
}
#endif


#ifdef __AVX__
#include <immintrin.h>
typedef float float8 __attribute__ ((vector_size (32)));
inline float8 vecLoad8(float *addr) 
{
  return _mm256_loadu_ps(addr); 
}

inline void vecStore8(float *addr, float8 x)
{
  _mm256_storeu_ps( (float*) addr, x);
}

inline float8 vecLoadSplatter8(float *addr)
{
  float x = *addr;
  return _mm256_set_ps(x,x,x,x,
		       x,x,x,x);
}

inline float8 vecAdd8(float8 a, float8 b)
{
  return _mm256_add_ps(a,b);
}

inline float8 vecSub8(float8 a, float8 b)
{
  return _mm256_sub_ps(a,b);
}

inline float8 vecMul8(float8 a, float8 b)
{
  return _mm256_mul_ps(a,b);
}

inline float8 vecDiv8(float8 a, float8 b)
{
  return _mm256_div_ps(a,b);
}
#endif


typedef struct
{
  float vec[32];
} float32;


inline float32 vecLoadSplatter32(float *addr)
{
  float32 t;
  float v = *addr;
  for(int i=0;i<32;i++) 
    t.vec[i] = v;
  return t;
}

inline float32 vecLoad32(float *addr)
{
  float32 t;
  for(int i=0;i<32;i++)
    t.vec[i] = addr[i];
  return t;
}

inline void vecStore32(float *addr, float32 v)
{
  for(int i=0;i<32;i++)
    addr[i] = v.vec[i];
}


inline float32 vecAdd32(float32 a, float32 b)
{
  float32 t;
  for(int i=0;i<32;i++)
    t.vec[i] = a.vec[i] + b.vec[i];
  return t;
}


inline float32 vecSub32(float32 a, float32 b)
{
  float32 t;
  for(int i=0;i<32;i++)
    t.vec[i] = a.vec[i] - b.vec[i];
  return t;
}


inline float32 vecMul32(float32 a, float32 b)
{
  float32 t;
  for(int i=0;i<32;i++)
    t.vec[i] = a.vec[i] * b.vec[i];
  return t;
}


inline float32 vecDiv32(float32 a, float32 b)
{
  float32 t;
  for(int i=0;i<32;i++)
    t.vec[i] = a.vec[i] / b.vec[i];
  return t;
}


#endif
