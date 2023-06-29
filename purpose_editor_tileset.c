#define editor_tileset_autoterrain_name(ts_editor, index) (ts_editor->autoterrain_names.chunks[index].name)
#define editor_tileset_terrain_name(ts_editor, index) (ts_editor->terrain_names.chunks[index].name)
#define editor_tileset_wall_name(ts_editor, index) (ts_editor->wall_names.chunks[index].name)
#define editor_tileset_selected_terrain_name(ts_editor) (editor_tileset_terrain_name(ts_editor, ts_editor->selected_terrain_index))
#define indices_capacity_per_AUTOTERRAIN 48


#define index_1d(x, y, w) (x + (y * w))
static void 
read_and_fill_bit_corners(
		i32 bit_rows_x,
		i32 bit_rows_y,
		i32 bit_w,
		i32 bit_h,
		i32 start_x,
		i32 end_x,
		i32 start_y,
		i32 end_y,
		u8 *corners_read,
		u8 *bit_rows
		)
{
	for(i32 y = start_y; y <= end_y; y++)
	{
		i32 y_at = bit_rows_y + y;
		i32 y_at_corner = y + 1; 
		if(y_at == -1)
		{
			continue;
		}
		if(y_at >= bit_h)
		{
			break;
		}
		for(i32 x = start_x; x <= end_x; x++)
		{
			i32 x_at = bit_rows_x + x;
			i32 x_at_corner = x + 1;
			if(x_at == -1)
			{
				continue;
			}
			if(x_at >= bit_w)
			{
				continue;
			}
#define _index_1d(x, y, w) (x + (y * w))
			u32 corners_index = _index_1d(x_at_corner, y_at_corner, 3);
			u32 rows_index = _index_1d(x_at, y_at, bit_w);
			corners_read[corners_index] = bit_rows[rows_index] == 1;
		}
	}
}

static u32
read_bit_corners(
		u8 *bit_rows, 
		i32 bit_x,
		i32 bit_y,
		i32 bit_w,
		i32 bit_h)
{

	u16 bit_rows_x = 0;
	u16 bit_rows_y = 0;
	u8 tl = 0;
	u8 tm = 1;
	u8 tr = 2;
	u8 ml = 3;
	u8 mm = 4;
	u8 mr = 5;
	u8 bl = 6;
	u8 bm = 7;
	u8 br = 8;
	u32 bit = 0;
	u8 corners_read[9] = {0};
	{
		i32 x = bit_x;
		i32 y = bit_y;
		read_and_fill_bit_corners(x, y, bit_w, bit_h, -1, 1, 0, 0, corners_read, bit_rows);
		read_and_fill_bit_corners(x, y, bit_w, bit_h, 0, 0, -1, 1, corners_read, bit_rows);
		i32 x_s = -1;
		i32 x_e = 1;
		//top corners
		if(corners_read[tm])
		{
			read_and_fill_bit_corners(x, y, bit_w, bit_h, x_s, x_e, -1, -1, corners_read, bit_rows);
		}
		x_s = -1;
		x_e = 1;
		//bottom corners
		if(corners_read[bm])
		{
			read_and_fill_bit_corners(x, y, bit_w, bit_h, x_s, x_e, 1, 1, corners_read, bit_rows);
		}
		if(corners_read[ml])
		{
			read_and_fill_bit_corners(x, y, bit_w, bit_h, -1, -1, -1, 1, corners_read, bit_rows);
		}
		if(corners_read[mr])
		{
			read_and_fill_bit_corners(x, y, bit_w, bit_h, 1, 1, -1, 1, corners_read, bit_rows);
		}
	}
	corners_read[4] = 0;
	//finally, read the bit values
	u8 bit_values[10] = {
		1, 2, 4,
		8, 0, 16,
		32, 64, 128
	};
	for(u32 y = 0; y < 3; y++)
	{
		for(u32 x = 0; x < 3; x++)
		{
			u32 i = x + (y * 3);
			bit |= (bit_values[i] * corners_read[i]);
		}
	}
	return(bit);
#if 0
	if(bit_rows_x + 1 + (bit_rows_y * bit_w) == 1)
	{
		for(i32 x = bit_rows_x; x < bit_rows_x + 3; x++)
		{
			if(!x)
			{
				x++;
			}
			if(x + 1 >= bit_w)
			{
				break;
			}
		}
		//read edges
		{
			i32 y = bit_rows_y;
			i32 x = bit_rows_x;
			if(x)
			{
				u32 index = index_1d(x - 1, y, bit_w); 
				corners_read[ml] = bit_rows[index] == 1;
			}
			if(x < bit_w)
			{
				u32 index = (x + 1) + (y * bit_w);
				corners_read[mr] = bit_rows[index] == 1;
			}
			if(y)
			{
				u32 index = x + ((y - 1) * bit_w);
				corners_read[tm] = bit_rows[index] == 1;
			}
			if(y < bit_h)
			{
				u32 index = x + ((y + 1) * bit_w);
				corners_read[bm] = bit_rows[index] == 1;
			}
		}
		//read corners
		u32 x = 0;
		u32 y = 0;
		if(corners_read[tm] && x)
		{
			u32 index = index_1d(x - 1, y - 1, bit_w); 
			corners_read[tl] = bit_rows[index] == 1;
		}
		//for(i32 y = -1; y < 2; y++)
		//{
		//	i32 x_at = bit_rows_x;
		//	i32 y_at = bit_rows_y + y;
		//	if(y_at == -1)
		//	{
		//		y++;
		//		y_at++;
		//	}
		//	if(y + 1 >= bit_rows_h)
		//	{
		//		break;
		//	}
		//	u32 index = index_1d(x_at, y_at, bit_rows_w); 
		//	u32 index = index_1d(x_at, y_at, bit_rows_w); 
		//}
	}
#endif
}

static void
editor_tileset_reset_adding_uvs(s_game_editor *game_editor)
{
	game_editor->tileset.adding_tile_uv0.x = 0;
	game_editor->tileset.adding_tile_uv0.y = 0;
	game_editor->tileset.adding_tile_uv1.x = 0;
	game_editor->tileset.adding_tile_uv1.y = 0;
	game_editor->tileset.adding_tile_uv2.x = 0;
	game_editor->tileset.adding_tile_uv2.y = 0;
	game_editor->tileset.adding_tile_uv3.x = 0;
	game_editor->tileset.adding_tile_uv3.y = 0;
}
//helper stuff
static inline b32 
_terrain_tile_image_button(
		game_ui *ui, 
		struct s_tileset_editor *tileset_editor, 
		editor_tileset_autoterrain *autoterrain,
		render_texture *tileset_texture,
		u32 bit,
		u32 capacity,
		u32 layer,
		u8 *label)
{
	
	b32 clicked = 0;
	ui_node *row_images_node = ui_labelf(ui, "%s%u", label, bit);
	ui_node *row_images_image_node = 0;
	layer = layer > autoterrain->base.extra_layers ? autoterrain->base.extra_layers : layer;
	u32 bit_index = 0;
	if(capacity == 16)
	{
		bit_index = bit;
	}
	else if(capacity == 48)
	{
		bit_index = autoterrain_index_from_mask46(0, bit);
	}
	bit_index += (layer * indices_capacity_per_AUTOTERRAIN);
	u16 *index = 0;
	memory_dyarray_get_safe(autoterrain->indices, index, bit_index);
	ui_push_id_string(ui, label);
	if(*index < tileset_editor->terrain_count)
	{
		model_mesh *uvs = memory_dyarray_get(tileset_editor->terrain[*index].meshes, 0);
		ui_set_parent(ui, row_images_node)
		{
			row_images_image_node = ui_button_image_uvs_nodef(
					ui,
					tileset_texture,
					uvs->uv0,
					uvs->uv1,
					uvs->uv2,
					uvs->uv3,
					"autoterrain_tile_image_uvs%u",
					bit);
		}
	}
	else
	{
		ui_set_parent(ui, row_images_node)
		{
			row_images_image_node = ui_selectable_boxf(
					ui,
					bit == tileset_editor->sel_autoterrain_mask_value,
					"autoterrain_tile_image_uvs%u", bit);
		}

	}
	if(row_images_image_node)
	{
		ui_node_push_textf(ui,
				row_images_image_node,
				1,
				1,
				0,
				V4(200, 200, 200, 200),
				"%u",
				bit);
		if(ui_node_mouse_l_up(ui, row_images_image_node))
		{
			//tileset_editor->sel_autoterrain_mask_value = bit;
			clicked = 1;
		}
	}
	ui_pop_id(ui);
	return(clicked);
}

static inline void
_list_of_terrain_with_images(game_ui *ui, 
		struct s_tileset_editor *tileset_editor, 
		render_texture *tileset_texture,
		f32 img_terrain_size,
		u32 *index,
		u8 *label)
{
	ui_push_id_string(ui, label);
	ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_specified(ui, img_terrain_size, 1.0f)
	for(u32 t = 0; t < tileset_editor->terrain_count; t++)
	{
		//f32 img_terrain_size = 32.0f;
		ui_node *sel_box = ui_selectable_boxf(ui, 
				*index == t,
				"%u. %s#terrain_from_at_index%u",
				t, tileset_editor->terrain_names.chunks[t].name, t);
		ui_set_parent(ui, sel_box)
		{
			ui_set_row(ui)
			{
				ui_set_column(ui)
				{
					ui_space_specified(ui, 4.0f, 1.0f);
					ui_set_row(ui)
					{
						ui_space_specified(ui, 4.0f, 1.0f);
						//top image
						editor_tileset_terrain *e_terrain = tileset_editor->terrain + t;
						model_mesh *mesh = memory_dyarray_get(
								e_terrain->meshes, 0);

						ui_set_wh(ui, ui_size_specified(img_terrain_size, 1.0f))
						{
							ui_image_uvs(ui,
									tileset_texture,
									mesh->uv0,
									mesh->uv1,
									mesh->uv2,
									mesh->uv3
									);
						}
						ui_space_specified(ui, 4.0f, 1.0f);
					}
					//space below
					ui_space_specified(ui, 4.0f, 1.0f);
				}
				ui_set_w_text(ui, 1.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
				{
					ui_node *text = ui_create_node(ui, node_text | node_text_centered, 0);
					ui_node_set_display_string(ui, text, tileset_editor->terrain_names.chunks[t].name);

				}
			}
		}
		//set if clicked
		if(ui_node_mouse_l_up(ui, sel_box))
		{
			*index = t;
		}
	}
	ui_pop_id(ui);
}

static inline void
editor_tileset_allocate(
		s_game_editor *game_editor)
{
	memory_area *editor_area = &game_editor->area;
	game_editor->tileset.terrain_max = 200;
	game_editor->tileset.autoterrain_max = 100;
	game_editor->tileset.wall_max = 100;

	game_editor->tileset.terrain_names = editor_name_chunks_allocate(
			editor_area,
			game_editor->tileset.terrain_max,
			30);
	game_editor->tileset.autoterrain_names = editor_name_chunks_allocate(
			editor_area,
			game_editor->tileset.autoterrain_max,
			30);
	game_editor->tileset.wall_names = editor_name_chunks_allocate(
			editor_area,
			game_editor->tileset.wall_max,
			30);
	//terrain array
	game_editor->tileset.terrain = memory_area_push_array(
			editor_area,
			editor_tileset_terrain,
			game_editor->tileset.terrain_max);
	//autoterrain array
	game_editor->tileset.autoterrains = memory_area_push_array(
			editor_area,
			editor_tileset_autoterrain,
			game_editor->tileset.autoterrain_max);
	//wall array
	game_editor->tileset.walls = memory_area_push_array(
			editor_area,
			editor_tileset_wall,
			game_editor->tileset.wall_max);
	//autoterrain_indices_array
	//the capacity will be based on the maximum amount of autoterrains.
	game_editor->tileset.autoterrain_max = game_editor->tileset.terrain_max * indices_capacity_per_AUTOTERRAIN;
//	game_editor->tileset.autoterrain_indices = memory_area_push_array(
//			editor_area,
//			u16,
//			game_editor->tileset.autoterrain_max);

	//expandable zone of terrains
	game_editor->tileset.terrain_meshes_zone = 
		memory_dyarray_area_create(
				editor_area, KILOBYTES(512));
}

inline void
editor_tileset_new(s_game_editor *editor)
{
	editor->tileset.selected_tileset_animation_index = 0;
	editor->tileset.selected_tileset_tile_index = 0;
	editor->tileset.terrain_count = 0;
	editor->tileset.selected_terrain_index = 0;
	editor->tileset.terrain_names.count = 0;
	editor->tileset.editing_tileset = 0;
	editor->tileset.tileset_texture = 0;
	editor->tileset.wall_count = 0;
	editor->tileset.selected_wall_index = 0;
	editor->tileset.wall_is_selected = 0;

	editor->tileset.autoterrain_count = 0;
//	editor->tileset.autoterrain_indices_count = 0;
	editor->tileset.autoterrain_names.count = 0;
	editor->tileset.wall_names.count = 0;

	memory_area_dyarrays_reset(editor->tileset.terrain_meshes_zone);

	world_tileset zero_tileset = {0};
	editor->tileset.loaded_tileset = zero_tileset; 

}

static inline void
editor_tileset_set_terrain_uvs_count(
		editor_tileset_terrain *terrain,
		u32 new_count)
{
	terrain->base.mesh_count = new_count;
	u32 array_count = memory_dyarray_count(
			terrain->meshes);
	if(array_count < (new_count + 1))
	{
		new_count++;
		u32 new_amount_pushed = new_count - array_count;
		memory_dyarray_clear_and_push_amount(
			terrain->meshes,
				new_amount_pushed);
	}
}
static inline void
editor_tileset_set_wall_uvs_count(
		editor_tileset_wall *wall ,
		u32 new_count)
{
	wall->base.uvs_count = new_count;
	u32 array_count = memory_dyarray_count(
			wall->uvs);
	if(array_count < (new_count + 1))
	{
		new_count++;
		u32 new_amount_pushed = new_count - array_count;
		memory_dyarray_clear_and_push_amount(
			wall->uvs,
				new_amount_pushed);
	}
}


static inline void
editor_tileset_load_image(
		s_editor_state *editor_state,
		game_resource_attributes *loaded_image)
{
	s_game_editor *game_editor = &editor_state->editor;

	if(loaded_image) //loaded_image.key.index != assets_WHITE_TEXTURE_INDEX)
	{
		game_editor->tileset.tileset_texture = loaded_image;
		Assert(game_editor->tileset.tileset_texture);
	}
}

static inline editor_tileset_terrain *
editor_tileset_add_terrain(s_game_editor *game_editor,
		u32 meshes_count)
{
	editor_tileset_terrain *new_terrain = 0;
	if(game_editor->tileset.terrain_count < game_editor->tileset.terrain_max)
	{
		editor_name_chunks_addf(&game_editor->tileset.terrain_names,
				"Terrain_%d", game_editor->tileset.terrain_count);
		new_terrain = game_editor->tileset.terrain + game_editor->tileset.terrain_count;

		new_terrain->meshes = memory_dyarray_create(game_editor->tileset.terrain_meshes_zone,
				model_mesh,
				meshes_count + 1,
				1);
		//push one
		memory_dyarray_push(new_terrain->meshes);
		new_terrain->base.mesh_count = 1;

		game_editor->tileset.terrain_count++;
	}

	return(new_terrain);
}

static inline editor_tileset_wall *
editor_tileset_add_wall(s_game_editor *game_editor,
		u32 meshes_count)
{
	editor_tileset_wall *new = 0;
	if(game_editor->tileset.wall_count < game_editor->tileset.wall_max)
	{
		editor_name_chunks_addf(&game_editor->tileset.wall_names,
				"wall_%d", game_editor->tileset.wall_count);
		new = game_editor->tileset.walls + game_editor->tileset.wall_count;

		new->uvs = memory_dyarray_create(game_editor->tileset.terrain_meshes_zone,
				model_mesh,
				meshes_count + 1,
				1);
		//push one
		memory_dyarray_push(new->uvs);
		new->base.uvs_count = 1;

		game_editor->tileset.wall_count++;
	}

	return(new);
}
//array_of_chunk_pointers->chunk->name

static inline editor_tileset_terrain *
editor_tileset_add_new_terrain(s_game_editor *game_editor)
{
	editor_tileset_terrain *new_terrain = editor_tileset_add_terrain(
			game_editor,
			2);
	return(new_terrain);
}

static inline editor_tileset_autoterrain *
editor_tileset_add_autoterrain(s_game_editor *game_editor, u32 extra_layers)
{
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	stream_data *info_stream = &game_editor->info_stream;
	editor_tileset_autoterrain *new_autoterrain = 0;
	//better than crashing...
	if(tileset_editor->autoterrain_count < tileset_editor->autoterrain_max)
	{
		new_autoterrain = tileset_editor->autoterrains + tileset_editor->autoterrain_count;
		//for every autoterrain
		u32 indices_capacity_per_autoterrain = indices_capacity_per_AUTOTERRAIN;
		//push indices
		new_autoterrain->indices = memory_dyarray_create(
				tileset_editor->terrain_meshes_zone,
				u16,
				48 * (extra_layers + 1),
				48);
		memory_dyarray_clear_all(new_autoterrain->indices);
		memory_dyarray_push_amount(new_autoterrain->indices,
				indices_capacity_per_autoterrain * (extra_layers + 1));
		//set default indices capacity
		new_autoterrain->base.capacity = 16;
		new_autoterrain->base.extra_layers = extra_layers;
//		tileset_editor->autoterrain_indices_count += indices_capacity_per_autoterrain;
		//clear pushed indices to 0
//		memory_clear(
//				tileset_editor->autoterrain_indices + tileset_editor->autoterrain_indices_count,
//				indices_capacity_per_autoterrain);
		//add name
		editor_name_chunks_addf(
				&tileset_editor->autoterrain_names,
				"Autoterrain %u",
				tileset_editor->autoterrain_count);
		tileset_editor->autoterrain_count++;
	}
	else
	{
		stream_pushf(
				info_stream,
				"-- Tileset editor : The maximum of autoterrain_count got reached. Please reset and allocate more memory next time!.");
		//log
	}
	return(new_autoterrain);
}

static inline void
editor_tileset_remove_autoterrain(s_game_editor *game_editor, u32 index)
{
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	stream_data *info_stream = &game_editor->info_stream;
	if(index < tileset_editor->autoterrain_count)
	{
		editor_tileset_autoterrain *autoterrain = tileset_editor->autoterrains + index;
		memory_dyarray_delete(autoterrain->indices);

		//shift autoterrains array
		memory_shift_array_l(
				tileset_editor->autoterrains,
				index,
				tileset_editor->autoterrain_count,
				tileset_autoterrain);
		//remove name
		editor_name_chunks_remove(
				&tileset_editor->autoterrain_names,
				index);
		tileset_editor->autoterrain_count--;
	}
	else
	{
		stream_pushf(
				info_stream,
				"-- Tileset editor : there was an attemp to delete an autoterrain at %u, but the count is %u",
				index, tileset_editor->autoterrain_count);
	}
}

static void
editor_tileset_set_autoterrain_layers(
		editor_tileset_autoterrain *autoterrain, u32 extra_layers)
{
	if(extra_layers > autoterrain->base.extra_layers)
	{
		u32 push_amount = extra_layers - autoterrain->base.extra_layers;
		memory_dyarray_clear_and_push_amount(autoterrain->indices, push_amount * indices_capacity_per_AUTOTERRAIN);
	}
	else
	{
		u32 removed_amount = autoterrain->base.extra_layers - extra_layers;
		memory_dyarray_remove_amount(autoterrain->indices, removed_amount * indices_capacity_per_AUTOTERRAIN);
	}
	autoterrain->base.extra_layers = extra_layers;
	if(autoterrain->editor_selected_layer > extra_layers)
	{
		autoterrain->editor_selected_layer = extra_layers;
	}
}

static inline void
editor_tileset_remove_terrain(s_game_editor *game_editor, u32 index)
{
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	if(tileset_editor->terrain_count &&
	   index < tileset_editor->terrain_count)
	{
		editor_tileset_terrain *terrain_to_remove = tileset_editor->terrain + index;
		memory_dyarray_delete(
				terrain_to_remove->meshes);
		//remove from dynamic array
		memory_shift_array_l(
				tileset_editor->terrain,
				index,
				tileset_editor->terrain_count,
				editor_tileset_terrain);
		//make sure the autoterrains point to the correct terrain
		for(u32 a = 0; a < tileset_editor->autoterrain_count; a++)
		{
			editor_tileset_autoterrain *autoterrain = tileset_editor->autoterrains + a;
			for(u32 l = 0; l < (u32)autoterrain->base.extra_layers + 1; l++)
			{
				for(u32 i = 0; i < autoterrain->base.capacity; i++)
				{
					u32 i_index = i + (autoterrain->base.capacity * l);
					u16 *autoterrain_index = memory_dyarray_get(autoterrain->indices, i_index);
					if(*autoterrain_index >= index)
					{
						(*autoterrain_index)--;
					}
				}
			}
		}
				
		//remove name
		editor_name_chunks_remove(
				&tileset_editor->terrain_names, index);
		tileset_editor->terrain_count--;
	}
}

static inline void
editor_tileset_remove_selected_terrain(s_game_editor *game_editor)
{
	editor_tileset_remove_terrain(game_editor, 
			game_editor->tileset.selected_terrain_index);
}

static void
editor_tileset_load_new(
		s_editor_state *editor_state,
		game_resource_attributes *loaded_resource)
{
	s_game_editor *game_editor = &editor_state->editor;
	platform_api *platform  = editor_state->platform;
	stream_data *info_stream = &game_editor->info_stream;
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	//to load dependencies
	game_assets *game_asset_manager = editor_state->editor_assets;
	u8 *path_and_name = loaded_resource->path_and_name;
	//specify errors
	b32 type_error = loaded_resource->type != asset_type_tileset;
	b32 not_loaded_error = loaded_resource->asset_key == 0;
	b32 invalid_file_error = 0;

	b32 error = type_error || not_loaded_error;

	if(!error)
	{
		stream_pushf(info_stream, "Loading tileset file at %s", path_and_name);
		editor_wr wr = editor_wr_begin_read(
				&game_editor->area,
				platform,
				path_and_name);
		if(wr.file.handle)
		{


			//reset the current editing tileset
			editor_tileset_new(game_editor);
			//set tileset to edit
			tileset_editor->editing_tileset = loaded_resource;
			//load dependency image
			if(loaded_resource->tileset.texture)
			{
				tileset_editor->tileset_texture = loaded_resource->tileset.texture->attributes;
			}
			else
			{
				stream_pushf(info_stream,
						"WARNING!. The tileset image could not be loaded. A white texture"
						"will be used instead. Please select a valid image or the game will not be"
						"able to use this tileset");
			}

			world_tileset *loading_tileset = &loaded_resource->asset_key->tileset;
			//copy the resource contents
			//terrain
			u32 uvs_index_at = 0;
			for(u32 t = 0; t < loading_tileset->terrain_count; t++)
			{
				s_tileset_terrain *terrain = loading_tileset->terrain + t;

				editor_tileset_terrain *editor_terrain = editor_tileset_add_terrain(
						game_editor, terrain->mesh_count);
				editor_terrain->base = *terrain;
				//meshes
				for(u32 m = 0; m < terrain->uvs_count; m++)
				{
					//get mesh and copy
					model_mesh *terrain_mesh = memory_dyarray_get(editor_terrain->meshes, m);
					*terrain_mesh = loading_tileset->meshes[uvs_index_at];
					uvs_index_at++;
				}
			}
			
			//autoterrains
			u32 copying_indices_index = 0;
			for(u32 a = 0; a < loading_tileset->autoterrain_count; a++)
			{
				tileset_autoterrain *loading_autoterrain = loading_tileset->autoterrains + a;
				//add new autoterrain
				editor_tileset_autoterrain *new_autoterrain = 
					editor_tileset_add_autoterrain(game_editor,
							loading_autoterrain->extra_layers);
				new_autoterrain->base.capacity = loading_autoterrain->capacity;
				//autoterrain indices
				u32 capacity = loading_autoterrain->capacity;
				u32 extra_layers = loading_autoterrain->extra_layers;
				for(u32 l = 0; l < extra_layers + 1; l++)
				{
					for(u32 ai = 0; ai < capacity; ai++)
					{
						u32 indices_editor_index = ai + (l * indices_capacity_per_AUTOTERRAIN);
						u16 *autoterrain_index = memory_dyarray_get(new_autoterrain->indices, indices_editor_index);
						(*autoterrain_index) = loading_tileset->autoterrain_indices[copying_indices_index++];
					}
				}
			}
			//walls
			for(u32 w = 0; w < loading_tileset->wall_count; w++)
			{
				tileset_wall *wall = loading_tileset->walls + w;
				editor_tileset_wall *editor_wall = editor_tileset_add_wall(
						game_editor, wall->uvs_count);
				//copy base data
				editor_wall->base = *wall;
				//load uvs
				for(u32 u = 0; u < wall->uvs_count; u++)
				{
					model_mesh *editor_wall_uvs = memory_dyarray_get(editor_wall->uvs, u);
					model_mesh *uvs = loading_tileset->meshes + wall->uvs_at + u;
					*editor_wall_uvs = *uvs;
				}
			}

			//load editor data
			//load terrain names
			ppse_tileset_header_new *tileset_file_header = editor_wr_read_struct(&wr, ppse_tileset_header_new);
			u32 names_line = tileset_file_header->line_to_names;
			//go to the names line
			editor_wr_read_to_line(&wr, names_line);

			editor_wr_set_name_chunks(
					&wr,
					&tileset_editor->terrain_names,
					info_stream);
			editor_wr_set_name_chunks(
					&wr,
					&tileset_editor->autoterrain_names,
					info_stream);
			editor_wr_end(&wr);
			

		}
		else
		{
			error = 1;
			invalid_file_error = 1;
		}
	}
	if(error)
	{
		stream_pushf(info_stream, "ERROR! when loading the tileset file \"%s\" !", path_and_name);
		if(type_error)
		{
			stream_pushf(info_stream, "- The type isn't a tileset type", path_and_name);
		}
		if(invalid_file_error)
		{
			stream_pushf(info_stream, "- Its file could not be opened for modifying");
		}
	}
}
static void
editor_tileset_save_new(s_editor_state *editor_state)
{
	s_game_editor *game_editor = &editor_state->editor;
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	platform_api *platform  = editor_state->platform;

	stream_data *info_stream = &game_editor->info_stream;

	u8 *path_and_name = tileset_editor->editing_tileset->path_and_name;

	world_tileset *tileset = &game_editor->tileset.loaded_tileset;
	if(!tileset_editor->editing_tileset)
	{
		stream_pushf(info_stream, "editor_tileset_save was called, but there wasn't a valid tileset resource loaded.");
		return;
	}
	u8 *image_path_and_name = tileset_editor->tileset_texture ? 
		tileset_editor->tileset_texture->path_and_name : "";
	editor_wr wr = editor_wr_begin_write(
			&game_editor->area,
			platform,
			platform_file_op_create_new,
			path_and_name);

	if(wr.file.handle)
	{
		world_tileset *tileset = &game_editor->tileset.loaded_tileset;

		u32 terrain_count = game_editor->tileset.terrain_count;

		ppse_tileset_header_new *tileset_header = editor_wr_put_header(&wr, ppse_tileset_header_new);

		tileset_header->header.signature = ppse_tileset_SIGNATURE;
		tileset_header->header.version = ppse_tileset_version;
		tileset_header->terrain_count = terrain_count;
		tileset_header->autoterrain_count = tileset_editor->autoterrain_count;
		tileset_header->wall_count = tileset_editor->wall_count;
		//terrain composite resource line
		editor_wr_put_line(&wr);
		{
			//TODO: Write names
			u32 mesh_count = tileset_header->mesh_count;

			stream_pushf(
					info_stream,
					"Saving tileset at %s with %d terrain and %u meshes!",
					path_and_name,
					terrain_count,
					mesh_count);
			//write image name
			tileset_header->line_to_image_source = (u8)wr.current_line_number;
			tileset_header->header.offset_to_composite_resources = wr.data_offset;
			tileset_header->header.composite_resource_count = 1;

			editor_wr_write_composite_resource_header(
					&wr,
					image_path_and_name);

		}

		//terrain data line
		editor_wr_put_line(&wr);
		{
			tileset_header->line_to_terrain_data = (u8)wr.current_line_number;

			//save tileset terrain
			for(u32 t = 0; t < terrain_count; t++)
			{
				editor_tileset_terrain *terrain = game_editor->tileset.terrain + t;
				ppse_tileset_terrain *file_terrain = editor_wr_put_struct(&wr, ppse_tileset_terrain);
				//fill data
				file_terrain->mesh_count = terrain->base.mesh_count;
				file_terrain->shape = terrain->base.shape;
				file_terrain->terrain_group = terrain->base.terrain_group;
				file_terrain->use_wall = terrain->base.use_wall && 
					terrain->base.wall_index < tileset_editor->wall_count;
				file_terrain->wall_index = terrain->base.wall_index;
				file_terrain->capacity = terrain->base.capacity;

				//increase the total uvs count
				tileset_header->mesh_count += file_terrain->mesh_count;

				stream_pushf(
						info_stream,
						"Saving terrain with %d meshes!",
						file_terrain->mesh_count);
				//save terrain's geometry
				for(u32 s = 0;
						s < terrain->base.mesh_count;
						s++)
				{
					ppse_mesh *file_mesh = editor_wr_put_struct(&wr, ppse_mesh);
					model_mesh *terrain_mesh = memory_dyarray_get(
							terrain->meshes, s);
					file_mesh->m = *terrain_mesh;
				}
			}
		}
		//save autoterrain and indices
		editor_wr_put_line(&wr);
		{
			tileset_header->line_to_autoterrain_data = (u8)wr.current_line_number;
			for(u32 a = 0; a < tileset_editor->autoterrain_count; a++)
			{
				editor_tileset_autoterrain *autoterrain = tileset_editor->autoterrains + a;
				ppse_tileset_autoterrain *file_autoterrain = editor_wr_put_struct(&wr, ppse_tileset_autoterrain);

				//fill data
				file_autoterrain->extra_layers = autoterrain->base.extra_layers;
				file_autoterrain->capacity = autoterrain->base.capacity;
				file_autoterrain->terrain_group = autoterrain->base.terrain_group;
				stream_pushf(info_stream, "Saving autoterrain %u with capacity of %u",
						a,
						autoterrain->base.capacity);
				//add capacity to the header in order to calculate the total size when loading
				u32 total_indices_saved = autoterrain->base.capacity + (autoterrain->base.extra_layers * autoterrain->base.capacity);
				tileset_header->autoterrain_indices_capacity += total_indices_saved;
				for(i32 l = 0; l < autoterrain->base.extra_layers + 1; l++)
				{
					//indices go after the header
					for(u32 i = 0; i < autoterrain->base.capacity; i++)
					{
						u16 index = *(u16 *)memory_dyarray_get(autoterrain->indices,
								i + (l * indices_capacity_per_AUTOTERRAIN));
						editor_wr_write_struct(&wr, u16, &index);
						stream_pushf(info_stream, "%u. Saved autoterrain index %u",
								i,
								index);
					}
				}
			}
		}
		//save walls
		editor_wr_put_line(&wr);
		{
			tileset_header->line_to_walls = wr.current_line_number;
			for(u32 w = 0; w < tileset_editor->wall_count; w++)
			{
				editor_tileset_wall *wall = tileset_editor->walls + w;
				ppse_tileset_wall *file_wall = editor_wr_put_struct(&wr, ppse_tileset_wall);
				file_wall->uvs_count = wall->base.uvs_count;
				file_wall->repeat = wall->base.repeat;
				file_wall->extra_frames_x = wall->base.extra_frames_x;
				file_wall->extra_frames_y = wall->base.extra_frames_y;
				//increase the total uvs count
				tileset_header->mesh_count += file_wall->uvs_count;
				//save wall uvs
				for(u32 u = 0; u < wall->base.uvs_count; u++)
				{
					ppse_tileset_wall_uvs *file_uvs = editor_wr_put_struct(&wr, ppse_tileset_wall_uvs);
					model_mesh *uvs = memory_dyarray_get(wall->uvs, u);
					file_uvs->uv0 = uvs->uv0;
					file_uvs->uv1 = uvs->uv1;
					file_uvs->uv2 = uvs->uv2;
					file_uvs->uv3 = uvs->uv3;
				}
			}
		}
		//all names
		editor_wr_put_line(&wr);
		{
			tileset_header->line_to_names = (u8)wr.current_line_number;

			stream_pushf(info_stream, "- Saving terrain names...");
			editor_wr_write_name_chunks(
					&wr,
					&tileset_editor->terrain_names,
					info_stream);
			stream_pushf(info_stream, "- Saving autoterrain names...");
			editor_wr_write_name_chunks(
					&wr,
					&tileset_editor->autoterrain_names,
					info_stream);
			stream_pushf(info_stream, "- Saving wall names...");
			editor_wr_write_name_chunks(
					&wr,
					&tileset_editor->wall_names,
					info_stream);
		}
		stream_pushf(info_stream, "Saved tileset file to %s", path_and_name);

	}
	else
	{
		stream_pushf(info_stream, "Error saving tileset file to %s", path_and_name);
	}
	editor_wr_end(&wr);

}


static void
editor_tileset_update_render(s_editor_state *editor_state,
		game_renderer *gameRenderer,
		editor_input *editor_input,
		f32 dt)
{
	struct s_tileset_editor *tileset_editor = &editor_state->editor.tileset;

//	vec3 fixed_camera_position = {0};
//	vec3 fixed_camera_rotation = {0, 0, 0.5f};
}

static void
editor_tileset_update_render_ui(s_editor_state *editor_state,
		                        game_renderer *gameRenderer,
								editor_input *editor_input,
								f32 dt)
{

	s_game_editor *game_editor = &editor_state->editor;
	struct s_tileset_editor *tileset_editor = &game_editor->tileset;
	game_ui *ui = editor_state->ui;
	game_assets *game_asset_manager = editor_state->editor_assets;
	stream_data *info_stream = &game_editor->info_stream;

	//initial data
	world_tileset *tileset = &game_editor->tileset.loaded_tileset;
	if(tileset_editor->editing_tileset)
	{
		tileset_editor->tileset_texture = tileset_editor->editing_tileset->tileset.texture ?
			tileset_editor->editing_tileset->tileset.texture->attributes : 0;
	}


	u32 open_load_tileset_image_panel     = 0;
	u32 remove_selected_tileset_animation = 0;

	if(tileset->tileSize < 8)
	{
		tileset->tileSize = 8;
	}

	//
	// tileset editor panel
	// 

	f32 tileset_editor_panel_w = gameRenderer->os_window_width;
	f32 tileset_editor_panel_h = gameRenderer->os_window_height;
	u32 selectedTile         = game_editor->tileset.selected_tileset_tile_index;
	u32 createDefaultTileset = 0;
	b32 remove_selected_terrain = 0;

	b32 new_clicked = 0;
	b32 save_clicked = 0;
	b32 load_clicked = 0;
	b32 save_as_clicked = 0;
	ui_id terrain_import_popup_id = ui_id_from_string("terrain_import_popup");

	render_texture *tileset_texture = tileset_editor->tileset_texture ?
		&tileset_editor->tileset_texture->asset_key->image : assets_get_white_texture(game_asset_manager);


	ui_node *top_bar_node = 0;
	ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
	{
		top_bar_node = ui_create_node(ui, node_background | node_clickeable, 0);
	}
	
	//new, save, load buttons
	ui_set_parent(ui, top_bar_node)
	{
		ui_set_row(ui) ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
		{
			new_clicked = ui_button(ui, "New##new_tileset");
			ui_space_specified(ui, 4.0f, 1.0f);
			save_clicked = ui_button(ui, "Save##save_tileset");
			ui_space_specified(ui, 4.0f, 1.0f);
			save_as_clicked = ui_button(ui, "Save as##save_as_tileset");
			ui_space_specified(ui, 4.0f, 1.0f);
			load_clicked = ui_button(ui, "Load##load_tileset");
			ui_space_ppct(ui, 1.0f, 0.0f);
			if(tileset_editor->editing_tileset)
			{
				ui_text(ui, tileset_editor->editing_tileset->path_and_name);
			}
		}
	}

	ui_push_width(ui, ui_size_percent_of_parent(1.0f, 0.0f));
	ui_push_height(ui, ui_size_percent_of_parent(1.0f, 0.0f));
	ui_node *label_node = ui_label(ui, "Tileset widgets");
	ui_pop_width(ui);
	ui_pop_height(ui);

	ui_set_parent(ui, label_node)
	{
		ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
		{
			ui_set_wh(ui, ui_size_text(2.0f, 1.0f))
			{
				open_load_tileset_image_panel = 
					ui_button(ui, "Load image");
				ui_set_text_color(ui, V4(255, 255, 0, 255))
				{
					if(ui_button(ui, "Quick load image"))
					{

						editor_tileset_load_image(
								editor_state,
								er_look_for_resource(game_editor, "data/images/tiles_0_8x8.png"));
					}
				}
			}
		}

		ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
		{
			ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
			{
				ui_text(ui, "Loaded image source:");
				ui_set_text_color(ui, V4(0xFF, 0xFF, 0x00, 0xff))
				{
					if(tileset_editor->tileset_texture)
					{
						ui_text(ui, tileset_editor->tileset_texture->path_and_name);
					}
				}
			}
		}


		enum tileset_editor_tab{
			tileset_editor_source_image,
			tileset_editor_terrain,
			tileset_editor_autoterrain,
			tileset_editor_walls
		};
		u32 tab_index = 0;
		b32 clicked_any_tab = 0;
		ui_tab_group(ui, &tab_index, "tileset editor tabs")
		{
			clicked_any_tab |= ui_tab(ui, "Source image");
			clicked_any_tab |= ui_tab(ui, "Terrain");
			clicked_any_tab |= ui_tab(ui, "Autoterrain");
			clicked_any_tab |= ui_tab(ui, "Walls");

		}

		switch(tab_index)
		{
			case tileset_editor_source_image:
				{
					ui_set_wh(ui, ui_size_specified(512, 1.0f))
					{
						vec2 uv0;
						vec2 uv1;
						vec2 uv2;
						vec2 uv3;
						render_fill_uvs_counter_cw(&uv0, &uv1, &uv2, &uv3);
						ui_image_uvs(ui,
								tileset_texture,
								uv0,
								uv1,
								uv2,
								uv3
								);
					}
				}break;
			case tileset_editor_terrain: //tiles
				{

					//"terrain" or "brushes"
					ui_push_row(ui, 0, 0);
					ui_set_h_ppct(ui, 0.6f, 1.0f)
					ui_set_w_ppct(ui, 0.5f, 1.0f)
					{
						ui_node *brushes_label = ui_label(ui, 0);
						ui_set_parent(ui, brushes_label)
						{
							ui_push_disable_if(ui, game_editor->tileset.tileset_texture == 0);
							ui_node *brushes_top_bar;
							ui_set_height(ui, ui_size_sum_of_children(1.0f)) ui_set_w_ppct(ui, 1.0f, 1.0f)
							{
								brushes_top_bar = ui_label(ui, 0);
							}

							ui_set_parent(ui, brushes_top_bar) 
							{
								ui_space_specified(ui, 2.0f, 1.0f);
								ui_set_row(ui) ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
								{
									ui_space_specified(ui, 2.0f, 1.0f);
									if(ui_button(ui, "+#add_terrain"))
									{
										editor_tileset_add_new_terrain(game_editor);
									}
									ui_push_disable_if(ui, 
											game_editor->tileset.terrain_count == 0);
									ui_space_specified(ui, 4.0f, 1.0f);
									if(ui_button(ui, "x#remove_terrain"))
									{
										remove_selected_terrain = 1;
									}
									if(ui_button(ui, "Add for 16"))
									{
										//if(tileset_editor->terrain_count + 16 < tileset_editor->terrain_max)
										//{
										//	for(u32 t = 0; t < 16; t++)
										//	{
										//		editor_tileset_add_new_terrain(game_editor);
										//	}
										//}
										ui_popup_open(ui, 60, 60, terrain_import_popup_id);
										tileset_editor->adding_tile_amount = 16;
										editor_tileset_reset_adding_uvs(game_editor);
									}
									if(ui_button(ui, "Add for 48"))
									{
										ui_popup_open(ui, 60, 60, terrain_import_popup_id);
										tileset_editor->adding_tile_amount = 48;
										editor_tileset_reset_adding_uvs(game_editor);
									}
									ui_pop_disable(ui);
									ui_space_ppct(ui, 1.0f, 0.0f);
									ui_set_w_em(ui, 26.0f, 1.0f)
									{
										ui_textf(ui, "Terrain count/capacity: {%u/%u}",
												tileset_editor->terrain_count,
												tileset_editor->terrain_max);
									}
								}
								ui_space_specified(ui, 2.0f, 1.0f);
							}
							ui_pop_disable(ui);

							//list below
							ui_node *brushes_box = 0;
							ui_set_wh_ppct(ui, 1.0f, 0.0f)
							{
								brushes_box = ui_box_with_scroll(ui, "Brushes");
							}
							ui_set_parent(ui, brushes_box)
							{

#if 1
								//display list with images
								f32 img_terrain_size = 32.0f;
								ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_specified(ui, img_terrain_size + 8, 1.0f)
								{
									for(u32 t = 0;
											t < game_editor->tileset.terrain_count;
											t++)
									{

										ui_node *selection_box = 0;
										ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, img_terrain_size + 4, 1.0f)
										{
											editor_tileset_terrain *editor_terrain = tileset_editor->terrain + t;
											b32 selected =
												tileset_editor->selected_terrain_index == t;
											selection_box = ui_selectable_boxf(ui, selected, "selectable_terrain%u", t); 
											selection_box->padding_x = 2;
											selection_box->padding_y = 2;
											ui_set_parent(ui, selection_box)
											{
												ui_set_row(ui)
												{
													//image
													ui_set_wh_specified(ui, img_terrain_size, 1.0f)
													{
														if(tileset_editor->tileset_texture)
														{
															uvs *first_uvs = &((model_mesh *)memory_dyarray_get(editor_terrain->meshes, 0))->uvs.uvs;
															ui_image_uvs(ui,
																	&tileset_editor->tileset_texture->asset_key->image,
																	first_uvs->uv0,
																	first_uvs->uv1,
																	first_uvs->uv2,
																	first_uvs->uv3
																	);
														}
														else
														{
															ui_space_specified(ui, 32.0f, 1.0f);
														}
													}
													//text
													ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
													{
														ui_text(ui, tileset_editor->terrain_names.chunks[t].name);
													}
												}
											}
										}
										//set if clicked
										if(ui_node_mouse_l_up(ui, selection_box))
										{
											tileset_editor->selected_terrain_index = t;
										}
									}
								}
#endif
							}
						}

#if 1
						ui_content_box_be(ui,
								"Brush properties")
						{
							if(game_editor->tileset.terrain_count)
							{
								game_editor->tileset.selected_terrain_index = MIN(
										(i32)game_editor->tileset.selected_terrain_index, game_editor->tileset.terrain_count - 1);
								editor_tileset_terrain *editor_selected_terrain = 
									game_editor->tileset.terrain + game_editor->tileset.selected_terrain_index;
								s_tileset_terrain *selected_terrain = 
									&game_editor->tileset.terrain[game_editor->tileset.selected_terrain_index].base;
								//REMOVE
								//<--- REMOVE
								ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f)) ui_set_w_em(ui, tileset_editor->terrain_names.length, 1.0f) ui_set_row(ui)
								{
									ui_set_w_text(ui, 4.0f, 1.0f)
										ui_text(ui, "Name");

									ui_input_text(
											ui,
											0,
											editor_tileset_selected_terrain_name(tileset_editor),
											tileset_editor->terrain_names.length - 1,
											"selected_terrain_name##input");
								}

								ui_push_row(ui, 0, 0); ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "capacity");
									ui_id at_capacity_popup_id = ui_id_from_string(
											"t_capacity");
									{
										//select capacity context menu
										u32 current_capacity = editor_selected_terrain->base.capacity;
										ui_set_w_em(ui, 6.0f, 1.0f)
										if(ui_drop_down_beginf(ui,
												at_capacity_popup_id,
												"%u##t_capacity_dd", current_capacity))
										{
											ui_set_w_em(ui, 3.0f, 1.0f)
												ui_set_h_text(ui, 4.0f, 1.0f)
												{
													u8 c1 = ui_selectable(ui, current_capacity == 1, "1#t_capacity");
													u8 c16 = ui_selectable(ui, current_capacity == 16, "16#t_capacity");
													u8 c48 = ui_selectable(ui, current_capacity == 48, "48#t_capacity");
													if(c1)
													{
														editor_selected_terrain->base.capacity = 1;
														ui_popup_close(ui, at_capacity_popup_id);
													}
													else if(c16)
													{
														editor_selected_terrain->base.capacity = 16;
														ui_popup_close(ui, at_capacity_popup_id);
													}
													else if(c48)
													{
														editor_selected_terrain->base.capacity = 48;
														ui_popup_close(ui, at_capacity_popup_id);
													}
												}
										}
										ui_drop_down_end(ui);
									}
								}
								ui_pop_row(ui);
								
								ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
								{
									u8 *shape_types[terrain_shapes_count] =
									{
										"cube",
										"slope_floor_t",
										"slope_floor_b",
										"slope_floor_r",
										"slope_floor_l",
										"slope_wall_only_tl",
										"slope_wall_only_tr",
										"slope_wall_only_bl",
										"slope_wall_only_br",
										"slope_wall_tl",
										"slope_wall_tr",
										"slope_wall_bl",
										"slope_wall_br",
									};
									u8 *preview = "INVALID";
									if(selected_terrain->shape < terrain_shapes_count)
									{
										preview = shape_types[selected_terrain->shape];
									}
									ui_textf(ui, "Shape: %s", preview);
									ui_spinner_u16(ui, 1, 0, terrain_shapes_count, &selected_terrain->shape, 0, "Terrain_shape");
								}
								ui_set_wh_text(ui, 4.0f, 1.0f) ui_set_row(ui)
								{
									ui_text(ui, "terrain_group:");
									ui_spinner_u32(ui, 1, 0, 256, &selected_terrain->terrain_group, 0, "terrain_group_terrain");
								}
								ui_set_w_em(ui, 12.0f, 1.0f)
									ui_set_h_em(ui, 2.0f, 1.0f)
									{
										u32 new_wall_uvs_count = selected_terrain->mesh_count;
										if(ui_spinner_u32(
													ui,
													1,
													1,
													10,
													&new_wall_uvs_count,
													ui_text_input_confirm_on_enter,
													"wall_mesh_uvs_count"))
										{
											editor_tileset_set_terrain_uvs_count(
													editor_selected_terrain,
													new_wall_uvs_count);
										}
									}
								//walls
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "wall");
									u8 *preview = "-";
									if(selected_terrain->use_wall && selected_terrain->wall_index < tileset_editor->wall_count)
									{
										preview = tileset_editor->wall_names.chunks[selected_terrain->wall_index].name;
									}
									//drop down
									ui_id drop_down_id = ui_id_from_string("terrain_select_wall_dd");
									ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
									if(ui_drop_down_beginf(ui, drop_down_id, "%s##terrain_select_wall_", preview)) ui_extra_flags(ui, node_text_centered)
									{
										//don't use any walls if clicked on this option
										if(ui_selectable(ui, 0, "-##use_no_wall_terrain"))
										{
											selected_terrain->use_wall = 0;
										}
										for(u32 w = 0; w < tileset_editor->wall_count; w++)
										{
											b32 active = selected_terrain->use_wall && selected_terrain->wall_index == w;
											if(ui_selectablef(ui, active, "%s##terrain_wall_index%u", tileset_editor->wall_names.chunks[w].name, w))
											{
												selected_terrain->use_wall = 1;
												selected_terrain->wall_index = w;
											}
										}
									}
									ui_drop_down_end(ui);
								}

								u32 open_selectable_uvs_panel = 0;
								//reserve one for the top
								for(i32 m = 0;
										m < selected_terrain->mesh_count;
										m++)
								{
									model_mesh *mesh = memory_dyarray_get(
											editor_selected_terrain->meshes, m);

									ui_set_wh(ui, ui_size_specified(60, 1.0f))
									{
										if(ui_button_image_uvf(
													ui,
													tileset_texture,
													mesh->uv0,
													mesh->uv1,
													mesh->uv2,
													mesh->uv3,
													"selected_terrain %d",
													m))
										{
											game_editor->tileset.selected_terrain_uvs_index = m;
											game_editor->tileset.selecting_terrain_uvs = !game_editor->tileset.selecting_terrain_uvs;
											ui_open_panel(ui, "mesh frame uvs");
										}
									}

								}
								//make sure
								game_editor->tileset.selecting_terrain_uvs &= editor_selected_terrain->base.mesh_count;
								game_editor->tileset.selecting_terrain_uvs &= game_editor->tileset.tileset_texture != 0;
								if(game_editor->tileset.selected_terrain_uvs_index >
										editor_selected_terrain->base.mesh_count)
								{
									game_editor->tileset.selected_terrain_uvs_index = editor_selected_terrain->base.mesh_count;
								}
								model_mesh *selected_uvs = memory_dyarray_get(
										editor_selected_terrain->meshes, game_editor->tileset.selected_terrain_uvs_index);
								//select terrain uvs

								if(ui_window_begin(
											ui,
											ui_panel_flags_borders | ui_panel_flags_move | ui_panel_flags_init_closed,
											60,
											60,
											500,
											500,
											//&game_editor->tileset.selecting_terrain_uvs,
											"mesh frame uvs"))
								{

									vec2 *uvs[] = {
										&selected_uvs->uv0,
										&selected_uvs->uv1,
										&selected_uvs->uv2,
										&selected_uvs->uv3
									};
									ui_set_wh_ppct(ui, 1.0f, 0.0f)
									{

										ui_image_selection_data selection_data = ui_image_selection_begin(ui,
												tileset_texture,
												&tileset_editor->image_selection_down,
												&tileset_editor->image_selection_hot,
												&tileset_editor->image_selection_zoom,
												"Terrain uvs selection");
										{
											ui_image_selection_grid(ui,
													selection_data,
													14,
													14);
											ui_image_selection_uvs(ui,
													selection_data,
													1,
													&selected_uvs->uv0,
													&selected_uvs->uv1,
													&selected_uvs->uv2,
													&selected_uvs->uv3
													);
											u32 tw = 0;
											u32 th = 0;
											et_fill_capacity_data(editor_selected_terrain->base.capacity, &tw, &th);


											if(editor_selected_terrain->base.capacity > 1)
											{
												u8 maps16[] = {
													1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 0
												};
												u8 maps48[48] = 
												{1, 1, 1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1, 1, 1,
													1, 1, 1, 1, 1, 1, 1, 1,
												};
												u8 *maps = editor_selected_terrain->base.capacity == 16 ? 
													maps16 : maps48;
												u32 fx = 0;
												u32 fy = 0;
												u32 fw = 0;
												u32 fh = 0;
												render_fill_frames_from_uvs(
														tileset_texture->width,
														tileset_texture->height,
														selected_uvs->uv0,
														selected_uvs->uv1,
														selected_uvs->uv2,
														selected_uvs->uv3,
														&fx,
														&fy,
														&fw,
														&fh
														);
												vec4 color = {255, 0, 0, 255};
												for(u32 y = 0; y < th; y++)
												{
													for(u32 x = 0; x < tw; x++)
													{
														u32 f = x + (y * tw);
														if(maps[f])
														{
															u32 atx = (x * fw + fx) + 2 * x;
															u32 aty = (y * fh + fy) + 2 * y;
															ui_image_selection_draw_frames(ui, selection_data, atx, aty, fw, fh, color);
														}
													}
												}
											}
										}
										ui_image_selection_end(ui);
									}

								}
								ui_panel_end(ui);

							}
						}
#endif
					}
					ui_pop_row(ui);
#if 0
					ui_set_w_specified(ui, 600.0f, 1.0f) ui_set_h_ppct(ui, 0.2f, 0.0f)
				    
					ui_content_box_be_ex(
							ui, node_scroll_y, "mesh with images")
					{
						f32 img_terrain_size = 32.0f;
						f32 x_advance = 0;
						u32 t = 0;

						 ui_set_h_specified(ui, img_terrain_size * 5, 1.0f) ui_set_w_specified(ui, img_terrain_size, 1.0f)

						 //terrain selectable images
						 while(t < game_editor->tileset.terrain_count)
						 {
							 ui_set_row(ui) while(x_advance < 600 && t < game_editor->tileset.terrain_count)
							 {
								 //get data
								 editor_tileset_terrain *e_terrain = tileset_editor->terrain + t;

								 //create selectable
								 ui_node *selectable = ui_create_nodef(
										 ui, node_border, "terrain_new_selectable%u", t);

								 ui_set_column(ui)
								 {
									 //put images
									 ui_set_parent(ui, selectable)
									 {
										 for(u32 u = 0; u < e_terrain->base.mesh_count; u++)
										 {
											 model_mesh *mesh = memory_dyarray_get(
													 e_terrain->meshes, u);

											 ui_set_wh(ui, ui_size_specified(img_terrain_size, 1.0f))
											 {
												 ui_image_uvs(ui,
														 tileset_texture,
														 mesh->uv0,
														 mesh->uv1,
														 mesh->uv2,
														 mesh->uv3
														 );
											 }
										 }
									 }
								 }
								 ui_space_specified(ui, 4.0f, 1.0f);
								 t++;
								 x_advance += img_terrain_size + 4;
							 }
							 x_advance = 0;
						 }
					}
#endif
				}break;
			case tileset_editor_autoterrain:
				{
					ui_node *autoterrain_list_node;
					ui_node *autoterrain_data_node;
					ui_node *autoterrain_terrain_selection_node;
					ui_set_row(ui)
					{

						ui_set_h_ppct(ui, 0.7f, 1.0f)
						{
							ui_set_w_ppct(ui, 0.2f, 1.0f)
								autoterrain_list_node = ui_node_box(
										ui,
										"Autoterrain_list");
							ui_space_specified(ui, 4.0f, 1.0f);
							ui_set_w_ppct(ui, 1.0f, 0.0f)
								autoterrain_data_node = ui_node_box(
										ui,
										"Autoterrain_data");
							ui_space_specified(ui, 4.0f, 1.0f);
							ui_set_w_ppct(ui, 0.3f, 0.0f)
								autoterrain_terrain_selection_node = ui_node_box(
										ui,
										"at_terrain_selection");
							ui_space_specified(ui, 4.0f, 1.0f);
							//							ui_space_specified(ui, 200.0f, 1.0f);
						}
					}
					ui_set_parent(ui, autoterrain_list_node)
					{
						ui_set_wh_text(ui, 4.0f, 1.0f)
							ui_set_row(ui)
							{
								if(ui_button(ui, "+#add_autoterrain"))
								{
									editor_tileset_add_autoterrain(
											game_editor, 0);
								}
								ui_space_specified(
										ui, 4.0f, 1.0f);
								if(ui_button(ui, "x#remove_autoterrain"))
								{
									editor_tileset_remove_autoterrain(
											game_editor,
											game_editor->tileset.selected_autoterrain_index);
								}
							}
						//autoterrain selection
						for(u32 a = 0;
								a < tileset_editor->autoterrain_count;
								a++)
						{
							ui_set_w_ppct(ui, 1.0f, 1.0f)
								ui_set_h_text(ui, 4.0f, 1.0f)
								{
									u8 *autoterrain_name = editor_tileset_autoterrain_name(
											tileset_editor, a);
									ui_selectable_set_u16f(
											ui,
											&tileset_editor->selected_autoterrain_index,
											a,
											"%s##Autoterrain%u",
											autoterrain_name,
											a);
								}

						}
					}
					ui_set_parent(ui, autoterrain_data_node)
					{
						if(tileset_editor->autoterrain_count)
						{
							//fix index
							if(tileset_editor->selected_autoterrain_index >=
									tileset_editor->autoterrain_count)
							{
								tileset_editor->selected_autoterrain_index =
									tileset_editor->autoterrain_count - 1;
							}
							//now pick the editing autoterrain
							editor_tileset_autoterrain *autoterrain = 
								tileset_editor->autoterrains + tileset_editor->selected_autoterrain_index;

							ui_push_row(ui, 0, 0); ui_push_column(ui, 0, 0);
							{
								ui_id at_capacity_popup_id = ui_id_from_string(
										"at_capacity");
								ui_set_wh(ui, ui_size_sum_of_children(1.0f))
								{
									//select capacity context menu
									ui_context_menu(ui,
											at_capacity_popup_id)
									{
										ui_set_w_em(ui, 3.0f, 1.0f)
											ui_set_h_text(ui, 4.0f, 1.0f)
											{
												if(ui_selectable_set_u16(ui,
															&autoterrain->base.capacity, 
															16,
															"16##at_capacity16"))
												{
													ui_popup_close(ui, at_capacity_popup_id);
												}
												if(ui_selectable_set_u16(ui,
															&autoterrain->base.capacity, 
															indices_capacity_per_AUTOTERRAIN,
															"48##at_capacity48"))
												{
													ui_popup_close(ui, at_capacity_popup_id);
												}
											}
									}
								}
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_extra_flags_begin(ui,node_text | node_text_centered | node_border);
									ui_node *at_capacity_drop_down = ui_selectable_boxf(
											ui, 0, "Capacity : %u##at_capacity_dd", autoterrain->base.capacity);
									ui_extra_flags_end(ui);
									//open or close capacity popup
									if(ui_node_mouse_l_up(ui, at_capacity_drop_down))
									{
										ui_popup_open_or_close(
												ui,
												at_capacity_drop_down->region.x0,
												at_capacity_drop_down->region.y1,
												at_capacity_popup_id
												);
									}
									ui_text(ui, "Terrain group:");
									ui_set_h_em(ui, 2.0f, 1.0f)
									ui_spinner_u16(
											ui, 1, 0, 255, &autoterrain->base.terrain_group, 0, "autoterrain_terrain_group");
								}
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Extra layers");
									ui_set_w_em(ui, 3.0f, 1.0f)
									{
										u16 extra_layers = autoterrain->base.extra_layers;
										ui_spinner_u16(ui, 1, 0, 3, &extra_layers, 0, "autoterrain_extra_layers");
										if(extra_layers != autoterrain->base.extra_layers)
										{
											editor_tileset_set_autoterrain_layers(autoterrain, extra_layers);
										}
									}
									ui_text(ui, "Layer");
									ui_set_w_em(ui, 3.0f, 1.0f)
									{
										u16 extra_layers = autoterrain->base.extra_layers;
										ui_spinner_u32(ui, 1, 0, autoterrain->base.extra_layers,
												&autoterrain->editor_selected_layer,
												0,
												"autoterrain_editor_selected_layer");
										if(extra_layers != autoterrain->base.extra_layers)
										{
											editor_tileset_set_autoterrain_layers(autoterrain, extra_layers);
										}
									}
								}

								ui_space_specified(ui, 4.0f, 1.0f);
								ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
								{
									ui_text(ui, "Indices");
								}
								ui_space_specified(ui, 4.0f, 1.0f);

								f32 tiles_image_sizes = 32.0f;
								if(autoterrain->base.capacity == 16)
								{
									//Buttons for easier editing
									ui_set_wh_soch(ui, 1.0f)
									{
										u8 *image_bit_values_text[9] = {
											"12", "14", "10",
											"13", "15", "11",
											"5", "7", "3"};
										u8 image_bit_values[9] = {
											12, 14, 10,
											13, 15, 11,
											5, 7, 3};
										u8 bit_values[] = {
											12, 14, 10, 4, 6, 2, 8, 0,
											13, 15, 11, 9,
											5, 7, 3, 1};
										u8 bit_rows[] = {
										1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 2,
										1, 1, 1, 0, 0, 0, 0, 0, 1, 2,
										1, 1, 1, 0, 0, 0, 0, 0, 1};
										//i16 bit_rows[] = {
										//12, 14, 10, -1, 4, 6, 2, -1, 8, 0, -2,
										//13, 15, 11, -1, -1, -1, -1, -1, 9, -2,
										//5, 7, 3, -1, -1, -1, -1, -1, 1};
										u32 bit_reads_c = ARRAYCOUNT(bit_rows);
										ui_set_row(ui) ui_set_wh(ui, ui_size_specified(tiles_image_sizes, 1.0f))
										{
											u32 put_index = 0;
											for(u32 b = 0; b < bit_reads_c; b++)
											{
												if(!bit_rows[b])
												{
													ui_space_specified(ui, tiles_image_sizes, 1.0f);
												}
												else if(bit_rows[b] == 1)
												{
													u32 bit = bit_values[put_index];
													b32 clicked = _terrain_tile_image_button(
															ui, 
															tileset_editor, 
															autoterrain,
															tileset_texture,
															bit_values[put_index],
															16,
															autoterrain->editor_selected_layer,
															"tile_with_edges");
													put_index++;
													if(clicked)
													{
														tileset_editor->sel_autoterrain_mask_value = bit;
													}
												}
												else if(bit_rows[b] == 2)
												{
													ui_pop_row(ui);
													ui_push_row(ui, 1, 1);
												}
											}
										}
									}
								}
								else if(autoterrain->base.capacity == indices_capacity_per_AUTOTERRAIN)
								{
#if 1
									//for 46:
									//1, 2, 4,
									//8, 0, 16
									//32, 64, 128

									// 208, 248,104
									// 214, 255,107
									//  22,  31, 11

									// 0,  64,  0,
									//16,  90,  8,
									// 0,   2,  0,
									//  ,    ,  64,    ,    ,
									//  , 208,  26, 104,    ,
									//16,  74,    ,  82,   8,
									//  ,  22,  88,  11,    ,
									//  ,    ,   2,    ,    ,
									//  ,    ,    ,    ,    ,


									u8 bit_values[200] = {
										0,   0,  64,  0,  0,   0,  64,   0,   0,   0, 208, 104,   0, 248, 104,   0, 208, 248, 104,   0,
										0,  16,  90,  8,  0, 208,  26, 104,   0,   0, 214, 123, 189, 222, 107,   0,  22,  95,  11,   0,
										0,   0,   2,  0, 16,  74,   0,  82,   8,   0,  22,  11,   0,  22,  11,   0,   0, 231,   0,   0,
										0,   0,   0,  0,  0,  22,  88,  11,   0,   0,   0,   0,   0,   0,   0,   0,   1, 250,   1,   0,
										1,   1,   0,  1,  1,   0,   2,   0,   0,   1,   1,   0,   1,   1,   0,   0,   1,   1,   1,   0,
										1,   1,   0,  1,  1,   0,   0,   0,   0,   1,   1,   1,   1,   1,   0,   0,   0,   0,   0,   0,
										1,   1,   1,  1,  1,   0,   1,   1,   0,   0,   1,   0,   1,   0,   0,   1,   1,   1,   1,   0,
										0,   1,   1,  1,  0,   0,   1,   1,   0,   1,   1,   1,   1,   1,   0,   1,   1,   1,   1,   0,
										0,   0,   0,  0,  0,   0,   1,   1,   0,   1,   1,   0,   1,   1,   0,   0,   0,   0,   0,   0,
										0,   0,   0,  0,  0,   0,   1,   1,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0
									};
									//width: 20
									u8 bit_rows[] = { 
										0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 2,
										0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 2,
										1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 2,
										0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2,
										1, 1, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0, 2,
										1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 1, 2,
										0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 0, 0, 1, 2,
										0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 0, 1, 1, 0, 0, 0, 1, 2,
										0, 0, 1, 1, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 2,
										0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2
									};
									i32 bit_w = 20;
									i32 bit_h = 11;


#else
									u16 bit_values[58] = {
										255, 1, 8, 10, 11, 16, 18, 22, 0,
										24, 26, 27, 30, 31, 64, 66, 72, 0,
										74, 75, 80, 82, 86, 88, 89, 91, 0,
										94, 95, 104, 106, 107, 120, 122, 123, 0,
										126, 127, 176, 178, 182, 216, 218, 219, 0,
										222, 223, 244, 246, 247, 254, 255, 0, 0,
									};
									u8 bit_rows[58] = { 
										1, 1, 1, 1, 1, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 1, 1, 1, 2,
										1, 1, 1, 1, 1, 1, 1, 1, 2,
									};
#endif
									//i16 bit_rows[] = {
									//12, 14, 10, -1, 4, 6, 2, -1, 8, 0, -2,
									//13, 15, 11, -1, -1, -1, -1, -1, 9, -2,
									//5, 7, 3, -1, -1, -1, -1, -1, 1};
									u32 bit_reads_c = 58;
									ui_set_row(ui) ui_set_wh(ui, ui_size_specified(tiles_image_sizes, 1.0f))
									{
										u32 put_index = 0;
										for(i32 y = 0; y < bit_h; y++)
										{
											for(i32 x = 0; x < bit_w; x++)
											{
												u32 b = x + (y * bit_w);
												if(!bit_rows[b])
												{
													ui_space_specified(ui, tiles_image_sizes, 1.0f);
												}
												else if(bit_rows[b] == 1)
												{
													u32 bit = read_bit_corners(bit_rows, x, y, bit_w, bit_h);
													ui_push_id_u32(ui, put_index);
													b32 clicked = _terrain_tile_image_button(
															ui, 
															tileset_editor, 
															autoterrain,
															tileset_texture,
															bit,
															48,
															autoterrain->editor_selected_layer,
															"tile_with_edges");
													ui_pop_id(ui);
													put_index++;
													if(clicked)
													{
														tileset_editor->sel_autoterrain_mask_value = 
															autoterrain_index_from_mask46(0, bit);
													}
												}
												else if(bit_rows[b] == 2)
												{
													ui_pop_row(ui);
													ui_push_row(ui, 1, 1);
												}
											}
										}
#if 0
										for(u32 b = 0; b < bit_reads_c; b++)
										{
											if(!bit_rows[b])
											{
												ui_space_specified(ui, tiles_image_sizes, 1.0f);
											}
											else if(bit_rows[b] == 1)
											{
												u32 bit = bit_values[b];
												ui_push_id_u32(ui, put_index);
												b32 clicked = _terrain_tile_image_button(
														ui, 
														tileset_editor, 
														autoterrain,
														tileset_texture,
														bit,
														autoterrain->editor_selected_layer,
														"tile_with_edges");
												ui_pop_id(ui);
												put_index++;
												if(clicked)
												{
													tileset_editor->sel_autoterrain_mask_value = 
														autoterrain_index_from_mask46(0, bit);
												}
											}
											else if(bit_rows[b] == 2)
											{
												ui_pop_row(ui);
												ui_push_row(ui, 1, 1);
											}
										}
#endif
									}
								}
								ui_pop_column(ui);
							}
							ui_pop_row(ui);

						}
					}
					ui_set_parent(ui, autoterrain_terrain_selection_node)
					{
						editor_tileset_autoterrain *autoterrain = 0;

						if(tileset_editor->autoterrain_count)
						{
							autoterrain = 
								tileset_editor->autoterrains + tileset_editor->selected_autoterrain_index;
							if(tileset_editor->sel_autoterrain_mask_value >= autoterrain->base.capacity)
							{
								tileset_editor->sel_autoterrain_mask_value = 0;
							}

						//display data
						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_textf(ui, "sel_autoterrain_mask_value : %u",
									tileset_editor->sel_autoterrain_mask_value);
						}

						ui_node *t_list;
						ui_node *t_image_and_data;
						ui_set_row(ui) ui_set_w_ppct(ui, 0.5f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
						{
							t_list = ui_box_with_scroll(ui, "terrain list for autoterrain");
							t_image_and_data = ui_node_box(ui, "terrain data of autoterrain");
						}
						u16 autoterrain_mask_index = tileset_editor->sel_autoterrain_mask_value + (autoterrain->editor_selected_layer * indices_capacity_per_AUTOTERRAIN);
						u16 selected_autoterrain_bit_index = *(u16 *)memory_dyarray_get(autoterrain->indices, autoterrain_mask_index);
						
						u32 current_selected_terrain = autoterrain ?
							selected_autoterrain_bit_index :
							0;
						//show list
						ui_set_parent(ui, t_list)
						{
							u32 index_sent = current_selected_terrain;
								_list_of_terrain_with_images(ui, 
										tileset_editor, 
										tileset_texture,
										24.0f,
										&index_sent,
										"autoterrain_terrain_selection");
								u16 *index = memory_dyarray_get(autoterrain->indices, autoterrain_mask_index);
								*index = index_sent;
										
						}
						if(autoterrain) ui_set_parent(ui, t_image_and_data)
						{
							if(current_selected_terrain >= tileset_editor->terrain_count)
							{
								ui_textf(ui, "Invalid terrain selected!");
							}
							else
							{
								editor_tileset_terrain *e_terrain = tileset_editor->terrain + current_selected_terrain;
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Name:");
									ui_text(ui, editor_tileset_terrain_name(tileset_editor, current_selected_terrain));
									ui_text(ui, "Shape:");
									ui_text(ui, "Cube for now?");
								}
								//show top and walls
								for(u32 u = 0; u < e_terrain->base.mesh_count; u++)
								{
									model_mesh *uvs = memory_dyarray_get(
											e_terrain->meshes, u);
									ui_set_wh(ui, ui_size_specified(32.0f, 1.0f))
									{
										if(tileset_texture)
										{
											ui_image_uvs(ui,
													tileset_texture,
													uvs->uv0,
													uvs->uv1,
													uvs->uv2,
													uvs->uv3
													);
										}
										else
										{
											ui_label(ui, 0);
										}
									}
								}
							}
						}
						}
					}
				}break;
			case tileset_editor_walls:
				{
					ui_node *wall_list_node;
					ui_node *wall_properties_node;
					ui_set_row(ui) ui_set_w_ppct(ui, 0.5f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						ui_node *wall_list_node_parent = ui_create_node(ui, 0, 0);
						wall_properties_node = ui_node_box(ui, "Selected wall properties");
						ui_set_parent(ui, wall_list_node_parent)
						{
							//label to display data or add more walls
							ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
							{
								ui_node *wall_label_data = ui_label(ui, 0);
								ui_set_parent(ui, wall_label_data) ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									if(ui_button(ui, "+##add_wall"))
									{
										editor_tileset_add_wall(game_editor, 1);
									}
									ui_space_specified(ui, 4.0f, 1.0f);
									ui_push_disable_if(ui, !tileset_editor->selected_wall);
									ui_button(ui, "x##remove_wall");
									ui_pop_disable(ui);

									ui_space_ppct(ui, 1.0f, 0.0f);

									ui_text(ui, "Wall count/max: ");
									ui_textf(ui, "%u/%u", tileset_editor->wall_count, tileset_editor->wall_max);
								}
							}
							//list
							ui_set_wh_ppct(ui, 1.0f, 0.0f)
							{
								wall_list_node = ui_box_with_scroll(ui, "te_wall_list");
								ui_set_parent(ui, wall_list_node)
								{
									f32 img_terrain_size = 32.0f;
									for(u32 w = 0; w < tileset_editor->wall_count; w++)
									{
										ui_node *selection_box = 0;
										ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, img_terrain_size + 4, 1.0f)
										{
											editor_tileset_wall *editor_wall = tileset_editor->walls + w;
											b32 selected = tileset_editor->wall_is_selected &&
												tileset_editor->selected_wall_index == w;
											selection_box = ui_selectable_boxf(ui, selected, "selectable_wall%u", w); 
											selection_box->padding_x = 2;
											selection_box->padding_y = 2;
											ui_set_parent(ui, selection_box)
											{
												ui_set_row(ui)
												{
													//image
													ui_set_wh_specified(ui, img_terrain_size, 1.0f)
													{
														if(tileset_editor->tileset_texture)
														{
															model_mesh *first_uvs = memory_dyarray_get(editor_wall->uvs, w);
															ui_image_uvs(ui,
																	&tileset_editor->tileset_texture->asset_key->image,
																	first_uvs->uv0,
																	first_uvs->uv1,
																	first_uvs->uv2,
																	first_uvs->uv3
																	);
														}
														else
														{
															ui_space_specified(ui, 32.0f, 1.0f);
														}
													}
													//text
													ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
													{
														ui_text(ui, tileset_editor->wall_names.chunks[w].name);
													}
												}
											}
											//select if clicked
											if(ui_node_mouse_l_up(ui, selection_box))
											{
												tileset_editor->selected_wall_index = w;
												tileset_editor->wall_is_selected = 1;
											}
										}
									}
								}
							}
						}
						//selected wall properties
						ui_set_parent(ui, wall_properties_node) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_text(ui, "Properties");
							if(tileset_editor->wall_is_selected)
							{
								editor_tileset_wall *editor_wall = tileset_editor->walls + tileset_editor->selected_wall_index;

								ui_text(ui, "Name");
								ui_set_w_em(ui, 20.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
								{
									ui_input_text(ui,
											0,
											tileset_editor->wall_names.chunks[tileset_editor->selected_wall_index].name,
											tileset_editor->wall_names.length - 1,
											"selected_wall_name");
								}
								ui_text(ui, "uvs");
								ui_set_w_em(ui, 4.0f, 1.0f) ui_set_h_text(ui, 4.0f ,1.0f)
								{
								//	u32 uvs_count = editor_wall->base.uvs_count;
								//	ui_spinner_u32(ui, 1, 1, 10, &uvs_count, 0, "editor_wall_uvs_count");
								//	if(uvs_count != editor_wall->base.uvs_count)
								//	{
								//		editor_tileset_set_wall_uvs_count(editor_wall, uvs_count);
								//	}
								}
								ui_set_row(ui)
								{
									ui_text(ui, "extra_frames_x");
									ui_set_w_em(ui, 6.0f, 1.0f)
									{
										ui_spinner_u16(ui, 1, 0, 10, &editor_wall->base.extra_frames_x, 0, "editor_wall_extra_frames_x");
									}
								}
								ui_set_row(ui)
								{
									ui_text(ui, "extra_frames_y");
									ui_set_w_em(ui, 6.0f, 1.0f)
									{
										ui_spinner_u16(ui, 1, 0, 10, &editor_wall->base.extra_frames_y, 0, "editor_wall_extra_frames_y");
									}
								}
	
								for(i32 m = 0;
										m < editor_wall->base.uvs_count;
										m++)
								{
									model_mesh *mesh = memory_dyarray_get(
											editor_wall->uvs, m);

									ui_set_wh(ui, ui_size_specified(60, 1.0f))
									{
										if(ui_button_image_uvf(
													ui,
													tileset_texture,
													mesh->uv0,
													mesh->uv1,
													mesh->uv2,
													mesh->uv3,
													"selected_wall_uvs %d",
													m))
										{
											game_editor->tileset.selected_wall_uvs_index = m;
											ui_open_panel(ui, "wall frame uvs");
										}
									}

								}

								//uvs selection panel
								if(ui_window_begin(
											ui,
											ui_panel_flags_borders | ui_panel_flags_move | ui_panel_flags_init_closed,
											60,
											60,
											500,
											500,
											"wall frame uvs"))
								{
									model_mesh *selected_uvs = memory_dyarray_get(
											editor_wall->uvs,
											game_editor->tileset.selected_wall_uvs_index);

									vec2 *uvs[] = {
										&selected_uvs->uv0,
										&selected_uvs->uv1,
										&selected_uvs->uv2,
										&selected_uvs->uv3
									};
									ui_set_wh_ppct(ui, 1.0f, 0.0f)
									{

										ui_image_selection_data selection_data = ui_image_selection_begin(ui,
												tileset_texture,
												&tileset_editor->image_selection_down,
												&tileset_editor->image_selection_hot,
												&tileset_editor->image_selection_zoom,
												"uvs selection");
										{
											ui_image_selection_grid(ui,
													selection_data,
													14,
													14);
											ui_image_selection_uvs(ui,
													selection_data,
													1,
													&selected_uvs->uv0,
													&selected_uvs->uv1,
													&selected_uvs->uv2,
													&selected_uvs->uv3
													);
										}
										ui_image_selection_end(ui);
									}

								}
								ui_panel_end(ui);
							}


						}
					}
				}break;
		}
		if(clicked_any_tab)
		{
		//	ui_close_panel(ui, "mesh frame uvs");
		}
	}
	//tile importing popup
	ui_popup(ui, terrain_import_popup_id) ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_next_nodes_interaction_only_begin(ui);
		ui_node *box = 0;
		ui_set_w_specified(ui, 1300, 1.0f) ui_set_h_specified(ui, 700, 1.0f)
		{
			//title bar
			ui_set_h_em(ui, 2.0f, 1.0f)
			{
				ui_node *title_bar = ui_label(ui, 0);
				title_bar->padding_x = 4;
				title_bar->padding_y = 4;
				ui_set_parent(ui, title_bar)
				{
					ui_text(ui, "Add terrains for autotiles");
				}
			}
			box = ui_create_node(ui, node_border | node_clickeable | node_background, 0);
			box->padding_x = 6;
			box->padding_y = 6;
		}
		
		ui_set_parent(ui, box) ui_extra_flags(ui, node_border)
		{
			//image selection
			u8 *frames_x = 0;
			u8 *frames_y = 0;
			u8 *maps = 0;
			u8 map_w = 0;
			u8 map_h = 0;
			ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 760, 0.0f)
			{
				//select the starting frame for this selection
				u32 frames_total = 0;

				u8 maps16[] = {
				1, 1, 1, 1, 1, 1,
				1, 1, 1, 1, 1, 0,
				1, 1, 1, 1, 0, 0,
				0, 0, 0, 1, 0, 0
				};
				u8 maps48[48] = 
				   {1, 1, 1, 1, 1, 1, 1, 1,
				    1, 1, 1, 1, 1, 1, 1, 1,
				    1, 1, 1, 1, 1, 1, 1, 1,
				    1, 1, 1, 1, 1, 1, 1, 1,
				    1, 1, 1, 1, 1, 1, 1, 1,
				    1, 1, 1, 1, 1, 1, 1, 1,
				   };

				ui_image_selection_data selection_data = ui_image_selection_begin(ui,
						tileset_texture,
						&tileset_editor->image_selection_down,
						&tileset_editor->image_selection_hot,
						&tileset_editor->image_selection_zoom,
						"terrain adding selection");
				{
					ui_image_selection_grid(ui,
							selection_data,
							14,
							14);
					ui_image_selection_uvs(ui,
							selection_data,
							1,
							&tileset_editor->adding_tile_uv0,
							&tileset_editor->adding_tile_uv1,
							&tileset_editor->adding_tile_uv2,
							&tileset_editor->adding_tile_uv3
							);
					//display frames for the auto uvs map
					if(tileset_editor->adding_tile_amount == 16)
					{
						map_w = 6;
						map_h = 3;
						maps = maps16;
					}
					else
					{
						map_w = 8;
						map_h = 6;
						maps = maps48;
					}
					vec4 color = {255, 0, 0, 255};
					u32 tz = 12;

					u32 fx = 0;
					u32 fy = 0;
					u32 fw = 0;
					u32 fh = 0;
					render_fill_frames_from_uvs(
							tileset_texture->width,
							tileset_texture->height,
							tileset_editor->adding_tile_uv0,
							tileset_editor->adding_tile_uv1,
							tileset_editor->adding_tile_uv2,
							tileset_editor->adding_tile_uv3,
							&fx,
							&fy,
							&fw,
							&fh
							);
					for(u32 y = 0; y < map_h; y++)
					{
						for(u32 x = 0; x < map_w; x++)
						{
							u32 f = x + (y * map_w);
							if(maps[f])
							{
								u32 atx = (x * fw + fx) + 2 * x;
								u32 aty = (y * fh + fy) + 2 * y;
								ui_image_selection_draw_frames(ui, selection_data, atx, aty, fw, fh, color);
							}
						}
					}
				}
				ui_image_selection_end(ui);

			}
			ui_set_row(ui)
			{

				if(ui_button(ui, "ok#add_terrains"))
				{
					ui_popup_close(ui, terrain_import_popup_id);
					//add the amount specified
					u32 fx = 0;
					u32 fy = 0;
					u32 fw = 0;
					u32 fh = 0;
					render_fill_frames_from_uvs(
							tileset_texture->width,
							tileset_texture->height,
							tileset_editor->adding_tile_uv0,
							tileset_editor->adding_tile_uv1,
							tileset_editor->adding_tile_uv2,
							tileset_editor->adding_tile_uv3,
							&fx,
							&fy,
							&fw,
							&fh
							);
					//only add them if the frames are at least of size 1
					if(fw && fh)
					{
						for(u32 y = 0; y < map_h; y++)
						{
							for(u32 x = 0; x < map_w; x++)
							{
								if(maps[x + (y * map_w)])
								{
									//add terrain
									editor_tileset_terrain *new_terrain = editor_tileset_add_terrain(game_editor, 1);
									//get uvs
									model_mesh *uvs = memory_dyarray_get(new_terrain->meshes, 0);
									//get frames and map them to the uvs
									u32 atx = (x * fw + fx) + 2 * x;
									u32 aty = (y * fh + fy) + 2 * y;
									render_fill_uvs_from_frames(tileset_texture->width, tileset_texture->height,
											atx, aty, fw, fh,
											&uvs->uv0,
											&uvs->uv1,
											&uvs->uv2,
											&uvs->uv3
											);
								}
							}
						}
					}
				}
				ui_space_specified(ui, 4.0f, 1.0f);
				if(ui_button(ui, "cancel#add_terrains"))
				{
					ui_popup_close(ui, terrain_import_popup_id);
				}
			}
		}
		ui_next_nodes_interaction_only_end(ui);
	}




	if(open_load_tileset_image_panel)
	{
//		ui_explorer_set_path(ui, DATA_FOLDER("tilesets"));
		er_explorer_set_process(
				game_editor,
				er_explorer_load,
				"Load tileset image");
	}
	//loaded new image!
	if(editor_resource_explorer_process_completed(game_editor, "Load tileset image"))
	{
		u8 *image_path_and_name = game_editor->asset.explorer_selected_resource->path_and_name;
		game_resource_attributes *r = er_reference_resource(game_editor, image_path_and_name);

		editor_tileset_load_image(
				editor_state, r);
	}

	if(new_clicked)
	{
		editor_tileset_new(game_editor);
	}
	else if(load_clicked)
	{
		er_explorer_set_process(game_editor,
				er_explorer_load,
				"Load tileset to edit");
	}
	else if(save_clicked && tileset_editor->editing_tileset)
	{
		//create explorer process and save to file
		editor_tileset_save_new(editor_state);

	}
	else if(save_clicked && !tileset_editor->editing_tileset ||
			save_as_clicked)
	{
//		ui_explorer_set_path(ui, DATA_FOLDER("tilesets"));
		er_explorer_set_process(
				game_editor,
				er_explorer_save,
				"Save tileset as");
	}
	if(editor_resource_explorer_process_completed(game_editor,  "Save tileset as"))
	{
		u8 *savePath = editor_resource_explorer_output(game_editor);
		game_resource_attributes *new_editing_tileset = editor_resource_create_and_save(
				editor_state, asset_type_tileset, 1, savePath);
		if(new_editing_tileset)
		{
			tileset_editor->editing_tileset = new_editing_tileset;
			editor_tileset_save_new(editor_state);
			editor_resources_reimport(editor_state, tileset_editor->editing_tileset);
		}
	}
	else if(editor_resource_explorer_process_completed(game_editor, "Load tileset to edit"))
	{
		platform_api *platform = editor_state->platform;
		u8 *load_path = editor_resource_explorer_output(game_editor);
		game_resource_attributes *r = er_reference_resource(game_editor, load_path);
		editor_tileset_load_new(
				editor_state,
				r);
	}

	//remove_add
	if(remove_selected_terrain)
	{ 
		editor_tileset_remove_selected_terrain(game_editor);
	}

}
