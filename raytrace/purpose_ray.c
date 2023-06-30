
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <time.h>

#include "../purpose_definitions.h"
#include "../purpose_all.h"
#include "../purpose_memory.c"
#include "../purpose_stream.h"
#include "../purpose_math.h"
//#include "purpose_random.h"
#include "agu_image.h"


#define LANEWIDTH 8

#if LANEWIDTH == 4 || LANEWIDTH == 1
#include "purpose_ray_lane.h"
#elif LANEWIDTH == 8
#include "purpose_ray_lane_x8.h"
#endif



typedef struct{
    lane_uint32 x;
    lane_uint32 seed;
}random_series;

typedef struct{
    vec3 normal;
    real32 normalDistance; //offset from origin of plane
    uint32 materialIndex;
}plane;

typedef struct{
    vec3 position;
    real32 radius;
    uint32 materialIndex;
}sphere;

typedef struct{
    real32 scatter;
    vec3 emitColor;
    vec3 reflectionColor;
}material;

typedef struct{
    uint32 materialCount;
    material *materials;

    uint32 sphereCount;
    sphere *spheres;

    uint32 planeCount;
    plane *planes;
}world;

typedef struct{

    world *rayWorld;
    image_data *image;
    uint32 startX;
    uint32 startY;
    uint32 endX;
    uint32 endY;
    random_series random;

}work_order;

typedef struct{
    uint32 workOrderCount;
    work_order *workOrders;

    uint32 raysPerPixel;
    uint32 maxBounceCount;

    volatile uint32 nextWorkOrderIndex;
    volatile uint64 bouncesComputed;
    volatile uint32 tilesComputed; 

}work_queue;



inline vec4
RGBAUnpack(uint32 pack)
{
    vec4 result = {
        (real32)((pack >> 0) & 0xff),
        (real32)((pack >> 8) & 0xff),
        (real32)((pack >> 16) & 0xff),
        (real32)((pack >> 24) & 0xff)
    };
    return(result);
}

inline uint32
RGBAPack(vec4 color)
{
    uint32 result = (((uint32)color.x << 0) |
                     ((uint32)color.y << 8) |
                     ((uint32)color.z << 16) |
                     ((uint32)color.w << 24));
    return(result);
}
inline uint32
BGRAPack(vec4 color)
{
    uint32 result = (((uint32)color.x << 16) |
                     ((uint32)color.y << 8) |
                     ((uint32)color.z << 0) |
                     ((uint32)color.w << 24));
    return(result);
}

inline vec4
RGBA255To1(vec4 v)
{
    real32 oneOverMaxColor = 1.0f / 255.0f;
    v.x *= oneOverMaxColor;
    v.y *= oneOverMaxColor;
    v.z *= oneOverMaxColor;
    v.w *= oneOverMaxColor;
    return(v);
}

inline vec4
RGBA1To255(vec4 v)
{
    real32 MaxColor = 255; 
    v.x *= MaxColor;
    v.y *= MaxColor;
    v.z *= MaxColor;
    v.w *= MaxColor;
    return(v);
}

//
// Random numbers
//

#define BITN1 0x68E31DA4
#define BITN2 0xB5298A4D
#define BITN3 0x1B56C4E9

#define F32MAX 3.402823E+38f //340,282,300,000,000,000,000,000,000,000,000,000,000

inline lane_uint32
Noise1D_U32(lane_uint32 x, lane_uint32 seed)
{
#if 0
     uint32 bit = x;
     bit *= BITN1;
     bit += seed;
     bit ^= bit >> 8;
     bit += BITN2;
     bit ^= bit << 8;
     bit *= BITN3;
     bit ^= bit >> 8
#endif

     lane_uint32 bit = x;
     bit = lane_U32_1U32_Mul(bit, BITN1);
     bit = lane_U32_Add(bit, seed);
     bit = lane_U32_Xor(bit, lane_U32_ShiftR(bit, 8));
     bit = lane_U32_1U32_Add(bit ,BITN2);
     bit = lane_U32_Xor(bit, lane_U32_ShiftL(bit, 8));
     bit = lane_U32_1U32_Mul(bit, BITN3);
     bit = lane_U32_Xor(bit, lane_U32_ShiftR(bit, 8));

     return bit;
}

inline lane_uint32 
random_GetU32(random_series *random)
{
    random->x = lane_U32_1U32_Add(random->x, 1);
    lane_uint32 result = Noise1D_U32(random->x, random->seed);
    return(lane_U32_ShiftR(result, 1));
}

inline lane_real32
random_GetF32(random_series *random)
{
#if LANEWIDTH == 4
    lane_real32 result = lane_U32_CVT_F32(random_GetU32(random));
#else
    lane_real32 result = lane_U32_CAST_F32(random_GetU32(random));
#endif
    return(result);
}

inline lane_real32
RandomUnilateral(random_series *r)
{
    //lane_real32 result = random_GetF32(r); 
    //lane_real32 result = (random_GetF32(r) / U32MAX); 
    lane_real32 result = lane_F32_Div(random_GetF32(r), lane_F32_FROM_1U32(U32MAX >> 1)); 

#if LANEWIDTH == 4
    real32 mA = 1.0f;
    Assert(*(real32 *)&result.v       >= -mA);
    Assert(*((real32 *)&result.v + 1) >= -mA);
    Assert(*((real32 *)&result.v + 2) >= -mA);
    Assert(*((real32 *)&result.v + 3) >= -mA);
#endif
    return(result);
}

inline lane_real32
RandomBilateral(random_series *r)
{
    lane_real32 result = lane_F32_1F32_SubR(1.0f, lane_F32_1F32_Mul(RandomUnilateral(r), 2.0f));
    return(result);
}


static real32
RayIntersects(vec3 rayOrigin, vec3 rayDir, vec3 planeNormal, real32 planeNormalDistance)
{
    real32 result = 0;
    return(result);
}


inline real32 
LinearToSRGB(real32 L)
{
  L = L < 0 ? 0 : L > 1.0f ? 1.0f : L;

  real32 S = L * 12.92f;
  if(L > 0.0031308f)
  {
      S = 1.055f * Pow(L, 1.0f / 2.4f) - 0.055f;
  }
  return(S);
}


#define GetImagePixel32(image, x , y) (uint32 *)GetImagePixel(image, x, y);
inline uint8 *
GetImagePixel(image_data *image, uint32 x, uint32 y)
{
    uint8 *result = image->pixels + y * (image->width * image->bpp) + (x * image->bpp);
    return(result);
}
inline uint32 *
_GetImagePixel(image_data *image, uint32 x, uint32 y)
{
    uint32 *result = (uint32 *)image->pixels + y * image->width + x;
    return(result);
}

#include <windows.h>

inline void
thread_Add(uint64 volatile *value, uint64 add)
{
    //*value += add;
    InterlockedExchangeAdd64(value, add);
}
inline void
thread_Add32(uint32 volatile *value, uint32 add)
{
    //*value += add;
    InterlockedExchangeAdd(value, add);
}

static int 
RenderTile(work_queue *workQueue)
{

    uint32 nextWorkOrderIndex = workQueue->nextWorkOrderIndex;
    thread_Add32(&workQueue->nextWorkOrderIndex, 1);
    if(nextWorkOrderIndex >= workQueue->workOrderCount)
    {
      return(0);
    }

    workQueue->raysPerPixel = 16;
    workQueue->maxBounceCount = 8;

    work_order *workOrder = workQueue->workOrders + nextWorkOrderIndex;
    world *rayWorld = workOrder->rayWorld;
    image_data *image = workOrder->image;

    random_series *random = &workOrder->random;

    uint32 imageW = image->width;
    uint32 imageH = image->height;
    uint32 imageSize = imageW * imageH * 4;

    vec3 vCamPosition = {0, -10, 1};
    vec3 vCamZ = vec3_Normalize(vCamPosition);
    vec3 vCamX = vec3_Normalize(vec3_Cross(V3(0, 0, 1), vCamZ));
    vec3 vCamY = vec3_Normalize(vec3_Cross(vCamZ, vCamX));

    lane_vec3 camRay =  {0};
    lane_vec3 camPosition = lane_V3_FROM_1V3(vCamPosition);
    lane_vec3 camZ = lane_V3_FROM_1V3(vCamZ);
    lane_vec3 camX = lane_V3_FROM_1V3(vCamX);
    lane_vec3 camY = lane_V3_FROM_1V3(vCamY);
    lane_real32 filmDistance = lane_F32_FROM_1F32(1.0f);
    lane_vec3 filmCenter = camPosition;
    filmCenter = lane_V3_Sub(filmCenter, lane_V3_F32_Mul(camZ, filmDistance));
 

    real32 fW = 1.0f;
    real32 fH = 1.0f;
    if(imageW > imageH)
    {
        fH = fW * ((real32)imageH / imageW);
    }
    else if(imageH > imageW)
    {
        fW = fH * ((real32)imageW / imageH);
    }

    lane_real32 filmX = {0};
    lane_real32 filmY = {0};
    lane_real32 filmW = lane_F32_FROM_1F32(fW);
    lane_real32 filmH = lane_F32_FROM_1F32(fH);
    lane_real32 filmHalfW = lane_F32_1F32_Mul(filmW, 0.5f);
    lane_real32 filmHalfH = lane_F32_1F32_Mul(filmH, 0.5f);
    uint32 maxBounceCount = workQueue->maxBounceCount;

    uint32 raysPerPixel   = workQueue->raysPerPixel;
    lane_uint32 bouncesComputed = {0};

    uint32 startX = workOrder->startX;
    uint32 startY = workOrder->startY;
    uint32 endX = workOrder->endX;
    uint32 endY = workOrder->endY;

    for(uint32 y = startY;
               y < endY;
               y++)
    {
        //map from -1 to 1
        uint32 *at = GetImagePixel32(image, startX, y); 

        filmY = lane_F32_FROM_1F32(1.0f - (2.0f * ((real32)y / (real32)imageH)));

       for(uint32 x = startX;
                  x < endX;
                  x++)
       {
           filmX = lane_F32_FROM_1F32(1.0f - (2.0f * ((real32)x / (real32)imageW)));

           uint32 laneWidth = LANEWIDTH;
           uint32 laneRayCount = raysPerPixel / laneWidth;
           Assert(laneRayCount * laneWidth == raysPerPixel);
           real32 contrib = 1.0f / laneRayCount;
           //lane from here!
           lane_vec3 color = {0};
           for(uint32 r = 0;
                      r < laneRayCount;
                      r++)
           {


                   lane_vec3 filmCamX  = lane_V3_F32_Mul(lane_V3_F32_Mul(camX, filmHalfW), filmX);
                   lane_vec3 filmCamY  = lane_V3_F32_Mul(lane_V3_F32_Mul(camY, filmHalfH), filmY);
                   lane_vec3 filmPoint = lane_V3_Add(lane_V3_Add(filmCenter, filmCamX), filmCamY);

                   lane_vec3 rayOrigin = camPosition;
                   lane_vec3 rayDir = lane_V3_Sub(filmPoint, camPosition);
                   rayDir = lane_V3_NormalizeZero(rayDir);

               lane_vec3 finalBounceColor = {0}; 
               lane_vec3 attenuation = lane_V3_FROM_1F32(1);

               lane_uint32 laneMask = lane_U32_FROM_1U32(0xffffffff);


           
               for(uint32 rayI = 0; 
                          rayI < workQueue->maxBounceCount;
                          rayI++)
               {
                   lane_real32 hitDistance = lane_F32_FROM_1F32(F32MAX);
                   lane_uint32 hitIndex = {0};
                   lane_vec3 nextNormal = {0};

                   lane_uint32 laneIncrement = lane_U32_FROM_1U32(1);
                   bouncesComputed = lane_U32_Add(bouncesComputed, lane_U32_And(laneIncrement,laneMask));
                   
                     for(uint32 p = 0;
                                p < rayWorld->planeCount;
                                p++)
                     {
                         plane rayPlane = rayWorld->planes[p];
                         lane_vec3 planeNormal      = lane_V3_FROM_1V3(rayPlane.normal);
                         lane_real32 normalDistance = lane_F32_FROM_1F32(rayPlane.normalDistance);
           
                         //position = f(t) = rayOrigin + t * rayDir
                         //ray equation = transpose(normal, position) + normalDistance
                         //t = -normalDistance - Inner(normal, rayOrigin) / Inner(normal, rayDirection)
                         //distance = t
           
                         lane_real32 distanceDenominator = lane_V3_Inner(planeNormal, rayDir);
                         lane_uint32 dDenominatorMask = lane_U32_Or(lane_F32_1F32_LT(distanceDenominator, -0.00001f), lane_F32_1F32_GT(distanceDenominator, 0.00001f));

                         if(!lane_MaskIsZero(dDenominatorMask))
                         {

                             lane_real32 t = (lane_F32_Sub(lane_F32_Neg(normalDistance), lane_V3_Inner(planeNormal, rayOrigin)));
                             t = lane_F32_Div(t, distanceDenominator);
                             lane_uint32 tMask = lane_U32_And(lane_F32_1F32_GT(t, 0.001f), lane_F32_LT(t, hitDistance));
                             //lane_uint32 tMask = ((t > 0.001f) && (t < hitDistance));
                             //real32 distance = RayIntersects(rayOrigin, rayDir, rayPlane.normal, rayPlane.normalDistance);
                             lane_uint32 hitMask = lane_U32_And(dDenominatorMask, tMask);
                             if(!lane_MaskIsZero(hitMask))
                             {
                                 lane_uint32 materialIndex  = lane_U32_FROM_1U32(rayPlane.materialIndex);
#if 1
                                 lane_F32_ConditionalSet_U32M(hitMask, &hitDistance, t);
                                 lane_U32_ConditionalSet(hitMask, &hitIndex, materialIndex);
                                 lane_V3_ConditionalSet_U32M(hitMask, &nextNormal, planeNormal);
#else
                                 hitDistance = t;
                                 hitIndex = materialIndex;
                                 nextNormal = planeNormal;
#endif
                             }
                         }
           
                         
                     }
           
                     
                     for(uint32 s = 0;
                                s < rayWorld->sphereCount;
                                s++)
                     {
                         sphere raySphere = rayWorld->spheres[s];
           
                         //position = f(t) = rayOrigin + t * rayDir
                         //shere equation = transpose(position, sphereOrigin) - sphereRadius^2
                         lane_vec3 spherePosition  = lane_V3_FROM_1V3(raySphere.position);
                         lane_real32 sphereRadius  = lane_F32_FROM_1F32(raySphere.radius);
           
                         lane_vec3 sphereFromOrigin = lane_V3_Sub(rayOrigin, spherePosition);
                         lane_real32 a = lane_V3_Inner(rayDir, rayDir);
                         lane_real32 b = lane_V3_Inner(sphereFromOrigin, rayDir);
                         b = lane_F32_1F32_Mul(b, 2);
                         lane_real32 c = lane_V3_Inner(sphereFromOrigin, sphereFromOrigin);
                         c = lane_F32_Sub(c, lane_F32_Mul(sphereRadius, sphereRadius));
           
                         lane_real32 todaEstaPoronga = lane_F32_Sub(lane_F32_Mul(b, b), lane_F32_Mul(lane_F32_1F32_Mul(a, 4), c));
                         lane_real32 rootTerm = lane_F32_Sqrt(todaEstaPoronga);
                         lane_uint32 rootMask = lane_F32_1F32_GT(rootTerm, 0.00001f);

                         if(!lane_MaskIsZero(rootMask))
                         {
                             lane_real32 denominator = lane_F32_1F32_Mul(a, 2);
                             lane_real32 tP = lane_F32_Div(lane_F32_Add(lane_F32_Neg(b), rootTerm), denominator);
                             lane_real32 tN = lane_F32_Div(lane_F32_Sub(lane_F32_Neg(b), rootTerm), denominator);

                             lane_real32 t = tP;
                             lane_uint32 tPickMask = lane_U32_And(lane_F32_1F32_GT(tN, 0), lane_F32_LT(tN, tP));
                             lane_F32_ConditionalSet_U32M(tPickMask, &t, tN);

                             lane_uint32 tMask = lane_U32_And(lane_F32_1F32_GT(t, 0.001f), lane_F32_LT(t, hitDistance));
                             lane_uint32 hitMask = lane_U32_And(rootMask, tMask);
                             if(!lane_MaskIsZero(hitMask))
                             {
                                lane_uint32 materialIndex = lane_U32_FROM_1U32(raySphere.materialIndex);
#if 0
                                hitDistance = t;
                                hitIndex = materialIndex;
                                lane_vec3 secondParam = lane_V3_NormalizeZero(lane_V3_Add(lane_V3_F32_Mul(rayDir, t), sphereFromOrigin));
                                nextNormal = secondParam;
#endif
                                lane_F32_ConditionalSet_U32M(hitMask ,&hitDistance, t);
                                lane_U32_ConditionalSet(hitMask ,&hitIndex, materialIndex);
                                lane_vec3 secondParam = lane_V3_NormalizeZero(lane_V3_Add(lane_V3_F32_Mul(rayDir, t), sphereFromOrigin));
                                lane_V3_ConditionalSet_U32M(hitMask ,&nextNormal, secondParam);
                             }
                         }
                     }
           
                     //material hittedMaterial = rayWorld->materials[hitIndex];


                     lane_vec3 reflectionColor = lane_Gather_V3(rayWorld->materials, hitIndex, reflectionColor);
                     //lane_vec3 emitColor       = lane_V3_U32_And(lane_Gather_V3(rayWorld->materials, hitIndex, emitColor), laneMask);
                     lane_vec3 emitColor       = lane_Gather_V3(rayWorld->materials, hitIndex, emitColor);
                     emitColor                 = lane_V3_U32_And(emitColor, laneMask);
                     lane_real32 scatter       = lane_Gather_F32(rayWorld->materials, hitIndex, scatter);

                     finalBounceColor = lane_V3_Add(finalBounceColor, lane_V3_Mul(attenuation, emitColor));
                     laneMask = lane_U32_And(laneMask, lane_U32_1U32_NEQ(hitIndex, 0));
                     if(lane_MaskIsZero(laneMask)) 
                     {
                         break;
                     }
                     else
                     {
                        //mask out those who didn't get hit.
                        lane_real32 attenuationCos = lane_F32_Max(lane_V3_Inner(lane_V3_1F32_Mul(rayDir, -1.0f), nextNormal), lane_F32_FROM_1F32(0));
               
                        //attenuation = vec3_Hadamard(attenuation, vec3_MultShittedMaterial.reflectionColor);
                        attenuation = lane_V3_Mul(attenuation, lane_V3_F32_Mul(reflectionColor, attenuationCos));
               
                        rayOrigin = lane_V3_Add(rayOrigin, lane_V3_F32_Mul(rayDir, hitDistance));
               
                        lane_real32 rayBounce = lane_F32_1F32_Mul(lane_V3_Inner(rayDir, nextNormal), 2.0f);
                        lane_vec3 pureBounce  = lane_V3_Sub(rayDir, lane_V3_F32_Mul(nextNormal, rayBounce));
    
                        lane_vec3 randomSum = lane_V3_Add(nextNormal ,lane_V3(RandomBilateral(&workOrder->random),
                                                                              RandomBilateral(&workOrder->random),
                                                                              RandomBilateral(&workOrder->random)
                                                                              ));
               
                        lane_vec3 randomBounce = lane_V3_NormalizeZero(randomSum);
                        rayDir = lane_V3_NormalizeZero(lane_V3_Lerp(randomBounce, scatter ,pureBounce));
                      //  rayDir = vec3_NormalizeZero(pureBounce);
               
                        //finalBounceColor = rayWorld->materials[hitIndex].reflectionColor;
                     }
               }
               //return
               lane_vec3 cAdd = lane_V3_Add(color, finalBounceColor); 
               color = lane_V3_F32_Mul(cAdd, lane_F32_FROM_1F32(contrib));
           }

//           vec4 finalColor = { 255 * LinearToSRGB(color.x) , 255 * LinearToSRGB(color.y) , 255 * LinearToSRGB(color.z) , 255};
           vec3 finalColor3 = {0};
           finalColor3 = lane_HorizontalAdd_V3(color);
           finalColor3.x = 255 * LinearToSRGB(finalColor3.x);
           finalColor3.y = 255 * LinearToSRGB(finalColor3.y);
           finalColor3.z = 255 * LinearToSRGB(finalColor3.z);

           vec4 finalColor = {finalColor3.x, finalColor3.y, finalColor3.z, 255};
                          
                         
           uint32 pixel = RGBAPack(finalColor);
           *at++ = pixel; 
        }
    }
   uint32 totalBouncesComputed = lane_HorizontalAdd_U32(bouncesComputed);
   thread_Add(&workQueue->bouncesComputed, totalBouncesComputed);
   thread_Add32(&workQueue->tilesComputed, 1);
   return 1;
}


static DWORD WINAPI
WorkerThread(void *param)
{
    work_queue *workQueue = (work_queue *)param;
    while(RenderTile(workQueue));
    return(0);
}
static void
thread_Create(void *param)
{
   DWORD threadID;
   HANDLE threadHandle = CreateThread(0, 0, WorkerThread, param, 0, &threadID);
   CloseHandle(threadHandle);
}
static uint32
GetCoreCount()
{
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    uint32 result = sysInfo.dwNumberOfProcessors;
    return(result);
}
int main(int argc, char **args)
{

    material rayMaterials[6] = {0};

    rayMaterials[0].emitColor       = V3(0.4f, 0.5f, 0.8f);
    rayMaterials[1].reflectionColor = V3(0.3f, 0.5f, 0.5f);
    rayMaterials[2].reflectionColor = V3(0.7f, 0.5f, 0.3f);
    rayMaterials[3].emitColor = V3(1.0f + 3, 0.0f, 0.0f);
    rayMaterials[3].reflectionColor = V3(0.3f, 0.0f, 0.0f);
    rayMaterials[4].reflectionColor = V3(1.0f, 0.0f, 0.2f);
    rayMaterials[4].scatter = 1;
    rayMaterials[5].reflectionColor = V3(0.2f, 0.8f, 0.6f);
    rayMaterials[5].scatter = 0.6f;


    plane rayPlanes[1] = {0};
    rayPlanes[0].normal.x = 0;
    rayPlanes[0].normal.y = 0;
    rayPlanes[0].normal.z = 1;
    rayPlanes[0].normalDistance = 0;
    rayPlanes[0].materialIndex = 1;

    sphere raySpheres[3] = {0};
    raySpheres[0].position = V3(0, 0, 0);
    raySpheres[0].radius = 0.7f;
    raySpheres[0].materialIndex = 2;

    raySpheres[1].position = V3(2, 0, 0);
    raySpheres[1].radius = 1.0f;
    raySpheres[1].materialIndex = 3;

    raySpheres[2].position = V3(3, -2, 2);
    raySpheres[2].radius = 1.0f;
    raySpheres[2].materialIndex = 4;

    world rayWorld = {0};
    rayWorld.materialCount = ARRAYCOUNT(rayMaterials);
    rayWorld.materials = rayMaterials;

    rayWorld.planeCount = ARRAYCOUNT(rayPlanes);
    rayWorld.planes = rayPlanes;

    rayWorld.sphereCount = ARRAYCOUNT(raySpheres);
    rayWorld.spheres = raySpheres;





    uint32 imageW = 1280;
    uint32 imageH = 720;
    uint32 imageSize = imageW * imageH * 4;
    uint32 *imageAndPixels = (uint32 *)malloc(sizeof(image_data) + imageSize);
    image_data *image = (image_data *)imageAndPixels;
    image->width = imageW;
    image->height = imageH;
    image->bpp = 4;
    image->pixels = (uint8 *)imageAndPixels + sizeof(image_data);
    
    printf("Starting raycasting...\n");
    uint32 coreCount = 1;// GetCoreCount();

    uint32 tileWidth  = (image->width + coreCount - 1) / coreCount;
    uint32 tileHeight = tileWidth;
    printf("Total cores %d with %dx%d \n", coreCount, tileWidth, tileHeight);

    clock_t startTime = clock();

    uint32 tileCountX = (image->width + tileWidth - 1) / tileWidth;
    uint32 tileCountY = (image->height + tileHeight - 1) / tileHeight;
    uint32 tilesTotal = tileCountX * tileCountY;


    work_queue queue = {0};
    queue.workOrders = malloc(tilesTotal * sizeof(work_order));
    work_order *workOrders = queue.workOrders;

    uint32 tilesComputed = 0;

    for(uint32 tY = 0;
               tY < tileCountY;
               tY++)
    {
        uint32 startY = tY * tileHeight;
        uint32 maxY = startY + tileHeight;
        if(maxY > image->height)
        {
            maxY = image->height;
        }

          for(uint32 tX = 0;
                     tX < tileCountX;
                     tX++)
          {

            uint32 startX = tX * tileWidth;
            uint32 maxX = startX + tileWidth;
            if(maxX > image->width)
            {
                maxX = image->width;
            }


            work_order *workOrder = workOrders + queue.workOrderCount++;
            Assert(queue.workOrderCount <= tilesTotal);

            workOrder->rayWorld = &rayWorld;
            workOrder->image  = image;
            workOrder->startX = startX; 
            workOrder->startY = startY;
            workOrder->endX   = maxX;
            workOrder->endY   = maxY;
            //workOrder->random.seed = rand(); 
            workOrder->random.x    = lane_U32(tX * tY,
                                              tX * tY + 1 * 20,
                                              tX * tY + 2 * 51,
                                              tX * tY + 3 * 44
#if LANEWIDTH == 8                              
                                              ,tX * tY + 4 * 2451,
                                              tX * tY + 5 * 41,
                                              tX * tY + 6 * 121,
                                              tX * tY + 7 * 831
#endif                                        
                                              );
            //workOrder->random.seed = lane_U32_FROM_1U32(tX + 15 * 3 + tY);
            uint32 seed = tX + 15 * 3 + tY;
            workOrder->random.seed = lane_U32(seed, seed++, seed++, seed++
#if LANEWIDTH == 8                              
                                              ,seed++, seed++, seed++, seed++
#endif                                        
                                              );
          }
    }
    Assert(queue.workOrderCount == tilesTotal);

    thread_Add((uint64 *)&queue.nextWorkOrderIndex, 0);
    for(uint32 cI = 1;
               cI < coreCount;
               cI++)
    {
        thread_Create(&queue);
    }


    while(queue.tilesComputed < tilesTotal)
    {
         if(RenderTile( &queue))
         {
           real32 percent = ((real32)queue.tilesComputed / tilesTotal) * 100.0f;
           printf("Raycasting to image: %.2f%% completed\n", percent);
           fflush(stdout);
         }
    }

    clock_t endTime = clock();
    clock_t totalTime = endTime - startTime;

    real64 bouncesPerMs= (real64)totalTime / (real64)queue.bouncesComputed;
    printf("Done!. Total time: %dms\n",totalTime); 
    printf("Performance: %f bounces/ms", bouncesPerMs);
    

    agu_BmpTransformAndToFile("raycast.bmp", (uint8 *)image->pixels, imageW, imageH, 4);
    //agu_BmpToFile("raycast.bmp", (uint8 *)pixels, imageW, imageH, 4);

    free(image);
    return 0;
}
