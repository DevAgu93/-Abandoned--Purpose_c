//OFFSET GLYPH POINTS BEFORE SCALING WHEN READING THE GLYF TABLE
/*
Definitions:
Ligature: a character consisting of two or more joined letters, e.g. æ, fl.

Encoding: convert (information or an instruction) into a particular form

Granularity: the scale or level of detail present in a set of data or other phenomenon.

Composite glyph: These are made by referencing one or more other glyphs in the font.

Winding number/index in glyph: Is an integer representing the total number of times the ray casted from that point hits some glyph curvature.

Required tables:
Tag	    Name
'cmap'	Character to glyph mapping
'head'	Font header
'hhea'	Horizontal header
'hmtx'	Horizontal metrics
'maxp'	Maximum profile
'name'	Naming table
OS/2	OS/2 and Windows specific metrics
'post'	PostScript information

Points:
If two consecutive points are "on" the curve (p0, p1), then draw a straight line
If the first point is on (p0), the second off (p1), and the third on (p2), then interpolate.
if the second and third are off (p1 and p2), then the third point is half the distance between them ((p2 - p1) * 0.5f)

scale:
ttf_get_height_scale or ttf_get_em_scale

Placing characters on image:
Since the coordinates are inverted on the y-axis, before rendering to the final bitmap the
glyph gets drawn to a buffer where its scaled and flipped, then it is rendered on the final
buffer on (glyph_left_side_bearing * scale, baseline - glyph_max_y * scale)

Baseline:
The acender from hhea * scale

Rasterizer steps:
first compute and get the windings of the glyph (I will write a better explanation later)
after that, for each divided point, compute how much covers a certain pixel on a scanline (which is 1 pixel tall)
*/
//NOTE: fonts with outlines uses 0x00010000 as the sfnt_version
//fonts containing CFF data uses 'OTTO' instead

//If the font file contains only one font, the table directory will begin at byte 0 of the file.
#define GLYPH_VALUE(g) (u8)(g)
#define true_type_SIGNATURE 0x5F0F3CF5

#pragma pack(push, 1)

typedef struct{
	union{
	    u8 value_a[4];
	    u32 value;
	};
}ttf_tag;

//Records occur sequentially within a parent structure, either within a sequence of table fields or within an array of records of a given type. If a record contains an offset to a sub-structure, that structure is logically a subtable of the record’s parent table and the offset is normally from the start of the parent table.
typedef struct{
	ttf_tag tag;
	u32 check_sum;
	u32 offset; //Offset from beginning of font file.
	u32 table_length;
}ttf_record;


//Tables are referenced by offsets. If a table contains an offset to a sub-structure, the offset is normally from the start of that table.
typedef struct{
	union{
	    u32 sfnt_version; //0x00010000 or 0x4F54544F ('OTTO')
	    u8 sfnt_version_chars[4];
	};

	u16 number_of_tables;
	u16 search_range; //Maximum power of 2 less than or equal to number_of_tables, times 16 ((2*floor(log2(number_of_tables))) * 16, where “**” is an exponentiation operator).
	u16 entry_selector;//Log2 of the maximum power of 2 less than or equal to numTables (log2(searchRange/16), which is equal to floor(log2(numTables))).
	u16 range_shift; //numTables times 16, minus searchRange ((numTables * 16) - searchRange).
	//void *table_records; //the amount is the number of tables
// The records in the array must be sorted in ascending order by tag

}ttf_table;

typedef struct{
	ttf_record *records;
}ttf_table_records;

//1.0
typedef struct{
	u8 tag[4];
	u16 major_version;
	u16 minor_version;
	u32 number_of_fonts;
	u32 table_directory_offset;

}ttc_header;

typedef struct{
	u16 start_glyph_id;
	u16 end_glyph_id;
	u16 class; //applied to all glyphs in the range of start and end
}ttf_class_range_record;
//classes
//1	Base glyph (single character, spacing glyph)
//2	Ligature glyph (multiple character, spacing glyph)
//3	Mark glyph (non-spacing combining glyph)
//4	Component glyph (part of single character, spacing glyph)

typedef struct{
	u16 class_format;
	u16 start_glyph_id;
	u16 glyph_count;
	//u16 class_value_array[glyph_count]
}ttf_class_definition_1;

typedef struct{
	u16 class_format;
	u16 class_range_count;
	//ttf_class_range_record[class_range_count]
}ttf_class_definition_2;

//Coverages tables are used to indicate which glyphs are used in an operation,
//almost every subtable uses one of these for their respective operations
typedef struct{
	//format 1 stores glyph ids in one simple array and glyph_count is the amount
	//format 2 stores a number of glyph ranges instead. It is followed by an array of ttf_coverage_range_record
	u16 coverage_format;
	union
	{
	    u16 glyph_count;
		u16 range_count;
	};
	//u16 glyph_id[glyph_count]
	//or
	//ttf_coverage_range_record[range_count]
}ttf_coverage_table;

typedef struct{
	u16 start_glyph_id;
	u16 end_glyph_id;
	u16 start_coverage_index; //Coverage Index of first glyph ID in range
}ttf_coverage_range_record;


//
//tables
//

// DSGI
// ;Complete?
typedef struct{
    u32 version; //0x00000001
	u16 number_of_signatures;
	u16 flags; //bit 0 cannot be resigned
	//signature_records[number of signatures]
}ttf_digital_signature_table;

typedef struct{
	u32 format;
	u32 length;
	u32 signature_block_offset;
}ttf_digital_signature_records;

// DSGI__

//GPOS
typedef struct{
	u16 v0;
	u16 v1;
}ttf_glyph_positioning_table_version;

typedef struct{
	u16 v0;
	u16 v1;
	u16 script_list_offset;
	u16 feature_list_offset;
	u16 lookup_list_offset;

	//for version 1.1 in v0 and v1
	//u32 feature_variations_offset
}ttf_glyph_positioning_header;
//GPOS__

//GDEF
typedef struct{
	u16 major_version;
	u16 minor_version;
	u16 class_definition_offset;
	u16 attachment_point_offset;
	u16 ligature_caret_list_offset;
	u16 mark_attachment_definition_offset;

	//1.2 | 1.3
	//u16 mark_glyph_sets_offset;
	//1.3
	//u32 item_variation_store_table_offset; // from beginning of GDEF header (may be NULL)
}ttf_glyph_definition_header;

typedef struct{
	u16 coverage_table_offset; //offset from the begininning of the points list table
	u16 glyphs_with_attachment_count;

	//Array of offsets to AttachPoint tables-from beginning of AttachList table-in Coverage Index order
	//u16 attach_points_offsets[glyphs_with_attachment_count]

}ttf_attachment_list_table;

typedef struct{
	u16 attachment_points_count; //offset from the begininning of the points list table
//	u16 attachment_points_indices[attachkment_points_count];

}ttf_attachment_points_table;

typedef struct{
	u16 coverage_table_offset;
	u16 ligature_glyphs_count;

	//Array of offsets to LigGlyph tables, from beginning of LigCaretList table
	//u16 ligature_glyphs_offsets[ligature_glyphs_count]

}ttf_ligature_caret_list_table;

typedef struct{
	u16 caret_count; //(components - 1)
	//u16 caret_value_offsets[caret_count]
}ttf_ligature_table;

typedef struct{
	i16 coordinate;
}ttf_caret_value_table_1;

typedef struct{
	u16 contour_point_index;
}ttf_caret_value_table_2;

typedef struct{
	i16 coordinate;
	u16 device_offset;
}ttf_caret_value_table_3;
//GDEF__

//cmap

typedef struct{
	u16 platform_id;
	u16 platform_encoding_id;
	u32 subtable_offset;//Byte offset from beginning of table to the subtable for this encoding

}ttf_cmap_encoding_record;

typedef struct{

	u16 version;
	u16 number_of_encoding_tables;
    //ttf_cmap_encoding_record[number_of_encoding_tables];
}ttf_cmap_header;

typedef struct{
	u16 subtable_format;
	u16 sub_table_length;
	u16 language;

	u16 segment_count_x2; //divide by two to get the segment_count
	u16 search_range; //Maximum power of 2 less than or equal to segCount, times 2 ((2**floor(log2(segCount))) * 2
	u16 entry_selector;
	u16 range_shift;
	/*
	   u16 end_code[segment_count];
	   u16 reserved_pad; //always 0
	   u16 start_code[segment_count];
	   i16 id_delta[segment_count];
	   u16 id_range_offset[segment_count];
	   u16 glyph_id_array[?]
	*/
}cmap_subtable_format_4;
//cmap__

//head
typedef struct{
	u16 v0;
	u16 v1;
	f32 font_revision;
	u32 checksum_adjustment;
	u32 signature; //0x5F0F3CF5
	u16 global_flags;
	u16 units_per_em; //from 16 to 16384. FUnit or font units
	i64 date_created;
	i64 date_modified;
	i16 min_x; //bounding box data
	i16 min_y;
	i16 max_x;
	i16 max_y;
	u16 mac_style;
	u16 smallest_readable_size;
	i16 font_direction_hint; //deprecated
	i16 index_to_loc_format; //0 for u16 1 for u32. for the 'loca' table.
	i16 glyph_data_format; //always 0

}ttf_head_table;

/*for mac_style
	Bit 0: Bold (if set to 1);
	Bit 1: Italic (if set to 1)
	Bit 2: Underline (if set to 1)
	Bit 3: Outline (if set to 1)
	Bit 4: Shadow (if set to 1)
	Bit 5: Condensed (if set to 1)
	Bit 6: Extended (if set to 1)
	Bits 7–15: Reserved (set to 0).
*/
//head__

//hhea
typedef struct{
	u16 v0;
	u16 v1;

	//apple specific
	i16 ascender;
	i16 descender;
	i16 line_gap;

	u16 advance_width_max; //max advance width in 'htmx' table
	i16 min_left_side_bearing; //also for htmx. Only with glyphs with contours
	i16 min_right_side_bearing;
	i16 max_extent; 	//Max(lsb + (xMax - xMin)).
    i16 caret_slope_rise; //Used to calculate the slope of the cursor (rise/run); 1 for vertical.
	i16 caret_slope_run; //0 for vertical
	i16 caret_offset; //The amount by which a slanted highlight on a glyph needs to be shifted to produce the best appearance. Set to 0 for non-slanted fonts
	i16 reserved_zero[4];
	i16 metric_data_format;
	u16 number_of_horizontal_metrics;
}ttf_horizontal_layout_table;
//hhea_

//hmtx  
//Used alongside the hhea table
//Provides glyph advance widths and left side bearings.
typedef struct{
	u16 advance_width;
	i16 lsb; //The horizontal distance from the current point to the left most point on the glyph
}ttf_horizontal_metrics_record;
//hmtx__

//maxp
//Fonts with CFF or CFF2 outlines must use Version 0.5 of this table, specifying only the numGlyphs field. Fonts with TrueType outlines must use Version 1.0

typedef struct{
	u32 version_packed;

	u16 number_of_glyphs;
	u16 max_points;
	u16 max_contours;
	u16 max_composite_points;
	u16 max_composite_contours;

	u16 max_zones;
	u16 max_twilight_points;
	u16 max_storage;
	u16 max_function_defs; //Equal to the highest function number + 1.
	u16 max_instruction_defs;

	u16 max_stack_elements;
	u16 max_size_of_instructions;
	u16 max_component_elements;
	u16 max_component_depth;
}ttf_maxp_table_1_0;
//maxp__

//name
typedef struct{
	u16 v;
	u16 name_record_count;
	u16 offset_to_storage; //from start of table
	//ttf_name_record[name_record_count]
}ttf_name_table;

typedef struct{
	u16 string_length;
	u16 string_offset;
}ttf_name_language_tag_record;

typedef struct{
	u16 platform_id;
	u16 encoding_id;
	u16 language_id;

	u16 name_id;
	u16 string_length;
	u16 string_offset;
}ttf_name_record;

/*name ids:
  0	Copyright notice.
  1	Font Family.
  2	Font Subfamily.
  3	Unique subfamily identification.
  4	Full name of the font.
  5	Version of the name table.
  6	PostScript name of the font. All PostScript names in a font must be identical.
  7	Trademark notice.
  8	Manufacturer name.
  9	Designer
  10	Description
  11	URL of the font vendor (with procotol, e.g., http://, ftp://)
  12	URL of the font designer
  13	License description
  14	License information URL, where additional licensing information can be found.
  15	Reserved
  16	Preferred Family. In Windows, the Family name is displayed in the font menu, and the Subfamily name is presented as the Style name.
  17	Preferred Subfamily.
  18	Compatible Full (macOS only).
  19	Sample text.
  20–24	Defined by OpenType.
  25	Variations PostScript Name Prefix.
*/

//name__

//OS/2
//metrics table version 0
typedef struct{
    u16	version;
    i16	average_character_width;
    u16	us_weight_class;
    u16	us_width_class;
    u16	type_flags;
    i16	subscript_horizontal_size;
    i16	subscript_vertical_size;
    i16	subscript_offset_x;
    i16	subscript_offset_y;
    i16	superscript_horizontal_size;
    i16	superscript_vertical_size;
    i16	superscript_offset_x;
    i16	superscript_offset_y;
    i16	strikeout_size;
    i16	strikeout_position;
    i16	family_class;
    u8	panose[10];
	//for platform 3 indicated in 'cmap'
    u32	unicode_range1;
    u32	unicode_range2;
    u32	unicode_range3;
    u32	unicode_range4;
    u8 vendor_id[4];
    u16	font_selection_flags;

    u16	first_char_index;
    u16	last_char_index;
    i16	typographic_ascender;
    i16	typographic_descender;
    i16	typographic_line_gap;
    u16	windows_ascender;
    u16	windows_descender;

}ttf_windows_metrics_table;

typedef struct{
	i16 sx_height; //
	i16 s_cap_height;
	u16 default_char;
	u16 default_break_char;
	u16 max_context_length;
	u16 lower_optical_point_size;
	u16 upper_optical_point_size;
    u32	code_page_character_range0;
    u32	code_page_character_range1;
}ttf_windows_metrics_table_footer;
//OS/2__

//glyf/glyph data table
typedef struct{
	i16 number_of_contours;
	i16 min_x;
	i16 min_y;
	i16 max_x;
	i16 max_y;
}ttf_glyf_header;

typedef enum{
	simple_glyph_on_curve_point = 0x01,
	simple_glyph_x_short_vector = 0x02,
	simple_glyph_y_short_vector = 0x04,
	simple_glyph_repeat_flag    = 0x08,

	simple_glyph_x_is_same_or_positive_x_short_vector = 0x10,
	simple_glyph_y_is_same_or_positive_y_short_vector = 0x20,
	simple_glyph_overlap_simple                       = 0x40,
}ttf_simple_glyph_flags;

typedef enum{
	component_glyph_arg_1_and_2_are_words = 0x0001,
	component_glyph_args_are_xy_values    = 0x0002,
	component_glyph_round_xy_to_grid      = 0x0004,
	component_glyph_we_have_a_scale       = 0x0008,
	component_glyph_more_components       = 0x0020,

	component_glyph_we_have_an_x_and_y_scale = 0x0040,
	component_glyph_we_have_a_two_by_two     = 0x0080,
	component_glyph_we_have_instructions     = 0x0100,
	component_glyph_use_my_metrics           = 0x0200,
	component_glyph_overlap_compound         = 0x0400,

	component_glyph_scaled_component_offset   = 0x0800,
	component_glyph_unscaled_component_offset = 0x1000,

}ttf_component_glyph_flags;
//glyf__

//kern
typedef struct{
	u16 version;
	u16 table_numbers;
}ttf_kern_header;

typedef struct{
	u16 version;
	u16 length;
	u16 information_type; //coverage

}ttf_kern_table;

typedef struct{
	u16 number_of_pairs;
	u16 search_range;
}ttf_kern_table_format0;
//__kern

//gpos
typedef struct{
	i16 x_placement;
	i16 y_placement;
	i16 x_advance;
	i16 y_advance;
	//these may be 0
	u16 x_placement_device_offset;
	u16 y_placement_device_offset;
	u16 x_advance_device_offset;
	u16 y_advance_device_offset;
}ttf_value_record;
//__gpos

//
//General use
//

typedef struct{
	u16 lookup_type;
	u16 lookup_flag;
	u16 sub_table_count;
	//u16 sub_table_offsets[sub_table_count];
	//u16 mark_filtering_set;
}ttf_lookup_table;

#pragma pack(pop)

typedef enum{
	lookup_right_to_left      = 0x0001,
	lookup_ignore_base_glyphs = 0x0002,
	lookup_ignore_ligatures   = 0x0004,

	lookup_ignore_marks           = 0x0008,
	lookup_use_mark_filtering_set = 0x0010,
	lookup_reserved               = 0x00E0,

	lookup_mark_attachment_type_mask = 0xFF00,
}ttf_lookup_flags;



//
// struct for parsing the font file
//

typedef struct{
	i16 x;
	i16 y;
	u32 flags;
}ttf_glyph_point;

typedef struct{

	u32 glyph_code;

	i16 min_x;
	i16 min_y;
	i16 max_x;
	i16 max_y;

	i16 number_of_contours;
	u16 *end_points_of_contours;

	u32 glyph_count;
	u32 *glyph_point_start_indices;

	u32 point_coordinates_count;
	ttf_glyph_point *point_coordinates;

}ttf_glyph_info;

typedef struct{

	//at start of file
	u32 table_records_count;
	ttf_record *table_records;

	//hhea
	u32 number_of_horizontal_metrics;
	i16 baseline_ascender;
	i16 baseline_descender;
	i32 baseline_line_gap;

	//'head'
	u16 units_per_em; //font units or FUnits

	u16 global_flags;
	i16 min_x;
	i16 min_y;
	i16 max_x;
	i16 max_y;
	u16 number_of_glyphs; //numGlyphs, 'maxp'
	i16 index_to_loc_format; //'head'
	u16 reserved;

	u32 offset_to_glyf_table;

	i16 *kerning_pairs;

	union{
		void *loca_offsets;
		u32 *loca_offsets_u32;
		u16 *loca_offsets_u16;
        //[number_of_glyphs + 1]
	};
	//from hmtx
    ttf_horizontal_metrics_record *horizontal_metrics_records;
    i16 *left_side_bearings;

	ttf_glyph_info *glyphs_info;

	//cmap
	u16 *glyph_indices;
	u16 *glyph_codes;
	
	u32 requested_glyph_s;
	u32 requested_glyph_e;
    stream_data *info_stream;

	f32 font_scale;

}ttf_file_work;


typedef struct
{
   void *dcHandle;
   void *bmpHandle;
   void *fontHandle;
   void *bits;
   
   ppse_font header;

   uint32 loadedBitmapW;
   uint32 loadedBitmapH;

   uint32 firstGlyph;
   uint32 lastGlyph;
   uint32 glyph_count;
   uint32 totalGlyphHeight;

   uint32 glyphDisplacementsSize;
   uint32 *glyphDisplacements;


   u32 glyphTableSize;
   ppse_glyph *glyphs;

   image_data font_image;
   //uint8 *fontImagePixels;
   uint32 font_image_size;

}ttf_font_creation_info;


//
// rasterizer structs
//

typedef struct{
	vec2 p0;
	vec2 p1;
	vec2 p2;
}ttf_curve;

typedef struct{
	union{
		struct{
			f32 x;
			f32 y;
		};
		vec2 v;
	};
}ttf_profile_points;


typedef struct{
	f32 y_top;
	f32 y_bot;
	u32 pixel_x;

	u32 x0;
	u32 x1;
	f32 *areas;
}ttf_active_edge;

typedef struct{
	i32 y_direction;
	u32 y0;
	u32 y1;

	u32 first_curve_index;
	u32 last_curve_index;

	u32 points_count;
	ttf_profile_points *points;
}ttf_curve_profile;

typedef struct{
	u32 contour_count;
	u32 *curve_counts_per_contour;
	ttf_curve *glyph_curves;

	u32 curve_profile_count;
	ttf_curve_profile *curve_profiles;
}ttf_glyph_work;


typedef struct{
	u32 points_count;
	ttf_glyph_point *points;
	i32 number_of_contours;
	u16 *contour_end_point_indices;

	u32 glyph_count;
	u32 *glyph_point_start_indices;
	
}ttf_glyf_coordinates_result;

typedef struct{
	u32 x;
	u32 y;
}ttf_glyph_dimensions;

//=============================================
//=============================================
//=============================================

static ttf_glyph_dimensions 
ttf_work_get_glyph_size_code(
		ttf_file_work *file_work,
		u32 glyph_code)
{
	ttf_glyph_dimensions glyph_size = {0};
	if(file_work && file_work->glyphs_info)
	{
		u32 i = glyph_code - file_work->requested_glyph_s;
		if(i <= file_work->requested_glyph_e)
		{
			glyph_size.x = (file_work->glyphs_info[i].max_x - file_work->glyphs_info[i].min_x);
			glyph_size.y = (file_work->glyphs_info[i].max_y - file_work->glyphs_info[i].min_y);
		}
	}
	return(glyph_size);
}

static inline u32
ttf_work_get_glyph_index(
		ttf_file_work *file_work,
		u32 glyph_code)
{
	u32 glyph_index = 0;
	if(file_work && file_work->glyph_indices)
	{
		glyph_index = file_work->glyph_indices[glyph_code];
	}

	return(glyph_index);
}
static inline u32 
ttf_work_get_glyph_index_bitmap_w(ttf_file_work *file_work, u32 glyph_index, f32 scale)
{
	//get the glyph metrics from the htmx table
	ttf_horizontal_metrics_record glyph_horizontal_metrics = file_work->horizontal_metrics_records[glyph_index];
	u32 result = glyph_horizontal_metrics.lsb + glyph_horizontal_metrics.advance_width;
	result = (u32)f32_ceil(glyph_horizontal_metrics.advance_width * scale);

	return(result);
}

static inline u32
ttf_work_get_glyph_code_bitmap_w(ttf_file_work *file_work, u32 glyph_code, f32 scale)
{
	u32 glyph_index = file_work->glyph_indices[glyph_code];

    u32 result = ttf_work_get_glyph_index_bitmap_w(file_work, glyph_index, scale);

	return(result);
}

static inline f32
ttf_get_em_scale(ttf_file_work *file_work, u32 s)
{
	f32 result = (f32)s / (file_work->units_per_em);
	return(result);
}

static inline f32
ttf_get_height_scale(ttf_file_work *file_work, u32 requested_height)
{
	f32 height_scale = (f32)requested_height / (file_work->baseline_ascender - file_work->baseline_descender);
	return(height_scale);
}

static inline u32
ttf_get_font_height(ttf_file_work *font_file_work, f32 scale)
{
	u32 result = (u32)(scale * (font_file_work->baseline_ascender - font_file_work->baseline_descender));
	return(result);

}

static inline void
ttf_correct_points(
		u32 glyph_points_count,
		ttf_glyph_point *glyph_points)
{
		i32 coordinate_correction_x0 = 10000;
		i32 coordinate_correction_y0 = 10000;

		i32 coordinate_correction_x1 = 0;
		i32 coordinate_correction_y1 = 0;

		i32 point_offset_x = 0;
		i32 point_offset_y = 0;
		//offset points and get the left-most and upper_most coordinate
        for(u32 p = 0;
	    		p < glyph_points_count;
	    		p++)
	    {
	    	ttf_glyph_point *point = glyph_points + p;
	    	point_offset_x += point->x;
	    	point_offset_y += point->y;
	    	point->x       = point_offset_x;
	    	point->y       = point_offset_y;

			if(point->x < coordinate_correction_x0)
			{
				coordinate_correction_x0 = point->x;
			}
			if(point->y < coordinate_correction_y0)
			{
				coordinate_correction_y0 = point->y;
			}

		}
		//
		//correct the coordinates
		//
        for(u32 p = 0;
	    		p < glyph_points_count;
	    		p++)
	    {
	    	ttf_glyph_point *point = glyph_points + p;

	    	point->x -= coordinate_correction_x0;
	    	point->y -= coordinate_correction_y0;
		}
}

static inline void
ttf_flip_glyph_points_vertically(
		u32 glyph_points_count,
		ttf_glyph_point *glyph_points,
		f32 glyph_scale,
		stream_data *info_stream)
{
		// ;for now
		Assert(glyph_points_count);
		//scale, offset the points and convert them to pixel coordinates
		i32 coordinate_correction_x0 = 10000;
		i32 coordinate_correction_y0 = 10000;

		i32 coordinate_correction_x1 = 0;
		i32 coordinate_correction_y1 = 0;

		f32 point_offset_x = 0;
		f32 point_offset_y = 0;
		//offset points and get the left-most and upper_most coordinate
        for(u32 p = 0;
	    		p < glyph_points_count;
	    		p++)
	    {
	    	ttf_glyph_point *point = glyph_points + p;

	    	point_offset_x += point->x;
	    	point_offset_y += point->y;

	    	point->x = (i32)(point->x * glyph_scale);
	    	point->y = (i32)(point->y * -glyph_scale);

			if(point->x < coordinate_correction_x0)
			{
				coordinate_correction_x0 = point->x;
			}
			if(point->y < coordinate_correction_y0)
			{
				coordinate_correction_y0 = point->y;
			}

		}
		//
		//correct the coordinates
		//
        for(u32 p = 0;
	    		p < glyph_points_count;
	    		p++)
	    {
	    	ttf_glyph_point *point = glyph_points + p;

	    	point->x -= coordinate_correction_x0;
	    	point->y -= coordinate_correction_y0;
	    	//point->x -= coordinate_correction_x1;
	    	//point->y -= coordinate_correction_y1;
			stream_pushf(
					info_stream,
					"%u. coordinates are (%d, %d)",
					p,
					point->x,
					point->y);
		}
}

inline u32
ttf_code_in_range(u32 c, u32 first_c, u32 end_c)
{
	u32 result = (c >= first_c) &&
				 (c <= end_c);
	return(result);
}

inline i16 *
ttf_allocate_kerning_table(memory_area *area, u32 first_code_point, u32 last_code_point)
{
	i16 *kerning_offsets = 0;
	u32 glyph_count = last_code_point - first_code_point + 1;
	if(last_code_point >= first_code_point)
	{
		kerning_offsets = memory_area_clear_and_push(
				area,
				sizeof(i16) * (glyph_count * glyph_count));
	}

	return(kerning_offsets);
}

inline void
ttf_decode_name_to(u8 *name, u8 *dest, u32 name_length)
{
	u32 next_char_to = 0;
	u32 c = 0;
	while(c < name_length)
	{
		if(name[c] != '\0')
		{
		    dest[next_char_to++] = name[c];
		}

		c++;
	}
}

inline void
ttf_decode_name(u8 *name, u32 name_length)
{
	u32 next_char_to = 0;
	u32 c = 0;
	while(c < name_length)
	{
		name[next_char_to] = name[c];

		if(name[next_char_to] != '\0')
		{
			next_char_to++;
		}

		c++;
	}
}

inline u32
ttf_tag_check(u8 *tag)
{
	u32 result = (tag[0] >= 0x20 && tag[0] <= 0x7E);
	result    &= (tag[1] >= 0x20 && tag[1] <= 0x7E);
	result    &= (tag[2] >= 0x20 && tag[2] <= 0x7E);
	result    &= (tag[3] >= 0x20 && tag[3] <= 0x7E);

	return(result);
}

inline u32
ttf_check_signature_u32_swapped(u32 signature)
{
	u32 result = (signature) == 0x00010000 ||
		         (signature) == 'OTTO';

	return(result);
}

inline u32
ttf_calculate_table_checksum(
		u32 *table_checksum,
		u32 table_length)
{
    u32 sum = 0L;
    uint32 *end_ptr = table_checksum+((table_length + 3) & ~3) / sizeof(u32);
    while (table_checksum < end_ptr)
	{
        sum += *table_checksum++;
	}
    return(sum);
}

inline f32
ttf_f2dot24_to_float(i16 v)
{
	f32 result = 0;

	result = (f32)((v >> 14) & 0x3);
	v      = (v & 0x3fff);
	result += ((f32)v / 16383);

	return(result);
}

inline u32
ttf_get_offset_to_table(
		ttf_file_work *file_work,
		u32 table_tag)
{
	u32 offset_to_table = 0;
	u32 r = 0;
	while(!offset_to_table && r < file_work->table_records_count)
	{
		if(file_work->table_records[r].tag.value == table_tag)
		{
			offset_to_table = file_work->table_records[r].offset;
		}
		r++;
	}
	return(offset_to_table);
}

inline void
ttf_read_kern_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'kern');
	if(offset_to_table)
	{
		ttf_kern_header *kern_header = (ttf_kern_header *)(font_file_memory + offset_to_table);

		endian_swap_16(&kern_header->version);
		endian_swap_16(&kern_header->table_numbers);

		Assert(kern_header->version == 0);
		file_work->kerning_pairs = ttf_allocate_kerning_table(
				area,
				file_work->requested_glyph_s,
				file_work->requested_glyph_e
				);

		u32 kern_subtable_offset = 0;
		for(u32 t = 0;
				t < kern_header->table_numbers;
				t++)
		{
			ttf_kern_table *kern_table = memory_advance_from(
					kern_header,
					sizeof(ttf_kern_header) + kern_subtable_offset);
		    endian_swap_16(&kern_table->version);
		    endian_swap_16(&kern_table->length);
		    endian_swap_16(&kern_table->information_type);


			u16 table_information = kern_table->information_type;

			u32 has_horizontal_data = (table_information & 0x1);
			if(!has_horizontal_data) //skip glyph
			{
			    //u32 has_kerning_tables  = !(table_information & 0x3);
			    u8 format = (table_information & 0xff00);

			    u32 offset_to_table_info = kern_subtable_offset + sizeof(ttf_kern_table);
				//I only need format 0 for horizontal advancements
			    if(format == 0)
			    {
			    	u32 kern_table_format_0_size = sizeof(u16) * 4;
			    	u16 number_of_pairs = *(u16 *)font_file_memory + offset_to_table_info;
			    	offset_to_table_info += kern_table_format_0_size;

			    	endian_swap_16(&number_of_pairs);
					u32 glyph_count = file_work->requested_glyph_e - file_work->requested_glyph_s + 1;
					u32 first_requested_code = file_work->requested_glyph_s;
					u32 last_requested_code  = file_work->requested_glyph_e;
					
					//to advance the offset
					u32 size_of_kerning_pairs_table = sizeof(u16) * 3;

					for(u32 p = 0;
							p < number_of_pairs;
							p++)
					{
					    u16 left_glyph_index  = *(u16 *)(font_file_memory + offset_to_table_info + 0);
					    u16 right_glyph_index = *(u16 *)(font_file_memory + offset_to_table_info + 2);
					    i16 kerning_value     = *(i16 *)(font_file_memory + offset_to_table_info + 4);

						endian_swap_16(&left_glyph_index);
						endian_swap_16(&right_glyph_index);
						endian_swap_16(&kerning_value);

						u32 in_range = (left_glyph_index >= first_requested_code && right_glyph_index >= first_requested_code) &&
							           (left_glyph_index <= last_requested_code && right_glyph_index <= last_requested_code);
						if(in_range)
						{

						    u32 kerning_index = (left_glyph_index * glyph_count) + right_glyph_index;
						    file_work->kerning_pairs[kerning_index] = kerning_value;
						}
						offset_to_table_info += size_of_kerning_pairs_table;

					}

			    }
			}
			//advance offset to next table
			kern_subtable_offset += kern_table->length;
		}
	}
}

inline ttf_glyf_coordinates_result
ttf_glyf_table_read_simple_or_compound_glyph(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		u32 glyph_index,
		u32 correct,
		stream_data *info_stream)
{

	//results
	ttf_glyf_coordinates_result result = {0};
	ttf_glyph_point *glyph_points  = 0;
	u32 points_count               = 0;
	u16 *contour_end_point_indices = 0;
	i16 contour_count = 0;
	u32 is_simple = 0;

	u32 index_to_loc_format = file_work->index_to_loc_format;
	u32 offset_to_glyf_table = ttf_get_offset_to_table(
			file_work,
			'glyf');
	Assert(offset_to_glyf_table);

	u8 *offset_to_glyph_headers = font_file_memory + offset_to_glyf_table;

	//get offset to the loc table
	u32 offset_to_description = 0;
    if(index_to_loc_format == 0)
    {
    	u16 offset_to_description_16 = file_work->loca_offsets_u16[glyph_index];
    	endian_swap_16(&offset_to_description_16);
    	offset_to_description_16 *= 2;
    
    	offset_to_description = offset_to_description_16;
    }
    else
    {
    	u32 offset_to_description_32 = file_work->loca_offsets_u32[glyph_index];
        endian_swap_32(&offset_to_description_32);
    	//offset_to_description_32 *= 2;
    
    	offset_to_description = offset_to_description_32;
    }
//	offset_to_description += offset_to_glyf_table;

	ttf_glyf_header current_glyph_header = *(ttf_glyf_header *)(offset_to_glyph_headers + offset_to_description);
	endian_swap_i16(&current_glyph_header.number_of_contours);

    offset_to_description += sizeof(ttf_glyf_header);


	if(current_glyph_header.number_of_contours >= 0)
	{
	    i16 number_of_contours = current_glyph_header.number_of_contours;
		contour_count = number_of_contours;
		//read as simple glyph table
		is_simple = 1;

		u32 current_offset = offset_to_description;

		u16 *end_points_of_contours = (u16 *)(offset_to_glyph_headers + current_offset);

		//the end index of every contour on the glyph
		contour_end_point_indices = memory_area_push_array(
				area,
				u16,
				number_of_contours);
		for(u32 c = 0;
				c < (u32)number_of_contours;
				c++)
		{
			u16 contour_end_point_index = end_points_of_contours[c];
			endian_swap_16(&contour_end_point_index);
			contour_end_point_indices[c] = contour_end_point_index;

			stream_pushf(info_stream,
					"%u. Contour end points %u",
					c,
					contour_end_point_index);
		}
		//the end of end_points_contours
		u16 flags_length = contour_end_point_indices[number_of_contours - 1];
		//endian_swap_16(&flags_length);
		flags_length += 1;

	    points_count = flags_length;
		glyph_points = memory_area_push_array(
				area,
				ttf_glyph_point,
				flags_length);

		current_offset += number_of_contours * sizeof(u16);

		u16 instruction_length = *(u16 *)(offset_to_glyph_headers + current_offset);
		endian_swap_16(&instruction_length);

		current_offset += sizeof(u16);
		u8 *instructions = (offset_to_glyph_headers + current_offset);;

		current_offset += instruction_length;
		u8 *glyph_flags =  (offset_to_glyph_headers + current_offset);

		u32 total_points_number = 0;


		i16 coordinate_x = 0;

		i16 x = 0;
		i16 y = 0;
		u32 flag_index = 0;
		u32 actual_flags_amount = 0;
		stream_pushf(
				info_stream,
				"it has %d contours",
				number_of_contours);

		u32 next_flag_count = 0;
		u32 flags_stored_count = 0;
#if 1 
		//get flags
		for(u32 f = 0;
				f < flags_length;
				f++)
		{
			actual_flags_amount++;
			u8 repeat_amount = 1;
			u8 flag          = glyph_flags[flag_index];
			if(flag & simple_glyph_repeat_flag)
			{
				repeat_amount += glyph_flags[flag_index + 1];
				flag_index++;
				f += repeat_amount - 1;
				//next_flag_count
			}
			while(repeat_amount--)
			{

			    ttf_glyph_point *coordinates = glyph_points + next_flag_count;
				stream_pushf(info_stream,
						"%u. Next flag bits(ON:%u, XSHORT:%u, YSHORT:%u, XDUAL:%u, YDUAL%u, repeat:%u)",
						next_flag_count,
						(flag & simple_glyph_on_curve_point) != 0,
						(flag & simple_glyph_x_short_vector) != 0,
						(flag & simple_glyph_y_short_vector) != 0,
						(flag & simple_glyph_x_is_same_or_positive_x_short_vector) != 0,
						(flag & simple_glyph_y_is_same_or_positive_y_short_vector) != 0,
						(flag & simple_glyph_repeat_flag)
						);
			    next_flag_count++;
				coordinates->flags = flag;
			}
			flag_index++;
			flags_stored_count = flag_index;
		}
#endif

		temporary_area temporary_coordinates_area = temporary_area_begin(area);

		u8 *contour_x_coordinates = glyph_flags + flags_stored_count;
		u8 *contour_y_coordinates = 0;

		i16 *temporary_x_coordinates_array = memory_area_push_array(
				area,
				i16,
				flags_length);

		i16 *temporary_y_coordinates_array = memory_area_push_array(
				area,
				i16,
				flags_length);
		flag_index = 0;
        //
		// x coordinate
		//

	    u8 *coordinates_at = contour_x_coordinates;
	    u32 log_contour_count  = 0;
	    u32 total_offset = 0;
		u32 offset_to_y_coordinates = 0;

	    for(u32 f = 0;
	    		f < flags_length;
	    		f++)
	    {
	    	u32 x_offset = 0;

			ttf_glyph_point *coordinates = glyph_points + f;
	    	u8 current_flag      = coordinates->flags;
	    	u32 bit_x_short_set  = current_flag & simple_glyph_x_short_vector;
	    	u32 bit_x_dual_set   = current_flag & simple_glyph_x_is_same_or_positive_x_short_vector;
	    	u32 same_as_previous = !bit_x_short_set && bit_x_dual_set;

	    	u8 repeat_amount = 1;

	    	//is one byte long
	    	if(bit_x_short_set)
	    	{
	    		x_offset = sizeof(u8);
	    		x = *coordinates_at;
	    		if(!bit_x_dual_set)
	    		{
	    			x = -x;
	    		}
	    	}
	    	if(same_as_previous)
	    	{
	    		//nothing
	    		x_offset = 0;
	    		x = 0;
	    	}
	    	else if(!bit_x_short_set && !bit_x_dual_set)
	    	{
	    		x_offset = sizeof(i16);
	    		x = *(i16 *)coordinates_at;
	    		endian_swap_i16(&x);
	    	}
	    	coordinates_at += x_offset;
	    	offset_to_y_coordinates   += x_offset;

	    	temporary_x_coordinates_array[f] = x;
	    	log_contour_count++;
			coordinates->x = x;

	    	flag_index++;
	    }
		//
		// y coordinates
		//

		flag_index = 0;
		contour_y_coordinates = contour_x_coordinates + offset_to_y_coordinates;

	    coordinates_at = contour_y_coordinates;
	    log_contour_count = 0;

	    for(u32 f = 0;
	    		f < flags_length;
	    		f++)
	    {
	    	u32 coordinates_array_offset = 0;

			ttf_glyph_point *coordinates = glyph_points + f;
	    	u8 current_flag      = coordinates->flags; 
	    	u32 bit_y_short_set  = current_flag & simple_glyph_y_short_vector;
	    	u32 bit_y_dual_set   = current_flag & simple_glyph_y_is_same_or_positive_y_short_vector;
	    	u32 same_as_previous = !bit_y_short_set && bit_y_dual_set;

	    	u8 repeat_amount = 1;

	    	//is one byte long
	    	if(bit_y_short_set)
	    	{
	    		coordinates_array_offset = sizeof(u8);
	    		x = *coordinates_at;
	    		if(!bit_y_dual_set)
	    		{
	    			x = -x;
	    		}
	    	}
	    	if(same_as_previous)
	    	{
	    		//nothing
	    		coordinates_array_offset = 0;
	    		x = 0;
	    	}
	    	else if(!bit_y_short_set && !bit_y_dual_set)
	    	{
	    		coordinates_array_offset = sizeof(i16);
	    		x = *(i16 *)coordinates_at;
	    		endian_swap_i16(&x);
	    	}
	    	coordinates_at += coordinates_array_offset;
	    	temporary_y_coordinates_array[f] = x;
			coordinates->y = x;


	    	flag_index++;
	    }

		temporary_area_end(&temporary_coordinates_area);


		//store the coordinates on the current processing glyph

		i32 current_x_offset = 0;
		i32 current_y_offset = 0;
		for(u32 f = 0;
				f < flags_length;
				f++)
		{
			ttf_glyph_point *coordinates = glyph_points + f;
			//coordinates->x = temporary_x_coordinates_array[f];
			//coordinates->y = temporary_y_coordinates_array[f];
			//coordinates->flags = glyph_flags[f];

			current_x_offset += coordinates->x;
			current_y_offset += coordinates->y;

			stream_pushf(
					info_stream,
					"Stored coordinates (%d, %d), At (%d, %d)",
					coordinates->x,
					coordinates->y,
					current_x_offset,
					current_y_offset);
		}

	}
	else //composite or compound glyphs
	{
		//NotImplemented;
		u8 *memory_at = offset_to_glyph_headers + offset_to_description;

		u16 composite_flags = 0;
		temporary_area composite_glyph_area = temporary_area_begin(area);

		stream_data composite_glyph_stream = stream_Create(area);
		u32 glyph_count = 0;

		do{
			//referenced from the apple documentation
			f32 composite_values[6] = 
			{ 1, 0, 0, 1, 0, 0};

		    composite_flags = *(u16 *)memory_at;
		    memory_at += sizeof(u16);
		    u16 other_glyph_index =  *(u16 *)memory_at;
			memory_at += sizeof(u16);

			endian_swap_16(&composite_flags);
			endian_swap_16(&other_glyph_index);

			f32 x = 0;
			f32 y = 0;

			if(composite_flags & component_glyph_args_are_xy_values)
			{
				if(composite_flags & component_glyph_arg_1_and_2_are_words)
				{
				    i16 mem_x = *(i16 *)(memory_at + 0);
				    i16 mem_y = *(i16 *)(memory_at + 2);
				    endian_swap_i16(&mem_x);
				    endian_swap_i16(&mem_y);
				    x = mem_x;
				    y = mem_y;

					memory_at += sizeof(i16) * 2;
				}
				else
				{
				    i8 mem_x = *(i8 *)(memory_at + 0);
				    i8 mem_y = *(i8 *)(memory_at + 1);
				    x = mem_x;
				    y = mem_y;
					memory_at += 2;
				}

				composite_values[4] = x;
				composite_values[5] = y;
			}
			else
			{
				NotImplemented;
			}

			if(composite_flags & component_glyph_we_have_a_scale)
			{
				i16 scale = *(i16 *)memory_at;
				endian_swap_i16(&scale);
				composite_values[0] = ttf_f2dot24_to_float(scale);
				composite_values[3] = composite_values[0];

 
				memory_at += sizeof(i16);
			}
			else if(composite_flags & component_glyph_we_have_an_x_and_y_scale)
			{
				i16 scale_x = *(i16 *)memory_at;
				i16 scale_y = *(i16 *)(memory_at + sizeof(u16));
				endian_swap_i16(&scale_x);
				endian_swap_i16(&scale_y);
				
				composite_values[0] = ttf_f2dot24_to_float(scale_x);
				composite_values[3] = ttf_f2dot24_to_float(scale_y);

				memory_at += sizeof(i16) * 2;
			}
			else if(composite_flags & component_glyph_we_have_a_two_by_two)
			{
				i16 scale_x = *(i16 *)memory_at;
				i16 scale_0 = *(i16 *)(memory_at + 2);
				i16 scale_1 = *(i16 *)(memory_at + 4);
				i16 scale_y = *(i16 *)(memory_at + 6);

				endian_swap_i16(&scale_x);
				endian_swap_i16(&scale_0);
				endian_swap_i16(&scale_1);
				endian_swap_i16(&scale_y);
				composite_values[0] = ttf_f2dot24_to_float(scale_x);
				composite_values[1] = ttf_f2dot24_to_float(scale_0);
				composite_values[2] = ttf_f2dot24_to_float(scale_1);
				composite_values[3] = ttf_f2dot24_to_float(scale_y);

				memory_at += sizeof(i16) * 4;
			}

			f32 a = composite_values[0];
			f32 b = composite_values[1];
			f32 c = composite_values[2];
			f32 d = composite_values[3];
			f32 e = composite_values[4];
			f32 f = composite_values[5];
			
			f32 m = MAX(ABS(a), ABS(b));
			f32 n = MAX(ABS(c), ABS(d));

			f32 m_2 = sqrtf(a * a + b * b);
			f32 n_2 = sqrtf(c * c + d * d);

			f32 that_number = (33.0f / 65536);
			if(ABS(ABS(a) - ABS(c)) <= that_number)
			{
				m = (2 * m);
			}
			if(ABS(ABS(b) - ABS(d)) <= that_number)
			{
				n = (2 * n);
			}


			ttf_glyf_coordinates_result other_glyf_coordinates = ttf_glyf_table_read_simple_or_compound_glyph(
					area,
					font_file_memory,
					file_work,
					other_glyph_index,
					0,
					info_stream);
			u32 other_points_count              = other_glyf_coordinates.points_count;
			ttf_glyph_point *other_glyph_points = other_glyf_coordinates.points;
    	    ttf_correct_points(
    	    		other_points_count,
    	    		other_glyph_points);
			for(u32 p = 0;
					p < other_points_count;
					p++)
			{
				ttf_glyph_point *pts = other_glyph_points + p;
				f32 x_copy = pts->x;
				f32 y_copy = pts->y;
				//pts->x = (i16)(m * ((a / m) * x_copy + (c / m) * y_copy + e));
				//pts->y = (i16)(n * ((b / n) * x_copy + (d / n) * y_copy + f));
				pts->x = (i16)(m * ((a * x_copy) + (c * y_copy) + e));
				pts->y = (i16)(n * ((b * x_copy) + (d * y_copy) + f));
			}

			//free other glyph

			//append the new coordinate points and contour indices
			u16 *ctr_indices = stream_push_and_copy_array(
					&composite_glyph_stream,
					other_glyf_coordinates.contour_end_point_indices,
					u16,
					other_glyf_coordinates.number_of_contours);
			stream_push_and_copy_array(
					&composite_glyph_stream,
					other_glyf_coordinates.points,
					ttf_glyph_point,
					other_points_count);

			for(u16 c = 0;
					c < other_glyf_coordinates.number_of_contours;
					c++)
			{
				ctr_indices[c] += points_count;
			}

			points_count  += other_points_count;
			contour_count += other_glyf_coordinates.number_of_contours;

		}while(composite_flags & component_glyph_more_components);

		temporary_area_end(&composite_glyph_area);

		//allocate arrays
		contour_end_point_indices = memory_area_push_array(
				area,
				u16,
				contour_count);

		glyph_points = memory_area_push_array(
				area,
				ttf_glyph_point,
				points_count);

		//copy stream data to linear array
		stream_chunk *chunk_ctr = composite_glyph_stream.first;

		u8 *pts_at = (u8 *)glyph_points;
		u8 *ctr_at = (u8 *)contour_end_point_indices;
		while(chunk_ctr)
		{

			memory_copy(
					chunk_ctr->contents,
					ctr_at,
					chunk_ctr->size);

		    stream_chunk *chunk_pts = chunk_ctr->next;

			memory_copy(
					chunk_pts->contents,
					pts_at,
					chunk_pts->size);

			pts_at += chunk_pts->size;
			ctr_at += chunk_ctr->size;

		    chunk_ctr = chunk_pts->next;
		};
	}

	result.points_count = points_count;
	result.points       = glyph_points;
	result.contour_end_point_indices = contour_end_point_indices;
	result.number_of_contours = contour_count;

	if(correct && is_simple)
	{
    	ttf_correct_points(
    			points_count,
    			glyph_points);
	}
	return(result);
}

inline void
ttf_read_glyf_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_glyf_table = ttf_get_offset_to_table(
			file_work,
			'glyf');
	Assert(offset_to_glyf_table);

	u32 index_to_loc_format = file_work->index_to_loc_format;
	u32 requested_glyph_s = file_work->requested_glyph_s;
	u32 requested_glyph_e = file_work->requested_glyph_e;

	u32 requested_glyph_count = requested_glyph_e - requested_glyph_s + 1;
	

	file_work->glyphs_info = memory_area_clear_and_push_array(
			area,
			ttf_glyph_info,
			requested_glyph_count);
	if(1)
	{
		Assert(offset_to_glyf_table);
		stream_pushf(
				info_stream,
				"Starting to process the 'glyf' or glyph data table");

		ttf_glyf_header *glyph_headers = (ttf_glyf_header *)(font_file_memory + offset_to_glyf_table);
		
		u32 next_glyph_index = 0;
		for(u32 g = requested_glyph_s;
				g <= requested_glyph_e;
				g++)
		{
			ttf_glyph_info *next_glyph_info = file_work->glyphs_info + next_glyph_index++;
			next_glyph_info->glyph_code = g;

			u32 glyph_index = file_work->glyph_indices[g];
			//ttf_horizontal_metrics_record *glyph_horizontal_metrics = file_work->horizontal_metrics_records + glyph_index;

			//u16 advance_width = glyph_horizontal_metrics->advance_width;
			//i16 lsb           = glyph_horizontal_metrics->lsb;

			//get offsets from the loca table
			u32 offset_to_description = 0;
			if(index_to_loc_format == 0)
			{
				u16 offset_to_description_16 = file_work->loca_offsets_u16[glyph_index];
				endian_swap_16(&offset_to_description_16);
				offset_to_description_16 *= 2;

				offset_to_description = offset_to_description_16;
			}
			else
			{
				u32 offset_to_description_32 = file_work->loca_offsets_u32[glyph_index];
			    endian_swap_32(&offset_to_description_32);
//				offset_to_description_32 *= 2;

				offset_to_description = offset_to_description_32;
			}

			ttf_glyf_header current_glyph_header = *memory_advance_and_read_type_from(
					glyph_headers,
					ttf_glyf_header,
					offset_to_description);
			offset_to_description += sizeof(ttf_glyf_header);

			endian_swap_i16(&current_glyph_header.number_of_contours);

			endian_swap_i16(&current_glyph_header.min_x);
			endian_swap_i16(&current_glyph_header.min_y);
			endian_swap_i16(&current_glyph_header.max_x);
			endian_swap_i16(&current_glyph_header.max_y);

			i16 min_x = current_glyph_header.min_x;
			i16 min_y = current_glyph_header.min_y;
			i16 max_x = current_glyph_header.max_x;
			i16 max_y = current_glyph_header.max_y;

			//i16 rsb = advance_width - (lsb + max_x - min_x);

			i16 number_of_contours = current_glyph_header.number_of_contours;

			stream_pushf(
					info_stream,
					"Glyph %c (min_x:%d, min_y:%d, max_x:%d, max_y:%d, number_of_contours:%d)",
					g,
					min_x,
					min_y,
					max_x,
					max_y,
					number_of_contours);

			//max_x = rsb;
			//glyph is not composite
			next_glyph_info->min_x = min_x;
			next_glyph_info->min_y = min_y;
			next_glyph_info->max_x = max_x;
			next_glyph_info->max_y = max_y;
			next_glyph_info->number_of_contours = number_of_contours;



			stream_pushf(
					info_stream,
					"Reading glyph %c with index %u",
					g,
					glyph_index);

			ttf_glyf_coordinates_result glyf_coordinates = ttf_glyf_table_read_simple_or_compound_glyph(
					area,
					font_file_memory,
					file_work,
					glyph_index,
					1,
					info_stream);

			next_glyph_info->point_coordinates       = glyf_coordinates.points;
			next_glyph_info->point_coordinates_count = glyf_coordinates.points_count;
			next_glyph_info->end_points_of_contours  = glyf_coordinates.contour_end_point_indices;
			next_glyph_info->number_of_contours      = glyf_coordinates.number_of_contours;
#if 0
			if(number_of_contours >= 0)
			{
				//read as simple glyph table

				u32 current_offset = offset_to_description;

				u16 *end_points_of_contours = memory_advance_from(
						glyph_headers,
						current_offset);

				//the end index of every contour on the glyph
				next_glyph_info->end_points_of_contours = end_points_of_contours;
				for(u32 c = 0;
						c < (u32)number_of_contours;
						c++)
				{
					u16 *end_points_contour = end_points_of_contours + c;
					endian_swap_16(end_points_contour);
					stream_pushf(info_stream,
							"%u. Contour end points %u",
							c,
							*end_points_contour);
				}
				//the end of end_points_contours
				u16 flags_length = end_points_of_contours[number_of_contours - 1];
				//endian_swap_16(&flags_length);
				flags_length += 1;

				next_glyph_info->point_coordinates_count = flags_length;
				next_glyph_info->point_coordinates = memory_area_push_array(
						area,
						ttf_glyph_point,
						flags_length);

				current_offset += number_of_contours * sizeof(u16);

				u16 instruction_length = *memory_advance_and_read_type_from(
						glyph_headers,
						u16,
						current_offset);
				endian_swap_16(&instruction_length);

				current_offset += sizeof(u16);
				u8 *instructions = memory_advance_from(
						glyph_headers,
						current_offset);

				current_offset += instruction_length;
				u8 *glyph_flags =  memory_advance_from(
						glyph_headers,
						current_offset);

				u32 total_points_number = 0;


				i16 coordinate_x = 0;

				i16 x = 0;
				i16 y = 0;
				u32 flag_index = 0;
				u32 actual_flags_amount = 0;


				stream_pushf(
						info_stream,
						"Reading simple glyph %c",
						g);
				stream_pushf(
						info_stream,
						"it has %d contours",
						number_of_contours);

				u32 next_flag_count = 0;
				u32 flags_stored_count = 0;
#if 1
				//get flags
				for(u32 f = 0;
						f < flags_length;
						f++)
				{
					actual_flags_amount++;
					u8 repeat_amount = 1;
					u8 flag          = glyph_flags[flag_index];
					if(flag & simple_glyph_repeat_flag)
					{
						repeat_amount += glyph_flags[flag_index + 1];
						flag_index++;
						f += repeat_amount - 1;
						//next_flag_count
					}
					while(repeat_amount--)
					{

					    ttf_glyph_point *coordinates = next_glyph_info->point_coordinates + next_flag_count;
						stream_pushf(info_stream,
								"%u. Next flag bits(ON:%u, XSHORT:%u, YSHORT:%u, XDUAL:%u, YDUAL%u, repeat:%u)",
								next_flag_count,
								(flag & simple_glyph_on_curve_point) != 0,
								(flag & simple_glyph_x_short_vector) != 0,
								(flag & simple_glyph_y_short_vector) != 0,
								(flag & simple_glyph_x_is_same_or_positive_x_short_vector) != 0,
								(flag & simple_glyph_y_is_same_or_positive_y_short_vector) != 0,
								(flag & simple_glyph_repeat_flag)
								);
					    next_flag_count++;
						coordinates->flags = flag;
					}
					flag_index++;
					flags_stored_count = flag_index;
				}
#endif

				temporary_area temporary_coordinates_area = temporary_area_begin(area);

				u8 *contour_x_coordinates = glyph_flags + flags_stored_count;
				u8 *contour_y_coordinates = 0;

				i16 *temporary_x_coordinates_array = memory_area_push_array(
						area,
						i16,
						flags_length);

				i16 *temporary_y_coordinates_array = memory_area_push_array(
						area,
						i16,
						flags_length);
				flag_index = 0;
                //
				// x coordinate
				//

	            u8 *coordinates_at = contour_x_coordinates;
	            u32 log_contour_count  = 0;
	            u32 total_offset = 0;
				u32 offset_to_y_coordinates = 0;

	            for(u32 f = 0;
	            		f < flags_length;
	            		f++)
	            {
	            	u32 x_offset = 0;

					ttf_glyph_point *coordinates = next_glyph_info->point_coordinates + f;
	            	u8 current_flag      = coordinates->flags;
	            	u32 bit_x_short_set  = current_flag & simple_glyph_x_short_vector;
	            	u32 bit_x_dual_set   = current_flag & simple_glyph_x_is_same_or_positive_x_short_vector;
	            	u32 same_as_previous = !bit_x_short_set && bit_x_dual_set;

	            	u8 repeat_amount = 1;

	            	//is one byte long
	            	if(bit_x_short_set)
	            	{
	            		x_offset = sizeof(u8);
	            		x = *coordinates_at;
	            		if(!bit_x_dual_set)
	            		{
	            			x = -x;
	            		}
	            	}
	            	if(same_as_previous)
	            	{
	            		//nothing
	            		x_offset = 0;
	            		x = 0;
	            	}
	            	else if(!bit_x_short_set && !bit_x_dual_set)
	            	{
	            		x_offset = sizeof(i16);
	            		x = *(i16 *)coordinates_at;
	            		endian_swap_i16(&x);
	            	}
	            	coordinates_at += x_offset;
	            	offset_to_y_coordinates   += x_offset;

	            	temporary_x_coordinates_array[f] = x;
	            	log_contour_count++;
					coordinates->x = x;

	            	flag_index++;
	            }
				//
				// y coordinates
				//

				flag_index = 0;
				contour_y_coordinates = contour_x_coordinates + offset_to_y_coordinates;

	            coordinates_at = contour_y_coordinates;
	            log_contour_count = 0;

	            for(u32 f = 0;
	            		f < flags_length;
	            		f++)
	            {
	            	u32 coordinates_array_offset = 0;

					ttf_glyph_point *coordinates = next_glyph_info->point_coordinates + f;
	            	u8 current_flag      = coordinates->flags; 
	            	u32 bit_y_short_set  = current_flag & simple_glyph_y_short_vector;
	            	u32 bit_y_dual_set   = current_flag & simple_glyph_y_is_same_or_positive_y_short_vector;
	            	u32 same_as_previous = !bit_y_short_set && bit_y_dual_set;

	            	u8 repeat_amount = 1;

	            	//is one byte long
	            	if(bit_y_short_set)
	            	{
	            		coordinates_array_offset = sizeof(u8);
	            		x = *coordinates_at;
	            		if(!bit_y_dual_set)
	            		{
	            			x = -x;
	            		}
	            	}
	            	if(same_as_previous)
	            	{
	            		//nothing
	            		coordinates_array_offset = 0;
	            		x = 0;
	            	}
	            	else if(!bit_y_short_set && !bit_y_dual_set)
	            	{
	            		coordinates_array_offset = sizeof(i16);
	            		x = *(i16 *)coordinates_at;
	            		endian_swap_i16(&x);
	            	}
	            	coordinates_at += coordinates_array_offset;
	            	temporary_y_coordinates_array[f] = x;
					coordinates->y = x;


	            	flag_index++;
	            }

				temporary_area_end(&temporary_coordinates_area);


				//store the coordinates on the current processing glyph

				i32 current_x_offset = 0;
				i32 current_y_offset = 0;
				for(u32 f = 0;
						f < flags_length;
						f++)
				{
					ttf_glyph_point *coordinates = next_glyph_info->point_coordinates + f;
					//coordinates->x = temporary_x_coordinates_array[f];
					//coordinates->y = temporary_y_coordinates_array[f];
					//coordinates->flags = glyph_flags[f];

					current_x_offset += coordinates->x;
					current_y_offset += coordinates->y;

					stream_pushf(
							info_stream,
							"Stored coordinates (%d, %d), At (%d, %d)",
							coordinates->x,
							coordinates->y,
							current_x_offset,
							current_y_offset);
				}

			}
			else //composite or compound glyphs
			{
				//NotImplemented;
				u8 *memory_at = memory_advance_from(
						glyph_headers,
						offset_to_description);

				u16 composite_flags = 0;
				do{
				    //read as composite glyph
					f32 component_scale_x = 1.0f;
					f32 component_scale_y = 1.0f;
					i16 component_scale_16 = 0;

				    composite_flags = *(u16 *)memory_at;
				    memory_at += sizeof(u16);
				    u16 glyph_index =  *(u16 *)memory_at;
					memory_at += sizeof(u16);

					endian_swap_16(&composite_flags);
					endian_swap_16(&glyph_index);

					u8 *arg1 = 0;
					u8 *arg2 = 0;
					i32 x = 0;
					i32 y = 0;

					if(composite_flags & component_glyph_arg_1_and_2_are_words)
					{
						arg1 = memory_at + 0;
						arg2 = memory_at + 2;
						memory_at += sizeof(i16) * 2;
						if(composite_flags & component_glyph_args_are_xy_values)
						{
							i16 mem_x = *(i16 *)arg1;
							i16 mem_y = *(i16 *)arg2;
							endian_swap_i16(&mem_x);
							endian_swap_i16(&mem_y);

							x = mem_x;
							y = mem_y;
						}
						else
						{
							u16 mem_x = *(u16 *)arg1;
							u16 mem_y = *(u16 *)arg2;
							endian_swap_16(&mem_x);
							endian_swap_16(&mem_y);

							x = mem_x;
							y = mem_y;
						}
						
					}
					else if(composite_flags & component_glyph_we_have_a_scale)
					{
						component_scale_16 = *(i16 *)memory_at;
						u32 s = 0;
						component_scale_x = ttf_f2dot24_to_float(component_scale_16);
						component_scale_y = component_scale_x;
					}
					else if(composite_flags & component_glyph_we_have_an_x_and_y_scale)
					{
						i16 scale_x = *(i16 *)memory_at;
						i16 scale_y = *(i16 *)(memory_at + sizeof(u16));
						u32 s = 0;
						component_scale_x = ttf_f2dot24_to_float(scale_x);
						component_scale_y = ttf_f2dot24_to_float(scale_y);;
					}
				}while(composite_flags & component_glyph_more_components);
			}
#endif
		}

	}
}

inline u32
ttf_get_glyph_code_by_index(ttf_file_work *file_work, u32 glyph_index);

//not required table
inline u32
ttf_read_gpos_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{

	u32 success = 0;

	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'GPOS');

	if(offset_to_table)
	{
	    u32 feature_variations_offset = 0;
	    u32 gpos_data_offset = offset_to_table;

	    ttf_glyph_positioning_header *gpos_header = (ttf_glyph_positioning_header *)(font_file_memory + offset_to_table);
	    gpos_data_offset += sizeof(ttf_glyph_positioning_header);

	    endian_swap_16(&gpos_header->v0);
	    endian_swap_16(&gpos_header->v1);
	    //endian_swap_16(&gpos_header->script_list_offset);
	    //endian_swap_16(&gpos_header->feature_list_offset);
		//I only support this table for now since I can get the kerning pairs from it.
	    endian_swap_16(&gpos_header->lookup_list_offset);

	    stream_pushf(
	    		info_stream,
	    		"Found GPOS table of version %u.%u",
	    		gpos_header->v0,
	    		gpos_header->v1);

	    Assert((gpos_header->v0 == 1) && (gpos_header->v1 == 1 || gpos_header->v1 == 0));

		/*
		   GPOS starts with a header, and it contains offsets to different tables from the start 
		   of said header. 
		   Lookup list is the only one supported at the moment because its used to get kerning pairs. It
		   contains offsets to lookup tables starting from the list.
		   */
		
		if(gpos_header->v1 == 0)
		{
			success = 1;
			//maybe this allocation is too soon
			u32 first_code_point = file_work->requested_glyph_s;
			u32 last_code_point  = file_work->requested_glyph_e;
			u32 glyph_count      = last_code_point - first_code_point + 1;
			file_work->kerning_pairs = ttf_allocate_kerning_table(
					area,
					first_code_point,
					last_code_point);

			u32 first_requested_glyph_index = file_work->glyph_indices[first_code_point];
			u32 last_requested_glyph_index  = file_work->glyph_indices[last_code_point];
		    //get lookup list table for reading lookup tables
			u32 offset_to_lookup_list = gpos_header->lookup_list_offset;

			//get to the lookup list
			u8 *lookup_list_table_mem = memory_advance_from(gpos_header, offset_to_lookup_list); 

			//the count of lookup tables
			u16 lookup_count = *(u16 *)(lookup_list_table_mem);
			endian_swap_16(&lookup_count);
			//offsets from the lookup list (lookup_list_table_mem)
			u16 *lookup_offsets = (u16 *)(lookup_list_table_mem + sizeof(u16));

			for(u32 l = 0;
					l < lookup_count;
					l++)
			{
				u16 offset_to_lookup_table = lookup_offsets[l];
				endian_swap_16(&offset_to_lookup_table);

				ttf_lookup_table lookup_table = *(ttf_lookup_table *)(lookup_list_table_mem + offset_to_lookup_table);
				endian_swap_16(&lookup_table.lookup_type);
				endian_swap_16(&lookup_table.lookup_flag);
				endian_swap_16(&lookup_table.sub_table_count);
				//offsets from te start of the lookup table
				u16 *sub_table_offsets = (u16 *)(lookup_list_table_mem + offset_to_lookup_table + sizeof(ttf_lookup_table));
				//skip the lookup table and its array


				//read subtables 
				if(lookup_table.lookup_type == 2) //only for kerning pair
				{
					for(u32 st = 0;
							st < lookup_table.sub_table_count;
							st++)
					{
					    u16 offset_to_pair_pos_table = sub_table_offsets[st];
					    endian_swap_16(&offset_to_pair_pos_table);

						//pair pos subtable
				        u8 *subtable_memory_at = (lookup_list_table_mem + offset_to_lookup_table + offset_to_pair_pos_table);

						//pos format
					
					    u16 format = *(u16 *)(subtable_memory_at + 0);
					    endian_swap_16(&format);


						//only format supported
					    if(format == 1)
					    {
					    	//read pair pos format 1
					    	//offset from the beginning of the pairPos table
					    	u16 coverage_offset = *(u16 *)(subtable_memory_at + 2);

					    	u16 value_format1 = *(u16 *)(subtable_memory_at + 4);
					    	u16 value_format2 = *(u16 *)(subtable_memory_at + 6);

					    	u16 pair_set_count    = *(u16 *)(subtable_memory_at + 8);
							//from the start of the pair pos subtable
					    	u16 *pair_set_offsets = (u16 *)(subtable_memory_at + 10);


					    	endian_swap_16(&coverage_offset);
					    	endian_swap_16(&pair_set_count);
					    	endian_swap_16(&value_format1);
					    	endian_swap_16(&value_format2);

							//for now, only support x_advance for the first format
					    	if(value_format1 != 4 || value_format2 != 0)
					    	{
					    		return(0);
					    	}

					    	//use coverage offset and get the table
							//SUPPORT MORE COVERAGE FORMATS
                            ttf_coverage_table first_glyphs_coverage = *(ttf_coverage_table *)(subtable_memory_at + coverage_offset);
					    	endian_swap_16(&first_glyphs_coverage.coverage_format);

							if(first_glyphs_coverage.coverage_format == 1)
							{
								NotImplemented;
					    	    endian_swap_16(&first_glyphs_coverage.glyph_count);
					    	    u16 *first_glyph_ids = (u16 *)(subtable_memory_at + coverage_offset + sizeof(ttf_coverage_table));

					    	    //use pair_set_count
					    	    for(u32 p = 0;
					    	    		p < pair_set_count;
					    	    		p++)
					    	    {
					    	    	u16 offset_to_pair_set = pair_set_offsets[p];
					    	    	endian_swap_16(&offset_to_pair_set);
					    	    	//read pair sets
					    	    	u16 pair_value_count = *(u16 *)(subtable_memory_at + offset_to_pair_set);
					    	    	//read pair value records after this number
					    	    	offset_to_pair_set += sizeof(u16);
					    	    	for(u16 pv = 0;
					    	    			pv < pair_value_count;
					    	    			pv++)
					    	    	{
					    	    		u16 first_glyph  = first_glyph_ids[pv];
					    	    	    u16 second_glyph = *(u16 *)(subtable_memory_at + offset_to_pair_set);
					    	    		endian_swap_16(&first_glyph);
					    	    		endian_swap_16(&second_glyph);
					    	    		offset_to_pair_set += sizeof(u16);

					    	    		//ttf_value_record value_record0 = *(ttf_value_record *)(subtable_memory_at + offset_to_pair_set);
					    	    		offset_to_pair_set += sizeof(ttf_value_record);

					    	    		//ttf_value_record value_record1 = *(ttf_value_record *)(subtable_memory_at + offset_to_pair_set);
					    	    		offset_to_pair_set += sizeof(ttf_value_record);
					    	    	}

					    	    }
							}
							else if(first_glyphs_coverage.coverage_format == 2)
							{
					    	    endian_swap_16(&first_glyphs_coverage.range_count);
								//read the array that follows the coverage table
								ttf_coverage_range_record *range_records = 
									(ttf_coverage_range_record *)(subtable_memory_at + coverage_offset + sizeof(ttf_coverage_table));

					    	    //use pair_set_count
					    	    for(u32 p = 0;
					    	    		p < pair_set_count;
					    	    		p++)
					    	    {
					    	    	u16 offset_to_pair_set = pair_set_offsets[p];
					    	    	endian_swap_16(&offset_to_pair_set);
					    	    	//read pair sets
					    	    	u16 pair_value_count = *(u16 *)(subtable_memory_at + offset_to_pair_set);
									endian_swap_16(&pair_value_count);
					    	    	//read pair value records after this number
					    	    	offset_to_pair_set += sizeof(u16);
									u32 size_of_pair_value_record = sizeof(u16) + (sizeof(ttf_value_record) * 2);

									//get the first indices for each pair on the coverage table
									for(u32 r = 0;
											r < first_glyphs_coverage.range_count;
											r++)
									{
                                        ttf_coverage_range_record range_record = range_records[r];
										endian_swap_16(&range_record.start_glyph_id);
										endian_swap_16(&range_record.end_glyph_id);
										u32 first_glyph_index = range_record.start_glyph_id;
										while(first_glyph_index <= range_record.end_glyph_id)
										{
										    if(first_glyph_index > glyph_count)
										    {
										    	break;
										    }

										    u32 first_glyph_covered_code = file_work->glyph_codes[first_glyph_index];
									        u32 first_glyph_is_in_requested_range = (first_glyph_covered_code >= first_code_point) &&
									        		                	            (first_glyph_covered_code <= last_code_point);
									        if(!first_glyph_is_in_requested_range)
									        {
												first_glyph_index++;
									        	//skip glyph
									        	continue;
									        }

									        //pair value records array
					    	    	        for(u16 pv = 0;
					    	    	        		pv < pair_value_count;
					    	    	        		pv++)
					    	    	        {
									        	u32 pair_data_offset = offset_to_pair_set + (size_of_pair_value_record * pv);

					    	    	            u16 second_glyph = *(u16 *)(subtable_memory_at + pair_data_offset);
					    	    	        	endian_swap_16(&second_glyph);
					    	    	        	pair_data_offset += sizeof(u16);

												if(second_glyph > glyph_count)
												{
													continue;
												}
												u32 second_glyph_code = file_work->glyph_codes[second_glyph]; 
									        	//second glyph is in requested range
									            u32 second_glyph_is_in_requested_range = (second_glyph_code >= first_code_point) &&
									        		                	                 (second_glyph_code <= last_code_point);
									        	if(!second_glyph_is_in_requested_range)
									            {
									        		//skip glyph
									        		continue;
									        	}


					    	    	            ttf_value_record value_record0 = *(ttf_value_record *)(subtable_memory_at + pair_data_offset);
					    	    	        	pair_data_offset += sizeof(ttf_value_record);

					    	    	        	ttf_value_record value_record1 = *(ttf_value_record *)(subtable_memory_at + pair_data_offset);
					    	    	        	pair_data_offset += sizeof(ttf_value_record);

									        	endian_swap_i16(&value_record0.x_advance);
									        	endian_swap_i16(&value_record0.x_placement);
									        	endian_swap_i16(&value_record1.x_advance);
									        	endian_swap_i16(&value_record1.x_placement);
									        	if(value_record0.x_advance < 0)
									        	{
									        		int s = 0;
									        	}
									        	//first glyph is in requested range
									        	u32 f_c = first_glyph_covered_code - first_code_point;
									        	u32 s_c = second_glyph_code - first_code_point;
												if(f_c == 0)
												{
													int s = 0;
												}

						                        u32 kerning_index = (f_c * glyph_count) + s_c;
									        	file_work->kerning_pairs[kerning_index] = value_record0.x_advance;
					    	    	        }

									        first_glyph_index++;
										}
									}

					    	    }
							}
							else
							{
								//something went wrong
								Assert(0);
							}

					    	//Assert(first_glyphs_coverage.coverage_format == 1);
					    }
					    else if(format == 2)
					    {
					    }
					    else
					    {
					    	//bad version
					    	Assert(0);
					    }
					}
				}
			}
		}
	    //version 1.1
		else if(gpos_header->v1 == 1)
	    {
			//not supported
			NotImplemented;
		}
	}
	return(success);
}

inline void
ttf_read_loca_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'loca');
	Assert(offset_to_table);

	stream_pushf(
			info_stream, 
			"Reading the loca table with version %d",
			file_work->index_to_loc_format);
	Assert(file_work->index_to_loc_format == 0 ||
			file_work->index_to_loc_format == 1);

	file_work->loca_offsets = font_file_memory + offset_to_table;
	//process_glyf_table = 1;
}

inline void
ttf_read_post_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
}

inline void
ttf_read_os2_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{

	return;
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'OS/2');
	Assert(offset_to_table);

	u16 version = *(u16 *)(font_file_memory + offset_to_table);
	endian_swap_16(&version);
	ttf_windows_metrics_table *metrics_table = (ttf_windows_metrics_table *)(font_file_memory + offset_to_table);

	endian_swap_i16(&metrics_table->average_character_width);

	endian_swap_16(&metrics_table->us_weight_class);
	endian_swap_16(&metrics_table->us_width_class);
	endian_swap_16(&metrics_table->type_flags);

	//endian_swap_i16_until(&metrics_table->average_character_width);
	endian_swap_i16(&metrics_table->subscript_horizontal_size);
	endian_swap_i16(&metrics_table->subscript_vertical_size);
	endian_swap_i16(&metrics_table->subscript_offset_x);
	endian_swap_i16(&metrics_table->subscript_offset_y);
	endian_swap_i16(&metrics_table->superscript_horizontal_size);
	endian_swap_i16(&metrics_table->superscript_vertical_size);
	endian_swap_i16(&metrics_table->superscript_offset_x);
	endian_swap_i16(&metrics_table->superscript_offset_y);
	endian_swap_i16(&metrics_table->strikeout_size);
	endian_swap_i16(&metrics_table->strikeout_position);
	endian_swap_i16(&metrics_table->family_class);

	endian_swap_32(&metrics_table->unicode_range1);
	endian_swap_32(&metrics_table->unicode_range2);
	endian_swap_32(&metrics_table->unicode_range3);
	endian_swap_32(&metrics_table->unicode_range4);

	endian_swap_16(&metrics_table->font_selection_flags);

	endian_swap_16(&metrics_table->first_char_index);
	endian_swap_16(&metrics_table->last_char_index);

	endian_swap_i16(&metrics_table->typographic_ascender);
	endian_swap_i16(&metrics_table->typographic_descender);
	endian_swap_i16(&metrics_table->typographic_line_gap);

	endian_swap_16(&metrics_table->windows_ascender);
	endian_swap_16(&metrics_table->windows_descender);

	ttf_windows_metrics_table_footer *metrics_table_footer = memory_advance_from(
			metrics_table,
			sizeof(ttf_windows_metrics_table));


	if(version == 5)
	{
	    endian_swap_16(&metrics_table_footer->lower_optical_point_size);
	    endian_swap_16(&metrics_table_footer->upper_optical_point_size);
	    endian_swap_32(&metrics_table_footer->code_page_character_range0);
	    endian_swap_32(&metrics_table_footer->code_page_character_range1);
		NotImplemented;
	}
	else if(version == 4)
	{
	    endian_swap_i16(&metrics_table_footer->sx_height);
	    endian_swap_i16(&metrics_table_footer->s_cap_height);

	    endian_swap_16(&metrics_table_footer->default_char);
	    endian_swap_16(&metrics_table_footer->default_break_char);
	    endian_swap_16(&metrics_table_footer->max_context_length);
	}
	else if(version == 3)
	{

	    endian_swap_i16(&metrics_table_footer->sx_height);
	    endian_swap_i16(&metrics_table_footer->s_cap_height);

	    endian_swap_16(&metrics_table_footer->default_char);
	    endian_swap_16(&metrics_table_footer->default_break_char);
	    endian_swap_16(&metrics_table_footer->max_context_length);
	}
	else if(version == 2)
	{
	}
	else if(version == 1)
	{
	}
	else if(version == 0)
	{
	}
	else
	{
		//bad version, log and cancel.
		Assert(0);
	}

}

inline void
ttf_read_name_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{

	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'name');
	Assert(offset_to_table);
	ttf_name_table *name_table = (ttf_name_table *)(font_file_memory + offset_to_table);

	endian_swap_16(&name_table->v);
	endian_swap_16(&name_table->name_record_count);
	endian_swap_16(&name_table->offset_to_storage);

	ttf_name_record *name_records = memory_advance_from(
			name_table,
			sizeof(ttf_name_table));

	//every records reads from here
	u8 *string_storage = memory_advance_from(
			name_table,
			name_table->offset_to_storage);

	u16 version = name_table->v;
	stream_pushf(
			info_stream,
			"Got to the 'name' table with version %u",
			version);
	if(version == 0)
	{
		for(u32 n = 0;
				n < name_table->name_record_count;
				n++)
		{
			ttf_name_record *current_name_record = name_records + n;

			endian_swap_16(&current_name_record->platform_id);
			endian_swap_16(&current_name_record->encoding_id);
			endian_swap_16(&current_name_record->language_id);

			endian_swap_16(&current_name_record->name_id);
			endian_swap_16(&current_name_record->string_length);
			endian_swap_16(&current_name_record->string_offset);

			//only on version 0 of the naming table
			Assert(current_name_record->language_id < 0x8000);
			Assert(current_name_record->name_id < 26);

			u8 *record_name = string_storage + current_name_record->string_offset;

			//for later use
			if(0)
			{
		     	u8 name_storage[10000] = {0};
		     	ttf_decode_name_to(
		     			record_name,
		     			name_storage,
		     			current_name_record->string_length);

		     	stream_pushf(info_stream,
		     			"Got to record %u with name id %u and the following text:\n%s",
		     			n,
		     			current_name_record->name_id,
		     			name_storage);
			}
		}
	}
	else if(version == 1)
	{
		u16 *language_tag_record_count = (u16 *)memory_advance_from(
				name_records,
				sizeof(ttf_name_record) * name_table->name_record_count);

		ttf_name_language_tag_record *language_tag_records = memory_advance_from(
				language_tag_record_count,
				sizeof(u16));

		NotImplemented;
	}
	else
	{
		//bad version, log or cancel.
		Assert(0);
	}
}

//after hhea
inline void
ttf_read_hmtx_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'hmtx');
	Assert(offset_to_table);

	//read metrics depending on number_of_horizontal_metrics
	//Can it be 0?
	u32 number_of_horizontal_metrics = file_work->number_of_horizontal_metrics;
	stream_pushf(
			info_stream,
			"Processing hmtx table with %u number of horizontal metrics",
			number_of_horizontal_metrics);
	Assert(number_of_horizontal_metrics);

	if(number_of_horizontal_metrics)
	{
	    ttf_horizontal_metrics_record *horizontal_metrics_records = (ttf_horizontal_metrics_record *)(font_file_memory + offset_to_table);
	    i16 *left_side_bearings = memory_advance_from(
	    		font_file_memory,
	    		offset_to_table + (sizeof(ttf_horizontal_metrics_record) * number_of_horizontal_metrics));

		file_work->horizontal_metrics_records = horizontal_metrics_records;
		file_work->left_side_bearings        = left_side_bearings;

	    for(u32 h = 0;
	    		h < number_of_horizontal_metrics;
	    		h++)
	    {
	    	//needed ?
			endian_swap_16(&horizontal_metrics_records[h].advance_width);
			endian_swap_i16(&horizontal_metrics_records[h].lsb);

			endian_swap_i16(left_side_bearings + h);
	    }
	}

}

inline void
ttf_read_hhea_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{

	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'hhea');
	Assert(offset_to_table);

	ttf_horizontal_layout_table *horizontal_layout_table = (ttf_horizontal_layout_table *)(font_file_memory + offset_to_table);
	stream_pushf(
			info_stream,
			"Reading hhea header...");

	endian_swap_16(&horizontal_layout_table->v0);
	endian_swap_16(&horizontal_layout_table->v1);

	//only used in apple
	endian_swap_i16(&horizontal_layout_table->ascender);
	endian_swap_i16(&horizontal_layout_table->descender);
	endian_swap_i16(&horizontal_layout_table->line_gap);

	endian_swap_16(&horizontal_layout_table->advance_width_max);

	endian_swap_i16(&horizontal_layout_table->min_left_side_bearing);
	endian_swap_i16(&horizontal_layout_table->min_right_side_bearing);
	endian_swap_i16(&horizontal_layout_table->max_extent);
	endian_swap_i16(&horizontal_layout_table->caret_slope_rise);
	endian_swap_i16(&horizontal_layout_table->caret_slope_run);
	endian_swap_i16(&horizontal_layout_table->caret_offset);

	endian_swap_i16(&horizontal_layout_table->metric_data_format);
	endian_swap_16(&horizontal_layout_table->number_of_horizontal_metrics);

	Assert(horizontal_layout_table->v0 == 1 && horizontal_layout_table->v1 == 0);

	file_work->number_of_horizontal_metrics = horizontal_layout_table->number_of_horizontal_metrics;
	//file_work->font_total_height = (horizontal_layout_table->ascender - horizontal_layout_table->descender);
	file_work->baseline_ascender  = horizontal_layout_table->ascender;
	file_work->baseline_descender = horizontal_layout_table->descender;
	file_work->baseline_line_gap  = horizontal_layout_table->line_gap;

}

inline void
ttf_read_head_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'head');
	Assert(offset_to_table);

	ttf_head_table *head_table = (ttf_head_table *)(font_file_memory + offset_to_table);
	endian_swap_16(&head_table->v0);
	endian_swap_16(&head_table->v1);
	//no swap for f32 I guess
	//unless I cast the pointer without loosing information and recast ?
	endian_swap_32(&head_table->checksum_adjustment);
	endian_swap_32(&head_table->signature);
	endian_swap_16(&head_table->global_flags);
	endian_swap_16(&head_table->units_per_em);
	// ;todo
	//endian_swap_i64();
	//endian_swap_i64();

	//used for glyph's boxes and 'glyph' table
	endian_swap_i16(&head_table->min_x);
	endian_swap_i16(&head_table->min_y);
	endian_swap_i16(&head_table->max_x);
	endian_swap_i16(&head_table->max_y);
	
	endian_swap_16(&head_table->mac_style);
	endian_swap_16(&head_table->smallest_readable_size);

	endian_swap_i16(&head_table->font_direction_hint);
	endian_swap_i16(&head_table->index_to_loc_format);
	endian_swap_i16(&head_table->glyph_data_format);

	Assert(head_table->v0 == 1 && head_table->v1 == 0);
	Assert(head_table->signature == true_type_SIGNATURE);

	stream_pushf(
			info_stream,
			"Head version is %u.%u",
			head_table->v0,
			head_table->v1);

	file_work->units_per_em = head_table->units_per_em;
	file_work->min_x        = head_table->min_x;
	file_work->min_y        = head_table->min_y;
	file_work->max_x        = head_table->max_x;
	file_work->max_y        = head_table->max_y;
	file_work->index_to_loc_format = head_table->index_to_loc_format;
	file_work->global_flags = head_table->global_flags;

}

inline void
ttf_read_maxp_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work,
			'maxp');
	Assert(offset_to_table);

	u32 version = *(u32 *)(font_file_memory + offset_to_table);
	endian_swap_32(&version);

	u16 v0 = ((version >> 16) & 0xffff);
	u16 v1 = (version & 0xffff);

	//version 1.0 for true type fonts
	if(v0 == 1 && v1 == 0)
	{
		ttf_maxp_table_1_0 *maximum_profile_table = (ttf_maxp_table_1_0 *)(font_file_memory + offset_to_table);

		endian_swap_16(&maximum_profile_table->number_of_glyphs);
		endian_swap_16(&maximum_profile_table->max_points);
		endian_swap_16(&maximum_profile_table->max_contours);
		endian_swap_16(&maximum_profile_table->max_composite_points);
		endian_swap_16(&maximum_profile_table->max_composite_contours);

		endian_swap_16(&maximum_profile_table->max_zones);
		endian_swap_16(&maximum_profile_table->max_twilight_points);
		endian_swap_16(&maximum_profile_table->max_storage);
		endian_swap_16(&maximum_profile_table->max_function_defs);
		endian_swap_16(&maximum_profile_table->max_instruction_defs);

		endian_swap_16(&maximum_profile_table->max_stack_elements);
		endian_swap_16(&maximum_profile_table->max_size_of_instructions);
		endian_swap_16(&maximum_profile_table->max_component_elements);
		endian_swap_16(&maximum_profile_table->max_component_depth);

		file_work->number_of_glyphs = maximum_profile_table->number_of_glyphs;

	}
	else if(v0 == 0 && v1 == 5)
	{
		file_work->number_of_glyphs = *(u16 *)(font_file_memory + 
				offset_to_table + 
				sizeof(u32));
		
		endian_swap_16(&file_work->number_of_glyphs);

	}
	else
	{
		//bad version
		Assert(0);
	}
}

inline void
ttf_read_cmap_table(
		memory_area *area,
		u8 *font_file_memory,
		ttf_file_work *file_work,
		stream_data *info_stream)
{
	u32 offset_to_table = ttf_get_offset_to_table(
			file_work, 'cmap');
	Assert(offset_to_table);

	//read maxp first to get this number
	Assert(file_work->number_of_glyphs);
	file_work->glyph_indices = memory_area_push_array(
			area, u16, file_work->number_of_glyphs);
	file_work->glyph_codes = memory_area_push_array(
			area, u16, file_work->number_of_glyphs);


	u32 cmap_data_offet = offset_to_table;
	ttf_cmap_header *cmap_header = (ttf_cmap_header *)(font_file_memory + offset_to_table);
	endian_swap_16(&cmap_header->version);
	endian_swap_16(&cmap_header->number_of_encoding_tables);

	stream_pushf(
			info_stream,
			"Got to the cmap header with version %u and encoding tables %u!",
			cmap_header->version,
			cmap_header->number_of_encoding_tables);
	//needed?
	Assert(cmap_header->version == 0);

	//get the encoding tables array
	ttf_cmap_encoding_record *cmap_encoding_record_array = 
		(ttf_cmap_encoding_record *)(font_file_memory +
				offset_to_table + 
				sizeof(ttf_cmap_header));
	for(u32 e = 0;
			e < cmap_header->number_of_encoding_tables;
			e++)
	{
	    ttf_cmap_encoding_record *encoding_record = cmap_encoding_record_array + e;

	    endian_swap_16(&encoding_record->platform_id);
	    endian_swap_16(&encoding_record->platform_encoding_id);
	    endian_swap_32(&encoding_record->subtable_offset);

		u16 subtable_format = *(u16 *)memory_advance_from(
				cmap_header,
				encoding_record->subtable_offset);
		endian_swap_16(&subtable_format);

	//    stream_pushf(
	//    		info_stream,
	//    		"Got to the cmap record with encodings %u, %u! and subtable format %u",
	//    		encoding_record->platform_id,
	//    		encoding_record->platform_encoding_id,
	//			subtable_format);

		switch(encoding_record->platform_id)
		{
			//loop through them all and depending on the platform, choose the needed one
			//unicode
			case 0:
				{
					Assert(encoding_record->platform_encoding_id >= 3 &&
						   encoding_record->platform_encoding_id <= 6);
					//NotImplemented;
				}break;
			//macintosh
			case 1:
				{
					Assert(encoding_record->platform_encoding_id != 1);
					//NotImplemented;
				}break;
			//windows
			case 3:
				{
					Assert((encoding_record->platform_encoding_id >= 0 &&
						   encoding_record->platform_encoding_id <= 6) ||
							encoding_record->platform_encoding_id == 10);
					if(subtable_format == 4)
					{
						cmap_subtable_format_4 *cmap_subtable = memory_advance_from(
				            cmap_header,
				            encoding_record->subtable_offset);

						endian_swap_16(&cmap_subtable->subtable_format);
						endian_swap_16(&cmap_subtable->sub_table_length);
						endian_swap_16(&cmap_subtable->language);
						endian_swap_16(&cmap_subtable->segment_count_x2);
						endian_swap_16(&cmap_subtable->search_range);
						endian_swap_16(&cmap_subtable->entry_selector);
						endian_swap_16(&cmap_subtable->range_shift);

						u16 segment_count = cmap_subtable->segment_count_x2 / 2;

						u32 offset_to_arrays = sizeof(cmap_subtable_format_4);

						u16 *end_codes = memory_advance_from(
								cmap_subtable,
								offset_to_arrays);
						//advance by the array amount and the reserved number
						offset_to_arrays += (segment_count * sizeof(u16)) + sizeof(u16);

						u16 *start_codes = memory_advance_from(
								cmap_subtable,
								offset_to_arrays);

						offset_to_arrays += (segment_count * sizeof(u16));

						i16 *id_deltas = memory_advance_from(
								cmap_subtable,
								offset_to_arrays);

						offset_to_arrays += (segment_count * sizeof(i16));

						u16 *id_array_range_offsets = memory_advance_from(
								cmap_subtable,
								offset_to_arrays);

						offset_to_arrays += (segment_count * sizeof(u16));
						//then the next array is an arbitrary length array
						//of character indices
						u16 *glyph_ids_array = memory_advance_from(
								cmap_subtable,
								offset_to_arrays);



							//stream_pushf(info_stream,
							//		"Start and end codes are %u, %u. id deltas and range offsets %d, %u.\n The next character to encode is idk",
							//		s_code,
							//		e_code,
							//		id_delta,
							//		id_range_offset
							//		);

						//segment loop
						for(u32 s = 0;
								s < segment_count;
								s++)
						{
							//searc for the first end_code that >= next_character_code
							u16 s_code              = start_codes[s];
							u16 e_code              = end_codes[s];
							i16 id_delta            = id_deltas[s];
							u16 id_range_offset     = id_array_range_offsets[s];
							u16 next_character_code = 0;
							endian_swap_16(&s_code);
							endian_swap_16(&e_code);
							endian_swap_i16(&id_delta);
							endian_swap_16(&id_range_offset);
							endian_swap_16(&next_character_code);

							//s_code += id_delta;
							//e_code += id_delta;

							//character inside segments loop
							for(u32 c = s_code;
									c <= e_code;
									c++)
							{
							    next_character_code      = c;
								u16 next_character_index = c + id_delta;
								u32 glyph_id = 0;
							    if(id_range_offset != 0)
							    {
							    	//do a thing
									next_character_index = *(id_range_offset / 2 +
											(next_character_code - s_code) +
											&id_array_range_offsets[s]);

									next_character_index = glyph_ids_array[next_character_code];

									endian_swap_16(&next_character_index);
							    }
							//	stream_pushf(
							//			info_stream,
							//			"The next character to encode is %c! at index %u",
							//			next_character_code,
							//			next_character_index);

								file_work->glyph_indices[next_character_code] = next_character_index;
								file_work->glyph_codes[next_character_index] = next_character_code;
							}
						}

							stream_pushf(info_stream,
									"\n\nEnd of cmap table\n\n");
					}
					else
					{
						NotImplemented;
					}
				}break;
			default:
				{
					//version not supported
					Assert(0);
				}break;
		}
	}


				

}

static ttf_file_work 
ttf_parse_from_memory(
		memory_area *area,
		u32 first_glyph_code,
		u32 last_glyph_code,
		u8 *font_file_memory,
		stream_data *info_stream)
{
	ttf_table *initial_table = (ttf_table *)font_file_memory;
	u32 data_offset          = sizeof(ttf_table);

	endian_swap_32(&initial_table->sfnt_version);
	endian_swap_16(&initial_table->number_of_tables);
	endian_swap_16(&initial_table->search_range);
	endian_swap_16(&initial_table->entry_selector);
	endian_swap_16(&initial_table->range_shift);

	Assert(ttf_check_signature_u32_swapped(initial_table->sfnt_version));

	stream_pushf(
			info_stream,
			"sfnt version is %c%c%c%c or %u",
			initial_table->sfnt_version_chars[0],
			initial_table->sfnt_version_chars[1],
			initial_table->sfnt_version_chars[2],
			initial_table->sfnt_version_chars[3],
			initial_table->sfnt_version
			);
	u32 r = 0;
	u32 parent_table_offset = 0;


	//data for parsing
	u16 number_of_horizontal_metrics = 0;
	u16 units_per_em = 0;
	i16 min_x = 0;
	i16 min_y = 0;
	i16 max_x = 0;
	i16 max_y = 0;
	i16 index_to_loc_format = 0;

	u32 process_glyf_table = 0;
	u32 offset_to_glyf_table = 0;

	ttf_file_work file_work = {0};
	file_work.requested_glyph_s   = first_glyph_code; 
	file_work.requested_glyph_e   = last_glyph_code;
	file_work.table_records_count = initial_table->number_of_tables;
	file_work.info_stream = info_stream;

	if(file_work.requested_glyph_s > file_work.requested_glyph_e)
	{
		u32 glyph_copy = file_work.requested_glyph_e;
		file_work.requested_glyph_e = file_work.requested_glyph_s;
        file_work.requested_glyph_s = glyph_copy;
	}
	//set or allocate table records array
	file_work.table_records = (ttf_record *)(font_file_memory + data_offset);

	while(r <  initial_table->number_of_tables)
	{

		//get records 
        ttf_record *record = (ttf_record *)(
         	   font_file_memory + data_offset);
		data_offset += sizeof(ttf_record);

		stream_pushf(
				info_stream,
				"Found table with tag %c%c%c%c",
				record->tag.value_a[0],
				record->tag.value_a[1],
				record->tag.value_a[2],
				record->tag.value_a[3]
				);
        endian_swap_32(&record->tag.value);
        endian_swap_32(&record->check_sum);
        endian_swap_32(&record->offset);
        endian_swap_32(&record->table_length);
		r++;


	    u32 offset_to_table = parent_table_offset + record->offset;
	}

	//
	//Essential tables
	//

	/*
	   Needed order:

	   maxp
	   cmap
	   hhea
	   hmtx

	   Not needed tables (for my purposes):

	   'gasp'
	   */
	ttf_read_maxp_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_cmap_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_hhea_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_hmtx_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);


	ttf_read_head_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_name_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_os2_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);


	//loca and glyf tables
	ttf_read_loca_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	ttf_read_glyf_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);

	//
	// optional tables
	//

	ttf_read_kern_table(
		area,
		font_file_memory,
        &file_work,
		info_stream);
	if(!file_work.kerning_pairs)
	{
		//try with GPOS?
	    ttf_read_gpos_table(
		    area,
		    font_file_memory,
            &file_work,
		    info_stream);
	}
		//
		// tables
		//
#if 0
		switch(record->tag.value)
		{
                //not required but needed
				case 'glyf': //used alongside 'maxp' and 'loca' table
				{
					offset_to_glyf_table = offset_to_table;
					u16 number_of_glyphs = file_work.number_of_glyphs;
					
					if(number_of_glyphs)
					{
						stream_pushf(
								info_stream,
								"Got to the glyf table with %u glyphs",
								number_of_glyphs);
					}
					else
					{
						stream_pushf(
								info_stream,
								"Could not read glyf table since number_of_glyphs is zero!",
								number_of_glyphs);
					}

				}break;
			case 'GDEF':
				{
					
					u32 gdef_data_offset = offset_to_table;

					ttf_glyph_definition_header *gdef_header = (ttf_glyph_definition_header *)(font_file_memory + gdef_data_offset); 
					endian_swap_16(&gdef_header->major_version);
					endian_swap_16(&gdef_header->minor_version);
					endian_swap_16(&gdef_header->class_definition_offset);
					endian_swap_16(&gdef_header->attachment_point_offset);
					endian_swap_16(&gdef_header->ligature_caret_list_offset);
					endian_swap_16(&gdef_header->mark_attachment_definition_offset);
					//data for 1.2 and 1.3
					u16 mark_glyph_sets_offset = 0;
					u32 item_variation_store_table_offset = 0;

					u16 v0 = gdef_header->major_version;
					u16 v1 = gdef_header->minor_version;
					Assert(v0 == 1 &&
						   (v1 == 0 || v1 == 2 || v1 == 3));


				stream_pushf(
						info_stream,
						"Found GDEF table of version %u.%u",
						v0,
						v1);

					gdef_data_offset += sizeof(ttf_glyph_definition_header);
					if(v1 == 0)
					{
					}
					else if(v1 == 2)
					{
						mark_glyph_sets_offset = *(u16 *)(font_file_memory + gdef_data_offset);
						gdef_data_offset += sizeof(u16);

					}
					else if(v1 == 3)
					{
						mark_glyph_sets_offset = *(u16 *)(font_file_memory + gdef_data_offset);
						gdef_data_offset += sizeof(u16);

						item_variation_store_table_offset = *(u32 *)(font_file_memory + gdef_data_offset);
						gdef_data_offset += sizeof(u16);
					}
					endian_swap_16(&mark_glyph_sets_offset);
					endian_swap_32(&item_variation_store_table_offset);

					//get to the class definitions table
					if(gdef_header->class_definition_offset)
					{
					    gdef_data_offset = offset_to_table + gdef_header->class_definition_offset;

					    ttf_class_definition_2 *class_definition_2 = (ttf_class_definition_2 *)(font_file_memory + gdef_data_offset);
					    endian_swap_16(&class_definition_2->class_format);
					    endian_swap_16(&class_definition_2->class_range_count);

					    //go to the class range array[class_range_count]

					    gdef_data_offset += sizeof(ttf_class_definition_2);
					    ttf_class_range_record *range_record_array = (ttf_class_range_record *)(font_file_memory + gdef_data_offset);

					    //Don't know if this uses the format "1"
					    Assert(class_definition_2->class_format == 2);

					    stream_pushf(
					    		info_stream,
					    		"Got to the class definition table %u!",
					    		class_definition_2->class_format);

					    for(u32 c = 0;
					    		c < class_definition_2->class_range_count;
					    		c++)
					    {
					    	ttf_class_range_record *current_record = range_record_array + c;
					    	endian_swap_16(&current_record->start_glyph_id);
					    	endian_swap_16(&current_record->end_glyph_id);
					    	endian_swap_16(&current_record->class);

					    	stream_pushf(
					    			info_stream,
					    			"This class assigns the glyph %u to %u to the class %u",
					    			current_record->start_glyph_id,
					    			current_record->end_glyph_id,
					    			current_record->class);
					    }
					}

					//
					//go to the attachment points list table
					//
					if(gdef_header->attachment_point_offset)
					{
					    gdef_data_offset = offset_to_table + gdef_header->attachment_point_offset;

					    ttf_attachment_list_table *attachment_list_table = 
					    	(ttf_attachment_list_table *)(font_file_memory + gdef_data_offset);
					    endian_swap_16(&attachment_list_table->coverage_table_offset);
					    endian_swap_16(&attachment_list_table->glyphs_with_attachment_count);

					    //get to the offsets array
					    gdef_data_offset += sizeof(ttf_attachment_list_table);
                        //Array of offsets to AttachPoint tables-from beginning of AttachList
					    //table-in Coverage Index order
					    //this array is got getting the attach points table for each glyph
					    //indicated on the coverage table
					    u16 *attach_points_offsets = (u16 *)(font_file_memory + gdef_data_offset);

					    //get to the coverage table indicated by the attachment table
				//	    gdef_data_offset = ((u8 *)attachkment_list_table + attachment_list_table->coverage_table_offset);
				//	    ttf_coverage_table *attachment_list_coverage_table = 
				//	    	(ttf_coverage_table *)(font_file_memory + gdef_data_offset);

					    ttf_coverage_table *attachment_list_coverage_table = memory_advance_from(
					    		attachment_list_table,
					    		attachment_list_table->coverage_table_offset);
				        endian_swap_16(&attachment_list_coverage_table->coverage_format);
					    endian_swap_16(&attachment_list_coverage_table->glyph_count);

					    //Don't know if this uses the coverage 2
					    Assert(attachment_list_coverage_table->coverage_format == 1);

					    u8 *attachment_list_at = (u8 *)attachment_list_table;
						// ;untested
					     // ;complete
					    for(u32 a = 0;
					    		a < attachment_list_table->glyphs_with_attachment_count;
					    		a++)
					    {
							//endian swap?
					        u16 attachment_data_offset = attach_points_offsets[a];
					    	
                            ttf_attachment_points_table *current_attachment_points = (ttf_attachment_points_table *)
					    		(attachment_list_at + attachment_data_offset);

					    	attachment_data_offset += sizeof(ttf_attachment_points_table);

					    	u16 *attachment_points_indices = (u16 *)
					    		(attachment_list_at + attachment_data_offset);

					    	endian_swap_16(&current_attachment_points->attachment_points_count);
					    	//set attachment points indicated on the 
					    	//coverage table using the same index
					    	//(a)

					    	attachment_data_offset += sizeof(u16);
					    }
					}

					//
					//ligature caret list table
					//
					if(gdef_header->ligature_caret_list_offset)
					{
					    gdef_data_offset = offset_to_table + gdef_header->ligature_caret_list_offset;
					    ttf_ligature_caret_list_table *caret_list_table = (ttf_ligature_caret_list_table *)(font_file_memory + gdef_data_offset);
						endian_swap_16(&caret_list_table->coverage_table_offset);
						endian_swap_16(&caret_list_table->ligature_glyphs_count);
						gdef_data_offset += sizeof(ttf_ligature_caret_list_table);
						u16 *ligature_glyph_table_offsets =
							(u16 *)(font_file_memory + caret_list_table->ligature_glyphs_count);
						

					    //get to the coverage table
					    u32 offset_to_coverage_table = 
					    	(offset_to_table + 
					    	 gdef_header->ligature_caret_list_offset + 
					    	 caret_list_table->coverage_table_offset);

					    ttf_coverage_table *ligature_caret_coverage_table = 
					    	(ttf_coverage_table *)(font_file_memory + offset_to_coverage_table);
					    endian_swap_16(&ligature_caret_coverage_table->coverage_format);
					    endian_swap_16(&ligature_caret_coverage_table->glyph_count);
					    //Don't know if this other one uses the coverage format 2
					    Assert(ligature_caret_coverage_table->coverage_format == 1);

						//untested
						for(u32 l = 0;
								l < caret_list_table->ligature_glyphs_count;
								l++)
						{
							u16 offset_to_ligature_table = ligature_glyph_table_offsets[l];
							endian_swap_16(&offset_to_ligature_table);

							ttf_ligature_table *ligature_table = memory_advance_from(
									caret_list_table,
									offset_to_ligature_table);
							endian_swap_16(&ligature_table->caret_count);
							//get the offsets at the end of the structs
							//it's count is the caret_count
							u16 *caret_value_offsets = (u16 *)(ligature_table + 1);
							//get to the caret tables
							for(u32 c = 0;
									c < ligature_table->caret_count;
									c++)
							{
								u16 offset_to_caret_table = caret_value_offsets[c];
								endian_swap_16(&offset_to_caret_table);

								u16 caret_value_format = *(u16 *)memory_advance_from(
										ligature_table,
										offset_to_caret_table);
								offset_to_caret_table += sizeof(u16);
								endian_swap_16(&caret_value_format);

								Assert(caret_value_format >= 1 && caret_value_format <= 3);

								if(caret_value_format == 1)
								{
									ttf_caret_value_table_1 *caret_value_table = memory_advance_from(
											ligature_table,
											offset_to_caret_table);
								}
								else if(caret_value_format == 2)
								{
									ttf_caret_value_table_2 *caret_value_table = memory_advance_from(
											ligature_table,
											offset_to_caret_table);
								}
								else if(caret_value_format == 3)
								{
									ttf_caret_value_table_3 *caret_value_table = memory_advance_from(
											ligature_table,
											offset_to_caret_table);
									//get the device and variation tables
								}

							}

						}
					}

				}break;
			case 'GPOS':
			{
				u32 feature_variations_offset = 0;
				u32 gpos_data_offset = offset_to_table;

				ttf_glyph_positioning_header *gpos_header = (ttf_glyph_positioning_header *)(font_file_memory + offset_to_table);
				gpos_data_offset += sizeof(ttf_glyph_positioning_header);

				endian_swap_16(&gpos_header->v0);
				endian_swap_16(&gpos_header->v1);
				endian_swap_16(&gpos_header->script_list_offset);
				endian_swap_16(&gpos_header->feature_list_offset);
				endian_swap_16(&gpos_header->lookup_list_offset);

				stream_pushf(
						info_stream,
						"Found GPOS table of version %u.%u",
						gpos_header->v0,
						gpos_header->v1);

				Assert((gpos_header->v0 == 1) && (gpos_header->v1 == 1 || gpos_header->v1 == 0))

			    //version 1.1
				if(gpos_header->v1 == 1)
				{
					feature_variations_offset = *(u32 *)(font_file_memory + gpos_data_offset);
					gpos_data_offset += sizeof(u32);
				}
			}break;
		}


#endif

	return(file_work);
}


//static u32
//ttf_glyph_work_store_glyph_curves(
//		memory_area *curves_area,
//		ttf_glyph_work *glyph_work,
//		ttf_glyph_info *glyph_info)
//{
//}

static u32
ttf_glyph_work_store_glyph_contours(
		memory_area *curves_area,
		ttf_glyph_work *glyph_work,
		ttf_glyph_info *glyph_info)
{

	ttf_glyph_point *point_coordinates = glyph_info->point_coordinates;
	ttf_curve *curves_array = 0;
	//keep stored memory if success, else recover.
	temporary_area recovery_area = temporary_area_begin(curves_area);


	u32 glyph_points_count = glyph_info->point_coordinates_count;
	u32 number_of_contours = glyph_info->number_of_contours;


	//store the number of curves per contour
	glyph_work->contour_count = number_of_contours;
	glyph_work->curve_counts_per_contour = memory_area_clear_and_push_array(
			curves_area, u32, number_of_contours);
	//;for now
	Assert(glyph_points_count);


	//read curves, make sure the first point is ON the curve
    ttf_glyph_point *first_points = point_coordinates + 0;
	u8 p0_flags = first_points->flags;
	f32 p0_x = 0;
	f32 p0_y = 0;
	if(!(p0_flags & simple_glyph_on_curve_point))
	{
        ttf_glyph_point last_points = point_coordinates[glyph_points_count - 1];
		u8 last_point_flag = last_points.flags;

		//the first point of the next curve will be (last - first) / 2
		if(!(last_point_flag & simple_glyph_on_curve_point))
		{
			p0_x = (first_points->x - last_points.x) / 2.0f;
			p0_y = (first_points->y - last_points.y) / 2.0f;
		}
		else
		{
			p0_x = last_points.x;
			p0_y = last_points.y;
		}
	}
	u32 contour_index = 0;
	u32 curve_color   = 0xff0000ff;
	u32 c = 0;
	u32 last_end_index = 0;
	u32 start_index = 0;
	ttf_curve last_curve = {0};

	while(contour_index < number_of_contours)
	{
		u32 current_point_end_index = glyph_info->end_points_of_contours[contour_index];
		u32 current_point_count     = current_point_end_index - last_end_index;
	//	u32 point_end = number_of_contours > 1 && (contour_index + 1) == number_of_contours ?
	//		current_point_count + 0:
	//		current_point_count + 1;
		u32 point_end = current_point_count + 1;
		u32 contour_end_index = (contour_index + 1) == number_of_contours ?
		current_point_count - 1 : current_point_count;
	    while(c <= current_point_count)
	    {
	    	u32 index_0 = start_index + (c % point_end);
	    	u32 index_1 = start_index + ((c + 1) % point_end);
	    	u32 index_2 = start_index + ((c + 2) % point_end);

            ttf_glyph_point g_p0 = glyph_info->point_coordinates[index_0];
            ttf_glyph_point g_p1 = glyph_info->point_coordinates[index_1];
	    	u8 g_p0_flags = g_p0.flags;
	    	u8 g_p1_flags = g_p1.flags;

	    	vec2 curve_p0            = {0};
	    	vec2 curve_interpolation = {0};
	    	vec2 curve_p1            = {0};

	    	if(g_p0_flags & simple_glyph_on_curve_point)
	    	{
	    		curve_p0.x = g_p0.x;
	    		curve_p0.y = g_p0.y;

	    		if(g_p1_flags & simple_glyph_on_curve_point)
	    		{
	    			curve_p1.x = g_p1.x;
	    			curve_p1.y = g_p1.y;
	    			curve_interpolation.x = (curve_p1.x - curve_p0.x) * 0.5f;
	    			curve_interpolation.y = (curve_p1.y - curve_p0.y) * 0.5f;
	    			curve_interpolation.x += curve_p0.x;
	    			curve_interpolation.y += curve_p0.y;

	    		    p0_x = (f32)curve_p0.x;
	    		    p0_y = (f32)curve_p0.y;

	    		}
	    		else
	    		{
                    ttf_glyph_point g_p2 = glyph_info->point_coordinates[index_2];

	    			if(g_p2.flags & simple_glyph_on_curve_point)
	    			{
	    			    curve_p1.x            = g_p2.x;
	    			    curve_p1.y            = g_p2.y;
	    			    curve_interpolation.x = g_p1.x;
	    			    curve_interpolation.y = g_p1.y;
	    				//p0 will be the last avadible starting point
	    		        p0_x = (f32)curve_p0.x;
	    		        p0_y = (f32)curve_p0.y;
						//c++;
	    			}
	    			else
	    			{
	    				curve_p1.x = (g_p2.x - g_p1.x) / 2.0f;
	    				curve_p1.y = (g_p2.y - g_p1.y) / 2.0f;
	    				curve_p1.x += g_p1.x;
	    				curve_p1.y += g_p1.y;
	    			    curve_interpolation.x = g_p1.x;
	    			    curve_interpolation.y = g_p1.y;

	    		        p0_x = (f32)curve_p1.x;
	    		        p0_y = (f32)curve_p1.y;
	    			    c++;
	    			}

	    		}
	    	}
	    	else
	    	{
	    		curve_p0.x = (f32)p0_x;
	    		curve_p0.y = (f32)p0_y;
	    		curve_interpolation.x = g_p0.x;
	    		curve_interpolation.y = g_p0.y;

                if(g_p1_flags & simple_glyph_on_curve_point)
	    		{
	    			curve_p1.x = g_p1.x;
	    			curve_p1.y = g_p1.y;
	    		}
	    		else
	    		{
	    			curve_p1.x = (g_p1.x - g_p0.x) / 2.0f;
	    			curve_p1.y = (g_p1.y - g_p0.y) / 2.0f;
	    			curve_p1.x += g_p0.x;
	    			curve_p1.y += g_p0.y;

	    		    p0_x = (f32)curve_p1.x;
	    		    p0_y = (f32)curve_p1.y;
	    		}
	    	}

	    	if(curve_p1.x < 0 || curve_p1.y < 0 ||
	    	   curve_p0.x < 0 || curve_p0.y < 0 ||
	    	   curve_interpolation.x < 0 || curve_interpolation.y < 0)
	    	{
	    		c++;
	    		continue;
	    	}
	    	//draw curve!
	    	c++;
            ttf_curve *pushed_curve = 0;
			if(!vec2_compare(last_curve.p0, curve_p0) ||
			   !vec2_compare(last_curve.p1, curve_interpolation) ||
			   !vec2_compare(last_curve.p2, curve_p1)
					)
			{
			    pushed_curve = memory_area_push_struct(curves_area, ttf_curve);
			    pushed_curve->p0 = curve_p0;
			    pushed_curve->p1 = curve_interpolation;
			    pushed_curve->p2 = curve_p1;
			    //glyph_work->curve_count++;
				glyph_work->curve_counts_per_contour[contour_index]++;
			    last_curve = *pushed_curve;
			}
			

			if(!curves_array) //make this point to the first curve
			{
				curves_array = pushed_curve;
			}

	    }
		start_index    = current_point_end_index + 1;
		last_end_index = current_point_end_index + 1;

		contour_index++;
		c = 0;
	}

	if(contour_index) // if any curve got stored
	{
		//keep the stored array
	    temporary_area_keep(&recovery_area);
	}
	else
	{
		//recover stored memory
		temporary_area_end(&recovery_area);
		curves_array = 0;
	}

	glyph_work->glyph_curves = curves_array;


	//0 if nothing stored
	return(contour_index);
}

static u32
ttf_divide_curve(
		memory_area *curves_area,
		vec2 p0,
		vec2 p1,
		vec2 p2,
		u32 *curve_count)
{

	f32 y_0 = p0.y;
	f32 y_1 = p1.y;
	f32 y_2 = p2.y;

	u32 is_ascending = p0.y >= p1.y &&
		               p1.y >= p2.y;
	u32 is_descending = p0.y <= p1.y &&
                        p1.y <= p2.y;

	    	u32 is_line = (((i32)p1.y) == ((i32)p0.y)) &&
				          (((i32)p1.y) == ((i32)p2.y)) && 
						  (((i32)p2.y) == ((i32)p0.y));

	if(!is_ascending && !is_descending)
	{
		if(!is_line)
		{
	        vec2 s_p0 = p0;
	        vec2 s_p1 = 
	        {
	        	p0.x + (p1.x - p0.x) * 0.5f,
	        	p0.y + (p1.y - p0.y) * 0.5f,
	        };

	        vec2 e_p1 = 
	        {
	        	p1.x + (p2.x - p1.x) * 0.5f,
	        	p1.y + (p2.y - p1.y) * 0.5f,
	        };

	        vec2 s_p2 = {
	        	s_p1.x + (e_p1.x - s_p1.x) * 0.5f,
	        	s_p1.y + (e_p1.y - s_p1.y) * 0.5f,
	        };
	        vec2 e_p0 = s_p2;
	        vec2 e_p2 = p2;

			ttf_divide_curve(curves_area, s_p0, s_p1, s_p2,curve_count);
			ttf_divide_curve(curves_area, e_p0, e_p1, e_p2,curve_count);
			//skip
		}
		else
		{
			//make a sub-curve and add push it to the list
		}
	}
	else
	{
#if 0
        ttf_curve_profile *curve_profile = memory_area_push_struct(curves_area, ttf_curve_profile);
		curve_profile->curve.p0 = p0;
		curve_profile->curve.p1 = p1;
		curve_profile->curve.p2 = p2;
		curve_profile->y_direction = y_0 < y_2 ? 1 : -1;
		curve_profile->height = (u32)ABS(y_0 - y_2);
		(*curve_count)++;
#endif
		if(!is_line)
		{
        ttf_curve *curve = memory_area_push_struct(curves_area, ttf_curve);
		curve->p0 = p0;
		curve->p1 = p1;
		curve->p2 = p2;
		//curve_profile->y_direction = y_0 < y_2 ? 1 : -1;
		//curve_profile->height = (u32)ABS(y_0 - y_2);
		(*curve_count)++;
		}
	}

	return(*curve_count);
}

inline void
ttf_push_curve_profile(
		memory_area *curves_area,
		ttf_curve *curves_array,
		ttf_glyph_work *glyph_work,
		u32 s_index,
		u32 e_index,
		i32 y_direction)
{
	Assert(s_index <= e_index);
	Assert(y_direction != 0);

	ttf_curve_profile *new_profile = memory_area_push_struct(curves_area, ttf_curve_profile);
	memory_zero_struct(new_profile, ttf_curve_profile);
	new_profile->first_curve_index = s_index;
	new_profile->last_curve_index  = e_index;
	new_profile->y_direction       = y_direction;

	ttf_curve s_curve =  curves_array[s_index];
	ttf_curve e_curve =  curves_array[e_index];

	glyph_work->curve_profile_count++;

	//get the total height
	//goes down
	if(y_direction > 0)
	{
		Assert(e_curve.p2.y > s_curve.p0.y);
		//new_profile->height = (u32)(e_curve.p2.y - s_curve.p0.y);
		new_profile->y0 = (i32)s_curve.p0.y;
		new_profile->y1 = (i32)e_curve.p2.y;
	}
	else
	{
		Assert(e_curve.p2.y < s_curve.p0.y);
	//	new_profile->height = (u32)(s_curve.p0.y - e_curve.p2.y);
		new_profile->y0 = (i32)e_curve.p2.y;
		new_profile->y1 = (i32)s_curve.p0.y;
	}
	int s = 0;
}

inline u32
ttf_curve_is_horizontal_line(ttf_curve c)
{
	vec2 p0 = c.p0;
	vec2 p1 = c.p1;
	vec2 p2 = c.p2;
	u32 is_line = (((i32)p1.y) == ((i32)p0.y)) &&
		          (((i32)p1.y) == ((i32)p2.y)) && 
				  (((i32)p2.y) == ((i32)p0.y));
	return(is_line);
}

//#define ttf_PRECISION (0.5f)
#define ttf_PRECISION (0.0625f)

inline f32
ttf_round_to_precision(f32 v)
{
	f32 y = v;

	y /= ttf_PRECISION;
	f32 y_decimal = y - (i32)y;
	f32 decimal1 = ttf_PRECISION * y_decimal;
	v += decimal1;
	return(v);
}

inline void
ttf_decompose_curve_to_points(
		memory_area *area,
		ttf_curve_profile *profile,
		ttf_curve curve,
		f32 scale)
{
	vec2 p0 = curve.p0;
	vec2 p1 = curve.p1;
	vec2 p2 = curve.p2;

	//p0.y = ttf_round_to_precision(p0.y);
	//p1.y = ttf_round_to_precision(p1.y);
	//p2.y = ttf_round_to_precision(p2.y);

	f32 p2_p0_y_distance = ABS(p2.y - p0.y);

	f32 p2_p0_x_distance = ABS(p2.x - p0.x);
	//hope this is precise enough!
	if((p2_p0_x_distance * p2_p0_x_distance) <= (ttf_PRECISION * ttf_PRECISION) &&
	   (p2_p0_y_distance * p2_p0_y_distance) <= (ttf_PRECISION * ttf_PRECISION))	
	{
	    ttf_profile_points *new_points = memory_area_push_struct(area, ttf_profile_points);
		profile->points_count++;
		if(profile->y_direction < 0)
		{
		     new_points->x = p0.x * scale;
		     new_points->y = p0.y * scale;
		}
		else
		{
		     new_points->x = p2.x * scale;
		     new_points->y = p2.y * scale;
		}

		if(!profile->points)
		{
			profile->points = new_points;
		}
	}
	else
	{
	    vec2 s_p0 = p0;
	    vec2 s_p1 = 
	    {
	    	p0.x + (p1.x - p0.x) * 0.5f,
	    	p0.y + (p1.y - p0.y) * 0.5f,
	    };

	    vec2 e_p1 = 
	    {
	    	p1.x + (p2.x - p1.x) * 0.5f,
	    	p1.y + (p2.y - p1.y) * 0.5f,
	    };

	    vec2 s_p2 = {
	    	s_p1.x + (e_p1.x - s_p1.x) * 0.5f,
	    	s_p1.y + (e_p1.y - s_p1.y) * 0.5f,
	    };
	    vec2 e_p0 = s_p2;
	    vec2 e_p2 = p2;

	    ttf_curve curve0;
	    ttf_curve curve1;

	    curve0.p0 = s_p0;
	    curve0.p1 = s_p1;
	    curve0.p2 = s_p2;

	    curve1.p0 = e_p0;
	    curve1.p1 = e_p1;
	    curve1.p2 = e_p2;

	    ttf_decompose_curve_to_points(area, profile, curve0, scale);
	    ttf_decompose_curve_to_points(area, profile, curve1, scale);
	}

}
//decompose curves until they only ascend or descend
static void
ttf_flatten_curves(
		memory_area *curves_area,
		ttf_glyph_work *glyph_work, f32 scale)
{
	ttf_curve *curves = glyph_work->glyph_curves;

	u32 curve_profile_count = 0;
	//;for now
	u32 c = 0;
	u32 new_curve_count = 0;
	u32 total_curve_count = 0;
	temporary_area restoring_area = temporary_area_begin(curves_area);

	u32 cc = 0;
	for(u32 contour_i = 0;
			contour_i < glyph_work->contour_count;
			contour_i++)
	{
		cc += glyph_work->curve_counts_per_contour[contour_i];
		u32 new_curve_count = 0;
	    while(c < cc)
	    {
	    	//ttf_curve_profile *curve_profile = memory_area_push_struct(curves_area, ttf_curves_profile);
	    	ttf_curve current_curve = curves[c];
	    	f32 y_0 = current_curve.p0.y;
	    	f32 y_1 = current_curve.p1.y;
	    	f32 y_2 = current_curve.p2.y;

            ttf_divide_curve(
	    	    curves_area,
	    	    current_curve.p0,
	    	    current_curve.p1,
	    	    current_curve.p2,
	    	    &new_curve_count);

	    	c++;
	    }
		glyph_work->curve_counts_per_contour[contour_i] = new_curve_count;
		total_curve_count += new_curve_count;
	}

	temporary_area_end(&restoring_area);

	//restoring_area = temporary_area_begin(curves_area);


	ttf_curve *pushed_curves_array = memory_area_push_array(
			curves_area, ttf_curve, total_curve_count);

	restoring_area = temporary_area_begin(curves_area);

	glyph_work->glyph_curves = pushed_curves_array;

	//sort curves by their orientation
	//if a line appears, it will be send to the start of the contour
#if 1
	c = 1;
	total_curve_count = 0;
	for(u32 contour_i = 0;
			contour_i < glyph_work->contour_count;
			contour_i++)
	{
		u32 first_index = total_curve_count;
		u32 curve_count = total_curve_count + glyph_work->curve_counts_per_contour[contour_i];
		u32 curve_shift_amount =  glyph_work->curve_counts_per_contour[contour_i] - 1;
		u32 last_curve_index = curve_count - 1;

		ttf_curve first_curve = pushed_curves_array[first_index];
		ttf_curve last_curve  = pushed_curves_array[curve_count - 1];
	    i32 f_curve_y_direction = first_curve.p0.y < first_curve.p2.y ? 1 : -1;
	    i32 l_curve_y_direction = last_curve.p0.y < last_curve.p2.y ? 1 : -1;
		//check if the last curve is a line
		if(ttf_curve_is_horizontal_line(last_curve))
		{
			l_curve_y_direction = 0;
			//u32 last = last_curve_index - 1;
			//last_curve = pushed_curves_array[last];
		}


	    u32 first_curve_is_line = ttf_curve_is_horizontal_line(first_curve); 
				          
						  

		if(!first_curve_is_line)
		{
		    while(l_curve_y_direction == f_curve_y_direction)
		    {
				//THIS GOT FIXED ASI NOMAS, IT NEEDS TO BE CHECKED
				NotImplemented;
		    	memory_shift_array_r(
		    			pushed_curves_array + total_curve_count,
		    			curve_shift_amount,
						total_curve_count + curve_count - curve_shift_amount,
						ttf_curve);
		    	pushed_curves_array[first_index] = last_curve;
		    	last_curve = pushed_curves_array[curve_count - 1];

	            l_curve_y_direction = last_curve.p0.y < last_curve.p2.y ? 1 : -1;
		    	if(ttf_curve_is_horizontal_line(last_curve))
		    	{
		    		l_curve_y_direction = 0;
		    	}
		    }
		}
		total_curve_count += glyph_work->curve_counts_per_contour[contour_i];

	}
#else
	//eliminate the horizontal lines
    c = 1;
	total_curve_count = 0;
	u32 eliminated_curves = 0;
	for(u32 contour_i = 0;
			contour_i < glyph_work->contour_count;
			contour_i++)
	{
		u32 first_index        = total_curve_count;
		u32 curve_count        = total_curve_count + glyph_work->curve_counts_per_contour[contour_i];
		u32 curve_shift_amount =  glyph_work->curve_counts_per_contour[contour_i] - 1;
		u32 last_curve_index   = curve_count - 1;

		ttf_curve curve = pushed_curves_array[first_index];
	}
#endif


	//glyph_work->curve_count  = new_curve_count;
	//create the profiles
	c = 0;
    total_curve_count = 0;
	u32 s_index = 0;
	u32 e_index = 0;
	u32 start_profile = 0;
	i32 current_direction = 0;
	//the curves should respect the glyph's orientation

	for(u32 contour_i = 0;
			contour_i < glyph_work->contour_count;
			contour_i++)
	{
		u32 curve_count = total_curve_count + glyph_work->curve_counts_per_contour[contour_i];
		total_curve_count += glyph_work->curve_counts_per_contour[contour_i];

	    while(c < curve_count)
	    {
	    	ttf_curve curve = pushed_curves_array[c];
	    	vec2 p0 = curve.p0;
	    	vec2 p1 = curve.p1;
	    	vec2 p2 = curve.p2;
	    	i32 curve_y_direction = curve.p0.y < curve.p2.y ? 1 : -1;

	    	u32 is_line_2 = (((i32)p1.y) == ((i32)p0.y)) &&
				          (((i32)p1.y) == ((i32)p0.y)) && 
						  (((i32)p2.y) == ((i32)p0.y));
			Assert(is_line_2 == 0);

	    	if(!current_direction)
	    	{
	    		//positive goes "down" on the final image
	    		current_direction = curve_y_direction;
	    		s_index = c;
	    	}

	    	if(current_direction != curve_y_direction)
	    	{
	    		//the previous curve ends the profile
	    		e_index = c - 1;
                ttf_push_curve_profile(
	    				curves_area,
	    				pushed_curves_array,
	    				glyph_work,
	    				s_index,
	    				e_index,
	    				current_direction);
	    		//start over
	    		current_direction = 0;
	    		s_index = c;
	    		current_direction = curve_y_direction;
	    	}
	    	c++;
	    }

		if(!current_direction)
		{
			ttf_curve s_curve = pushed_curves_array[s_index];
			current_direction = s_curve.p0.y < s_curve.p2.y ? 1 : -1;
		}

		//push the last ignored curves if those are not lines
	    e_index = c - 1;
		ttf_curve last_curve = pushed_curves_array[e_index];
		while(e_index > s_index && ttf_curve_is_horizontal_line(last_curve))
		{
			e_index--;
		}
		if(e_index >= s_index)
		{
            ttf_push_curve_profile(
	        		curves_area,
	        		pushed_curves_array,
	        		glyph_work,
	        		s_index,
	        		e_index,
	        		current_direction);
		    s_index = c;
		}
		current_direction = 0;
	}

	//for(u32 p = 0;
	//		p < profiles_count;
	//		p++)
	//{
	//}

	temporary_area_end(&restoring_area);
	//set array
	glyph_work->curve_profiles = memory_area_push_array(
			curves_area, ttf_curve_profile, glyph_work->curve_profile_count);

	//look for connected curves
	for(u32 p = 0;
			p < glyph_work->curve_profile_count;
			p++)
	{
		ttf_curve_profile *prof = glyph_work->curve_profiles + p;

		c = prof->first_curve_index;
		while(c <= prof->last_curve_index)
		{
			ttf_curve curve = pushed_curves_array[c];

#if 1
			curve.p0 = vec2_scale(curve.p0, scale);
			curve.p1 = vec2_scale(curve.p1, scale);
			curve.p2 = vec2_scale(curve.p2, scale);
			ttf_decompose_curve_to_points(
					curves_area, prof, curve, 1);
#else
			ttf_decompose_curve_to_points(
					curves_area, prof, curve, scale);
#endif
			//divide curve's points
			/*
			   agarrar curva desde s_index hasta e_index
			   achicar las curvas en orden hasta que tengan el tamaño de un pixel
			   del scanner
			   almacenarlos en el array
			*/

			c++;
		}
		int s = 0;

	}

	//temporary_area_end(&restoring_area);

	//glyph_work->curve_profiles = memory_area_push_array(
	//		curves_area, ttf_curve_profile, glyph_work->curve_profile_count);
}

inline f32
flip_decimal(f32 v)
{
	f32 d = v - (i32)v;
	d = 1.0f - d;
	v = (i32)v + d;

	return(v);
}

typedef struct{
	u32 pixel;
	vec2 p0;
	vec2 p1;
}ttf_pixel_points;


static void
ttf_fill_pixels_areas(
		memory_area *area,
		u32 scanline_y,
		ttf_curve_profile a,
        u32 *active_edges_count_ptr,
		ttf_active_edge *active_edges,
		i32 y_direction
		)
{
	u32 active_edges_count = *active_edges_count_ptr;

	u32 scanline_top = scanline_y == 0 ? 0 : scanline_y - 1;
	for(u32 a0 = 0;
			a0 < a.points_count;
			a0++)
	{
		if((i32)a.points[a0].y == scanline_y)
		{
		    //found the first point inside the scan line, so build an edge inside the pixel
		    ttf_active_edge new_edge = {0}; 
			u32 p_index    = a0;
			new_edge.y_top = a.points[a0].y;
			new_edge.y_bot = a.points[a0].y;
			new_edge.pixel_x = (u32)a.points[a0].x;
			//new_edge.points = memory_area_get_next_ptr(area);
			//create a new active edge
			//ascender
			temporary_area temporary_points_area = temporary_area_begin(area);
			u32 point_count = 0;
//			vec2 *points = memory_area_get_next_ptr(area);
            ttf_pixel_points *points = memory_area_get_next_ptr(area);
			u32 x0 = (u32)a.points[a0].x;
			u32 x1 = (u32)a.points[a0].x;

		    i32 current_x_direction = 0;

			// 1 = right. -1 left
			vec2 edge_point = a.points[p_index].v;
			ttf_pixel_points *new_edge_point = 0;

			u32 current_x_pixel = x0;
			if(scanline_y == 15)
			{
				int s = 0;
			}
			vec2 last_p1       = edge_point;
			p_index++;

			if(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y)
			{
				current_x_direction = SIGN(a.points[p_index].x - edge_point.x);

				u32 added_point = 0;
                while(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y)
				{
			            edge_point = a.points[p_index].v;
						u32 x_pixel = (u32)(edge_point.x);
						added_point = 0;
				    	if(x_pixel < x0)
				    	{
				    		x0 = (u32)x_pixel;
				    	}
				    	if(x_pixel > x1)
				    	{
				    		x1 = (u32)x_pixel;
				    	}


				    	i32 previous_direction = current_x_direction;
				    	vec2 previous_edge     = a.points[p_index - 1].v;
				    	current_x_direction    = SIGN_OR_ZERO(edge_point.x - previous_edge.x);
						if(current_x_direction == 0)
						{
							current_x_direction = previous_direction;
						}


						if(x_pixel != current_x_pixel)
						{
							if(current_x_direction > 0)
							{
			                    //new_edge_point->x = (f32)x_pixel;

								u32 x_index = current_x_pixel;
								while(x_index < x_pixel)
								{
			                        new_edge_point = memory_area_push_struct(area, ttf_pixel_points);
									new_edge_point->p0   = last_p1;
			                        new_edge_point->p1.y = a.points[p_index - 1].y;
			                        new_edge_point->p1.x = (f32)x_index + 1;
									last_p1 = new_edge_point->p1;
									new_edge_point->pixel = x_index;
				                    point_count++;
									x_index++;
								}
							}
							else
							{
								u32 x_index = current_x_pixel;
								//while(x_index > x0)
								while(x_index > x_pixel)
								{
			                        new_edge_point = memory_area_push_struct(area, ttf_pixel_points);
									new_edge_point->p0   = last_p1;
			                        new_edge_point->p1.y = a.points[p_index - 1].y;
			                        new_edge_point->p1.x = (f32)x_index;
									last_p1 = new_edge_point->p1;
									new_edge_point->pixel = x_index;
				                    point_count++;
									x_index--;
								}
							}
						}
						current_x_pixel = x_pixel;
				    	if(current_x_direction != previous_direction)
				    	{
							Assert(added_point == 0);
				    		//add point
			                new_edge_point = memory_area_push_struct(area, ttf_pixel_points);
			                new_edge_point->p0 = last_p1;
			                new_edge_point->p1 = edge_point;
							new_edge_point->pixel = x_pixel;
							last_p1 = new_edge_point->p1;
				            point_count++;
							added_point = 1;
				    	}

				    	p_index++;

						if(y_direction < 0)
						{
				    	    Assert(new_edge.y_top >= edge_point.y);
				    	    new_edge.y_top = edge_point.y;
						}
						else
						{
				    	    Assert(new_edge.y_bot <= edge_point.y);
				    	    new_edge.y_bot = edge_point.y;
						}

		
				}
			    if(!added_point)
				{
			        new_edge_point = memory_area_push_struct(area, ttf_pixel_points);
			        new_edge_point->p0 = last_p1;
			        new_edge_point->p1 = edge_point;
				    new_edge_point->pixel = current_x_pixel;
				    last_p1 = new_edge_point->p1;
				    point_count++;

				    if(edge_point.x < x0)
				    {
				    	x0 = (u32)edge_point.x;
				    }
				    if(edge_point.x > x1)
				    {
				    	x1 = (u32)edge_point.x;
				    }
					if(current_x_direction < 0)
					{
						int s = 0;
					}
				}
				//add the bottom point 
			}

			if(!point_count)
			{
			    temporary_area_end(&temporary_points_area);

			    new_edge.areas    = memory_area_push_array(area, f32, 1);
				new_edge.areas[0] = 1.0f;
			    new_edge.x0 = x0;
			    new_edge.x1 = x0;


	            active_edges[active_edges_count] = new_edge;
                active_edges_count++;
	            *active_edges_count_ptr = active_edges_count;
			    break;
			}


		    //sort by pixels
			if(y_direction > 0) // sort in descending order
			{
		        for(u32 y = 0;
		        	    y < point_count;
		        	    y++)
		        {
		        	ttf_pixel_points tpps0 = points[y];
		        	for(u32 x = 0;
		        			x < point_count;
		        			x++)
		        	{
		        	    ttf_pixel_points tpps1 = points[x];
		        		if(tpps0.pixel > tpps1.pixel)
		        		{
		        			points[x] = tpps0;
		        			points[y] = tpps1;
		        			tpps0 = points[y];
		        		}

		        	}

		        }
			}
			else // sort in ascending order
			{
		        for(u32 y = 0;
		        	    y < point_count;
		        	    y++)
		        {
		        	ttf_pixel_points tpps0 = points[y];
		        	for(u32 x = 0;
		        			x < point_count;
		        			x++)
		        	{
		        	    ttf_pixel_points tpps1 = points[x];
		        		if(tpps0.pixel < tpps1.pixel)
		        		{
		        			points[x] = tpps0;
		        			points[y] = tpps1;
		        			tpps0 = points[y];
		        		}

		        	}

		        }
			}
			//calculate areas
			u32 covered_pixels_count = 1 + x1 - x0;
			f32 contribuiting_height = y_direction > 0 ? 0.0f : 0.0f;

			f32 *areas = memory_area_push_array(area, f32, covered_pixels_count);
			u32 pixel_index = 0;
			u32 p = 0;
			while(pixel_index < covered_pixels_count)
			{
				ttf_pixel_points *current_pixel_points = points + p;
				u32 current_pixel = current_pixel_points->pixel;
				f32 pixel_area = 0;
				f32 total_h = 0;

				while(p < point_count && (points[p].pixel == current_pixel_points->pixel))
				{   
					current_pixel_points = points + p;
					p++;
                    current_pixel = current_pixel_points->pixel;
					vec2 p0 = current_pixel_points->p0;
					vec2 p1 = current_pixel_points->p1;
					u32 x_index = current_pixel_points->pixel;

					if(y_direction > 0)
					{
						//p0.x = flip_decimal(p0.x);

						if(p0.x  == x_index + 1)
						{
							p0.x -= 1;
						}
						else
						{
						    p0.x = flip_decimal(p0.x);
						}
						if(p1.x  == x_index + 1)
						{
							p1.x -= 1;
						}
						else
						{
						    p1.x = flip_decimal(p1.x);
						}
#if 0
						if(p1.y == scanline_y + 1)
						{
							p1.y -= 1;
						}
						else
						{
							p1.y = flip_decimal(p1.y);
						}
						if(p0.y == scanline_y + 1)
						{
							p0.y -= 1;
						}
						else
						{
							p0.y = flip_decimal(p0.y);
						}
#endif
					}

					f32 p_h = ABS(p1.y - p0.y);
					f32 s   = ABS(p0.x - (x_index + 1)) + ABS(p1.x - (x_index + 1));
					f32 a = (s * p_h) * 0.5f;
					pixel_area += a;
					total_h += p_h;
				}

				if(0 && y_direction > 0)
				{
				    contribuiting_height -= total_h;
				    pixel_area += contribuiting_height;
				}
				else
				{
				    pixel_area += contribuiting_height;
				    contribuiting_height += total_h;
				}
				//REMINDER: the area is not well calculated when only getting it from the right
				Assert(contribuiting_height >= 0 && contribuiting_height <= 1);

				Assert(pixel_area >= 0 && pixel_area <= 1.0f);
				u32 array_pixel_index = current_pixel_points->pixel - x0;
				areas[array_pixel_index] = pixel_area;
                pixel_index++;
			}
		    int s = 0;
			temporary_area_end(&temporary_points_area);

			new_edge.areas = memory_area_push_and_copy_array(area, f32, areas, covered_pixels_count);
			new_edge.x0 = x0;
			new_edge.x1 = x1;


	        active_edges[active_edges_count] = new_edge;
            active_edges_count++;
	        *active_edges_count_ptr = active_edges_count;
			break;
		}

	}
}

static void
ttf_fill_pixels_areas_old(
		memory_area *area,
		u32 scanline_y,
		ttf_curve_profile a,
        u32 *active_edges_count_ptr,
		ttf_active_edge *active_edges,
		i32 y_direction
		)
{
	u32 active_edges_count = *active_edges_count_ptr;

	u32 scanline_top = scanline_y == 0 ? 0 : scanline_y - 1;
	for(u32 a0 = 0;
			a0 < a.points_count;
			a0++)
	{
		if((i32)a.points[a0].y == scanline_y)
		{
		    //found the first point inside the scan line, so build an edge inside the pixel
		    ttf_active_edge new_edge = {0}; 
			u32 p_index    = a0;
			new_edge.y_top = a.points[a0].y;
			new_edge.y_bot = a.points[a0].y;
			new_edge.pixel_x = (u32)a.points[a0].x;
			//new_edge.points = memory_area_get_next_ptr(area);
			//create a new active edge
			//ascender
			temporary_area temporary_points_area = temporary_area_begin(area);
			u32 point_count = 0;
			vec2 *points = memory_area_get_next_ptr(area);
			u32 x0 = (u32)a.points[a0].x;
			u32 x1 = (u32)a.points[a0].x;

		    i32 current_x_direction = 0;

			// 1 = right. -1 left
			vec2 edge_point = a.points[p_index].v;
			vec2 *new_edge_point = memory_area_push_struct(area, vec2);
			new_edge_point->x    = edge_point.x;
			new_edge_point->y    = edge_point.y;
			point_count++;
			p_index++;

			u32 current_x_pixel = x0;

			if(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y)
			{
				current_x_direction = SIGN(a.points[p_index].x - edge_point.x);

				u32 added_point = 0;
                while(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y)
				{
			            edge_point = a.points[p_index].v;
						u32 x_pixel = (u32)(edge_point.x);
						added_point = 0;
				    	if(x_pixel < x0)
				    	{
				    		x0 = (u32)x_pixel;
				    	}
				    	if(x_pixel > x1)
				    	{
				    		x1 = (u32)x_pixel;
				    	}


				    	i32 previous_direction = current_x_direction;
				    	vec2 previous_edge = a.points[p_index - 1].v;
				    	current_x_direction = SIGN_OR_ZERO(edge_point.x - previous_edge.x);
						if(current_x_direction == 0)
						{
							current_x_direction = previous_direction;
						}


						if(x_pixel != current_x_pixel)
						{
							if(current_x_direction > 0)
							{
			                    //new_edge_point->x = (f32)x_pixel;

								u32 x_index = current_x_pixel;
								while(x_index < x1)
								{
			                        new_edge_point = memory_area_push_struct(area, vec2);
			                        new_edge_point->y = a.points[p_index - 1].y;
			                        new_edge_point->x = (f32)x_index + 1;
				                    point_count++;
									x_index++;
								}
							}
							else
							{
								u32 x_index = current_x_pixel;
								while(x_index > x0)
								{
			                        new_edge_point = memory_area_push_struct(area, vec2);
			                        new_edge_point->y = a.points[p_index - 1].y;
			                        new_edge_point->x = (f32)x_index;
				                    point_count++;
									x_index--;
								}
							}
						}
						current_x_pixel = x_pixel;
				    	if(current_x_direction != previous_direction)
				    	{
							Assert(added_point == 0);
				    		//add point
			                new_edge_point = memory_area_push_struct(area, vec2);
			                new_edge_point->x    = edge_point.x;
			                new_edge_point->y    = edge_point.y;
				            point_count++;
							added_point = 1;
				    	}

				    	p_index++;

						if(y_direction < 0)
						{
				    	    Assert(new_edge.y_top >= edge_point.y);
				    	    new_edge.y_top = edge_point.y;
						}
						else
						{
				    	    Assert(new_edge.y_bot <= edge_point.y);
				    	    new_edge.y_bot = edge_point.y;
						}

		
				}
				if(p_index == 65536)
				{
					int s = 0;
				}
			    if(!added_point)
				{
			        new_edge_point = memory_area_push_struct(area, vec2);
			        new_edge_point->x    = edge_point.x;
			        new_edge_point->y    = edge_point.y;
				    point_count++;

				    if(edge_point.x < x0)
				    {
				    	x0 = (u32)edge_point.x;
				    }
				    if(edge_point.x > x1)
				    {
				    	x1 = (u32)edge_point.x;
				    }
					if(current_x_direction < 0)
					{
						int s = 0;
					}
				}
				//add the bottom point 
			}
			if(y_direction > 0)
			{
				//for(u32 p = 0;
				//		p < point_count;
				//		p++)
				//{
				//	vec2 *v_p = points + p;
				//	f32 d = v_p->x - (i32)v_p->x;
				//	d = 1.0f - d;
				//	v_p->x = (i32)v_p->x + d;
				//}
			}

			u32 covered_pixels_count = 1 + x1 - x0;
			f32 *areas = memory_area_push_array(area, f32, covered_pixels_count);
			u32 x_index = x0;
			f32 last_computed_area = 0;
			f32 started_line_height = 0;
			f32 contribuiting_height = y_direction > 0 ? 1.0f : 0;
			while(x_index <= x1)
			{
			    f32 pixel_area = 0;
				f32 y_min = 0;
				f32 y_max = 0;
				u32 point_per_x = 0;
				f32 total_pixel_area = 0;
			    for(u32 p = 0;
			    		p < point_count;
			    		p++)
			    {
#if 0
			        vec2 edge_point = points[p];
			    	if((i32)edge_point.x == (i32)x_index || (edge_point.x == x_index + 1))
			    	{
					    i32 x_direction = SIGN(points[p + 1].x - edge_point.x);
					    do
			    	    {
							f32 sum = 0;
			                edge_point = points[p];

							if(x_direction > 0)
							{
					    	    total_pixel_area += ABS(edge_point.x - (x_index + 1)); 
							}
							else
							{
					    	    total_pixel_area += ABS(edge_point.x - (x_index + 1)); 
							}

					    	point_per_x++;
					    	p++;
			    	    }while(p < point_count && ((i32)points[p].x == (i32)x_index || (points[p].x == x_index + 1)));
						total_pixel_area /= point_per_x;
						if(y_direction > 0)
						{
							total_pixel_area = 1.0f - total_pixel_area;
						}
					    break;
					}
#else
					//Assert(point_count > 1);
					if(point_count == 1)
					{
						//Not sure if this should always be true
						total_pixel_area = 1;
						break;
					}

			        vec2 edge_point = points[p];
			    	if((i32)edge_point.x == (i32)x_index || (edge_point.x == x_index + 1))
			    	{
						if(x1 - x0 > 1)
						{
							int s = 0;
						}
						if(x0 != x1)
						{
						    y_min = edge_point.y;
						    y_max = edge_point.y;
						    if(scanline_y == 5)
						    {
								int s = 0;
						    }
						    u32 subtract_h = 0;
							f32 total_h = 0;
					        do
						    {
							    vec2 p0 = points[p];
							    vec2 p1 = points[p + 1];

								if(y_direction > 0)
								{
									//p0.x = flip_decimal(p0.x);

									if(p0.x  == x_index + 1)
									{
										p0.x -= 1;
									}
									else
									{
									    p0.x = flip_decimal(p0.x);
									}
									if(p1.x  == x_index + 1)
									{
										p1.x -= 1;
									}
									else
									{
									    p1.x = flip_decimal(p1.x);
									}
								}
						    	f32 sum = 0;
								f32 p_h = ABS(p1.y - p0.y);
								f32 s   = ABS(p0.x - (x_index + 1)) + ABS(p1.x - (x_index + 1));
								f32 a = (s * p_h) * 0.5f;
								total_pixel_area += a;
								total_h += p_h;

						        point_per_x++;
						    	p++;

								if(p0.y < y_min)
								{
									y_min = p0.y;
								}
								if(p0.y > y_max)
								{
									y_max = p0.y;
								}
								if(p1.y < y_min)
								{
									y_min = p1.y;
								}
								if(p1.y > y_max)
								{
									y_max = p1.y;
								}
						    }while(p + 1 < point_count && ((i32)points[p + 1].x == (i32)x_index || (points[p + 1].x == x_index + 1)));

							if(y_direction > 0)
							{
							    contribuiting_height -= total_h;
							    total_pixel_area += contribuiting_height;
							}
							else
							{
							    total_pixel_area += contribuiting_height;
							    contribuiting_height += total_h;
							}
							//REMINDER: the area is not well calculated when only getting it from the right
							Assert(contribuiting_height >= 0 && contribuiting_height <= 1);
						}
						else
						{
							//simple case
					        do
						    {
							    vec2 p0 = points[p];
							    vec2 p1 = points[p + 1];

								if(y_direction > 0)
								{
									//p0.x = flip_decimal(p0.x);

									if(p0.x  == x_index + 1)
									{
										p0.x -= 1;
									}
									else
									{
									    p0.x = flip_decimal(p0.x);
									}
									if(p1.x  == x_index + 1)
									{
										p1.x -= 1;
									}
									else
									{
									    p1.x = flip_decimal(p1.x);
									}
								}
						    	f32 sum = 0;
								f32 p_h = ABS(p1.y - p0.y);
								f32 s   = ABS(p0.x - (x_index + 1)) + ABS(p1.x - (x_index + 1));
								f32 a = (s * p_h) * 0.5f;
								total_pixel_area += a;

						        point_per_x++;
						    	p++;

								if(p0.y < y_min)
								{
									y_min = p0.y;
								}
								if(p0.y > y_max)
								{
									y_max = p0.y;
								}
								if(p1.y < y_min)
								{
									y_min = p1.y;
								}
								if(p1.y > y_max)
								{
									y_max = p1.y;
								}
						    }while(p + 1 < point_count && ((i32)points[p + 1].x == (i32)x_index || (points[p + 1].x == x_index + 1)));
						}
					//	break;
			    	}
#endif

			    }
				pixel_area = total_pixel_area;
				last_computed_area = pixel_area;
				areas[x_index - x0] = pixel_area;
				if(pixel_area > 0 && pixel_area < 1)
				{
					int s = 0;
				}
				Assert(pixel_area >= 0);

				//add pixel area
				x_index++;

			}
			temporary_area_end(&temporary_points_area);

			new_edge.areas = memory_area_push_and_copy_array(area, f32, areas, covered_pixels_count);
			new_edge.x0 = x0;
			new_edge.x1 = x1;


	        active_edges[active_edges_count] = new_edge;
            active_edges_count++;
	        *active_edges_count_ptr = active_edges_count;
			break;
		}
	}
}

static void
ttf_get_edge_distances(
		memory_area *area,
		u32 scanline_y,
		ttf_curve_profile a,
        u32 *active_edges_count_ptr,
		ttf_active_edge *active_edges,
		i32 y_direction
		)
{
	u32 active_edges_count = *active_edges_count_ptr;

	u32 scanline_top = scanline_y == 0 ? 0 : scanline_y - 1;
	for(u32 a0 = 0;
			a0 < a.points_count;
			a0++)
	{
		if((i32)a.points[a0].y == scanline_y)
		{
		    //found the first point inside the scan line, so build an edge inside the pixel
		    ttf_active_edge new_edge = {0}; 
			u32 p_index    = a0;
			new_edge.y_top = a.points[a0].y;
			new_edge.y_bot = a.points[a0].y;
			new_edge.pixel_x = (u32)a.points[a0].x;
			//new_edge.points = memory_area_get_next_ptr(area);
			//create a new active edge
			//ascender
			u32 point_count = 0;
			u32 x0 = (u32)a.points[a0].x;
			u32 x1 = (u32)a.points[a0].x;
			if(y_direction < 0)
			{
			    do
			    {
			        vec2 edge_point = a.points[p_index].v;
			    	p_index++;

					if(edge_point.x < x0)
					{
						x0 = (u32)edge_point.x;
					}
					if(edge_point.x > x1)
					{
						x1 = (u32)edge_point.x;
					}

			    	Assert(new_edge.y_top >= edge_point.y);

			    	new_edge.y_top = edge_point.y;
			    }while(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y);
			}
			else //descender
			{

				do
				{
			        vec2 edge_point = a.points[p_index].v;
			    	p_index++;

					if(edge_point.x < x0)
					{
						x0 = (u32)edge_point.x;
					}
					if(edge_point.x > x1)
					{
						x1 = (u32)edge_point.x;
					}
					p_index++;

					Assert(new_edge.y_bot <= edge_point.y);

					new_edge.y_bot = edge_point.y;
				}while(p_index < a.points_count && (i32)a.points[p_index].y == scanline_y);

				//add the bottom point 
			}
			new_edge.x0 = x0;
			new_edge.x1 = x1;

	        active_edges[active_edges_count] = new_edge;
            active_edges_count++;
	        *active_edges_count_ptr = active_edges_count;
			break;
		}
	}
}

static void
ttf_glyph_work_rasterize_to_pixels_anti_alias(
		memory_area *area,
		ttf_glyph_work *glyph_work,
		u32 image_width,
		u32 image_height,
		u8 *image_pixels)
{
	//initial data
	u32 profiles_count = glyph_work->curve_profile_count;

	u32 single_profile_array_count = profiles_count / 2;

	//temporary array to divide te ascending and descending profiles
	temporary_area t_profiles_area = temporary_area_begin(area);

	ttf_curve_profile *ascending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	ttf_curve_profile *descending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	u32 ascending_index = 0;
	u32 descending_index = 0;
	//store the profiles on the temporary arrays
	for(u32 p = 0;
			p < profiles_count;
			p++)
	{
		ttf_curve_profile current_profile = glyph_work->curve_profiles[p];
		if(current_profile.y_direction > 0)
		{
			descending_profiles[descending_index++] = current_profile;
		}
		else
		{
			ascending_profiles[ascending_index++] = current_profile;
		}
	}

	
	Assert(ascending_index == descending_index);

	u32 ascender_active_edge_count  = 0;
	ttf_active_edge ascender_active_edges[64] = {0};

	u32 descender_active_edge_count = 0;
	ttf_active_edge descender_active_edges[64] = {0};

	//these "counts" are used as indices when adding points to the arrays above
	u32 scan_profiles_count = 0;
	u32 s_y = 0;

	u32 *pixels_32 = (u32 *)image_pixels;
	while(s_y < image_height)
	{
		for(u32 p = 0;
				p < single_profile_array_count;
				p++)
		{
			ttf_curve_profile a = ascending_profiles[p];
			ttf_curve_profile d = descending_profiles[p];

			//get active ascendant edges points
			
            ttf_fill_pixels_areas(
					area,
            		s_y,
            		a,
                    &ascender_active_edge_count,
            		ascender_active_edges,
					a.y_direction
            		);


			//now do the same with the descendant edges
            ttf_fill_pixels_areas(
					area,
            		s_y,
            		d,
                    &descender_active_edge_count,
            		descender_active_edges,
					d.y_direction
            		);
		}


		scan_profiles_count = MIN(ascender_active_edge_count, descender_active_edge_count);

		//sort the active edges by their x position
		for(u32 ay = 0;
				ay < ascender_active_edge_count;
				ay++)
		{
			for(u32 ax = 0;
					ax < ascender_active_edge_count;
					ax++)
			{
			    ttf_active_edge edge0 = ascender_active_edges[ay];
				ttf_active_edge edge1 = ascender_active_edges[ax];
				if(edge1.pixel_x > edge0.pixel_x)
				{
					ascender_active_edges[ay] = edge1;
					ascender_active_edges[ax] = edge0;
				}

			}
		}

		//same with descenders
		for(u32 ay = 0;
				ay < descender_active_edge_count;
				ay++)
		{
			for(u32 ax = 0;
					ax < descender_active_edge_count;
					ax++)
			{
			    ttf_active_edge edge0 = descender_active_edges[ay];
				ttf_active_edge edge1 = descender_active_edges[ax];
				if(edge1.pixel_x > edge0.pixel_x)
				{
					descender_active_edges[ay] = edge1;
					descender_active_edges[ax] = edge0;
				}

			}
		}

		for(u32 s = 0;
				s < scan_profiles_count;
				s++)
		{
			ttf_active_edge edge0 =  ascender_active_edges[s];
			ttf_active_edge edge1 = descender_active_edges[s];

			Assert(edge0.pixel_x <= edge1.pixel_x);
			u32 x0 = edge0.x0;
			u32 x1 = edge1.x1;

			u32 x = x0;
			f32 total_area = 0;
			while(x <= x1)
			{
			//	//calculate the area of this pixel
			//	u32 curve_index = 0;
				f32 pixel_area = 1.0f;
				u32 edge0_area_applied = 0;
				if(x >= edge0.x0 && x <= edge0.x1)
				{
					pixel_area = 0;
					u32 area_index = x - edge0.x0;//edge0.x1 - x;
					pixel_area += edge0.areas[area_index];
                    edge0_area_applied = 1;
				}
				if(x >= edge1.x0 && x <= edge1.x1)
				{
					u32 area_index = x - edge1.x0;
					if(edge0_area_applied)
					{

					    f32 a0 = 1.0f - pixel_area;
					    f32 a1 = 1.0f - edge1.areas[area_index];
					    pixel_area = 1.0f - a0 - a1;
						if(pixel_area < 0)
						{
							pixel_area = 0;
						}
					}
					else
					{
					    pixel_area = 0;
					    u32 area_index = x - edge1.x0;
					    pixel_area += edge1.areas[area_index];
					}
					area_index = area_index;
				}

				Assert(pixel_area >= 0 && pixel_area <= 1.0f);
				vec4 pixel_color_v4 = 
				{255 * pixel_area,
					255 * pixel_area,
					255 * pixel_area,
					255 * pixel_area};
				u32 pixel_color = RGBAPack(pixel_color_v4);
				*(pixels_32 + x + (s_y * image_width)) = pixel_color;

			    x++;
			}
		}
		scan_profiles_count      = 0;
	    ascender_active_edge_count = 0;
	    descender_active_edge_count = 0;
		s_y++;
	}

	Assert(descending_index == ascending_index);
	
	temporary_area_end(&t_profiles_area);
}

static void
ttf_glyph_work_rasterize_to_pixels_non_anti_alias(
		memory_area *area,
		ttf_glyph_work *glyph_work,
		u32 image_width,
		u32 image_height,
		u8 *image_pixels)
{
	//initial data
	u32 profiles_count = glyph_work->curve_profile_count;

	u32 single_profile_array_count = profiles_count / 2;

	//temporary array to divide te ascending and descending profiles
	temporary_area t_profiles_area = temporary_area_begin(area);

	ttf_curve_profile *ascending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	ttf_curve_profile *descending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	u32 ascending_index = 0;
	u32 descending_index = 0;
	//store the profiles on the temporary arrays
	for(u32 p = 0;
			p < profiles_count;
			p++)
	{
		ttf_curve_profile current_profile = glyph_work->curve_profiles[p];
		if(current_profile.y_direction > 0)
		{
			descending_profiles[descending_index++] = current_profile;
		}
		else
		{
			ascending_profiles[ascending_index++] = current_profile;
		}
	}

	
	Assert(ascending_index == descending_index);

	u32 ascender_active_edge_count  = 0;
	ttf_active_edge ascender_active_edges[64] = {0};

	u32 descender_active_edge_count = 0;
	ttf_active_edge descender_active_edges[64] = {0};

	//these "counts" are used as indices when adding points to the arrays above
	u32 scan_profiles_count = 0;
	u32 s_y = 0;

	u32 *pixels_32 = (u32 *)image_pixels;
	while(s_y < image_height)
	{
		for(u32 p = 0;
				p < single_profile_array_count;
				p++)
		{
			ttf_curve_profile a = ascending_profiles[p];
			ttf_curve_profile d = descending_profiles[p];

			//get active ascendant edges points
			
            ttf_get_edge_distances(
					area,
            		s_y,
            		a,
                    &ascender_active_edge_count,
            		ascender_active_edges,
					a.y_direction
            		);


			//now do the same with the descendant edges
            ttf_get_edge_distances(
					area,
            		s_y,
            		d,
                    &descender_active_edge_count,
            		descender_active_edges,
					d.y_direction
            		);
		}


		scan_profiles_count = ascender_active_edge_count;

		//sort the active edges by their x position
		for(u32 ay = 0;
				ay < ascender_active_edge_count;
				ay++)
		{
			for(u32 ax = 0;
					ax < ascender_active_edge_count;
					ax++)
			{
			    ttf_active_edge edge0 = ascender_active_edges[ay];
				ttf_active_edge edge1 = ascender_active_edges[ax];
				if(edge1.pixel_x > edge0.pixel_x)
				{
					ascender_active_edges[ay] = edge1;
					ascender_active_edges[ax] = edge0;
				}

			}
		}

		//same with descenders
		for(u32 ay = 0;
				ay < descender_active_edge_count;
				ay++)
		{
			for(u32 ax = 0;
					ax < descender_active_edge_count;
					ax++)
			{
			    ttf_active_edge edge0 = descender_active_edges[ay];
				ttf_active_edge edge1 = descender_active_edges[ax];
				if(edge1.pixel_x > edge0.pixel_x)
				{
					descender_active_edges[ay] = edge1;
					descender_active_edges[ax] = edge0;
				}

			}
		}

		for(u32 s = 0;
				s < scan_profiles_count;
				s++)
		{
			ttf_active_edge edge0 =  ascender_active_edges[s];
			ttf_active_edge edge1 = descender_active_edges[s];

			//Assert(edge0.pixel_x <= edge1.pixel_x);
			u32 x0 = edge0.x0;
			u32 x1 = edge1.x1;

			u32 x = x0;
			f32 total_area = 0;
			while(x <= x1)
			{
				u32 pixel_color = 0xffffffff;
				*(pixels_32 + x + (s_y * image_width)) = pixel_color;

			    x++;
			}
		}
		scan_profiles_count      = 0;
	    ascender_active_edge_count = 0;
	    descender_active_edge_count = 0;
		s_y++;
	}

	Assert(descending_index == ascending_index);
	
	temporary_area_end(&t_profiles_area);
}

static void
ttf_glyph_work_rasterize_to_pixels_anti_alias_old(
		memory_area *area,
		ttf_glyph_work *glyph_work,
		u32 image_width,
		u32 image_height,
		u8 *image_pixels)
{
	//initial data
	u32 profiles_count = glyph_work->curve_profile_count;

	u32 single_profile_array_count = profiles_count / 2;

	temporary_area t_profiles_area = temporary_area_begin(area);

	ttf_curve_profile *ascending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	ttf_curve_profile *descending_profiles = memory_area_push_array(
			area, ttf_curve_profile, single_profile_array_count);

	u32 ascending_index = 0;
	u32 descending_index = 0;
	//store the profiles on the temporary arrays
	for(u32 p = 0;
			p < profiles_count;
			p++)
	{
		ttf_curve_profile current_profile = glyph_work->curve_profiles[p];
		if(current_profile.y_direction > 0)
		{
			descending_profiles[descending_index++] = current_profile;
		}
		else
		{
			ascending_profiles[ascending_index++] = current_profile;
		}
	}
	ttf_profile_points ascender_points[64] = {0};
	ttf_profile_points descender_points[64] = {0};
	//these "counts" are used as indices when adding points to the arrays above
	u32 ascender_points_count = 0;
	u32 descender_points_count = 0;
	u32 scan_profiles_count = 0;
	u32 s_y = 0;

	u32 *pixels_32 = (u32 *)image_pixels;
	while(s_y < image_height)
	{
		if(s_y >= 279)
		{
			int s = 0;
		}
		for(u32 p = 0;
				p < single_profile_array_count;
				p++)
		{
			ttf_curve_profile a = ascending_profiles[p];
			ttf_curve_profile d = descending_profiles[p];

			for(u32 a0 = 0;
					a0 < a.points_count;
					a0++)
			{
				ttf_profile_points a_points = a.points[a0];
				if((i32)a.points[a0].y == s_y)
				{
					ascender_points[ascender_points_count].x = a_points.x;
					ascender_points[ascender_points_count].y = a_points.y;
					ascender_points_count++;
					break;
				}
			}

			for(u32 d0 = 0;
					d0 < d.points_count;
					d0++)
			{
				ttf_profile_points d_points = d.points[d0];
				if((i32)d.points[d0].y == s_y)
				{
					descender_points[descender_points_count].x = d_points.x;
					descender_points[descender_points_count].y = d_points.y;
					descender_points_count++;
					break;
				}
			}
		}


		scan_profiles_count = ascender_points_count;

		for(u32 ay = 0;
				ay < ascender_points_count;
				ay++)
		{
			for(u32 ax = 0;
					ax < ascender_points_count;
					ax++)
			{
			    ttf_profile_points a0 = ascender_points[ay];
				ttf_profile_points p_index = ascender_points[ax];
				if(p_index.x > a0.x)
				{
					ascender_points[ay] = p_index;
					ascender_points[ax] = a0;
				}

			}
		}

		for(u32 dy = 0;
				dy < descender_points_count;
				dy++)
		{
			for(u32 dx = 0;
					dx < descender_points_count;
					dx++)
			{
			    ttf_profile_points d0 = descender_points[dy];
				ttf_profile_points d1 = descender_points[dx];
				if(d1.x > d0.x)
				{
					descender_points[dy] = d1;
					descender_points[dx] = d0;
				}

			}
		}

		for(u32 s = 0;
				s < scan_profiles_count;
				s++)
		{
			ttf_profile_points scan_p0 = ascender_points[s];
			ttf_profile_points scan_p1 = descender_points[s];

			//Assert(scan_p0.x < scan_p1.x);
			u32 x0 = (u32)scan_p0.x;
			u32 x1 = (u32)scan_p1.x;
			if(scan_p1.x < scan_p0.x)
			{
				u32 x_c = x1;
				x1 = x0;
				x0 = x_c;
			}

			u32 x = x0;
			while(x < x1) 
			{
				*(pixels_32 + x + (s_y * image_width)) = 0xffffffff;

				x++;
			}
		}
		scan_profiles_count      = 0;
	    ascender_points_count  = 0;
	    descender_points_count = 0;
		s_y++;
	}

	Assert(descending_index == ascending_index);
	
	temporary_area_end(&t_profiles_area);
}

static void
ttf_draw_contours_to_pixels(
		ttf_curve *curves,
		u32 curve_count,
		u32 image_width,
		u32 image_height,
		u8 *image_pixels)
{
	u32 c = 0;
	while(c < curve_count )
	{
		ttf_curve curve = curves[c];

		Assert(curve.p0.x >= 0 && curve.p0.y >= 0 &&
				curve.p1.x >= 0 && curve.p1.y >= 0 &&
				curve.p2.x >= 0 && curve.p2.y >= 0);

		    	draw_curve_to_pixels_subdivided(
		    			curve.p0,
		    			curve.p1,
		    			curve.p2,
		    			0xffffffff,
		    			128,
		    			image_width,
		    			image_height,
		    			image_pixels);
		c++;
	}
}


inline u32
ttf_debug_check_curve_profiles(
		ttf_glyph_work *glyph_work)
{
	NotImplemented;
	u32 curve_profile_count = glyph_work->curve_profile_count;
	Assert(curve_profile_count);
	u32 c = 0;
	u32 ascend_count  = 0;
	u32 descend_count = 0;
	while(c < curve_profile_count)
	{
		ttf_curve curve = {0}; 
		vec2 p0 = curve.p0;
		vec2 p1 = curve.p1;
		vec2 p2 = curve.p2;

		vec2 mid = {
			p0.x + ((p2.x - p0.x) * 0.5f),
			p0.y + ((p2.y - p0.y) * 0.5f)
		};

	    u32 is_ascending = p0.y >= p1.y &&
	    	               p1.y >= p2.y;
	    u32 is_descending = p0.y <= p1.y &&
                            p1.y <= p2.y;

	    u32 same_height = (p2.y - p0.y) == 0; 
		if(!same_height)
		{
		    ascend_count  += is_ascending;
		    descend_count += is_descending;
		}
		else
		{
			Assert((p1.y == p0.y) && (p1.y == p2.y));
		}

		c++;
	}
	//properly count profiles

	u32 end_value = ABS((i32)ascend_count - (i32)descend_count);
	Assert(end_value == 0 || end_value == 1);

	return(1);
}

static image_data
ttf_push_and_make_atlas_from_images(
		memory_area *area,
		u32 atlas_image_width,
		u32 font_height,
		u32 glyph_image_count,
		u32 padding_x,
		u32 padding_y,
		image_data *glyph_images,
		stream_data *info_stream)
{
	image_data atlas_image = {0};

    u32 advance_y = (font_height + padding_y);

	u32 pixels_row = (atlas_image_width * 4) * advance_y;
	u32 font_image_height    = font_height;
	//Allocate image's pixels to fill on the stream and write to file.
    atlas_image.pixels = memory_area_clear_and_push(area, pixels_row);
    atlas_image.bpp    = 4;
    Assert(font_height > 0);
    Assert(glyph_image_count > 0);

    uint32 used_image_width = 0;
    uint32 row_count = 0;

	u32 gi = 0;
	while(gi < glyph_image_count)
	{
        image_data *glyph_image = glyph_images + gi;//stream_consume_data(&glyphStream, image_data);
        u32 glyph_w = glyph_image->width;
        u32 glyph_h = glyph_image->height;
		u32 advance_x = (glyph_w + padding_x);

        if(used_image_width + advance_x >= atlas_image_width)
        {
            used_image_width = 0;
            row_count++;

	    	stream_pushf(info_stream, "Pushing more pixels...\n");
	    	memory_area_clear_and_push(area, pixels_row);
	    	font_image_height += advance_y;
        }
	    u32 row_offset = row_count * advance_y;
        u32 *atlas_pixels_row = (u32 *)atlas_image.pixels + atlas_image_width * row_offset;
        atlas_pixels_row     += used_image_width;
        used_image_width   += advance_x;

        uint32 *glyphPixels = (uint32 *)glyph_image->pixels;
        for(uint32 y = 0; y < glyph_h; y++)
        {
            uint32 *atlas_pixels_row_at = atlas_pixels_row;
            for(uint32 x = 0; x < glyph_w; x++)
            {
                *atlas_pixels_row_at = *glyphPixels;
                glyphPixels++;
                atlas_pixels_row_at++;
            }
            atlas_pixels_row += atlas_image_width;

        }
		gi++;
	}

	atlas_image.width = atlas_image_width;
	atlas_image.height = font_image_height;
	return(atlas_image);
}

//same as above, but assumes the dimensions were calculated and the pixels allocated got already allocated
static image_data
ttf_fill_and_make_atlas(
		u32 font_height,
		u32 glyph_image_count,
		image_data *glyph_images,
		u32 atlas_image_width,
		u32 atlas_image_height,
		u8 *atlas_image_pixels,
		stream_data *info_stream)
{
	image_data atlas_image = {0};

	u32 pixels_row = (atlas_image_width * 4) * font_height;
	u32 font_image_height    = font_height;
	//Allocate image's pixels to fill on the stream and write to file.
    atlas_image.pixels = atlas_image_pixels; 
    atlas_image.width  = atlas_image_width;
    atlas_image.height = atlas_image_height; 
    atlas_image.bpp    = 4;
    Assert(font_height > 0);
    Assert(glyph_image_count > 0);

    uint32 used_image_width = 0;
    uint32 row_count = 0;

	u32 gi = 0;
	while(gi < glyph_image_count)
	{
        image_data *glyph_image = glyph_images + gi;//stream_consume_data(&glyphStream, image_data);
        u32 glyph_w = glyph_image->width;
        u32 glyph_h = glyph_image->height;

        if(used_image_width + glyph_w >= atlas_image_width)
        {
            used_image_width = 0;
            row_count++;

	    	font_image_height += font_height;
        }
	    u32 row_offset = row_count * font_height;
        u32 *atlas_pixels_row = (u32 *)atlas_image.pixels + atlas_image_width * row_offset;
        atlas_pixels_row         += used_image_width;

        used_image_width   += glyph_w;
        uint32 *glyphPixels = (uint32 *)glyph_image->pixels;
        for(uint32 y = 0; y < glyph_h; y++)
        {
            uint32 *atlas_pixels_row_at = atlas_pixels_row;
            for(uint32 x = 0; x < glyph_w; x++)
            {
                *atlas_pixels_row_at = *glyphPixels;
                glyphPixels++;
                atlas_pixels_row_at++;
            }
            atlas_pixels_row += atlas_image_width;

        }
		gi++;
	}

	return(atlas_image);
}

inline ttf_glyph_work
ttf_allocate_glyph_rasterizer_info(
		memory_area *area,
		ttf_file_work *font_file_work,
		ttf_glyph_info *glyph_info,
		f32 scale,
		stream_data *info_stream)
{
	u32 glyph_points_count     = glyph_info->point_coordinates_count;
	//create a glyph work to create the final image
	ttf_glyph_work glyph_work = {0};

	ttf_glyph_point *glyph_points = memory_area_push_array(
			area,
			ttf_glyph_point,
			glyph_points_count);

	//store the original glyph's points
	memory_copy_array(
			glyph_info->point_coordinates,
			glyph_points,
			ttf_glyph_point,
			glyph_points_count);
	//scale, offset the points and convert them to pixel coordinates
	ttf_flip_glyph_points_vertically(
			glyph_points_count,
			glyph_points,
			1,
			info_stream);

	ttf_glyph_info glyph_info_copy = *glyph_info;
	glyph_info_copy.point_coordinates = glyph_points;

	ttf_glyph_work_store_glyph_contours(
			area,
			&glyph_work,
			&glyph_info_copy);

    ttf_flatten_curves(
			area,
			&glyph_work,
			scale);

	return(glyph_work);
}

static image_data
ttf_create_glyph_bitmap_anti_aliased(
		memory_area *area,
		ttf_file_work *font_file_work,
		f32 glyph_scale,
		ttf_glyph_info *glyph_info,
		stream_data *info_stream)
{
	image_data result = {0};
    
	//ttf_glyph_info *glyph_info = font_file_work.glyphs_info + g;


	//gather required data
	u32 glyph_points_count = glyph_info->point_coordinates_count;
	u32 glyph_index        = font_file_work->glyph_indices[glyph_info->glyph_code];

	i32 min_x = (i32)f32_floor(glyph_info->min_x * glyph_scale);
	i32 min_y = (i32)f32_floor(glyph_info->min_y * glyph_scale);

	i32 max_x = (i32)f32_ceil(glyph_info->max_x * glyph_scale);
	i32 max_y = (i32)f32_ceil(glyph_info->max_y * glyph_scale);

	u32 total_glyph_w =  0 + (u32)(max_x - min_x);
	u32 total_glyph_h =  0 + (u32)(max_y - min_y);

	u32 pixel_buffer_size = (total_glyph_w * total_glyph_h * 4);
	u8 *pixel_buffer = memory_area_clear_and_push(
			area,
			pixel_buffer_size);
	result.width  = total_glyph_w;
	result.height = total_glyph_h;
	result.pixels = pixel_buffer;
	result.bpp = 4;

	//leave this bitmap as empty
	if(glyph_info->glyph_code == ' ')
	{
		return(result);
	}
	temporary_area temporary_glyph_work_area = temporary_area_begin(area);

	//create a glyph work to create the final image
	ttf_glyph_work glyph_work = ttf_allocate_glyph_rasterizer_info(
			area,
			font_file_work,
			glyph_info,
			glyph_scale,
			info_stream);

	//rasterizer
    ttf_glyph_work_rasterize_to_pixels_anti_alias(
	        area,
	        &glyph_work,
    		total_glyph_w,
    		total_glyph_h,
    		pixel_buffer);

	temporary_area_end(&temporary_glyph_work_area);
	//g++;
	return(result);
}

static image_data
ttf_create_glyph_bitmap_non_anti_aliased(
		memory_area *area,
		f32 glyph_scale,
		ttf_file_work *font_file_work,
		ttf_glyph_info *glyph_info,
		stream_data *info_stream)
{
	image_data result = {0};
    
	//ttf_glyph_info *glyph_info = font_file_work.glyphs_info + g;


	//gather required data
	i32 min_x = (i32)f32_floor(glyph_info->min_x * glyph_scale);
	i32 min_y = (i32)f32_floor(glyph_info->min_y * glyph_scale);

	i32 max_x = (i32)f32_ceil(glyph_info->max_x * glyph_scale);
	i32 max_y = (i32)f32_ceil(glyph_info->max_y * glyph_scale);

	u32 total_glyph_w =  0 + (u32)(max_x - min_x);
	u32 total_glyph_h =  0 + (u32)(max_y - min_y);

	u32 pixel_buffer_size = (total_glyph_w * total_glyph_h * 4);
	u8 *pixel_buffer = memory_area_clear_and_push(
			area,
			pixel_buffer_size);
	result.width  = total_glyph_w;
	result.height = total_glyph_h;
	result.pixels = pixel_buffer;
	result.bpp = 4;

	//leave this bitmap as empty
	if(glyph_info->glyph_code == ' ')
	{
		return(result);
	}
	temporary_area temporary_glyph_work_area = temporary_area_begin(area);

	//create a glyph work to create the final image
	ttf_glyph_work glyph_work = ttf_allocate_glyph_rasterizer_info(
			area,
			font_file_work,
			glyph_info,
			glyph_scale,
			info_stream);

	//rasterizer
    ttf_glyph_work_rasterize_to_pixels_non_anti_alias(
	        area,
	        &glyph_work,
    		total_glyph_w,
    		total_glyph_h,
    		pixel_buffer);

	temporary_area_end(&temporary_glyph_work_area);
	//g++;
	return(result);
}

static image_data
ttf_create_glyph_contour_image(
		memory_area *area,
		f32 glyph_scale,
		ttf_file_work *font_file_work,
		ttf_glyph_info *glyph_info,
		stream_data *info_stream)
{
	image_data result = {0};
    
	//ttf_glyph_info *glyph_info = font_file_work.glyphs_info + g;


	//gather required data
	i32 min_x = (i32)f32_floor(glyph_info->min_x * glyph_scale);
	i32 min_y = (i32)f32_floor(glyph_info->min_y * glyph_scale);

	i32 max_x = (i32)f32_ceil(glyph_info->max_x * glyph_scale);
	i32 max_y = (i32)f32_ceil(glyph_info->max_y * glyph_scale);

	u32 total_glyph_w =  0 + (u32)(max_x - min_x);
	u32 total_glyph_h =  0 + (u32)(max_y - min_y);

	u32 pixel_buffer_size = (total_glyph_w * total_glyph_h * 4);
	u8 *pixel_buffer = memory_area_clear_and_push(
			area,
			pixel_buffer_size);
	result.width  = total_glyph_w;
	result.height = total_glyph_h;
	result.pixels = pixel_buffer;
	result.bpp = 4;

	//leave this bitmap as empty
	if(glyph_info->glyph_code == ' ')
	{
		return(result);
	}
	temporary_area temporary_glyph_work_area = temporary_area_begin(area);

	//create a glyph work to create the final image
#if 0
	ttf_glyph_work glyph_work = ttf_allocate_glyph_rasterizer_info(
			area,
			font_file_work,
			glyph_info,
			glyph_scale,
			info_stream);
#else
	ttf_glyph_work glyph_work = {0};

	u32 glyph_points_count     = glyph_info->point_coordinates_count;
	//create a glyph work to create the final image
	ttf_glyph_point *glyph_points = memory_area_push_and_copy_array(
			area,
			ttf_glyph_point,
			glyph_info->point_coordinates,
			glyph_points_count);

	ttf_flip_glyph_points_vertically(
			glyph_points_count,
			glyph_points,
			1,
			info_stream);

	ttf_glyph_info cheese_info = *glyph_info;
	cheese_info.point_coordinates = glyph_points;

    ttf_glyph_work_store_glyph_contours(
    		area,
    		&glyph_work,
    		&cheese_info);
#endif
	//rasterizer
	u32 total_contours = 0;
	for(u32 c = 0;
			c < glyph_work.contour_count;
			c++)
	{
		total_contours += glyph_work.curve_counts_per_contour[c];
	}
	for(u32 t = 0;
			t < total_contours;
			t++)
	{
		ttf_curve *c = glyph_work.glyph_curves + t;
		c->p0 = vec2_scale(c->p0, glyph_scale);
		c->p1 = vec2_scale(c->p1, glyph_scale);
		c->p2 = vec2_scale(c->p2, glyph_scale);
	}
    ttf_draw_contours_to_pixels(
	        glyph_work.glyph_curves,
			total_contours,
    		total_glyph_w,
    		total_glyph_h,
    		pixel_buffer);

	temporary_area_end(&temporary_glyph_work_area);
	//g++;
	return(result);
}

static image_data
ttf_create_glyph_bitmap_index_sdf(
		memory_area *area,
		ttf_file_work *font_file_work,
		f32 scale,
		u32 glyph_index,
		stream_data *info_stream)
{
	image_data result = {0};

	if(font_file_work)
	{
		ttf_glyph_info *glyph_info = font_file_work->glyphs_info + glyph_index;

#if 0
        result = ttf_create_glyph_bitmap_non_anti_aliased(
        		area,
        		scale,
        		font_file_work,
        		glyph_info,
        		info_stream);
#endif
		result = ttf_create_glyph_contour_image(
				area,
				scale,
				font_file_work,
				glyph_info,
				info_stream);
		image_convert_to_sdf(
				area,
				&result,
				0xffffffff);

	}

	return(result);

	//get a non anti-alised glyph with black background
	// generate a "boolean" array indicating where the pixel values are
	// reeplace all true values with 0 and false with infinity
    //inline f32
	/*
EDT: (Euclidean Distance Transform) can be defined as consuming a field of booleans and producing a field of scalars such that each value in the output is the distance to the nearest “true” cell in the input.

	   compute_edt(u8 *bool_field, u32 width)
	  {
          sedt = bool_field.where(0, ∞)
          for(u32 w = 0;
		          w < width;
				  w++)
		  {
              horizontal_pass(sedt[row])
		  }
		  //rotate by 90º
          transpose(sedt)
          for row in len(sedt):
              horizontal_pass(sedt[row])
          transpose(sedt)
	  }
      static void
	  find_hull_parabolas(
	   u8 *single_row,
	   u32 row_length,
	   vec2 *hull_vertices,
	   vec2 *hull_intersections)
	   {
          u8 *d = single_row;
          vec2 *v = hull_vertices;
          vec2 *z = hull_intersections;
          k = 0;
          v[0].x = 0;
          z[0].x = f32_MIN;
          z[1].x = f32_MAX;
          for( u32 i = 0;
		           i < row_length;
				   i++)
		  {
              q = (i, d[i])
              p = v[k]
              s = intersect_parabolas(p, q)
              while s.x <= z[k].x:
                  k = k - 1
                  p = v[k]
                  s = intersect_parabolas(p, q)
              k = k + 1
              v[k] = q
              z[k].x = s.x
              z[k + 1].x = +INF
		 }
	 }
    return sqrt(sedt)
    */
}

inline image_data
ttf_create_glyph_bitmap_from_index_anti_aliased(
		memory_area *area,
		ttf_file_work *font_file_work,
		f32 scale,
		u32 glyph_index,
		stream_data *info_stream)
{
	image_data result = {0};
	if(font_file_work)
	{
    	u32 glyph_count = font_file_work->requested_glyph_e - font_file_work->requested_glyph_s + 1;
    
		if(glyph_index < glyph_count)
		{
			result =
            ttf_create_glyph_bitmap_anti_aliased(
    	    	area,
				font_file_work,
    	    	scale,
    	    	font_file_work->glyphs_info + glyph_index,
    	    	info_stream);
		}
	}
	return(result);
}

inline image_data
ttf_create_glyph_bitmap_from_index_non_anti_aliased(
		memory_area *area,
		ttf_file_work *font_file_work,
		f32 scale,
		u32 glyph_index,
		stream_data *info_stream)
{
	image_data result = {0};
	if(font_file_work)
	{
    	u32 glyph_count = font_file_work->requested_glyph_e - font_file_work->requested_glyph_s + 1;
    
		if(glyph_index < glyph_count)
		{
			result =
            ttf_create_glyph_bitmap_non_anti_aliased(
    	    	area,
    	    	scale,
				font_file_work,
    	    	font_file_work->glyphs_info + glyph_index,
    	    	info_stream);
		}
	}
	return(result);
}


static image_data
ttf_create_glyph_bitmap_for_atlas_anti_aliased(
		memory_area *area,
		u32 font_height,
		ttf_file_work *font_file_work,
		ttf_glyph_info *glyph_info,
		u32 anti_alias,
		stream_data *info_stream)
{
	//needed for re-positioning the glyph's pixels
	f32 glyph_scale = ttf_get_height_scale(font_file_work, font_height);
    u32 baseline    = (u32)((font_file_work->baseline_ascender) * glyph_scale);

	//allocate resulting image
	image_data result_image = {0};

	u32 glyph_image_w = (u32)(ttf_work_get_glyph_code_bitmap_w(font_file_work, glyph_info->glyph_code, glyph_scale));
	u32 glyph_image_h = font_height;

	result_image.width  = glyph_image_w;
	result_image.height = glyph_image_h;
	result_image.pixels = memory_area_clear_and_push(
	        		area,
	        		glyph_image_w * glyph_image_h * 4);

	//temporarely allocate the glyph's bitmap 
    temporary_area temporary_glyph_image_area = temporary_area_begin(area);
	//gather glyph data
	image_data glyph_image = {0};
	if(anti_alias)
	{
	glyph_image = ttf_create_glyph_bitmap_anti_aliased(
			area,
			font_file_work,
			glyph_scale,
			glyph_info,
			info_stream);
	}
	else
	{
	glyph_image = ttf_create_glyph_bitmap_non_anti_aliased(
			area,
			glyph_scale,
			font_file_work,
			glyph_info,
			info_stream);
	}

	u32 glyph_index = font_file_work->glyph_indices[glyph_info->glyph_code];
	ttf_horizontal_metrics_record glyph_horizontal_metrics = font_file_work->horizontal_metrics_records[glyph_index];

	u32 copy_to_x = 0; 
	u32 copy_to_y = baseline - (u32)f32_ceil(glyph_info->max_y * glyph_scale);
	if(glyph_horizontal_metrics.lsb > 0)
	{
        copy_to_x = (u32)(glyph_horizontal_metrics.lsb * glyph_scale);
	}
	else
	{
		i32 brek = 0;
	}

	//get coordinates to the final result image
	u8 *copy_to = get_pixel_coordinates_from(
			copy_to_x,
			copy_to_y,
			result_image.width,
			result_image.height,
			result_image.pixels);

	//copy temporary buffer to final result
	copy_pixels_to(
			glyph_image.width,
			glyph_image.height,
			glyph_image.pixels,
			result_image.width,
			result_image.height,
			copy_to);

	//free memory
	temporary_area_end(&temporary_glyph_image_area);

	return(result_image);
}


static u32
ttf_fill_file_work_from_path(
		memory_area *area,
		platform_api *platform,
		ttf_file_work *font_file_work,
		u32 first_glyph_code,
		u32 last_glyph_code,
		u8 *font_path_and_name,
		stream_data *info_stream
		)
{

	u32 success = 0;
	platform_file_handle font_file_handle = platform->f_open_file(
			font_path_and_name, platform_file_op_read);

	if(font_file_handle.handle)
	{
		//read and get the whole font's file data
		platform_entire_file entire_font_file = platform_read_entire_file_handle(
				platform,
				font_file_handle,
				area);
		//close the file
		platform->f_close_file(font_file_handle);
		success = entire_font_file.contents > 0;
    
		if(success)
		{
		    //gather the requested information
    	    *font_file_work = 
            ttf_parse_from_memory(
    	    	area,
    	    	first_glyph_code,
    	    	last_glyph_code,
    	    	entire_font_file.contents,
    	    	0);
		}
		else
		{
			//log
		}
	}

	return(success);
}

static u32
ttf_fill_file_work_from_handle(
		platform_api *platform,
		memory_area *area,
		stream_data *info_stream,
		ttf_file_work *font_file_work,
		u32 first_glyph_code,
		u32 last_glyph_code,
		platform_file_handle font_file_handle)
{

	u32 success = 0;
	platform_entire_file entire_font_file = platform_read_entire_file_handle(
			platform,
			font_file_handle,
			area);
    
	if(entire_font_file.contents)
	{
	    //gather the requested information
        *font_file_work = 
        ttf_parse_from_memory(
        	area,
        	first_glyph_code,
        	last_glyph_code,
        	entire_font_file.contents,
        	0);
	    success = 1;
	}

	return(success);
}

inline void
ttf_fill_glyph_bitmap_dimensions(
		ttf_file_work *font_file_work,
		u32 glyph_index,
		f32 glyph_scale,
		u32 *w,
		u32 *h)
{
	ttf_glyph_info *glyph_info = font_file_work->glyphs_info + glyph_index;

	i32 min_x = (i32)f32_floor(glyph_info->min_x * glyph_scale);
	i32 min_y = (i32)f32_floor(glyph_info->min_y * glyph_scale);

	i32 max_x = (i32)f32_ceil(glyph_info->max_x * glyph_scale);
	i32 max_y = (i32)f32_ceil(glyph_info->max_y * glyph_scale);

	u32 total_glyph_w =  0 + (u32)(max_x - min_x);
	u32 total_glyph_h =  0 + (u32)(max_y - min_y);

	if(w)
	{
		*w = total_glyph_w;
	}
	if(h)
	{
		*h = total_glyph_h;
	}
}

inline image_data
ttf_calculate_atlas_dimensions(
		ttf_file_work *font_file_work,
		u32 image_width,
		u32 font_height,
		u32 padding_x,
		u32 padding_y)
{
	u32 requested_glyph_count = font_file_work->requested_glyph_e - font_file_work->requested_glyph_s + 1;

	f32 glyph_scale = ttf_get_height_scale(font_file_work, font_height);
	u32 g = 0;
	u32 used_width  = 0;
	u32 used_height = font_height;
	while(g < requested_glyph_count)
	{
    
	    ttf_glyph_info *glyph_info = font_file_work->glyphs_info + g;
	    u32 glyph_image_w = (u32)(ttf_work_get_glyph_code_bitmap_w(font_file_work, glyph_info->glyph_code, glyph_scale));
	    u32 glyph_image_h = font_height;
		u32 advance_x = (padding_x + glyph_image_w);

		if(used_width + advance_x > image_width)
		{
		    u32 advance_y = (padding_y + glyph_image_h);
			used_width = 0;
			used_height += advance_y;
		}
		used_width += advance_x;

	    g++;
	}

	image_data atlas_image = {0};
	atlas_image.width  = image_width;
	atlas_image.height = used_height;
	atlas_image.bpp = 4;

	return(atlas_image);
}

static image_data 
ttf_create_atlas_padding(
		platform_api *platform,
		memory_area *area,
		stream_data *info_stream,
		ttf_file_work *font_file_work,
		u32 image_width,
		u32 font_height,
		u32 anti_alias,
		u32 padding_x,
		u32 padding_y)
{
	//gather the requested information
	u32 requested_glyph_count = font_file_work->requested_glyph_e - font_file_work->requested_glyph_s + 1;
	//u32 font_height = ttf_get_font_height(font_file_work, scale);

	image_data *glyph_images = memory_area_push_array(
			area,
			image_data,
			requested_glyph_count);

	image_data atlas_image = ttf_calculate_atlas_dimensions(
			font_file_work,
			image_width,
			font_height,
			padding_x,
			padding_y);

	atlas_image.pixels = memory_area_push_size(area,
			atlas_image.width * atlas_image.height * atlas_image.bpp);

	f32 glyph_scale = ttf_get_height_scale(font_file_work, font_height);

	u32 used_image_width = 0;
	u32 row_count = 0;
	u32 g = 0;
	u32 atlas_pixel_row_size = (atlas_image.width * 4) * font_height;
	while(g < requested_glyph_count)
	{
    
	    ttf_glyph_info *glyph_info = font_file_work->glyphs_info + g;

		//get the dimensions of the next glyph image
	    u32 glyph_image_w = (u32)(ttf_work_get_glyph_code_bitmap_w(font_file_work, glyph_info->glyph_code, glyph_scale));
	    u32 glyph_image_h = font_height;
		u32 advance_x = (padding_x + glyph_image_w);

        if(used_image_width + advance_x >= atlas_image.width)
        {
            used_image_width = 0;
            row_count++;

	    	stream_pushf(info_stream, "Pushing more pixels...\n");
	    	//memory_area_clear_and_push(area, atlas_pixel_row_size);
        }

		temporary_area temporary_glyph_image_area = temporary_area_begin(area);

        image_data glyph_image = ttf_create_glyph_bitmap_for_atlas_anti_aliased(
        		area,
        		font_height,
        		font_file_work,
        		glyph_info,
			    anti_alias,
				info_stream);

		temporary_area_end(&temporary_glyph_image_area);

	    u32 row_offset        = row_count * (font_height + padding_y);
        u32 *atlas_pixels_row = (u32 *)atlas_image.pixels + atlas_image.width * row_offset;

        atlas_pixels_row += used_image_width;
        used_image_width += advance_x;

        uint32 *glyphPixels = (uint32 *)glyph_image.pixels;
        for(uint32 y = 0; y < glyph_image_h; y++)
        {
            uint32 *atlas_pixels_row_at = atlas_pixels_row;
            for(uint32 x = 0; x < glyph_image_w; x++)
            {
                *atlas_pixels_row_at = *glyphPixels;
                glyphPixels++;
                atlas_pixels_row_at++;
            }
            atlas_pixels_row += atlas_image.width;

        }
	    g++;
	}
	


	return(atlas_image);
}

inline image_data
ttf_create_atlas_anti_aliased(
		platform_api *platform,
		memory_area *area,
		stream_data *info_stream,
		ttf_file_work *font_file_work,
		u32 image_width,
		u32 font_height,
		u32 anti_alias)
{

    ttf_create_atlas_padding(
    		platform,
    		area,
    		info_stream,
    		font_file_work,
    		image_width,
    		font_height,
    		anti_alias,
    		0,
    		0);
}

static image_data 
ttf_create_atlas_from_path_anti_aliased(
		platform_api *platform,
		memory_area *area,
		stream_data *info_stream,
		u32 image_width,
		u32 font_height,
		u32 first_glyph_code,
		u32 last_glyph_code,
		u8 *font_path_and_name)
{
	image_data result            = {0};
	ttf_file_work font_file_work = {0};

	//gather the requested information
    u32 parsing_success = ttf_fill_file_work_from_path(
    		area,
    		platform,
    		&font_file_work,
    		first_glyph_code,
    		last_glyph_code,
    		font_path_and_name,
    		info_stream);

	if(parsing_success)
	{
		result = ttf_create_atlas_anti_aliased(
		platform,
		area,
		info_stream,
		&font_file_work,
		image_width,
		font_height,
		1);
	}


	return(result);
}

static image_data 
ttf_create_atlas_from_path_non_anti_aliased(
		platform_api *platform,
		memory_area *area,
		stream_data *info_stream,
		u32 image_width,
		u32 font_height,
		u32 first_glyph_code,
		u32 last_glyph_code,
		u8 *font_path_and_name)
{
	image_data result            = {0};
	ttf_file_work font_file_work = {0};

	//gather the requested information
    u32 parsing_success = ttf_fill_file_work_from_path(
    		area,
    		platform,
    		&font_file_work,
    		first_glyph_code,
    		last_glyph_code,
    		font_path_and_name,
    		info_stream);

	if(parsing_success)
	{
		result = ttf_create_atlas_anti_aliased(
		platform,
		area,
		info_stream,
		&font_file_work,
		image_width,
		font_height,
		0);
	}


	return(result);
}
