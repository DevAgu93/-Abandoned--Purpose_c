/*NOTES:

=== ppan ===
(Requires one or multiple ppmo)

The animation .ppan format starts with a header containing the total amount of 
clips, keyframes, uvs (for frames), pre-defined frame lists and frames. It is
followed by the pre defined frame lists and frames, then the actual animations.

======

=== pptl ===
(Requires an image ppimg, png or bmp)

Starts with header containing:
- terrain_count : "terrain" or "brushes" count.
- image_id : the id of the image it uses to render.
- image_source_length : length of path and name of the image.
- offset_to_terrain_data : offset to "paint" or "brushes" data. Saved as ppse_tileset_terrain.
- offset_to_image_source : offset where the image path and name is.

ppse_tileset_terrain contains:
- mesh_count : of ppse_mesh
- shape : 
Followed by ppse_mesh with an amount of mesh_count.
======

=== ppmo ===
(Requires one or multiple images ppimg, png or bmp)
======

=== ppmp ===
(Requires one or multiple pptl, ppmo, entities...)
======

Formats:
Tileset:   pptl
font:      pfnt
map:       ppmp
animation: ppan
model:     ppmo
custom img pimg
Entity:    pcos

*/
//===========PPSE file specifics
#define ppse_SIGNATURE FormatEncodeU32('ppse')
#define ppse_version 1
//#define FormatDecode(a,b,c,d) ((a >> 0) | (b >> 8) | (c >> 16) | (d >> 24))
//#define FormatEncode(a,b,c,d) ((a << 0) | (b << 8) | (c << 16) | (d << 24))
#define FormatDecode(a) ((a[0] >> 0) | (a[1] >> 8) | (a[2] >> 16) | (a[3] >> 24))
#define FormatEncode(a) ((a[0] << 0) | (a[1] << 8) | (a[2] << 16) | (a[3] << 24))
#define FormatEncodeU32(a) (((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) | ((a & 0x0000ff00) << 8) | ((a & 0x000000ff) << 24))
//#define FOURCC (s) (((uint32)(s[0]) << 0) | ((uint32)(s[1]) << 8) | ((uint32)(s[2]) << 16) | ((uint32)(s[3]) << 24))
//#define FOURCC (a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))
#define game_resource_path_and_name_MAX 126

typedef enum{
	
	file_result_success = 0,
	file_result_invalid_handle  = 0x02,
	file_result_signature_error = 0x04,
	file_result_version_error = 0x08,
	file_result_composite_resource_not_found = 0x10,
	file_result_composite_resource_signature_failed = 0x20,
	file_result_composite_resources_missing = 0x30,
	file_result_needs_creating = 0x40,
	file_result_composite_resource_load_error = 0x80
}resource_check_result;

typedef enum{
	file_info_new,
	file_info_reeplace,
	file_info_uncompatible,
	file_info_packed,
	file_info_easy_image,
}asset_file_info_flags;

typedef enum{

    asset_type_undefined = 0,

    asset_type_image = 1,
    asset_type_font = 2,
    asset_type_sound = 3,

    asset_type_map = 4,
    asset_type_model = 5,
    asset_type_frame_animation = 6,
    asset_type_tileset = 7,
    asset_type_entity = 8,

    asset_type_COUNT
}asset_type;

typedef enum{
	asset_format_png,
	asset_format_bmp,
	asset_format_pimg,
	asset_format_pptl,
	asset_format_pfnt,
	asset_format_ppmp,
	asset_format_ppmo,
	asset_format_pcos,
	asset_format_pfan,

	asset_format_COUNT
}asset_format;

typedef enum{
    asset_state_free,
	asset_state_data_unloaded,
    asset_state_loaded,
    asset_state_Requested,

    asset_state_invalid,
    asset_state_not_compatible,
    asset_state_outdated,
}asset_state;

typedef enum{
	asset_source_coded,
	asset_source_packed,
	asset_source_from_disk
}asset_source;

typedef enum{
	resource_flags_outdated = 0x1,
}resource_flags;

typedef struct{
	u32 id;
	u8 path_and_name[256];
}asset_identifier;

//Note(Agu) If the size of the texture isn't enough, I might need to store an index.
//===================Loaded asset 

typedef struct{
	u32 index;
	u32 id;
}asset_key;

#pragma pack(push,1)

    typedef struct{
    	u32 signature;
    	u32 asset_count;
    }ppse_raw_assets_header;
    
    typedef struct{
    	u32 id;
        u32 type;
    	u8 path_and_name[256];
    }ppse_asset_identifier;
    
    typedef struct{
        uint16 textureOffsetX;
        uint16 textureOffsetY;
    }ppse_glyph;
    
    typedef struct{
    	u32 signature;
    
        u16 pixelsW;
        u16 pixelsH;
    
        u16 glyph_count;
        u16 font_height;
       //uint32 pixelTableCount;
    	u32 offset_to_pixels;
    
    }ppse_font;
    
    typedef struct{
    	u32 signature;
    
    	u16 w;
    	u16 h;
    	u32 bpp;
    }ppse_image;
    
    typedef struct{
        u32 signature; //Identifier
    	u32 version;
        //uint32 totalSize;
        u32 asset_count;
        u32 offset_to_file_headers;
    
    	u32 reserved[10];
    }  ppse_file_header;
    
    typedef struct{
    	uint32 nameLength;
    	uint8 *name;
    }ppse_asset_data_entry;
    
    typedef struct{
        u32 id;
        u32 data_offset;
    
    	u32 data_size;
    	u64 date;
    	//nameLength
    
        //asset_type type;
    	u32 reserved[10];
    
    }ppse_asset_data;
    
    typedef struct{
    	u32 offset_to_name;
    	ppse_asset_data data;
    }ppse_offset_and_asset_data;
    
    typedef struct{
    	u32 packaged;
    	u32 offset_to_name;
    	ppse_asset_data data;
    	platform_file_handle handle;
    }ppse_resource_handle;
    
    
    //headers for all assets
    typedef struct{
    	u32 signature;
    	u32 version;
    	u16 composite_resource_count;
    	u16 offset_to_first_line;
    	u32 offset_to_composite_resources;
    }ppse_asset_header;
    
    
    //
    //tileset
    //
    //typedef struct{
    //	u32 signature;
    //	u32 version;
    //	u32 terrain_count;
    //	u32 mesh_count;
    //
    //	u32 image_id;
    //	u32 offset_to_terrain_data;
    //	u32 offset_to_image_source;
    //	u32 image_source_length;
    //	//editor_stuff
    //
    //	u32 offset_to_terrain_names;
    //	u16 autoterrain_count;
    //	u16 autoterrain_indices_capacity;
    //	u32 offset_to_autoterrain_data;
    //	u32 offset_to_autoterrain_names;
    //
    //	u32 reserved[17];
    //}ppse_tileset_header;
    typedef struct{
		u16 number;
		b16 end;
		u32 offset;
		u32 next_line_offset;
		u32 prev_line_offset;
		u32 reserved[10];
	}ppse_line;

    typedef struct{
		ppse_asset_header header;
		u16 default_state;
		u16 state_count;
		u16 action_count;
		u16 state_lines_count;
		u16 action_lines_count;
		u16 state_conditions_count;

		u8 line_to_composite_resources;
		u8 line_to_stats;
		u8 line_to_actions;
		u8 line_to_states;
		u8 line_to_state_names;

		u8 line_to_collisions;
		u32 collision_count;
		u32 reserved[9];
	}ppse_coso_header;

    typedef struct{
		vec3 offset;
		vec3 size;
		u32 reserved[10];
	}ppse_coso_collision;

    typedef struct{
		f32 speed;
		f32 z_speed;
		f32 speed_max;
		f32 collision_x;
		f32 collision_y;
		f32 collision_z;
		f32 collision_offset_x;
		f32 collision_offset_y;
		f32 collision_offset_z;
		u32 reserved[30];
	}ppse_coso_stats;

    typedef struct{
		u16 line_count;
		u32 reserved[10];
	}ppse_coso_state;

    typedef struct{
		u32 condition_count;
		state_do_flags flags;
		u16 state_index;
		u16 action_index;
		u32 reserved[10];
	}ppse_coso_state_line;

    typedef struct{
		state_trigger_type type;
		b16 not;
		b16 eq;
		f32 radius;
		u32 reserved[10];
	}ppse_coso_state_condition;

    typedef struct{
		u16 line_count;
		u32 reserved[10];
	}ppse_coso_action;

    typedef struct{
		state_action_line_type type;
		u16 target_index;
		u16 animation_index;
		f32 time_total;
		u32 reserved[10];
	}ppse_coso_action_line;
    
    typedef struct{
    	ppse_asset_header header;
    
    	u16 terrain_count;
    	u16 autoterrain_count;
    	u32 mesh_count;
    	u32 offset_to_terrain_data;
    	u32 offset_to_image_source;
    
    	u32 offset_to_terrain_names;
    	u32 autoterrain_indices_capacity;
    	u32 offset_to_autoterrain_data;
    	u32 offset_to_autoterrain_names;
    
    	u32 reserved[17];
    }ppse_tileset_header;
    
    typedef struct{
    	ppse_asset_header header;
    
    	u16 terrain_count;
    	u16 autoterrain_count;
    	u32 mesh_count;
    	u16 autoterrain_indices_capacity;

    	u8 line_to_terrain_data;
    	u8 line_to_image_source;
    	u8 line_to_names;
    	u8 line_to_autoterrain_data;
    
		union{
		u32 _n;
		u8 line_to_walls;
		};
		u32 wall_count;
    	u32 reserved[8];
	}ppse_tileset_header_new;
    
    typedef struct{
    	u32 total_frames;
    
    	u32 tiles_at;
    
    	u32 reserved[20];
    }ppse_tileset_animated_tile;
    
    typedef struct{
    	u32 shape;
    	u32 mesh_count;
		u32 terrain_group;
		u16 wall_index;
		b16 use_wall;
		u32 capacity;
    	u32 reserved[17];
    }ppse_tileset_terrain;

    typedef struct{
		u16 uvs_count;
		b16 repeat;
		u16 extra_frames_x;
		u16 extra_frames_y;
		u32 corner_frames;
		u32 reserved[8];
	}ppse_tileset_wall;

    typedef struct{
    	union{
    		uvs uvs;
    		struct{
    			vec2 uv0;
    			vec2 uv1;
    			vec2 uv2;
    			vec2 uv3;
    		};
    	};
		u32 reserved[10];
	}ppse_tileset_wall_uvs;
    
    typedef struct{
    	u32 capacity;
		u16 terrain_group;
		u16 extra_layers;
    	u32 reserved[9];
    }ppse_tileset_autoterrain;
    
    typedef struct{
    	model_mesh m;
    }ppse_mesh;
    
    typedef struct{
    	union{
    		uvs uvs;
    		struct{
    			vec2 uv0;
    			vec2 uv1;
    			vec2 uv2;
    			vec2 uv3;
    		};
    	};
    
    }ppse_uvs;

    typedef struct{
    	union{
    		uvs uvs;
    		struct{
    			vec2 uv0;
    			vec2 uv1;
    			vec2 uv2;
    			vec2 uv3;
    		};
    	};
    
		f32 offset_x;
		f32 offset_y;
		f32 offset_z;
		u16 option;
		i8 x_rot_index;
		i8 y_rot_index;
		u32 skin_index;
		u32 reserved[8];
	}ppse_frame_list_uvs;
    
    //
    // map
    //
    
    typedef struct{
    	ppse_asset_header header;
    	
    	u8 line_to_tileset_paths;
    	u8 line_to_external_entities;
    
    	u16 map_terrain_count;
    
    	u16 model_count;
    	u16 entity_count;
    	u16 tileset_count;
    	u32 map_w;
    	u32 map_h;
    
    	u8 line_to_terrain_data;
    	u8 line_to_entity_data;
		u16 line_to_hareas;
		u16 line_to_tags;
    	u32 reserved[29];
    }ppse_map_file_header;

    typedef struct{
		u32 type;
		f32 x;
		f32 y;
		f32 z;
		f32 w;
		f32 h;
		u16 model_count;
		//where to start reading on the composite resources array
		u16 composite_model_at;
		u32 reserved[10];
	}ppse_map_harea;

    typedef struct{
		u32 type;
		vec3 position;
		//indices to composite resources of the map
		u32 model_index;
		u32 entity_index;
		u32 reserved[30];
	}ppse_map_entity;
    
    typedef struct{
    	u16 tileset_index;
    	u16 tileset_terrain_index;
    	i32 height;
    
    	b16 is_autoterrain;
    	u16 autoterrain_index;
    	u32 reserved[59];
    
    }ppse_world_tile;
    
    typedef struct{
    	u32 width;
    	u32 height;
    	vec3 position;
    
    	f32 rotation_x;
    	f32 rotation_y;
    	f32 rotation_z;
    
    	u32 tileSize;
    	//u32 *tiles;
    	u32 tile_count;
    	u32 tileCapacity;
    }ppse_grid;
    
    // map
    
    //
    // model
    //
    
    typedef struct{
    	ppse_asset_header header;
    
		//bone and figure data
    	u32 bone_count;
    	u32 sprite_count;
    	u32 sprite_sheet_count;
    	u32 uvs_count;
		//animation data
    	u32 animation_count;
    	u16 keyframe_count;
    	u16 frame_keyframe_count;
    	u32 frame_list_count;
    
    
    	u32 offset_to_animations;
    	u32 offset_to_model_data;
		u32 offset_to_animation_data;
    	u32 offset_to_sheet_headers;
		//name chunks
    	u32 offset_to_editor_data;

		u32 orientation_amount;
    	u32 reserved[19];
    }ppse_model_header;

    typedef struct{
    	ppse_asset_header header;
		//bone and figure data
    	u16 bone_count;
    	u16 sprite_count;
    	u16 sprite_sheet_count;
    	u16 uvs_count;
		//animation data
    	u16 animation_count;
    	u16 keyframe_count;
    	u16 frame_keyframe_count;
    	u16 frame_list_count;
    
    
		u8 line_to_bones;
		u8 line_to_sprites;
		u8 line_to_frame_lists;
    	u8 line_to_animations;
		u8 line_to_animation_data;
    	u8 line_to_sheet_headers;
		//name chunks
    	u8 line_to_names;
		u8 orientation_amount;

    	u32 reserved[20];
    }ppse_model_header_new;
    
    typedef struct{
    	f32 offset_x;
    	f32 offset_y;
    	f32 offset_z;
    
    	vec2 uv0;
    	vec2 uv1;
    	vec2 uv2;
    	vec2 uv3;
    
    	u32 reserved[40];
    }ppse_sprite_orientation;
    
    typedef struct{
    	u32 parent;
    	vec3 p;
    	vec3 displacement;
    
    	union{
    		quaternion q;
    		struct{
    			f32 w;
    			f32 x;
    			f32 y;
    			f32 z;
    		};
    	};
    
		b32 two_dim;
		u16 sprite_count;
		u16 frame_key_count;
    	u32 reserved[57];
    
    }ppse_model_bone;
    
    typedef struct{
    
    	u32 type;
    	u32 bone_index;
    
    	u32 uvs_count;
    
    	vec3 p;
    
    	f32 depth_x;
    	f32 depth_y;
    	f32 depth_z;
    
    	f32 pivotX;
    	f32 pivotY;
    	f32 pivotZ;
    
    	u32 face_axis;
    
		u32 frame_list_index;
    	u32 height;
    
        f32 rotation_x;
        f32 rotation_y;
        f32 rotation_z;
    
    	//sprite_animation animations[10];
    
    	u32 texture_index;
    
    	vec3 v0;
    	vec3 v1;
    	vec3 v2;
    	vec3 v3;
    
        ppse_sprite_orientation orientations[16];
        //sprite_orientation orientations[16];
		u32 extra_frame_count;
    	u32 reserved[29];
    
    }ppse_model_sprite;
    
    typedef struct{
		u32 attached_bone_index;
		u32 reserved[10];
	}ppse_model_attachment_data;
    
    typedef struct{
    	u32 signature;
    	u32 offset_to_animation_names;
    	u32 offset_to_frame_list_names;
    	u32 reserved[10];
    }ppse_animation_editor_data;
    
    typedef struct{
    	u32 animation_count;
    }ppse_animation_read_data;
    
    
    
    
    typedef struct{
    	u32 signature;
    	u32 reserved[9];
    }ppse_model_editor_data_header;
    
    typedef struct{
    	u32 nameLength;
    	//reserved for more editing data
    	u32 reserved[10];
    }ppse_model_editor_data;
    
    // animation
    
    typedef struct{
		ppse_asset_header header;
    
    	u32 animation_count;
    
    	u16 total_keyframe_count;
    	u16 total_clip_count;
    	u16 total_uvs_count;
    	u16 total_sprite_frame_list_count;
    	u16 total_sprite_frame_count;
    
    	u32 offset_to_animations;
    	u32 offset_to_editor_data;
    
    	u32 reserved[9];
    }ppse_animation_file_header;
    
    typedef struct{
    	u32 uvs_count;
		u32 sprite_index;
		u16 total_frames_count;
		u16 _total_uvs_count;
    	u32 reserved[9];
    }ppse_animation_sprite_frame_list;
    
    typedef struct{
    
    	u16 keyframe_count;
		u16 frame_keyframe_count;
    
    	u32 frames_total;
    
    	b32 loop;
    	u16 frame_loop_start;
    	u16 frame_loop_end;
    
    	u32 frames_per_ms;
    
    	b32 frame_timer;
    	u32 frame_timer_repeat;
    
    	u32 offset_to_data;
    	u32 keep_timer_on_transition;
		b32 repeat;
    	u32 reserved[28];
    
    }ppse_model_animation;
    
    typedef struct{
    		u16 bone_index; // or bone
    		u16 mesh_index;
    
			u16 frame_start;
    		u32 frame_duration; //or total frames
    
    		u32 uvs_count;
    
    		model_animation_spline spline;
    
    		vec3 offset;
    
    		quaternion q;
    
    		f32 rotation_x;
    		f32 rotation_y;
    		f32 rotation_z;
    
    		u32 timer_frame;
    		u32 frame_repeat;
    
			union{
    		u16 frame_list_index;
			u16 frame_key;
			};
    		u16 flip_h;
    
			b16 switch_parent;
			u16 parent_index;
    		u32 reserved[9];
    
    }ppse_model_animation_keyframe;
    
    typedef struct{
    	vec2 uv0;
    	vec2 uv1;
    	vec2 uv2;
    	vec2 uv3;
    	u32 reserved[10];
    }ppse_animation_keyframe_uvs;
    
    typedef struct{
    
    	u32 keyframe_count;
    
    	u16 frameStart;
    	u16 frameDuration; //or total frames
    
        model_animation_spline spline;
    
    	u32 reserved[40];
    }ppse_model_animation_clip;
    
    // animation_
    //
    // File specific__
    //
    
    typedef struct{
    
        vec3 position;
    
    	u32 model_table_index;
    	//model model;
    	model_animations animation;
    
    	u32 orientation;
    	vec2 looking_direction;
    
    	//Physics data
    	vec3 velocity;
    	vec3 collision_size;
    	vec3 collision_offset;
    
    	u32 path_index;
    	u32 reserved[39];
    
    }ppse_entity;
    
    //for reading external files
    typedef struct{
    	u32 file_count;
    	u32 end_of_names_offset_from_header;
    
    	u32 reserved[10];
    }ppse_external_resources_header;
    
    typedef struct{
    	u32 signature;
    	u32 count;
    	u32 reserved[4];
    }ppse_editor_names_header;

    typedef struct{
		u32 signature;
		u32 length;
	}ppse_editor_name_chunk;
    
    typedef struct{
    	u32 signature;
    	u32 id;
    	u8 path_and_name[64];
    	u32 reserved[5];
    }ppse_composite_resource;

//for reading imported resources
    typedef struct ppse_editor_resources_header{
    	u32 signature;
    	u32 total_resources_count;
    	u32 free_headers_count;
    	u32 reserved[10];
    }ppse_editor_resources_header;

    typedef struct ppse_editor_resources_footer{
		u32 free_headers_count;
	}ppse_editor_resources_footer;
    
#if 1
    typedef struct{
		asset_type type;
    	u8 source[game_resource_path_and_name_MAX];
		u16 ugh;
    	u32 reserved[10];
    }ppse_editor_resource;
#endif


#pragma pack(pop)


inline platform_file_handle
ppse_create_pack_file(platform_api *platform,
		              u8 *file_name)
{
    platform_file_handle pack_file_handle= platform->f_open_file(file_name,
			                                             platform_file_op_write | platform_file_op_read | platform_file_op_share | platform_file_op_create);
	//create default header
	if(pack_file_handle.handle)
	{
        ppse_file_header pack_file_header = {0};
        pack_file_header.signature        = ppse_SIGNATURE; 
		pack_file_header.version          = ppse_version;
        pack_file_header.asset_count      = 0; 
		pack_file_header.offset_to_file_headers = sizeof(ppse_file_header);

		platform->f_write_to_file(pack_file_handle,
				                     0,
									 sizeof(ppse_file_header),
									 &pack_file_header);

	}
	return(pack_file_handle);
}

inline u32 
ppse_create_and_save_pack_file(platform_api *platform,
		                       u8 *file_name)
{
	u32 success = 0;
	platform_file_handle pack_file_handle = ppse_create_pack_file(platform,
			                                                      file_name);
	if(pack_file_handle.handle)
	{
		success = 1;
		platform->f_close_file(pack_file_handle);
	}
	return(success);
}

static inline asset_key
assets_zero_key()
{
	asset_key result = {0};
	return(result);
}

//typedef struct asset_image{
//	asset_key key;
//    render_texture *image;
//}asset_image;


//typedef struct{
//	asset_key key;
//    //model_animation *animation;
//}asset_model_animation;
//
//typedef struct{
//    //uint32 baseLineTop
//	asset_key key;
//    font_proportional *font;
//}asset_font;

typedef struct{
    platform_file_handle handle;
    ppse_file_header header;

}asset_file;

typedef struct{

	asset_type type;
	asset_file_info_flags flags;
	asset_source source;

	ppse_asset_data data;
    u8 path_and_name[256];
	//file_path_name_and_extension_info path_and_nameInfo;
}asset_file_info;

typedef struct{
	u32 id;
	
	u32 opened;
	u32 file_count;
	u32 file_array_index_start;
}ppse_directory;

typedef struct{
	asset_type type;
	u32 flags;

	u32 is_directory;
	u32 is_loaded;
	asset_key loaded_slot_key;

	u32 id;
	u8 path_and_name[256];
}ppse_resource_file;

typedef struct{
	asset_type type;
	u32 version_check_failed;
}ppse_file_check_result;

typedef struct ppse_resources_file_and_folders{
	
	u32 directory_count;
	u32 total_file_count;

	u8 resources_path[256];

	ppse_directory *directories;
	ppse_resource_file *files;
}ppse_resources_file_and_folders;

typedef struct{
	u32 nameLength;
	ppse_asset_data data;
	u8 path_and_name[256];
}asset_pack_header_info;

//world assets
typedef enum{
	player_state_idle,
	player_state_running,
	player_state_attacking,
	player_state_jumping,
	player_state_falling
}player_state;


typedef enum{
	shape_plane,
}world_collider_shape;

typedef struct{
	world_collider_shape shape;

	union{
		struct{
			f32 width;
			f32 height;
			f32 z;
		};

	//temp use for cube
	vec3 size;
	};


}world_collider;

typedef struct{
	vec3 offset;
	vec3 size;
}collision_cube;

typedef struct{
	u32 type;
	u32 default_state;
	f32 speed;
	f32 z_speed;
	f32 speed_max;
	u32 collision_count;
	collision_cube *collisions;
	vec3 collision_size;
	vec3 collision_offset;
	model *model;
	state_main states;
}asset_entity;

typedef struct{
	u32 stop_at_end;

	u16 current_frame;
	u16 frames_total;
	f32 timer_total;

    u16 frame_w;
	u16 frame_h;
	sprite_animation_frame *frames;
}asset_frame_animation;

typedef struct{
	u32 type;
	vec3 position;
	//indices to composite resources of the map
	u32 model_index;
	u32 entity_index;

	vec3 collision_size;
	vec3 collision_offset;
}map_entity_data;

typedef struct world_entity{

	coso_id id;

	struct world_entity *prev;
	struct world_entity *next;

	struct world_entity *parent;
	struct world_entity *first;
	struct world_entity *next_sibling;
	struct world_entity *prev_sibling;

	coso_flags flags;
	b16 remove_with_parent;
	b16 marked_for_remove;

	u32 detections_count;
    cosos_detection	*detections;

	//coso_id parent;
	//coso_id prev;
	//coso_id next;

	u16 type;
	u16 orientations;

	model_render_data model;
	//model model;
	model_animations animation;
	model_animation_timer animation_timer;

	//for states
	state_main *brain;

	vec2 looking_direction;
	vec3 relative_p;
	//sweep
	f32 distance;
	f32 start_angle;
	f32 end_angle;
	i32 side;

	game_body *body;

	union{
		u32 bools[2];
		struct{
			u8 allow_movement;
		};
	};

	f32 life_time;
	u32 hitpoints;

	attack_data dmg;
	u32 damage_contact_count;
	damage_contact damage_contact[10];

	struct entity_detection_data nearest_entity;
}world_entity;

//TODO(Agu): Whole world or region ?
#define world_MAX_QUADS_PER_ENTITY 100
typedef struct{
	coso_id *ids;
}map_cache;

typedef struct s_game_world{

	bool32 geometry_locked;
	u32 tileset_count;
	struct world_tileset **tilesets_a;
    //asset_image *tilesets;


	// ;Prepare for later entity process
	u32 entity_count;
	world_entity *entities;


	event_main events;

	u16 default_mark;
	u16 mark_count;
	map_mark *marks;

    vec3 offset;

	u32 w;
	u32 h;

	//Needed?
	u32 colliders_count;
	world_collider *colliders;
	//draw and collisions
    u32 tile_count;
    world_tile *tiles;

}game_world;

typedef struct asset_map_data{
    vec3 offset;

	u32 map_w;
	u32 map_h;

	u32 tileset_count;
	struct world_tileset **tilesets_a;
	u32 models_count;
	struct s_model **models;

    u32 tile_count;
    world_tile *tiles;

	u32 entity_count;
	map_entity_data *entities;

	u32 colliders_count;
	world_collider *colliders;

}asset_map_data;


struct asset_memory_chunk;
typedef struct asset_memory_chunk{
	u32 used;
	u32 asset_index;

	struct asset_memory_chunk *next_header;
	struct asset_memory_chunk *previous_header;
}asset_memory_header;

typedef struct asset_slot_header{

	u32 offsetToName;

	asset_source source;
    asset_type type;
    asset_state state;

	//for resource type
	u32 id;
	u32 version;

	u32 index;

	asset_memory_header *allocated_memory_header;
	struct asset_slot_header *next;
}asset_slot_header;

typedef struct asset_slot_data{

	union {
		font_proportional font;
		render_texture image;

		model model;
		model_animations animation;
		world_tileset tileset;
		asset_map_data map;
		asset_entity entity;
	};

	u32 offsetToName;

	asset_source source;
    asset_type type;
    asset_state state;

	//for resource type
	u32 id;
	u32 version;

	u32 index;

	asset_memory_header *allocated_memory_header;
	struct asset_slot_data *next;
}asset_slot_data;

//#define asset_slot_key(type)
typedef struct asset_slot_key{

	union{

		asset_key key;
		struct{
		u32 index;
		u32 id;
		};
	};

	union
	{
		font_proportional *font;
		render_texture *image;
		model *model;
		world_tileset *tileset;
		asset_map_data *map;
		void *data;
	};

}asset_slot_key;

typedef struct{
	u32 working_asset_index;
	memory_chunk chunk;
}asset_memory_operation;

typedef struct s_game_assets
{
    memory_area area;
	//memory_area operations_area;

	//memory_area assets_area;
    //memory_area restoredMemoryArea;
	platform_api *platform;
	render_texture_data *texture_operations;

	u32 current_free_slot;
	u32 using_package;
	
	/*memory for assets are stored in here. If a slot has memory associated with it,
	its header will be in some part at the start of asset_memory*/	
	u32 asset_memory_used;
	u32 asset_memory_max;
	u8 *asset_memory;

	asset_memory_header first_asset_memory_header;

    u32 totalSize;
    u16 asset_max;
    u16 asset_count;

    asset_file assetFile;
    asset_slot_data *assets;
	asset_slot_data *first_free_slot;

    ppse_raw_assets_header raw_assets_file_header;
	platform_file_handle raw_assets_file;


	memory_area resource_files_area;
	ppse_resources_file_and_folders *resource_files;


	asset_slot_data zero_asset;
	stream_data info_stream;

}game_assets;


typedef struct{
	u32 data_offset;
	ppse_asset_header *header;
	ppse_line line;

	u8 *entire_file;
}resource_reader;


#define resource_reader_get_struct(reader, type) (type *)resource_reader_get_size(reader, sizeof(type))
static void *
resource_reader_get_size(
		resource_reader *reader, u32 size)
{
	void *result = reader->entire_file + reader->data_offset;
	reader->data_offset += size;

	return(result);
}

static void 
resource_reader_read_line(
		resource_reader *reader)
{
	reader->line = *(ppse_line *)(reader->entire_file + reader->data_offset);
	reader->data_offset += sizeof(ppse_line);
}

//always assumes there is a line at the start
static resource_reader
resource_reader_begin(void *entire_file)
{
	resource_reader reader = {0};
	reader.entire_file = entire_file;
	reader.header = (ppse_asset_header *)entire_file;
//	resource_reader_read_line(&reader);
	return(reader);
}

static void
resource_reader_next_line(
		resource_reader *reader)
{
	if(!reader->line.next_line_offset)
	{
		resource_reader_read_line(reader);
	}
	else
	{
		ppse_line current_line = reader->line;
		reader->data_offset = current_line.next_line_offset;
		reader->line = *(ppse_line *)(reader->entire_file + reader->data_offset);
		reader->data_offset += sizeof(ppse_line);
	}
}

static b32 
resource_reader_get_to_line(
		resource_reader *reader,
		u32 line_number)
{
	//still haven't read the first line
	if(!reader->line.offset)
	{
		reader->data_offset = reader->header->offset_to_first_line;
		resource_reader_read_line(reader);
	}
	b32 success = 0;
	if(line_number < reader->line.number)
	{
		u32 line_at = reader->line.number;
		u32 prev_line_offset = reader->line.prev_line_offset;
		ppse_line *prev_line = 0;
		do
		{
			prev_line = (ppse_line *)(reader->entire_file + prev_line_offset);
			prev_line_offset = prev_line->prev_line_offset;
			line_at = prev_line->number;
		}while(line_at > line_number);

		reader->data_offset = prev_line->offset + sizeof(ppse_line);
		reader->line = *prev_line;
		success = 1;
	}
	else
	{
		u32 line_at = reader->line.number;
		u32 next_line_offset = reader->line.next_line_offset;
		ppse_line *next_line = 0;
		do
		{
			next_line = (ppse_line *)(reader->entire_file + next_line_offset);
			next_line_offset = next_line->next_line_offset;
			line_at = next_line->number;
		}while(line_at < line_number && !next_line->end);

		if(line_at == line_number)
		{
			success = 1;
			reader->data_offset = next_line->offset + sizeof(ppse_line);
			reader->line = *next_line;
		}
	}
	return(success);
}
