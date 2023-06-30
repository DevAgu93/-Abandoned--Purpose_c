
//Note(Agu): Bmps are readed from bottom-left.
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
             EndianSwap32(&pixel);

             to[0] = pixel >> 8;
             to[1] = pixel >> 16;
             to[2] = pixel >> 24;
             to += 3;
         }
    }
}
#if 1
static image_data
bmp_FromStream(stream_data *stream)
{
  image_data image = {0};

  bmp_header2 header = *(bmp_header2 *)stream->buffer.contents; 
  stream_ConsumeSize(stream, header.BitmapOffset);
  uint32 BytesPerPixel = header.BitsPerPixel / 8;
  uint32 ImageSize = header.Height * (header.Width * BytesPerPixel + 1);

  uint8 *pixels = stream->buffer.contents;
  uint8 *FinalPixels = AllocatePixels(ImageSize); 
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



static image_data
bmp_FromFile(file_data *bmpFile)
{
  image_data image = {0};

  uint8 *fileContents = bmpFile->contents;
  bmp_header2 header = *(bmp_header2 *)fileContents;
  fileContents += header.BitmapOffset;

  uint32 BytesPerPixel = header.BitsPerPixel / 8;
  uint32 ImageSize = header.Height * (header.Width * BytesPerPixel + 1);
  uint8 *pixels = fileContents; 
  uint8 *FinalPixels = AllocatePixels(ImageSize); 
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
agu_BmpToFile(uint8 *imagePath, uint8 *pixels, uint32 w, uint32 h, uint32 bpp)
{
  FILE *bmpFile = fopen(imagePath, "wb");
  if(!bmpFile)
  {
      printf("Error while creating the bmp file to the specified path");
      return;
  }

  Assert(bpp == 4);
  uint32 ImageSize = h * (w * bpp);

  bmp_header2 header = {0};
  header.FileType       = 0x4D42;
  header.FileSize       = ImageSize + sizeof(bmp_header2);
  header.SizeOfBitmap   = ImageSize;
  header.Size           = sizeof(bmp_header2) - 14;
  header.BitmapOffset   = sizeof(bmp_header2);
  header.Width          = w;
  header.Height         = h;
  header.BitsPerPixel   = bpp * 8;
  header.Planes         = 1;
  header.Compression    = 3; //To preserve transparency
  header.HorzResolution = 0;
  header.VertResolution = 0;
  header.r              = 16711680;
  header.g              = 65280;
  header.b              = 255;
  header.a              = 4278190080;
//  header.CSType = 1934772034;

  fwrite(&header,sizeof(bmp_header2), 1,bmpFile);
  fwrite(pixels, ImageSize,1,bmpFile);
  fflush(bmpFile);
  fclose(bmpFile);
}
static void
agu_BmpTransformAndToFile(uint8 *imagePath, uint8 *pixels, uint32 w, uint32 h, uint32 bpp)
{
  PixelsToBmp(pixels, w, h, bpp);
  agu_BmpToFile(imagePath, pixels, w, h, bpp);
}
static void
agu_BmpToFileImage(uint8 *imagePath, image_data *image)
{
    agu_BmpToFile(imagePath, image->pixels, image->width, image->height, image->bpp);
}
inline void
agu_BmpTransformAndToFileImage(image_data *image, uint8 *imagePath)                                             
{
  PixelsToBmp(image->pixels, image->width, image->height, image->bpp);
  agu_BmpToFile(imagePath, image->pixels, image->width, image->height, image->bpp);
}
#endif
