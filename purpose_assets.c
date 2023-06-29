
/*
   The correct formats are
Tileset:   pptl
font:      pfnt
map:       ppmp
model:     ppmo
custom img ppimg
entity:    pcos
frame animation: pfan

for the editor, the imported resources data file is .prdt
*/



/*
Notes:
Asset data allocations:
Memory for different assets are divided in chunks (asset_memory_header) and saved
in asset_memory.
asset_memory_header are composed of:
- u32 used : how much memory is using.
- u32 asset_index : which slot is occupying (asset_slot).
- next and previous header pointers.

When allocating assets, to look for enough allocation space, every header
gets read from the end of the current one (sizeof(asset_memory_header) + used)
and subtracting the location in memory of the next one in to know if the
space between is enough to allocate the next asset.
If enough space is found, the new header gets allocated between the current compared
header and its next pointing one and the needed size is taken.
If there is no next header after the current one, a new one gets allocated at the end
of asset_memory.

assets_data_allocation_begin and assets_data_allocation_end are used for these kind of
operations.


=== asset_slot_data ===
Composed of:
- u32 offset_to_name : offset to its name header inside the pack file
- asset_type type : self explanatory.
- asset_state state : loaded or free.
- u32 id.
- u32 index.
- asset_memory_header : Where its memory gets allocated.
- An union of all of its data.

*/
#define ppse_image_SIGNATURE      FormatEncodeU32('pimg')
#define ppse_model_SIGNATURE      FormatEncodeU32('ppmo')
#define ppse_animation_SIGNATURE  FormatEncodeU32('ppan')
#define ppse_map_SIGNATURE        FormatEncodeU32('ppmp')
#define ppse_tileset_SIGNATURE    FormatEncodeU32('pptl')
#define ppse_RAW_ASSETS_SIGNATURE FormatEncodeU32('ppra')
#define ppse_font_SIGNATURE       FormatEncodeU32('pfnt')
#define ppse_coso_SIGNATURE       FormatEncodeU32('pcos')
//editor stuff
#define ppse_composite_resource_SIGNATURE FormatEncodeU32('PPCR')
#define ppse_EDITOR_NAMES_SIGNATURE FormatEncodeU32('PENM')

#define model_editor_data_SIGNATURE FormatEncodeU32('EMOD')
#define ppse_editor_resources_SIGNATURE FormatEncodeU32('prdt')

#define ppse_pack_version    0
#define ppse_map_version     0
#define ppse_tileset_version 1

#define ppse_model_version   1
#define ppse_model_next_version 1

#define ppse_animation_version 3
#define ppse_coso_version 0


#define assets_WHITE_TEXTURE_INDEX 0
#define assets_WHITE_TEXTURE_ID 'WWWW'
#define assets_NULL_ID 0

#define assets_MAX_ASSET_CAPACITY 200

#define assets_MAX_BONE_CAPACITY 10000
#define assets_MAX_TILE_CAPACITY 10000

#define assets_MAX_NAME_LENGTH 256

#define image_DEFAULT_BPP 4


//slot
static asset_slot_data *
assets_load_and_get_image_slot(game_assets *assets, u8 *path_and_name);
//texture directly from slot
static inline render_texture *
assets_load_and_get_image(game_assets *assets, u8 *path_and_name);

static inline u32
asset_type_version(asset_type type)
{
	u32 asset_versions[asset_type_COUNT] = 
	{
		0,

		0,
		0,
		0,

		ppse_map_version,
		ppse_model_version,
		ppse_animation_version,
		ppse_tileset_version,
		ppse_coso_version
	};

	return(asset_versions[type]);
}

static inline u32
asset_type_signature(asset_type type)
{
	u32 asset_signatures[asset_type_COUNT] =
	{
		0,

		0,
		0,
		0,

		ppse_map_SIGNATURE,
		ppse_model_SIGNATURE,
		ppse_animation_SIGNATURE,
		ppse_tileset_SIGNATURE,
		ppse_coso_SIGNATURE
	};
	return(asset_signatures[type]);
}

typedef struct{
	u8 *path_and_name;
    asset_type type;

}game_initial_asset_data;

typedef struct{
	u32 dev_build;
	u32 use_pack;

	u8 *resource_pack_file_name;
	u8 *resource_folder_path;


	u32 texture_request_max;
	u32 total_slots;
	u32 allocation_size;
	u32 operations_size;

	u32 initial_assets_count;
	game_initial_asset_data *initial_assets;

	u32 stop_at_fail;
}game_resource_initial_settings;

typedef struct{
	u32 signature_checked;
	u32 version_checked;
}ppse_signature_and_version_check;

#include "agu_image.c"
#include "agu_png.c"
#include "agu_bmp.c"

static image_data 
parse_png_or_bmp_from_handle(memory_area *area,
                              platform_api *platform,
                              platform_file_handle file_handle,
                              stream_data *infostream)
{
    image_data result = {0};
    platform_file_min_attributes fileInfo = platform->f_get_file_info(file_handle);
	//
    uint8 *fileContents = memory_area_push_size(area, fileInfo.size);
    platform->f_read_from_file(file_handle, 0, fileInfo.size, fileContents);

    stream_buffer buffer = {0};
    buffer.contents      = fileContents; 
    buffer.size          = fileInfo.size;
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
    return(result);
}

static inline void 
assets_fill_data_png_or_bmp_handle(
		memory_area *area,
		platform_api *platform,
		platform_file_handle file_handle,
		u32 *w_ptr,
		u32 *h_ptr,
		u8 *pixels,
		stream_data *info_stream)
{
	temporary_area temp_area = temporary_area_begin(area);
	image_data parsed_pixels = parse_png_or_bmp_from_handle(
			area,
			platform,
			file_handle,
			info_stream);

	memory_copy(
			parsed_pixels.pixels,
			pixels,
			parsed_pixels.width * parsed_pixels.height * parsed_pixels.bpp);
	*w_ptr = parsed_pixels.width;
	*h_ptr = parsed_pixels.height;

	temporary_area_end(&temp_area);

}

inline void
ppse_close_resource_handle(game_assets *assets, ppse_resource_handle resource_handle)
{
	if(!resource_handle.offset_to_name)
	{
		assets->platform->f_close_file(resource_handle.handle);
	}
}

inline resource_check_result
ppse_model_file_check_from_header(ppse_model_header model_file_header)
{
	resource_check_result result = 1;

	u32 signature_check = model_file_header.header.signature == ppse_model_SIGNATURE;
	u32 version_check   = model_file_header.header.version == ppse_model_version;

	if(!signature_check)
	{
		result = file_result_signature_error;
	}
	else if(!version_check)
	{
		result = file_result_version_error;
	}
	return(result);
}

inline resource_check_result
ppse_model_file_check_from_handle(
		platform_api *platform,
		platform_file_handle handle)
{
	ppse_model_header model_file_header = {0};
	platform->f_read_from_file(
			handle,
			0,
			sizeof(ppse_model_header),
			&model_file_header);

	resource_check_result result = ppse_model_file_check_from_header(model_file_header);
	return(result);
}

static void
ppse_fill_font_struct_from_handle(
		                        platform_api *platform,
								platform_file_handle file_handle,
								u32 data_offset,
								font_proportional *font)
{

	//for later use
	u32 signature_check = 1;
	u32 version_check = 1;

	if(signature_check && version_check)
	{
	}
	ppse_font font_header = {0};
	platform->f_read_from_file(file_handle, data_offset, sizeof(font_header), &font_header);
	data_offset += sizeof(font_header);

    u16 pixels_w = font_header.pixelsW;
    u16 pixels_h = font_header.pixelsH;

    u16 glyph_count = font_header.glyph_count;
    u16 font_height = font_header.font_height;

    u32 glyph_table_size         = sizeof(font_glyph) * glyph_count;
    u32 displacements_table_size = sizeof(uint32)	* (glyph_count * glyph_count);

    //font->glyphs                  = memory_area_push_size(area, glyph_table_size);
    //font->horizontal_displacements = memory_area_push_size(area, displacements_table_size); 
	font->font_height              = font_height;
	font->glyph_count              = glyph_count;

    platform->f_read_from_file(file_handle, data_offset, glyph_table_size, font->glyphs);
    platform->f_read_from_file(file_handle, data_offset + glyph_table_size, displacements_table_size, font->horizontal_displacements);

}

static ppse_asset_header
ppse_read_common_header_handle(
		platform_api *platform,
		platform_file_handle file)
{
	ppse_asset_header common_header = {0};
	platform->f_read_from_file(
			file,
			0,
			sizeof(common_header),
			&common_header);
	return(common_header);
}



#define assets_get_white_texture(assets) \
	(&assets_get_white_texture_slot(assets)->image)
inline asset_slot_data * 
assets_get_white_texture_slot(game_assets *game_asset_manager)
{
	asset_slot_data *white_texture_slot = game_asset_manager->assets + assets_WHITE_TEXTURE_INDEX;

	return(white_texture_slot);
}

inline platform_entire_file
assets_read_entire_file_from_resource_handle(game_assets *game_asset_manager,
		                                     ppse_resource_handle resource_handle)
{
	platform_api *platform = game_asset_manager->platform;
	u32 data_size          = resource_handle.data.data_size;
	u32 data_offset        = resource_handle.data.data_offset;

	//to return
	platform_entire_file entire_file = {0};

	//push on operations area
	entire_file.contents = memory_area_push_size(
			&game_asset_manager->area,
			data_size);

	//read the entire file contents from package or disk
    platform->f_read_from_file(
			resource_handle.handle,
			data_offset,
			data_size,
			entire_file.contents);

	entire_file.size = data_size;

	return(entire_file);
}

inline asset_memory_operation
assets_data_allocation_begin(game_assets *game_asset_manager,
		                     u32 needed_size,
							 u32 asset_index)
{
	stream_data *info_stream = &game_asset_manager->info_stream;

	asset_memory_header *first_header = &game_asset_manager->first_asset_memory_header;
	asset_memory_header *current_memory_header = first_header->next_header;

	//the total needed size including the memory header
	u32 total_needed_size = needed_size + sizeof(asset_memory_header);

	u8 *memory_at_result = 0;

	//No headers are allocated and this is the first one
	if(!game_asset_manager->asset_memory_used)
	{
		asset_memory_header *new_header = (asset_memory_header *)game_asset_manager->asset_memory;
		first_header->next_header = new_header;
		new_header->previous_header = first_header;
		//zero initialize.
		memory_zero_struct(new_header, asset_memory_header);

		//make sure the asset manager has enough space.

	    u32 space_between = 
			(u32)((game_asset_manager->asset_memory + game_asset_manager->asset_memory_max) - (u8 *)new_header);

	    // ;unfinished
	    Assert(space_between + sizeof(asset_memory_header) > needed_size);

		//set at the end of the header.
		memory_at_result = (game_asset_manager->asset_memory + 
				            game_asset_manager->asset_memory_used + 
							sizeof(asset_memory_header));

		game_asset_manager->asset_memory_used += total_needed_size;

		Assert(game_asset_manager->asset_memory_max > game_asset_manager->asset_memory_used);

		stream_pushf(
				info_stream,
				"the first memory header for asset data is beign allocated with a total space of %d",
				space_between);

	}
	else //look for enough space to store the next asset
	{
		u32 found_space = 0;
	    while(!found_space)
	    {
	    	asset_memory_header *next_memory_header =
				current_memory_header->next_header;

			//start reading from the end of this header and the memory its occupiing
	    	u8  *current_memory_header_end = ((u8 *)(current_memory_header + 1) + 
	    				              current_memory_header->used);

			//there is another header after this one
	    	if(next_memory_header)
	    	{
	    	    u32 space_between = (u32)((u8 *)next_memory_header - current_memory_header_end);

	    		//The needed size has to count the size of the header.
	    		if((space_between + sizeof(asset_memory_header)) > needed_size)
	    		{
					//found needed size!
	    			//push new header
	    			asset_memory_header *new_header =
						(asset_memory_header *)current_memory_header_end;
					//initialize to zero
					memory_zero_struct(new_header, asset_memory_header);

					//set the "previous" and "next" header for the new allocated header.
	    			current_memory_header->next_header = new_header;

					new_header->previous_header = current_memory_header;
					new_header->next_header     = next_memory_header;

	    			memory_at_result = (u8 *)(new_header + 1);
					stream_pushf(info_stream, "found space between memory headers to allocate an asset! the space between is %d", space_between);

					found_space = 1;

	    		}
	    	}
	    	else //no header after the current one, so allocate a new one.
	    	{

				//push new header at the end of the current header
	    		asset_memory_header *new_header = 
					(asset_memory_header *)current_memory_header_end;
				//initialize to zero
				memory_zero_struct(new_header, asset_memory_header);

				//set new next header.
				current_memory_header->next_header = new_header;

				new_header->previous_header = current_memory_header;

				// make sure enough space is avadible.
	    		u32 space_between = 
					(u32)((game_asset_manager->asset_memory + game_asset_manager->asset_memory_max)
							- current_memory_header_end);

	    		// ;unfinished
	    		Assert(space_between + sizeof(asset_memory_header) > needed_size);

				//set the base of the next header
				memory_at_result = (game_asset_manager->asset_memory + 
						            game_asset_manager->asset_memory_used +
									sizeof(asset_memory_header));

				game_asset_manager->asset_memory_used += total_needed_size;

				Assert(game_asset_manager->asset_memory_max > game_asset_manager->asset_memory_used);

				stream_pushf(info_stream, "Allocating new header with a needed space of %d and a remaining of %d",
						     total_needed_size,
							 space_between);

				found_space = 1;

	    	}
	    	current_memory_header = next_memory_header;
	    }
	}


	asset_memory_operation result = {0};
	result.working_asset_index = asset_index;

	memory_chunk *operation_chunk = &result.chunk;
	operation_chunk->size = needed_size;
	operation_chunk->base = memory_at_result;
	operation_chunk->used = 0;

	//clear the needed memory to zero.
	memory_clear(memory_at_result, needed_size);
	return(result);
}

//TODO:Make a cancel funcion in case of not correctly loading the asset
inline void
assets_data_allocation_end(asset_memory_operation *operation)
{
	//get header at the start of the asset data
    asset_memory_header *current_memory_header = 
		(asset_memory_header *)(operation->chunk.base - sizeof(asset_memory_header));
	memory_size actual_used_size = operation->chunk.size - operation->chunk.used;

	current_memory_header->used = operation->chunk.size;
	current_memory_header->asset_index = operation->working_asset_index;

	operation->chunk.size = 0;
	operation->chunk.used = 0;
	operation->chunk.base = 0;
}


static void
assets_unload_slot(
		game_assets *game_asset_manager,
		asset_slot_data *unloading_slot)
{

	u32 freed_space = 0;
	stream_data *info_stream = &game_asset_manager->info_stream;

	unloading_slot->state = asset_state_free;

	//free memory header
	if(unloading_slot->allocated_memory_header)
	{
		//get the current memory header and it's previous and next header
		asset_memory_header *previous_header = unloading_slot->allocated_memory_header->previous_header;
		asset_memory_header *next_header = unloading_slot->allocated_memory_header->next_header;

		//set correct pointing headers
		previous_header->next_header = next_header;
		//in case this is the last slot
		if(next_header)
		{
		    next_header->previous_header = previous_header;
		}
		
		freed_space = unloading_slot->allocated_memory_header->used + sizeof(asset_memory_header);
	}
	//set new free slot if it's not pointing to the last avadible one

	unloading_slot->next = game_asset_manager->first_free_slot;
	game_asset_manager->first_free_slot = unloading_slot;
	//decrease count if this is the last slot

	stream_pushf(
			info_stream,
			"Deallocated asset %d and freed space %d",
			unloading_slot->index,
			freed_space);

}

static inline void
assets_unload_asset_by_index(game_assets *game_asset_manager,
		                     u32 asset_index)
{


	asset_slot_data *asset_slot_array = game_asset_manager->assets;
	asset_slot_data *unloading_slot = asset_slot_array + asset_index;
	assets_unload_slot(game_asset_manager, unloading_slot);

}

static inline void
assets_unload_slot_by_ptr(
		game_assets *assets,
		void *asset)
{
}

inline void 
assets_fill_image_data_from_handle(game_assets *game_asset_manager,
		               platform_file_handle file_handle,
					   u32 data_offset,
					   u32 *w_ptr,
					   u32 *h_ptr,
					   u8 *pixels)
{

	platform_api *platform = game_asset_manager->platform;

    u32 signature = 0;
	platform->f_read_from_file(file_handle,
			                      data_offset,
								  sizeof(u32),
								  &signature);
	u32 w = 0;
	u32 h = 0;

	//the file is an "easy image" so read directly
	if(signature == ppse_image_SIGNATURE)
	{

	     ppse_image ppse_image_header = {0};
		 platform->f_read_from_file(file_handle,
				                       data_offset,
									   sizeof(ppse_image),
									   &ppse_image_header);

		 //advance offset to the location of the pixels
		 data_offset += sizeof(ppse_image);

		 w = ppse_image_header.w;
		 h = ppse_image_header.h;

		 //op area
		 u32 image_size = w * h * ppse_image_header.bpp;

		 platform->f_read_from_file(
				 file_handle,
				 data_offset,
				 image_size,
				 pixels);

		 *w_ptr = w;
		 *h_ptr = h;

	}
	else
	{
		//the file is either a png or bmp file, so parse.
        assets_fill_data_png_or_bmp_handle(
				&game_asset_manager->area,
				game_asset_manager->platform,
        		file_handle,
        		w_ptr,
				h_ptr,
				pixels,
        		0);
		//result = parse_png_or_bmp_from_handle(
		//		&game_asset_manager->area,
		//		game_asset_manager->platform,
		//		file_handle,
		//		0);
	}
}

inline u32
ppse_check_image_signature_from_handle(platform_api *platform,
		                               platform_file_handle file_handle,
									   u32 data_offset)
{

	//detect if it is a custom image file, bmp or png
    u64 signature = 0;
	platform->f_read_from_file(file_handle,
			                      data_offset,
								  sizeof(u64),
								  &signature);
	u32 is_valid_image = 0;

	if((u32)signature == ppse_image_SIGNATURE)
	{
		is_valid_image = 1;
	}
	else if((u16)signature == 'MB') //BM or 4d42
    {
		is_valid_image = 1;
    }
    else if(png_CheckSignature_u64(signature))
    {
		is_valid_image = 1;
    }

	return(is_valid_image);
}

inline u32
ppse_check_image_signature_from_path(
		platform_api *platform,
		u8 *path_and_name)
{
	platform_file_handle file_handle = platform->f_open_file(path_and_name, platform_file_op_read | platform_file_op_share);
	u32 is_valid_image = 0;
	if(file_handle.handle)
	{
	    is_valid_image = ppse_check_image_signature_from_handle(
				platform,
				file_handle,
				0);
	}
	return(is_valid_image);
}

static u32
ppse_convert_and_save_image_to_pimg_handle(memory_area *area,
		                                   platform_api *platform,
									       platform_file_handle source_file_handle,
									       u8 *path_and_name)
{

	u32 success = 0;
	platform_file_handle new_image_file = platform->f_open_file(path_and_name, platform_file_op_create_new);

	if(new_image_file.handle)
	{
	    temporary_area temporary_image_area = temporary_area_begin(area);
        //get source file data and close it
        image_data sourceImage = parse_png_or_bmp_from_handle(area,
	    		                                                platform,
	    														source_file_handle,
	    														0);

        u32 sourceImageSize    = sourceImage.width * sourceImage.height * sourceImage.bpp;

        u32 fileCompatible = sourceImage.pixels != 0;
        if(fileCompatible)
        {
             //Compare source data with file data
             //Assert(sourceImage.width  <= 512);
             //Assert(sourceImage.height <= 512);

	         //Set header data
	         ppse_image *assetImageData = memory_area_push_struct(area, ppse_image);
	    	 assetImageData->signature  = ppse_image_SIGNATURE;
	         assetImageData->w          = sourceImage.width;
	         assetImageData->h          = sourceImage.height;
	         assetImageData->bpp        = sourceImage.bpp;
			 //allocate after image header to read this data linearly
	         u8 *pixels                 = memory_area_push_and_copy(area, sourceImage.pixels, sourceImageSize);
	         u32 total_image_file_size  = sizeof(ppse_image) + sourceImageSize;

			 platform->f_write_to_file(new_image_file,
					                      0,
										  sourceImageSize,
										  (u8 *)assetImageData);

			 success = 1;
        }
		platform->f_close_file(new_image_file);
	    temporary_area_end(&temporary_image_area);
	}

}

inline void
assets_get_type_names(u8 *assetTypes[asset_type_COUNT])
{
  assetTypes[0] = "Image";
  assetTypes[1] = "Font";
  assetTypes[2] = "Sound";
  assetTypes[3] = "Map";
  assetTypes[4] = "Model";
  assetTypes[5] = "Animation";
  assetTypes[6] = "Tileset";
}


static u32
assets_generate_id(u8 *text)
{
	u8 textToLow[256] = {0};
	string_copy(text, textToLow);
    string_to_low(textToLow);
	u32 newId = string_kinda_hash(textToLow);

	return(newId);
}

inline u8 *
assets_create_image_file(memory_area *area, u32 image_w, u32 image_h, u8 *pixels)
{
	ppse_image *assetImageData = memory_area_push_struct(area, ppse_image);
	assetImageData->w      = image_w;
	assetImageData->h      = image_h;
	assetImageData->bpp    = image_DEFAULT_BPP;

	u32 imageSize = image_w * image_h * image_DEFAULT_BPP;
	u8 *storedPixels = memory_area_push_and_copy(area, pixels, imageSize);

	return((u8 *)assetImageData);

}


inline void
ppse_update_header(game_assets *game_asset_manager, u32 assetIndex, u8 *newFileName)
{

	//platform_file_handle assetHandle = game_asset_manager->assetFile.handle;
	//asset_slot_data *assetData = game_asset_manager->assets + assetIndex;

    //u32 path_and_nameLength = string_count(newFileName);
    ////The new path name is longer
    //if(path_and_nameLength != sourceNameLength)
    //{
    //	//allocate a new header at the end of the file
    //	platform_file_min_attributes assetFileInfo = platform->f_get_file_info(assetHandle);
    //
    //	//Used later to find and get rid of
    //	ppse_asset_data invalidFileData = {0};
    //	invalidFileData.type = asset_state_invalid;
    //	invalidFileData.id = ASSET_ID_INVALID;
    //	//Write an invalid file header on the old space
    //	platform->f_write_to_file(assetHandle, offsetToFileData, sizeof(ppse_asset_data), &invalidFileData);
    //
    //	u32 newDataOffset = assetFileInfo.size;
    //	//set new offset to name and header
    //	assetData->offsetToName = newDataOffset;
    //	//write new file name at the end of the asset file
    //	platform->f_write_to_file(assetHandle, newDataOffset, sizeof(u32), &path_and_nameLength);
    //	newDataOffset += sizeof(u32);
    //	platform->f_write_to_file(assetHandle, newDataOffset, path_and_nameLength, newFileName);
    //	newDataOffset += path_and_nameLength; 
    //	platform->f_write_to_file(assetHandle, newDataOffset, sizeof(ppse_asset_data), &fileData);
    //
    //}
    //else
    //{
    //	//just reeplace the name
    //	u32 newNameOffset = offsetToName;
    //	platform->f_write_to_file(assetHandle, offsetToName, sizeof(u32), &path_and_nameLength);
    //	offsetToName += sizeof(u32);
    //	platform->f_write_to_file(assetHandle, offsetToName, path_and_nameLength, newFileName);
    //	offsetToName += path_and_nameLength;
    //
    //}
}


inline u32
ppse_check_font_signature_from_handle(
		platform_api *platform,
		platform_file_handle file_handle)
{
	NotImplemented;
	return(0);
}

inline u32
assets_animation_check_signature(platform_api *platform,
		                         platform_file_handle file_handle)
{
	u32 signature = 0;
	platform->f_read_from_file(file_handle, 0, sizeof(u32), &signature);

	return(signature == ppse_animation_SIGNATURE);
}

inline u32
assets_map_check_signature(platform_api *platform,
		                   platform_file_handle file_handle)
{
	u32 signature = 0;
	platform->f_read_from_file(file_handle, 0, sizeof(u32), &signature);

	return(signature == ppse_map_SIGNATURE);
}

inline u32
assets_tileset_check_signature(platform_api *platform,
		platform_file_handle file_handle)
{
	u32 signature = 0;
	platform->f_read_from_file(file_handle, 0, sizeof(u32), &signature);
	return(signature == ppse_tileset_SIGNATURE);
}

inline u32
ppse_tileset_open_and_check_signature_source(platform_api *platform,
		                                     u8 *path_and_name)
{
	u32 success = 0;
	platform_file_handle file_handle = platform->f_open_file(path_and_name, platform_file_op_read | platform_file_op_share);
	if(file_handle.handle)
	{
        success = assets_tileset_check_signature(platform, file_handle);

		platform->f_close_file(file_handle);
	}

	return(success);
}

inline u32
assets_model_check_signature(platform_api *platform, platform_file_handle file_handle)
{
	u32 signature = 0;
	platform->f_read_from_file(file_handle, 0, sizeof(u32), &signature);
	return(signature == ppse_model_SIGNATURE);
}

//static void
//assets_file_work_put_data(game_assets *game_asset_manager,
//		                  asset_file_work *asset_work,
//						  u32 offset,
//						  u32 size,
//						  u8 *data)
//{
//	platform_api *platform           = game_asset_manager->platform;
//	ppse_file_header *ppseHeader     = &game_asset_manager->assetFile.header;
//	platform_file_handle assetHandle = game_asset_manager->assetFile.handle;
//
//	u32 current_file_headers_offset = ppseHeader->offset_to_file_headers;
//
//	//if the offset to the new data goes beyond the headers offset
//	if((offset + size) > current_file_headers_offset)
//	{
//		u32 newDataAmount = (offset + size) - current_file_headers_offset;
//		ppseHeader->offset_to_file_headers += newDataAmount;
//		asset_work->totalDataAllocated   += newDataAmount;
//	}
//    u32 bytes_written = platform->f_write_to_file(assetHandle, offset, size, data);
//    Assert(bytes_written);
//
//	//mark this file as updated
//	asset_work->updatedFile = 1;
//}
//
//
//static void
//assets_file_work_create_or_reeplace_file(game_assets *game_asset_manager,
//		                  asset_file_work *asset_work,
//						  u32 size,
//						  u8 *data,
//						  u8 *file_name
//						  )
//{
//
//
//	//get and update the pack header, and count the new asset
//	ppse_file_header *ppseHeader = &game_asset_manager->assetFile.header;
//
//    //platform_file_handle assetHandle = game_asset_manager->assetFile.handle;
//	asset_pack_header_info *asset_header_for_packing = 0;
//
//	u32 asset_data_offset = ppseHeader->offset_to_file_headers;
//	u32 asset_size        = size;
//	u32 asset_id          = assets_generate_id(file_name);;
//
//	//look for existing id
//	u32 asset_exists = 0;
//	for(u32 h = 0;
//			h < asset_work->asset_header_count;
//			h++)
//	{
//		asset_pack_header_info *current_header = asset_work->asset_headers + h;
//		if(current_header->data.id == asset_id)
//		{
//			asset_exists    = 1;
//			//set the existing header
//			asset_header_for_packing = current_header;
//		}
//
//	}
//	//check is size equals
//	if(asset_exists)
//	{
//		//rewrite the data on the same place, else write to the end
//		if(size < asset_header_for_packing->data.data_size)
//		{
//			asset_data_offset = asset_header_for_packing->data.data_offset;
//		}
//	}
//	else
//	{
//	    asset_header_for_packing = (asset_work->asset_headers +
//		                            asset_work->asset_header_count);
//        string_copy(file_name, asset_header_for_packing->path_and_name);
//	    ppseHeader->asset_count++;
//	    asset_work->asset_header_count++;
//	}
//
//	//set data needed to write to the pack file
//	asset_header_for_packing->nameLength    = string_count(file_name);
//	asset_header_for_packing->data.id       = asset_id;
//	asset_header_for_packing->data.data_size = asset_size;
//	asset_header_for_packing->data.data_offset = asset_data_offset;
//
//
//	//write all of the data
//	assets_file_work_put_data(game_asset_manager,
//			                  asset_work,
//							  asset_data_offset,
//							  asset_size,
//							  data);
//
//    Assert(ppseHeader->asset_count < assets_MAX_ASSET_CAPACITY);
//    //u32 bytesWritten = platform->f_write_to_file(assetHandle, offsetToData, sourceImageSize, sourceImage.pixels);
//
//}
//
//static void
//asset_file_work_put_existing_file(game_assets *game_asset_manager,
//		                          asset_file_work *asset_work,
//								  platform_file_handle file_handle,
//						          u8 *file_name)
//{
//	temporary_area temporary_file_area = temporary_area_begin(&game_asset_manager->area);
//
//	//file size and data
//	platform_entire_file entire_file_data = platform_read_entire_file_handle(game_asset_manager->platform,
//			                                                                 file_handle,
//																			 &game_asset_manager->area);
//
//    assets_file_work_create_or_reeplace_file(game_asset_manager,
//    		                                 asset_work,
//    						                 entire_file_data.size,
//    						                 entire_file_data.contents,
//    						                 file_name
//    						                 );
//
//	temporary_area_end(&temporary_file_area);
//}
//
//static void
//assets_file_work_rewrite(game_assets *game_asset_manager,
//		                 asset_file_work *asset_work,
//						 u32 assetId,
//						 u32 size,
//						 u8 *data,
//						 u8 *newName)
//{
//	u32 asset_header_count = asset_work->asset_header_count;
//	asset_pack_header_info *asset_headers = asset_work->asset_headers;
//	asset_pack_header_info *assetHeaderInfo = 0;
//
//	//get index to the existing asset
//	for(u32 a = 0; a < asset_header_count; a++)
//	{
//		if(asset_headers[a].data.id == assetId)
//		{
//			assetHeaderInfo = asset_headers + a;
//			break;
//		}
//	}
//	Assert(assetHeaderInfo);
//
//	u32 offsetToData = assetHeaderInfo->data.data_offset;
//	if(size > assetHeaderInfo->data.data_size)
//	{
//		//Write this to the end
//		offsetToData = game_asset_manager->assetFile.header.offset_to_file_headers;
//        assetHeaderInfo->data.data_offset = offsetToData;
//	}
//	assetHeaderInfo->data.data_size = size;
//	assetHeaderInfo->data.id       = assets_generate_id(newName);
//	assetHeaderInfo->nameLength    = string_count(newName);
//	//update name
//	string_copy(newName, assetHeaderInfo->path_and_name);
//	
//	assets_file_work_put_data(game_asset_manager, asset_work, offsetToData, size, data);
//
//}
//
////static void
////asset_file_work_update_file_name()
//
//static asset_file_work
//assets_begin_asset_work(game_assets *game_asset_manager)
//{
//
//	u32 asset_count_in_file = game_asset_manager->assetFile.header.asset_count;
//
//	asset_file_work asset_work = {0};
//	asset_work.asset_headers = memory_area_push_array(&game_asset_manager->area,
//			                                          asset_pack_header_info,
//													  assets_MAX_ASSET_CAPACITY);
//	asset_work.assetFile     = game_asset_manager->assetFile;
//
//
//	//Get headers
//	u32 headerOffset     = game_asset_manager->assetFile.header.offset_to_file_headers;
//
//	asset_work.asset_header_count = asset_count_in_file;
//	//read all of the current asset headers in the pack file
//	for(u32 h = 0;
//			h < asset_count_in_file;
//			h++)
//	{
//		asset_pack_header_info *assetHeader = asset_work.asset_headers + h;
//
//		u32 *nameLength = &assetHeader->nameLength;
//		u8 *nameBuffer  = assetHeader->path_and_name;
//		ppse_asset_data *header_in_file = &assetHeader->data; 
//		//name length
//		game_asset_manager->platform->f_read_from_file(game_asset_manager->assetFile.handle,
//				                           headerOffset,
//										   sizeof(u32),
//										   nameLength);
//		headerOffset += sizeof(u32);
//		//name
//		game_asset_manager->platform->f_read_from_file(game_asset_manager->assetFile.handle,
//				                           headerOffset,
//										   *nameLength,
//										   nameBuffer);
//		headerOffset += (*nameLength);
//		//header
//		game_asset_manager->platform->f_read_from_file(game_asset_manager->assetFile.handle,
//				                           headerOffset,
//										   sizeof(ppse_asset_data),
//										   header_in_file);
//		headerOffset += sizeof(ppse_asset_data);
//
//	}
//	return(asset_work);
//
//}
//
//static void 
//assets_EndAssetWork(game_assets *game_asset_manager, asset_file_work asset_work)
//{
//	u32 fileGotUpdated = (asset_work.updatedFile);
//	if(fileGotUpdated)
//	{
//		asset_file *assetFile = &game_asset_manager->assetFile;
//		//assetFile->header.offset_to_file_headers += asset_work.totalDataAllocated;
//		u32 headerOffset = assetFile->header.offset_to_file_headers;
//
//		//Write ALL headers at the end of the file
//		for(u32 h = 0; h < asset_work.asset_header_count; h++)
//		{
//		    asset_pack_header_info *assetHeader = asset_work.asset_headers + h;
//
//		    u32 *nameLength           = &assetHeader->nameLength;
//		    u8 *nameBuffer            = assetHeader->path_and_name;
//		    ppse_asset_data *fileData = &assetHeader->data; 
//
//			//name length before name
//		    game_asset_manager->platform->f_write_to_file(assetFile->handle,
//					                          headerOffset,
//											  sizeof(u32),
//											  nameLength);
//			headerOffset += sizeof(u32);
//		    game_asset_manager->platform->f_write_to_file(assetFile->handle,
//					                          headerOffset,
//											  *nameLength,
//											  nameBuffer);
//			headerOffset += *nameLength;
//			//all header data with offset, id ,date and size
//		    game_asset_manager->platform->f_write_to_file(assetFile->handle,
//					                          headerOffset,
//											  sizeof(ppse_asset_data),
//											  fileData);
//			headerOffset += sizeof(ppse_asset_data);
//		}
//
//
//		//update the already loaded assets to point to the updated header locations
//		for(u32 a = 0; a < game_asset_manager->asset_count ; a++)
//		{
//			if(game_asset_manager->assets[a].source == asset_source_packed)
//			{
//			    game_asset_manager->assets[a].offsetToName += asset_work.totalDataAllocated;
//			}
//		}
//		//add asset slots for the new files
//
//
//		//assetFile->header.asset_count = asset_work.previousAssetsCount;
//		game_asset_manager->platform->f_write_to_file(assetFile->handle, 0, sizeof(ppse_file_header), &assetFile->header);
//	}
//
//}

static void
assets_fill_entity_from_memory(
		asset_entity *entity,
		u8 *memory,
		stream_data *info_stream)
{
	resource_reader re = resource_reader_begin(memory);
	//	ppse_coso_header header = *resource_reader_get_struct(&re, ppse_coso_header);
	ppse_coso_header *header = resource_reader_get_struct(&re, ppse_coso_header);

	//get to stats
	resource_reader_next_line(&re);
	ppse_coso_stats *file_stats = resource_reader_get_struct(&re, ppse_coso_stats);
	entity->speed = file_stats->speed;
	entity->z_speed = file_stats->z_speed;
	entity->speed_max = file_stats->speed_max;
	entity->collision_size.x = file_stats->collision_x;
	entity->collision_size.y = file_stats->collision_y;
	entity->collision_size.z = file_stats->collision_z;
	entity->collision_offset.x = file_stats->collision_offset_x;
	entity->collision_offset.y = file_stats->collision_offset_y;
	entity->collision_offset.z = file_stats->collision_offset_z;
	//get to states
	resource_reader_next_line(&re);
	state_main *coso_states = &entity->states;
	coso_states->state_count = header->state_count;

	u32 loaded_lines = 0;
	u32 loaded_conditions = 0;
	for(u32 s = 0; s < header->state_count; s++)
	{
		ppse_coso_state *file_state = resource_reader_get_struct(&re, ppse_coso_state);
		state_node *state = entity->states.states + s;
		state->state_do_at = loaded_lines;
		loaded_lines += file_state->line_count;
		state->state_line_count = file_state->line_count;
		//load lines
		for(u32 l = 0; l < file_state->line_count; l++)
		{
			//get line
			ppse_coso_state_line *file_state_line = 
				resource_reader_get_struct(&re, ppse_coso_state_line);
			state_line *line = 
				coso_states->state_lines + state->state_do_at + l;
			line->type = file_state_line->flags;
			line->action_index = file_state_line->action_index;
			line->state_index = file_state_line->state_index;
			//set correct conditions pointing values
			line->trigger_count = file_state_line->condition_count;
			line->triggers_at = loaded_conditions;
			loaded_conditions += line->trigger_count;
			//load line conditions
			for(u32 c = 0; c < line->trigger_count; c++)
			{
				//get condition
				ppse_coso_state_condition *file_condition = resource_reader_get_struct(&re, ppse_coso_state_condition);
				state_trigger *condition = coso_states->triggers + line->triggers_at + c;
				condition->type = file_condition->type;
				condition->not = file_condition->not;
				condition->eq = (u8)file_condition->eq;
				condition->radius = file_condition->radius;
			}
		}
	}
	//get to actions
	resource_reader_next_line(&re);
	u32 loaded_actions = 0;
	for(u32 a = 0; a < header->action_count; a++)
	{
		ppse_coso_action *file_action = resource_reader_get_struct(&re, ppse_coso_action);
		state_action *action = coso_states->actions + a;
		action->action_lines_count = file_action->line_count;
		action->action_lines_at = loaded_actions;
		loaded_actions += action->action_lines_count;
		for(u32 l = 0; l < file_action->line_count; l++)
		{
			ppse_coso_action_line *file_action_line = resource_reader_get_struct(&re, ppse_coso_action_line);
			state_action_line *action_line = coso_states->action_lines + action->action_lines_at + l;
			action_line->target_index = file_action_line->target_index;
			action_line->animation_index = file_action_line->animation_index;
			action_line->time_total = file_action_line->time_total;
		}
	}
}





static void
assets_fill_tileset_from_memory(
		world_tileset *tileset,
		u8 *memory,
		stream_data *info_stream)
{
	resource_reader re = resource_reader_begin(memory);
	//header
	ppse_tileset_header_new *tileset_header = resource_reader_get_struct(&re, ppse_tileset_header_new);
	//get data
	u32 terrain_count = tileset_header->terrain_count;
	u32 autoterrain_count = tileset_header->autoterrain_count;
	u32 autoterrain_indices_count = tileset_header->autoterrain_indices_capacity;
	tileset->terrain_count = terrain_count;
	tileset->autoterrain_count = autoterrain_count;
	tileset->autoterrain_indices_count = autoterrain_indices_count;
	tileset->uvs_count = tileset_header->mesh_count;
	tileset->wall_count = tileset_header->wall_count;

	resource_reader_next_line(&re);
	u32 loaded_uvs_count = 0;
	//terrain_line
	resource_reader_get_to_line(&re, tileset_header->line_to_terrain_data);
	{
		for(u32 t = 0;
				t < terrain_count;
				t++)
		{
			ppse_tileset_terrain *file_tileset_terrain = resource_reader_get_struct(&re, ppse_tileset_terrain);

			stream_pushf(info_stream,
					"Loading terrain with %u meshes and %u shape",
					file_tileset_terrain->mesh_count,
					file_tileset_terrain->shape);

			s_tileset_terrain *ts_terrain = tileset->terrain + t;
			//fill data
			ts_terrain->shape = file_tileset_terrain->shape;
			ts_terrain->mesh_count = file_tileset_terrain->mesh_count;
			ts_terrain->uvs_at_vertices_at = loaded_uvs_count;
			ts_terrain->terrain_group = file_tileset_terrain->terrain_group;
			ts_terrain->use_wall = (b8)file_tileset_terrain->use_wall;
			ts_terrain->wall_index = file_tileset_terrain->wall_index;
			ts_terrain->capacity = file_tileset_terrain->capacity;
			//Currently, I'm just using one
			for(u32 m = 0;
					m < ts_terrain->mesh_count;
					m++)
			{
				//get mesh for tileset
				ppse_mesh *loaded_mesh = resource_reader_get_struct(&re, ppse_mesh);
				model_mesh *terrain_mesh = tileset->meshes + loaded_uvs_count + m;
				*terrain_mesh = loaded_mesh->m;
				ts_terrain->uv0 = terrain_mesh->uv0;
				ts_terrain->uv1 = terrain_mesh->uv1;
				ts_terrain->uv2 = terrain_mesh->uv2;
				ts_terrain->uv3 = terrain_mesh->uv3;
			}
			loaded_uvs_count += ts_terrain->mesh_count;
		}
	}
	//auto terrain line
	resource_reader_next_line(&re);
	{
		//read autoterrains
		u16 *indices_array = tileset->autoterrain_indices;
		u16 loaded_indices_count = 0;
		for(u32 a = 0;
				a < autoterrain_count;
				a++)
		{

			ppse_tileset_autoterrain *loaded_autoterrain = resource_reader_get_struct(&re, ppse_tileset_autoterrain);
			//add new autoterrain
			tileset_autoterrain *new_autoterrain = 
				tileset->autoterrains + a;
			//fill data
			new_autoterrain->extra_layers = loaded_autoterrain->extra_layers;
			new_autoterrain->terrain_group = loaded_autoterrain->terrain_group;
			new_autoterrain->capacity = loaded_autoterrain->capacity;
			new_autoterrain->indices = indices_array + loaded_indices_count;
			stream_pushf(info_stream, "Loaded autoterrain with capacity %u", new_autoterrain->capacity);
			//advance to indices

			stream_pushf(info_stream, "--%u. Loading autoterrain indices", new_autoterrain->capacity);
			for(i32 l = 0; l < new_autoterrain->extra_layers + 1; l++)
			{
				for(u32 i = 0;
						i < new_autoterrain->capacity;
						i++)
				{
					u16 index = *resource_reader_get_struct(&re, u16);
					stream_pushf(info_stream, "got %u index", index);
					new_autoterrain->indices[i + (l * new_autoterrain->capacity)] = index;
					loaded_indices_count++;
				}
			}
		}
	}
	//load tileset walls
	resource_reader_next_line(&re);
	{
		for(u32 w = 0; w < tileset->wall_count; w++)
		{
			ppse_tileset_wall *file_wall = resource_reader_get_struct(&re, ppse_tileset_wall);
			tileset_wall *wall = tileset->walls + w;
			wall->uvs_at = loaded_uvs_count;
			wall->uvs_count = file_wall->uvs_count;
			loaded_uvs_count += wall->uvs_count;
			wall->repeat = file_wall->repeat;
			wall->extra_frames_x = file_wall->extra_frames_x;
			wall->extra_frames_y = file_wall->extra_frames_y;
			//load wall uvs
			for(u32 u = 0; u < wall->uvs_count; u++)
			{
				ppse_tileset_wall_uvs *file_uvs = resource_reader_get_struct(&re, ppse_tileset_wall_uvs);
				model_mesh *terrain_mesh = tileset->meshes + wall->uvs_at + u;
				terrain_mesh->uv0 = file_uvs->uv0;
				terrain_mesh->uv1 = file_uvs->uv1;
				terrain_mesh->uv2 = file_uvs->uv2;
				terrain_mesh->uv3 = file_uvs->uv3;
				
			}
		}
	}

}


inline i32
assets_get_index_from_id(game_assets *game_asset_manager, u32 assetId)
{
	i32 assetExists = 0;
	for(i32 i = 0; i < game_asset_manager->asset_count; i++)
	{
	    if(assetId == game_asset_manager->assets[i].id)
	    {
	 	   assetExists = i;
	 	   break;
	    }

	}

	return(assetExists);
}

inline ppse_offset_and_asset_data
assets_get_packed_file_header_by_id(game_assets *game_asset_manager,
		                            u32 resource_id)
{
	platform_api *platform = game_asset_manager->platform;

	//initial data
	ppse_offset_and_asset_data asset_offset_and_header_data = {0};

	//get header and pack file handle
	ppse_file_header resources_file_header     = game_asset_manager->assetFile.header;
	platform_file_handle resources_file_handle = game_asset_manager->assetFile.handle;
	u32 resources_in_file_count = resources_file_header.asset_count;
	u32 file_offset             = resources_file_header.offset_to_file_headers;
	u32 current_offset_to_name  = file_offset;

	u32 c = 0;
	while(c < resources_in_file_count)
	{
	    ppse_asset_data current_header_data = {0};
	    u32 file_name_length   = 0;
		current_offset_to_name = file_offset;
        
	    //get file name length
        platform->f_read_from_file(resources_file_handle,
	    		                      file_offset,
	    					          sizeof(u32),
	    					          &file_name_length);
		//skip name
	    file_offset += sizeof(u32);
	    file_offset += file_name_length; 

	    //get file header
        platform->f_read_from_file(resources_file_handle,
	    		                      file_offset, 
	    					          sizeof(ppse_asset_data),
	    					          &current_header_data);
        file_offset += sizeof(ppse_asset_data);

		if(current_header_data.id == resource_id)
		{
		    asset_offset_and_header_data.data           = current_header_data;
            asset_offset_and_header_data.offset_to_name = current_offset_to_name;
			break;
		}
	}

	return(asset_offset_and_header_data);
}



static ppse_asset_data
assets_get_asset_header_from_pack( game_assets *game_asset_manager,
		                           platform_api *platform,
								   u32 index)
{
    ppse_asset_data asset = {0};

	asset_slot_data *assetData = game_asset_manager->assets + index;
	Assert(assetData->source == asset_source_packed);

    platform_file_handle assetFileHandle = game_asset_manager->assetFile.handle;
    uint32 fileOffset = assetData->offsetToName;
    
	uint32 fileNameLength = 0;
    platform->f_read_from_file(assetFileHandle, fileOffset ,sizeof(uint32),&fileNameLength);
	fileOffset += sizeof(uint32);
	Assert(fileNameLength < 64);

	uint8 fileNameBuffer[64] = {0};
    platform->f_read_from_file(assetFileHandle, fileOffset , fileNameLength, fileNameBuffer);
	fileOffset += fileNameLength; 

    platform->f_read_from_file(assetFileHandle, fileOffset ,sizeof(ppse_asset_data),&asset);
    fileOffset += sizeof(ppse_asset_data);


    return(asset);
}

static ppse_asset_data
assets_LoadAssetInfoAndName(game_assets *assets,
		                    platform_api *platform, uint32 id)
{
    ppse_asset_data asset = {0};
    platform_file_handle assetFileHandle = assets->assetFile.handle;
    uint32 fileOffset					 = assets->assetFile.header.offset_to_file_headers; 
    
    for(uint32 assetI = 0;
        assetI < assets->assetFile.header.asset_count;
        assetI++)
    {
		uint32 fileNameLength = 0;
        platform->f_read_from_file(assetFileHandle, fileOffset ,sizeof(uint32),&fileNameLength);
		fileOffset += sizeof(uint32);
		Assert(fileNameLength < 64);

		uint8 fileNameBuffer[64] = {0};
        platform->f_read_from_file(assetFileHandle, fileOffset , fileNameLength, fileNameBuffer);
		fileOffset += fileNameLength; 

        platform->f_read_from_file(assetFileHandle, fileOffset ,sizeof(ppse_asset_data),&asset);
        fileOffset += sizeof(ppse_asset_data);
        if(asset.id == id)
        {
            break;
        }
    }
    return(asset);
}

static void
assets_LoadFromFile(game_assets *assets, platform_api *platform, u8 *filePathAndName)
{
	platform_file_handle file_handle = platform->f_open_file(filePathAndName, platform_file_op_read | platform_file_op_share | platform_file_op_write);
	u32 invalidFile = file_handle.handle != 0;
	if(file_handle.handle)
	{
		//Load
        file_path_name_and_extension_info  fileInfo = path_get_file_path_info(filePathAndName);
		u32 isImage = string_compare(fileInfo.extension, "png") || string_compare(fileInfo.extension, "bmp");
		if(isImage)
		{
		}
		else
		{
			invalidFile = 1;
		}
		platform->f_close_file(file_handle);
	}

	if(invalidFile)
	{
		//Log
	}
}

static asset_slot_data *
assets_get_asset_by_index(game_assets *assets, u32 asset_index)
{
	asset_slot_data *result = 0;
	if(asset_index < assets->asset_count)
	{
		result = assets->assets + asset_index;
	}
   return(result);
}

static asset_slot_data *
assets_find_asset_by_id(game_assets *assets, uint32 id)
{
	//Look for asset by id
   u32 asset_count     = assets->asset_count;
   asset_slot_data *result = 0;
   for(u32 assetI = 0; assetI < asset_count; assetI++)
   {
       if(assets->assets[assetI].id == id)
       {
		   result = assets->assets + assetI;
		   break;
       }
   }
   return(result);
}

static void
assets_allocate_from_image_file(
		game_assets *assets,
		render_texture *texture,
		ppse_resource_handle resource_handle)
{
#if 0
	//in case of supporting larger textures, I must detect if it's going
	//to occuppy more that one slot, where in such case I have to
	//look for two consecutive free slots instead
	platform_api *platform = assets->platform;
   asset_slot_data *assetData = assets->assets + asset_t_request->asset_index; 

   texture_request *new_request;
   if(asset_t_request->flags & request_reload)
   {
	   u32 texture_slot = asset_t_request->flags & request_font ?
		   assetData->font.texture.index : assetData->image.index;
	   new_request = render_texture_request_start(
			   game_renderer,
			   render_use_texture_slot(game_renderer, texture_slot));
   }
   else
   {
	   new_request = render_texture_request_start(
			   game_renderer,
			   render_get_texture_slot(game_renderer));
   }

   ppse_resource_handle resource_handle = asset_t_request->resource_handle;
   texture_request_flags request_flags = asset_t_request->flags;

   u32 imageW    = 0; 
   u32 imageH    = 0;
   u32 image_size = 0;

   //Note(Agu): temporary pixel allocation

   if(request_flags & request_white_texture)
   {
	   //For now, just allocate a white texture
	   imageW = game_renderer->texture_array_w;
	   imageH = game_renderer->texture_array_h;

       image_size = imageW * imageH * 4;
	   u32 image_size_32 = imageW * imageH;

	   u32 *pixels32 = (u32 *)new_request->pixels;
       for(u32 i = 0; i < image_size_32; i++)
       {
          pixels32[i] = 0xffffffff;
       }
	   new_request->state = request_ready_to_transfer;
   }
   else
   {
	   platform_file_handle file_data_handle = resource_handle.handle;
	   u32 data_offset = resource_handle.data.data_offset;

	   Assert(file_data_handle.handle);

	   if(request_flags & request_default)
	   {
		   image_data file_image_data = {0};
		   file_image_data.pixels = new_request->pixels;

		   assets_fill_image_from_handle(assets,
				   file_data_handle,
				   data_offset,
				   &file_image_data);
		   //get pixels and total size
		   u8 *pixels = file_image_data.pixels;

           imageW = file_image_data.width;
           imageH = file_image_data.height;

           image_size  = imageW * imageH * file_image_data.bpp;

	       assetData->image.width = imageW;
	       assetData->image.height = imageH;
		   assetData->image.index = new_request->avadible_texture_index;

	   }
	   else
       {
		   //font
		   ppse_font font_header = {0};
		   platform->f_read_from_file(file_data_handle,
				                         data_offset,
										 sizeof(ppse_font),
										 &font_header);
		   //get the offset to the font pixels
           uint32 glyph_count             = font_header.glyph_count;
           uint32 glyph_table_size         = sizeof(font_glyph) * glyph_count;
           uint32 displacements_table_size = sizeof(uint32) * (glyph_count * glyph_count);

	       imageW = font_header.pixelsW;
           imageH = font_header.pixelsH;
           data_offset += sizeof(ppse_font) + glyph_table_size + displacements_table_size;
           image_size  = imageW * imageH * 4;

		   Assert(imageW <= 512 && imageH <= 512);
		   //read the font pixels
		   assetData->font.texture.index = new_request->avadible_texture_index;
		   assetData->font.texture.width = imageW;
		   assetData->font.texture.height = imageW;
           platform->f_read_from_file(file_data_handle,
	    		                         data_offset,
	    								 image_size,
	    								 new_request->pixels);
       }
   }
   new_request->state = request_ready_to_transfer;


   assetData->state = asset_state_loaded;

   assets->texture_request_count--;

   if(!assets->using_package)
   {
	   assets->platform->f_close_file(resource_handle.handle);
   }
#endif
}
/*
   start_texture_request()
   {
   }
   read_from_file(pixels, request);
*/



static inline asset_slot_data *
assets_get_slot(
		game_assets *game_asset_manager,
		void *resource)
{
	u8 *resource_8  = (u8 *)resource;
	asset_slot_data *slot_data = 0;
	//check if resource is is range
	if(resource_8 >= (u8 *)game_asset_manager->assets && 
		resource_8 < (u8 *)(game_asset_manager->assets + game_asset_manager->asset_max))
	{
		slot_data = (asset_slot_data *)resource;
	}
	return(slot_data);
}

inline asset_slot_data *
assets_get_new_slot(game_assets *game_asset_manager)
{

	asset_slot_data *result = game_asset_manager->first_free_slot;

	//found a recycled slot
	if(result)
	{
		game_asset_manager->first_free_slot = result->next;
		result->next = 0;
	}
	else
	{
		//use a new slot.
	    result = game_asset_manager->assets + game_asset_manager->asset_count;
	    result->index = game_asset_manager->asset_count;
        game_asset_manager->asset_count++;
		// ;assert for now
		Assert(game_asset_manager->asset_count < game_asset_manager->asset_max);
	}

	result->state = asset_state_data_unloaded;
	result->version = 0;
	// ;clean
	Assert(result);

	return(result);

}

static inline void
assets_unload_slot_data(
		game_assets *game_asset_manager,
		asset_slot_data *slot)
{
	if(slot->allocated_memory_header)
	{
		slot->state = asset_state_data_unloaded;
		//get the current memory header and it's previous and next header
		asset_memory_header *previous_header = slot->allocated_memory_header->previous_header;
		asset_memory_header *next_header     = slot->allocated_memory_header->next_header;

		//set correct pointing headers
		previous_header->next_header = next_header;
		//in case this is the last slot
		if(next_header)
		{
		    next_header->previous_header = previous_header;
		}
		
		//freed_space = slot->allocated_memory_header->used + sizeof(asset_memory_header);
		slot->allocated_memory_header = 0;
	}
}


static inline b32
assets_free_slot(
		game_assets *game_asset_manager,
		asset_slot_data *unloading_slot)
{

	u32 freed_space = 0;
	stream_data *info_stream = &game_asset_manager->info_stream;

	unloading_slot->state = asset_state_free;

	//free memory header
	if(unloading_slot->allocated_memory_header)
	{
		//get the current memory header and it's previous and next header
		asset_memory_header *previous_header = unloading_slot->allocated_memory_header->previous_header;
		asset_memory_header *next_header     = unloading_slot->allocated_memory_header->next_header;

		//set correct pointing headers
		previous_header->next_header = next_header;
		//in case this is the last slot
		if(next_header)
		{
		    next_header->previous_header = previous_header;
		}
		
		freed_space = unloading_slot->allocated_memory_header->used + sizeof(asset_memory_header);
	}
		unloading_slot->next = game_asset_manager->first_free_slot;
		game_asset_manager->first_free_slot = unloading_slot;

	stream_pushf(
			info_stream,
			"Deallocated asset %d and freed space %d",
			unloading_slot->index,
			freed_space);

	return(freed_space);
}


inline ppse_resource_handle
assets_get_resource_handle_from_pack_or_disk(game_assets *game_asset_manager,
		                                 u8 *file_name)
{
	ppse_resource_handle resource_handle = {0};
	platform_api *platform               = game_asset_manager->platform;

	//assets are packed
	if(game_asset_manager->using_package)
	{
		u32 file_id = assets_generate_id(file_name);
		//get data from id
        ppse_offset_and_asset_data offset_and_data = assets_get_packed_file_header_by_id(game_asset_manager,
		                                                                                 file_id);
		u32 file_offset_to_header       = offset_and_data.offset_to_name;
		ppse_asset_data file_asset_data = offset_and_data.data;

		//the file exists inside the pack
		if(file_offset_to_header)
		{
			//set the location of the file inside the pack file
			resource_handle.offset_to_name = file_offset_to_header;
			resource_handle.data           = file_asset_data;
			resource_handle.handle         = game_asset_manager->assetFile.handle;
		}
		else
		{
			//log
		}


	}
	else
	{
		//read from disk
		platform_file_handle file_source_handle = platform->f_open_file(file_name, 
				                                                        platform_file_op_read | platform_file_op_share);
		if(file_source_handle.handle)
		{
			//size, date
			platform_file_min_attributes disk_file_info = platform->f_get_file_info(file_source_handle);

			resource_handle.handle         = file_source_handle;
			resource_handle.data.id        = assets_generate_id(file_name);
			resource_handle.data.data_size = disk_file_info.size;
		}
		else
		{
			//log
		}
	}
	return(resource_handle);
}

inline asset_file_info
assets_get_file_info(game_assets *assets,
		             u8 *path_and_name)
{
	// ;finish?
    ppse_resource_handle resource_handle = assets_get_resource_handle_from_pack_or_disk(
			assets,
			path_and_name);

	asset_file_info info_result = {0};

	if(resource_handle.handle.handle)
	{
		string_copy(path_and_name, info_result.path_and_name);
		info_result.data = resource_handle.data;

		if(!assets->using_package)
		{
			assets->platform->f_close_file(resource_handle.handle);
		}
	}
	return(info_result);
}

inline platform_entire_file
assets_read_file_from_pack_or_disk(game_assets *game_asset_manager,
		                          platform_file_handle handle,
								  u32 data_offset,
								  u32 data_size)
{
	platform_api *platform           = game_asset_manager->platform;
	platform_entire_file entire_file = {0};


	entire_file.size = data_size;

	entire_file.contents = memory_area_push_size(
			&game_asset_manager->area,
			data_size);

	//read the entire file contents
	platform->f_read_from_file(handle,
			                      data_offset,
								  data_size,
								  entire_file.contents);

	return(entire_file);
	
}

inline platform_entire_file
assets_read_entire_file_from_pack_or_disk(game_assets *game_asset_manager,
		                                  u8 *file_name)
{
    platform_entire_file entire_file = {0};
	platform_api *platform           = game_asset_manager->platform;

	if(game_asset_manager->using_package)
	{
		u32 file_id = assets_generate_id(file_name);
		//get data from id
        ppse_offset_and_asset_data offset_and_data = assets_get_packed_file_header_by_id(game_asset_manager,
		                                                                                 file_id);
		u32 file_offset_to_header = offset_and_data.offset_to_name;
		ppse_asset_data file_asset_data = offset_and_data.data;

		if(file_offset_to_header)
		{
		    platform_file_handle pack_file_handle = game_asset_manager->assetFile.handle;

		    //allocate entire file memory and fill data
		    entire_file.contents = memory_area_push_size(&game_asset_manager->area, file_asset_data.data_size);
		    entire_file.size     = offset_and_data.data.data_size;

			//read the entire file contents
			platform->f_read_from_file(pack_file_handle,
					                      file_asset_data.data_offset,
										  file_asset_data.data_size,
										  entire_file.contents);
		}


	}
	else
	{
		//read from disk
		platform_file_handle file_source_handle = platform->f_open_file(file_name, 
				                                                           platform_file_op_read | platform_file_op_share);
		if(file_source_handle.handle)
		{
			entire_file = platform_read_entire_file_handle(platform, 
					                                       file_source_handle,
														   &game_asset_manager->area);
			platform->f_close_file(file_source_handle);
		}
		else
		{
			//log
		}
	}

	return(entire_file);
}



static game_world *
assets_get_map_by_name(game_assets *assets, u8 *name)
{
	u32 name_id = assets_generate_id(name);
    asset_slot_data *world_slot = assets_find_asset_by_id(assets, name_id);

	game_world *map = 0;

	//slot found!
	if(world_slot)
	{
	    //map = &world_slot->world;
	}
	NotImplemented;

	return(map);
}


static render_texture * 
assets_get_texture_by_index(game_assets *assets, u32 index)
{
    asset_slot_data *assetData = assets_get_asset_by_index(assets, index);
    render_texture *result = {0};

	if(!assetData)
	{
		assetData = assets->assets + assets_WHITE_TEXTURE_INDEX;
	}
	result = &assetData->image;
    return(result);

}
//TODO(Agu): Fill the correct image to the asset when passing it!
static render_texture * 
assets_get_texture_by_id(game_assets *assets, uint32 id)
{
    asset_slot_data *assetData = assets_find_asset_by_id(assets, id);
    render_texture *result = 0;

	if(assetData)
	{
        result = assets_get_texture_by_index(assets, assetData->index);
	}
	else
	{
		assetData = assets->assets + assets_WHITE_TEXTURE_INDEX;
		//return white image
		result = &assetData->image;
	}
    
    return(result);
}


static asset_slot_data *
assets_load_and_get_image_slot(
		game_assets *assets,
		u8 *path_and_name)
{
	stream_data *info_stream = &assets->info_stream;
	u32 image_id = assets_generate_id(path_and_name);

    asset_slot_data *image_slot_data = 
		assets_find_asset_by_id(assets, image_id);

	//image was already stored and loaded
	//else allocate new slot
	if(!image_slot_data)
	{
		platform_api *platform = assets->platform;
		//in case of failure return white texture on main build
//		image_slot_data = assets_get_white_texture_slot(assets);
		
		ppse_resource_handle image_file_handle_and_header = assets_get_resource_handle_from_pack_or_disk(
				assets,
		        path_and_name);
		//image file got loaded
		if(image_file_handle_and_header.handle.handle)
		{
			//images don't have version
			u32 signature_check = ppse_check_image_signature_from_handle(
					assets->platform,
					image_file_handle_and_header.handle,
					image_file_handle_and_header.data.data_offset);
			u32 is_from_disk = assets->using_package;

			if(signature_check)
			{
				image_slot_data = assets_get_new_slot(assets);


				//request and fill texture for the gpu
				texture_request *new_request = render_texture_request_start(
						assets->texture_operations,
						render_get_texture_slot(assets->texture_operations));

				assets_fill_image_data_from_handle(assets,
						image_file_handle_and_header.handle,
						image_file_handle_and_header.data.data_offset,
						(u32 *)&image_slot_data->image.width,
						(u32 *)&image_slot_data->image.height,
						new_request->pixels);
				//put render texture here?
				image_slot_data->image.index = new_request->avadible_texture_index;

				//=========


				image_slot_data->id = image_id;
				image_slot_data->state = asset_state_Requested;
				image_slot_data->type = asset_type_image;
				//while the image is on request, return a white texture
				//render_texture then has to ask again for the same image
				//to get the correct data
				if(is_from_disk)
				{
				    image_slot_data->source = asset_source_from_disk;
				}
				else
				{
				    image_slot_data->source = asset_source_packed;
				}

			}
			else
			{
				stream_pushf(info_stream, "Error while loading %s, the signature check failed!", path_and_name);

				//get rid of the handle
			}
		}
		else
		{
			stream_pushf(info_stream, "Error while loading %s, the file was not found!", path_and_name);
		}

		if(!assets->using_package)
		{
			platform->f_close_file(image_file_handle_and_header.handle);
		}
	}
    
    return(image_slot_data);
}

static inline render_texture *
assets_load_and_get_image(game_assets *assets, u8 *path_and_name)
{
	asset_slot_data *slot = assets_load_and_get_image_slot(assets, path_and_name);
	return(slot ? &slot->image : 0);
}

#define assets_load_and_get_font(assets, path_and_name) (&assets_load_and_get_font_slot(assets, path_and_name)->font)
static asset_slot_data * 
assets_load_and_get_font_slot(
		game_assets *assets,
		u8 *font_path_and_name)
{
	u32 font_id = assets_generate_id(font_path_and_name);
    asset_slot_data *asset_slot = assets_find_asset_by_id(assets, font_id);

	platform_api *platform = assets->platform;
	//this was already allocated
	if(!asset_slot)
	{
		//get font from pack or disk
		ppse_resource_handle font_resource_handle = assets_get_resource_handle_from_pack_or_disk(assets,
		                                                                                     font_path_and_name);

		platform_file_handle font_file_handle = font_resource_handle.handle;
		ppse_asset_data font_file_header      = font_resource_handle.data;

		//get avadible asset slot
		asset_slot = assets_get_new_slot(assets);

		asset_slot->offsetToName = font_resource_handle.offset_to_name;
		asset_slot->state = asset_state_loaded;
		asset_slot->id = font_id;

		//get font's file header
	    ppse_font font_header = {0};
	    platform->f_read_from_file(
				font_file_handle,
				font_file_header.data_offset,
				sizeof(ppse_font),
				&font_header);

        u16 glyph_count = font_header.glyph_count;
        u16 font_height = font_header.font_height;

        u32 glyph_table_size         = sizeof(font_glyph) * glyph_count;
        u32 displacements_table_size = sizeof(uint32)	* (glyph_count * glyph_count);

		asset_memory_operation memory_operation = assets_data_allocation_begin(
				assets,
				(glyph_table_size + displacements_table_size),
				asset_slot->index);

		asset_slot->font.glyphs = memory_chunk_push_array(
				&memory_operation.chunk,
				font_glyph,
				glyph_count);

		asset_slot->font.horizontal_displacements = memory_chunk_push_array(
				&memory_operation.chunk,
				i32,
				glyph_count * glyph_count);

		assets_data_allocation_end(&memory_operation);
		//data offset is 0 if this comes from disk
        ppse_fill_font_struct_from_handle(assets->platform,
								        font_resource_handle.handle,
								        font_resource_handle.data.data_offset,
								        &asset_slot->font);

		texture_request *new_request = render_texture_request_start(
				assets->texture_operations,
				render_get_texture_slot(assets->texture_operations));
		//request and fill textures
	       u32 imageW = font_header.pixelsW;
           u32 imageH = font_header.pixelsH;
           u32 image_size = imageW * imageH * 4;

		   Assert(imageW <= 512 && imageH <= 512);
		   //read the font pixels
		   asset_slot->font.texture.index = new_request->avadible_texture_index;
		   asset_slot->font.texture.width = imageW;
		   asset_slot->font.texture.height = imageW;
           platform->f_read_from_file(font_resource_handle.handle,
				   font_resource_handle.data.data_offset + font_header.offset_to_pixels,
				   image_size,
				   new_request->pixels);
	}
	return(asset_slot);
}

static resource_check_result 
assets_model_check_header(
		game_assets *assets,
		ppse_asset_header header)
{
	u32 signature_check = header.signature == ppse_model_SIGNATURE;
	u32 version_check = header.version == ppse_model_version;

	resource_check_result check_result = file_result_success;
	if(!signature_check)
	{
		check_result = file_result_signature_error;
	}
	else if(!version_check)
	{
		check_result = file_result_version_error;
	}
	return(check_result);
}

static inline resource_check_result 
assets_model_check_handle(
		game_assets *assets,
		platform_file_handle file_handle)
{
	platform_api *platform = assets->platform;

	ppse_asset_header asset_header = {0};
	platform->f_read_from_file(
			file_handle,
			0,
			sizeof(ppse_asset_header),
			&asset_header);

	resource_check_result check_result = assets_model_check_header(
			assets, asset_header);

	return(check_result);
}

static resource_check_result 
assets_tileset_check_header(
		game_assets *assets,
		ppse_asset_header asset_header)
{
	u32 signature_check = asset_header.signature == ppse_tileset_SIGNATURE;
	u32 version_check = asset_header.version   == ppse_tileset_version;

	resource_check_result check_result = file_result_success;
	if(!signature_check)
	{
		check_result = file_result_signature_error;
	}
	else if(!version_check)
	{
		check_result = file_result_version_error;
	}
	return(check_result);
}

static inline resource_check_result 
assets_tileset_check_handle(
		game_assets *assets,
		platform_file_handle file_handle)
{
	platform_api *platform = assets->platform;

	ppse_asset_header asset_header = {0};
	platform->f_read_from_file(
			file_handle,
			0,
			sizeof(ppse_asset_header),
			&asset_header);

	resource_check_result check_result = assets_tileset_check_header(
			assets, asset_header);

	return(check_result);
}

static asset_slot_data *
assets_load_and_get_tileset_slot(game_assets *assets, u8 *path_and_name);
static inline world_tileset *
assets_load_and_get_tileset(game_assets *assets, u8 *path_and_name);

static asset_slot_data *
assets_load_and_get_model_slot(game_assets *assets, u8 *path_and_name);
static inline model *
assets_load_and_get_model(game_assets *assets, u8 *path_and_name);

static inline asset_slot_data *
assets_load_and_get_entity_slot(game_assets *assets, u8 *path_and_name);
static inline asset_entity *
assets_load_and_get_entity(game_assets *assets, u8 *path_and_name);

static inline asset_slot_data *
assets_load_and_get_frame_animation_slot(game_assets *assets, u8 *path_and_name);
static inline asset_frame_animation *
assets_load_and_get_frame_animation(game_assets *assets, u8 *path_and_name);


static inline b32 
assets_allocate_tileset(game_assets *assets,
		asset_slot_data *asset_data,
		u8 *path_and_name)
{
#if 0

	b32 success = 0;
	stream_data *info_stream = &assets->info_stream;

	platform_api *platform = assets->platform;

	//get file from te pack file or disk
	ppse_resource_handle resource_handle = assets_get_resource_handle_from_pack_or_disk(
			assets,
			path_and_name);

	temporary_area temporary_entire_file_area = temporary_area_begin(
			&assets->area);

	platform_entire_file entire_tileset_file = assets_read_entire_file_from_pack_or_disk(
			assets,
			path_and_name);

	//close
	ppse_close_resource_handle(assets, resource_handle);
	//If this fails. This will stay unloaded but not free.
	asset_data->state = asset_state_data_unloaded;

	//file exists!
	if(entire_tileset_file.contents)
	{
		//get header to check for signature and version
		ppse_tileset_header tileset_header = *(ppse_tileset_header *)entire_tileset_file.contents;

		u32 signature_check = tileset_header.header.signature == ppse_tileset_SIGNATURE;
		u32 version_check = tileset_header.header.version   == ppse_tileset_version;

		if(signature_check && version_check)
		{
			//load dependencies
			//get image source path and length
			u32 image_source_offset = tileset_header.offset_to_image_source;
			ppse_composite_resource *tileset_image_path = (ppse_composite_resource *)(entire_tileset_file.contents + image_source_offset);

			//get the tileset image path and make sure it's loaded to fully use the tileset

			render_texture *tileset_image = assets_load_and_get_image(
					assets,
					tileset_image_path->path_and_name);

			//image was found so proceed
			if(tileset_image && tileset_image->index != 0)
			{
				//get avadible asset slot
				success = 1;

				//fill data
				asset_data->offsetToName = resource_handle.offset_to_name;
				asset_data->state = asset_state_loaded;
				asset_data->id = assets_generate_id(path_and_name);
				asset_data->type = asset_type_tileset;
				asset_data->version = tileset_header.header.version;

				//calculate the needed allocation size for this asset
				u32 terrain_count = tileset_header.terrain_count;
				u32 mesh_count = tileset_header.mesh_count;
				u32 autoterrain_count = tileset_header.autoterrain_count;
				u32 autoterrain_indices_count = tileset_header.autoterrain_indices_capacity;

				u32 tileset_needed_allocation_size = 
					(terrain_count * sizeof(s_tileset_terrain)) +
					(mesh_count * sizeof(model_mesh)) +
					(autoterrain_count * sizeof(tileset_autoterrain)) +
					(autoterrain_indices_count * sizeof(u16));
				//start operation to allocate the tiles and animated tiles
				asset_memory_operation memory_operation = assets_data_allocation_begin(
						assets,
						tileset_needed_allocation_size,
						asset_data->index);

				asset_data->tileset.terrain = memory_chunk_push_array(
						&memory_operation.chunk,
						s_tileset_terrain,
						terrain_count);
				asset_data->tileset.meshes = memory_chunk_push_array(
						&memory_operation.chunk,
						model_mesh,
						mesh_count);
				asset_data->tileset.autoterrains = memory_chunk_push_array(
						&memory_operation.chunk,
						tileset_autoterrain,
						autoterrain_count);
				asset_data->tileset.autoterrain_indices = memory_chunk_push_array(
						&memory_operation.chunk,
						u16,
						autoterrain_indices_count);
				asset_data->tileset.image = tileset_image;

				assets_fill_tileset_from_memory(
						&asset_data->tileset,
						entire_tileset_file.contents,
						&assets->info_stream);

				assets_data_allocation_end(&memory_operation);
			}
			else
			{
				stream_pushf(
						info_stream,
						"The tileset file \"%s\" could not be loaded because the image file \"%s\" was not found!",
						path_and_name,
						tileset_image_path->path_and_name);
			}

		}
		else
		{
			if(!signature_check)
			{
				stream_pushf(info_stream, "Error while loading the tileset file %s. Signature check failed!", path_and_name);
			}
			else
			{
				stream_pushf(info_stream, "Error while loading the tileset file %s. The version check failed! "
						"got %d but expected %d",
						path_and_name,
						tileset_header.header.version,
						ppse_tileset_version);
			}
		}

	} //<- if file found
	else
	{
		stream_pushf(info_stream, "Error while loading the tileset file %s it was not found!", path_and_name);
	}

	temporary_area_end(&temporary_entire_file_area);

	return(success);
#else

	b32 success = 0;
	stream_data *info_stream = &assets->info_stream;

	platform_api *platform = assets->platform;

	//get file from te pack file or disk
	ppse_resource_handle resource_handle = assets_get_resource_handle_from_pack_or_disk(
			assets,
			path_and_name);

	temporary_area temporary_entire_file_area = temporary_area_begin(
			&assets->area);

	platform_entire_file entire_tileset_file = assets_read_entire_file_from_pack_or_disk(
			assets,
			path_and_name);

	//close
	ppse_close_resource_handle(assets, resource_handle);
	//If this fails. This will stay unloaded but not free.
	asset_data->state = asset_state_data_unloaded;

	//file exists!
	if(entire_tileset_file.contents)
	{
		//get header to check for signature and version
		ppse_tileset_header_new tileset_header = *(ppse_tileset_header_new *)entire_tileset_file.contents;

		u32 signature_check = tileset_header.header.signature == ppse_tileset_SIGNATURE;
		u32 version_check = tileset_header.header.version   == ppse_tileset_version;

		if(signature_check && version_check)
		{
			//load dependencies
			//get image source path and length
			ppse_composite_resource *tileset_image_path = (ppse_composite_resource *)(entire_tileset_file.contents + tileset_header.header.offset_to_composite_resources);

			//get the tileset image path and make sure it's loaded to fully use the tileset

			render_texture *tileset_image = assets_load_and_get_image(
					assets,
					tileset_image_path->path_and_name);

			//image was found so proceed
			if(tileset_image && tileset_image->index != 0)
			{
				//get avadible asset slot
				success = 1;

				//fill data
				asset_data->offsetToName = resource_handle.offset_to_name;
				asset_data->state = asset_state_loaded;
				asset_data->id = assets_generate_id(path_and_name);
				asset_data->type = asset_type_tileset;
				asset_data->version = tileset_header.header.version;

				//calculate the needed allocation size for this asset
				u32 terrain_count = tileset_header.terrain_count;
				u32 mesh_count = tileset_header.mesh_count;
				u32 autoterrain_count = tileset_header.autoterrain_count;
				u32 autoterrain_indices_count = tileset_header.autoterrain_indices_capacity;
				u32 wall_count = tileset_header.wall_count;

				u32 tileset_needed_allocation_size = 
					(terrain_count * sizeof(s_tileset_terrain)) +
					(mesh_count * sizeof(model_mesh)) +
					(autoterrain_count * sizeof(tileset_autoterrain)) +
					(autoterrain_indices_count * sizeof(u16)) +
					(wall_count * sizeof(tileset_wall));
				//start operation to allocate the tiles and animated tiles
				asset_memory_operation memory_operation = assets_data_allocation_begin(
						assets,
						tileset_needed_allocation_size,
						asset_data->index);

				asset_data->tileset.terrain = memory_chunk_push_array(
						&memory_operation.chunk,
						s_tileset_terrain,
						terrain_count);
				asset_data->tileset.meshes = memory_chunk_push_array(
						&memory_operation.chunk,
						model_mesh,
						mesh_count);
				asset_data->tileset.autoterrains = memory_chunk_push_array(
						&memory_operation.chunk,
						tileset_autoterrain,
						autoterrain_count);
				asset_data->tileset.autoterrain_indices = memory_chunk_push_array(
						&memory_operation.chunk,
						u16,
						autoterrain_indices_count);
				asset_data->tileset.walls = memory_chunk_push_array(
						&memory_operation.chunk, tileset_wall, wall_count);
				asset_data->tileset.image = tileset_image;

				assets_fill_tileset_from_memory(
						&asset_data->tileset,
						entire_tileset_file.contents,
						&assets->info_stream);

				assets_data_allocation_end(&memory_operation);
			}
			else
			{
				stream_pushf(
						info_stream,
						"The tileset file \"%s\" could not be loaded because the image file \"%s\" was not found!",
						path_and_name,
						tileset_image_path->path_and_name);
			}

		}
		else
		{
			if(!signature_check)
			{
				stream_pushf(info_stream, "Error while loading the tileset file %s. Signature check failed!", path_and_name);
			}
			else
			{
				stream_pushf(info_stream, "Error while loading the tileset file %s. The version check failed! "
						"got %d but expected %d",
						path_and_name,
						tileset_header.header.version,
						ppse_tileset_version);
			}
		}

	} //<- if file found
	else
	{
		stream_pushf(info_stream, "Error while loading the tileset file %s it was not found!", path_and_name);
	}

	temporary_area_end(&temporary_entire_file_area);

	return(success);
#endif
}

static inline b32
assets_allocate_map(game_assets *assets,
		asset_slot_data *asset_data,
		u8 *path_and_name)
{
	b32 success = 0;
	stream_data *info_stream = &assets->info_stream;
	asset_data->id = assets_generate_id(path_and_name);
	asset_data->type = asset_type_map;

	asset_data->state = asset_state_loaded;

	asset_map_data *map = &asset_data->map;
	platform_api *platform = assets->platform;
	//for log
	//get header data
	temporary_area temp_file_area = temporary_area_begin(&assets->area);
	{
		platform_entire_file entire_map_file = assets_read_entire_file_from_pack_or_disk(
				assets,
				path_and_name);

		resource_reader re = resource_reader_begin(entire_map_file.contents);

		ppse_map_file_header *header = resource_reader_get_struct(&re, ppse_map_file_header);
		//get to stats
		u32 signature_check = header->header.signature == ppse_map_SIGNATURE;
		u32 version_check = header->header.version == ppse_map_version;
		if(!signature_check || !version_check)
		{
			if(!signature_check)
			{
				stream_pushf(
						info_stream,
						"Error while loading the file \"%s\" as a map file! Signature check error",
						path_and_name);
			}
			else
			{
				stream_pushf(
						info_stream,
						"Error while loading the map file \"%s\"! Version check error, got %d but expected %d",
						path_and_name,
						header->header.version,
						ppse_map_version);
			}
			temporary_area_end(&temp_file_area);
			return(success);;
		}

		//let's do this baby!
		//Load data
		u32 terrain_count = header->map_terrain_count;
		u32 collider_count = header->map_terrain_count;
		u32 entity_count = header->entity_count;
		u32 tileset_count = header->tileset_count;
		u32 model_count = header->model_count;

		u32 total_needed_size = (
				tileset_count * sizeof(world_tileset *) +
				terrain_count * sizeof(world_tile) +
				entity_count * sizeof(map_entity_data) + 
				terrain_count * sizeof(world_collider) +
				model_count * sizeof(model *));
		//allocate memory needed for arrays
		asset_memory_operation memory_operation = assets_data_allocation_begin(
				assets,
				total_needed_size,
				asset_data->index);

		//allocate memory needed
		map->tilesets_a = memory_chunk_push_array(
				&memory_operation.chunk,
				world_tileset*,
				tileset_count);

		map->tiles= memory_chunk_push_array(
				&memory_operation.chunk,
				world_tile,
				terrain_count);

		map->entities = memory_chunk_push_array(
				&memory_operation.chunk,
				map_entity_data,
				entity_count);

		map->colliders = memory_chunk_push_array(
				&memory_operation.chunk,
				world_collider,
				terrain_count);
		map->models = memory_chunk_push_array(
				&memory_operation.chunk,
				model *,
				model_count);


		assets_data_allocation_end(&memory_operation);

		success = 1;
		//set data
		map->tile_count = terrain_count;
		map->colliders_count = collider_count;
		map->entity_count = entity_count;
		map->tileset_count= tileset_count;
		map->map_w = header->map_w;
		map->map_h = header->map_h;


		//get to terrain array
		resource_reader_get_to_line(&re, header->line_to_terrain_data);
		{
			world_tile *terrain_array = map->tiles;
			//skip "tileset array"
			for(u32 t = 0;
					t < map->tile_count;
					t++)
			{
				world_tile *e_t = map->tiles + t;
				ppse_world_tile *file_tile = resource_reader_get_struct(&re, ppse_world_tile);
				//read tile data

				e_t->tileset_index = file_tile->tileset_index;
				e_t->tileset_terrain_index = file_tile->tileset_terrain_index;
				e_t->height = file_tile->height;
				e_t->is_autoterrain = file_tile->is_autoterrain;
				e_t->autoterrain_index = file_tile->autoterrain_index;
			}
		}
		//entity line
		resource_reader_next_line(&re);
		{
			for(u32 e = 0; e < header->entity_count; e++)
			{
				ppse_map_entity *file_ent = resource_reader_get_struct(&re, ppse_map_entity); 
				map_entity_data *entity = map->entities + e;
				//load data
				entity->position = file_ent->position;
				entity->type = file_ent->type;
				entity->model_index = file_ent->model_index;
				entity->entity_index = file_ent->entity_index;
			}
		}

		resource_reader_get_to_line(&re, header->line_to_tileset_paths);


		for(u32 t = 0;
				t < tileset_count;
				t++)
		{
			ppse_composite_resource *tileset_source = resource_reader_get_struct(&re, ppse_composite_resource);

			world_tileset *loaded_tileset = assets_load_and_get_tileset(
					assets,
					tileset_source->path_and_name);

			map->tilesets_a[t] = loaded_tileset;
			if(loaded_tileset)
			{
				stream_pushf(
						info_stream,
						"The external tileset file \"%s\" got successfuly loaded!", 
						tileset_source->path_and_name);
			}
			else
			{
				stream_pushf(
						info_stream,
						"The external tileset file \"%s\" could not be loaded.",
						tileset_source->path_and_name);
			}

		}
		for(u32 m = 0;
				m < model_count;
				m++)
		{
			ppse_composite_resource *source = resource_reader_get_struct(&re, ppse_composite_resource);

			model *loaded = assets_load_and_get_model(
					assets,
					source->path_and_name);

			map->models[m] = loaded;
			if(loaded)
			{
				stream_pushf(
						info_stream,
						"The external model file \"%s\" got successfuly loaded!", 
						source->path_and_name);
			}
			else
			{
				stream_pushf(
						info_stream,
						"The external model file \"%s\" could not be loaded.",
						source->path_and_name);
			}

		}

	}
	temporary_area_end(&temp_file_area);
	return(success);
}


static b32
assets_allocate_model(
		game_assets *assets,
		asset_slot_data *asset_data,
		u8 *path_and_name)
{

	stream_data *info_stream = &assets->info_stream;
	platform_api *platform = assets->platform;
	b32 success = 0;
	temporary_area temporary_model_area = temporary_area_begin(&assets->area);
	{
		asset_data->id = assets_generate_id(path_and_name);
		asset_data->type = asset_type_model;

		platform_entire_file model_entire_file = assets_read_entire_file_from_pack_or_disk(
				assets,
				path_and_name);
		resource_reader re = resource_reader_begin(model_entire_file.contents);
		//file got readed and allocated!
		if(model_entire_file.contents)
		{
			ppse_model_header_new *model_header = resource_reader_get_struct(&re, ppse_model_header_new);//*(ppse_model_header *)model_entire_file.contents;

			u32 signature_check = model_header->header.signature == ppse_model_SIGNATURE;
			u32 version_check = (model_header->header.version == ppse_model_version) ||
			 (model_header->header.version == ppse_model_next_version);

			if(signature_check && version_check)
			{
				asset_data->state = asset_state_loaded;
				success = 1;
				//calculate needed size
				u32 bone_count = model_header->bone_count;
				u32 sprite_count = model_header->sprite_count;
				u32 sheets_count = model_header->sprite_sheet_count;
				u32 uvs_count = model_header->uvs_count;
				u32 animations_count = model_header->animation_count;
				u32 keyframe_count = model_header->keyframe_count;
				u32 frame_keyframe_count = model_header->frame_keyframe_count;
				u32 sprite_frame_list_count = model_header->frame_list_count;

				u32 needed_size = sizeof(model_bone) * bone_count;
				needed_size += (sizeof(model_sprite) * sprite_count);
				needed_size += (sizeof(render_texture *) * sheets_count);
				needed_size += (sizeof(sprite_orientation) * uvs_count);
				needed_size += (sizeof(model_animation) * animations_count);
				needed_size += (sizeof(model_animation_keyframe) * keyframe_count);
				needed_size += (sizeof(model_animation_keyframe) * frame_keyframe_count);
				needed_size += (sizeof(model_mesh_frame_list) * sprite_frame_list_count);

				asset_memory_operation memory_operation = assets_data_allocation_begin(
						assets,
						needed_size,
						asset_data->index);
				//external sprite sheets
				asset_data->model.sprite_sheets_a = memory_chunk_push_array(
						&memory_operation.chunk,
						render_texture *,
						sheets_count);
				//bones
				asset_data->model.bones = memory_chunk_push_array(
						&memory_operation.chunk,
						model_bone,
						bone_count);

				//sprites
				asset_data->model.sprites = memory_chunk_push_array(
						&memory_operation.chunk,
						model_sprite,
						sprite_count);

				//uvs used in frame lists
				asset_data->model.uvs = memory_chunk_push_array(
						&memory_operation.chunk,
						sprite_orientation,
						uvs_count);

				//push animations array
				asset_data->model.animations = memory_chunk_push_array(
						&memory_operation.chunk,
						model_animation,
						animations_count);
				//frame list
				asset_data->model.mesh_frame_list = memory_chunk_push_array(
						&memory_operation.chunk,
						model_mesh_frame_list,
						sprite_frame_list_count);
				//transform keyframes
				asset_data->model.keyframes = memory_chunk_push_array(
						&memory_operation.chunk,
						model_animation_keyframe,
						keyframe_count);
				//sub keyframes
				asset_data->model.frame_keyframes = memory_chunk_push_array(
						&memory_operation.chunk,
						model_animation_keyframe,
						frame_keyframe_count);

				//uvs
				//asset_data->model.

				//fill the struct on the slot
				{
					model *loaded_model = &asset_data->model;
					//allocate data
					loaded_model->bone_count = bone_count;
					loaded_model->sprite_count = sprite_count;
					loaded_model->sprite_sheet_count = sheets_count;
					loaded_model->uvs_count = 0;
					loaded_model->animation_count = model_header->animation_count;
					loaded_model->frame_list_count = model_header->frame_list_count;
					loaded_model->keyframe_count = model_header->keyframe_count;
					loaded_model->frame_keyframe_count = model_header->frame_keyframe_count;
					loaded_model->orientation_amount = model_header->orientation_amount;


					//
					// sprite quad loading
					//

					//get the bones line
#define asset_UP 1
					resource_reader_get_to_line(&re, model_header->line_to_bones);
					{
						//load bones
						u32 loaded_sprites = 0;
						u32 loaded_uvs = 0;
						for(u32 b = 0; b < bone_count; b++)
						{
							ppse_model_bone *file_model_bone = resource_reader_get_struct(&re, ppse_model_bone);
							model_bone *model_bone = loaded_model->bones + b;
							//model_bone *spriteBone = editor_AddBone(game_editor); 

							model_bone->parent = file_model_bone->parent;
							model_bone->p = file_model_bone->p;
							model_bone->displacement = file_model_bone->displacement;
							model_bone->q = file_model_bone->q;
							model_bone->two_dim = file_model_bone->two_dim;
							model_bone->sprite_count = file_model_bone->sprite_count;
							model_bone->frame_key_count = file_model_bone->frame_key_count;
							model_bone->sprites_at = loaded_sprites;
							//model_bone->rotation_x   = file_model_bone->rotation_x;
							//model_bone->rotation_y   = file_model_bone->rotation_y;
							//model_bone->rotation_z   = file_model_bone->rotation_z;

							//load sprites
#if asset_UP
							for(u32 s = 0; s < model_bone->sprite_count; s++)
							{
								loaded_sprites++;
								ppse_model_sprite *file_model_sprite = resource_reader_get_struct(&re, ppse_model_sprite);
								//fill data
								model_sprite *model_sprite = loaded_model->sprites + model_bone->sprites_at + s;

								model_sprite->type          = file_model_sprite->type;
								model_sprite->bone_index    = file_model_sprite->bone_index;
								model_sprite->p             = file_model_sprite->p;
								model_sprite->depth_x       = file_model_sprite->depth_x;
								model_sprite->depth_y       = file_model_sprite->depth_y;
								model_sprite->depth_z       = file_model_sprite->depth_z;
								model_sprite->pivotX        = file_model_sprite->pivotX;
								model_sprite->pivotY        = file_model_sprite->pivotY;
								model_sprite->pivotZ        = file_model_sprite->pivotZ;
								model_sprite->texture_index = file_model_sprite->texture_index;
								model_sprite->extra_frame_count = file_model_sprite->extra_frame_count;;
								model_sprite->frame_list_index = file_model_sprite->frame_list_index;

								model_sprite->face_axis = file_model_sprite->face_axis;

								model_sprite->size = file_model_sprite->v0;
								model_sprite->size2 = file_model_sprite->v1;
								//model_sprite->v0 = file_model_sprite->v0;
								//model_sprite->v1 = file_model_sprite->v1;
								model_sprite->v2 = file_model_sprite->v2;
								model_sprite->v3 = file_model_sprite->v3;
								model_sprite->frame_at = loaded_uvs;
								//load sprite uvs
								for(u32 u = 0; u <= model_sprite->extra_frame_count; u++)
								{
									ppse_frame_list_uvs *file_uvs = resource_reader_get_struct(&re, ppse_frame_list_uvs);

									sprite_orientation *model_uvs_at =
										loaded_model->uvs + model_sprite->frame_at + u;
									model_uvs_at->uv0 = file_uvs->uv0;
									model_uvs_at->uv1 = file_uvs->uv1;
									model_uvs_at->uv2 = file_uvs->uv2;
									model_uvs_at->uv3 = file_uvs->uv3;
									model_uvs_at->offset.x = file_uvs->offset_x;
									model_uvs_at->offset.y = file_uvs->offset_y;
									model_uvs_at->offset.z = file_uvs->offset_z;
									model_uvs_at->option = file_uvs->option;
									model_uvs_at->x_rot_index = file_uvs->x_rot_index;
									model_uvs_at->y_rot_index = file_uvs->y_rot_index;
									model_uvs_at->skin_index = file_uvs->skin_index;

									loaded_model->uvs_count++;
									loaded_uvs++;

								}
								//load sprite frames or uvs

								//Load dimension data for billboard sprite.

								//where it is on the uvs array
							}
#endif


						}
					}
					//line to animations
					resource_reader_next_line(&re);
					{
						u32 used_keyframe_slots = 0;
						u32 used_frame_keyframe_slots = 0;
						u32 used_clip_slots = 0;
						for(u32 a = 0;
								a < loaded_model->animation_count;
								a++)
						{
							//read animation
							model_animation *model_animation = loaded_model->animations + a;

							ppse_model_animation *file_model_animation = resource_reader_get_struct(&re, ppse_model_animation);

							u32 keyframe_count = file_model_animation->keyframe_count;

							model_animation->frames_total = file_model_animation->frames_total;
							//model_animation->keyframes          = memory_area_push_array(memory, model_animation_keyframe, keyframe_count);
							model_animation->loop = file_model_animation->loop;
							model_animation->frame_loop_start = file_model_animation->frame_loop_start;
							model_animation->frame_loop_end = file_model_animation->frame_loop_end;
							model_animation->frames_per_ms = file_model_animation->frames_per_ms;
							model_animation->frame_timer = file_model_animation->frame_timer;
							model_animation->frame_timer_repeat = file_model_animation->frame_timer_repeat;
							model_animation->keep_timer_on_transition = file_model_animation->keep_timer_on_transition;
							model_animation->repeat = file_model_animation->repeat;
							model_animation->keyframe_count = file_model_animation->keyframe_count;
							model_animation->frame_keyframe_count = file_model_animation->frame_keyframe_count;
							model_animation->keyframes_at = used_keyframe_slots;
							model_animation->frame_keyframes_at = used_frame_keyframe_slots;

							used_keyframe_slots += file_model_animation->keyframe_count;
							used_frame_keyframe_slots += file_model_animation->frame_keyframe_count;
							//read transform keyframes
							for(u32 k = 0; k < model_animation->keyframe_count; k++)
							{
								ppse_model_animation_keyframe *file_model_key_frame = resource_reader_get_struct(&re, ppse_model_animation_keyframe);
								//get keyframe from array
								model_animation_keyframe *model_key_frame = 
									loaded_model->keyframes + model_animation->keyframes_at + k;

								//transform data
								model_key_frame->bone_index = file_model_key_frame->bone_index; // or bone
								model_key_frame->offset = file_model_key_frame->offset;
								model_key_frame->q = file_model_key_frame->q;
								model_key_frame->spline            = file_model_key_frame->spline;

								model_key_frame->timer_frame_repeat = file_model_key_frame->frame_repeat;
								model_key_frame->timer_frame = file_model_key_frame->timer_frame;
								model_key_frame->frame_start = file_model_key_frame->frame_start;
								model_key_frame->switch_parent = file_model_key_frame->switch_parent;
								model_key_frame->parent_index = file_model_key_frame->parent_index;

#if 0
								f32 angle = 0;
								quaternion_fill_rotations_radians(
										model_key_frame->q,
										&angle,
										&model_key_frame->rotation_x,
										&model_key_frame->rotation_y,
										&model_key_frame->rotation_z
										);
#else
								model_key_frame->rotation_x = file_model_key_frame->rotation_x;
								model_key_frame->rotation_y = file_model_key_frame->rotation_y;
								model_key_frame->rotation_z = file_model_key_frame->rotation_z;
#endif
							}
							//read frame keyframes
							for(u32 k = 0; k < model_animation->frame_keyframe_count; k++)
							{
								ppse_model_animation_keyframe *file_model_key_frame = resource_reader_get_struct(&re, ppse_model_animation_keyframe);
								//get keyframe from array
								model_animation_keyframe *model_key_frame = 
									loaded_model->frame_keyframes + model_animation->frame_keyframes_at + k;

								//frame data
								model_key_frame->mesh_index = file_model_key_frame->mesh_index; // or bone
								model_key_frame->frame_key = file_model_key_frame->frame_key;
								model_key_frame->flip_h = file_model_key_frame->flip_h;

								model_key_frame->timer_frame_repeat = file_model_key_frame->frame_repeat;
								model_key_frame->timer_frame = file_model_key_frame->timer_frame;
								model_key_frame->frame_start = file_model_key_frame->frame_start;
							}
						}
					}
				}
				//load the external images to use as sprite sheets
				// load the sprite sheets. They come first on the composite resources array

				//read composite resources line
				resource_reader_get_to_line(&re, model_header->line_to_sheet_headers);

				for(u32 s = 0;
						s < sheets_count;
						s++)
				{
					//get external files header
					ppse_composite_resource *composite_resource = resource_reader_get_struct(&re, ppse_composite_resource);
					stream_pushf(info_stream,
							"Attemping to load the external sheet \"%s\"",
							composite_resource->path_and_name);
					render_texture *loaded_image = assets_load_and_get_image(
							assets,
							composite_resource->path_and_name);
					//logs
					if(loaded_image)
					{
						stream_pushf(info_stream,
								"Success!");
					}
					else
					{
						stream_pushf(info_stream,
								"Error while loading the external image Nº%d from the resources table",
								composite_resource->path_and_name);
					}
					asset_data->model.sprite_sheets_a[s] = loaded_image;
				}

				//fill returning data
				assets_data_allocation_end(&memory_operation);
			}
			else
			{
				if(!signature_check)
				{
					stream_pushf(info_stream,
							"Error while loading the file %s as a model file! Signature check failed.",
							path_and_name);
				}
				else
				{

					stream_pushf(info_stream,
							"Error while loading the model file %s! Version check failed got %d but expected %d.",
							path_and_name,
							ppse_model_version,
							model_header->header.version);
				}
			}
		}
	}
	temporary_area_end(&temporary_model_area);
	return(success);

}



static b32
assets_allocate_entity(
		game_assets *assets,
		asset_slot_data *asset_data,
		u8 *path_and_name)
{
	stream_data *info_stream = &assets->info_stream;
	platform_api *platform = assets->platform;
	b32 success = 0;
	asset_data->id = assets_generate_id(path_and_name);
	asset_data->type = asset_type_entity;


	temporary_area temporary_model_area = temporary_area_begin(&assets->area);

	platform_entire_file entire_file = assets_read_entire_file_from_pack_or_disk(
			assets,
			path_and_name);
	//file got readed and allocated!
	if(entire_file.contents)
	{
		resource_reader re = resource_reader_begin(entire_file.contents);
		ppse_coso_header *header = resource_reader_get_struct(&re, ppse_coso_header);

		u32 signature_check = header->header.signature == ppse_coso_SIGNATURE;
		u32 version_check = header->header.version == ppse_coso_version;

		if(signature_check && version_check)
		{
			asset_data->state = asset_state_loaded;
			success = 1;
			//calculate needed size
			//skip the first line for now

			//calculate the size required to store the model
			u32 needed_size = 
				header->state_count * sizeof(state_node) +
				header->action_count * sizeof(state_action) +
				header->state_lines_count * sizeof(state_line) +
				header->collision_count * sizeof(collision_cube)
				;

#if 1
			asset_memory_operation memory_operation = assets_data_allocation_begin(
					assets,
					needed_size,
					asset_data->index);
			//transform keyframes
			asset_data->entity.states.states = memory_chunk_push_array(
					&memory_operation.chunk,
				    state_node,	
					header->state_count);
			//frame keyframes
			asset_data->entity.states.actions = memory_chunk_push_array(
					&memory_operation.chunk,
					state_action,
					header->action_count);

			asset_data->entity.states.state_lines = memory_chunk_push_array(
					&memory_operation.chunk,
					state_line,
					header->state_lines_count);
			asset_data->entity.collisions = memory_chunk_push_array(&memory_operation.chunk,
					collision_cube, header->collision_count);

			//uvs
			//asset_data->model.

			assets_data_allocation_end(&memory_operation);
#endif
			//fill the struct on the slot
			//assets_fill_entity_from_memory(
			//		&asset_data->entity,
			//		entire_file.contents,
			//		info_stream);
			{

				asset_entity *entity = &asset_data->entity;
				entity->default_state = header->default_state;
				//get to stats
				resource_reader_get_to_line(&re, header->line_to_stats);
				{
					ppse_coso_stats *file_stats = resource_reader_get_struct(&re, ppse_coso_stats);
					entity->speed = file_stats->speed;
					entity->z_speed = file_stats->z_speed;
					entity->speed_max = file_stats->speed_max;
					entity->collision_size.x = file_stats->collision_x;
					entity->collision_size.y = file_stats->collision_y;
					entity->collision_size.z = file_stats->collision_z;
					entity->collision_offset.x = file_stats->collision_offset_x;
					entity->collision_offset.y = file_stats->collision_offset_y;
					entity->collision_offset.z = file_stats->collision_offset_z;
				}
				state_main *coso_states = &entity->states;
				coso_states->state_count = header->state_count;
				//get to states
				resource_reader_next_line(&re);
				{

					u32 loaded_lines = 0;
					u32 loaded_conditions = 0;
					for(u32 s = 0; s < header->state_count; s++)
					{
						ppse_coso_state *file_state = resource_reader_get_struct(&re, ppse_coso_state);
						state_node *state = entity->states.states + s;
						state->state_do_at = loaded_lines;
						loaded_lines += file_state->line_count;
						state->state_line_count = file_state->line_count;
						//load lines
						for(u32 l = 0; l < file_state->line_count; l++)
						{
							//get line
							ppse_coso_state_line *file_state_line = 
								resource_reader_get_struct(&re, ppse_coso_state_line);
							state_line *line = 
								coso_states->state_lines + state->state_do_at + l;
							line->type = file_state_line->flags;
							line->action_index = file_state_line->action_index;
							line->state_index = file_state_line->state_index;
							//set correct conditions pointing values
							line->trigger_count = file_state_line->condition_count;
							line->triggers_at = loaded_conditions;
							loaded_conditions += line->trigger_count;
							//load line conditions
							for(u32 c = 0; c < line->trigger_count; c++)
							{
								//get condition
								ppse_coso_state_condition *file_condition = resource_reader_get_struct(&re, ppse_coso_state_condition);
								state_trigger *condition = coso_states->triggers + line->triggers_at + c;
								condition->type = file_condition->type;
								condition->not = file_condition->not;
								condition->eq = (u8)file_condition->eq;
								condition->radius = file_condition->radius;
							}
						}
					}
				}
				//get to actions
				resource_reader_next_line(&re);
				{
					u32 loaded_actions = 0;
					for(u32 a = 0; a < header->action_count; a++)
					{
						ppse_coso_action *file_action = resource_reader_get_struct(&re, ppse_coso_action);
						state_action *action = coso_states->actions + a;
						action->action_lines_count = file_action->line_count;
						action->action_lines_at = loaded_actions;
						loaded_actions += action->action_lines_count;
						for(u32 l = 0; l < file_action->line_count; l++)
						{
							ppse_coso_action_line *file_action_line = resource_reader_get_struct(&re, ppse_coso_action_line);
							state_action_line *action_line = coso_states->action_lines + action->action_lines_at + l;
							action_line->target_index = file_action_line->target_index;
							action_line->animation_index = file_action_line->animation_index;
							action_line->time_total = file_action_line->time_total;
						}
					}
				}
				//collisions
				resource_reader_next_line(&re);
				{
					for(u32 c = 0; c < header->collision_count; c++)
					{
						ppse_coso_collision *file_collision = resource_reader_get_struct(&re, ppse_coso_collision);
						collision_cube *collision = entity->collisions;
						collision->offset = file_collision->offset;
						collision->size = file_collision->size;
						entity->collision_count++;
					}
				}
			}
			//load the external images to use as sprite sheets
			// load the sprite sheets. They come first on the composite resources array
			resource_reader_get_to_line(&re, header->line_to_composite_resources);

			ppse_composite_resource *composite_resource = resource_reader_get_struct(&re, ppse_composite_resource);
			asset_data->entity.model = assets_load_and_get_model(assets, 
					composite_resource->path_and_name);

			//fill returning data
		}
		else
		{
			if(!signature_check)
			{
				stream_pushf(info_stream,
						"Error while loading the file %s as an entity file! Signature check failed.",
						path_and_name);
			}
			else
			{

				stream_pushf(info_stream,
						"Error while loading the entity file %s! Version check failed got %d but expected %d.",
						path_and_name,
						ppse_model_version,
						header->header.version);
			}
		}
	}
	temporary_area_end(&temporary_model_area);
	return(success);
}

static asset_slot_data *
assets_load_and_get_tileset_slot(game_assets *assets, u8 *path_and_name)
{

	stream_data *info_stream = &assets->info_stream;

    asset_slot_data *asset_data = assets_find_asset_by_id(assets, assets_generate_id(path_and_name));
	//this was already allocated
	if(!asset_data)
	{

		asset_data = assets_get_new_slot(assets);
		u32 success = assets_allocate_tileset(
				assets,
				asset_data,
				path_and_name);
		if(!success)
		{
			assets_free_slot(assets, asset_data);
			asset_data = 0;
		}

	}
	return(asset_data);
}

static inline world_tileset *
assets_load_and_get_tileset(game_assets *assets,
		                 u8 *path_and_name)
{
	asset_slot_data *slot = assets_load_and_get_tileset_slot(assets, path_and_name);
	return(slot ? &slot->tileset : 0);
}


#define assets_load_and_get_map(assets, path_and_name)\
	(&assets_load_and_get_map_slot(assets, path_and_name)->map)

static asset_slot_data *
assets_load_and_get_map_slot(game_assets *assets,
		                u8 *path_and_name)
{

	u32 map_id = assets_generate_id(path_and_name);
	stream_data *info_stream = &assets->info_stream;
    asset_slot_data *asset_data = assets_find_asset_by_id(assets, map_id);

	//this was already allocated
	if(!asset_data)
	{
		asset_data = assets_get_new_slot(assets);
		u32 success = assets_allocate_map(
				assets,
				asset_data,
				path_and_name);
		if(!success)
		{
			assets_free_slot(assets, asset_data);
			asset_data = 0;
		}
	}
	return(asset_data);
}

static asset_slot_data *
assets_load_and_get_model_slot(game_assets *assets,
		                  u8 *path_and_name)
{

	//initial data
	stream_data *info_stream = &assets->info_stream;

	u32 model_id = assets_generate_id(path_and_name);

    asset_slot_data *asset_data = assets_find_asset_by_id(assets, model_id);
	//this was already allocated
	if(!asset_data)
	{
		asset_data = assets_get_new_slot(assets);
		b32 success = assets_allocate_model(assets, asset_data, path_and_name);
		if(!success)
		{
			assets_free_slot(assets, asset_data);
			asset_data = 0;
		}

	}
	return(asset_data);

}
static inline model *
assets_load_and_get_model(game_assets *assets, u8 *path_and_name)
{
	asset_slot_data *slot = assets_load_and_get_model_slot(assets, path_and_name);
	return(slot ? &slot->model : 0);
}

static asset_slot_data *
assets_load_and_get_entity_slot(game_assets *assets,
		                  u8 *path_and_name)
{

	//initial data
	stream_data *info_stream = &assets->info_stream;

	u32 id = assets_generate_id(path_and_name);

    asset_slot_data *asset_data = assets_find_asset_by_id(assets, id);
	//this was already allocated
	if(!asset_data)
	{
		asset_data = assets_get_new_slot(assets);
		b32 success = assets_allocate_entity(assets, asset_data, path_and_name);
		if(!success)
		{
			assets_free_slot(assets, asset_data);
			asset_data = 0;
		}

	}
	return(asset_data);

}
static inline asset_entity *
assets_load_and_get_entity(game_assets *assets, u8 *path_and_name)
{
	asset_slot_data *slot = assets_load_and_get_entity_slot(assets, path_and_name);
	return(slot ? &slot->entity : 0);
}



static inline b32 
assets_reload_slot(
		game_assets *assets,
		asset_slot_data *slot_data,
		u8 *path_and_name)
{
	b32 success = 0;
	u32 id = assets_generate_id(path_and_name);
	ppse_resource_handle resource_handle = 
		assets_get_resource_handle_from_pack_or_disk(
				assets,
				path_and_name);

	switch(slot_data->type)
	{
		//for images, make a new texture request
		case asset_type_image:
			{
				texture_request *new_request = render_texture_request_start(
						assets->texture_operations,
						render_use_texture_slot(assets->texture_operations,
							slot_data->image.index));

				assets_fill_image_data_from_handle(
						assets,
						resource_handle.handle,
						resource_handle.data.data_offset,
						(u32 *)&slot_data->image.width,
						(u32 *)&slot_data->image.height,
						new_request->pixels);
			}break;
		case asset_type_tileset: //for the rest, unload and reallocate
			{
				assets_unload_slot_data(
						assets,
						slot_data);
				assets_allocate_tileset(
						assets,
						slot_data,
						path_and_name);
			}break;
		case asset_type_map:
			{
				assets_unload_slot_data(
						assets,
						slot_data);
				assets_allocate_map(
						assets,
						slot_data,
						path_and_name);
			}break;
		case asset_type_model:
			{
				assets_unload_slot_data(
						assets,
						slot_data);
				assets_allocate_model(
						assets,
						slot_data,
						path_and_name);
			}break;
		case asset_type_entity:
			{
				assets_unload_slot_data(
						assets,
						slot_data);
				assets_allocate_entity(
						assets,
						slot_data,
						path_and_name);
			}break;
		default:
			{
				//you forgot something...
				Assert(0);
			}break;
	}

	ppse_close_resource_handle(assets, resource_handle);
	return(success);
}


static inline asset_slot_data * 
assets_reload_or_allocate(
		game_assets *assets,
		asset_type type,
		u8 *path_and_name)
{
	b32 success = 0;
	asset_slot_data *slot_data = assets_find_asset_by_id(assets, assets_generate_id(path_and_name));
	if(slot_data)
	{
		success = assets_reload_slot(assets, slot_data, path_and_name);
	}
	else
	{
		//allocate for the new slot based on type
		switch(type)
		{
			case asset_type_image:
				{
					slot_data = assets_load_and_get_image_slot(assets, path_and_name);
				}break;
			case asset_type_tileset:
				{
					slot_data = assets_load_and_get_tileset_slot(assets, path_and_name);
				}break;
			case asset_type_map:
				{
					slot_data = assets_load_and_get_map_slot(assets, path_and_name);
				}break;
			case asset_type_model:
				{
					slot_data = assets_load_and_get_model_slot(assets, path_and_name);
				}break;
			case asset_type_entity:
				{
					slot_data = assets_load_and_get_entity_slot(assets, path_and_name);
				}break;
			default:
				{
					Assert(0); //for forgot something...
				}break;
		}
	}
	return(slot_data);
}
#define assets_reload_or_allocate_tileset assets_reload_or_allocate(assets, asset_type_tileset)
#define assets_reload_or_allocate_image assets_reload_or_allocate(assets, asset_type_tileset)


static image_data
assets_read_image_data_from_index(game_assets *game_asset_manager, platform_api *platform, memory_area *area, u32 assetIndex)
{
	image_data imageData = {0};
	asset_slot_data *assetData = game_asset_manager->assets + assetIndex;

	if(assetData->type == asset_type_image)
	{

	} 
	else if(assetData->type == asset_type_font)
	{
	}
	else
	{
		Assert(0);
	}

	return(imageData);
}



inline void
assets_allocate_white_texture(
		game_assets *game_asset_manager)
{
	asset_slot_data *whiteTextureData = game_asset_manager->assets + 0;
	//manually fill the slot data
	whiteTextureData->index  = 0;
	whiteTextureData->id = assets_WHITE_TEXTURE_ID;
	whiteTextureData->source = asset_source_coded;
	whiteTextureData->image.width  = 512;
	whiteTextureData->image.height = 512;
	game_asset_manager->asset_count++;
	//REMOVE!
	whiteTextureData->state = asset_state_loaded;
	//

	ppse_resource_handle empty_handle = {0};
	texture_request *new_request = render_texture_request_start(
			game_asset_manager->texture_operations,
			render_get_texture_slot(game_asset_manager->texture_operations));
	render_texture_request_fill_pixels(
			game_asset_manager->texture_operations,
			new_request,
			0xffffffff);
}


static game_assets *
assets_allocate_game_assets(
		memory_area *_area,
		platform_api *platform,
		render_texture_data *texture_operations,
		game_resource_initial_settings initial_settings,
		memory_area *info_stream_area)
{
	u32 asset_allocation_size   = initial_settings.allocation_size;
	u32 asset_operations_size   = initial_settings.operations_size;
	u32 stop_program_on_failure = initial_settings.stop_at_fail;
	u32 initial_assets_count    = initial_settings.initial_assets_count;
	game_initial_asset_data *initial_assets = initial_settings.initial_assets;
	u8 *resource_file_name      = initial_settings.resource_pack_file_name;
	u32 use_pack_file           = initial_settings.use_pack;
	u32 read_resources_folder   = initial_settings.dev_build;
	//
	// game_assets setup
	//
	//Create a sub area from the main memory area and set it to game_asset_manager.
    memory_area _assetArea = memory_area_create_from(_area, asset_operations_size);
	//initialize allocated memory to zero
    memory_area_clear(&_assetArea);

    game_assets *game_asset_manager = memory_area_push_struct(&_assetArea, game_assets);
    game_asset_manager->area = _assetArea;
    memory_area *asset_mng_area = &game_asset_manager->area;
	//for logging
	if(info_stream_area)
	{
		game_asset_manager->info_stream = stream_Create(info_stream_area);
	}

#if 1
	game_asset_manager->asset_memory_max = asset_allocation_size;
	game_asset_manager->asset_memory = memory_area_clear_and_push(_area, asset_allocation_size);
	game_asset_manager->texture_operations = texture_operations;


	game_asset_manager->platform = platform;

	game_asset_manager->using_package   = use_pack_file;
	//Allocate game assets
	game_asset_manager->asset_max = assets_MAX_ASSET_CAPACITY;
    game_asset_manager->assets = memory_area_push_array(
			asset_mng_area,
			asset_slot_data,
			assets_MAX_ASSET_CAPACITY);

#else
	game_asset_manager->modelBones      = memory_area_push_array(asset_mng_area, model_bone, assets_MAX_BONE_CAPACITY);
	game_asset_manager->model_sprites   = memory_area_push_array(asset_mng_area, model_sprite, assets_MAX_BONE_CAPACITY);
	game_asset_manager->tilesetTiles    = memory_area_push_array(asset_mng_area, world_tileset_tile, assets_MAX_TILE_CAPACITY);
	game_asset_manager->tileset_animations = memory_area_push_array(asset_mng_area, world_tileset_animation, 400);
	game_asset_manager->tileset_animations_max = 400;
	//allocate world memory
	game_asset_manager->game_world_memory.entity_max  = 1000;
	game_asset_manager->game_world_memory.tileset_max = 100;
	game_asset_manager->game_world_memory.tiles_max   = 10000;


	game_asset_manager->game_world_memory.tiles    = memory_area_push_array(asset_mng_area, world_tile, game_asset_manager->game_world_memory.tiles_max);
	game_asset_manager->game_world_memory.tilesets = memory_area_push_array(asset_mng_area, u32, game_asset_manager->game_world_memory.tileset_max);
	game_asset_manager->game_world_memory.entities = memory_area_push_array(asset_mng_area, world_entity, game_asset_manager->game_world_memory.entity_max);
#endif
    
	//
	// white texture
	//
    assets_allocate_white_texture(game_asset_manager);

	//
	// file loading and asset setup
	//
	//use the packed file to allocate and read assets, else just load from disk
	if(use_pack_file)
	{
        platform_file_handle asset_file_handle = platform->f_open_file(resource_file_name,
	    		                                                          platform_file_op_read | platform_file_op_share | platform_file_op_write);
	    if(asset_file_handle.handle)
	    {

            ppse_file_header ppse_resources_header = {0}; 
            platform->f_read_from_file(asset_file_handle,
	    			                      0,
	    								  sizeof(ppse_file_header),
	    								  &ppse_resources_header);

	        u32 signature_check = ppse_resources_header.signature == ppse_SIGNATURE;
	        u32 version_check   = ppse_resources_header.version == ppse_version;

            if(signature_check && version_check)
            {
	        	//start reading the files
	            uint32 resource_file_offset = ppse_resources_header.offset_to_file_headers;

                game_asset_manager->assetFile.handle = asset_file_handle;
                game_asset_manager->assetFile.header = ppse_resources_header;
//                game_asset_manager->asset_max	 += ppse_resources_header.asset_count;
	        	//Push more asset space
                memory_area_push_array(asset_mng_area, asset_slot_data, ppse_resources_header.asset_count);

	        	u32 assetIndex = 0;
	        	while(assetIndex < initial_assets_count)
                {

	    			//generate the expected id using the name
	        		u32 expected_resource_id = assets_generate_id(initial_assets[assetIndex].path_and_name);

	    			//find the asset information by the id
                    ppse_offset_and_asset_data resource_file_offset_and_data = assets_get_packed_file_header_by_id(game_asset_manager, expected_resource_id);

	    			u32 resource_offset_to_name   = resource_file_offset_and_data.offset_to_name;
                    ppse_asset_data ppseAssetData = resource_file_offset_and_data.data;

	    			//asset was found
	    			if(resource_offset_to_name)
	    			{
        
                        asset_slot_data assetData = {0};
        	    		//;Remove
                        Assert(ppseAssetData.id);

                        assetData.id    = ppseAssetData.id;
        
        	    		//Offset to name to look up for the file when editing.
        	    		assetData.offsetToName = resource_offset_to_name;
                    //    assetData.type		   = ppseAssetData.type;
                        assetData.state		   = asset_state_free;
        	    		assetData.source       = asset_source_packed;
        
                        u32 file_data_offset = ppseAssetData.data_offset; 

	    				//get magick number
        
#if 0
                        if(ppseAssetData.type == asset_type_image)
                        {
        	    			ppse_image imageDataFromFile = {0};
        	    			platform->f_read_from_file(asset_file_handle,
	    							                      file_data_offset,
	    												  sizeof(image_data),
	    												  &imageDataFromFile);

                            assetData.image.width  = imageDataFromFile.h;
                            assetData.image.height = imageDataFromFile.w;
                            assetData.image.index = TEXTUREUNLOADED;
        
                        }
                        else if(ppseAssetData.type == asset_type_font)
                        {
                            font_proportional *font = &assetData.font;
        	    			ppse_fill_font_struct_from_handle(asset_mng_area,
	    							                        platform,
	    													asset_file_handle,
	    													file_data_offset,
	    													font);
                        }
	    				else if(ppseAssetData.type == asset_type_model)
	    				{
	    				}
        	    		else
        	    		{
        	    			NotImplemented;
        	    		}

	    				//advance and set loaded asset data.
	        		    assetData.index = game_asset_manager->asset_count;
                        game_asset_manager->assets[game_asset_manager->asset_count++] = assetData;
#endif
	    			}
	    			else
	    			{
	    				//log
	    			}
	        		assetIndex++;
	        		//Store to the asset array.
                }

                platform_file_min_attributes fileInfo = platform->f_get_file_info(asset_file_handle);
                game_asset_manager->totalSize = fileInfo.size;

            }
            else
            {
	        	if(stop_program_on_failure)
	        	{
	        		Assert(0);
	        	}
	        	//log
	        	if(!signature_check)
	        	{
	        	}
	        	else if(!version_check)
	        	{
	        	}
            }
        }
    }

#if 0
//if debug
	u8 *raw_assets_filePath = "data/AssetLoading.assets";
	platform_file_handle raw_assets_file = platform->f_open_file(raw_assets_filePath,
			                                                platform_file_op_read | platform_file_op_share | platform_file_op_write);
	u32 failedLoading = 1;
	if(raw_assets_file.handle)
	{
		u32 data_offset = 0;

		//write header and set data
        ppse_raw_assets_header rawAssetsHeader = {0};
		platform->f_read_from_file(raw_assets_file, 0, sizeof(ppse_raw_assets_header), &rawAssetsHeader);
		data_offset += sizeof(ppse_raw_assets_header);


		u32 signature_check = rawAssetsHeader.signature == ppse_RAW_ASSETS_SIGNATURE;
		u32 rawFileCount     = rawAssetsHeader.asset_count;

		if(signature_check)
		{
			failedLoading = 0;

		    game_asset_manager->raw_assets_file_header = rawAssetsHeader;
		    game_asset_manager->raw_assets_file       = raw_assets_file;

			for(u32 a = 0; a < rawFileCount; a++)
			{

                ppse_asset_identifier asset_identifier = {0};
				platform->f_read_from_file(raw_assets_file, data_offset, sizeof(ppse_asset_identifier), &asset_identifier);

				//check for existing id on the existing assets
				platform_file_handle assetSourceFile = platform->f_open_file(asset_identifier.path_and_name, platform_file_op_read | platform_file_op_share | platform_file_op_write | platform_file_op_share);
				if(assetSourceFile.handle)
				{
					platform->f_close_file(assetSourceFile);

				    asset_slot_data newAsset = {0};
				    //newAsset.file = assetSourceFile;
				    newAsset.id    = asset_identifier.id;
				    newAsset.type         = asset_identifier.type;
				    newAsset.state        = asset_state_free;
				    newAsset.source       = asset_source_from_disk;
					newAsset.offsetToName = data_offset;

				    assets_add_asset_from_disk(game_asset_manager, newAsset);
				}
				else
				{
					//log
				}
				data_offset += sizeof(ppse_asset_identifier);

			}

		}
		else
		{
			//log
		}
	}

	if(failedLoading)
	{
		//log
		game_asset_manager->raw_assets_file = platform->f_open_file(raw_assets_filePath, platform_file_op_create_new);

		ppse_raw_assets_header rawAssetsHeader = {0};
		rawAssetsHeader.signature = ppse_RAW_ASSETS_SIGNATURE;

		platform->f_write_to_file(game_asset_manager->raw_assets_file, 0, sizeof(ppse_raw_assets_header), &rawAssetsHeader);

		game_asset_manager->raw_assets_file_header = rawAssetsHeader;
	}
#endif

    return(game_asset_manager);
}
