#define editor_TOP_BAR_H 30
#define MINIMUM_TILESIZE 1
////#define game_resource_path_and_name_MAX 126

#define editor_world_tileset_MAX 10
#define editor_model_sprite_sheets_MAX 10

typedef enum{
	s_game_editor_world,
	s_game_editor_model,
	s_game_editor_tileset,
	s_game_editor_coso,
	editor_mode_frame_animation,
}s_game_editor_mode;


#define eui_selection_selected(euis, target_index) (euis.selected && euis.index == target_index)
#define eui_selection_deselect(euis) euis.selected = 0
typedef struct{
	b16 selected;
	u16 index;
}eui_selection;

static void
eui_selection_select(eui_selection *euis, u32 index) 
{
	euis->selected = 1;
	euis->index = index;
}










typedef struct{
	platform_api *platform;
	platform_file_handle file;
	
	ppse_asset_header *header;
	b16 read;
	u8 line_count;
	u8 current_line_number;
	ppse_line *current_line;

	u32 data_offset;
	u32 file_size;


	u8 *contents;
	memory_area *area;
	resource_reader resource_reader;
	temporary_area temporary_area;
}editor_wr;

#define editor_array(type) editor_array 

typedef struct{
	u8 *base;
	u32 signature;
	u16 count;
	u16 max;
	u32 size_of_type;
}editor_array;

#define editor_array_base(ea, type) ((type *)ea.base)
static editor_array
editor_array_allocate(
		memory_area *area,
		u32 size_of_type,
		u32 max)
{
	editor_array array = {0};
	array.base = memory_area_push_size(area, size_of_type * max);
	array.size_of_type = size_of_type;
	array.max = max;
	return(array);
}

static editor_wr
editor_wr_begin_write(
		memory_area *area,
		platform_api *platform,
		platform_file_op file_op,
		u8 *path_and_name)
{
	editor_wr result = {0};
	result.read = 0;
	result.platform = platform;
	result.file = platform->f_open_file(path_and_name, platform_file_op_create_new);
	result.data_offset = 0;
	if(result.file.handle)
	{
		result.temporary_area = temporary_area_begin(area);
		result.area = area;
	}

	return(result);
}

static editor_wr
editor_wr_begin_read(
		memory_area *area,
		platform_api *platform,
		u8 *path_and_name)
{
	editor_wr result = {0};
	result.read = 1;
	result.platform = platform;
	result.file = platform->f_open_file(path_and_name, platform_file_op_read);
	result.data_offset = 0;
	if(result.file.handle)
	{
		result.temporary_area = temporary_area_begin(area);
		result.area = area;
		platform_entire_file entire_file = platform_read_entire_file_handle(platform, result.file, area);
		result.contents = entire_file.contents;
		result.resource_reader = resource_reader_begin(result.contents);
		result.header = (ppse_asset_header *)result.contents;

		platform->f_close_file(result.file);
	}

	return(result);
}


static void
editor_wr_end_line(
		editor_wr *wr)
{
	if(wr->current_line)
	{
		wr->current_line->next_line_offset = wr->data_offset;
	}
}

static u32 
editor_wr_put_line(
		editor_wr *wr)
{
	ppse_line *prev_line = wr->current_line;
	editor_wr_end_line(wr);
	//set first line offset for future reads
	if(!wr->line_count)
	{
		wr->header->offset_to_first_line = wr->data_offset;
	}

	ppse_line *new_line = memory_area_clear_and_push_aligned(wr->area, sizeof(ppse_line), 0);
	new_line->prev_line_offset = prev_line ? prev_line->offset : 0;
	wr->current_line = new_line;
	wr->current_line->offset = wr->data_offset;
	wr->current_line->number = wr->line_count;
	wr->current_line_number = wr->line_count++;
	wr->data_offset += sizeof(ppse_line);

	return(wr->current_line->number);
}

#define editor_wr_write_struct(wr, type, memory) (type *)editor_wr_write_size(wr, sizeof(type), memory)
static void *
editor_wr_write_size(
		editor_wr *wr, u32 size, void *memory)
{
	void *memory_at = memory_area_push_size_aligned(wr->area, size, 0);
	memory_copy(memory, memory_at, size);
	wr->data_offset += size;
	return(memory_at);
}


#define editor_wr_put_struct(wr, type) (type *)editor_wr_put_size(wr, sizeof(type))
static void *
editor_wr_put_size(
		editor_wr *wr, u32 size)
{
	void *memory_at = memory_area_clear_and_push_aligned(wr->area, size, 0);
	wr->data_offset += size;
	return(memory_at);
}

static u8 *
editor_wr_put_string(editor_wr *wr, u8 *string)
{
	u32 count = string_count(string);
    u8 *result = editor_wr_write_size(wr, count, string);
	return(result);
}

#define editor_wr_put_header(wr, header_type) (header_type *)_editor_wr_put_header(wr, sizeof(header_type))
static void *
_editor_wr_put_header(
		editor_wr *wr, u32 asset_header_size)
{
	void *header = editor_wr_put_size(wr, asset_header_size);
	wr->header = header;
	return(header);
}

#define editor_wr_read_struct(wr, type) (type *)editor_wr_read_size(wr, sizeof(type))
static void *
editor_wr_read_size(
		editor_wr *wr, u32 size)
{
	void *result = resource_reader_get_size(&wr->resource_reader, size);
	wr->data_offset = wr->resource_reader.data_offset;
	return(result);
}

static b32
editor_wr_read_to_line(
		editor_wr *wr, u32 line_number)
{
	b32 success = resource_reader_get_to_line(&wr->resource_reader, line_number);
	wr->current_line_number = (u8)wr->resource_reader.line.number;
	wr->data_offset = wr->resource_reader.data_offset;
	return(success);
}

static void
editor_wr_read_next_line(
		editor_wr *wr)
{
	resource_reader_next_line(&wr->resource_reader);
	wr->current_line_number = (u8)wr->resource_reader.line.number;
	wr->data_offset = wr->resource_reader.data_offset;
}

static inline void 
editor_wr_write_composite_resource_header(
		editor_wr *wr,
		u8 *path_and_name)
{
	ppse_composite_resource cr_name = {0};
	//generate id
	cr_name.signature = ppse_composite_resource_SIGNATURE;
	cr_name.id = assets_generate_id(path_and_name);
	//copy path and name
	string_copy(path_and_name,
			cr_name.path_and_name);
	editor_wr_write_size(
			wr, sizeof(cr_name), &cr_name);
}

static void
editor_wr_end(
		editor_wr *wr)
{
	if(wr->file.handle && !wr->read)
	{
		editor_wr_end_line(wr);
		wr->current_line->end = 1;

		//write entire file
		u8 *entire_file_memory = wr->area->base + wr->temporary_area.area_start;
		wr->platform->f_write_to_file(wr->file, 0, wr->data_offset, entire_file_memory);
		wr->platform->f_close_file(wr->file);
	}
	wr->platform = 0;
	wr->area = 0;
	temporary_area_end(&wr->temporary_area);
}



typedef struct editor_hash_index{
	u32 key;
	u16 index;
	u16 order;
	union{
	void *element;
	};
	struct editor_hash_index *next;
}editor_hash_index;

typedef struct{
	u16 count;
	u16 max;
	editor_hash_index *indices;
}editor_hash;



typedef enum{
	//editor_cursor_mode_MoveCamera,
	editor_cursor_tiles,
	editor_cursor_collisions,
	editor_cursor_entities,

	editor_cursor_mode_Total
}editor_cursor_mode;

typedef enum{
	world_mode_normal,
	world_mode_selection,
	world_mode_entities,
	world_mode_shift,
	world_mode_resize,
	world_mode_harea,
	world_mode_view,
}editor_world_mode;

typedef enum{
	world_tool_paint,
	world_tool_selection,
	world_tool_bucket,
	world_tool_grid,
}editor_world_tool;

typedef enum{
	model_mode_view,
	model_mode_paint,
	model_mode_selection,
	model_mode_nodes,
	model_mode_sprite_editing,
	model_mode_properties,
	model_mode_animation,

	model_tool_max
}editor_model_tool;

typedef enum{
	editor_world_graphics,
	editor_world_entities
}editor_world_tab;

typedef enum{
	graphics_tab_model,
	graphics_tab_animation,

	graphics_tab_total,
}editor_graphics_tab;

typedef enum{
	asset_error_None,
	asset_error_NotCompatible,
	asset_error_NotFound,
}asset_loading_error;

typedef enum{
	world_cursor_z_axis,
	world_cursor_x_axis,
	world_cursor_y_axis,
}editor_world_cursor_axis;

typedef enum{
	tile_edge_v0 = 0x1,
	tile_edge_v1 = 0x2,
	tile_edge_v2 = 0x4,
	tile_edge_v3 = 0x8,

	//tile_edge_u = 3, //v0 and v1
	//tile_edge_d = 12, //v2 and v3
	//tile_edge_l = 5, //v0 and v2
	//tile_edge_r = 10 //v1 and v3

	tile_edge_u = 0x10, //v0 and v1
	tile_edge_d = 0x20, //v2 and v3
	tile_edge_l = 0x40, //v0 and v2
	tile_edge_r = 0x80 //v1 and v3
}editor_mesh_edge;

typedef enum{
	tile_uv_u,
	tile_uv_d,
	tile_uv_r,
	tile_uv_l,
}editor_tile_uv_orientation;

typedef enum{
	quad_orientation_u,
	quad_orientation_d,
	quad_orientation_l,
	quad_orientation_r,
}quad_orientation;

typedef enum{
	editor_world_camera_free,
	editor_world_camera_orbit_cursor,
	editor_world_camera_padding,

	editor_world_camera_mode_total
}editor_world_camera_mode;

#define editor_TILE_CLIPBOARD_MAX 5000
#define editor_SPRITE_CLIPBOARD_MAX 1000
#define editor_GRID_CAPACITY_TOLERANCE 1024
#define editor_BONE_NAME_MAX 64
#define editor_SPRITE_NAME_MAX 256
#define editor_ANIMATION_NAME_LENGTH 64

//Node: this is an idea for fixing indices on the editor side of things
//and the process that could lead to its use. It looks like a mess, but
//most of it can be simplified by using functions
#if 0
add_hash_index(u8 *string, u32 index)
{
	u32 key = string_kinda_hash(string);
	editor_hash_index *hash_index = get_hash_index_by_key(key);
	if(!hash_index)
	{
		hash_index = get_hash_index_slot(key);
		hash_index->key = key;
		hast_index->index = index;
	}
	
}
static void
editor_ui_name_chunk_input(
		editor_name_chunks *name_chunks,
		editor_hash_index *hash_indices)
{
	editor_name_chunks name_chunk = name_chunks->chunks[b];
	u8 name_buffer[256] = {0};
	string_copy(name_chunks.name, name_buffer);
	if(ui_text_input(ui, name_chunks->lenght - 1, &name_buffer))
	{
		editor_hash_index *existing_hash = get_hash_index(hash_indices, name_buffer);
		if(!existing_hash)
		{
			//modifies existing hash
			existing_hash = get_hash_index(hash_indices, name_chunk.name);
			existing_hash->key = generate_hash_key(name_buffer);
			//modifies name chunk
			string_copy(name_buffer, name_chunk.name);
		}
	}
}

static void
make_model_bone()
{
	for(u32 b = 0; b < model_editor->bone_count; b++)
	{
		editor_bone *editor_bone = model_editor->bones + b;
		model_bone *bone = model->bones + b;
		editor_hash_index *bone_index = get_hash_index(
				model_editor->bone_indices, model_editor->bone_names[editor_bone->parent_name_index]);
		bone->parent = bone_index->index;
	}
}
#endif
//undo/redo steps
#if 0
static void
editor_model_remove_frame_list(
		s_game_editor *game_editor,
		u32 index)
{
	s_model_editor *model_editor = &game_editor->model;
	if(index < model_editor->frame_list_count)
	{
		editor_model_add_to_history(game_editor, model_editor_removed_frame_list);
		editor_ur_removed_frame_list *ur_chunk = editor_model_add_do(game_editor, model_editor_removed_frame_list);
		ur_chunk->removed_index = index;

		editor_model_frame_list *frame_list = model_editor->frame_lists + index;
		//push frame list struct
		editor_model_ur_push_struct(ur_chunk->base, editor_model_frame_list, frame_list);
		editor_model_ur_push_size(ur_chunk->base, frame_list->mesh_frames->base, frame_list->mesh_frames->used);

		memory_dyarray_delete(frame_list->mesh_frames);
		editor_name_chunks_remove(
				&model_editor->frame_list_names, index);
		//shift array
		for(i32 f = index; f < model_editor->frame_list_count - 1; f++)
		{
			model_editor->frame_lists[f] = model_editor->frame_lists[f + 1];
		}
		model_editor->frame_list_count--;

		//make sure all keyframes point to the correct bone for every animation.
		if(model_editor->frame_list_count)
		{
			for(u32 a = 0; a < model_editor->animation_count; a++)
			{
				editor_animation *editor_animation = model_editor->animations + a;
				//go through rows
				for(editor_animation_keyframe_row *keyframe_row = editor_animation->first_frame_row;
						keyframe_row; keyframe_row = keyframe_row->next)
				{
					editor_model_ur_push_struct(ur_chunk->base, editor_animation_keyframe_row, keyframe_row);
					//go through all keyframes
					for(editor_animation_keyframe *keyframe = keyframe_row->first_keyframe;
							keyframe; keyframe = keyframe->next_in_row)
					{
						editor_model_ur_push_struct(ur_chunk->base, editor_animation_keyframe, keyframe);
						if(keyframe->base.frame_list_index >= index)
						{
							keyframe->base.frame_list_index--;
							keyframe->base.frame_list_frame_index = 0;
						}
					}
				}
			}
		}
	}
}

//this should depend on the current mode
static void
editor_model_undo()
{
	editor_ur *ur = model_editor->last_undo;
	switch(ur->type)
	{
		case mode_ur_remove_frame_list:
			{
				editor_ur_removed_frame_list *ur_chunk = (editor_ur_removed_frame_list *)ur;
				editor_model_frame_list *frame_list = editor_ur_consume_struct(ur_chunk->base, editor_model_frame_list);
				//get the frame list count
				//consume size and retrieve the frame list count
				//get the frame list at the specified index, allocate one if needed
				//copy all contents from the allocated frame list to the other.
				
			}break;
	}
}

#endif
//steps
//-allocate hash indices on a linear array
//-if deleting an element and the index is bigger than the deleted one, decrease by one
//-When trying to get a pointing index, use the id to look for it on the hash_index list

typedef struct{

    u16 count;
	u16 max;
	u32 length;

	struct name_chunk{
	          u8 *name;
	}*chunks;

}editor_name_chunks;

typedef struct name_chunk_link{
	struct name_chunk *chunk;
	struct name_chunk_link *next_free;
}name_chunk_link;

typedef struct{
	editor_name_chunks names;
	struct name_chunk *first_free;
}editor_name_chunks_link;

typedef struct{
	u32 type;
	u32 index;
	u8 v0_selected;
	u8 v1_selected;
	u8 v2_selected;
	u8 v3_selected;

}editor_mesh_selection;

typedef struct{
	//index to the editor_mesh_selection array
	u16 selection_index;
	u16 mesh_index;
	f32 distance_to_ray;

	u32 selection_value;
}editor_hot_vertices_info;

typedef enum{
	world_history_tile_added,
	world_history_tile_removed,
	world_history_moved_selections,
	world_history_paste,
	world_history_cut,
}editor_world_history_record_data_type;

struct editor_world_history_record_data;
typedef struct editor_world_history_record_data{

	editor_world_history_record_data_type type;

	u32 map_terrain_count;
	u32 tile_data_count;

	union{
		struct{
			u32 tile_index;
	        world_tile target_tile;
		};
		struct{
			u32 added_tile_count;
	        world_tile added_tile;
		};
		struct{
			u16 move_gizmo;
			u16 ignore_tile;

			u32 ignored_tile;
			u32 map_terrain_count;
			vec3 delta_moved;
		}moved_selections;

		struct{
			u32 map_terrain_count;
		}pasted_tiles;
		struct{
			u32 map_terrain_count;
		}cut_tiles;
	};
	editor_mesh_selection target_tile_data;

}editor_world_history_record_data;

typedef struct{
	editor_mesh_selection selection;
	world_tile data;
}editor_world_tile_selection_and_data;

typedef struct{
	editor_world_history_record_data *record;
}editor_world_history_header;

typedef struct{
	u32 mesh_orientation;
	model_mesh mesh;
	vec3 mesh_uAxis;
	vec3 mesh_rAxis;

	u32 stick_to_near_mesh;
    u32 hovering_edge;	
	u16 hitted_mesh;
	u16 hitted_mesh_index;


	u32 mode;
}editor_cursor_paint;

typedef struct{
    model_vertices *vertices;
	model_uvs *uvs;

}editor_mesh;

typedef struct{

	u16 selected_meshes_count;
	u16 selected_meshes_max;
	editor_mesh_selection *selected_meshes;

	u16 hot_vertices_count;
	u16 hot_vertices_max;
	editor_hot_vertices_info *hot_vertices;

	u16 meshes_count;
	u16 meshes_max;
	editor_mesh *meshes;

}editor_cursor_memory;

typedef struct{
	f32 move_down_timer;
	u32 vertex_to_move_with_gizmo;

	f32 tile_displacement;
	vec3 position;
	vec3 uAxis;
	vec3 rAxis;

	u32 mesh_orientation;
	model_mesh mesh;
	vec3 mesh_uAxis;
	vec3 mesh_rAxis;

	u32 gizmo_cube_size;

	vec3 gizmo_position;
	vec3 gizmo_position_last;

}editor_cursor;

typedef struct{
	vec3 p;
	u32 hot;
	u32 interacting;

	u32 apply_rotation_x;
	u32 apply_rotation_y;
	u32 apply_rotation_z;

}editor_cursor_gizmo;

typedef struct{
	memory_dyarray(model_uvs, uvs);
}editor_animation_sprite_frame;

typedef struct{
	u32 sprite_index;
	u32 uvs_count;
	u32 total_frames_count;

	u32 editor_selected_frame;
//	model_mesh_frame_list *base;
	memory_dyarray(sprite_orientation, mesh_frames);
}editor_model_frame_list;

//typedef struct editor_animation_keyframe_group{
//	union{
//		u32 bone_index;
//		u32 sprite_index;
//	};
//	model_animation_keyframe_type type;
//	u32 keyframe_count;
//	struct editor_animation_keyframe_group *next;
//	struct editor_animation_keyframe_group *prev;
//	memory_dyarray(model_animation_keyframe) *keyframes;
//
//	struct editor_animation_keyframe_group *next_in_column;
//	struct editor_animation_keyframe_group *prev_in_column;
//}editor_animation_keyframe_group;

//typedef struct editor_animation_timeline_column{
//	struct editor_animation_timeline_column *next;
//	struct editor_animation_timeline_column *prev;
//
//	u32 frame;
//	editor_animation_keyframe_group *first_group;
//}editor_animation_timeline_column;

typedef struct editor_animation_keyframe{
	struct editor_animation_keyframe_row *row;
	struct editor_animation_keyframe_column *column;
	union{
			struct editor_animation_keyframe *next_free;
		struct{
			struct editor_animation_keyframe *next_in_row;
			struct editor_animation_keyframe *prev_in_row;
			struct editor_animation_keyframe *next_in_column;
			struct editor_animation_keyframe *prev_in_column;
		};
	};
	model_animation_keyframe base;
}editor_animation_keyframe;

typedef struct editor_animation_keyframe_row{
	struct editor_animation_keyframe_row *prev;
	struct editor_animation_keyframe_row *next;
	u16 bone_index;
	model_animation_keyframe_type type;
	editor_animation_keyframe *first_keyframe;
}editor_animation_keyframe_row;

typedef struct editor_animation_keyframe_column{
	//other column data
	struct editor_animation_keyframe_column *prev;
	struct editor_animation_keyframe_column *next;
	u32 frame;
	editor_animation_keyframe *first_keyframe;
}editor_animation_keyframe_column;


typedef struct{
	model_animation base;
	//editor_animation_keyframe_group *first_keyframe_group;
	//editor_animation_keyframe_group *first_frame_keyframe_group;

	editor_animation_keyframe_column *first_column;
	union{
		editor_animation_keyframe_row *first_rows[2];
		struct{
			editor_animation_keyframe_row *first_row;
			editor_animation_keyframe_row *first_frame_row;
		};
	};
}editor_animation;

typedef struct{
	s_tileset_terrain base;
	memory_dyarray(model_mesh, meshes);
}editor_tileset_terrain;

typedef struct{
	tileset_wall base;
	memory_dyarray(model_mesh, uvs);
}editor_tileset_wall;

typedef struct{
	u32 editor_selected_layer;
	tileset_autoterrain base;
	memory_dyarray(u16, indices);
}editor_tileset_autoterrain;











typedef struct editor_resource_slot{
	struct game_resource_attributes *r;
	u32 local_reference_count;

	struct editor_resource_slot *next;
	struct editor_resource_slot *prev;
}editor_resource_slot;

typedef struct{

	u16 count;
	u16 max;
	editor_resource_slot *array;
	editor_resource_slot *first;
	editor_resource_slot *first_free;
}editor_resource_slots;

static editor_resource_slot *
editor_resource_slot_ref(
		editor_resource_slots *slots_main, struct game_resource_attributes *r)
{
	editor_resource_slot *result = 0;
	for(editor_resource_slot *slot = slots_main->first;
			slot; slot = slot->next)
	{
		if(slot->r == r)
		{
			result = slot;
			break;
		}
	}
	if(!result)
	{
		result = slots_main->first_free;
		if(!result)
		{
			result = slots_main->array + slots_main->count++;
		}
		else
		{
			slots_main->first_free = result->next;
		}

		if(slots_main->first)
		{
			slots_main->first->prev = result;
		}
		result->next = slots_main->first;
		slots_main->first = result;
	}
	result->local_reference_count++;
}

static void
editor_resource_slot_deref(
		editor_resource_slots *slots_main,
		editor_resource_slot *slot)
{
	slot->local_reference_count--;
	if(!slot->local_reference_count)
	{
	     if(slot->prev) slot->prev->next = slot->next;
	     if(slot->next) slot->next->prev = slot->prev;

		 slot->r = 0;
		 slot->next = slots_main->first_free;
		 slots_main->first_free = slot;
	}
}



typedef struct game_resource_memory_chunk{
	u32 used;
	struct game_resource_memory_chunk *next;
	struct game_resource_memory_chunk *previous;
}game_resource_memory_chunk;


//typedef struct{
//}game_composite_resource_memory_header;
typedef struct{
	asset_type type;
	b32 needs_composite_resources;
	u32 signature;
	u32 signature_encoded;

	u32 additional_formats_count;
	u32 additional_formats[2];
}game_resource_info;

typedef struct game_composite_resource{
	u8 path_and_name[game_resource_path_and_name_MAX];
	b16 invalid;
	b16 not_found;
	struct game_resource_attributes *attributes;

	struct game_composite_resource *first;
	struct game_composite_resource *last;

	struct game_composite_resource *next_slots;
	struct game_composite_resource *prev_slots;
}game_composite_resource;


typedef struct game_resource_attributes{
	asset_slot_data *asset_key;
	asset_type type;
	//you can only free this resource if nobody is referencing it.
	u32 reference_count;
	//asset information, filled even if not loaded.
	u32 version;
	//depends on path_and_name
	u32 id;

	//inside imported_resources file.
	u32 index_in_file;

	b16 imported_for_game;
	u16 composite_resource_count;
	struct game_composite_resource *composite_resources;

	b32 load_success;
	platform_file_handle file_handle;
	resource_check_result error_flags;
	platform_file_time write_time;
	u8 name[32];
	u8 file_type[5];
	u8 path[64];
    u8 path_and_name[game_resource_path_and_name_MAX];
	//per type specific data
	union{
		struct{
			struct game_composite_resource *texture;
		}tileset;
		struct{
			u32 texture_count;
			struct game_composite_resource *textures;
		}model;
		struct{
			u32 tileset_count;
			struct game_composite_resource *tilesets;
			u32 model_count;
			struct game_composite_resource *models;
		}map;
		struct{
			struct game_composite_resource *model;
		}coso;
	};

	union{

		struct game_resource_attributes *next_free_resource;
		struct game_resource_attributes *next;
	};
	struct game_resource_attributes *prev;

	u32 directory_id;
	struct game_resource_attributes *next_in_directory;
	struct game_resource_attributes *prev_in_directory;

	struct game_resource_attributes *next_of_type;
	struct game_resource_attributes *prev_of_type;
}game_resource_attributes;

typedef struct{
	game_resource_attributes *r;
	u32 reference_count;
}game_resource_handle;

typedef struct game_resources_directory{
	u32 id;
	u32 parent_id;
	u8 name[32];
	u8 path[64];
	u8 path_and_name[128];

	struct game_resources_directory *first;
	struct game_resources_directory *last;
	struct game_resources_directory *next;
	struct game_resources_directory *parent;
	struct game_resources_directory *child_in_hierarchy;

//	struct game_resources_directory *next;
//	struct game_resources_directory *first;

	game_resource_attributes *first_resource;
	game_resource_attributes *last_resource;
}game_resources_directory;



//Editors
//frame animation editor
typedef struct{
	sprite_animation base;
	u32 frame_count;
	memory_dyarray(sprite_animation_frame, frames);
}editor_sprite_animation;

typedef struct s_frame_animation_editor{
	game_resource_attributes *editing_frame_animation;
	game_resource_attributes *external_image;
	u32 tab;

	memory_dyarray_area *dyarrays_area;

	struct franim_reset_memory{
		b16 frame_index_is_selected;
		u16 selected_frame_index;

		b16 animation_is_selected;
		u16 selected_animation_index;

		u16 animation_count;
	};

	//constants
	u16 animation_max;
	editor_sprite_animation *animations;


	editor_name_chunks animation_names;
}s_frame_animation_editor;

typedef struct{
	u32 lines_count;
	memory_dyarray(state_action_line, action_lines);
}editor_state_action;


typedef struct{
	state_line base;
	u8 *state_name;
	u8 *action_name;
	memory_dyarray(state_trigger, conditions);
}editor_state_line;

typedef struct editor_state{
	u32 lines_count;
	memory_dyarray(editor_state_line, lines);
	struct editor_state *next_free;
}editor_state_node;

typedef struct{
	collision_cube base;
}editor_collision;

typedef enum{
	coso_base,
	coso_collisions,
	coso_states,
	coso_state_actions,

	coso_editor_mode_COUNT
}coso_editor_mode;

typedef struct{
	u32 value;
	u8 name[64];
}e_line;

typedef struct{
	b16 selected;
	u16 index;
}e_select;

typedef struct s_coso_editor{

	struct coso_editor_reset_data{
		game_resource_attributes *editing_entity;
		game_resource_attributes *editing_coso_model;

		u8 *coso_default_state;
		u16 state_count;
		u16 actions_count;
		u16 state_lines_count;
		b16 renaming_state;
		u16 line_conditions_count;
		u16 collision_count;

		b16 state_is_selected;
		u16 selected_state_index;
		b8 state_line_is_selected;
		b8 state_line_new_is_selected;
		u16 selected_state_line_index;

		b16 renaming_collision;

		u16 state_line_selected_condition_index;
		b8 state_line_condition_is_selected;
		b8 state_line_condition_new_is_selected;

		b16 collision_is_selected;
		u16 selected_collision_index;

		struct{
			f32 speed;
			f32 speed_max;
			f32 z_speed;
		}stats;
		vec3 collision_size;
		vec3 collision_offset;

		editor_state_node *first_free_state;
	};

	u16 condition_values_count;
	e_line *condition_values;

	u32 mode;
	memory_dyarray_area *dyarrays_area;

	memory_area per_frame_area;
	world_entity simulating_entity;
	game_body body;

	u16 state_max;
	editor_state_node *states;

	u16 state_mode_ui_focus;

	u16 actions_max;
	editor_state_action *actions;

	u8 name_change_buffer[126];
	b16 state_name_avadible;

	u16 collisions_max;


	u16 state_lines_max;
	state_line *state_lines;

	u16 line_conditions_max;
	state_trigger *line_conditions;

	editor_hash states_hash;
	editor_name_chunks state_names;
	editor_hash action_hash;
	editor_name_chunks action_names;

	editor_collision *collisions;
	editor_hash collisions_hash;
	editor_name_chunks collision_names;

	game_render_parameters render_parameters;
}s_coso_editor;

typedef struct{
	entity_type type;
	vec3 position;
	u32 model_index;
	u32 entity_index;

	game_resource_attributes *model;
}editor_map_entity;

typedef struct{
	u32 local_ref_count;
	game_resource_attributes *r;
}editor_harea_model_slot;

typedef struct{
//	game_resource_attributes *model;
	u16 model_index;
	//editor_resource_slot *model;
	u16 weight;
	f32 radius;
}editor_map_harea_model;

typedef struct{
	vec2 p;
	u32 model_index;
}editor_map_harea_point;

typedef struct{
	world_harea base;
	memory_dyarray(world_harea_tile, tiles);
	memory_dyarray(editor_map_harea_point, points);
	b16 map_tiles;
	u16 model_count;
	u16 point_count;
	u16 external_model_count;
	b32 generate_poisson;
	memory_dyarray(editor_harea_model_slot, external_models);
	union{
		memory_dyarray(sprite_animation, animations);
		memory_dyarray(editor_map_harea_model, models);
	};
}editor_map_harea;

#if 1
#endif



typedef struct s_world_editor{
	editor_world_tab tab;
	editor_world_tool tool;
	editor_cursor_mode cursor_mode;
	union{
		struct ew_rdt{
			eui_selection harea_model_selection;
			b32 generate_harea_poisson;

			u16 map_terrain_count;
			u16 tilesets_count;
			u32 history_cursor;
			u16 history_count;
			u32 history_buffer_used;
			u16 models_count;
			b32 draw_locked;

			u16 entity_count;

			b16 entity_got_selected;
			b32 entity_copied;

			game_resource_attributes *editing_map;
		};
		struct ew_rdt ew_reset_data;
	};

	editor_resource_slot harea_model_slots;

	memory_dyarray_area *dyarrays_area;

	editor_name_chunks entity_tags;
	editor_name_chunks harea_tags;

	u32 harea_to_add_type;
	b8 harea_grid_edit;
	b8 add_harea;
	b16 open_add_context_menu;

	u16 tilesets_max;
	game_resource_attributes **editing_map_tilesets;
	u16 models_max;
	game_resource_attributes **models; 
	u16 total_models_count;

	u16 next_tile_animated;
	u16 next_tile_animation_index;
	editor_cursor cursor;

	editor_cursor_memory cursor_memory;

	memory_area per_frame_area;

	vec3 reserved_vec;
	vec3 mouse_holding_position;
	vec3 mouse_holding_delta_last;

	u32 in_camera_mode;

	u32 selected_tileset_for_painting_index;
	u32 selected_meshes_tileset_id;

//	u16 hareas_max;
//	u16 hareas_count;
	b16 harea_is_hot;
	u16 harea_hot_index;
	b16 harea_is_selected;
	u16 selected_harea_index;
	editor_array(editor_map_harea) hareas;



	u16 movingSelectedVertex;
	u16 moving_selections;

	vec3 recorded_rotation_point;
	vec3 rotation_point;

	//painting
	u16 stick_to_near_tile;
	u16 repaint_tile;
	//to not be choose by the camera. it should be optional
	editor_mesh_edge edge_hot;
	editor_mesh_edge edge_selected;

	//f32 rotation_apply;
	u32 apply_rotation;
	i32 rotation_apply_x;
	i32 rotation_apply_y;
	i32 rotation_apply_z;

	u16 next_tile_uv_flip_x;

	u16 frame_tile_size_x;
	u16 frame_tile_size_y;

	u32 move_reference_grid;

	u32 camera_mode;
	f32 camera_distance_from_cursor;

	f32 camera_rotation_x;
	f32 camera_rotation_z;
	vec3 camera_pad_position;
	vec3 camera_position;

	memory_area history_area;

	u32 history_buffer_total;
	u8 *history_buffer;

	u16 history_max;
	editor_world_history_header *history;

	u32 tile_clipboard_count;
	world_tile *tile_clipboard;

	u16 map_terrain_max;
	world_tile *terrain;


	editor_map_entity yanked_entity;
	u16 selected_entity_index;
	b16 ray_hits_entity;
	u16 ray_entity_index;
	vec3 ray_tile_position;
	vec3 cached_ray_position;

	u16 entity_max;
	editor_map_entity *entities;

	i16 selection_amount_x;
	i16 selection_amount_y;
	editor_world_mode mode;
	u32 selection_tile_from_index;
	b32 selection_started;

	u16 selected_tileset_index;
	b16 next_tile_autoterrain;
	u16 selected_tileset_terrain_index;
	u16 selected_autoterrain_index;

	b16 ray_hits_tile;
	u16 ray_tile_index;

	u16 new_map_w;
	u16 new_map_h;
	u16 map_w;
	u16 map_h;
	game_world *loaded_world;
	b32 resize_after_shift;
	i16 shift_x;
	i16 shift_y;

}s_world_editor;

struct s_tileset_editor{

	game_resource_attributes *tileset_texture;
	game_resource_attributes *editing_tileset;

	u32 selected_tileset_tile_index;
	world_tileset loaded_tileset;

	u16 selected_tileset_animation_index;
	u16 selected_animation_frame_index;


	editor_name_chunks terrain_names;
	editor_name_chunks wall_names;
	editor_name_chunks autoterrain_names;

	memory_dyarray_area(model_mesh) *terrain_meshes_zone;
//	memory_dyarray_area(u16) *autoterrain_indices;

	u16 autoterrain_max;
	u16 autoterrain_count;
	editor_tileset_autoterrain *autoterrains;

	u16 terrain_count;
	u16 terrain_max;
	u32 selected_terrain_index;
	u32 selected_terrain_uvs_index;
	bool32 selecting_terrain_uvs;
	editor_tileset_terrain *terrain;

	u16 wall_count;
	u16 wall_max;
	editor_tileset_wall *walls;
	editor_tileset_wall *selected_wall;
	u16 selected_wall_index;
	b16 wall_is_selected;
	u16 selected_wall_uvs_index;

	//u16 autoterrain_indices_max;
	//u16 autoterrain_indices_count;
	//u16 *autoterrain_indices;

	u16 selected_autoterrain_index;
	u16 sel_autoterrain_mask_value;

	f32 image_selection_zoom;
	b8 image_selection_down;
	b8 image_selection_hot;

	u32 adding_tile_amount;
	u32 adding_tile_frame_x;
	u32 adding_tile_frame_y;
	vec2 adding_tile_uv0;
	vec2 adding_tile_uv1;
	vec2 adding_tile_uv2;
	vec2 adding_tile_uv3;

}s_tileset_editor;

typedef struct{
	b32 is_directory;
	union{
		game_resource_attributes *resource;
		game_resources_directory *directory;
	};
}resource_explorer_file;

#define er_explorer_load (er_explorer_close_on_complete | er_explorer_copy_selected_file_name | er_explorer_select_file)
#define er_explorer_save (er_explorer_close_on_complete | er_explorer_copy_selected_file_name)
typedef enum{
	er_explorer_close_on_complete = 0x01,
	er_explorer_copy_selected_file_name = 0x02,
	er_explorer_select_file = 0x04,
	er_explorer_reestrict_to_type = 0x08,
//	er_explorer_choose_multiple_files = 0x02
}er_explorer_flags;

typedef struct s_game_resource_editor{

	game_resource_info resources_info[asset_type_COUNT];
	u8 resource_formats[asset_type_COUNT][5];

	memory_area content_area;
	struct ppse_editor_resources_header file_header;
	platform_file_handle imported_resources_file;
	b16 auto_check_files;
	b16 file_check_time;

	//for editors asking for resources
	u16 r_handle_count;
	u16 r_handle_max;
	game_resource_handle *r_handles;

	u32 resources_loaded_count;
	u16 resources_count;
	u16 resources_max;
	game_resource_attributes *resources;
	game_resource_attributes *first_free_resource;
	game_resource_attributes *first;
	game_resource_attributes *last;
	//based on types

	u16 composite_resources_count;
	u16 composite_resources_max;
	game_composite_resource *composite_resources;
	game_composite_resource *first_free_composite_resource;

	u16 directories_count;
	u16 directories_max;
	game_resources_directory *directories;
	game_resources_directory *root_dir;
	game_resources_directory *explorer_current_dir;

	game_resource_attributes *browser_selected_resource;
	//for renaming files
	b32 renaming_resource;
	u8 rename_buffer[32];
	//logging
	u32 log_buffer_used;
	u32 log_buffer_max;
	u8 *log_buffer;
	memory_area log_stream_area;
	stream_data log_stream;

	//explorer for selecting resources
	u8 explorer_process_name[64];
	u8 explorer_selected_resource_name[256];
	b8 explorer_process_completed;
	b8 explorer_valid_file_focused;
	u32 explorer_process_id;
	u16 explorer_selected_resource_index;
	b32 resource_explorer_closed;
	game_resource_attributes *explorer_selected_resource;

	b8 update_path;
	u8 explorer_current_path[256];
	asset_type explorer_reestricted_type;
	u16 explorer_file_count;
	u16 explorer_file_max;
	resource_explorer_file *explorer_files;
	b8 explorer_updated_path;
	b8 explorer_file_is_selected;
	u16 explorer_selected_file_index;

	er_explorer_flags explorer_flags;
	u8 explorer_process_input[game_resource_path_and_name_MAX];
	u8 explorer_output[game_resource_path_and_name_MAX];

	u8 import_path[game_resource_path_and_name_MAX];

}s_game_resource_editor;

typedef enum{
	ea_focus_nothing,
	ea_focus_animation_list,
	ea_focus_timeline,
}editor_animation_ui_focus;

typedef struct editor_clipboard_chunk{
	struct editor_clipboard_chunk *next;
	struct editor_clipboard_chunk *prev;
	u32 used;
	u32 max;
	u8 *base;
}editor_clipboard_chunk;

typedef struct{
	editor_clipboard_chunk *first_chunk;
	editor_clipboard_chunk *last_chunk;
	u32 used;
	u32 max;
	u8 *memory;
}editor_clipboard;

typedef struct{
	u8 *name;
}editor_frame_key;

typedef struct{
	model_sprite base;
	memory_dyarray(sprite_orientation, uvs);
}editor_model_sprite;

typedef struct{
	model_bone base;
	u32 sprite_count;
	u32 frame_key_count;
	memory_dyarray(editor_frame_key, frame_keys);
	memory_dyarray(editor_model_sprite, sprites);
}editor_model_bone;

typedef struct{
	model_attachment_data base;
	game_resource_attributes *model;
}editor_model_attachment;

typedef struct s_model_editor{

	union{
		struct em_restorable{
			u16 sprite_sheets_count;
			u16 attachment_count;
			u16 virtual_node_count;
			u16 bone_count;
			u16 sprite_count;
			u16 frame_key_count;
			u16 keyframe_clipboard_count;
			u16 selected_bone_index;

			u16 selected_sprite_index;
			b16 sprite_is_selected;

			u16 uvs_count;
			u16 keyframe_group_count;
			editor_animation_keyframe_row *first_free_row;
			editor_animation_keyframe_column *first_free_column;
			editor_animation_keyframe_row *selected_row;
			editor_animation_keyframe_column *selected_column;
			b8 column_is_selected;
			b8 row_is_selected;
			u32 sprite_clipboard_count;
			u16 animation_count;
			u16 clip_count;
			u16 keyframe_count;
			u32 frame_keyframe_count;
			b16 reproduce_animation;

			b16 animation_is_selected;
			u16 selected_animation_index;
			b32 keyframe_is_selected;
			editor_animation_keyframe *selected_keyframe;
			u16 frame_list_count;
			u16 timeline_column_count;
			u16 timeline_row_count;

			game_resource_attributes *editing_model;

			b16 frame_list_is_selected;
			u16 selected_frame_list_paint_mode;

			b16 timeline_row_is_selected;
			b16 timeline_frame_is_selected;
			u16 timeline_selected_frame;

			b8 animation_bone_is_selected;
			u16 animation_selected_bone_index;


			eui_selection sprite_selection_bone;
			eui_selection bone_selected_sprite;
			eui_selection bone_selected_frame_key;
			eui_selection sprite_frame_selection;
			eui_selection attachment_selection;
		};
		struct em_restorable reset;
	};
	u32 tp_count;
	vec3 temp_points[100];

	memory_area per_frame_area;
	u32 orientation_amount;
	u16 sprite_sheets_max;
	render_texture **sprite_sheets_as_asset;
	game_resource_attributes **sprite_sheets;
	model loaded_model;
	model_pose loaded_model_pose;
	model_pose loaded_model_ik_pose;
	editor_animation_ui_focus ui_focus;
	u8 properties_ui_focus;

	model_mesh_frame_list *model_frame_lists;
	model_animation *render_model_animations;
	model_animation_keyframe *render_model_keyframes;

	u16 attachment_max;
	u16 attachment_to_model;
	//not used yet
	editor_model_attachment *attachments;
	u16 virtual_node_max;
	model_virtual_node *virtual_nodes;


	editor_cursor cursor;
	editor_cursor_memory cursor_memory;
	editor_model_tool tool;

	b16 in_camera_mode;
	b16 display_edges;

	u16 boneCapacity;
	editor_model_bone *bones;

	u16 sprite_max;
	model_sprite *sprites;

	b32 node_preview_mode;

	b16 billboard_sprite_editing_orientations;
	u16 billboard_sprite_selected_orientation;
	u16 billboard_sprite_selected_upper_orientation;
	u16 billboard_sprite_selected_lower_orientation;

	u16 individual_sprite_editing;
	u16 individual_sprite_selecting_uvs;
	u32 individual_sprite_editing_uvs_copied;
	u32 individual_sprite_editing_uvs_index;

	vec3 ik_vec;
	f32 timeline_zoom;
	vec2 sprite_editing_uv2_copy;
	vec2 sprite_editing_uv3_copy;

	u32 rotating_node_axis;
	u32 selected_node_axis_index;
	b16 link_sprites_to_bone;

	u32 selected_meshes_texture_index;

	f32 cameraDistance;

	editor_name_chunks attachment_bone_names;
	editor_name_chunks bone_name_chunks;
	editor_name_chunks sprite_name_chunks;
	//names
	editor_name_chunks animation_name_chunks;
	editor_name_chunks frame_list_names;

	u16 frame_key_max;
	editor_frame_key *frame_keys;
	editor_name_chunks frame_key_names;

	f32 camera_rotation_x;
	f32 camera_rotation_z;

	vec3 camera_position;

	u32 selected_cursor_texture_index;

	vec2 next_sprite_uv0;
	vec2 next_sprite_uv1;
	vec2 next_sprite_uv2;
	vec2 next_sprite_uv3;

	u16 cursor_repaint_mode;
	u16 cursor_stick_mode;

	i32 gizmo_rotation_x;
	i32 gizmo_rotation_y;
	i32 gizmo_rotation_z;
	u32 apply_gizmo_rotation;
	u32 moving_gizmo;

	u32 cursor_selection_size_x;
	u32 cursor_selection_size_y;
	u32 moving_selections;
	u32 selected_sprite_sheet_index;

	model_sprite *sprite_clipboard;

	u16 uvs_max;
	//used for creating a rendering model
	sprite_orientation *uvs;


	//where to make it look
	u16 model_orientation_index;
	u16 model_orientation_count;
	vec2 model_foward;
	//animation data

	u16 animation_max;

	u16 keyframe_max;

	u16 keyframe_group_max;
	//editor_animation_keyframe_group *first_free_keyframe_group;
	//editor_animation_keyframe_group *keyframe_groups;

	b32 keyframe_group_is_selected;
	//editor_animation_keyframe_group *selected_keyframe_group;
	u16 selected_timeline_row;
	u16 timeline_column_max;
	//editor_animation_timeline_column *timeline_columns;

	editor_animation_keyframe *animation_keyframes;
	editor_animation_keyframe *first_free_keyframe;
	u16 timeline_row_max;
	editor_animation_keyframe_row *timeline_rows;
	editor_animation_keyframe_column *timeline_columns;

	model_animation_timer timer;
	//	    model_animation   *animations;
	editor_animation *animations;
	b8 lock_descendants;
	b32 apply_ik;

	memory_dyarray_area *dyarrays_area;

	u16 frame_list_max;
	editor_model_frame_list *frame_lists;
	u16 selected_frame_list_index;
	b16 frame_list_frame_is_selected;
	u16 frame_list_selected_frame;
	b16 frame_list_is_selected_paint_mode;
	u32 frame_list_selected_index;

	editor_clipboard frame_list_clipboard;

	b32 focused_on_keyframe_only;
	//clipboard
	u16 keyframe_clipboard_max;
	model_animation_keyframe *keyframe_clipboard;


	//misc



	//for frame list uvs
	//copied uvs
	vec2 c_uv0;
	vec2 c_uv1;
	vec2 c_uv2;
	vec2 c_uv3;
	b16 selection_uvs_copied;
	b16 display_image_selection_grid;
	u16 selection_grid_w;
	u16 selection_grid_h;

	f32 image_selection_zoom;
	u32 image_selection_corner;
	b8 image_selection_hot;
	b8 image_selection_down;
	b8 image_selection_selecting;
	b8 image_selection_display_grid;


}s_model_editor;


typedef struct s_game_editor{

	s_game_editor_mode mode;

	bool32 ui_is_interacting;
	bool32 ui_is_focused;
	bool32 explorer_is_opened;

	s_game_console *game_console;
	stream_data info_stream;

	memory_area area;

	u8 *reserved_space;
	u32 reserved_space_size;

	u32 reload_file_info;

	u16 data_files_max;
	u16 data_files_count;
	asset_file_info *data_files;

	f32 tipTimer;
	f32 tipTotalTime;

	u32 tipBufferSize;
	u8 *tipBuffer;

	u32 packed_assets_count;
	asset_file_info *packed_assets;

	bool32 process_input;
	//global settings
	struct{
		f32 vertices_cube_size;
	}settings;
	//
	// World editor
	//
	//struct{
	//struct s_world_editor world;
	struct s_coso_editor coso;
	struct s_world_editor world;
	struct s_tileset_editor tileset;
	struct s_game_resource_editor asset;
	struct s_model_editor model;
	struct s_frame_animation_editor frame_animation;

	editor_graphics_tab graphics_tab;


	f32 sin_value;
	f32 tan_x;
	f32 tan_y;
	union{

		quaternion e_q;
		struct{
			f32 w;
			f32 x;
			f32 y;
			f32 z;
		};
	};
	vec3 e_foward;

}s_game_editor;

typedef struct s_editor_state{

	platform_api *platform;
	game_ui *ui;
	font_proportional *ui_font;
	s_game_console debug_console;

	struct s_game_assets *editor_assets;
	s_game_editor editor;

}s_editor_state;














inline editor_name_chunks
editor_name_chunks_allocate(memory_area *area,
		                    u32 maximum_chunks,
							u32 maximum_name_length)
{
	editor_name_chunks result = {0};
	if(maximum_name_length)
	{
		result.max    = maximum_chunks;
		result.length = maximum_name_length;
		u32 name_chunks_max_size = maximum_chunks * maximum_name_length;

		//push a chunk structs array
		result.chunks = memory_area_push_array(area, struct name_chunk, name_chunks_max_size);
		u32 n = 0;
		//allocate a name for every chunk
		while(n < maximum_chunks)
		{
			result.chunks[n].name = memory_area_push_size(area, maximum_name_length);
			n++;
		}
	}

	return(result);
}

inline struct name_chunk * 
editor_name_chunks_add(editor_name_chunks *name_chunks, u8 *name_default)
{
	u32 chunk_count            = name_chunks->count;
	struct name_chunk *name_at = name_chunks->chunks + chunk_count;

	memory_clear(name_at->name,
			     name_chunks->length);
	string_copy(name_default,
			    name_at->name);
	Assert(name_chunks->count < name_chunks->max);
	name_chunks->count++;

	return(name_at);

}

inline struct name_chunk *
editor_name_chunks_addf(editor_name_chunks *name_chunks, u8 *name_default, ...)
{
	text_format_quick quickFormat = {0};
	FORMAT_TEXT_QUICK(quickFormat, name_default);

	struct name_chunk *name = editor_name_chunks_add(name_chunks, quickFormat.text_buffer);

	return(name);
}

static void
editor_name_chunks_add_amount(
		editor_name_chunks *chunks,
		u32 amount,
		u8 *name_default)
{
	for(u32 a = 0; a < amount && a < chunks->max; a++)
	{
		editor_name_chunks_addf(chunks, "%s%u", name_default, a);
	}
}

static void
editor_name_chunks_set(
		editor_name_chunks *name_chunks,
		u32 index,
		u8 *new_name)
{
	//range check?
	memory_clear(
			name_chunks->chunks[index].name,
			name_chunks->length);
	string_copy(new_name,
			name_chunks->chunks[index].name);
}

inline void
editor_name_chunks_reset(editor_name_chunks *name_chunks)
{
	name_chunks->count = 0;
}

inline void
editor_name_chunks_remove(editor_name_chunks *name_chunks,
		                  u32 chunk_index)
{
	u32 chunk_count       = name_chunks->count;
	u32 chunk_name_length = name_chunks->length;

	u32 b = chunk_index;
	while(b < chunk_count - 1)
	{
	    struct name_chunk name0 = name_chunks->chunks[b];
	    struct name_chunk name1 = name_chunks->chunks[b + 1];

	    memory_Copy(name1.name,
	                name0.name,
	                chunk_name_length);
	    b++;
	}
	name_chunks->count--;
}


static inline u32
editor_write_name_chunks_to_file(
		editor_name_chunks *name_chunks,
		platform_api *platform,
		platform_file_handle file,
		u32 data_offset,
		stream_data *info_stream)
{
	//write header
	ppse_editor_names_header names_header = {0};
	names_header.signature = ppse_EDITOR_NAMES_SIGNATURE;
	u32 names_header_write = data_offset;
	data_offset += sizeof(ppse_editor_names_header);
	//write the names for the editor
	for(u32 t = 0;
			t < name_chunks->count;
			t++)
	{
		u8 *name = name_chunks->chunks[t].name;
		u32 name_length = string_count(name);
		//write name length
		platform->f_write_to_file(
				file,
				data_offset,
				sizeof(u32),
				&name_length);
		data_offset += sizeof(u32);
		//write name
		platform->f_write_to_file(
				file,
				data_offset,
				name_length,
				name);
		data_offset += name_length;

		stream_pushf(info_stream,
				"%u. Saved name \"%s\"",
				t,
				name);
		names_header.count++;
	}

	platform->f_write_to_file(
			file,
			names_header_write,
			sizeof(ppse_editor_names_header),
			&names_header);
	stream_pushf(info_stream, "Total amount of names saved: %u", names_header.count); 

	return(data_offset);

}

static inline u32
editor_load_name_chunks_from_file(
		editor_name_chunks *name_chunks,
		platform_api *platform,
		platform_file_handle file,
		u32 *data_offset,
		stream_data *info_stream)
{
	//reset
	name_chunks->count = 0;

	u32 read_offset = *data_offset;
	u32 loaded_names_count = 0;

	ppse_editor_names_header names_header = {0};
	platform->f_read_from_file(
			file, read_offset, sizeof(names_header), &names_header);
	read_offset += sizeof(names_header);

	if(names_header.signature == ppse_EDITOR_NAMES_SIGNATURE)
	{
		for(u32 c = 0; c < names_header.count; c++)
		{
			loaded_names_count++;
			u32 length = 0;
			//not very safe if the length of the names > 256
			u8 name_buffer[256] = {0};
			platform->f_read_from_file(file, read_offset, sizeof(u32), &length);
			//advance length size
			read_offset += sizeof(u32);
			//read name
			platform->f_read_from_file(file, read_offset, length, name_buffer);
			//advance total length
			read_offset += length;
			//add name. All names are saved with their null-terminated character
			editor_name_chunks_add(name_chunks, name_buffer);
		}
	}

	*data_offset = read_offset;
	return(loaded_names_count);
}

static inline u32
editor_load_name_chunks_from_memory(
		editor_name_chunks *name_chunks,
		u32 *data_offset,
		u8 *contents,
		stream_data *info_stream)
{
	//reset
	name_chunks->count = 0;

	u32 read_offset = *data_offset;
	u32 loaded_names_count = 0;

	ppse_editor_names_header names_header = *(ppse_editor_names_header *)(contents  + read_offset);
	read_offset += sizeof(names_header);

	if(names_header.signature == ppse_EDITOR_NAMES_SIGNATURE)
	{
		for(u32 c = 0; c < names_header.count; c++)
		{
			loaded_names_count++;
			//not very safe if the length of the names > 256
			u32 length = *(u32 *)(contents + read_offset);
			//advance length size
			read_offset += sizeof(u32);
			//read name
			u8 *name_buffer = (contents + read_offset);
			//advance total length
			read_offset += length;
			//add name. All names are saved with their null-terminated character
			editor_name_chunks_add(name_chunks, name_buffer);
		}
	}

	*data_offset = read_offset;
	return(loaded_names_count);
}

static inline u32
editor_set_name_chunks_from_file(
		editor_name_chunks *name_chunks,
		platform_api *platform,
		u32 *data_offset,
		platform_file_handle file,
		stream_data *info_stream)
{
	u32 old_count = name_chunks->count;
	//reset
	editor_load_name_chunks_from_file(
			name_chunks,
			platform,
			file,
			data_offset,
			info_stream);
	if(name_chunks->count < old_count)
	{
		//add difference without clearing!
		name_chunks->count = old_count;
	}
	return(name_chunks->count);
}

static u32
editor_wr_set_name_chunks(
		editor_wr *wr,
		editor_name_chunks *name_chunks,
		stream_data *info_stream)
{
	u32 old_count = name_chunks->count;

	u32 loaded_count = editor_load_name_chunks_from_memory(
		name_chunks,
		&wr->data_offset,
		wr->contents,
		info_stream);
	if(loaded_count < old_count)
	{
		name_chunks->count = old_count;
	}
	return(name_chunks->count);
}




//clipboard for arbitrary data

static editor_clipboard
editor_clipboard_create(
		u8 *memory,
		u32 max)
{
	editor_clipboard result = {0};
	result.memory = memory;
	result.max = max;

	return(result);
}

static void
editor_clipboard_reset(
		editor_clipboard *clipboard)
{
	clipboard->used = 0;
	clipboard->first_chunk = 0;
	clipboard->last_chunk = 0;
}

static editor_clipboard_chunk *
editor_clipboard_copy_begin(
		editor_clipboard *clipboard)
{
	editor_clipboard_chunk *header = clipboard->first_chunk;
	if(!header)
	{
		header = (editor_clipboard_chunk *)clipboard->memory;
		clipboard->used += sizeof(*header);
		clipboard->first_chunk = header;
	}
	else
	{
		header = (editor_clipboard_chunk *)(clipboard->memory + clipboard->used);
		clipboard->used += sizeof(*header);
	}

	if(clipboard->last_chunk)
	{
		clipboard->last_chunk->next = header;
	}
	memory_clear(header, sizeof(*header));

	header->max = clipboard->max - clipboard->used;
	header->base = clipboard->memory + clipboard->used;
	header->prev = clipboard->last_chunk;

	clipboard->last_chunk = header;
	return(header);
}

static b32
editor_clipboard_copy_end(
		editor_clipboard *clipboard,
		editor_clipboard_chunk *chunk)
{
	b32 success = 0;
	if(chunk->used < chunk->max)
	{
		clipboard->used += chunk->used;
		success = 1;
	}
	else
	{
		//cancel this copy
		clipboard->used -= sizeof(editor_clipboard_chunk);
		clipboard->last_chunk = chunk->prev;
		if(chunk == clipboard->first_chunk)
		{
			clipboard->first_chunk = 0;
		}
	}
	return(success);
}

static u8 *
editor_clipboard_chunk_push_size(
		editor_clipboard_chunk *chunk,
		u32 size)
{
	u8 *result = 0;
	if(chunk->used + size < chunk->max)
	{
		result = chunk->base + chunk->used;
		memory_clear(result, size);
	}
	chunk->used += size;
	return(result);
}

static u8 *
editor_clipboard_chunk_push_and_copy_size(
		editor_clipboard_chunk *chunk,
		u32 size,
		void *memory)
{
	u8 *result = editor_clipboard_chunk_push_size(chunk, size);
	if(result)
	{
		memory_copy(memory, result, size);
	}
	return(result);
}

static void *
editor_clipboard_chunk_read_size(
		editor_clipboard_chunk *chunk,
		u32 size,
		u32 *at)
{
	void *result = chunk->base + *at;
	*at += size;
	return(result);
}





//hash


static inline u32
editor_generate_hash_key_u32(u32 h, u32 seed) {
	h ^= h >> 16;
	h += seed;
	h *= 0x3243f6a9U;
	h ^= h >> 16;
	return(h);
}

static u32
editor_generate_hash_key(u8 *text)
{
	u32 seed = 249144;
	u8 c = 0;
	u32 i = 0;
	u32 kindaHash = 0;
	while((c = text[i++]) != '\0')
	{
		u32 c32 = (u32)c;
		kindaHash += noise_u32(c32, i + seed);
	}

	return(kindaHash);
}

#define editor_hash_get_slot(hash, string) editor_hash_get_slot_by_key(hash, editor_generate_hash_key(string))
static editor_hash_index *
editor_hash_get_slot_by_key(
		editor_hash *hash,
		u32 key)
{
	u32 index = key % hash->max;

	editor_hash_index *result = 0;
	if(hash->count < hash->max)
	{
		result = hash->indices + index;
		while(result->key != 0 &&
			  result->key != key)
		{
			index++;
			index %= hash->max;
			result = hash->indices + index;
		}
	}
	return(result);
}

#define editor_hash_get_index(hash, string) editor_hash_get_index_by_key(hash, editor_generate_hash_key(string))
static editor_hash_index *
editor_hash_get_index_by_key(
		editor_hash *hash,
		u32 key)
{
	editor_hash_index *result = editor_hash_get_slot_by_key(hash, key);
	result = result && result->key ? result : 0;
	return(result);
}


static b32 
editor_add_hash_index(
		editor_hash *hash,
		u32 index,
		u8 *string)
{
	// <-[key, index]
	//[0][1][2][3][4][4]
	u32 key = editor_generate_hash_key(string);
	b32 success = 0;
	editor_hash_index *hash_index = editor_hash_get_slot_by_key(hash, key);
	//this slot was not occuppied
	if(!hash_index->key)
	{
		hash_index->key = key;
		hash_index->index = index;
		hash->count++;
		success = 1;

	}
	else
	{
		success = 0;
	}
	return(success);
}

static b32
editor_add_hash_indexf(
		editor_hash *hash,
		u32 index,
		u8 *string)
{
	u8 text_buffer[256] = {0};
	va_list args;
	va_start_m(args, string);
	format_text_list(text_buffer, sizeof(text_buffer), string, args);
	va_end_m(args);

	b32 result = editor_add_hash_index(hash, index, text_buffer);
}

static b32
editor_hash_modify_key(editor_hash *hash, u8 *string0, u8 *string1)
{
	editor_hash_index *index0 = editor_hash_get_slot(hash, string0);
	//this one should not exist
	editor_hash_index *index1 = editor_hash_get_slot(hash, string1);
	b32 success = 0;
	//it exists, so modify
	if(index0->key && !index1->key)
	{
		u32 new_key = editor_generate_hash_key(string1);
		if(new_key)
		{
			index0->key = 0;
			*index1 = *index0;
			index1->key = new_key;
			success = 1;
		}
	}
	return(success);
}

typedef enum{
	hash_name_null,
	hash_name_zero,
	hash_name_exists,
}editor_hash_check;
static b32
editor_hash_string_is_avadible(editor_hash *hash, editor_hash_check *hash_check, u8 *string)
{
	editor_hash_index *index0 = editor_hash_get_index(hash, string);
	b32 success = 0;
	//it exists, so modify
	if(!index0)
	{
		u32 key = editor_generate_hash_key(string);
		if(key)
		{
			success = 1;
		}
		else
		{
			*hash_check = hash_name_zero;
		}
	}
	else
	{
		*hash_check = hash_name_exists;
	}
	return(success);
}

static void
editor_hash_remove(editor_hash *hash, u8 *string)
{
	editor_hash_index *index = editor_hash_get_index(hash, string);
	index->key = 0;
}

static inline editor_hash
editor_hash_allocate(
		memory_area *area,
		u32 max)
{
	editor_hash result = {0};
	result.indices = memory_area_clear_and_push_array(
			area, editor_hash_index, max);
	result.max = max;
	return(result);
}







//hash and names


static b32
editor_hash_and_name_add_index(
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 index,
		u8 *string)
{
	b32 success = editor_add_hash_index(
			hash,
			index,
			string);
	if(success)
	{
		editor_name_chunks_add(
				name_chunks,
				string);
	}
	return(success);
}

static b32
editor_hash_and_name_add_indexf(
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 index,
		u8 *string,
		...)
{
	u8 text_buffer[256] = {0};
	va_list args;
	va_start_m(args, string);
	format_text_list(text_buffer, sizeof(text_buffer), string, args);
	va_end_m(args);

	b32 success = editor_add_hash_index(
			hash,
			index,
			text_buffer);
	if(success)
	{
		editor_name_chunks_add(
				name_chunks,
				text_buffer);
	}
	return(success);
}


static b32
editor_hash_and_name_add_index_numbered(
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 index,
		u8 *string,
		...)
{
	u32 n = index;
	while(!editor_hash_and_name_add_indexf(
				hash,
				name_chunks,
				index,
				"%s %u",
				string,
				n++));
	return(1);
}

static void
editor_hash_and_name_remove(
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 index)
{
	if(index < name_chunks->count)
	{
		u8 *name = name_chunks->chunks[index].name;
		editor_hash_remove(hash, name);
		editor_name_chunks_remove(name_chunks, index);
	}
}
static void
editor_hash_and_name_chunks_allocate(
		memory_area *area,
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 max,
		u32 name_length)
{
	Assert(hash->max == 0 && name_chunks->chunks == 0);
	*hash = editor_hash_allocate(area, max);
	*name_chunks = editor_name_chunks_allocate(area, max, name_length);
}

static b32
editor_hash_and_name_modify_key_by_index(
		editor_hash *hash,
		editor_name_chunks *name_chunks,
		u32 name_index,
		u8 *new_name)
{
	b32 success = 
		editor_hash_modify_key(hash, name_chunks->chunks[name_index].name, new_name);
	if(success)
	{
		editor_name_chunks_set(
				name_chunks, name_index, new_name);
	}
	return(success);
}

static u32
editor_hash_and_name_get_index(
		editor_hash *hash, editor_name_chunks *name_chunks, u32 chunk_index)
{
	u8 *name = name_chunks->chunks[chunk_index].name;
	editor_hash_index *hash_index = editor_hash_get_index(hash, name);
	//keep going....
	return(hash_index->index);
}

static void
editor_hash_and_name_reset(
		editor_hash *hash, editor_name_chunks *name_chunks)
{
	memory_clear(hash->indices, sizeof(editor_hash_index) * hash->max);
	editor_name_chunks_reset(name_chunks);
}


//wr

static void
editor_wr_write_name_chunks(
		editor_wr *wr,
		editor_name_chunks *name_chunks,
		stream_data *info_stream)
{
	//write header
	ppse_editor_names_header *names_header = editor_wr_put_struct(
			wr, ppse_editor_names_header);
	names_header->signature = ppse_EDITOR_NAMES_SIGNATURE;
	//write the names for the editor
	for(u32 t = 0;
			t < name_chunks->count;
			t++)
	{
		u8 *name = name_chunks->chunks[t].name;
		u32 name_length = string_count(name);
		//write name length
		editor_wr_write_size(
				wr,
				sizeof(u32),
				&name_length);
		wr->data_offset += sizeof(u32);
		//write name
		editor_wr_write_size(
				wr,
				name_length,
				name);
		wr->data_offset += name_length;

		stream_pushf(info_stream,
				"%u. Saved name \"%s\"",
				t,
				name);
		names_header->count++;
	}
	stream_pushf(info_stream, "Total amount of names saved: %u", names_header->count); 
}
