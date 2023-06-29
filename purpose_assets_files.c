#define Assert(Condition) \
    if(!(Condition)) {*(int*)0 = 0;}
#define ARRAYCOUNT(a) (sizeof(a) / sizeof(a[0]))

#define _CRT_SECURE_NO_WARNINGS

//#include "purpose_crt.h"
#include "global_definitions.h"
#include "agu_random.h"
#include "purpose_math.h"
#include "global_all_use.h"
#include "purpose_memory.h"
#include "purpose_stream.h"
#include "purpose_platform.h"
#include "purpose_console.h"
#include "purpose_global.h"
#include "purpose_render.h"
#include "purpose_render.c"
#include "purpose_ui.h"
#include "purpose_ui.c"


#include "purpose_platform_layer.h"
#include "purpose_all.h"
#include "purpose_files.c"
#include "brains.h"
#include "purpose_assets.h"
#include "purpose.h"
#include "purpose_assets.c"
#include "model_render.c"
#include "purpose_world.h"
#include "purpose_hash.c"



#include "purpose_ttf.c"
#include <stdio.h>

#define WINDOWSASSETS 1

#if WINDOWSASSETS 

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "platform_win32.c"

#define ASCII_FIRST_CHAR (' ')
#define ASCII_LAST_CHAR ('~')

typedef struct{
	uint8 *path;
	uint8 *name;
}font_identifier;

typedef struct{
   void *dcHandle;
   void *bmpHandle;
   void *fontHandle;
   void *bits;

   uint32 loadedBitmapW;
   uint32 loadedBitmapH;
   u32 glyph_height;
}win32_font_load_op;

typedef struct
{
   ppse_font header;

   uint32 first_glyph;
   uint32 last_glyph;
   uint32 glyph_count;
   uint32 font_glyph_height;

   uint32 glyph_displacements_size;
   i32 *glyph_displacements;


   uint32 glyph_table_size;
   ppse_glyph *glyphs;
   

   uint8 *fontImagePixels;
   uint32 fontImageSize;

   uint32 totalSizeUsed;

}font_load_info;


stream_data global_ProgramStream;

//Note(Agu): The horizontal displacement is the width of the glyph, a.k.a ImageW
//The deltas are the tables of kerning pairs
static void
win32_end_font_op(win32_font_load_op *font_load_process)
{
	DeleteObject(font_load_process->fontHandle);
	DeleteObject(font_load_process->bmpHandle);
	DeleteDC(font_load_process->dcHandle);

}

static win32_font_load_op 
win32_start_font_op(memory_area *area,
                    uint32 fontHeight,
                    uint8 *fontPath,
                    uint8 *fontName,
					stream_data *info_stream)
{
    HDC hdc = 0; 
	win32_font_load_op windows_font_op = {0};
    if(!windows_font_op.bits)
    {
         //Note(Agu): The default brush background dc color is white!

         hdc = CreateCompatibleDC(GetDC(0));
         uint32 fontsAdded = AddFontResourceEx(fontPath, FR_PRIVATE, 0);
         Assert(fontsAdded);
		 //Close with DeleteObject(font); 
         HFONT fontHandle = CreateFont(-(int32)fontHeight,
                                  0,
                                  0,
                                  0,
                                  FW_NORMAL,
                                  FALSE, //Italic
                                  FALSE, //Underline
                                  FALSE, //Strikeout
                                  DEFAULT_CHARSET,
                                  OUT_CHARACTER_PRECIS,
                                  CLIP_DEFAULT_PRECIS,
                                  ANTIALIASED_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE,
                                  fontName 
                                  );
         SelectObject(hdc, fontHandle);
#if 1
		 //For detecting successfull font name.
         uint8 selectedFontName[30];
         GetTextFace(hdc, sizeof(selectedFontName), selectedFontName);
		 stream_pushf(info_stream,"Font %s was choosen, found %s.========\n",fontName ,selectedFontName);
#endif
         TEXTMETRICW textMetrics = {0};
         GetTextMetricsW(hdc, &textMetrics);
         uint32 bitmapW = 2 * ((uint32)textMetrics.tmMaxCharWidth + (uint32)textMetrics.tmOverhang);
         uint32 bitmapH = 2 * ((uint32)textMetrics.tmHeight + (uint32)textMetrics.tmOverhang);

         BITMAPINFO bitmapInfo = {0};
         bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
         bitmapInfo.bmiHeader.biWidth  = bitmapW;
         bitmapInfo.bmiHeader.biHeight = bitmapH;
         bitmapInfo.bmiHeader.biPlanes = 1;
         bitmapInfo.bmiHeader.biBitCount = 32;
         bitmapInfo.bmiHeader.biCompression = BI_RGB;
         bitmapInfo.bmiHeader.biSizeImage = 0;
         bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
         bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
         bitmapInfo.bmiHeader.biClrUsed = 0;
         bitmapInfo.bmiHeader.biClrImportant = 0;

         uint8 *bitmapBits = 0;
         HBITMAP hBitmap = CreateDIBSection(hdc, &bitmapInfo, DIB_RGB_COLORS, &bitmapBits,0, 0);
         SelectObject(hdc, hBitmap);
         SelectObject(hdc, fontHandle);
         //SetDCBrushColor(hdc, RGB(0, 0, 0));
         SetBkColor(hdc, RGB(0, 0, 0));
         SetTextColor(hdc, RGB(255, 255, 255));


         windows_font_op.dcHandle      = hdc;
         windows_font_op.bmpHandle     = hBitmap;
		 windows_font_op.fontHandle    = fontHandle;
         windows_font_op.bits          = bitmapBits;
         windows_font_op.loadedBitmapW = bitmapW;
         windows_font_op.loadedBitmapH = bitmapH;
         windows_font_op.glyph_height  = textMetrics.tmHeight;
    }
	return(windows_font_op);
}

static u32
win32_push_kerning_pairs(
		memory_area *area,
		font_load_info *font_load_process,
		win32_font_load_op windows_font_op)
{

    HDC hdc         = windows_font_op.dcHandle; 

	if(hdc && font_load_process && font_load_process->glyph_displacements);
	{
        uint32 kerningPairCount   = GetKerningPairs(hdc, 0, 0);

		//temporally allocate memory for the kerning pairs recieved by windows.
		temporary_area temporary_kerning_pairs_area = temporary_area_begin(area);

        KERNINGPAIR *kerningTable = memory_area_push_size(area, kerningPairCount * sizeof(KERNINGPAIR));
        GetKerningPairs(hdc, kerningPairCount, kerningTable);

        uint32 startCode = (uint32)font_load_process->first_glyph;
	    //Fill the horizontal advancements table
        for(uint32 k = 0; k < kerningPairCount; k++)
        {
            KERNINGPAIR *kerningPair = &kerningTable[k];
            uint32 firstCode  = kerningPair->wFirst;
            uint32 secondCode = kerningPair->wSecond;
            uint32 inRange = (firstCode >= startCode && secondCode >= startCode) &&
                             (firstCode < font_load_process->glyph_count && secondCode < font_load_process->glyph_count);
            //If both glyph codes are in range
            if(inRange)
            {
                uint32 fc = firstCode - startCode;
                uint32 sc = secondCode - startCode;
                Assert(fc < font_load_process->glyph_count);
                Assert(sc < font_load_process->glyph_count);
                int32 pairIndex = (fc * font_load_process->glyph_count) + sc; 
                font_load_process->glyph_displacements[pairIndex] = kerningPair->iKernAmount;
            }
        }

		//free used memory
		temporary_area_end(&temporary_kerning_pairs_area);
	}

	u32 success = 0;
	return(success);
}

static image_data
win32_push_glyph_bitmap(memory_area *area,
                     uint32 fontHeight,
                     uint32 codePoint,
                     win32_font_load_op *font_load_process)
{
    HDC hdc         = font_load_process->dcHandle; 
    HBITMAP hBitmap = font_load_process->bmpHandle;
    VOID *bits      = font_load_process->bits;
    int32 bitmapW   = font_load_process->loadedBitmapW;
    int32 bitmapH   = font_load_process->loadedBitmapH; 
    Assert(font_load_process->dcHandle);
         wchar_t cheesePoint = (wchar_t)codePoint;

         //for the range of the table.
       //  ABC *abcTable = memory_area_push_array(area, ABC, font_load_process->glyph_count);
        // GetCharABCWidthsW(font_load_process->dcHandle, font_load_process->first_glyph, font_load_process->last_glyph, abcTable);
         //Gives back the horizontal spacing from start to end of the character in the table.

         SIZE size = {0};
		 //Computes width and height of text. only one letter in this case.
         GetTextExtentPoint32W(hdc, &cheesePoint, 1, &size);

         int32 textW = size.cx;
         int32 textH = size.cy;

         memset(bits, 0, bitmapW * bitmapH * 4);
         //PatBlt(hdc, 0, 0, textW, textH, BLACKNESS);
		 //Writes text to the bitmap
         TextOutW(hdc, 0, 0, &cheesePoint, 1);
       //  GetObject(hBitmap);

         int32 textBoundsX = 10000;
         int32 textBoundsW = -10000;
         uint32 *dibBitsStart = (uint32 *)bits + (bitmapW * (bitmapH - 1));
         uint32 *bitsRow = dibBitsStart;
#if 1
		 //Skip space since it doesn't have pixels.
         if(codePoint != ' ')
         {
            for(int32 y = 0; y < bitmapH; y++)
            {
                uint32 *pixel = bitsRow;
                for(int32 x = 0; x < bitmapW; x++)
                {
                    if(*pixel != 0)
                    {
                        if(textBoundsX > x)
                        {
                            textBoundsX = x;
                        }
                        if(textBoundsW < x)
                        {
                            textBoundsW = x;
                        }

                    }
                    pixel++;
                }
                bitsRow -= bitmapW;
            }
         }
         else
         {
            textBoundsX = 1;
            textBoundsW = size.cx - 1;
         }
         textBoundsX--;
         textBoundsW += 2;
         int32 textBoundsY = 0;
         int32 textBoundsH = size.cy;
#endif
#if 0
         textBoundsX = 0;
         textBoundsW = size.cx;
         textBoundsY = 0;
         textBoundsH = size.cy;
#endif
         //font_load_process->glyphs[codePoint - startCode].baseLineY = 0;//textBoundsY;

         int32 imageW = (textBoundsW - textBoundsX);
         if(size.cx > imageW)
         {
             imageW = size.cx;
             textBoundsX = 0;
             textBoundsW = size.cx;
         }
         int32 imageH = (textBoundsH - textBoundsY);

         uint32 imageStride = imageW * 4;
         uint32 imageSize = imageStride * imageH;
         image_data image = {0};
         image.pixels = memory_area_push_size(area, imageSize);
         image.width  = imageW;
         image.height = imageH;
         image.bpp    = 4;

         uint8 *toRow = image.pixels;
         bitsRow = dibBitsStart - (bitmapW * textBoundsY); 
         for(int32 y = textBoundsY; y < textBoundsH; y++)
         {
             uint32 *to = (uint32 *)toRow;
             uint32 *pixel = bitsRow + textBoundsX;
             for(int32 x = textBoundsX; x < textBoundsW; x++)
             {
                 uint32 alpha = (uint32)((*pixel) & 0xff);
                 pixel++;
               //  uint32 w = to - (uint32 *)toRow;
                 *to++ = ((alpha << 24) | 0x00ffffff);
             }
             toRow += imageStride;
             bitsRow -= bitmapW;

         }
        // printf("horizontalSpacing for %c: %d\n", codePoint, imageW);
         return(image);

}




#endif //Windows stuff

//
//CRT I/O functions. 
//
#if 0
PLATFORM_READ_FROM_FILE(CRTReadFromFile)
{

    FILE *filePtr = file_handle.handle;
    fseek(filePtr, 0, SEEK_SET);
    fseek(filePtr, (uint32)offset, SEEK_CUR);

    fread(destination, size, 1,filePtr);

    fseek(filePtr, 0, SEEK_SET);
    
    return(0);
}
#endif



static void
crt_read_info_stream(stream_data *stream)
{
    stream_chunk *chunk = stream->first; 
    while(chunk)
    {
       uint8 *contents = (uint8 *)chunk->contents; 
       printf(contents);
	   printf("\n");
       chunk = chunk->next;
    }
}
static void
CRTWriteStreamToFile(stream_data *stream, uint8 *path)
{
 
  FILE *file = fopen(path, "wb");
  if(!file)
  {
      Assert(0);
	  //log in other cases and return
      return;
  }
    stream_chunk *chunk = stream->first; 
    if(chunk)
    {
       while(chunk)
       {
          uint8 *contents = (uint8 *)chunk->contents; 
          fwrite(contents, chunk->size, 1, file);
          chunk = chunk->next;
       }
    }
    fflush(file);
    fclose(file);
}

static void
CRTWriteProgramFromGlobalStream(uint8 *path)
{

  FILE *file = fopen(path, "wb");
  if(!file)
  {
      Assert(0);
      return;
  }
    stream_chunk *chunk = global_ProgramStream.first; 
    if(chunk)
    {
       while(chunk)
       {
          uint8 *contents = (uint8 *)chunk->contents; 
          fwrite(contents, chunk->size - 1, 1, file);
          chunk = chunk->next;
       }
    }
    fflush(file);
    fclose(file);
}

inline font_load_info
font_load_info_allocate_and_initialize(
		memory_area *area,
		u32 first_glyph,
		u32 last_glyph,
		u32 font_height)
{
    font_load_info font_load_process = {0};

	if(first_glyph <= last_glyph)
	{

        uint32 total_file_size_used = 0;

        ppse_font *font_header  = &font_load_process.header;

	    font_header->signature   = ppse_font_SIGNATURE;
	    font_header->font_height = font_height;
	    //TODO: swap them in case of confusion
        font_load_process.first_glyph = first_glyph;
        font_load_process.last_glyph  = last_glyph;
        font_load_process.glyph_count  = font_load_process.last_glyph - font_load_process.first_glyph + 1;

        uint32 glyph_count       = font_load_process.glyph_count;
        font_header->glyph_count = glyph_count;

	    //type of glyphs for the asset file
        uint32 glyph_table_size        = sizeof(ppse_glyph) * glyph_count;
        uint32 displacement_table_size = sizeof(uint32) * (glyph_count * glyph_count);
	    font_header->offset_to_pixels  = sizeof(ppse_font) + glyph_table_size + displacement_table_size;

	    font_load_process.glyph_table_size = glyph_table_size;
        font_load_process.glyphs         = memory_area_push_array(area , ppse_glyph, glyph_count);

	    font_load_process.glyph_displacements_size = displacement_table_size;
        font_load_process.glyph_displacements = memory_area_push_array(area , uint32, (glyph_count * glyph_count)); 

	}
	return(font_load_process);
}

static font_load_info 
//GenerateFontAsset
font_allocate_file_info(memory_area *area,
                  u8 *font_path_and_name,
			      u32 font_image_width,
				  u32 font_height,
				  u32 first_glyph,
				  u32 last_glyph,
				  platform_api *platform,
                  stream_data *info_stream)
           
{
	u32 padding_x = 25;
	u32 padding_y = 25;

    font_load_info font_load_process = {0};
    uint32 total_file_size_used = 0;

    ppse_font *font_header  = &font_load_process.header;

	font_header->signature   = ppse_font_SIGNATURE;
	font_header->font_height = font_height;
	//TODO: swap them in case of confusion
    font_load_process.first_glyph = first_glyph;
    font_load_process.last_glyph  = last_glyph;
    font_load_process.glyph_count  = font_load_process.last_glyph - font_load_process.first_glyph + 1;
	stream_pushf(info_stream, "\nGenerating font from '%c'-'%c' with a total amount of %u characters\n", font_load_process.first_glyph, font_load_process.last_glyph, font_load_process.glyph_count);

    //and not use a sheet.
    uint32 glyph_count      = font_load_process.glyph_count;
    font_header->glyph_count = glyph_count;

	//type of glyphs for the asset file
    uint32 glyph_table_size        = sizeof(ppse_glyph) * glyph_count;
    uint32 displacement_table_size = sizeof(uint32) * (glyph_count * glyph_count);
	font_header->offset_to_pixels  = sizeof(ppse_font) + glyph_table_size + displacement_table_size;

	font_load_process.glyph_table_size = glyph_table_size;
    font_load_process.glyphs         = memory_area_push_array(area , ppse_glyph, glyph_count);

	font_load_process.glyph_displacements_size = displacement_table_size;
    font_load_process.glyph_displacements = memory_area_push_array(area , uint32, (glyph_count * glyph_count)); 

	temporary_area temporary_font_work_area = temporary_area_begin(area);

	ttf_file_work font_file_work     = {0};
	image_data font_atlas_image_copy = {0};
	u32 parsing_success = ttf_fill_file_work_from_path(
			area,
			platform,
			&font_file_work,
			first_glyph,
			last_glyph,
			font_path_and_name,
			info_stream);

	if(parsing_success)
	{
		f32 glyph_scale = ttf_get_height_scale(&font_file_work, font_height);
	    //scale and copy kerning pairs
	    u32 k = 0;
		if(font_file_work.kerning_pairs)
		{
	        while(k < (glyph_count * glyph_count))
	        {
	        	font_load_process.glyph_displacements[k] = (i32)(glyph_scale * font_file_work.kerning_pairs[k++]);
	        }
		}
		//allocate array of images for the atlas
		image_data *glyph_atlas_images = memory_area_push_array(area, image_data, glyph_count);

		//offset on the resulting atlas
		u32 total_displacement_x = 0;
		u32 total_displacement_y = 0;
		for(u32 g = 0;
				g < glyph_count;
				g++)
		{
	        ttf_glyph_info *glyph_info = font_file_work.glyphs_info + g;

			// ;TD: check for error?
			glyph_atlas_images[g] = ttf_create_glyph_bitmap_for_atlas_anti_aliased(
					area,
		            font_height,
		            &font_file_work,
		            glyph_info,
					1,
		            info_stream);

			u32 glyph_w = glyph_atlas_images[g].width;
			u32 advance_x = (glyph_w + padding_x);
			u32 advance_y = (font_height + padding_y);
            font_load_process.glyph_displacements[g * glyph_count] += glyph_w;


			if(total_displacement_x + advance_x >= font_image_width)
			{
				total_displacement_x = 0;
				total_displacement_y += advance_y;
			}

			font_load_process.glyphs[g].textureOffsetX = total_displacement_x;
			font_load_process.glyphs[g].textureOffsetY = total_displacement_y;

			total_displacement_x += advance_x;
		}

		//allocate and make atlas
		font_atlas_image_copy = ttf_push_and_make_atlas_from_images(
				area,
				font_image_width,
				font_height,
				glyph_count,
				padding_x,
				padding_y,
				glyph_atlas_images,
				info_stream);
		
	}
	else
	{
		//free memory and log
	}


	//free memory
	temporary_area_end(&temporary_font_work_area);

	//copy the non-cleared atlas memory to the final atlas image
	image_data font_atlas_image = font_atlas_image_copy;
	font_atlas_image.pixels = memory_area_push_and_copy(
			area,
			font_atlas_image_copy.pixels,
			font_atlas_image.width * font_atlas_image.height * font_atlas_image.bpp);
//	image_convert_to_sdf(
//			area,
//			&font_atlas_image,
//			0xffffffff);


    font_header->pixelsW    = font_atlas_image.width; 
    font_header->pixelsH    = font_atlas_image.height;
	//I was about to allocate more space for the atlas image
    uint32 atlas_image_size = font_atlas_image.width * font_atlas_image.height * font_atlas_image.bpp;
    total_file_size_used = glyph_table_size + displacement_table_size + atlas_image_size;

	font_load_process.fontImagePixels = font_atlas_image.pixels;
	font_load_process.totalSizeUsed = total_file_size_used;
	font_load_process.fontImageSize = atlas_image_size;

	return(font_load_process);

}

static font_load_info 
//GenerateFontAsset
font_allocate_file_info_win32(memory_area *area,
                  u8 *fontPath,
                  u8 *fontName,
                  u32 fontBitmapHeight,
			      u32 fontImageW,
				  u32 first_glyph,
				  u32 last_glyph,
                  stream_data *info_stream)
           
{

    font_load_info font_load_process = {0};
    uint32 total_file_size_used   = 0;
    ppse_font *font_header   = &font_load_process.header;
    stream_data glyphStream = stream_Create(area);

	font_header->signature = ppse_font_SIGNATURE;
	//TODO: swap them in case of confusion
    font_load_process.first_glyph = first_glyph;
    font_load_process.last_glyph  = last_glyph;
    font_load_process.glyph_count  = font_load_process.last_glyph - font_load_process.first_glyph + 1;
	stream_pushf(info_stream, "\nGenerating font from '%c'-'%c' with a total amount of %u characters\n", font_load_process.first_glyph, font_load_process.last_glyph, font_load_process.glyph_count);

    //and not use a sheet.
    uint32 glyph_count      = font_load_process.glyph_count;
    font_header->glyph_count = glyph_count;

	//type of glyphs for the asset file
    uint32 glyph_table_size        = sizeof(ppse_glyph) * glyph_count;
    uint32 displacement_table_size = sizeof(uint32) * (glyph_count * glyph_count);
	font_header->offset_to_pixels = sizeof(ppse_font) + glyph_table_size + displacement_table_size;

	font_load_process.glyph_table_size = glyph_table_size;
    font_load_process.glyphs         = memory_area_push_array(area , ppse_glyph, glyph_count);

	font_load_process.glyph_displacements_size = displacement_table_size;
    font_load_process.glyph_displacements = memory_area_push_array(area , i32, (glyph_count * glyph_count)); 




	//fill the font info data
    win32_font_load_op windows_font_op = win32_start_font_op(
			area,
			fontBitmapHeight,
			fontPath,
			fontName,
			info_stream);

	win32_push_kerning_pairs(
			area,
			&font_load_process,
			windows_font_op);
    //all letters height
    font_header->font_height = windows_font_op.glyph_height;
    font_load_process.font_glyph_height = windows_font_op.glyph_height;
    for(uint32 character = font_load_process.first_glyph;
			   character <= font_load_process.last_glyph;
			   character++)
    {
       image_data charImage = win32_push_glyph_bitmap(area, fontBitmapHeight, character, &windows_font_op);
       stream_PushData(&glyphStream, &charImage, sizeof(image_data));
    }

	win32_end_font_op(&windows_font_op);





	stream_pushf(info_stream, "Generating font spritesheet...\n"); 
	//
	//generate font spritesheet.
	//Process one image per glyph
	u32 font_height = font_header->font_height;
	u32 pixels_row = (fontImageW * 4) * font_height;
	u32 pixels_h    = font_height;
    image_data result_image = {0};
	//Allocate image's pixels to fill on the stream and write to file.
    result_image.pixels = memory_area_clear_and_push(area, pixels_row);
    result_image.bpp    = 4;
    uint32 font_glyph_height = font_load_process.font_glyph_height;
    Assert(font_glyph_height > 0);
    Assert(glyph_count > 0);

    uint32 usedImageWidth = 0;
    uint32 rowCount = 0;
    for(uint32 glyph_index = 0; glyph_index < glyph_count; glyph_index++)
    {
		//Get every glyph bitmap stored.
        image_data *glyphImage = stream_consume_data(&glyphStream, image_data);

        uint32 glyphW = glyphImage->width;
        uint32 glyphH = glyphImage->height;
		//stream_pushf(info_stream, "Allocating glyph with dimensions %ux%u\n", glyphW, glyphH);
        if(usedImageWidth + glyphW >= fontImageW)
        {
            usedImageWidth = 0;
            rowCount++;

			stream_pushf(info_stream, "Pushing more pixels...\n");
			memory_area_clear_and_push(area, pixels_row);
			pixels_h += font_height;
        }
		//Fill glyph info
        ppse_glyph *ppseGlyph = font_load_process.glyphs + glyph_index;
		//Image locations of every glyph
        ppseGlyph->textureOffsetX = usedImageWidth;
        ppseGlyph->textureOffsetY = rowCount * font_glyph_height;

        uint32 rowOffset = rowCount * font_glyph_height;
		if(rowOffset > pixels_h)
		{
		}

        font_load_process.glyph_displacements[glyph_index * glyph_count] += glyphW;

        //Offset from the baseline. This was used to be added to the baseLineOffset below
        //uint32 baseLineOffset = 0;//font->glyphs[glyph_index].baseLineY;
        uint32 *imageRow = (uint32 *)result_image.pixels + fontImageW * rowOffset ;
        imageRow         += usedImageWidth;
        usedImageWidth   += glyphW;
        uint32 *glyphPixels = (uint32 *)glyphImage->pixels;
        for(uint32 y = 0; y < glyphH; y++)
        {
            uint32 *imagePixel = imageRow;
            for(uint32 x = 0; x < glyphW; x++)
            {
                *imagePixel = *glyphPixels;
                glyphPixels++;
                imagePixel++;
            }
            imageRow += fontImageW;

        }
    }
	stream_pushf(info_stream, "Finished font atlas with dimensions %ux%u",
			fontImageW,
			pixels_h);
    result_image.width  = fontImageW;
    result_image.height = pixels_h;

    font_header->pixelsW    = fontImageW; 
    font_header->pixelsH    = pixels_h;
	//I was about to allocate more space for the atlas image
    //TODO(Agu): Use multiple spritesheets or store the glyph images sequentially and then convert.
    uint32 atlas_image_size = fontImageW * pixels_h * 4;
    total_file_size_used        = glyph_table_size + displacement_table_size + atlas_image_size;

	font_load_process.fontImagePixels = result_image.pixels;
	font_load_process.totalSizeUsed = total_file_size_used;
	font_load_process.fontImageSize = atlas_image_size;
	stream_pushf(info_stream, "Finished font processing. Returning data.\n\n");
    return(font_load_process);
}

inline font_load_info
font_allocate_file_info_from_images(
		          memory_area *area,
				  platform_api *platform,
                  u8 *images_path,
                  u32 font_height,
			      u32 font_image_width,
				  u32 first_character_code,
				  u32 last_character_code,
                  stream_data *info_stream)
{
	font_load_info font_file_info = font_load_info_allocate_and_initialize(
			area,
			first_character_code,
			last_character_code,
			font_height);

	u32 glyph_count = font_file_info.glyph_count;

	//scan all the bmp and png files and count them
	//allocate all images on the output file
	//if the count exeeds the ascii range (93) just use the first 93 files
	//if the count is less than the ascii range, then fill the rest with the "null" character
	//the "null" character should be named "000.*"
	//is there isn't any null character, then use pure white as the null character.

	//store the images array
	image_data null_character = {0};
	image_data *glyph_images_array = memory_area_push_array(
			area,
			image_data,
			glyph_count);
	//combine the path with the search pattern
	u8 path_and_pattern[1024] = {0};
	format_text(path_and_pattern, sizeof(path_and_pattern), "%s/*.png", images_path);

	u32 maximum_character_width = 0;

	platform_file_search_info file_search_info = {0};

	//read the first character and start the scanning
	platform_file_search search_handle = platform->f_find_first_file(
			path_and_pattern, &file_search_info);

	u32 null_character_index = 0;
	u32 found_files_count = 0;

	if(search_handle.handle)
	{
		temporary_area temporary_images_area = temporary_area_begin(area);

		do
		{


		    u32 code_point = 0;

			u8 glyph_file_name_without_extension[128] = {0};
			u32 extension_dot_index = string_get_next_char_index_from(
					file_search_info.name,
					0,
					'.');
			memory_copy(
					file_search_info.name,
					glyph_file_name_without_extension,
					extension_dot_index);

		    u32 format_success = u32_from_string(glyph_file_name_without_extension, &code_point);

        	memory_clear(path_and_pattern, 1024);
	        string_concadenate(
	        		images_path,
	        		"/",
	        		path_and_pattern,
	        		1024);
	        string_add(
		    		file_search_info.name,
	        		path_and_pattern,
	        		1024);
		    
		    if(format_success && code_point == 0 && !null_character.pixels)
		    {
		    	platform_file_handle null_file_handle = platform->f_open_file(path_and_pattern, platform_file_op_read);

		    	if(null_file_handle.handle)
		    	{
		        	null_character = parse_png_or_bmp_from_handle(
		        			area,
		        			platform,
		        			null_file_handle,
		        			0);

					platform->f_close_file(null_file_handle);
		    	}
		    	else
		    	{
		    		stream_pushf(
		    					info_stream,
		    					"Could not open the null character bitmap %s !",
		    					file_search_info.name);
		    	}
		    }
		    else if(format_success)
		    {
                //ignore characters out of range
		    	if(code_point >= first_character_code && code_point <= last_character_code) 
		    	{
		    	    u32 character_index = code_point - first_character_code;
		    		platform_file_handle glyph_image_handle = platform->f_open_file(path_and_pattern, platform_file_op_read);
		    		if(glyph_image_handle.handle)
		    		{
		    	        found_files_count++;


						//store on the array
						glyph_images_array[character_index] = parse_png_or_bmp_from_handle(
								area, platform, glyph_image_handle, 0);
						u32 w = glyph_images_array[character_index].width;

						maximum_character_width = w > maximum_character_width ? w : maximum_character_width;

		    		    platform->f_close_file(glyph_image_handle);
		    		}
		    		else
		    		{
		    			stream_pushf(
		    					info_stream,
		    					"Could not open the character bitmap %s !",
		    					file_search_info.name);
		    		}
		    	}
		    }

		}while(platform->f_find_next_file(search_handle, &file_search_info));

		//if no null character for allocated
		if(!null_character.pixels)
		{
			u32 null_character_image_size = maximum_character_width * font_height * 4;
			null_character.width = maximum_character_width;
			null_character.height = font_height;
			null_character.pixels = memory_area_push_size(
					area,
					maximum_character_width * font_height * 4);
			memory_fill(255, null_character.pixels, null_character_image_size);
		}

		//look if there is any character missing
		//fill font data
		u32 used_width  = 0;
		u32 used_height = 0;
		
		for(u32 g = 0;
				g < glyph_count;
				g++)
		{
			image_data g_img = glyph_images_array[g];
			u32 glyph_index = first_character_code + g;
			//make this image point to the null character
			if(!g_img.pixels)
			{
				glyph_images_array[g] = null_character;
				//if no space got specified, then allocate an empty image
				if(g == ' ')
				{
					glyph_images_array[g].pixels = memory_area_clear_and_push(
							area,
							maximum_character_width + font_height * 4);
				}
			}

			u32 advance_x = glyph_images_array[g].width;
			u32 advance_y = font_height;
			if(used_width + advance_x > font_image_width)
			{
				used_width = 0;

				used_height += advance_y;
			}
			font_file_info.glyphs[g].textureOffsetX = used_width;
			font_file_info.glyphs[g].textureOffsetY = used_height;
			font_file_info.glyph_displacements[g * glyph_count] = glyph_images_array[g].width;


			used_width += advance_x;
		}

        image_data atlas_image_copy = ttf_push_and_make_atlas_from_images(
				area,
				font_image_width,
				font_height,
				glyph_count,
				0,
				0,
				glyph_images_array,
				info_stream);

		//create atlas and
		//fill the corresponding glyph data
		temporary_area_end(&temporary_images_area);

		u32 font_atlas_size = atlas_image_copy.width * atlas_image_copy.height * 4;

		image_data font_atlas_image = atlas_image_copy;
		font_atlas_image.pixels = memory_area_push_and_copy(
				area, atlas_image_copy.pixels, font_atlas_size); 


        font_file_info.header.pixelsW    = font_atlas_image.width; 
        font_file_info.header.pixelsH    = font_atlas_image.height;
    	//I was about to allocate more space for the atlas image
        u32 atlas_image_size     = font_atlas_image.width * font_atlas_image.height * font_atlas_image.bpp;
        u32 total_file_size_used = font_file_info.glyph_table_size + font_file_info.glyph_displacements_size + atlas_image_size;
    
    	font_file_info.fontImagePixels = font_atlas_image.pixels;
    	font_file_info.totalSizeUsed = total_file_size_used;
    	font_file_info.fontImageSize = atlas_image_size;
			
	}
	else
	{
		stream_pushf(info_stream, "No files were found on \"%s\" !", images_path);
	}

	return(font_file_info);

}

inline font_load_info
font_allocate_file_info_from_images_ascii(
		          memory_area *area,
				  platform_api *platform,
                  u8 *images_path,
                  u32 font_height,
			      u32 font_image_width,
                  stream_data *info_stream)
{

    font_load_info result = font_allocate_file_info_from_images(
		          area,
				  platform,
                  images_path,
                  font_height,
			      font_image_width,
				  ASCII_FIRST_CHAR,
				  ASCII_LAST_CHAR,
                  info_stream);

	return(result);
}

static font_load_info 
//GenerateFontAsset
font_allocate_file_info_ascii_win32(memory_area *area,
                  u8 *fontPath,
                  u8 *fontName,
                  u32 fontBitmapHeight,
			      u32 fontImageW,
                  stream_data *info_stream)
{
    u32 first_glyph = ' ';
    u32 last_glyph  = '~';

    font_load_info result = font_allocate_file_info_win32(area,
                      fontPath,
                      fontName,
                      fontBitmapHeight,
    			      fontImageW,
					  first_glyph,
					  last_glyph,
                      info_stream);

	return(result);
}

static font_load_info 
//GenerateFontAsset
font_allocate_file_info_ascii(
		          memory_area *area,
				  platform_api *platform,
                  u8 *font_path_and_name,
                  u32 font_height,
			      u32 font_image_width,
                  stream_data *info_stream)
{
    u32 first_glyph = ' ';
    u32 last_glyph  = '~';

    font_load_info result = font_allocate_file_info(area,
                      font_path_and_name,
                      font_height,
    			      font_image_width,
					  first_glyph,
					  last_glyph,
					  platform,
                      info_stream);

	return(result);
}

#define NotImplemented Assert(0)

typedef struct{

	stream_data assetHeaderStream;

	platform_api *platform;
	platform_file_handle file_handle;
	platform_file_min_attributes fileInfo;

	ppse_file_header header;
	//ppse_asset_data *assets;

	uint32 allocatedFileCount;
	uint32 allocatedDataFromHeader;
}ppse_file_work;

static void
ppse_Load()
{
	NotImplemented;
}

static void
ppse_Read()
{
	NotImplemented;
}

inline ppse_asset_data *
ppse_put_header(ppse_file_work *fileWork,
		        asset_type type,
				uint8 *path_and_name)
{
	//The file name length and actual name always go first
	uint32 fileNameLength = string_count(path_and_name);
	stream_PushData(&fileWork->assetHeaderStream, &fileNameLength, sizeof(uint32));
	stream_PushData(&fileWork->assetHeaderStream, path_and_name, fileNameLength);

	ppse_asset_data *assetData = stream_OutStruct(&fileWork->assetHeaderStream, ppse_asset_data); 
	//assetData->type		  = type;
    assetData->data_offset = fileWork->allocatedDataFromHeader; 
	assetData->id         = assets_generate_id(path_and_name);

	fileWork->allocatedFileCount++;

	return(assetData);
}

inline void
ppse_put_file(ppse_file_work *fileWork, memory_area *area, stream_data *info_stream, asset_type type, uint8 *path_and_name)
{

	stream_pushf(info_stream, "Opening file %s to allocate", path_and_name);
    platform_api *platform = fileWork->platform;

	platform_file_handle newFile = platform->f_open_file(path_and_name, platform_file_op_read);
	if(newFile.handle)
	{
	    stream_pushf(info_stream, "File found! allocating %s into the pack file", path_and_name);


		//temporary_area temporaryArea = temporary_area_begin(area);
		platform_entire_file entireFile  = platform_read_entire_file_handle(platform, newFile, area);

		platform->f_write_to_file(fileWork->file_handle, fileWork->allocatedDataFromHeader, entireFile.size, entireFile.contents);

		//temporary_area_end(&temporaryArea);
		platform->f_close_file(newFile);

	    ppse_asset_data *assetData = ppse_put_header(fileWork, type, path_and_name);

		assetData->data_size = entireFile.size;
		fileWork->allocatedDataFromHeader += entireFile.size;

	}
	else
	{

	   stream_pushf(info_stream, "Error! could not find or open file %s", path_and_name);
	}
}

inline void
ppse_put_image(ppse_file_work *fileWork, memory_area *area, uint8 *path_and_name)
{
    platform_api *platform = fileWork->platform;
	asset_type type = asset_type_image;

	platform_file_handle newFile = platform->f_open_file(path_and_name, platform_file_op_read);
	if(newFile.handle)
	{
		//temporary_area temporaryArea = temporary_area_begin(area);

		image_data image  = parse_png_or_bmp_from_handle(area, platform, newFile, 0);
		u32 imageSize     = (image.width * image.height * image.bpp);
		u32 totalFileSize = sizeof(image_data) + imageSize;

		//temporary_area_end(&temporaryArea);
		platform->f_close_file(newFile);

		ppse_image imageHeader = {0};
		imageHeader.w   = image.width;
		imageHeader.h   = image.height;
		imageHeader.bpp = image.bpp;


		u32 data_offset = fileWork->allocatedDataFromHeader;
		platform->f_write_to_file(fileWork->file_handle, data_offset, sizeof(ppse_image), &imageHeader);
		data_offset += sizeof(ppse_image);
		platform->f_write_to_file(fileWork->file_handle, data_offset, imageSize, image.pixels);
		data_offset += imageSize;


	    ppse_asset_data *assetData = ppse_put_header(fileWork, type, path_and_name);
		assetData->data_size = totalFileSize;
        fileWork->allocatedDataFromHeader = data_offset;

	}
}

static font_load_info
font_create_file_to_memory(memory_area *area, stream_data *info_stream, uint8 *fontPath ,uint8 *fontName)
{

    font_load_info font_load_process = font_allocate_file_info_ascii_win32(area,
												      fontPath,
												      fontName,
												      64,
												      512,
												      info_stream);
	return(font_load_process);

}

static void 
ppse_WriteFont(ppse_file_work *fileWork,
		       memory_area *area,
			   platform_api *platform, uint8 *fontPath ,uint8 *fontName, stream_data *info_stream)
{

#if 0

	//Common file name and length
	uint32 fileNameLength = string_count(fontName);
	stream_PushData(&fileWork->assetHeaderStream, &fileNameLength, sizeof(uint32));
	stream_PushData(&fileWork->assetHeaderStream, fontName, fileNameLength);

	ppse_asset_data *assetData = stream_OutStruct(&fileWork->assetHeaderStream, ppse_asset_data); 
	assetData->id		  = asset_font_default;
	assetData->type		  = asset_type_font;
    assetData->data_offset = fileWork->allocatedDataFromHeader; 
#else
	ppse_asset_data *assetData = ppse_put_header(fileWork, asset_type_font, fontName); 
	assetData->id		  = assets_generate_id(fontName);
#endif

    font_load_info font_load_process = font_allocate_file_info_ascii_win32(area,
												      fontPath,
												      fontName,
												      64,
												      512,
												      info_stream);


	//First glyph table, then horizontal advancements, then font spritesheet.
	//Get current offset
	uint64 fontDataOffset = fileWork->allocatedDataFromHeader;

	stream_pushf(info_stream, "Allocating  glyphs\n");
	platform->f_write_to_file(fileWork->file_handle, fontDataOffset,font_load_process.glyph_table_size, font_load_process.glyphs);
	fontDataOffset += font_load_process.glyph_table_size;

	stream_pushf(info_stream, "Allocating displacements\n");
	platform->f_write_to_file(fileWork->file_handle, fontDataOffset,font_load_process.glyph_displacements_size, font_load_process.glyph_displacements);
	fontDataOffset += font_load_process.glyph_displacements_size;

	stream_pushf(info_stream, "Allocating image pixels...\n");
	platform->f_write_to_file(fileWork->file_handle, fontDataOffset, font_load_process.fontImageSize, font_load_process.fontImagePixels);
	fontDataOffset = font_load_process.fontImageSize;

	uint32 totalFileSize = font_load_process.totalSizeUsed;
	//Add file count and the total size.
    fileWork->allocatedDataFromHeader += totalFileSize; 
	assetData->data_size = totalFileSize;

	stream_pushf(info_stream, "Finished allocation. total size used %u\n", totalFileSize);
}


static void
ppse_Write(ppse_file_work *fileWork, platform_api *platform, uint32 id, asset_type type, void* contents)
{
	NotImplemented;
}

static void
ppse_ReadAssetsInfo(ppse_file_work *fileWork, platform_api *platform, stream_data *info_stream)
{
	uint32 totalFileCount  = fileWork->allocatedFileCount + fileWork->header.asset_count;
	uint32 assetReadOffset = fileWork->allocatedDataFromHeader;
    stream_pushf(info_stream, "Starting to read %u assets...\n", totalFileCount);
	for(uint32 fileI = 0; fileI < totalFileCount; fileI++)
	{
		uint32 fileNameLength = 0;
		platform->f_read_from_file(fileWork->file_handle, assetReadOffset, sizeof(uint32), &fileNameLength);
		assetReadOffset += sizeof(uint32);
		Assert(fileNameLength);

		//Not safe
	    uint8 fileNameBuffer[126] = {0};
		platform->f_read_from_file(fileWork->file_handle, assetReadOffset, fileNameLength, &fileNameBuffer);
		assetReadOffset += fileNameLength;
		
		ppse_asset_data assetData = {0};
		platform->f_read_from_file(fileWork->file_handle, assetReadOffset, sizeof(ppse_asset_data), &assetData);
		assetReadOffset += sizeof(ppse_asset_data);


	    stream_pushf(info_stream, "File N.%u, id=%s, Name length=%u, Name=%s, Data from header=%u\n",
					 fileI,
					 (uint8 *)&assetData.id,
					 fileNameLength,
					 fileNameBuffer,
					 assetData.data_offset);
	}

}
static void
ppse_work_end_and_close(ppse_file_work *fileWork, platform_api *platform, stream_data *info_stream)
{
  
	if(fileWork->allocatedFileCount)
	{
	    //Write new header data
	    fileWork->header.asset_count += fileWork->allocatedFileCount;
	    fileWork->header.offset_to_file_headers = fileWork->allocatedDataFromHeader;

        stream_pushf(info_stream, "Allocating footer data...\n");
        stream_pushf(info_stream, "New assets amount: %u\n", fileWork->allocatedFileCount);
        stream_pushf(info_stream, "Total assets on file: %u\n", fileWork->header.asset_count);
		//Update header data
	    platform->f_write_to_file(fileWork->file_handle, 0, sizeof(ppse_file_header), &fileWork->header);

        stream_chunk *chunk = fileWork->assetHeaderStream.first; 
	    uint32 assetI = 0;
	    uint32 assetOffset = fileWork->allocatedDataFromHeader;
        while(assetI < fileWork->allocatedFileCount)
        {
	       u32 fileNameLength      = *(u32 *)stream_consume_size(&fileWork->assetHeaderStream, sizeof(uint32));
	       u8 *fileName            = stream_consume_size(&fileWork->assetHeaderStream, fileNameLength);
           ppse_asset_data *assetData = stream_consume_data(&fileWork->assetHeaderStream, ppse_asset_data);

	       stream_pushf(info_stream, "Asset N.%u, id=%u, Name=%s, Data size=%u\n", assetI, assetData->id, fileName, assetData->data_size);

	       platform->f_write_to_file(fileWork->file_handle, assetOffset, sizeof(uint32), &fileNameLength);
	       assetOffset += sizeof(uint32);
	       platform->f_write_to_file(fileWork->file_handle, assetOffset, fileNameLength, fileName);
	       assetOffset += fileNameLength;
	       platform->f_write_to_file(fileWork->file_handle, assetOffset, sizeof(ppse_asset_data), assetData);
	       assetOffset += sizeof(ppse_asset_data);

		   //stream_pushf(info_stream, "Allocated asset of size %u", assetData->size);

	       assetI++;
        }
        stream_pushf(info_stream, "File allocation finished and closed.\n");
	}
	else 
	{
		stream_pushf(info_stream, "No new files allocated. Closing file...\n");
	}

     
    platform->f_close_file(fileWork->file_handle);
}

//TODO(Agu): Get footer from file
static ppse_file_work
ppse_OpenFile(uint8 *filePath, platform_api *platform, stream_data *info_stream)
{
	//Note(Agu): I might need to clear the memory first.
    ppse_file_work fileWork = {0};
    fileWork.file_handle = platform->f_open_file(filePath, platform_file_op_write | platform_file_op_read);
	fileWork.platform = platform;

    if(!fileWork.file_handle.handle)
    {
      stream_pushf(info_stream, "Error opening the pack file %s", filePath);
      return(fileWork);
    }

	platform->f_read_from_file(fileWork.file_handle, 0, sizeof(ppse_file_header), &fileWork.header);
	ppse_file_header *ppseHeader = &fileWork.header;

	//Read magic num
	if(ppseHeader->signature != ppse_SIGNATURE)
	{
	  platform->f_close_file(fileWork.file_handle);
      stream_pushf(info_stream, "The magic num %u was invalid. Expected %u", ppseHeader->signature, ppse_SIGNATURE);

	}
	else
	{
		fileWork.fileInfo = platform->f_get_file_info(fileWork.file_handle);
		fileWork.allocatedDataFromHeader = ppseHeader->offset_to_file_headers; 
	}


    return(fileWork);
}

static ppse_file_work
ppse_create_pack_file_for_work(uint8 *filePathAndName, memory_area *area, platform_api *platform, stream_data *info_stream)
{
	//Note(Agu): I might need to clear the memory first.
    ppse_file_work fileWork = {0};
	fileWork.assetHeaderStream = stream_Create(area);
	fileWork.platform = platform;

    platform_file_handle file_handle = platform->f_open_file(filePathAndName, platform_file_op_write | platform_file_op_read | platform_file_op_create);
    fileWork.file_handle = file_handle;
    if(!file_handle.handle)
    {
      stream_pushf(info_stream, "There was an error while creating the file on %s", filePathAndName);
      return(fileWork);
    }
     
    ppse_file_header *ppseHeader = &fileWork.header;
    ppseHeader->signature = ppse_SIGNATURE; 
    ppseHeader->asset_count = 0; 
    //Points to the end of the file without data.
    ppseHeader->offset_to_file_headers = sizeof(ppse_file_header);

//    fileWork.header = ppseHeader;
	fileWork.allocatedFileCount = 0;
	fileWork.allocatedDataFromHeader = ppseHeader->offset_to_file_headers;

	platform->f_write_to_file(fileWork.file_handle, 0, sizeof(ppse_file_header), &fileWork.header);

    return(fileWork);
}

static image_data
load_image_from_handle(memory_area *area,
                       platform_api *platform,
                       platform_file_handle file_handle,
                       stream_data *infostream)
{
    image_data result = {0};
    platform_file_min_attributes fileInfo = platform->f_get_file_info(file_handle);
	//
    uint8 *fileContents = memory_area_push_size(area, fileInfo.size);
    platform->f_read_from_file(file_handle, 0, fileInfo.size, fileContents);

#if 1
    stream_buffer buffer = {0};
    buffer.contents = fileContents; 
    buffer.size     = fileInfo.size;
    stream_data stream   = stream_CreateFromBuffer(buffer);

    u64 signature = *(u64 *)stream.buffer.contents;

    if((u16)signature == 'MB') //BM or 4d42
    {
       result = bmp_from_stream(area, &stream);
    }
    else if(png_CheckSignature_u64(signature))
    {
       result = png_parse_from_stream(area, &stream, infostream);
    }
#endif
    return(result);
}

inline void
ppse_extract_font_sheet_to_path(
		memory_area *extracting_area,
		platform_api *platform,
		u8 *font_path_and_name,
		u8 *image_path_and_name,
		stream_data *info_stream)
{
	ppse_font font_header = {0};
	platform_file_handle font_file_handle = platform->f_open_file(
			font_path_and_name,
			platform_file_op_read);

	if(font_file_handle.handle)
	{
		temporary_area temporary_extracting_area = temporary_area_begin(extracting_area);

	    platform->f_read_from_file(
	    		font_file_handle,
	    		0,
	    		sizeof(ppse_font),
	    		&font_header);

	    u32 signature_check = font_header.signature == ppse_font_SIGNATURE;
	    if(signature_check)
	    {

			stream_data png_stream = stream_Create(extracting_area);

			image_data font_image = {0};
			font_image.width  = font_header.pixelsW;
			font_image.height = font_header.pixelsH;
			font_image.bpp    = 4;
			u32 total_font_pixels_size = font_image.width * font_image.height * font_image.bpp;
			font_image.pixels = memory_area_push_size(
					extracting_area,
					total_font_pixels_size);
			//read pixels from the font file
			u32 font_pixels_offset = font_header.offset_to_pixels;
			platform->f_read_from_file(
					font_file_handle,
					font_pixels_offset,
					total_font_pixels_size,
					font_image.pixels);

			png_write_to_stream(
					&font_image,
					&png_stream,
					info_stream);

		//	u32 success = platform_write_stream_to_file(
		//			platform,
		//			&png_stream,
		//			image_path_and_name);
            CRTWriteStreamToFile(&png_stream, image_path_and_name);
			u32 success = 1;

			if(!success)
			{
				stream_pushf(
						info_stream,
						"Error while extracting the font atlas at %s! could not create file",
						image_path_and_name);
			}
			
	    }
	    else
	    {
	    	stream_pushf(
					info_stream,
					"Error while extracting the atlas from the given font file! signature check failed!");
	    }
	    temporary_area_end(&temporary_extracting_area);

		platform->f_close_file(font_file_handle);
	}
	else
	{
		stream_pushf(
				info_stream,
				"Error while attempting to extract the image atlas from the file %s! it could not be opened!",
				font_path_and_name);
	}

}

inline void
ppse_print_font_info(
		platform_api *platform,
		u8 *font_path_and_name,
		stream_data *info_stream)
{
	platform_file_handle font_file = platform->f_open_file(
			font_path_and_name, platform_file_op_read);
	ppse_font font_file_header = {0};
	if(font_file.handle)
	{
		platform->f_read_from_file(
				font_file,
				0,
				sizeof(ppse_font),
				&font_file_header);
		u32 signature_check = font_file_header.signature == ppse_font_SIGNATURE;
		if(signature_check)
		{
			u32 offset_to_kerning_pairs = sizeof(ppse_font) + (font_file_header.glyph_count * sizeof(ppse_glyph));

	//		stream_pushf(
	//				info_stream,
	//				"Reading kerning pairs...\n");
			printf("Reading kerning pairs...\n");
			for(u32 y = 0;
					y < font_file_header.glyph_count;
					y++)
			{
				//stream_pushf(info_stream, "[");
				printf("(%u). [", y);
				for(u32 x = 0;
						x < font_file_header.glyph_count;
						x++)
				{
					i32 displacement = 0;
					u32 at = offset_to_kerning_pairs + ((x + (y * font_file_header.glyph_count)) * sizeof(i32));
					platform->f_read_from_file(
							font_file,
							at,
							sizeof(i32),
							&displacement);
					//stream_pushf(info_stream, "%u", displacement);
					printf("%u", displacement);
					if(x + 1 != font_file_header.glyph_count)
					{
					    //stream_pushf(info_stream, ",");
						printf(",");
					}
				}
				//stream_pushf(info_stream, "]\n");
				printf("]\n");
			}
		}
		else
		{
	    	stream_pushf(
					info_stream,
					"Error while reading the given font file! signature check failed!");

		}
		platform->f_close_file(
				font_file);
	}
	else
	{
		stream_pushf(info_stream, "Could not open or find the font file \"%s\"", font_path_and_name);
	}
}

static inline void
ppse_print_asset_info(
		platform_api *platform,
		u8 *path_and_name,
		stream_data *info_stream)
{
	platform_file_handle file = platform->f_open_file(path_and_name, platform_file_op_read);
	if(file.handle)
	{
		asset_type type = resources_get_type_by_path_and_name(path_and_name);
		if(type > 0)
		{
			ppse_asset_header header = {0};
			u8 *types[asset_type_COUNT] = {0};
			types[asset_type_map] = "map";
			types[asset_type_model] = "model";
			types[asset_type_tileset] = "tileset";
			types[asset_type_image] = "image";
			u8 *type_name = types[type];
			type_name = type_name ? type_name : "";

			platform->f_read_from_file(file, 0, sizeof(header), &header);
			stream_pushf(info_stream, "-- File: \"%s\"", path_and_name);
			stream_pushf(info_stream, "Type: %s", type_name);
			stream_pushf(info_stream, "Signature: %u", header.signature);
			stream_pushf(info_stream, "Version: %u", header.version);
			stream_pushf(info_stream, "Composite resources count: %u", header.composite_resource_count);
			stream_pushf(info_stream, "----- Composite resources -----");
			
			u32 read_offset = header.offset_to_composite_resources;
			for(u32 c = 0; c < header.composite_resource_count; c++)
			{
				ppse_composite_resource composite_resource = {0};
				platform->f_read_from_file(file, read_offset, sizeof(ppse_composite_resource), &composite_resource);
				stream_pushf(info_stream, "%u. \"%s\"", c, composite_resource.path_and_name);
				read_offset += sizeof(ppse_composite_resource);
			}

			platform->f_close_file(file);
		}
		else
		{
			stream_pushf(info_stream, "Error while trying to read the asset info of \"%s\". The type of asset could not be identified!", path_and_name);
		}
	}
	else
	{
		stream_pushf(info_stream, "Error! could not open the file \"%s\"", path_and_name);
	}
}

inline void
test_create_blank_image(
		memory_area *extracting_area,
		platform_api *platform,
		u8 *image_path_and_name,
		stream_data *info_stream)
{
		temporary_area temporary_extracting_area = temporary_area_begin(extracting_area);

			stream_data png_stream = stream_Create(extracting_area);

			image_data font_image = {0};
			font_image.width  = 512;
			font_image.height = 539;
			font_image.bpp    = 4;
			u32 total_font_pixels_size = font_image.width * font_image.height * font_image.bpp;
			font_image.pixels = memory_area_push_size(
					extracting_area,
					total_font_pixels_size);
			//read pixels from the font file
			png_write_to_stream(
					&font_image,
					&png_stream,
					info_stream);

		//	u32 success = platform_write_stream_to_file(
		//			platform,
		//			&png_stream,
		//			image_path_and_name);
            CRTWriteStreamToFile(&png_stream, "blank.png");

	    temporary_area_end(&temporary_extracting_area);
}

inline void
test_read_png_image(memory_area *area,
                    platform_api *platform,
                    stream_data *infostream,
					u8 *path_and_name)
{
	platform_file_handle file_handle = platform->f_open_file("blank.png", platform_file_op_read);
    parse_png_or_bmp_from_handle(area,
                                 platform,
                                 file_handle,
                                 infostream);
	platform->f_close_file(file_handle);
}

inline void
test_create_image_and_and_draw_curve(
		memory_area *extracting_area,
		platform_api *platform,
		u8 *image_path_and_name,
		stream_data *info_stream)
{
		temporary_area temporary_extracting_area = temporary_area_begin(extracting_area);

			stream_data png_stream = stream_Create(extracting_area);

			image_data font_image = {0};
			font_image.width  = 512;
			font_image.height = 512;
			font_image.bpp    = 4;
			u32 total_font_pixels_size = font_image.width * font_image.height * font_image.bpp;
			font_image.pixels = memory_area_clear_and_push(
					extracting_area,
					total_font_pixels_size);
			//Draw curve!
			vec2 p0 = {0};
			vec2 p_interpolation = {0};
			vec2 p1 = {0};

			p0.x = 0;
			p0.y = font_image.height / 2.0f ;
			p_interpolation.x = font_image.width * 0.5f;
			p_interpolation.y = 0;
			p1.x = font_image.width - 1.0f;
			p1.y = font_image.height / 2.0f ;

#if 0
			draw_curve_to_pixels_subdivided(
					p0,
					p_interpolation,
					p1,
					0xff0000ff,
					16,
					font_image.width,
					font_image.height,
					font_image.pixels);
#else
			draw_curve_to_pixels_interpolated(
					p0,
					p_interpolation,
					p1,
					0xff0000ff,
					font_image.width,
					font_image.height,
					font_image.pixels);

#endif
			//read pixels from the font file
			png_write_to_stream(
					&font_image,
					&png_stream,
					info_stream);

		//	u32 success = platform_write_stream_to_file(
		//			platform,
		//			&png_stream,
		//			image_path_and_name);
            CRTWriteStreamToFile(&png_stream, "blank.png");

	    temporary_area_end(&temporary_extracting_area);
}

inline void
read_ttf_from_path(
		memory_area *area,
		platform_api *platform,
		stream_data *info_stream,
		u8 *path_and_name)
{
	stream_pushf(
			info_stream,
			"Attempting to read %s\n", path_and_name);

	platform_file_handle font_file_handle = platform->f_open_file(
			path_and_name,
			platform_file_op_read);

	if(font_file_handle.handle)
	{
		temporary_area temporary_font_area = temporary_area_begin(area);

		platform_entire_file font_entire_file = platform_read_entire_file_handle(
				platform,
				font_file_handle,
				area);
		platform->f_close_file(font_file_handle);

        //stream_buffer buffer    = {0};
        //buffer.contents         = font_entire_file.contents; 
        //buffer.size             = font_entire_file.size;
        //stream_data font_stream = stream_CreateFromBuffer(buffer);

        ttf_parse_from_memory(
				area,
				(u8)ASCII_LAST_CHAR,
				(u8)ASCII_LAST_CHAR,
			    font_entire_file.contents,
				info_stream);

		temporary_area_end(&temporary_font_area);

	}
	else
	{
		stream_pushf(
				info_stream,
				"Unable to open font file \"%s\"",
				path_and_name);
	}
}

inline void
save_glyph_image_from_path(
		memory_area *area,
		platform_api *platform,
		u32 font_height,
		u8 *path_and_name,
		u8 *image_path_and_name,
		stream_data *info_stream)

{

	//Create box filter
#if 1

	ttf_file_work font_file_work = {0};
	ttf_fill_file_work_from_path(
			area,
			platform,
			&font_file_work,
			(u8)ASCII_FIRST_CHAR,
			(u8)ASCII_LAST_CHAR,
			path_and_name,
			info_stream);
	image_data font_image = 
ttf_create_atlas_padding(
		platform,
		area,
		info_stream,
		&font_file_work,
		512,
		font_height,
		0,
		20, 20);
	
	//image_convert_to_sdf(
	//		area,
	//		&font_image,
	//		0xffffffff);
//	down_sample_image_linear_x2(&font_image);
#else
	ttf_file_work font_file_work = {0};
	ttf_fill_file_work_from_path(
			area,
			platform,
			&font_file_work,
			'f',
			(u8)ASCII_LAST_CHAR,
			path_and_name,
			info_stream);
	image_data font_image = ttf_create_glyph_bitmap_from_index_anti_aliased(
			area,
			&font_file_work,
		    ttf_get_height_scale(&font_file_work, 72),
			0,
			info_stream);

	image_convert_to_sdf_8ssedt(
			area,
			&font_image,
			0xffffffff);

#endif

		stream_data png_stream = stream_Create(area);

		png_write_to_stream(
				&font_image,
				&png_stream,
				info_stream);

     CRTWriteStreamToFile(&png_stream, image_path_and_name);
}

static void
get_max_error_sine_cosine(stream_data *info_stream)
{
	f64 sine_error = 0;
	f64 cosine_error = 0;
	f64 sine_error64 = 0;
	f64 cosine_error64 = 0;
	f64 tan_error = 0;
	f64 tan_error64 = 0;
	f64 atan_error = 0;
	f64 atan2_error = 0;
	f64 sqrt_error = 0;
	f64 sqrt64_error = 0;

	f32 l = 0;
	f32 l_max = 50;
	f32 au = 0.00001f;
	f32 r0 = sqrt32(2);
	f64 r1 = sqrt64_new(2);
	f32 r2 = sqrt32_16_16(2);
	/*
	   pi / 2 - (atan(0.5) + atan(x)) = atan(x);
	*/
	while(l <= l_max)
	{
		f32 og_sin = sinf(l);
		f32 og_cos = cosf(l);
		f32 og_tan = tanf(l);
		f32 og_atan = atanf(l);
		f32 n_sin  = sin32(l);
		f32 n_cos  = cos32(l);
		f32 n_tan  = tan32(l);
		f32 n_atan  = arctan32(l);

		f64 og_sin64 = sin(l);
		f64 og_cos64 = cos(l);
		f64 n_sin64  = sin64(l);
		f64 n_cos64  = cos64(l);

		f32 n_atan2 = 0;
		f32 og_atan2 = 0;

		f32 og_sqrt = sqrtf(l);
		f32 n_sqrt = sqrt32(l);
		f64 og_sqrt64 = sqrt(l);
		f64 n_sqrt64 = sqrt64(l);
		if(l != 0)
		{
			og_atan2 = atan2f(-20, l);
			n_atan2 = arctan232(-20, l);
		}

		l += au;

		sine_error = MAX(sine_error, ABS(og_sin - n_sin));
		cosine_error = MAX(cosine_error, ABS(og_cos - n_cos));
		sine_error64 = MAX(sine_error64, ABS(og_sin64 - n_sin64));
		cosine_error64 = MAX(cosine_error64, ABS(og_cos64 - n_cos64));
		tan_error = MAX(tan_error, ABS(ABS(og_tan) - ABS(n_tan)));
		atan_error = MAX(atan_error, ABS(ABS(og_atan) - ABS(n_atan)));
		atan2_error = MAX(atan2_error, ABS(ABS(og_atan2) - ABS(n_atan2)));
		sqrt_error = MAX(sqrt_error, ABS(og_sqrt - n_sqrt));
		sqrt64_error = MAX(sqrt64_error, ABS(og_sqrt64 - n_sqrt64));
	//	tan_error64 = MAX(tan_error64, ABS(og_cos64 - n_cos64));

//	stream_pushf(
//			info_stream,
//			"sine_error:%.9f, cosine_error:%.9f, sine_error:%.9f, cosine_error:%.9f, tan_error:%.9f, tan_error64:%.9f with %.9f",
//			sine_error,
//			cosine_error,
//			sine_error64,
//			cosine_error64,
//			tan_error,
//			tan_error64,
//			l);

	}


	stream_pushf(
			info_stream,
			"sine_error:%.9f, cosine_error:%.9f, sine_error:%.9f, cosine_error:%.9f, tan_error:%.9f, tan_error64:%.9f, atan_error:%.9f, atan2_error:%.9f, sqrt_error:%.9f",
			sine_error,
			cosine_error,
			sine_error64,
			cosine_error64,
			tan_error,
			tan_error64,
			atan_error,
			atan2_error,
			sqrt_error);

}

typedef enum{
	command_flags_args_more_than,
	command_flags_args_more_equal_than,
}ppse_command_flags;
typedef struct{
	ppse_command_flags flags;
}ppse_command;


int main(int argc, char** args)
{
    platform_api platform = platform_initialize();
	//Font options
    font_identifier fonts[] = {{"c:/windows/fonts/arial.ttf", "arial"},
    							{"c:/users/jijia/appdata/local/microsoft/windows/fonts/roboto-regular.ttf", "Roboto"}};
	f32 test_atan20 = arctan232(0.0913702101f, -0.995816946f);
	f32 test_atan21 = atan2f(0.0913702101f, -0.995816946f);
	if(argc > 1)
	{

        memory_size memorysize = MEGABYTES(256); 
        void *mem = malloc(memorysize); 
        memset(mem, 0, memorysize);
        memory_area area = memory_area_create(memorysize, mem); 
    
        //global_ProgramStream = stream_Create(&area);
        memory_area info_streamArea = memory_area_create_from(&area, MEGABYTES(1)); 
        stream_data info_stream	   = stream_Create(&info_streamArea);

		u8 *argCommand = string_to_low((u8 *)args[1]);

		if(string_compare(argCommand, "create"))
		{
			if(argc > 2)
			{
			    //file_path_name_and_extension_info fileInfo = path_get_file_path_info(args[2]);

				u8 *pack_path_and_name = args[2];
				u32 success = ppse_create_and_save_pack_file(&platform, pack_path_and_name);

				if(!success)
				{
				    printf("An error ocurred while creating the pack file %s", pack_path_and_name);
				}
				else
				{
				    printf("Created pack file at: %s", pack_path_and_name);
				}

			}
			else
			{
				printf("A name for the pack file was not specified");
			}
		}
		else if(string_compare(argCommand, "createdev"))
		{
			//create and store font

		}
		else if(string_compare(argCommand, "buildassets"))
		{
			if(argc > 2)
			{
				u8 *packNameAndPath = args[2];
				if(path_and_name_is_valid(packNameAndPath))
				{

                    ppse_file_work fileWork = ppse_create_pack_file_for_work(packNameAndPath, &area, &platform, &info_stream);
    	            //
    	            //Image writing test
    	            //
    	            ppse_put_image(&fileWork, &area,  "data/images/stforest_blend.png");
    	            ppse_put_image(&fileWork, &area,  "data/images/mdf.png");
    	            ppse_put_image(&fileWork, &area,  "data/images/fx.png");
    	            ppse_put_image(&fileWork, &area,  "data/images/sForest0_blend.png");
    	            ppse_put_image(&fileWork, &area,  "data/images/him.png");
    

    	            ppse_put_file(&fileWork, &area,  &info_stream, asset_type_font, "data/fonts/roboto.font");
    
    	            ppse_work_end_and_close(&fileWork, &platform, &info_stream);
				}
				else
				{
					printf("The file name \"%s\" was invalid", packNameAndPath);
				}
			}
			else
			{
				printf("No name was specified!");
			}
		}
		else if(string_compare(argCommand, "setupimage"))
		{
    
            platform_file_handle filehandle = platform.f_open_file(args[2], platform_file_op_read);
            if(!filehandle.handle)
            {
                printf("Image not found!");
                return(0);
    
            }
            image_data imagefile = parse_png_or_bmp_from_handle(&area, &platform, filehandle, &info_stream);
    
            stream_data writeStream = stream_Create(&area);
			u32 tile_size = 8;
			if(argc > 4)
			{
				u32 success = u32_from_string(args[4], &tile_size);
			}
            //Linear_DownScaleImage(&imagefile, 1);
    	    //image_data result_image = image_OffsetSpriteSheet(&imagefile, &area, 16, 16, 2, 2, 1);
    	    image_data result_image = image_set_tiles_for_blending(&imagefile, &area, tile_size, tile_size, 0);
    
            png_write_to_stream(&result_image, &writeStream, &info_stream);
    
			u8 *output_image_path_and_name = 0;
			u8 finalImageName[96] = {0};
			file_path_name_and_extension_info image_file_info = path_get_file_path_info(args[2]);
			if(argc > 3)
			{
				output_image_path_and_name = args[3];

			}
			else
			{
				printf("No output path and name specified. Using %s_blend.png as default.",
						image_file_info.name);
				format_text(finalImageName, sizeof(finalImageName), "%s_blend.png", image_file_info.name);
				output_image_path_and_name = finalImageName;
			}
            CRTWriteStreamToFile(&writeStream, output_image_path_and_name);

			printf("Setup image finished with a tile size of %u!", tile_size);
    
            platform.f_close_file(filehandle);

		}
		else if(string_compare(argCommand, "createfont"))
		{

			if(argc > 2)
			{
				u8 *fontNameAndPath = args[2];
				u32 font_bitmap_height = 72;
				if(argc > 3)
				{
				    u32_from_string(args[3], &font_bitmap_height);
					if(font_bitmap_height < 20)
					{
						font_bitmap_height = 20;
					}
					printf("Specified bitmap height of %u", font_bitmap_height);
				}
				if(path_and_name_is_valid(fontNameAndPath))
				{

					platform_file_handle fontFile = platform.f_open_file(fontNameAndPath, platform_file_op_create_new);

					if(fontFile.handle)
					{

					    temporary_area temporaryArea = temporary_area_begin(&area);

#if 0
                        font_load_info fontFileData = font_allocate_file_info_ascii_win32(
					    		          &area,
                                          fonts[1].path,
                                          fonts[1].name,
                                          font_bitmap_height, //bitmap height
                        			      512, //dimensions
                                          &info_stream);
#else
                        font_load_info fontFileData = font_allocate_file_info_ascii(
					    		          &area,
										  &platform,
										  "roboto-regular.ttf",
                        			      512, //dimensions
                                          font_bitmap_height, //bitmap height
                                          &info_stream);
#endif
					    temporary_area_end(&temporaryArea);

						//write font file!
						u32 data_offset = 0;
						ppse_font font_header = fontFileData.header;

	                    stream_pushf(&info_stream, "Allocating  header\n");
	                    platform.f_write_to_file(fontFile, data_offset, sizeof(ppse_font), &font_header);
	                    data_offset += sizeof(ppse_font);

	                    stream_pushf(&info_stream, "Allocating  glyphs\n");
	                    platform.f_write_to_file(fontFile, data_offset,fontFileData.glyph_table_size, fontFileData.glyphs);
	                    data_offset += fontFileData.glyph_table_size;

	                    stream_pushf(&info_stream, "Allocating displacements\n");
	                    platform.f_write_to_file(fontFile, data_offset,fontFileData.glyph_displacements_size, fontFileData.glyph_displacements);
	                    data_offset += fontFileData.glyph_displacements_size;

	                    stream_pushf(&info_stream, "Allocating image pixels...\n");
	                    platform.f_write_to_file(fontFile, data_offset, fontFileData.fontImageSize, fontFileData.fontImagePixels);
	                    data_offset = fontFileData.fontImageSize;

						stream_pushf(
								&info_stream,
								"Font file successfully created and saved at %s!",
								fontNameAndPath);
						platform.f_close_file(fontFile);
					}
					else
					{
						printf("Error while creating a font file with path and name %s", fontNameAndPath);
					}


				}
				else
				{
					printf("The file name \"%s\" was invalid", fontNameAndPath);
				}
			}
			else
			{
				printf("No command was specified!");
			}
		}
		else if(string_compare(argCommand, "createfontbm"))
		{
			if(argc > 3)
			{
				u8 *images_path = args[2];
				u8 *output_path_and_name = args[3];
				u32 font_height = 12;
				if(argc > 4)
				{
				    u32_from_string(args[4], &font_height);
					if(font_height < 12)
					{
						font_height = 12;
					}
				}

				if((path_and_name_is_valid(output_path_and_name)))
				{
					platform_file_handle output_file_handle = platform.f_open_file(
							output_path_and_name, platform_file_op_create_new);

					if(output_file_handle.handle)
					{
                        font_load_info fontFileData = font_allocate_file_info_from_images_ascii(
					    		          &area,
					    				  &platform,
					    				  images_path,
                                          font_height, //bitmap height
                        			      512, //dimensions
                                          &info_stream);

						//write font file!
						u32 data_offset = 0;
						ppse_font font_header = fontFileData.header;

	                    stream_pushf(&info_stream, "Allocating header\n");
	                    platform.f_write_to_file(output_file_handle, data_offset, sizeof(ppse_font), &font_header);
	                    data_offset += sizeof(ppse_font);

	                    stream_pushf(&info_stream, "Allocating glyphs\n");
	                    platform.f_write_to_file(output_file_handle, data_offset,fontFileData.glyph_table_size, fontFileData.glyphs);
	                    data_offset += fontFileData.glyph_table_size;

	                    stream_pushf(&info_stream, "Allocating displacements\n");
	                    platform.f_write_to_file(output_file_handle, data_offset,fontFileData.glyph_displacements_size, fontFileData.glyph_displacements);
	                    data_offset += fontFileData.glyph_displacements_size;

	                    stream_pushf(&info_stream, "Allocating image pixels...\n");
	                    platform.f_write_to_file(output_file_handle, data_offset, fontFileData.fontImageSize, fontFileData.fontImagePixels);
	                    data_offset = fontFileData.fontImageSize;

						platform.f_close_file(output_file_handle);
					}
					else
					{
						printf("Error! Could not create a font file at \"%s\" !", output_path_and_name);
					}

				}
				else
				{
					printf("The output path \"%s\" for the font is not a valid path", images_path);
				}
			}
			else
			{
				printf("Error of CreateFontBm. Not enough arguments");
			}
		}
		else if(string_compare(argCommand, "dumpfont"))
		{
			if(argc > 3)
			{
				u8 *font_path_and_name  = args[2];
				u8 *image_path_and_name = args[3];

                ppse_extract_font_sheet_to_path(
						&area,
						&platform,
						font_path_and_name,
						image_path_and_name,
						&info_stream);
//test_create_blank_image(&area, &platform, font_path_and_name, &info_stream);
//test_read_png_image(&area, &platform, &info_stream, 0);
			}
			else
			{
				printf("could not run the DumpFont command. not enough arguments given.");
			}
		}
		else if(string_compare(argCommand, "readfont"))
		{
			if(argc > 2)
			{
				u8 *font_path_and_name = args[2];
				ppse_print_font_info(
						&platform,
						font_path_and_name,
						&info_stream);
			}
			else
			{
				printf("Not enough arguments given for ReadFont");
			}
		}
		else if(string_compare(argCommand, "assetinfo"))
		{
			if(argc > 2)
			{
				u8 *path_and_name = args[2];
				ppse_print_asset_info(
						&platform,
						path_and_name,
						&info_stream);
			}
			else
			{
				printf("Not enough arguments given for AssetInfo");
			}
		}
		else if(string_compare(argCommand, "importedresources"))
		{
			if(argc > 2)
			{
				u8 *path_and_name = args[2];
				platform_file_handle file = platform.f_open_file(
						path_and_name, platform_file_op_read);
				if(file.handle)
				{
					ppse_editor_resources_header header = {0};
					platform.f_read_from_file(file, 0, sizeof(header), &header);
					if(header.signature == ppse_editor_resources_SIGNATURE)
					{
						printf("-- Reading imported assets sources:");
						u32 read_offset = sizeof(ppse_editor_resources_header);
						for(u32 r = 0; r < header.total_resources_count; r++)
						{
							ppse_editor_resource resource = {0};
							platform.f_read_from_file(file, read_offset, sizeof(resource), &resource);
							read_offset += sizeof(resource);

							u8 *types[asset_type_COUNT] = {0};
							types[asset_type_map] = "map";
							types[asset_type_model] = "model";
							types[asset_type_tileset] = "tileset";
							types[asset_type_image] = "image";
							u8 *type_name = types[resource.type];
							type_name = type_name ? type_name : "";

							printf("-%u:\nType: %s\npath and name: %s\n", r, type_name, resource.source);
						}
					}
					else
					{
						printf("Error! The file \"%s\" isn't a resource data file!", path_and_name);
					}
					platform.f_close_file(file);
				}
				else
				{
					printf("Error! could not open or find the resources data file \"%s\"!", path_and_name);
				}
			}
			else
			{
				printf("Not enough arguments given for ImportedResources");
			}
		}
		else if(string_compare(argCommand, "dev"))
		{

			u8 *resource_types[] = {
				"*.png", "*.bmp", "*.ppimg", "*.pptl", "*.ppmo"};
			u32 format_index = 0;
			u32 format_count = ARRAYCOUNT(resource_types);

			platform_file_scan_work scan_work = platform_file_scan_begin(
					&platform,
					&area,
					1,
					"..\\data");
			printf("-- Starting to read the game's content folder...\n");

			while(platform_scanning_directory(&scan_work))
			{
				format_index = 0;
				while(format_index < format_count)
				{
					platform_scan_first_file(&scan_work, resource_types[format_index]);
					while(platform_scanning_next_file(&scan_work))
					{
						platform_file_search_info current_file = scan_work.current_file;

						//copy data
						union{
							u32 value;
							u8 value8[4];
						}type_value = {0};
						//get extension
						u32 extension_length = path_get_extension(
								current_file.name,
								type_value.value8,
								4);
						//format
						type_value.value = FormatEncodeU32(type_value.value);
						//get rid of gaps
						u32 difference = 4 - extension_length;
						while(difference)
						{
							type_value.value >>= 8;
							difference--;
						}
						u8 file_path_and_name[256] = {0};
						string_concadenate(
								scan_work.next_full_directory,
								current_file.name,
								file_path_and_name,
								256);
						printf("Found file \"%s\"!\n", file_path_and_name);

						//compare with types
						switch(type_value.value)
						{
							case 'png': case 'bmp': case'pimg':
								{
									printf("The file is an image resource!\n");
								}break;
							case 'pptl':
								{
									printf("The file is a tileset resource!\n");
									platform_file_handle file = platform.f_open_file(
											file_path_and_name, platform_file_op_read);
									ppse_tileset_header t_header = {0};
									platform.f_read_from_file(
											file,
											0,
											sizeof(t_header),
											&t_header);
									u32 offset_to_image = t_header.offset_to_image_source;
									u8 image_source_name[256] = {0};
									printf("It uses the image \"%s\"!", image_source_name);
								}break;
							case 'ppmo':
								{
								}break;
						}
					}
					format_index++;
				}
			}
		}
		else
		{
			printf("The command \"%s\" was invalid.\n", args[1]);
		}
    
    	//These are scaling experiments
    #if 0
    #else
    #endif

       crt_read_info_stream(&info_stream);
	}
	else
	{
		printf("\n=== Assetbuilder commands:\n ==="
				"-create: creates a clean pack file with the specified name\nArgs:[name]\n"
				"-createdev: creates a default pack file for starting up with the editor. the default name is resources.ppse\n"
				"-BuildAssets: picks coded assets and packs them into the specified name in a .pack format\nArgs:\n[name]\n\n"
				"-SetupImage: picks image sheet and sets it up for blending. Default tile size = 16\nArgs:[image] (output path) (tile size)\n"
				"-CreateFont: Creates a .pfnt file with the specified name to use as asset using roboto as default. The font's height minimum value is 48 and the default 72\nArgs: [path and name] (font height)\n"
				"-CreateFontBm: Creates an ASCII .pfnt file using the bitmap letters in order, the bitmaps should be named by their character codes.\nArgs:[images path] [output path and name] (font_height)\n"
				"-DumpFont: Extracts the font's image atlas to a png file at the given path and name\nArgs: [font path and name] [image path and name]\n"
				"-ReadFont: read the information of font files\nArgs:[font path and name]\n"
				"-AssetInfo: read asset information and composite resources\nArgs:[path and name]\n"
				"-ImportedResources: Reads a .prdt (resource data) file to see the choosen assets for packaging\nArgs:[path and name]\n"
				"-dev: Reserved for testing new functions\n"
				"============================");
	}

    return(0);

}
