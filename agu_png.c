#include "agu_png.h"

static void
png_endian_swap_u32(uint32 *v)
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

inline uint32 
png_reverse_bits(uint32 v, uint32 bit_amount)
{
        uint32 result = 0;
     for(uint32 bitflipi = 0;
                bitflipi <= (bit_amount / 2);
                bitflipi++)
     {
               uint32 inv = (bit_amount - (bitflipi + 1));
               result |= ((v >> bitflipi) & 0x1) << inv;
               result |= ((v >> inv) & 0x1) << bitflipi;
     }

     return(result);
}

static png_huffman 
AllocHuffman(memory_area *area, uint32 LengthBits)
{
    Assert(LengthBits <= 16);
    png_huffman result = {0};

    result.BitLength = LengthBits;
    result.EntryCount = (1 << LengthBits);
    result.Entries = memory_area_push_array(
			area,
			png_huffman_entry,
			result.EntryCount); 
    
    return result;
}
static void
ComputeHuffman(uint32 symbolcount, uint32 *symbolcodelength, png_huffman *result)
{
    //Count number of codes for each length
    //Count how many codes have the same length
    uint32 CodeLengthCount[16] = {0};
    for(uint32 c = 0; c < symbolcount; c++)
    {
        uint32 N = symbolcodelength[c];
        Assert(N <= ArrayCount(CodeLengthCount));
       ++CodeLengthCount[N];
    }
    uint32 nextunusedcode[16] = {0};
    CodeLengthCount[0] = 0;

    //get the numerical values (code) depending of how long the count for each length is
    //where every code should start
    uint32 code = 0;
    uint32 arraycount = ArrayCount(nextunusedcode);
    for(uint32 bits = 1;
               bits < arraycount;
               bits++)
    {
        code = (code + CodeLengthCount[bits - 1]) << 1;
        nextunusedcode[bits] = code;
    }
    //Assign the nextcodes to the tree codes as long as their length > 0
    //This generates the code for each symbol in order
    for(uint32 s = 0; 
               s < symbolcount;
               s++)
    {
        uint32 codelengthbits = symbolcodelength[s];
        if(codelengthbits)
        {
            //The codes are read in sequence
            Assert(codelengthbits < ArrayCount(nextunusedcode));
            uint32 code = nextunusedcode[codelengthbits]++; 

            uint32 RemainingBits = result->BitLength - codelengthbits;
            uint32 entrybits = (1 << RemainingBits); 
            for(uint32 i = 0;
                       i < entrybits;
                       i++)
            {
                //uint32 bindex = (i << codelengthbits) | code;
                uint32 bindex = ((code << RemainingBits) | i);
                uint32 index = png_reverse_bits(bindex , result->BitLength);

                png_huffman_entry *entry = result->Entries + index;

                entry->BitsUsed = (uint16)codelengthbits;
                entry->Symbol   = (uint16)s;
                Assert(entry->BitsUsed == codelengthbits);
                Assert(entry->Symbol == s);
            }
        }

    }
}
static uint32 
DecodeHuffman(png_huffman *h, stream_data *stream)
{
    uint32 EntryIndex = stream_PeekBits(stream, h->BitLength);
    Assert(EntryIndex < h->EntryCount);
    png_huffman_entry entry = h->Entries[EntryIndex];

    uint32 result = entry.Symbol; 
    stream_DiscardBits(stream, entry.BitsUsed);
    Assert(entry.BitsUsed);
    return(result);
}
inline void *
PngAllocateImagePixels(memory_area *area, uint32 w, uint32 h, uint32 bpp)
{
    void *result = memory_area_push_size(area, h * (w * bpp + 1));
    return(result);
}
inline void *
AllocatePixels(memory_area *area, uint32 ImageSize) 
{
    void *result = memory_area_push_size(area, ImageSize);
    return(result);
}



static png_huffman_entry LengthTable[29] = 
{
    { 3, 0},
    { 4, 0},
    { 5, 0},
    { 6, 0},
    { 7, 0},
    { 8, 0},
    { 9, 0},
    { 10, 0},
    { 11, 1},
    { 13, 1},
    { 15, 1},
    { 17, 1},
    { 19, 2},
    { 23, 2},
    { 27, 2},
    { 31, 2},
    { 35, 3},
    { 43, 3},
    { 51, 3},
    { 59, 3},
    { 67, 4},
    { 83, 4},
    { 99, 4},
    { 115, 4},
    { 131, 5},
    { 163, 5},
    { 195, 5},
    { 227, 5},
    { 258, 0},
};

static png_huffman_entry DistTable[] = 
{
    { 1, 0},
    { 1, 0},
    { 1, 0},
    { 1, 0},
    { 1, 1},
    { 2, 1},
    { 3, 2},
    { 6, 2},
    { 9, 3},
    { 16, 3},
    { 23, 4},
    { 38, 4},
    { 53, 5},
    { 84, 5},
    { 115, 6},
    { 178, 6},
    { 241, 7},
    { 368, 7},
    { 495, 8},
    { 750, 8},
    { 1005, 9},
    { 1516, 9},
    { 2027, 10},
    { 3050, 10},
    { 4073, 11},
    { 6120, 11},
    { 8167 , 12},
    { 12262 , 12},
    { 16357 , 13},
    { 24548 , 13}
};
static png_huffman_entry DistTable2[] = 
{
    { 1, 0},
    { 2, 0},
    { 3, 0},
    { 4, 0},
    { 5, 1},
    { 7, 1},
    { 9, 2},
    { 13, 2},
    { 17, 3},
    { 25, 3},
    { 33, 4},
    { 49, 4},
    { 65, 5},
    { 97, 5},
    { 129, 6},
    { 193, 6},
    { 257, 7},
    { 385, 7},
    { 513, 8},
    { 769, 8},
    { 1025, 9},
    { 1537, 9},
    { 2049, 10},
    { 3073, 10},
    { 4097, 11},
    { 6145, 11},
    { 8193 , 12},
    { 12289 , 12},
    { 16385 , 13},
    { 24577 , 13}
};



inline int32
Paeth(uint8 a, uint8 b, uint8 c)
{
   int32 Pr = 0;
   int32 p = a + b - c;
   int32 pa = abs(p - a);
   int32 pb = abs(p - b);
   int32 pc = abs(p - c);
   if(pa <= pb && pa <= pc)
   {
     Pr = a;
   }
   else if(pb <= pc)
   {
     Pr = b;
   }
   else 
   {
     Pr = c;
   }
   return(Pr);
}


ReconstructPixels(uint32 width, uint32 height,uint32 BytesPerPixel,uint8 *DecompressedPixels,uint8 *FinalPixels)
{

          uint32 stride = width * BytesPerPixel;
          uint32 zero = 0;

          uint8 *s = DecompressedPixels;
          uint8 *d = FinalPixels;

          uint8 *a = 0; 
          uint8 *b = 0;
          uint8 *c = 0;
          for(uint32 y = 0; y < height; y++)
          {
              a = (uint8 *)&zero;
              b = (uint8 *)&zero;  //1 byte per filter
              c = (uint8 *)&zero; 

              d = FinalPixels + (stride * y);
              uint8 filter = *s++;
              switch(filter)
              {
                  case 0://Note(Agu): No filter
                      {
                          uint32 index = 0;
                          for(uint32 x = 0; x < width; x++) 
                          {
                              index = x * 4;

                              *(uint32 *)(d + index) = *(uint32 *)(s + index);
                          }
                      }break;
                  case 1: //Note(Agu):Sub. Difference between current byte and previous
                      {
                          uint8 *at = s;
                          uint8 *to = d;
                          for(uint32 x = 0; x < width; x++)
                          {
                              if(x)
                              {
                                  a = (to - 4);
                              }
                              to[0] = a[0] + at[0];
                              to[1] = a[1] + at[1];
                              to[2] = a[2] + at[2];
                              to[3] = a[3] + at[3];
                              at += 4;
                              to += 4;
                          }
                      }break;
                  case 2: //Note(Agu):Up. Difference between current byte and the one above
                      {
                          uint8 *at = s;
                          uint8 *to = d;
                          for(uint32 x = 0; x < width; x++)
                          {
                              b = to - stride; 

                              to[0] = b[0] + at[0];
                              to[1] = b[1] + at[1];
                              to[2] = b[2] + at[2];
                              to[3] = b[3] + at[3];
                              at += 4;
                              to += 4;
                          }
                      }break;
                  case 3: //Note(Agu):Average. NOT TESTED!!!
                      {
                          uint8 *at = s;
                          uint8 *to = d;
                          for(uint32 x = 0; x < stride; x++)
                          {
                              a = (uint8 *)&zero;
                              b = to - stride; 
                              if(x)
                              {
                                  a = (to - 4);
                              }

                              to[0] = at[0] + (((uint32)a[0] + (uint32)b[0]) / 2);
                              to[1] = at[1] + (((uint32)a[1] + (uint32)b[1]) / 2);
                              to[2] = at[2] + (((uint32)a[2] + (uint32)b[2]) / 2);
                              to[3] = at[3] + (((uint32)a[3] + (uint32)b[3]) / 2);
                              at += 4;
                              to += 4;
                          }
                      }break;
                  case 4: //Note(Agu): Paeth??????
                      {
                          uint8 *at = s;
                          uint8 *to = d;
                          for(uint32 x = 0; x < stride; x++)
                          {
                              b = to - stride; 
                              if(x)
                              {
                                  a = (to - 4);
                                  c = (a - stride); 
                              }

                              to[0] = at[0] + Paeth(a[0], b[0], c[0]); 
                              to[1] = at[1] + Paeth(a[1], b[1], c[1]);
                              to[2] = at[2] + Paeth(a[2], b[2], c[2]);
                              to[3] = at[3] + Paeth(a[3], b[3], c[3]);
                              at += 4;
                              to += 4;
                          }
                      }break;
                  default:
                      {
                          Assert(0);
                      }break;
              }
              s += stride;
          }
}

inline u32
png_check_signature_from_handle(platform_api *platform, platform_file_handle fileHandle)
{
	
	u8 signature[8] = {0};
	platform->f_read_from_file(fileHandle, 0, sizeof(signature), signature);

    uint32 supported = 1;

    supported &= signature[0] == 137;
    supported &= signature[1] == 80;
    supported &= signature[2] == 78;
    supported &= signature[3] == 71;
    supported &= signature[4] == 13;
    supported &= signature[5] == 10;
    supported &= signature[6] == 26;
    supported &= signature[7] == 10;

	return(supported);

}

inline u32 
png_CheckSignature(u8 *signature)
{
    //137 80 78 71 13 10 26 10
    uint32 supported = 1;

    supported &= signature[0] == 137;
    supported &= signature[1] == 80;
    supported &= signature[2] == 78;
    supported &= signature[3] == 71;
    supported &= signature[4] == 13;
    supported &= signature[5] == 10;
    supported &= signature[6] == 26;
    supported &= signature[7] == 10;

	return(supported);
}

inline u32 
png_CheckHeaderSignature(png_header *header)
{
    //137 80 78 71 13 10 26 10
    uint32 supported = 1;

    supported &= header->signature[0] == 137;
    supported &= header->signature[1] == 80;
    supported &= header->signature[2] == 78;
    supported &= header->signature[3] == 71;
    supported &= header->signature[4] == 13;
    supported &= header->signature[5] == 10;
    supported &= header->signature[6] == 26;
    supported &= header->signature[7] == 10;

	return(supported);
}

inline u32 
png_CheckSignature_u64(u64 signature)
{
	return(png_CheckSignature((u8 *)&signature));
}

static image_data
png_parse_from_stream(
		memory_area *area,
		stream_data *stream,
		stream_data *infostream)
{
  uint32 Compatible = 0;
  image_data image = {0};

  png_header *header = stream_consume_data(stream, png_header);
  u32 supported = png_CheckHeaderSignature(header);
  if(!supported)
  {
	  return(image);
  }

  stream_data CompressedData = stream_Create(area); 

  uint8 *FinalPixels = 0; 

  uint32 width = 0;
  uint32 height = 0;
  uint32 BytesPerPixel = 0;

  while(stream->buffer.size > 0)
  {
     png_chunk_header *chunkheader = stream_consume_data(stream, png_chunk_header);

     png_endian_swap_u32(&chunkheader->length);
     //Note(Agu): endian swapped the type to use it on the switch statement
     png_endian_swap_u32(&chunkheader->type32);

     void* chunkdata = stream_consume_size(stream, chunkheader->length);

     stream_pushf(infostream,"Header %c%c%c%c\n", chunkheader->type[3],
                                                  chunkheader->type[2],
                                                  chunkheader->type[1],
                                                  chunkheader->type[0]);
     png_chunk_footer *chunkfooter = stream_consume_data(stream, png_chunk_footer);

     png_endian_swap_u32(&chunkfooter->crc);

     stream_pushf(infostream, "crc %u \n", chunkfooter->crc);
     //Note(Agus): I can also do == 'text' for comparison but i'd have to flip it.
     switch(chunkheader->type32)
     {
		 //NOTE: the PLTE header always appears on headers with colour type 3 and NOT in 0 and 4, the rest is optional...
         case 'IHDR':
             {
                 png_ihdr *ihdr = (png_ihdr *)chunkdata; 
                 png_endian_swap_u32(&ihdr->width);
                 png_endian_swap_u32(&ihdr->height);
                 stream_pushf(infostream,"   width, height: {%u, %u}\n"
                        "   Bit depth: %u\n"
                        "   Colour type: %u\n"
                        "   Compresion method: %u\n"
                        "   Filter: %u\n"
                        "   Interlace: %u\n",
                        ihdr->width,
                        ihdr->height,
                        ihdr->bit_depth,
                        ihdr->colour_type,
                        ihdr->compresion_method,
                        ihdr->filter_method,
                        ihdr->interlace_method);
                 //Use colour_type 6 and Bit Depth 8 or 16 for transparent images
                 //Compresion method should always be 0 since it is the only one defined (lossless compresion)
                 //Also only filtering 0 is defined
                 if(ihdr->compresion_method != 0 || ihdr->filter_method != 0)
                 {
                     stream_pushf(infostream,"WARNING! Image is not compatible\n");
                 }
                 else
                 { 
                     Compatible = 1;
                     width = ihdr->width;
                     height = ihdr->height;
                     switch(ihdr->colour_type)
                     {
                         case 0: //Note(Agu):Greyscale
                             {
                                 BytesPerPixel = 1;
                             }break;
                         case 2: //Note(Agu):TrueColor (rgb)
                             {
                                 BytesPerPixel = 3;
                             }break;
                         case 3: //Note(Agu):Indexed colour no alpha  
                             {
                                 Assert(0);
                             }break;
                         case 4: //Note(Agu):Greyscale with alpha
                             {
                                 BytesPerPixel = 2;
                             }break;
                         case 6: //Note(Agu):TrueColor with alpha
                             {
                                 BytesPerPixel = 4;
                             }break;
                         default:
                             {
                                 Assert(0);
                             }break;
                     }
                     Assert(BytesPerPixel == 4);

                 }

             }break;
         case 'tEXt':
             {
                 uint8 *datatext = (uint8 *)chunkdata;
                 uint8 *datacomment = datatext;
                 
                 while(datacomment[0])
                 {
                     stream_pushf(infostream,"%c", datacomment[0]);
                     ++datacomment;
                 }
                 stream_pushf(infostream, "\n");
             }break;
         case 'IDAT': 
             {
                 stream_PushChunk(&CompressedData, chunkheader->length, chunkdata);
             }break;
         case 'IEND': 
             {
                 stream_pushf(infostream,"End\n");
             }break;
     }

  }

   if(Compatible)
   {
       stream_pushf(infostream,"Starting to decompress...\n");
       png_idat_header *idatheader = stream_consume_data(&CompressedData, png_idat_header); 

       //Png uses compression method 8
       //Read the needed information about the zlib flags and it's additionals.
       uint8 CM      = (idatheader->ZLibFlags & 0xf);
       uint8 CInfo   = (idatheader->ZLibFlags >> 4);
       //FLG byte covers these 3 values 
       uint8 FCHECK  = (idatheader->AdditionalFlags & 0x1f);
       uint8 FDICT   = ((idatheader->AdditionalFlags >> 5) & 0x1);
       //Compression level, not necessary for decompression;
       uint8 FLEVEL  = (idatheader->AdditionalFlags >> 6);
       stream_pushf(infostream,"   CM %u\n"
              "   CInfo %u\n"
              "   FCHECK %u\n"
              "   FDICT %u\n"
              "   FLEVEL %u\n",
              CM, CInfo, FCHECK, FDICT, FLEVEL);

       if(CM == 8 && FDICT == 0)
       {
          uint32 ImageSize = (height * (width * BytesPerPixel + 1));
          FinalPixels = AllocatePixels(area , ImageSize);
          uint8 *DecompressedPixels = AllocatePixels(area, ImageSize); 
          uint8 *PixelsDest = DecompressedPixels; 
          uint8 *DecompressedPixelsEnd = DecompressedPixels + ImageSize; 
          
          uint32 BFINAL = 0;
          do
          {
             BFINAL       = stream_ConsumeBits(&CompressedData, 1);
             uint32 BTYPE = stream_ConsumeBits(&CompressedData, 2);
             stream_pushf(infostream,"BTYPE: %u\n",BTYPE);
             
             if(BTYPE == 3)
             {
                 stream_pushf(infostream,"Error while decompressing the file! BTYPE is 3!\n");
                 Assert(0);
             }
             //No compression, so copy everything.
             if(BTYPE == 0)
             {
                 //NOTE(Agu) skip remaining bits and read LEN and NLEN values.
                 stream_FlushByte(&CompressedData);

                 //uint16 LEN  = *(uint16 *)stream_consume_size(&CompressedData, sizeof(uint16));
                 //uint16 NLEN = *(uint16 *)stream_consume_size(&CompressedData, sizeof(uint16));
                 uint16 LEN  = (uint16)stream_ConsumeBits(&CompressedData, 16);
                 uint16 NLEN = (uint16)stream_ConsumeBits(&CompressedData, 16);
                 if((uint16)LEN != (uint16)~NLEN)
                 {
                     Assert(0);
                 }

                 while(LEN)
                 {
                     stream_RefillIfEmpty(&CompressedData);
                     uint16 TOTALLEN = LEN;
                     if(TOTALLEN > CompressedData.buffer.size)
                     {
                         //Read whatever is left of the chunk
                         TOTALLEN = (uint16)CompressedData.buffer.size;
                     }
                     uint8 *data = (uint8 *)stream_consume_size(&CompressedData, TOTALLEN);
                     LEN -= TOTALLEN;
                     Assert(data);
                     while(TOTALLEN--)
                     {
                        *PixelsDest++ = *data++;
                     }
                 }

             }
             else
             {
                 //Literals and lengths are a separated tree
                png_huffman LitLenHuffman = AllocHuffman(area, 15);
                png_huffman DistHuffman   = AllocHuffman(area, 15);
                uint32 HLIT = 0;
                uint32 HDIST = 0;
                uint32 LenDistTable[512] = {0};
                //Case when compressed with dynamic huffman code
                if(BTYPE == 1) //Note(Agu) Fixed huffman code
                {
                    HLIT = 288;
                    HDIST = 32;
                    uint32 FixedAlphabet[][2] =
                    {
                        {143 ,8},
                        {255 ,9},
                        {279 ,7},
                        {287 ,8},
                        {319 ,5}
                    };

                    uint32 bindex = 0; 
                    for(int i = 0; i < ArrayCount(FixedAlphabet); i++)
                    {
                        uint32 bcount = FixedAlphabet[i][1];
                        uint32 lastbit = FixedAlphabet[i][0];
                        while(bindex <= lastbit)
                        {
                            LenDistTable[bindex++] = bcount;
                        }
                    }
                }
                else if(BTYPE == 2)
                {
                    //These come after the first consumed data from the header and before the actual data.
                    //# of literal/length codes, distance codes, length of codes. 
                    HLIT  = stream_ConsumeBits(&CompressedData, 5) + 257; //(257 - 286)
                    HDIST = stream_ConsumeBits(&CompressedData, 5) + 1; //(1 - 32)
                    uint32 HCLEN = stream_ConsumeBits(&CompressedData, 4) + 4; //(4 - 19)
                    //Length read order
                    uint32 HCLENOrder[19] = {16, 17, 18,
                     0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

                    uint32 HCLENTable[19] = {0};

                    //read the next lengths in order 
                    for(uint32 i = 0; 
                               i < HCLEN;
                               i++)
                    {
                      HCLENTable[HCLENOrder[i]] = stream_ConsumeBits(&CompressedData, 3); 
                    }

                    //Data of this block
                    png_huffman DictHuffman = AllocHuffman(area, 8);
                    ComputeHuffman(ArrayCount(HCLENOrder), HCLENTable, &DictHuffman);


                    //Goes from HLIT to HDIST
                    uint32 LengthRepeat = HDIST + HLIT;
                    uint32 LengthCount = 0;
                    while(LengthCount < LengthRepeat)
                    {
                        uint32 CodeLen = DecodeHuffman(&DictHuffman, &CompressedData);
                        uint32 Repeat = 1;
                        uint32 Len = 0;
                        if(CodeLen <= 15)
                        {
                            Len = CodeLen;
                        }
                        if(CodeLen == 16)
                        {
                            //Repeat previous value.   
                            //The repeat count must be 3..6. the next 2 bits is a number between those
                            Repeat = 3 + stream_ConsumeBits(&CompressedData, 2);
                            Len = LenDistTable[LengthCount - 1];
                        }
                        if(CodeLen == 17)
                        {
                            //Note(Agu): Is this really 3?
                            Repeat = 3 + stream_ConsumeBits(&CompressedData, 3);
                            Len = 0; 
                        }
                        if(CodeLen == 18)
                        {
                            Repeat = 11 + stream_ConsumeBits(&CompressedData, 7);
                            Len = 0; 
                        }

                        while(Repeat--)
                        {
                            LenDistTable[LengthCount++] = Len;
                        }
                    }
                    Assert(LengthCount == LengthRepeat);
                }
                    //Make the tables
                    ComputeHuffman(HLIT, LenDistTable,&LitLenHuffman);
                    ComputeHuffman(HDIST, LenDistTable + HLIT, &DistHuffman);

#if 1
                 while(1)
                 {
                   uint32 DataLength = DecodeHuffman(&LitLenHuffman, &CompressedData);
                   if(DataLength < 256)
                   {
                     //Copy to stream as byte
                       uint32 value = (DataLength & 0xff);
                       *PixelsDest++ = (uint8)value;
                   }
                   else
                   {
                        if(DataLength == 256)
                        {
                            break;
                        }
                        else
                        {
                            uint32 LenIndex = DataLength - 257;
                            png_huffman_entry LenEntry = LengthTable[LenIndex];

                            uint32 Len = LenEntry.Symbol;
                            if(LenEntry.BitsUsed)
                            {
                                uint32 ExtraValue = stream_ConsumeBits(&CompressedData, LenEntry.BitsUsed);
                                //ExtraValue = png_reverse_bits(ExtraValue, LenEntry.BitsUsed);
                                Len += ExtraValue; 
                            }
                            //0-29
                            uint32 DistanceIndex = DecodeHuffman(&DistHuffman, &CompressedData) ;
                            Assert(DistanceIndex <= 29);
                            png_huffman_entry DistEntry = DistTable2[DistanceIndex];

                            uint32 Distance = DistEntry.Symbol;
                            if(DistEntry.BitsUsed)
                            {
                                uint32 ExtraValue = stream_ConsumeBits(&CompressedData, DistEntry.BitsUsed);
                                //ExtraValue = png_reverse_bits(ExtraValue, DistEntry.BitsUsed);
                                Distance += ExtraValue;
                            }

                            uint8 *source = PixelsDest - Distance;
                            Assert((source + Len) <= DecompressedPixelsEnd);
                            Assert((PixelsDest + Len) <= DecompressedPixelsEnd);
                            Assert(source >= DecompressedPixels);
                            while(Len--)
                            {
                               *PixelsDest++ = *source++;
                            }
                            //move back the distance and copy until current point
                        
                        }
                   }
                 }
#endif
             }
          }while(!BFINAL);

          //TODO: Support less than 4 bytes per pixel
          ReconstructPixels(width, height, BytesPerPixel, DecompressedPixels, FinalPixels);
           
       }

   }
   image.width = width;
   image.height = height;
   image.pixels = FinalPixels;
   image.bpp = BytesPerPixel;
   return(image);
}




uint32 crc_table[256] = 
{0x0, 0x77073096, 0xee0e612c, 0x990951ba, 0x76dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0xedb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x9b64c2b,
0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856,
0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1,
0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59, 0x26d930ac,
0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87,
0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x1db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x6b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2,
0xf00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x86d3d2d, 0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed,
0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65, 0x4db26158,
0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73,
0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e,
0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x3b6e20c, 0x74b1d29a, 0xead54739,
0x9dd277af, 0x4db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0xd6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0xa00ae27, 0x7d079eb1, 0xf00f9344,
0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f,
0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda,
0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795,
0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d, 0x9b64c2b0,
0xec63f226, 0x756aa39c, 0x26d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x5005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0xcb61b38, 0x92d28e9b,
0xe5d5be0d, 0x7cdcefb7, 0xbdbdf21, 0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6,
0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661,
0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9, 0xbdbdf21c,
0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37,
0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};

uint32
crcfromchunk(stream_chunk *startchunk)
{
    uint32 crc = 0xffffffffL;

    stream_chunk *chunk = startchunk;
    //Note(Agu): Sum to ignore length value from header
    uint32 InitialOffset = sizeof(uint32);
    while(chunk)
    {
        uint8 *buffer = chunk->contents + InitialOffset;  
        uint32 len = chunk->size - InitialOffset;

        InitialOffset = 0;
        for(uint32 n = 0; n < len; n++)
        {
            crc = crc_table[(crc ^ buffer[n]) & 0xff] ^ (crc >> 8);
        }
        chunk = chunk->next;
    }
    return(crc ^ 0xffffffffL);
}
static void
CalculateAdler(adler32 *adler, stream_chunk *startchunk)
{   
    stream_chunk *chunk = startchunk->next;
    uint32 s1 = adler->s1;
    uint32 s2 = adler->s2;
    while(chunk)
    {
        uint8 *buffer = chunk->contents;
        uint32 len = chunk->size;
        for(uint32 n = 0; n < len; n++)
        {
            s1 = (s1 + buffer[n]) % 65521;
            s2 = (s2 + s1) % 65521;
        }
        chunk = chunk->next;
    }
    adler->s1 = s1;
    adler->s2 = s2;
}
static adler32
BeginAdler()
{
    adler32 result = {0};
    result.s1 = 1;
    result.s2 = 0;
    return(result);
}
static uint32
EndAdler(adler32 *adler)
{
    uint32 result = (adler->s2 << 16) + adler->s1;
    png_endian_swap_u32(&result);
    return(result);
}

static void
png_write_to_stream(image_data *image,
             stream_data *stream,
             stream_data *info_stream)
{
  stream_chunk *crcchunk = 0;
  //Note(Agu): First data, then endian swap, then crc;


  png_chunk_header chunkheader = {0};
  png_chunk_footer chunkfooter = {0};

  png_header signature = {137, 80, 78, 71, 13, 10, 26, 10};
  stream_PushData(stream, &signature, sizeof(png_header));

  png_ihdr ihdr = {0};
  /*
	 type              color_type
	 greyscale         0
	 truecolor         2
	 indexed-color     3
	 greyscale w/alpha 4
	 truecolor w/alpha 6
	 this should also have the avadible depths
	 */

  //default bit depth will be 8 for now
  ihdr.width            = image->width;
  ihdr.height           = image->height;
  ihdr.bit_depth         = 8;
  ihdr.colour_type       = 6;
  ihdr.compresion_method = 0; //always 0
  ihdr.filter_method     = 0; //always 0
  ihdr.interlace_method  = 0; //0 no interlace 1 adam interlace
  png_endian_swap_u32(&ihdr.width);
  png_endian_swap_u32(&ihdr.height);

  chunkheader.type32 = 'IHDR';
  chunkheader.length = sizeof(png_ihdr);
  png_endian_swap_u32(&chunkheader.type32);
  png_endian_swap_u32(&chunkheader.length);
  //ihdr
  stream_PushData(stream, &chunkheader, sizeof(png_chunk_header));
  crcchunk = stream->next;
  stream_PushData(stream, &ihdr, sizeof(png_ihdr));

  chunkfooter.crc = crcfromchunk(crcchunk);
  png_endian_swap_u32(&chunkfooter.crc);

  //footer after header
  stream_PushData(stream, &chunkfooter, sizeof(png_chunk_footer));

  png_idat_header idatheader = {0};
  //0x38 * 256 = 14336
  //14336 + 0x11 = 14353
  //14353 / 31 = 463 <= is multiple of 31!
  idatheader.ZLibFlags = 0x38;
  idatheader.AdditionalFlags = 0x11;


  //NOTE: Normally one would store multiple IDAT chunks, but not a non-compressed png 
  //depending on the size I will use one BIG IDAT chunk.
  uint8 BTYPEANDFINAL = 0x0000;
  uint8 *at = (uint8 *)image->pixels;
  uint32 IdatLimit   = U16MAX;//16384; 
  uint32 ImageSize   = image->height * (image->width * image->bpp + 1);
  uint32 IdatValues  = sizeof(BTYPEANDFINAL) + sizeof(uint16) + sizeof(uint16);

  chunkheader.type32 = 'IDAT';
  png_endian_swap_u32(&chunkheader.type32);
  png_chunk_header *png_idat_chunk_header = stream_PushData(stream, &chunkheader, sizeof(png_chunk_header));
  crcchunk = stream->next; //to start reading the data after this one
  stream_PushData(stream, &idatheader, sizeof(png_idat_header));

  uint32 ImageSizeUsing = ImageSize;
  uint32 stride = image->width * 4;
  uint32 totalstride = stride + 1;
  adler32 adler = BeginAdler();

  u32 total_chunks_pushed = 0;
  while(!BTYPEANDFINAL)
  {
    total_chunks_pushed++;
    if(ImageSizeUsing <= IdatLimit)
    {
       BTYPEANDFINAL |= 0x1;
    }
    //Len is how much pixels can store in one header.
    uint16 LEN  = (uint16)(ImageSizeUsing > IdatLimit ? IdatLimit : ImageSizeUsing); 

    uint32 RemainingLen = LEN - ((LEN / totalstride) * totalstride);
    Assert(RemainingLen < LEN);
    LEN -= RemainingLen;
    uint16 NLEN = (uint16)~LEN;

    Assert(LEN > 0);

    stream_PushData(stream, &BTYPEANDFINAL,sizeof(BTYPEANDFINAL));
    stream_PushData(stream, &LEN,          sizeof(LEN));
    stream_PushData(stream, &NLEN,         sizeof(NLEN));

    uint16 UsedLen = LEN;
    uint8 ZeroFilter = 0;
    while(LEN >= totalstride)
    {
        stream_chunk *adlerchunk = stream->next; //start before pushing the data
        stream_PushData(stream, &ZeroFilter, sizeof(ZeroFilter));
        stream_PushData(stream, at, stride);
        CalculateAdler(&adler, adlerchunk);
        at += stride;
        LEN -= totalstride;
    }
	stream_pushf(info_stream, "End LEN %u\n", LEN);
    Assert(LEN == 0);

    ImageSizeUsing -= UsedLen; 
	stream_pushf(info_stream, "remaining using image %u\n", ImageSizeUsing);
  }
  //calculate the header's length after processing the chunk data
  png_idat_chunk_header->length = sizeof(idatheader) +
	                   (IdatValues * total_chunks_pushed) +
					   ImageSize +
					   sizeof(uint32);
  png_endian_swap_u32(&png_idat_chunk_header->length);



  uint32 Adler32 = EndAdler(&adler); 
  stream_PushData(stream, &Adler32, sizeof(Adler32));

  chunkfooter.crc = crcfromchunk(crcchunk);
  png_endian_swap_u32(&chunkfooter.crc);
  stream_PushData(stream, &chunkfooter , sizeof(png_chunk_footer));


  chunkheader.type32 = 'IEND'; 
  chunkheader.length = 0;
  png_endian_swap_u32(&chunkheader.length);
  png_endian_swap_u32(&chunkheader.type32);
  stream_PushData(stream, &chunkheader, sizeof(png_chunk_header));
  crcchunk = stream->next;

  chunkfooter.crc = crcfromchunk(crcchunk);
  png_endian_swap_u32(&chunkfooter.crc);
  stream_PushData(stream, &chunkfooter, sizeof(png_chunk_footer));
}

inline void
png_write_to_handle(
		platform_api *platform)
{
}
