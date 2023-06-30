#define _CRT_SECURE_NO_WARNINGS
#include <stdlib.h>

#ifndef AGU_DEFINITIONS
#define AGU_DEFINITIONS
//Simple definitions. I like them
typedef unsigned int u32;
typedef int s32;
#endif

//
//=======File headers
//
#pragma pack(push, 1)
//LONG = int32
//WORD = uint16
//DWORD = uint32
//Note(Agu):Complete header for bmps with alpha
typedef struct
{
	uint16   FileType;
	uint32  FileSize;  
	uint16   Reserved1; 
	uint16   Reserved2; 
	uint32 BitmapOffset;

	uint32 Size;
	int32  Width;
	int32  Height;
	uint16  Planes;
	uint16  BitsPerPixel;

	uint32 Compression;
	uint32 SizeOfBitmap;
	int32  HorzResolution;
	int32  VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;

    uint32 r;
    uint32 g;
    uint32 b;
    uint32 a;
    uint32 CSType;
    int32  RedX;
	int32  RedY;
	int32  RedZ;
	int32  GreenX;
	int32  GreenY;
	int32  GreenZ;
	int32  BlueX;
	int32  BlueY;
	int32  BlueZ;
	uint32 GammaRed;
	uint32 GammaGreen;
	uint32 GammaBlue;
} bmp_header2;
#pragma pack(pop)
//
//
//







typedef struct{
    uint8 *contents;
    uint64 size;
}file_data;

typedef struct{
    uint32 width;
    uint32 height;
    uint32 bpp;
    uint8 *pixels;
}image_data;

inline void *
PngAllocateImagePixels(uint32 w, uint32 h, uint32 bpp)
{
    void *result = calloc(1, h * (w * bpp + 1));
    return(result);
}
inline void *
AllocatePixels(uint32 ImageSize) 
{
    void *result = calloc(1, ImageSize);
    return(result);
}

inline uint32 
ReverseBits(uint32 v, uint32 bits)
{
        uint32 result = 0;
     for(uint32 bitflipi = 0;
                bitflipi <= (bits / 2);
                bitflipi++)
             {
               uint32 inv = (bits - (bitflipi + 1));
               result |= ((v >> bitflipi) & 0x1) << inv;
               result |= ((v >> inv) & 0x1) << bitflipi;
             }
 return(result);
}
static void
EndianSwap32(uint32 *v)
{
    uint32 _v = (*v);
#if 0
    uint8 b1 = ((_v >> 0) & 0xff);
    uint8 b2 = ((_v >> 8) & 0xff); 
    uint8 b3 = ((_v >> 16) & 0xff);
    uint8 b4 = ((_v >> 24) & 0xff);

    _v = (b1 << 24) | (b2 << 16) | (b3 << 8) | (b4 << 0);
#endif
    _v = (_v << 24) |
         ((_v & 0xff00) << 8) |
         ((_v >> 8) & 0xff00) |
         (_v >> 24);
    (*v) = _v;
}
static void
EndianSwap24(uint32 *v)
{
    uint32 _v = (*v);
    _v = ((_v << 16) |
          (_v << 8) |
         (_v >> 16));
    (*v) = _v;
}
static void
EndianSwap16(uint16 *v)
{
    uint16 _v = (*v);
    _v = ((_v << 8) & (_v >> 8));
    (*v) = _v;
}


#include "agu_bmp.h"





static file_data 
CRTReadEntireFile(uint8 * name)
{
    FILE *filePtr = fopen(name, "rb");
    //Get size
    file_data result = {0};
    if(filePtr)
    {
       fseek(filePtr, 0, SEEK_END);
       uint64 fileSize = ftell(filePtr);
       fseek(filePtr, 0, SEEK_SET);
       Assert(fileSize < 1024*1024*1024 * 4ull);
       
#if 0
       file->area.start= 
       file->size     = (uint32)FileSize;
#endif
       result.contents = (char *)malloc(fileSize);
       result.size = (uint32)fileSize;

       fread(result.contents, 1, fileSize, filePtr);
       fclose(filePtr);
    }
    else
    {
        printf("Error on ReadEntireFile");
    }
    return(result);
}
static stream_data 
CRTReadEntireFileToStream(uint8 * name)
{
    FILE *filePtr = fopen(name, "rb");
    //Get size
    stream_data result = {0};
    stream_buffer fileBuffer = {0};
    if(filePtr)
    {
       fseek(filePtr, 0, SEEK_END);
       uint64 fileSize = ftell(filePtr);
       fseek(filePtr, 0, SEEK_SET);
       Assert(fileSize < 1024*1024*1024 * 4ull);
       
#if 0
       file->area.start= 
       file->size     = (uint32)FileSize;
#endif
       fileBuffer.contents = (char *)malloc(fileSize);
       fileBuffer.size = (uint32)fileSize;

       fread(fileBuffer.contents, 1, fileSize,filePtr);
       fclose(filePtr);
    }
    else
    {
        printf("Error on ReadEntireFile");
    }
    result = stream_CreateFromBuffer(fileBuffer);
    return(result);
}

#if 1
static image_data 
agu_LoadImage(uint8 *path) 
{
    image_data result = {0};
    stream_data stream  = CRTReadEntireFileToStream(path); 

    uint16 signature = *(uint16 *)stream.buffer.contents;
    if(signature == 'MB') //BM or 4d42
    {
      result = bmp_FromStream(&stream);
    }
    else
    {
      //result = ParsePng(area, &stream, infostream);
        printf("png loader yet not implemented!");
    }
    return(result);
}
static void
agu_FreeImage(image_data *image)
{
    free(image->pixels);
    image->width  = 0;
    image->height = 0;
    image->bpp    = 0;
}
#endif
