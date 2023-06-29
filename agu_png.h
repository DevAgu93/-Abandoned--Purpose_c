#define ArrayCount(a) (sizeof(a) / sizeof(a[0]))

#pragma pack(push,1)
//These are the first decimal values ALWAYS:
//137 80 78 71 13 10 26 10
typedef struct{
    uint8 signature[8];
}png_header;

typedef struct{
    uint32 length;
    union
    {
        uint8  type[4];
        uint32 type32;
    };
}png_chunk_header;

typedef struct{
    u32 crc;
}png_chunk_footer;

typedef struct
{
    u32 width;
    u32 height;

    u8 bit_depth; 
    u8 colour_type;
    u8 compresion_method;
    u8 filter_method;
    u8 interlace_method;

}png_ihdr;

//NOTE: the PLTE header always appears on headers with colour type 3 and NOT in 0 and 4, the rest is optional...
typedef struct{
	u8 r;
	u8 g;
	u8 b;
}png_plte;

typedef struct
{
    uint8 ZLibFlags;
    uint8 AdditionalFlags;
}png_idat_header;
typedef struct
{
   uint32 check;
}png_idat_footer;

typedef struct 
{
    uint32 s1;
    uint32 s2;
}adler32;

#pragma pack(pop)

typedef struct
{
    uint16 Symbol;
    uint16 BitsUsed;
}png_huffman_entry;

typedef struct
{
    uint32 BitLength;
    uint32 EntryCount;
    png_huffman_entry *Entries;
}png_huffman;
