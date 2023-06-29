#pragma pack(push, 1)
//LONG = int32
//WORD = uint16
//DWORD = uint32
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

typedef struct{
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
}bmp_header_basic;

#pragma pack(pop)

//Note(Agu): Bmps are read from bottom-left.
inline uint32
rbinvert(uint32 v)
{
    return ((v >> 16) & 0xff) |
           ((v << 16) & 0xff0000) |
           (v & 0xff00ff00);
}
static void
PixelsToBmp(uint8 *pixels , uint32 Width, uint32 Height, uint32 BytesPerPixel)
{
    uint32 stride = Width * BytesPerPixel;
    uint32 size   = Width * Height * BytesPerPixel;
    uint32 MHeight = Height / 2;
    uint32 MWidth = Width / 2;

    uint8 *tl = 0;
    uint8 *tr = 0;
    uint8 *bl = 0;
    uint8 *br = 0;
    
    for(uint32 y = 0;
               y < MHeight;
               y++)
    {
         bl = pixels + size - (stride *(y+1));
         br = bl + stride - 4; 
         tl = pixels + (stride * y);
         tr = tl + stride - 4;

         for(uint32 x = 0;
                    x < MWidth;
                    ++x)
         {
             uint32 trPixel = *(uint32 *)(tr);
             uint32 brPixel = *(uint32 *)(br);

             uint32 tlPixel = *(uint32 *)(tl);
             uint32 blPixel = *(uint32 *)(bl);
#if 0
             tr[0] = brPixel >> 16;
             tr[1] = brPixel >> 8;
             tr[2] = brPixel;
             tr[3] = brPixel >> 24;
#endif
             *(uint32 *)tr = rbinvert(brPixel);

             br[0] = trPixel >> 16;
             br[1] = trPixel >> 8;
             br[2] = trPixel;
             br[3] = trPixel >> 24;

             tl[0] = blPixel >> 16;
             tl[1] = blPixel >> 8;
             tl[2] = blPixel;
             tl[3] = blPixel >> 24;

             bl[0] = tlPixel >> 16;
             bl[1] = tlPixel >> 8;
             bl[2] = tlPixel;
             bl[3] = tlPixel >> 24;
             tl += 4;
             bl += 4;

             tr -= 4;
             br -= 4;

         }
    }
}
static void
ProcessPixelsBE4(uint8 *pixels, uint8* FinalPixels, uint32 Width, uint32 Height, uint32 BytesPerPixel)
{
    uint8 *to = FinalPixels;
    uint8 *at = pixels;
    uint32 stride = Width * BytesPerPixel;
    uint32 size   = Width * Height * BytesPerPixel;

    for(uint32 y = 0;
               y < Height;
               y++)
    {
         at = (pixels + size) - stride * (y+1);

         for(uint32 x = 0;
                    x < Width;
                    ++x)
         {
             uint32 pixel = *(uint32 *)(at + x*4);

             to[0] = pixel >> 16;
             to[1] = pixel >> 8;
             to[2] = pixel;
             to[3] = pixel >> 24;
             to += 4;
         }
    }
}
static void
ProcessPixelsBE3(uint8 *pixels, uint8* FinalPixels, uint32 Width, uint32 Height, uint32 BytesPerPixel)
{
    uint8 *to = FinalPixels;
    uint8 *at = pixels;
    uint32 stride = Width * BytesPerPixel;
    uint32 size   = Width * Height * BytesPerPixel;

    for(uint32 y = 0;
               y < Height;
               y++)
    {
         at = (pixels + size) - stride * (y+1);

         for(uint32 x = 0;
                    x < Width;
                    ++x)
         {
             uint32 pixel = *(uint32 *)(at + x*3);
             endian_swap_32(&pixel);

             to[0] = pixel >> 8;
             to[1] = pixel >> 16;
             to[2] = pixel >> 24;
             to += 3;
         }
    }
}

static image_data
bmp_from_stream(memory_area *area, stream_data *stream)
{
  image_data image = {0};

  bmp_header2 header = *(bmp_header2 *)stream->buffer.contents; 
  stream_consume_size(stream, header.BitmapOffset);
  uint32 BytesPerPixel = header.BitsPerPixel / 8;
  uint32 ImageSize = header.Height * (header.Width * BytesPerPixel + 1);

  uint8 *pixels = stream->buffer.contents;
  uint8 *FinalPixels = AllocatePixels(area, ImageSize); 
  if(BytesPerPixel == 4)
  {
    ProcessPixelsBE4(pixels, FinalPixels, header.Width, header.Height, BytesPerPixel);
    //InvertPixel
  }
  else if(BytesPerPixel == 3)
  {
    ProcessPixelsBE3(pixels, FinalPixels, header.Width, header.Height, BytesPerPixel);
  }
  else
  {
      Assert(0);
  }


  image.width  = header.Width;
  image.height = header.Height;
  image.bpp    = BytesPerPixel;
  image.pixels = FinalPixels;

  return(image);
}

static void
BmpToStream(image_data *image,
            stream_data *stream, 
            stream_data *errorstream)
{
#if 1

  Assert(image->bpp == 4);
  uint32 ImageSize = image->height * (image->width * image->bpp);

  bmp_header2 header = {0};
  header.FileType       = 0x4D42;
  header.FileSize       = ImageSize + sizeof(bmp_header2);
  header.SizeOfBitmap   = ImageSize;
  header.Size           = sizeof(bmp_header2) - 14;
  header.BitmapOffset   = sizeof(bmp_header2);
  header.Width          = image->width;
  header.Height         = image->height;
  header.BitsPerPixel   = image->bpp * 8;
  header.Planes         = 1;
  header.Compression    = 3; //To preserve transparency
  header.HorzResolution = 0;
  header.VertResolution = 0;
  header.r              = 16711680;
  header.g              = 65280;
  header.b              = 255;
  header.a              = 4278190080;
//  header.CSType = 1934772034;

  stream_PushData(stream,&header ,sizeof(bmp_header2));
  stream_PushData(stream,image->pixels,ImageSize);
#endif

}
inline void
BmpTransformAndToStream(image_data *image,
                        stream_data *stream, 
                        stream_data *infostream)
{
  PixelsToBmp(image->pixels, image->width, image->height, image->bpp);
  BmpToStream(image, stream, infostream);
}

inline u32
bmp_check_signature(platform_api *platform, platform_file_handle fileHandle)
{
	u16 signature = 0;
	platform->f_read_from_file(fileHandle, 0, sizeof(u16), &signature);

	u32 result = (signature == 'MB');
	return(result);

}

inline image_data
bmp_get_image_data(platform_api *platform, platform_file_handle fileHandle)
{
	bmp_header_basic basicInfo = {0};
	platform->f_read_from_file(fileHandle, 0, sizeof(bmp_header_basic), &basicInfo);

	image_data imageData = {0};

    imageData.width  = basicInfo.Width;
    imageData.height = basicInfo.Height;
    imageData.bpp    = basicInfo.BitsPerPixel;

	return(imageData);
}
