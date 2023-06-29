#define editor_world_redraw(editor) editor->world.draw_locked = 0

#define _tileset_terrain_uvs(tileset, index) (tileset->meshes + (tileset->terrain[index].uvs_at_vertices_at))
#define ew_harea_model_add_PROCESS "Add model to harea"

static vec2
ew_harea_size(editor_map_harea *harea)
{
	vec2 result = {(f32)(harea->base.w * GAME_TILESIZE), (f32)(harea->base.h * GAME_TILESIZE)};
	return(result);
}
static void
editor_world_tile_indices(
		s_game_editor *editor,
		i32 *x,
		i32 *y,
		i32 *z)
{
	s_world_editor *world_editor = &editor->world;
	u32 ray_tile_index = world_editor->ray_tile_index;
	if(world_editor->ray_hits_tile)
	{
		i32 ray_tile_x = ray_tile_index % world_editor->map_w;
		i32 ray_tile_y = ray_tile_index / world_editor->map_w;
		i32 ray_tile_z = (i32)world_editor->ray_tile_position.z / GAME_TILESIZE;

		*x = ray_tile_x;
		*y = ray_tile_y;
		*z = ray_tile_z;
	}
}
static void
editor_world_update_render(s_editor_state *editor_state,
		                   game_renderer *game_renderer,
						   editor_input *editor_input,
						   f32 dt);
static inline void
editor_world_allocate(
		s_game_editor *game_editor)
{
	//
	//initialize world editor
	//
	//create and allocate fake world for the editor
	game_editor->world.dyarrays_area = memory_dyarray_area_create(&game_editor->area,
			MEGABYTES(1));
	game_editor->world.loaded_world = memory_area_push_struct(
			&game_editor->area,
			game_world);
	//tiles
	game_editor->world.map_terrain_max = U16MAX;
	game_editor->world.terrain = memory_area_clear_and_push_array(
			&game_editor->area,
			world_tile,
			game_editor->world.map_terrain_max);
	//entities
	game_editor->world.entity_max = 200;
	game_editor->world.entities = memory_area_clear_and_push_array(
			&game_editor->area,
			editor_map_entity,
			game_editor->world.entity_max);
	//tilesets
	game_editor->world.tilesets_max = editor_world_tileset_MAX;
	//tileset from game attributes
	game_editor->world.editing_map_tilesets = memory_area_clear_and_push_array(
			&game_editor->area,
			game_resource_attributes *,
			game_editor->world.tilesets_max);

	game_editor->world.models_max = 100;
	game_editor->world.models = memory_area_clear_and_push_array(
			&game_editor->area,
			game_resource_attributes *,
			game_editor->world.models_max);

	game_editor->world.per_frame_area = memory_area_create_from(&game_editor->area, KILOBYTES(256));
	//game_editor->world.rotation_apply = 0.0f;
	game_editor->world.camera_distance_from_cursor = 200.0f;
	game_editor->world.camera_position = V3(0, 0, 200);
	game_editor->world.frame_tile_size_x = 16;
	game_editor->world.frame_tile_size_y = 16;

	game_editor->world.cursor.tile_displacement = 16;

	game_editor->world.cursor_memory.selected_meshes_max = 100;
	game_editor->world.cursor_memory.hot_vertices_max   = 10;

	game_editor->world.history_max = 50;
	game_editor->world.history     = memory_area_push_array(&game_editor->area, editor_world_history_header, game_editor->world.history_max);

	game_editor->world.history_buffer_total = KILOBYTES(256);
	game_editor->world.history_buffer       = memory_area_push_size(&game_editor->area, game_editor->world.history_buffer_total);

	game_editor->world.tile_clipboard = memory_area_push_array(&game_editor->area, world_tile, editor_TILE_CLIPBOARD_MAX);

	game_editor->world.entity_tags = editor_name_chunks_allocate(
			&game_editor->area, game_editor->world.entity_max, 32);
	game_editor->world.harea_tags = editor_name_chunks_allocate(
			&game_editor->area, game_editor->world.entity_max, 64);
	game_editor->world.hareas = editor_array_allocate(&game_editor->area, sizeof(editor_map_harea), 100);


}

static inline world_tileset *
editor_world_get_tileset(
		s_game_editor *editor,
		u32 tileset_index)
{
	world_tileset *tileset = &editor->world.editing_map_tilesets[tileset_index]->asset_key->tileset;
	return(tileset);
}


static inline void
editor_world_change_map_size(
		s_game_editor *editor)
{
	u32 new_w = editor->world.new_map_w;
	u32 new_h = editor->world.new_map_h;
	editor->world.map_terrain_count = (new_w * new_h);
	editor->world.draw_locked = 0;
	if((new_w && new_h) && 
		new_w != editor->world.map_w ||
		new_h != editor->world.map_h)
	{
		u32 old_w = editor->world.map_w;
		u32 old_h = editor->world.map_h;
		u32 new_w = editor->world.new_map_w;
		u32 new_h = editor->world.new_map_h;

		editor->world.map_w = editor->world.new_map_w;
		editor->world.map_h = editor->world.new_map_h;
		//keep the tile positions on the array
		world_tile *tile_array = editor->world.terrain;

		world_tile zero_tile = {0};
		i32 min_w = old_w;
		i32 min_h = old_h;
		i32 max_w = new_w;
		i32 max_h = new_h;
		if(old_w > new_w)
		{
			min_w = new_w;
			max_w = old_w;
		}
		if(old_h > new_h)
		{
			min_h = new_h;
			max_h = old_h;
		}
		if(new_w > old_w)
		{
			for(i32 y = min_h - 1; y; y--)
			{
				for(i32 x = min_w - 1; x; x--)
				{
					u32 index = x + (y * min_w);
					u32 new_index = x + (y * max_w);
					tile_array[new_index] = tile_array[index];
					tile_array[index] = zero_tile;
				}
			}
		}
		else
		{
			u32 cursor_new = new_w;
			u32 cursor_old = old_w;

			for(u32 current_row = 0;
					cursor_old && current_row < new_h;
					current_row++)
			{
				for(u32 r = 0;
						r < new_w;
						r++)
				{
					tile_array[cursor_new + r] = tile_array[cursor_old + r];
				}
				//get one row down
				cursor_new += new_w;
				cursor_old += old_w; 
			}
		}

	}

}

static inline void
editor_world_shift_map(
		s_game_editor *editor)
{
	s_world_editor *world_editor = &editor->world;
	if(!world_editor->shift_x && !world_editor->shift_y)
	{
		return;
	}

	if(world_editor->resize_after_shift)
	{
		//first resize by the amount
		i32 new_w = world_editor->new_map_w + world_editor->shift_x;
		i32 new_h = world_editor->new_map_h + world_editor->shift_y;

		new_w = new_w <= 1 ? 1 : new_w;
		new_h = new_h <= 1 ? 1 : new_h;
		u32 total_terrain_needed = 
			new_w * new_h;
		//for now just return
		if(total_terrain_needed >= world_editor->map_terrain_max)
		{
			return;
		}
		//set the values before resizing
		world_editor->new_map_w = new_w; 
		world_editor->new_map_h = new_h; 
	}

	world_tile zero_tile = {0};
	if(world_editor->shift_x < 0)
	{
		i32 shift_x = -world_editor->shift_x;
		for(u32 y = 0; y < world_editor->map_h; y++)
		{
			u32 x = y * world_editor->map_w;
			i32 j = 0; 
			while(j + shift_x < world_editor->map_w)
			{
				world_editor->terrain[x] = world_editor->terrain[x + shift_x];
				world_editor->terrain[x + shift_x] = zero_tile;
				x++;
				j++;
			}
		}

	}
	if(world_editor->shift_y < 0)
	{
		i32 shift_rows = world_editor->shift_y * world_editor->map_w;
		i32 shift_y = -world_editor->shift_y;
		for(i32 y = 0;
				y + shift_y < world_editor->map_h;
				y++)
		{
			u32 x = 0;
			u32 row0 = y * world_editor->map_w;
			u32 row1 = (y + shift_y) * world_editor->map_w;
			while(x < world_editor->map_w)
			{
				u32 i = row0 + x;
				u32 j = row1 + x;
				world_editor->terrain[i] = world_editor->terrain[j];
				world_editor->terrain[j] = zero_tile;
				x++;
			}
		}
	}
	//change size after if the shift value is negative
	if(world_editor->resize_after_shift)
	{
		editor_world_change_map_size(
				editor);
	}

	if(world_editor->shift_x > 0)
	{
		i32 shift_amount = world_editor->shift_x + (world_editor->shift_y * world_editor->map_w);
		u32 tile_count = world_editor->map_w * world_editor->map_h;
		for(u32 y = 0; y < world_editor->map_h; y++)
		{
			u32 x = y * world_editor->map_w;
			i32 j = x + world_editor->map_w - 1;
			while((i32)j - (i32)world_editor->shift_x >= (i32)x)
			{
				world_editor->terrain[j] = world_editor->terrain[j - world_editor->shift_x];
				world_editor->terrain[j - world_editor->shift_x] = zero_tile;
				j--;
			}
		}
	}

	if(world_editor->shift_y > 0)
	{
		i32 shift_rows = world_editor->shift_y * world_editor->map_w;
		for(i32 y = world_editor->map_h - 1;
				(y - world_editor->shift_y) >= 0; y--)
		{
			u32 x = 0;
			u32 row = y * world_editor->map_w;
			while(x < world_editor->map_w)
			{
				u32 i = row + x;
				u32 j = row - shift_rows + x;
				world_editor->terrain[i] = world_editor->terrain[j];
				world_editor->terrain[j] = zero_tile;
				x++;
			}
		}
	}
	//move entities
	for(u32 e = 0; e < world_editor->entity_count; e++)
	{
		world_editor->entities[e].position.x += world_editor->shift_x * GAME_TILESIZE;
		world_editor->entities[e].position.y += world_editor->shift_y * GAME_TILESIZE;
	}

}

inline void
editor_world_delete_tile(s_game_editor *editor,
		                 u32 tile_index)
{
	world_tile *tile_array = editor->world.terrain;
	u32 map_terrain_count         = editor->world.map_terrain_count;
	u32 t = tile_index;
	//shift the terrain array to the left by one step
	while(t < map_terrain_count - 1)
	{
		world_tile tile_a = tile_array[t];
		tile_array[t]   = tile_array[t + 1];
		tile_array[t + 1] = tile_a;
		t++;
	}
	map_terrain_count--;
	editor->world.map_terrain_count = map_terrain_count;
}


static editor_map_entity * 
editor_world_add_entity(
		s_game_editor *editor,
		vec3 position)
{
	s_world_editor *world_editor = &editor->world;
	editor_map_entity *entity = 0;
	if(world_editor->entity_count < world_editor->entity_max)
	{
		entity = world_editor->entities + world_editor->entity_count;
		memory_clear(entity, sizeof(editor_map_entity));
		entity->model = 0;
		entity->position = position;
		world_editor->entity_count++;
	}
	return(entity);
}

static void
editor_world_delete_entity(
		s_game_editor *editor,
		u32 entity_index)
{
	s_world_editor *world_editor = &editor->world;
	if(entity_index < world_editor->entity_count)
	{
		editor_map_entity *ent = world_editor->entities + entity_index;
		//shift left
		for(i32 e = entity_index;
				e < world_editor->entity_count - 1;
				e++)
		{
			world_editor->entities[e] = world_editor->entities[e + 1];
		}
		world_editor->entity_count--;
	}
}

static editor_map_harea *
editor_world_add_harea(s_game_editor *editor)
{
	s_world_editor *world_editor = &editor->world;
	editor_map_harea *editor_harea = 0;
	if(world_editor->hareas.count < world_editor->hareas.max)
	{
		editor_harea = 
			editor_array_add_clear(&world_editor->hareas, editor_map_harea);
		editor_harea->base.w = 1;
		editor_harea->base.h = 1;
	}
	return(editor_harea);
}

static editor_map_harea *
editor_world_add_harea_at(
		s_game_editor *game_editor,
		f32 x,
		f32 y,
		f32 z,
		u32 w,
		u32 h)
{
	editor_map_harea *editor_harea = editor_world_add_harea(game_editor);
	if(editor_harea)
	{
		editor_harea->base.x = x;
		editor_harea->base.y = y;
		editor_harea->base.z = z;
		editor_harea->base.w = w;
		editor_harea->base.h = h;
	}
	return(editor_harea);
}

static editor_map_harea *
editor_world_add_animated_tiles_harea(
		s_game_editor *game_editor,
		f32 x,
		f32 y,
		f32 z)
{
	editor_map_harea *editor_harea = editor_world_add_harea_at(
			game_editor, x, y, z, 1, 1);
	editor_harea->base.type = 0;
	editor_harea->tiles = memory_dyarray_create(game_editor->world.dyarrays_area,
			world_harea_tile,
			1,
			1);
	editor_harea->animations = memory_dyarray_create(game_editor->world.dyarrays_area,
			sprite_animation,
			2,
			2);
	memory_dyarray_set_count(editor_harea->tiles, 1);
	return(editor_harea);
}

static editor_map_harea *
editor_world_add_model_harea(
		s_game_editor *game_editor,
		f32 x,
		f32 y,
		f32 z)
{
	editor_map_harea *editor_harea = editor_world_add_harea_at(
			game_editor, x, y, z, 1, 1);
	editor_harea->base.type = 1;
	//tiles
	editor_harea->tiles = memory_dyarray_create(game_editor->world.dyarrays_area,
			world_harea_tile,
			1,
			1);
	//model data to place points
	editor_harea->models = memory_dyarray_create(game_editor->world.dyarrays_area,
			editor_map_harea_model,
			2,
			2);
	//points placed
	memory_dyarray_create_safe(
			game_editor->world.dyarrays_area,
			editor_harea->points,
			editor_map_harea_point,
			50, 50);
	//external models
	mdy_create_safe(game_editor->world.dyarrays_area, 
			editor_harea->external_models, 
			editor_harea_model_slot,
			2, 2);
	memory_dyarray_set_count(editor_harea->tiles, 1);
	return(editor_harea);
}

static void
ew_generate_harea_poisson(
		s_game_editor *editor,
		editor_map_harea *harea)
{
	temporary_area ta = temporary_area_begin(&editor->area);
	{
		//reset points
		harea->point_count = 0;
		harea->generate_poisson = 0;
		editor->world.generate_harea_poisson = 0;
		memory_dyarray_reset(harea->points);
		random_series random_s = random_series_create(0);

		f32 sampling_radius = 8;
		f32 grid_wh = (sampling_radius / sqrt32(2));
		//create grid
		//the harea real size is the "canvaz"
		u32 harea_w = harea->base.w * GAME_TILESIZE;
		u32 harea_h = harea->base.h * GAME_TILESIZE;
		i32 grid_w = (i32)(harea_w / grid_wh);
		i32 grid_h = (i32)(harea_h / grid_wh);
		u32 grid_count = grid_w * grid_h;
		Assert(grid_count);
		i16 *grid = memory_area_push_array(&editor->area, i16, grid_count);
		//set the whole grid to "inactive"
		for(u32 g = 0; g < grid_count; g++) grid[g] = -1;
		//active points
		i16 *active_points = memory_area_push_array(&editor->area, i16, 10000);
		u16 active_points_count = 0;
		u16 active_points_max = 10000;
		//randomness weights
		u32 total_weight = 0;
		for(u32 m = 0; m < harea->model_count; m++)
		{
			editor_map_harea_model *model_data = 0;
			memory_dyarray_get_safe(harea->models, model_data, m);
			total_weight += model_data->weight;
		}
		//choose random point in the grid
		{
			harea->point_count++;
			active_points_count++;
			editor_map_harea_point *first_point = 0;
			memory_dyarray_push_safe(harea->points, first_point);
			memory_clear(first_point, sizeof(*first_point));
			first_point->p.x = random_get_f32_between(&random_s, 0, (f32)grid_wh);
			first_point->p.y = random_get_f32_between(&random_s, 0, (f32)grid_wh);
			u32 gx = (u32)(first_point->p.x / grid_wh);
			u32 gy = (u32)(first_point->p.y / grid_wh);
			u32 first_point_grid_index = (gx + (gy * grid_w));
			grid[first_point_grid_index] = 0;
			active_points[0] = 0;
		}
		while(active_points_count)
		{
			u32 active_points_count_b = active_points_count;
			for(u32 ap = 0; ap < active_points_count; ap++)
			{
				//get index from the active points list
				i16 parent_p_index = active_points[ap];
				editor_map_harea_point *harea_point = 0;
				//use the index
				memory_dyarray_get_safe(harea->points, harea_point, parent_p_index);
				vec2 parent_p = harea_point->p;
				b32 keep_sample = 0;
				u32 sample_count = 30;
				for(u32 p = 0; p < sample_count; p++)
				{
					//choose radius between r-r/2
					f32 radius = random_get_f32_between(&random_s,
							sampling_radius, sampling_radius * 2);
					//choose random angle
					f32 angle = random_get_f32_between(&random_s, 0, PI * 2);
					//get the final position of this point
					vec2 position = {cos32(angle), sin32(angle)};
					position.x *= radius;
					position.y *= radius;
					position = vec2_add(parent_p, position);
					//could be calculated once
					i32 rdx = (i32)((sampling_radius * 2) / grid_wh);
					i32 grid_x = (i32)(position.x / grid_wh);
					i32 grid_y = (i32)(position.y / grid_wh);
					if(grid_x > 0 && grid_x < grid_w &&
							grid_y > 0 && grid_y < grid_h)
					{
						//make sure this grid isn't "active"
						u32 grid_index = grid_x + (grid_y * grid_w);
						i16 grid_value = grid[grid_index];
						if(grid_value != -1)
						{
							continue;
						}

						b32 cancel_point = 0;
						//check neightbour points inside the bounds
						i32 ys = grid_y - rdx;
						i32 ye = grid_y + rdx;
						i32 xs = grid_x - rdx;
						i32 xe = grid_x + rdx;
						ys = ys < 0 ? 0 : ys;
						ye = ye > grid_h ? grid_h : ye;
						xs = xs < 0 ? 0 : xs;
						xe = xe > grid_w ? grid_w : xe;
						for(i32 iy = ys; iy < ye; iy++)
						{
							i32 index_y = iy;

							for(i32 ix = xs; ix < xe; ix++)
							{
								i32 index_x = ix;
								//check bounds
								index_x = index_x < 0 ? 0: index_x;

								u32 n_index = index_x + (index_y * grid_w);
								//don't compare to yourself
								if(n_index != grid_index)
								{
									//this grid isn't active anymore if its index is -1
									i16 n_grid_value = grid[n_index];
									//compare distances if a point is inside this grid
									if(n_grid_value != -1)
									{
										editor_map_harea_point *p1 = 0;
										memory_dyarray_get_safe(harea->points, p1, n_grid_value);
										//distances_squared
										f32 distance = vec2_inner_squared(vec2_sub(position, p1->p));
										if(distance < (sampling_radius * sampling_radius))
										{
											cancel_point = 1;
										}
									}
								}
							}
						}
						//add point if it's not closer to any
						if(!cancel_point)
						{
							u32 final_index = harea->point_count;

							editor_map_harea_point *point = 0;
							memory_dyarray_push_safe(harea->points, point);
							memory_clear(point, sizeof(*point));
							point->p = position;
							grid[grid_index] = final_index;
							//add this to the active points
							active_points[active_points_count] = final_index;
							harea->point_count++;
							active_points_count++;
							Assert(active_points_count < active_points_max);
							keep_sample = 1;
//							Assert(program->final_points_count < program->final_points_max);
							//choose a model for this point
							i32 weight = random_get_u32_between(&random_s, 0, total_weight);
							for(u32 m = 0; m < harea->model_count; m++)
							{
								editor_map_harea_model *model = 0;
								memory_dyarray_get_safe(harea->models, model, m);
								if(weight < model->weight)
								{
									point->model_index = model->model_index;
									break;
								}
								else
								{
									weight -= model->weight;
								}
								Assert(weight >= 0);
							}
							//since we added this points, we have to activate this grid cell
						}
					}
				}
				//remove this from the active list
				if(!keep_sample)
				{
					active_points[ap] = -1;
				}
			}
			//remove inactive points
			u32 ip = 0;
			while(ip < active_points_count)
			{
				//shift the array
				if(active_points[ip] == -1)
				{
					for(u32 i = ip; i < (u32)(active_points_count - 1); i++)
					{
						i32 copy = active_points[i];
						active_points[i] = active_points[i + 1];
						active_points[i + 1] = copy;
					}
					active_points_count--;
				}
				else
				{
					ip++;
				}
			}
		}
	}
	temporary_area_end(&ta);
}

static editor_map_harea_model *
ew_add_model_to_harea(
		s_game_editor *game_editor,
		editor_map_harea *harea,
		u8 *path_and_name)
{
	game_resource_attributes *r = er_look_for_resource(game_editor, path_and_name);
	editor_map_harea_model *hm = 0;
	editor_harea_model_slot *ext_r = 0;
	if(r)
	{
		u32 model_index = 0;
		b32 found_model = 0;
		//look for this model
		for(u32 e = 0; e < harea->external_model_count; e++)
		{
			editor_harea_model_slot *other_r = 0;
			memory_dyarray_get_safe(harea->external_models, other_r, e);
			//no need to add another model
			if(r == other_r->r)
			{
				model_index = e;
				ext_r = other_r;
				break;
			}
		}
		//not ofound
		if(!ext_r)
		{
			//look for a free slot
			for(u32 e = 0; e < harea->external_model_count; e++)
			{
				editor_harea_model_slot *other_r = 0;
				memory_dyarray_get_safe(harea->external_models,	other_r, e);
				//no need to add another model
				if(!other_r->local_ref_count)
				{
					model_index = e;
					ext_r = other_r;
					break;
				}
			}
			//push a new model slot
			if(!ext_r)
			{
				memory_dyarray_push_safe(harea->external_models, ext_r);
				memory_clear(ext_r, sizeof(*ext_r));
				model_index = harea->external_model_count++;
			}
			ext_r->r = er_reference_existing_resource(r);
		}
		ext_r->local_ref_count++;
		//make sure this model wasnt already added
		memory_dyarray_push_safe(
				harea->models, hm);
//		hm->model = r;
		hm->model_index = model_index;
//		hm->model = editor_resource_slot_ref(&game_editor->model.harea_model_slots, r);
		hm->radius = 1;
		hm->weight = 0;
		harea->model_count++;
	}
	return(hm);
}

static void
ew_remove_model_from_harea(
		s_game_editor *game_editor,
		editor_map_harea *harea,
		u32 index)
{
	if(index < harea->model_count)
	{
		editor_map_harea_model *hm = 0;
		//dereference model
		memory_dyarray_get_safe(harea->models, hm, index);
		//restart points to re-generate the poisson
		harea->point_count = 0;
		harea->generate_poisson = 1;
		game_editor->world.generate_harea_poisson = 1;
		//dereference model
		{
			editor_harea_model_slot *model = 0;
			memory_dyarray_get_safe(harea->external_models, model, hm->model_index);
			model->local_ref_count--;
			if(!model->local_ref_count)
			{
				er_dereference_resource(model->r);
				model->r = 0;
			}
			memory_dyarray_remove_at(harea->external_models, hm->model_index);
			for(u32 m = 0; m < harea->model_count; m++)
			{
				editor_map_harea_model *hmodel = 0;
				memory_dyarray_get_safe(harea->models, hmodel, m);
				if(hmodel->model_index > hm->model_index) hmodel->model_index--;
			}
			harea->external_model_count--;
		}
//		editor_resource_slot_deref(hm->model_slot);
//		er_dereference_resource(hm->model);
		//finally, remove
		memory_dyarray_remove_at(harea->models, index);
		harea->model_count--;

		if(game_editor->world.harea_model_selection.index == index)
		{
			game_editor->world.harea_model_selection.selected = 0;
		}
	}
}

static void
editor_world_remove_harea(
		s_game_editor *editor,
		u32 index)
{
	s_world_editor *world_editor = &editor->world;
	editor_map_harea *r_harea = editor_array_base(
			world_editor->hareas, editor_map_harea) + index;
	switch(r_harea->base.type)
	{
		//animations
		case 0:
			{
				memory_dyarray_delete(r_harea->tiles);
				memory_dyarray_delete(r_harea->animations);
			}
			//models
		case 1:
			{
				//remove models one by one in order to correctly dereference them
				//for(u32 m = 0; m < r_harea->model_count; m++)
				//{
				//	editor_map_harea_model *hm = 0;
				//	memory_dyarray_get_safe(r_harea->models, hm, m);
				//	//dereference model
//				//	er_dereference_resource(hm->model);
				//}
				for(u32 m = 0; m < r_harea->external_model_count; m++)
				{
					editor_harea_model_slot *model = 0;
					memory_dyarray_get_safe(r_harea->external_models, model, m);
					if(model->r)
					{
						er_dereference_resource(model->r);
					}
				}
				memory_dyarray_delete(r_harea->tiles);
				memory_dyarray_delete(r_harea->models);
				memory_dyarray_delete(r_harea->points);
				memory_dyarray_delete(r_harea->external_models);
			}
			break;
	}

	b32 success = editor_array_remove_and_shift(&world_editor->hareas, index);
}

static inline void
editor_world_set_model_to_entity(
		s_game_editor *editor,
		editor_map_entity *entity,
		game_resource_attributes *resource)
{
	s_world_editor *world_editor = &editor->world;
	if(!resource) return;
	//look if this model was already added, else add it
	b32 exists = 0;
	u32 index = 0;
	for(u32 m = 0; !exists && m < world_editor->models_count; m++)
	{
		if(world_editor->models[m] == resource)
		{
			exists = 1;
			index = m;
		}
	}

	if(!exists && world_editor->models_max > world_editor->models_count)
	{
		index = world_editor->models_count++;
		world_editor->models[index] = resource;
		entity->model = resource;
		entity->model_index = index;
	}
	else if(exists)
	{
		entity->model = resource;
		entity->model_index = index;
	}
}

static u32 
editor_world_add_tileset(s_game_editor *editor,
		game_resource_attributes *resource)

{
	if(!resource) return(0);

	//platform_api *platform;
	u32 success = 0;
	s_world_editor *world_editor = &editor->world;
	stream_data *info_stream = &editor->info_stream;

	if(world_editor->tilesets_count < world_editor->tilesets_max)
	{

		u32 t = 0;
		u32 already_added = 0;
		while(!already_added && t < world_editor->tilesets_count)
		{
			if(world_editor->editing_map_tilesets[t++] == resource)
			{
				already_added = 1;
			}
		}

		//add new tileset to the world if it's not added
		if(!already_added)
		{
			success = 1;
			world_editor->editing_map_tilesets[world_editor->tilesets_count] = resource;
			editor->world.tilesets_count++;

		}
	}
	else
	{
		stream_pushf(
				info_stream, "--- WARNING! a new tileset to add was requested, but the final count would exceed the maximum capacity! (%u)",
				world_editor->tilesets_max);
	}


	return(success);
}

static b32
editor_world_reference_and_add_tileset(s_game_editor *editor,
		u8 *path_and_name)
{
	game_resource_attributes *resource = er_reference_resource(editor, path_and_name);
	if(!resource) return(0);
	if(resource->type != asset_type_tileset)
	{
		er_dereference_resource(resource);
		return(0);
	}
	b32 success = editor_world_add_tileset(
			editor, resource);
	return(success);
}


static void
editor_world_remove_tileset(s_game_editor *editor,
		                    u32 tileset_index)
{

	u32 tileset_count = editor->world.tilesets_count;
	s_world_editor *world_editor = &editor->world;
	if(tileset_count && tileset_index < tileset_count)
	{
		//delete the tiles pointing to this one
		world_tile *tile_array = editor->world.terrain;
		u32 map_terrain_count = world_editor->map_w * world_editor->map_h;
		//fix index in terrains
		for(u32 t = 0; t < map_terrain_count; t++)
		{
			world_tile *terrain = world_editor->terrain + t;
			if(terrain->tileset_index == tileset_index)
			{
				terrain->tileset_index = 0;
				terrain->tileset_terrain_index = 0;
			}
			else if(terrain->tileset_index > tileset_index)
			{
				terrain->tileset_index--;
			}
		}
		//shift tileset array
		for(u32 a = tileset_index; a < (u32)(world_editor->tilesets_count - 1); a++)
		{
			world_editor->editing_map_tilesets[a] = 
				world_editor->editing_map_tilesets[a + 1];
		}
		

		world_editor->tilesets_count--;
	}
}

//DO THIS :)

typedef struct{
	vec3 position;
	vec3 size;
}cube_shape;
//gets the "origin" of the tile cube and size used for ray casting of displaying.
static inline cube_shape 
editor_world_get_tile_cube(
		s_game_editor *editor,
		u32 index)
{
	cube_shape result = {0};
	if(index < editor->world.map_terrain_count)
	{
		f32 tz = (f32)GAME_TILESIZE;
		f32 tz_h = tz * 0.5f;
		world_tile *tile = editor->world.terrain + index;
		result.size.x = tz;
		result.size.y = tz;
		u32 x = index % editor->world.map_w;
		u32 y = index / editor->world.map_w;
		result.position.x = x * tz + tz_h;
		result.position.y = y * tz + tz_h;

		if(tile->height > 0)
		{
			result.size.z = tz + (tile->height * tz);
			result.position.z = (tile->height * tz_h) - tz_h;
		}
		else
		{
			result.position.z =tile->height * tz;
			result.size.z = 0;
		}
	}
	return(result);
}

static vec3
editor_world_get_ray_tile_position(
		s_game_editor *editor)
{
	vec3 result = {0};
	return(result);
}

static inline void
editor_world_fill_selection_distances(
		s_game_editor *game_editor,
		i32 *start_x_ptr,
		i32 *start_y_ptr,
		i32 *end_x_ptr,
		i32 *end_y_ptr)
{

		u32 current_tile_index = game_editor->world.ray_tile_index;
		i32 number_x = game_editor->world.selection_amount_x;
		i32 number_y = game_editor->world.selection_amount_y;
		i32 selection_x_distance = 0;
		i32 selection_y_distance = 0;

		//get x and y coordinates of the current selected tile index
		u32 current_tile_x = current_tile_index % game_editor->world.map_w;
		u32 current_tile_y = current_tile_index / game_editor->world.map_w;
		//Make sure these stay within the bounds of the map!
		i32 limit_x = current_tile_x + number_x;
		i32 limit_y = current_tile_y + number_y;
		limit_x = limit_x < 0 ? 0 : limit_x > (i32)game_editor->world.map_w - 1 ? 
			      (i32)game_editor->world.map_w - 1 : limit_x;
		limit_y = CLAMP(limit_y, 0, (i32)game_editor->world.map_h - 1);

		i32 start_x = current_tile_x;
		i32 end_x = limit_x;

		i32 start_y = current_tile_y;
		i32 end_y = limit_y;
		//inver the values, depending on where this starts
		if(end_x < start_x)
		{
			start_x = limit_x;
			end_x = current_tile_x;
		}
		if(end_y < start_y)
		{
			start_y = limit_y;
			end_y = current_tile_y;
		}

		//fill values
		if(start_x_ptr)
		{
			*start_x_ptr = start_x;
		}
		if(end_x_ptr)
		{
			*end_x_ptr = end_x;
		}
		if(start_y_ptr)
		{
			*start_y_ptr = start_y;
		}
		if(end_y_ptr)
		{
			*end_y_ptr = end_y;
		}
}

//tile0 is the autoterrain
//mismo mask y misma altura
static b32
_check_autoterrain_layer(
		world_tile *tile0,
		world_tile *tile1,
		world_tileset *tileset,
		u32 *layer_index)
{
	u32 terrain0_group = tileset->autoterrains[tile0->autoterrain_index].terrain_group;
	u32 terrain1_group = tileset->terrain[tile1->tileset_terrain_index].terrain_group;
	b32 same_group = terrain0_group &&
		terrain0_group == terrain1_group;

	b32 same_autoterrain = tile1->is_autoterrain &&
		tile1->autoterrain_index == tile0->autoterrain_index &&
		tile1->height == tile0->height;

	if(tile1->autoterrain_layer > *layer_index)
	{
		(*layer_index) = tile1->autoterrain_layer;
	}

	return(same_autoterrain);
}

static b32
_check_autoterrain(
		world_tile *tile0,
		world_tile *tile1,
		world_tileset *tileset)
{
	u32 terrain0_group = tileset->autoterrains[tile0->autoterrain_index].terrain_group;
	u32 terrain1_group = tileset->terrain[tile1->tileset_terrain_index].terrain_group;
	b32 same_group = terrain0_group &&
		terrain0_group == terrain1_group;

	b32 same_autoterrain = tile1->is_autoterrain &&
		tile1->autoterrain_index == tile0->autoterrain_index &&
		tile1->height == tile0->height;
	return(same_group || same_autoterrain);
}

static void 
editor_world_read_and_fill_bit_corners(
		i32 bit_rows_x,
		i32 bit_rows_y,
		i32 bit_w,
		i32 bit_h,
		i32 start_x,
		i32 end_x,
		i32 start_y,
		i32 end_y,
		u8 *corners_read,
		world_tile *bit_rows,
		world_tile *tile,
		world_tileset *tileset
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
#define ew_index_1d(x, y, w) (x + (y * w))
			u32 corners_index = ew_index_1d(x_at_corner, y_at_corner, 3);
			u32 rows_index = ew_index_1d(x_at, y_at, bit_w);
			world_tile *tile1 = bit_rows + rows_index;
			corners_read[corners_index] = 
				tile1->height == tile->height &&
				tile1->tileset_terrain_index == tile->tileset_terrain_index &&
				tile1->tileset_index == tile->tileset_index;
		}
	}
}

static u32
editor_world_read_bit_corners(
		world_tile *bit_rows,
		world_tileset *tileset,
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
	//the current tile
	world_tile *tile = bit_rows + (bit_x + (bit_y * bit_w));
	s_tileset_terrain *terrain = tileset->terrain + tile->tileset_terrain_index;
	if(terrain->capacity < 16)
	{
		return(0);
	}
	u8 corners_read[9] = {0};
	{
		i32 x = bit_x;
		i32 y = bit_y;
		editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, -1, 1, 0, 0, corners_read, bit_rows, tile, tileset);
		editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, 0, 0, -1, 1, corners_read, bit_rows, tile, tileset);
		i32 x_s = -1;
		i32 x_e = 1;
		if(terrain->capacity != 16)
		{
			//top corners
			if(corners_read[tm])
			{
				editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, x_s, x_e, -1, -1, corners_read, bit_rows, tile, tileset);
			}
			x_s = -1;
			x_e = 1;
			//bottom corners
			if(corners_read[bm])
			{
				editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, x_s, x_e, 1, 1, corners_read, bit_rows, tile, tileset);
			}
			if(corners_read[ml])
			{
				editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, -1, -1, -1, 1, corners_read, bit_rows, tile, tileset);
			}
			if(corners_read[mr])
			{
				editor_world_read_and_fill_bit_corners(x, y, bit_w, bit_h, 1, 1, -1, 1, corners_read, bit_rows, tile, tileset);
			}
		}
	}
	corners_read[4] = 0;
	//finally, read the bit values
	//values are inverted for y
	u8 bit_values[10] = {
		32, 64, 128,
		8, 0, 16,
		1, 2, 4,
	};
	if(terrain->capacity == 16)
	{
		bit_values[7] = 1;
		bit_values[3] = 2;
		bit_values[5] = 4;
		bit_values[1] = 8;
	}
	for(u32 y = 0; y < 3; y++)
	{
		for(u32 x = 0; x < 3; x++)
		{
			u32 i = x + (y * 3);
			bit |= (bit_values[i] * corners_read[i]);
		}
	}
	return(bit);
}

static void
editor_world_read_wall_frame(
		s_game_editor *game_editor,
		world_tile *tile,
		u32 x,
		u32 y)
{
	s_world_editor *world_editor = &game_editor->world;
	//read autoterrain data from tileset
	world_tileset *tileset = editor_world_get_tileset(
			game_editor,
			tile->tileset_index);
	s_tileset_terrain *terrain = tileset->terrain + tile->tileset_terrain_index;
	tileset_wall *wall = tileset->walls + terrain->wall_index;
	i32 r_index = (x + 1) + (y * world_editor->map_w);
	i32 l_index = (x - 1) + (y * world_editor->map_w);
	i32 b_index = x + ((y - 1) * world_editor->map_w);
	i32 t_index = x + ((y + 1) * world_editor->map_w);
	b8 read_l = x > 0;
	b8 read_r = (x < (i32)world_editor->map_w);
	b8 read_b = y > 0;
	b8 read_t = y < (i32)world_editor->map_h;
	u32 wall_frame_index = 0;
	b8 indices[4] = {l_index, r_index, t_index, b_index};
	b8 reads_b[4] = {read_l, read_r, read_t, read_b};
	for(u32 r = 0; r < 4; r++)
	{
		if(reads_b[r])
		{
			world_tile *t1 = world_editor->terrain + indices[r];
			if(t1->wall_frame > wall_frame_index)
			{
				wall_frame_index = t1->wall_frame;
			}
		}
	}
	wall_frame_index++;
	wall_frame_index %= (wall->extra_frames_x + 1);
	tile->wall_frame = wall_frame_index;
}

static void
editor_world_read_autotiles(
		s_game_editor *game_editor,
		game_assets *game_asset_manager)
{
	//read autotile layer
	struct s_world_editor *world_editor = &game_editor->world;
	world_tile *tiles = world_editor->terrain;

#if 0
	for(i32 y = 0;
			y < (i32)world_editor->map_h;
			y++)
	{
		for(i32 x = 0;
				x < (i32)world_editor->map_w;
				x++)
		{
			u32 t = x + (y * world_editor->map_w);
			world_tile *tile = tiles + t;
			if(tile->is_autoterrain)
			{
				//read autoterrain data from tileset
				world_tileset *tileset = editor_world_get_tileset(
						game_editor,
						tile->tileset_index);
				Assert(tileset);
				//get data
				tileset_autoterrain *autoterrain = tileset->autoterrains + tile->autoterrain_index;
				//check adyacent tiles, starting from the middle
				if(autoterrain->capacity == 16)
				{
					u8 mask_values[] = {
					0, 1, 0,
					2, 0, 4,
					0, 8, 0};
					u32 bit_mask = 0;
					b8 read_l = x > 0;
					b8 read_r = (x < (i32)world_editor->map_w);
					b8 read_b = y > 0;
					b8 read_t = y < (i32)world_editor->map_h;
					i32 r_index = (x + 1) + (y * world_editor->map_w);
					i32 l_index = (x - 1) + (y * world_editor->map_w);
					i32 b_index = x + ((y - 1) * world_editor->map_w);
					i32 t_index = x + ((y + 1) * world_editor->map_w);
					//read left
					if(read_l)
					{
						i32 i = (x - 1) + (y * world_editor->map_w);
						bit_mask |= mask_values[3] * _check_autoterrain(tile, tiles + i, tileset);
					}
					//read right
					if (x < (i32)world_editor->map_w)
					{
						i32 i = (x + 1) + (y * world_editor->map_w);
						bit_mask |= mask_values[5] * _check_autoterrain(tile, tiles + i, tileset);
					}
					//read bottom
					if(y > 0)
					{
						i32 i = x + ((y - 1) * world_editor->map_w);
						bit_mask |= mask_values[7] * _check_autoterrain(tile, tiles + i, tileset);
					}
					//read top
					if(y < (i32)world_editor->map_h)
					{
						i32 i = x + ((y + 1) * world_editor->map_w);
						bit_mask |= mask_values[1] * _check_autoterrain(tile, tiles + i, tileset);
					}
					//get index based on value
					i16 final_index = bit_mask; 
					//read adyacent autoterrains and decide which layer to use
					u32 layer_index = 0;
					if(autoterrain->extra_layers)
					{
						//get maximum_layer
						if(read_l)
						{
							_check_autoterrain_layer(tile, tiles + l_index, tileset, &layer_index);
						}
						if(read_r)
						{
							_check_autoterrain_layer(tile, tiles + r_index, tileset, &layer_index);
						}
						if(read_t)
						{
							_check_autoterrain_layer(tile, tiles + t_index, tileset, &layer_index);
						}
						if(read_b)
						{
							_check_autoterrain_layer(tile, tiles + b_index, tileset, &layer_index);
						}
						layer_index++;
						layer_index %= (autoterrain->extra_layers + 1);
						final_index = bit_mask + (layer_index * autoterrain->capacity);
					}
					tile->tileset_terrain_index = autoterrain->indices[final_index];
					tile->autoterrain_layer = layer_index;
				}
				
			}
			//pick tile from layer and detect if is autotile
		}
	}
#else
	for(i32 y = 0;
			y < (i32)world_editor->map_h;
			y++)
	{
		for(i32 x = 0;
				x < (i32)world_editor->map_w;
				x++)
		{
			u32 t = x + (y * world_editor->map_w);
			world_tile *tile = tiles + t;
			if(game_editor->world.tilesets_count && tile->tileset_index < game_editor->world.tilesets_count)
			{
				//read autoterrain data from tileset
				world_tileset *tileset = editor_world_get_tileset(
						game_editor,
						tile->tileset_index);
				Assert(tileset);
				//get data
				//check adyacent tiles, starting from the middle
				s_tileset_terrain *terrain = tileset->terrain + tile->tileset_terrain_index;
				if(terrain->capacity > 1)
				{
					int s = 0;
				}
				u32 bit_mask = 
					editor_world_read_bit_corners(
							tiles,
							tileset,
							x,
							y,
							world_editor->map_w,
							world_editor->map_h);
				//get index based on value
				i16 final_index = 
					terrain->capacity == 16 ? autoterrain_index_from_mask16(bit_mask) :
					terrain->capacity == 48 ? autoterrain_index_from_mask46(0, bit_mask) :
					0;
				//read adyacent autoterrains and decide which layer to use
				u32 layer_index = 0;
	//			tile->tileset_terrain_index = t;
				tile->tileset_terrain_frame = final_index;
				editor_world_read_wall_frame(
						game_editor,
						tile,
						x,
						y);
			}
			//pick tile from layer and detect if is autotile
		}
	}
#endif
	
}

static void
editor_world_render_entities(
		s_editor_state *editor_state,
		render_commands *render_commands)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_world_editor *world_editor = &game_editor->world;
	game_assets *game_asset_manager = editor_state->editor_assets;

	for(u32 e = 0; e < world_editor->entity_count; e++)
	{
		editor_map_entity *entity = world_editor->entities + e;
		if(entity->type != map_entity_display)
		{
			continue;
		}
		//fix position first
		i32 x = (i32)entity->position.x / 8;
		i32 y = (i32)entity->position.y / 8;
		i32 tile_index = x + (y * world_editor->map_w);
		i32 tile_count = world_editor->map_w * world_editor->map_h;
		if(tile_index >= 0 && tile_index < tile_count)
		{
			//fix position to correctly stay with z
			world_tile *tile = world_editor->terrain + tile_index;
			entity->position.z = tile->height * 8.0f;
		}
		

		//render
		if(entity->model_index < world_editor->models_count)
		{
			game_resource_attributes *model_res = world_editor->models[
			entity->model_index];
			model_render_meshes(render_commands,
					game_asset_manager,
					model_res->asset_key->model,
					entity->position,
					V2(0, -1));
		}
	}
}

static void
editor_world_update_render_tiles(
		s_editor_state *editor_state,
		game_assets *game_asset_manager,
		render_commands *render_commands,
		f32 dt)
{

	s_game_editor *game_editor = &editor_state->editor;
	s_world_editor *world_editor = &game_editor->world;
	game_world *game_world = game_editor->world.loaded_world;

	editor_world_render_entities(editor_state, render_commands);

	if(!game_editor->world.draw_locked)
	{
		//check autotiles, set corresponding terrain index.
		editor_world_read_autotiles(
				game_editor, game_asset_manager);
	}

		//build geometry based on selected terrains
		game_world->geometry_locked = game_editor->world.draw_locked;
		game_world->tileset_count = game_editor->world.tilesets_count;
		game_world->tiles= game_editor->world.terrain;
		game_world->w = game_editor->world.map_w;
		game_world->h = game_editor->world.map_h;
		//for every tileset loaded on the editor, make sure the simulated world
		//uses it as asset
		//make a temporary area to allocate the array
		temporary_area temporary_tileset_area = temporary_area_begin(&game_editor->area);
		game_world->tilesets_a = memory_area_push_array(
				&game_editor->area, world_tileset *, game_editor->world.tilesets_count);
		for(u32 t = 0; t < game_editor->world.tilesets_count; t++)
		{
			game_world->tilesets_a[t] = editor_world_get_tileset(game_editor, t);
		}
	//
	//Draw every terrain
	//

	//Might need more!
	u32 map_terrain_count = game_editor->world.map_w * game_editor->world.map_h;

    if(map_terrain_count)
	{

//        game_editor->world.draw_locked = 0;
	    if(!game_editor->world.draw_locked)
	    {
	    	render_refresh_locked_vertices(render_commands);
	    }

	    render_push_locked_vertices(render_commands);
		world_tile *gridTiles = game_editor->world.terrain;

		if(!game_editor->world.draw_locked) //or if the renderer needs a lock update
		{
		    game_editor->world.draw_locked = 1;

	        //u32 map_terrain_count = 10;
			for(u32 y = 0;
					y < game_editor->world.map_h;
					y++)
			{
				for(u32 x = 0;
						x < game_editor->world.map_w;
						x++)
				{
					i32 at_x = game_editor->world.shift_x + x;
					i32 at_y = game_editor->world.shift_y + y;
					vec4 color = {255, 255, 255, 255};
					if(at_x < 0 || at_y < 0 || 
					   at_x >= game_editor->world.map_w ||
					   at_y >= game_editor->world.map_h
					   )
					{
						//color.g = 0;
						//color.b = 0;
						color.a = 120;
					}
					u32 t = x + (y * game_editor->world.map_w);
					world_tile *tile = game_world->tiles + t;
					if(tile->tileset_index < world_editor->tilesets_count)
					game_draw_terrain(
							game_world,
							game_asset_manager,
							render_commands,
							x, y,
							game_editor->world.shift_x,
							game_editor->world.shift_y,
							color);
				}
			}
		}
		render_pop_locked_vertices(render_commands);
	}
	//simulate hareas
	if(world_editor->tilesets_count)
	{
		world_tileset *tileset = editor_world_get_tileset(game_editor, 0);
		render_texture *tileset_texture = tileset->image;
		static sprite_animation temp_anim0 = {0};
		static sprite_animation temp_anim1 = {0};
		static sprite_animation_frame temp_frames0[6] = {0};
		static sprite_animation_frame temp_frames1[6] = {0};
		for(u32 f = 0; f < 6; f++)
		{
			temp_frames0[f].totalMiliseconds = 200;
			temp_frames0[f].fx = 17 + f;
			temp_frames0[f].fy = 6;
		}
		for(u32 f = 0; f < 6; f++)
		{
			temp_frames1[f].totalMiliseconds = 200;
			temp_frames1[f].fx = 17;
			temp_frames1[f].fy = 7;
		}
		temp_frames0[0].fx = 17;
		temp_frames0[0].fy = 6;
		temp_anim0.frames_total = 6;
		temp_anim0.frame_w = 12;
		temp_anim0.frame_h = 12;
		temp_anim0.frames = temp_frames0;

		temp_anim1.frames_total = 6;
		temp_anim1.frame_w = 12;
		temp_anim1.frame_h = 12;
		temp_anim1.frames = temp_frames1;
		//reproduce animations
		render_reproduce_sprite_animation(&temp_anim0,
				dt);
		render_reproduce_sprite_animation(&temp_anim1,
				dt);
		//this update_render_hareas
		for(u32 h = 0; h < world_editor->hareas.count; h++)
		{
			editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + h;
			if(harea->base.type == 0)
			{
				for(u32 y = 0; y < harea->base.h; y++)
				{
					for(u32 x = 0; x < harea->base.w; x++)
					{
						u32 tile_index = x + (y * harea->base.w);
						world_harea_tile *tile = memory_dyarray_get(harea->tiles, tile_index);
						if(tile->off)
						{
							continue;
						}
						f32 x_at = harea->base.x + (x * GAME_TILESIZE);//(f32)(harea->base.x + x) * GAME_TILESIZE;
						f32 y_at = harea->base.y + (y * GAME_TILESIZE);//(f32)(harea->base.y + y) * GAME_TILESIZE;
						f32 z_at = harea->base.z;//(f32)(harea->base.z) * GAME_TILESIZE;

						vec3 v0 = {x_at, y_at, z_at};
						vec3 v1 = {x_at, y_at + 12, z_at};
						vec3 v2 = {x_at + 12, y_at + 12, z_at};
						vec3 v3 = {x_at + 12, y_at, z_at};

						sprite_animation_frame *frames_array = 0;
						//the animation should be picked from the specified tile.
						sprite_animation_frame *frame = temp_frames0 + temp_anim0.current_frame;

						vec2 uv0 = {0};
						vec2 uv1 = {0};
						vec2 uv2 = {0};
						vec2 uv3 = {0};
						u32 fx = (frame->fx * temp_anim0.frame_w) + (frame->fx * 2 + 1);
						u32 fy = (frame->fy * temp_anim0.frame_h) + (frame->fy * 2 + 1);
						render_fill_uvs_from_frames(
								tileset_texture->width,
								tileset_texture->height,
								fx,
								fy,
								temp_anim0.frame_w,
								temp_anim0.frame_h,
								&uv0,
								&uv1,
								&uv2,
								&uv3
								);

						render_push_sprite(render_commands,
								tileset_texture,
								v0, v1, v2, v3, uv0, uv1, uv2, uv3);
					}
				}
			}
			else
			{

				model_render_data *models = memory_area_push_array(&game_editor->area, model_render_data, harea->external_model_count);
				for(u32 m = 0; m < harea->external_model_count; m++)
				{
					editor_harea_model_slot *model_slot = 0;
					memory_dyarray_get_safe(harea->external_models, model_slot, m);
					models[m] = model_allocate_render_data(&game_editor->area, &model_slot->r->asset_key->model);
				}
				//setup models for rendering
				//display models
				for(u32 p = 0; p < harea->point_count; p++)
				{
					editor_map_harea_point *point = 0;
					memory_dyarray_get_safe(harea->points, point, p);
					vec3 p = V3(harea->base.x + point->p.x, harea->base.y + point->p.y, harea->base.z);
					//editor_map_harea_model *model = 0;
					model_render_data *model_render = models + point->model_index;
					model_animate_and_render(render_commands, model_render, p, V2(0, -1), dt);
					//memory_dyarray_get_safe(harea->models, model, );
					//render_cube(render_commands, p, V3(2, 2, 2), V4(0, 255, 0, 255));

				}
			}
		}
	}
	
	temporary_area_end(&temporary_tileset_area);
}

static void
editor_world_reset(s_game_editor *game_editor)
{
	s_world_editor *world_editor = &game_editor->world;
	world_tile *worldTerrain = game_editor->world.terrain;
	world_tile emptyTerrain = {0};
	for(u32 w = 0; w < game_editor->world.map_terrain_count; w++)
	{
		worldTerrain[w] = emptyTerrain;
	}
	//dereference map
	if(world_editor->editing_map)
	{
		er_dereference_resource(world_editor->editing_map);
	}
	//derefecence tilesets
	for(u32 t = 0; t < world_editor->tilesets_count; t++)
	{
		game_resource_attributes *r = world_editor->editing_map_tilesets[t];
		er_dereference_resource(r);
	}
	//derefecence models 
	for(u32 t = 0; t < world_editor->models_count; t++)
	{
		game_resource_attributes *r = world_editor->models[t];
		er_dereference_resource(r);
	}
	//dereference harea models
	for(u32 h = 0; h < world_editor->hareas.count; h++)
	{
		editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + h;
		for(u32 m = 0; m < harea->external_model_count; h++)
		{
			editor_harea_model_slot *model = 0;
			memory_dyarray_get_safe(harea->external_models, model, m);
			if(model->r)
			{
				er_dereference_resource(model->r);
			}
		}
	}

	//reset data
	memory_clear(&world_editor->ew_reset_data, sizeof(world_editor->ew_reset_data));


	game_editor->world.map_w = 1;
	game_editor->world.map_h = 1;
	game_editor->world.new_map_w = 1;
	game_editor->world.new_map_h = 1;
	game_editor->world.entity_copied = 0;
}

static void
editor_world_load_map(s_editor_state *editor_state,
			          u8 *path_and_name)
{
	game_resource_attributes *resource = er_reference_resource(
			&editor_state->editor, path_and_name);
	if(!resource)
	{
		return;
	}

	s_game_editor *game_editor = &editor_state->editor;
	game_assets *game_asset_manager = editor_state->editor_assets;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &game_editor->info_stream;
	s_world_editor *world_editor = &game_editor->world;

	platform_file_handle map_file_handle = platform->f_open_file(
			resource->path_and_name,
			platform_file_op_read);

	if(map_file_handle.handle)
	{
		game_editor->world.draw_locked = 0;
		editor_world_reset(game_editor);
		world_editor->editing_map = resource;

		//copy data from asset
		asset_map_data *loading_map = &resource->asset_key->map;
		world_editor->map_w = loading_map->map_w;
		world_editor->map_h = loading_map->map_h;
		world_editor->new_map_w = loading_map->map_w;
		world_editor->new_map_h = loading_map->map_h;
		//terrain
		u32 terrain_count = world_editor->map_w * world_editor->map_h;
		for(u32 t = 0; t < terrain_count; t++)
		{
			world_editor->terrain[t] =
				loading_map->tiles[t];
		}
		Assert(resource->map.tileset_count < world_editor->tilesets_max);
		//tilesets
		for(u32 r = 0; r < resource->map.tileset_count; r++)
		{
			editor_world_add_tileset(
					game_editor, er_reference_existing_resource(resource->map.tilesets[r].attributes));
		}
		for(u32 m = 0; m < resource->map.model_count; m++)
		{
			world_editor->models[m] = resource->map.models[m].attributes;
			world_editor->models_count++;
		}
		//entities
		for(u32 e = 0; e < loading_map->entity_count; e++)
		{
			map_entity_data *entity = loading_map->entities + e;
			editor_map_entity *map_entity = editor_world_add_entity(
					game_editor, entity->position);
			map_entity->type = entity->type;
			map_entity->model_index = entity->model_index;
			map_entity->entity_index = entity->entity_index;
		}

		platform->f_close_file(map_file_handle);

		world_editor->map_terrain_count = world_editor->map_w * world_editor->map_h;
	}
	else
	{
		stream_pushf(
				info_stream,
				"There was an error while trying to open the map file %s",
				resource->path_and_name);
	}
}

static void
editor_world_save_map_new(s_editor_state *editor_state)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_world_editor *world_editor = &game_editor->world;
	game_assets *game_asset_manager = editor_state->editor_assets;
	platform_api *platform = editor_state->platform;
	//for log
	stream_data *info_stream = &game_editor->info_stream;
	if(!world_editor->editing_map)
	{
		return;
	}

	u8 *path_and_name = world_editor->editing_map->path_and_name;


	editor_wr wr = editor_wr_begin_write(
			&game_editor->area,
			platform,
			platform_file_op_create_new,
			path_and_name);
	game_world *world  = game_editor->world.loaded_world;
	u32 map_terrain_count     = game_editor->world.map_terrain_count;
	u32 colliderCount  = 0; 
	u32 entity_count   = world_editor->entity_count;
	u32 tileset_count  = game_editor->world.tilesets_count;

	world_collider *colliderArray = world->colliders;
	world_tile *terrainArray = game_editor->world.terrain;

	//check
	colliderCount = 0;

	if(wr.file.handle)
	{


		//uint8 *path_and_name	  = "Data/MAP0.level";	
		ppse_map_file_header *map_file_header = editor_wr_put_header(&wr, ppse_map_file_header);

		//Write data
		map_file_header->header.signature = (ppse_map_SIGNATURE);
		map_file_header->header.version = ppse_map_version;
		map_file_header->map_terrain_count = map_terrain_count;
		map_file_header->model_count = world_editor->models_count;
		map_file_header->entity_count = entity_count;
		map_file_header->tileset_count = tileset_count;
		map_file_header->map_w = game_editor->world.map_w;
		map_file_header->map_h = game_editor->world.map_h;

		//write header at the end with the tileset paths
		map_file_header->header.composite_resource_count = 
			world_editor->tilesets_count + world_editor->models_count;

		//composite resources
		editor_wr_put_line(&wr);
		{
			map_file_header->line_to_tileset_paths = wr.current_line_number;
			map_file_header->header.offset_to_composite_resources = wr.data_offset;
			//Write tileset headers
			for(u32 t = 0;
					t < tileset_count;
					t++)
			{

				stream_pushf(&game_editor->info_stream, "Saved tileset with name \"%s\"",
						game_editor->world.editing_map_tilesets[t]->path_and_name);

				editor_wr_write_composite_resource_header(
						&wr,
						game_editor->world.editing_map_tilesets[t]->path_and_name);
			}
			//Write model headers
			for(u32 m = 0;
					m < world_editor->models_count;
					m++)
			{
				editor_wr_write_composite_resource_header(
						&wr,
						game_editor->world.models[m]->path_and_name);

				stream_pushf(&game_editor->info_stream, "Saved model with name \"%s\"",
						game_editor->world.models[m]->path_and_name);
			}
			//for every harea, save their corresponding model
			for(u32 h = 0;
					h < world_editor->hareas.count;
					h++)
			{
				//editor_map_harea *harea = editor_array_base(
				//		world_editor->hareas, editor_map_harea) + h;
				//for(u32 m = 0; m < harea->model_count; m++)
				//{
				//	editor_wr_write_composite_resource_header(
				//			&wr,
				//			game_editor->world.models[m]->path_and_name);

				//	stream_pushf(&game_editor->info_stream, "Saved model with name \"%s\"",
				//			game_editor->world.models[m]->path_and_name);
				//}
			}
		}
		
		//terrain
		editor_wr_put_line(&wr);
		{
			map_file_header->line_to_terrain_data = wr.current_line_number;
			//save tiles
			for(u32 t = 0; t < map_terrain_count; t++)
			{	
				world_tile *tile_data = (terrainArray + t);
				ppse_world_tile *file_tile_data = editor_wr_put_struct(&wr, ppse_world_tile);
				file_tile_data->height = tile_data->height;
				file_tile_data->tileset_index = tile_data->tileset_index;
				file_tile_data->tileset_terrain_index = tile_data->tileset_terrain_index;
				file_tile_data->is_autoterrain = tile_data->is_autoterrain;
				file_tile_data->autoterrain_index = tile_data->autoterrain_index;

			}
		}
		//save entities
		editor_wr_put_line(&wr);
		{
			map_file_header->line_to_entity_data = wr.current_line_number;
			//write entity data 
			for(u32 e = 0; e < world_editor->entity_count; e++)
			{
				editor_map_entity *entity = world_editor->entities + e;
				ppse_map_entity *file_entity = editor_wr_put_struct(&wr, ppse_map_entity);

				file_entity->type = entity->type;
				file_entity->position = entity->position;
				file_entity->model_index = entity->model_index;
				file_entity->entity_index = entity->entity_index;
			}
		}
		//save hareas
		editor_wr_put_line(&wr);
		{
			//map_file_header->line_to_hareas = wr.current_line_number;
			//for(u32 h = 0; h < world_editor->hareas.count; h++)
			//{
			//	editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + h;
			//	ppse_map_harea *file_harea = editor_wr_put_struct(&wr, ppse_map_harea);
			//	file_harea->type = harea->base.type;
			//	file_harea->x = harea->base.x;
			//	file_harea->y = harea->base.y;
			//	file_harea->z = harea->base.z;
			//	file_harea->w = harea->base.w;
			//	file_harea->h = harea->base.h;
			//}
		}
		//save tags
		editor_wr_put_line(&wr);
		{
			map_file_header->line_to_tags = wr.current_line_number;
			//entity tags
			editor_wr_write_name_chunks(&wr, &world_editor->entity_tags, 0);
			editor_wr_write_name_chunks(&wr, &world_editor->harea_tags, 0);
		}
		editor_wr_end(&wr);


		stream_pushf(&game_editor->info_stream, "Saved map to: %s", path_and_name);
	}
	else //invalid file
	{
		stream_pushf(&game_editor->info_stream, "Unable to save map to: %s", path_and_name);
	}
}


static void
editor_world_save_map_old(s_editor_state *editor_state)
{
#if 0
	s_game_editor *game_editor = &editor_state->editor;
	s_world_editor *world_editor = &game_editor->world;
	game_assets *game_asset_manager = editor_state->editor_assets;
	platform_api *platform = editor_state->platform;
	//for log
	stream_data *info_stream = &game_editor->info_stream;
	if(!world_editor->editing_map)
	{
		return;
	}

	u8 *path_and_name = world_editor->editing_map->path_and_name;


	game_world *world  = game_editor->world.loaded_world;
	u32 map_terrain_count     = game_editor->world.map_terrain_count;
	u32 colliderCount  = 0; 
	u32 entity_count   = world_editor->entity_count;
	u32 tileset_count  = game_editor->world.tilesets_count;

	world_collider *colliderArray = world->colliders;
	world_tile *terrainArray = game_editor->world.terrain;

	//uint8 *path_and_name	  = "Data/MAP0.level";	
	ppse_map_file_header map_file_header = {0};

	//Write data
	map_file_header.header.signature = (ppse_map_SIGNATURE);
	map_file_header.header.version = ppse_map_version;
	map_file_header.map_terrain_count = map_terrain_count;
	map_file_header.model_count = world_editor->models_count;
	map_file_header.entity_count = entity_count;
	map_file_header.tileset_count = tileset_count;
	map_file_header.map_w = game_editor->world.map_w;
	map_file_header.map_h = game_editor->world.map_h;

	platform_file_handle map_file = platform->f_open_file(path_and_name, platform_file_op_create_new);
	//check
	colliderCount = 0;

	if(map_file.handle)
	{

		//Offset to data
		u32 data_offset = sizeof(ppse_map_file_header);

		//write header at the end with the tileset paths
		map_file_header->line_to_tileset_paths = data_offset;
		map_file_header->header.offset_to_composite_resources = data_offset;
		map_file_header->header.composite_resource_count = 
			world_editor->tilesets_count + world_editor->models_count;
		//Write tileset headers
		for(u32 t = 0;
				t < tileset_count;
				t++)
		{
			data_offset = editor_write_composite_resource_header(
					platform,
					map_file,
					data_offset,
					game_editor->world.editing_map_tilesets[t]->path_and_name);

			stream_pushf(&game_editor->info_stream, "Saved tileset with name \"%s\"",
					game_editor->world.editing_map_tilesets[t]->path_and_name);
		}
		//Write model headers
		for(u32 m = 0;
				m < world_editor->models_count;
				m++)
		{
			data_offset = editor_write_composite_resource_header(
					platform,
					map_file,
					data_offset,
					game_editor->world.models[m]->path_and_name);

			stream_pushf(&game_editor->info_stream, "Saved model with name \"%s\"",
					game_editor->world.models[m]->path_and_name);
		}
		
		map_file_header.offset_to_terrain_data = data_offset;

		//save tiles
		for(u32 t = 0; t < map_terrain_count; t++)
		{	
			world_tile *tile_data = (terrainArray + t);
			ppse_world_tile file_tile_data = {0};
			file_tile_data.height = tile_data->height;
			file_tile_data.tileset_index = tile_data->tileset_index;
			file_tile_data.tileset_terrain_index = tile_data->tileset_terrain_index;
			file_tile_data.is_autoterrain = tile_data->is_autoterrain;
			file_tile_data.autoterrain_index = tile_data->autoterrain_index;

			platform->f_write_to_file(
					map_file, 
					data_offset, 
					sizeof(ppse_world_tile), 
					&file_tile_data);
			data_offset += sizeof(ppse_world_tile);

		}
		//write entity data 
		for(u32 e = 0; e < world_editor->entity_count; e++)
		{
			editor_map_entity *entity = world_editor->entities + e;
			ppse_map_entity file_entity = {0};

			file_entity.type = entity->type;
			file_entity.position = entity->position;
			file_entity.model_index = entity->model_index;
			file_entity.entity_index = entity->entity_index;

			uint32 data_size = sizeof(ppse_map_entity);
			platform->f_write_to_file(map_file, data_offset, data_size, &file_entity);
			data_offset += data_size;
		}

		//write header
		platform->f_write_to_file
			(map_file,
			 0,
			 sizeof(ppse_map_file_header),
			 &map_file_header);



		stream_pushf(&game_editor->info_stream, "Saved map to: %s", path_and_name);
		platform->f_close_file(map_file);
	}
	else //invalid file
	{
		stream_pushf(&game_editor->info_stream, "Unable to save map to: %s", path_and_name);
	}
#endif
}


//
//MAINS
//
static void
editor_world_update_camera(game_renderer *gameRenderer,
		              s_editor_state *editor_state,
					  editor_input *editor_input)
{
	s_game_editor *game_editor = &editor_state->editor;
    u16 ui_is_interacting  = game_editor->ui_is_interacting; 
    u16 ui_focused         = game_editor->ui_is_focused; 
    u32 input_text_focused = ui_input_text_focused(editor_state->ui);

    u32 mouse_r_down    = input_down(editor_input->mouse_right);
    u32 mouse_l_down    = input_down(editor_input->mouse_left);

	u32 lock_camera_rotation = input_text_focused;

    f32 mouse_delta_x = editor_input->mouse_clip_x_last - editor_input->mouse_clip_x;
    f32 mouse_delta_y = editor_input->mouse_clip_y_last - editor_input->mouse_clip_y;

	f32 camera_rotation_x = gameRenderer->camera_rotation.x;
	f32 camera_rotation_z = gameRenderer->camera_rotation.z;

        //if(!lock_camera_rotation && editor_state->platform->window_is_focused)
        //{
        //  if(editor_input->spaceBarDown && !editor_input->ctrl_l)
        //  {

        //    f32 rx = 0;
        //    f32 rz = 0;

        //    rx += (f32)mouse_delta_y * PI * 0.0005f;
        //    rz += (f32)mouse_delta_x * PI * 0.0005f;

        //    gameRenderer->camera_rotation.x += rx; 
        //    gameRenderer->camera_rotation.z += rz; 
        //  }

        //}
	    //f32 xr = gameRenderer->camera_rotation.x;
	    //f32 zr = gameRenderer->camera_rotation.z;
	    //gameRenderer->camera_rotation.z = zr < 0 ? 2.0f : zr >= 2.0f ? 0 : zr;
	    //gameRenderer->camera_rotation.x = xr < 0 ? 2.0f : xr >= 2.0f ? 0 : xr;

	if(game_editor->mode == s_game_editor_world)
	{

	    //camera rotation

		game_editor->world.in_camera_mode = 0;
        if(!lock_camera_rotation && editor_state->platform->window_is_focused)
        {

		    camera_rotation_x = game_editor->world.camera_rotation_x;
		    camera_rotation_z = game_editor->world.camera_rotation_z;
            if(editor_input->spaceBarDown && mouse_l_down)
            {
			    game_editor->world.in_camera_mode = 1;

                camera_rotation_x += ((f32)mouse_delta_y * PI * 0.0005f); 
                camera_rotation_z += ((f32)mouse_delta_x * PI * 0.0005f); 
            }

	        camera_rotation_x = camera_rotation_x < 0 ? 2.0f : camera_rotation_x >= 2.0f ? 0 : camera_rotation_x;
	        camera_rotation_z = camera_rotation_z < 0 ? 2.0f : camera_rotation_z >= 2.0f ? 0 : camera_rotation_z;

            game_editor->world.camera_rotation_x = camera_rotation_x;
            game_editor->world.camera_rotation_z = camera_rotation_z;
	        gameRenderer->camera_rotation.x = camera_rotation_x;
	        gameRenderer->camera_rotation.z = camera_rotation_z;

        }
	

        real32 camSpeed = 6.0f;

	    if(!input_text_focused)
	    {
			vec3 camera_position = game_editor->world.camera_position;
			
			{

				i32 mouseWheelValue = editor_input->mouse_wheel * 10;
		        if(!ui_focused)
		        {
		            game_editor->world.camera_distance_from_cursor -= mouseWheelValue;

				    if(game_editor->world.camera_distance_from_cursor < 0)
				    {
                        game_editor->world.camera_distance_from_cursor = 0;

				    	//instead, add the new value to the pad location
				    	vec3 add_zAxis = vec3_scale(gameRenderer->camera_z, (f32)mouseWheelValue);

						game_editor->world.camera_pad_position.x -= add_zAxis.x;
						game_editor->world.camera_pad_position.y -= add_zAxis.y;
						game_editor->world.camera_pad_position.z -= add_zAxis.z;
				    }
		        }

				f32 camera_distance_from_cursor = game_editor->world.camera_distance_from_cursor;
				vec3 camera_distance_from_cursor_vec = {0, 0, camera_distance_from_cursor};


		        matrix4x4 camera_rotation = matrix4x4_rotation_scale(gameRenderer->camera_rotation.x,
		   		                                                    gameRenderer->camera_rotation.y,
		   						                                    gameRenderer->camera_rotation.z);

		        camera_distance_from_cursor_vec = matrix4x4_v3_mul_rows(
						                       camera_rotation,
						                       camera_distance_from_cursor_vec,
											   0);

				camera_position = vec3_add(camera_distance_from_cursor_vec,
						                  game_editor->world.camera_pad_position);

				//move padding position
				if(editor_input->spaceBarDown && mouse_r_down)
				{
					f32 mouse_delta_x = editor_input->mouse_x - editor_input->mouse_x_last;
					f32 mouse_delta_y = editor_input->mouse_y - editor_input->mouse_y_last;

					f32 paddingSpeed = 0.3f;

					vec3 add_xAxis = vec3_scale(gameRenderer->camera_x, mouse_delta_x * paddingSpeed);
					vec3 add_yAxis = vec3_scale(gameRenderer->camera_y, mouse_delta_y * paddingSpeed);

					game_editor->world.camera_pad_position.x -= add_xAxis.x - add_yAxis.x;
					game_editor->world.camera_pad_position.y -= add_xAxis.y - add_yAxis.y;
					game_editor->world.camera_pad_position.z -= add_xAxis.z - add_yAxis.z;
				}
			}


            game_editor->world.camera_position = camera_position;
            gameRenderer->camera_position = game_editor->world.camera_position;
	    }
	}
	if(game_editor->mode == s_game_editor_model)
	{   
		//model editor camera
		//rotate camera
		if(!lock_camera_rotation && editor_state->platform->window_is_focused)
        {

		    camera_rotation_x = game_editor->model.camera_rotation_x;
		    camera_rotation_z = game_editor->model.camera_rotation_z;
            if(editor_input->spaceBarDown)
            {
				//rotate model camera
				if(mouse_l_down)
				{
			        game_editor->model.in_camera_mode = 1;

                    camera_rotation_x += ((f32)mouse_delta_y * PI * 0.0005f); 
                    camera_rotation_z += ((f32)mouse_delta_x * PI * 0.0005f); 
	                camera_rotation_x = camera_rotation_x < 0 ? 2.0f : camera_rotation_x >= 2.0f ? 0 : camera_rotation_x;
	                camera_rotation_z = camera_rotation_z < 0 ? 2.0f : camera_rotation_z >= 2.0f ? 0 : camera_rotation_z;
				}
				else if(mouse_r_down)
				{
					f32 mouse_delta_x = editor_input->mouse_x - editor_input->mouse_x_last;
					f32 mouse_delta_y = editor_input->mouse_y - editor_input->mouse_y_last;

					f32 paddingSpeed = 0.02f * game_editor->model.cameraDistance * 0.5f;
					paddingSpeed = (paddingSpeed > 0.2f) ? 
						           0.2f : (paddingSpeed < 0.02f) ? 0.02f : paddingSpeed;

					vec3 add_xAxis = vec3_scale(gameRenderer->camera_x, mouse_delta_x * paddingSpeed);
					vec3 add_yAxis = vec3_scale(gameRenderer->camera_y, mouse_delta_y * paddingSpeed);

					game_editor->model.camera_position.x -= add_xAxis.x - add_yAxis.x;
					game_editor->model.camera_position.y -= add_xAxis.y - add_yAxis.y;
					game_editor->model.camera_position.z -= add_xAxis.z - add_yAxis.z;

					//game_editor->model.cameraDistance = vec3_length(game_editor->model.camera_position);
				}
            }


            game_editor->model.camera_rotation_x = camera_rotation_x;
            game_editor->model.camera_rotation_z = camera_rotation_z;
	        gameRenderer->camera_rotation.x     = camera_rotation_x;
	        gameRenderer->camera_rotation.z     = camera_rotation_z;

        }
		//set position by rotation
		if(!ui_focused)
		{

			i32 mouseWheelValue = editor_input->mouse_wheel * 10;
		    game_editor->model.cameraDistance -= mouseWheelValue;

			if(game_editor->model.cameraDistance < 0)
			{
                game_editor->model.cameraDistance = 0;

				//instead, add the new value to the pad location
				vec3 add_zAxis = vec3_scale(gameRenderer->camera_z, (f32)mouseWheelValue);

				game_editor->model.camera_position.x -= add_zAxis.x;
				game_editor->model.camera_position.y -= add_zAxis.y;
				game_editor->model.camera_position.z -= add_zAxis.z;
			}
		}
		
		vec3 model_editor_origin = {0, 0, 0};
		vec3 distanceFromCamera = {0, 0, game_editor->model.cameraDistance};

		matrix4x4 camera_rotation = 
		    matrix4x4_rotation_scale(gameRenderer->camera_rotation.x,
		   		                     gameRenderer->camera_rotation.y,
		   						     gameRenderer->camera_rotation.z);

		distanceFromCamera           = matrix4x4_v3_mul_rows(camera_rotation, distanceFromCamera, 0);
		gameRenderer->camera_position = vec3_add(game_editor->model.camera_position,
				                                distanceFromCamera);
		//gameRenderer->camera_position = game_editor->model.camera_position;
				                                

	}

	gameRenderer->camera_rotation.x = camera_rotation_x;
	gameRenderer->camera_rotation.z = camera_rotation_z;
}

static void
editor_world_update_render(s_editor_state *editor_state,
		                   game_renderer *game_renderer,
						   editor_input *editor_input,
						   f32 dt)
{
	s_game_editor *game_editor = &editor_state->editor;
	game_assets *game_asset_manager = editor_state->editor_assets;
	stream_data *info_stream = &game_editor->info_stream;
	struct s_world_editor *world_editor = &game_editor->world;

	if(world_editor->map_w == 0 || world_editor->map_h == 0)
	{
		world_editor->map_w = MAX(1, world_editor->map_w);
		world_editor->map_h = MAX(1, world_editor->map_h);
		world_editor->draw_locked = 0;
	}

	f32 ray_distance = 100000.0f;
	bool32 tile_hitted = 0;
	u32 ui_focused = game_editor->ui_is_focused; 
	b8 mouse_l_down = input_down(editor_input->mouse_left);
	b8 mouse_l_pressed = input_pressed(editor_input->mouse_left);
	b8 mouse_r_pressed = input_pressed(editor_input->mouse_right);
	b8 mouse_r_down = input_down(editor_input->mouse_right);
    u32 input_text_focused = ui_input_text_focused(editor_state->ui);
	mouse_l_down = mouse_l_down && game_editor->process_input;
	mouse_l_pressed = mouse_l_pressed && game_editor->process_input;

	//Set ray trace
	vec2 mouse_point = {editor_input->mouse_clip_x, editor_input->mouse_clip_y};
	vec3 mouse_in_world = render_mouse_coordinates_to_world(
			game_renderer, mouse_point, 1.0f);
	vec3 ray_origin = game_renderer->camera_position;
	vec3 ray_direction = vec3_normalize_safe(vec3_sub(mouse_in_world, ray_origin));

	//cast ray to every tile
	f32 tile_size_half = GAME_TILESIZE * 0.5f;
	game_editor->world.ray_hits_tile = 0;

	if(!game_editor->ui_is_focused)
	{
		for(u32 y = 0;
				y < game_editor->world.map_h;
				y++)
		{
			for(u32 x = 0;
					x < game_editor->world.map_w;
					x++)
			{
				//tiles with height of 0 should be treated has a cube with the size of 
				//16. In order the correctly cast the rays, start from the bottom left corner of
				//the "cube" and add the half of the tile size.
				u32 tile_index = x + (y * world_editor->map_w);
				world_tile *target_tile = world_editor->terrain + tile_index;
				cube_shape tile_as_cube = editor_world_get_tile_cube(
						game_editor, tile_index);
				//calculate the position of the tile, starting from the z of the layer if any
				//aded half the tile size in x, y ,z
				vec3 cube_origin = tile_as_cube.position;
				//count the height of the tile!
				vec3 cube_size = tile_as_cube.size; 

				ray_cube_result ray_terrain_result = ray_cube_get_result(
						ray_origin,
						ray_direction,
						cube_origin,
						cube_size
						);
				bool32 ray_hits_tile = ray_terrain_result.t_min &&
					ray_terrain_result.t_min <= ray_distance;
				//set the new ray distance
				if(ray_hits_tile)
				{
					ray_distance = ray_terrain_result.t_min;
					game_editor->world.ray_hits_tile = 1;
					game_editor->world.ray_tile_index = tile_index;
							world_editor->ray_tile_position = vec3_scale(
									ray_direction, ray_terrain_result.t_min);
							world_editor->ray_tile_position = vec3_add(
									world_editor->ray_tile_position, ray_origin);
				}

			}
		}
	}

	//update tile editing or painting
	if(game_editor->world.mode == world_mode_normal)
	{
		if(world_editor->tilesets_count)
		{
			world_editor->selected_tileset_index = MIN(
					world_editor->selected_tileset_index, (u32)(world_editor->tilesets_count - 1));
		}
		if(world_editor->tilesets_count &&
				world_editor->ray_hits_tile &&
				game_editor->process_input)
		{
			i32 start_x = 0;
			i32 start_y = 0;
			i32 end_x = 0;
			i32 end_y = 0;
			editor_world_fill_selection_distances(
					game_editor,
					&start_x,
					&start_y,
					&end_x,
					&end_y);

			//paint based on selection
			for(i32 y = start_y;
					y <= end_y;
					y++)
			{
				for(i32 x = start_x;
						x <= end_x;
						x++)
				{
					i32 tile_index = x + (y * world_editor->map_w);
					world_tile *painting_tile = 
						world_editor->terrain + tile_index;
					if(mouse_l_down)
					{
						painting_tile->tileset_index = world_editor->selected_tileset_index;
						//autoterrain
						painting_tile->is_autoterrain = world_editor->next_tile_autoterrain;
						if(world_editor->next_tile_autoterrain)
						{
							painting_tile->autoterrain_index = world_editor->selected_autoterrain_index;
							//;cleanup. This is just in case
							painting_tile->tileset_terrain_index = 0;
						}
						else //normal tile
						{
							painting_tile->tileset_terrain_index = world_editor->selected_tileset_terrain_index;
							//redraw after painting
						}
						world_editor->draw_locked = 0;
					}
					//increase height of selecting tiles
					if(input_pressed(editor_input->w))
					{
						painting_tile->height++;
						world_editor->draw_locked = 0;
					}
					//decrease height of selecting tiles
					else if(input_pressed(editor_input->s))
					{
						painting_tile->height--;
						world_editor->draw_locked = 0;
					}
				}
			}
			//paint!
		}
	}
	else if(game_editor->world.mode == world_mode_selection)
	{
		if(world_editor->ray_hits_tile)
		{
			//get the first tile hitted by the ray
			if(!world_editor->selection_started)
			{
				world_editor->selection_started = 1;
				world_editor->selection_tile_from_index = world_editor->ray_tile_index;
			}
			else
			{
				//get x and y from the starting tile selection index
				i32 x_start = world_editor->selection_tile_from_index % world_editor->map_w;
				i32 y_start = world_editor->selection_tile_from_index / world_editor->map_w;
				//get x and y from the current index
				i32 x_end = world_editor->ray_tile_index % world_editor->map_w;
				i32 y_end = world_editor->ray_tile_index / world_editor->map_w;

				world_editor->selection_amount_x = x_start - x_end;
				world_editor->selection_amount_y = y_start - y_end;

			}
		}
	}
	//world editor mode specific functions
	switch(game_editor->world.mode)
	{
		case world_mode_entities:
			{
				vec3 entities_cube_size = {10, 10, 20};
				if(!game_editor->ui_is_focused)
				{
					f32 ray_distance = 100000.0f;
					//reset to zero
					game_editor->world.ray_hits_entity = 0;
					//cast ray to entities
					for(u32 e = 0; e < world_editor->entity_count; e++)
					{
						editor_map_entity *ent = world_editor->entities + e;
						vec3 cube_origin = ent->position;
						//add half the size
						cube_origin.z += entities_cube_size.z * 0.5f;
						vec3 cube_size = entities_cube_size; 

						ray_cube_result ray_cube_result = ray_cube_get_result(
								ray_origin,
								ray_direction,
								cube_origin,
								cube_size
								);
						bool32 ray_hits_tile = ray_cube_result.t_min &&
							ray_cube_result.t_min <= ray_distance;
						//set the new ray distance
						if(ray_hits_tile)
						{
							ray_distance = ray_cube_result.t_min;
							game_editor->world.ray_hits_entity = 1;
							game_editor->world.ray_entity_index = e;

						}
					}
				}
				//drag selected entities

				//select clicked entity
				if(mouse_l_pressed && !game_editor->ui_is_focused)
				{
					if(world_editor->ray_hits_entity)
					{
						world_editor->entity_got_selected = 1;
						world_editor->selected_entity_index = world_editor->ray_entity_index;
					}
				}


				//display entity boxes
				render_commands *render_commands = render_commands_begin(game_renderer, render_flags_Blending); 

				//display entity boxes
				for(u32 e = 0; e < world_editor->entity_count; e++)
				{
					editor_map_entity *ent = world_editor->entities + e;
					vec4 color = {255, 0, 0, 255};
					if(!world_editor->entity_got_selected ||
						world_editor->selected_entity_index != e)
					{
						color.a = 160.0f;
					}
					if(world_editor->ray_hits_entity &&
					   world_editor->ray_entity_index == e)
					{
						color.a = 200.0f;
					}
					if(world_editor->entity_got_selected &&
						world_editor->selected_entity_index == e)
					{
						color.a = 255;
					}


					vec3 cube_p = ent->position;
					cube_p.z += entities_cube_size.z * 0.5f;

					render_cube_borders(
							render_commands,
							cube_p,
							entities_cube_size,
							1,
							color
							);

				}

				render_commands_end(render_commands);

			}break;
		case world_mode_resize:
			{
				//display new map size
				render_commands *render_commands = render_commands_begin(game_renderer, render_flags_Blending); 

				vec2 wh = {
					(f32)game_editor->world.new_map_w * GAME_TILESIZE,
					(f32)game_editor->world.new_map_h * GAME_TILESIZE};
				render_hollow_rec(
						render_commands,
						V3(0, 0 , 0),
						wh,
						V3(1, 0, 0),
						V3(0, 1, 0),
						1,
						1,
						1,
						V4(0, 0, 255, 255));
				render_commands_end(render_commands);
			}break;
		case world_mode_harea:
			{
				f32 ray_distance = 100000.0f;
				//cast ray to hareas
				world_editor->harea_is_hot = 0;
				for(u32 h = 0; h < world_editor->hareas.count; h++)
				{
					editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + h;
					vec3 p = {(f32)harea->base.x, (f32)harea->base.y, (f32)harea->base.z};
					vec2 size = {(f32)harea->base.w * GAME_TILESIZE, (f32)harea->base.h * GAME_TILESIZE};
					//cast and set distance
					f32 new_ray_distance = ray_rectangle_cast_xy(ray_origin, ray_direction, ray_distance,
							p, size, 1.0f, 1.0f);
					//hits!
					if(new_ray_distance != ray_distance)
					{
						world_editor->harea_is_hot = 1;
						world_editor->harea_hot_index = h;
					}
				}
				render_commands *render_commands = render_commands_begin(game_renderer, render_flags_Blending); 
				//select hot harea
				//selection mode
				if(!world_editor->harea_grid_edit || !world_editor->harea_is_selected)
				{
					if(mouse_l_pressed && !world_editor->in_camera_mode)
					{
						if(world_editor->harea_is_hot)
						{
							world_editor->harea_is_selected = 1;
							world_editor->selected_harea_index = world_editor->harea_hot_index;
						}
						else
						{
							world_editor->harea_is_selected = 0;
						}
					}
					//display hareas 
					for(u32 h = 0; h < world_editor->hareas.count; h++)
					{
						editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + h;
						//display hareas
						vec3 p = {(f32)harea->base.x, (f32)harea->base.y, (f32)harea->base.z};
						vec2 size = {(f32)harea->base.w * GAME_TILESIZE, (f32)harea->base.h * GAME_TILESIZE};
						vec4 color = {255, 0, 0, 120};
						if(world_editor->harea_is_selected && world_editor->selected_harea_index == h)
						{
							color.a = 255;
						}
						else if(world_editor->harea_is_hot && world_editor->harea_hot_index == h)
						{
							color.a = 180;
						}

						render_hollow_rec(
								render_commands,
								p,
								size,
								V3(1, 0, 0),
								V3(0, 1, 0),
								1.0f,
								1.0f,
								1.0f,
								color);
					}
				}//grid edit mode
				else
				{
					//display this harea
					b8 tile_is_hot = 0;
					u32 hot_tile_index = 0;
					vec2 ray_harea = {0};
					editor_map_harea *harea = editor_array_base(
							world_editor->hareas, editor_map_harea) + world_editor->selected_harea_index;
					{
						ray_rectangle_fill_position_xy_bl(
								ray_origin,
								ray_direction,
								100000.0f,
								harea->base.p,
								ew_harea_size(harea),
								&ray_harea.x,
								&ray_harea.y);
						i32 tile_x = (i32)(ray_harea.x / GAME_TILESIZE);
						i32 tile_y = (i32)(ray_harea.y / GAME_TILESIZE);
						//indices are inside the harea
						if(tile_x >= 0 && tile_y >= 0 && 
						   tile_x < harea->base.w && tile_y < harea->base.h)
						{
							tile_is_hot = 1;
							hot_tile_index = (u32)(tile_x + (tile_y * harea->base.w));
						}
					}

					if(ray_harea.x > GAME_TILESIZE * 3)
					{
						int s = 0;
					}
					//get the tile position of the ray on this grid
					for(u32 y = 0; y < harea->base.h; y++)
					{
						for(u32 x = 0; x < harea->base.w; x++)
						{
							u32 tile_index = x + (y * harea->base.w);
							//display grids!
							world_harea_tile *tile = memory_dyarray_get(harea->tiles, tile_index);
							vec3 tile_p = {
								harea->base.x + (x * GAME_TILESIZE),
								harea->base.y + (y * GAME_TILESIZE),
								harea->base.z};
							vec2 tile_sz = {GAME_TILESIZE, GAME_TILESIZE};
							if(!tile->off)
							{
								render_rectangle_xy_bl(
										render_commands,
										tile_p,
										tile_sz,
										V4(255, 255, 255, 120));
							}
							//highlight selected tile, turn off if clicked
							if(tile_is_hot && hot_tile_index == tile_index)
							{
								render_hollow_rec(
										render_commands,
										tile_p,
										tile_sz,
										V3(1, 0, 0),
										V3(0, 1, 0),
										1.0f,
										1.0f,
										1.0f,
										V4(255, 255, 255, 255));
								if(mouse_l_pressed)
								{
									tile->off = !tile->off;
								}
							}
						}
					}

					//display the size of this harea
						vec4 color = {255, 0, 0, 255};
					render_hollow_rec(
							render_commands,
							V3(harea->base.x, harea->base.y, harea->base.z),
							V2((f32)(harea->base.w * GAME_TILESIZE), (f32)(harea->base.h * GAME_TILESIZE)),
							V3(1, 0, 0),
							V3(0, 1, 0),
							1.0f,
							1.0f,
							1.0f,
							color);
				}

				render_commands_end(render_commands);
			}break;
	}
	//world editor hotkeys
	//selection mode
				if(editor_state->ui->keep_input_text_interaction)
				{
					int s = 0;
				}
	if(!input_text_focused)
	{
		b8 switch_view_mode = editor_input->number_keys[0];
		b8 switch_normal_mode = editor_input->number_keys[1];
		b8 switch_visual_mode = editor_input->number_keys[2];
		b8 switch_entity_mode = editor_input->number_keys[3];
		b8 switch_harea_mode = editor_input->number_keys[4];
		switch(game_editor->world.mode)
		{

			case world_mode_normal:
				{
					if(input_pressed(editor_input->esc))
					{
						//reset tile selections to default value
						b32 selecting_amount = 
							world_editor->selection_amount_x != 0 +
							world_editor->selection_amount_y != 0;
						if(selecting_amount)
						{
							world_editor->selection_amount_x = 0;
							world_editor->selection_amount_y = 0;
						}
					}
				}break;
			case world_mode_selection:
				{
					b8 cancel_selection = input_pressed(editor_input->esc);
					if(cancel_selection)
					{
						game_editor->world.mode = world_mode_normal;
						game_editor->world.selection_started = 0;
					}
				}break;
			case world_mode_entities:
				{
					b8 cancel_selection = input_pressed(editor_input->esc) && game_editor->world.entity_got_selected;
					b8 delete_selected = input_pressed(editor_input->d) && game_editor->world.entity_got_selected;
					b8 copy = input_pressed(editor_input->y);
					b8 paste = input_pressed(editor_input->p);
					if(input_pressed(editor_input->c) && world_editor->ray_hits_tile) //place or "paint"
					{
						editor_world_add_entity(game_editor, world_editor->ray_tile_position);
					}
					else if(delete_selected && world_editor->entity_got_selected)
					{
						u32 i = world_editor->selected_entity_index;
						editor_world_delete_entity(game_editor, i);

						world_editor->entity_got_selected = 0;
					}
					else if(cancel_selection)
					{
						world_editor->entity_got_selected = 0;
					}
					else if(copy && world_editor->entity_got_selected)
					{
						world_editor->yanked_entity = world_editor->entities[world_editor->selected_entity_index];
					}
					else if(paste && world_editor->ray_hits_tile)
					{
						editor_map_entity *map_entity = editor_world_add_entity(game_editor, world_editor->yanked_entity.position);
						*map_entity = world_editor->yanked_entity;
						map_entity->position = world_editor->ray_tile_position;
					}
				}break;
			case world_mode_harea:
				{
					b8 add_harea = input_pressed(editor_input->c);
					b8 remove_harea = input_pressed(editor_input->del);
					if(add_harea && world_editor->ray_hits_tile)
					{
						u32 tile_x = 0;
						u32 tile_y = 0;
						u32 tile_z = 0;
						editor_world_tile_indices(game_editor, &tile_x, &tile_y, &tile_z);
						//editor_world_add_harea_at(game_editor,
						//		(f32)tile_x * GAME_TILESIZE,
						//		(f32)tile_y * GAME_TILESIZE,
						//		(f32)tile_z * GAME_TILESIZE,
						//		1, 1);
						world_editor->open_add_context_menu = 1;
						world_editor->cached_ray_position = world_editor->ray_tile_position;
					}
					if(remove_harea && world_editor->harea_is_selected)
					{
						editor_world_remove_harea(game_editor, world_editor->selected_harea_index);
						world_editor->harea_is_selected = 0;
					}
				}break;
		}
		if(game_editor->world.mode == world_mode_normal ||
		   game_editor->world.mode == world_mode_harea ||
		   game_editor->world.mode == world_mode_view ||
		   game_editor->world.mode == world_mode_entities)
		{
			if(switch_normal_mode)
			{
				game_editor->world.mode = world_mode_normal;
			}
			if(switch_visual_mode)
			{
				//KEEP WORKING.
				game_editor->world.mode = world_mode_selection;
				game_editor->world.selection_amount_x = 0;
				game_editor->world.selection_amount_y = 0;
				game_editor->world.selection_started = 0;
			}
			if(switch_entity_mode)
			{
				game_editor->world.mode = world_mode_entities;
			}
			if(switch_view_mode)
			{
				game_editor->world.mode = world_mode_view;
			}
			if(switch_harea_mode)
			{
				game_editor->world.mode = world_mode_harea;
			}
		}
	}

	//display 3d information on the level.
	render_commands *render_commands = render_commands_begin(game_renderer, render_flags_Blending); 

	//display hot and selected terrains
	if(game_editor->world.ray_hits_tile &&
	   (world_editor->mode == world_mode_normal ||
		world_editor->mode == world_mode_selection))
	{
#if 0
		cube_shape tile_as_cube = editor_world_get_tile_cube(
				game_editor, game_editor->world.ray_tile_index);
		render_cube_borders(
				render_commands,
				tile_as_cube.position,
				tile_as_cube.size,
				1,
				V4(255, 255, 255, 255)
				);
#else
		i32 start_x = 0;
		i32 start_y = 0;
		i32 end_x = 0;
		i32 end_y = 0;

		editor_world_fill_selection_distances(
				game_editor,
				&start_x,
				&start_y,
				&end_x,
				&end_y);

		for(i32 y = start_y;
				y <= end_y;
				y++)
		{
			for(i32 x = start_x;
					x <= end_x;
					x++)
			{
				i32 next_index = x + (y * game_editor->world.map_w);
				cube_shape tile_as_cube = editor_world_get_tile_cube(
						game_editor, next_index);
				render_cube_borders(
						render_commands,
						tile_as_cube.position,
						tile_as_cube.size,
						1,
						V4(255, 255, 255, 255)
						);
			}

		}
#endif
	}

	//display map size
	render_hollow_rec(
			render_commands,
			V3(0, 0 , 0),
			V2((f32)game_editor->world.map_w * GAME_TILESIZE, (f32)game_editor->world.map_h * GAME_TILESIZE),
			V3(1, 0, 0),
			V3(0, 1, 0),
			1,
			1,
			1,
			V4(255, 0, 0, 255));

	render_commands_end(render_commands);

	//generate poisson for the selected harea
	if(game_editor->world.generate_harea_poisson && world_editor->harea_is_selected)
	{
		editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + world_editor->selected_harea_index;
		ew_generate_harea_poisson(
				game_editor, harea);
		
	}
	else
	{
		world_editor->generate_harea_poisson = 0;
	}

	//commands for rendering the 3d world with depth
	render_commands = render_commands_begin_default(game_renderer); 

	editor_world_update_render_tiles(
			editor_state,
			game_asset_manager,
			render_commands, dt);

	render_commands_end(render_commands);

	//from context menus

}

static inline void
editor_world_db_panel(s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input,
		game_ui *ui)
{
	s_game_editor *game_editor = &editor_state->editor;
	game_assets *game_asset_manager = editor_state->editor_assets;

	if(ui_window_begin(
				ui, ui_panel_flags_front_panel,
				100, 100, 200, 200, "World editor debug panel"))
	{
		u8 *current_mode_name[] = {
			"Normal",
			"Selection"};
		u8 *current_mode_name_ptr = game_editor->world.mode < ARRAYCOUNT(current_mode_name) ?
			current_mode_name[game_editor->world.mode] : "Undefined";
		ui_textf(ui, "Current mode: %s",
				current_mode_name_ptr);

		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			if(ui_button(ui, "Confirm"))
			{
				editor_world_change_map_size(
						game_editor);
			}
			if(ui_button(ui, "Redraw"))
			{
				game_editor->world.draw_locked = 0;
			}

			//ui_space_specified(ui, 4.0f, 1.0f);
			//ui_set_wh_text(ui, 4.0f, 1.0f)
			//{
			//	ui_text(ui, "Map size:");
			//}
			//ui_set_h_em(ui, 2.0f, 1.0f)
		    //ui_set_w_em(ui, 12.0f, 1.0f)
			//{
			//	ui_spinner_u32(ui,
			//			1,
			//			1,
			//			10000,
			//			&game_editor->world.new_map_w,
			//			0,
			//			"new_map_w");

			//	ui_spinner_u32(ui,
			//			1,
			//			1,
			//			10000,
			//			&game_editor->world.new_map_h,
			//			0,
			//			"new_map_h");
			//}
			//ui_set_wh_text(ui, 4.0f, 1.0f)
			//{
			//	ui_textf(ui, "Current maximum terrain capacity: %u", game_editor->world.map_terrain_max);
			//	u32 total_terrain_needed = 
			//		game_editor->world.new_map_w * game_editor->world.new_map_h;
			//	b32 more_terrain_allocation_needed = total_terrain_needed >= game_editor->world.map_terrain_max;
			//	ui_set_row(ui)
			//	{
			//		ui_text(ui, "Occuppied by new sizes:");
			//		if(more_terrain_allocation_needed) ui_set_color(ui, ui_color_background, V4(255, 0, 0, 255))
			//		{
			//			ui_textf(ui, "%u", total_terrain_needed);
			//		}
			//		else
			//		{
			//			ui_textf(ui, "%u", total_terrain_needed);
			//		}
			//	}
			//	ui_push_disable_if(ui, more_terrain_allocation_needed);
			//	if(ui_button(ui, "Confirm##map_size"))
			//	{
			//		editor_world_change_map_size(
			//				game_editor);
			//	}
			//	ui_pop_disable(ui);
			//}

		}
	}
	ui_panel_end(ui);

}

static void
editor_world_update_render_ui(s_editor_state *editor_state,
		                      game_renderer *game_renderer,
							  editor_input *editor_input)
{
	s_game_editor *game_editor = &editor_state->editor;
	game_world *editor_game_world = game_editor->world.loaded_world;
	game_assets *game_asset_manager = editor_state->editor_assets;
	struct s_world_editor *world_editor = &game_editor->world;

	game_ui *ui = editor_state->ui;

	editor_world_db_panel(editor_state,
			game_renderer,
			editor_input,
			ui);

	b32 new_clicked = 0;
	b32 save_clicked = 0;
	b32 load_clicked = 0;
	b32 save_as_clicked = 0;

	//Tileset panel at right ->
	f32 paintPanel_w = 400;
	f32 paintPanel_x = game_renderer->os_window_width - paintPanel_w;
	f32 paintPanel_y = 0;
	f32 paintPanel_h = game_renderer->os_window_height - paintPanel_y - 20;
	ui_node *tool_bar = 0;

	ui_id new_map_dialogue_id = ui_id_from_string("_WE_NEW_MAP_");
	ui_id add_plane_popup_id = ui_id_from_string("_ADD_PLANE_");

	ui_set_h_em(ui, 3.0f, 1.0f)
	ui_set_w_ppct(ui, 1.0f ,1.0f)
	{
		tool_bar = ui_label(ui, "world_editor_tool_bar");
	}
	ui_set_parent(ui, tool_bar)
	{
		ui_space(ui, ui_size_specified(4.0f, 1.0f));
		ui_set_row(ui)
		{
			ui_space(ui, ui_size_specified(4.0f, 1.0f));
			ui_set_wh_text(ui, 4.0f, 1.0f)
			{
				b32 save_clicked = 0;
				new_clicked = ui_button(ui, "New##new_map");
				ui_space(ui, ui_size_specified(4.0f, 1.0f));
				load_clicked = ui_button(ui, "Load##load_map");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_clicked = ui_button(ui, "Save##save_map");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_as_clicked = ui_button(ui, "Save as##save_as_map");
				ui_space_specified(ui, 16.0f, 1.0f);
				ui_button(ui, "s##world_editor_options");
				ui_space_ppct(ui, 1.0f, 0.0f);

				ui_button(ui, "Shift##map_editor_options");
				ui_space_specified(ui, 4.0f, 1.0f);
				b32 resize_mode = ui_button(ui, "Resize##map_editor_resize");
				ui_space_specified(ui, 16.0f, 1.0f);

				if(load_clicked)
				{
					editor_resource_explorer_set_process_reestricted(
						   game_editor,
						   er_explorer_load,
						   asset_type_map,
						   "Load map");
				}
				if(save_clicked && world_editor->editing_map)
				{
					editor_world_save_map_new(editor_state);
				}
				else if(save_as_clicked || (save_clicked && !world_editor->editing_map))
				{
					er_explorer_set_process(
						   game_editor,
						   er_explorer_save,
						   "Save map as");
				}

				if(resize_mode)
				{
					world_editor->mode = world_mode_resize;
				}
			}
		}
	}

	ui_set_row(ui)
	{
		ui_space(ui, ui_size_percent_of_parent(1.0f, 0.0f));
		ui_node *space_node;
		ui_set_w_soch(ui, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
		{
			space_node = ui_create_node(ui, 0, 0);
		}
		//update and render ui for modes 
		ui_set_parent(ui, space_node) switch(world_editor->mode)
		{
			case world_mode_normal: case world_mode_selection:
				{

					ui_node *node_panel_right;

					ui_set_h_ppct(ui, 1.0f, 0.0f)
						ui_set_w_specified(ui, 300, 1.0f)
						{
							node_panel_right = ui_node_box(
									ui,
									0);
						}
					ui_set_parent(ui, node_panel_right)
					{
						u32 tileset_count       = game_editor->world.tilesets_count;
						u32 packed_assets_count = game_editor->packed_assets_count;
						u32 tileset_index        = game_editor->world.selected_tileset_for_painting_index;
						// &game_editor->panelTileset;
						//ui_content_box childPanel = ui_begin_child_panel(ui, "Child_Panel", 400, 400);
						game_assets *game_asset_manager		       = editor_state->editor_assets;
						game_resource_attributes *tileset_for_painting_info = 0;

						if(tileset_count && (tileset_index >= tileset_count))
						{
							tileset_index = tileset_count - 1;
							game_editor->world.selected_tileset_for_painting_index = tileset_index;
						}
						//set info after correcting
						if(tileset_count)
						{
							tileset_for_painting_info = game_editor->world.editing_map_tilesets[tileset_index];
						}



						//select tileset from the loaded tileset index
						ui_push_width(ui, ui_size_percent_of_parent(1.0f, 1.0f));
						ui_push_height(ui, ui_size_percent_of_parent(0.4f, 1.0f));
						ui_content_box_be(ui,
								"Tileset list")
						{
							ui_pop_width(ui);
							ui_pop_height(ui);

							for(u32 t = 0;
									t < tileset_count;
									t++)
							{
								game_resource_attributes *tilesetInfo = game_editor->world.editing_map_tilesets[t]; 

								u32 selectableActive = game_editor->world.selected_tileset_for_painting_index == t;
								ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
									ui_set_height(ui, ui_size_text(1.0f, 1.0f))
									{
										u32 selected = ui_selectable(ui, selectableActive, tilesetInfo->path_and_name);
										if(selected)
										{
											game_editor->world.selected_tileset_for_painting_index = t;
											tileset_index = t;
										}
									}

							}

							ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
								ui_set_height(ui, ui_size_text(1.0f, 1.0f))
								{
									if(ui_selectable(ui, 0, "..."))
									{
										game_editor->explorer_is_opened = 1;
										ui_explorer_set_path(ui, DATA_PATH);
										er_explorer_set_process(
												game_editor,
												er_explorer_load,
												"Add tileset to map");
									}
								}
						}

						ui_push_disable_if(ui, 
								!game_editor->world.loaded_world->tileset_count ||
								world_editor->selected_tileset_index < world_editor->tilesets_count);
						ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
						{
							ui_button(ui, "Remove tileset");
						}
						ui_pop_disable(ui);

						ui_set_height(ui, ui_size_specified(160.0f, 1.0f))
						{
							enum terrains_tab{
								tab_terrains = 0,
								tab_autoterrains = 1
							};
							u32 tab_index = 0;
							ui_tab_group(ui, &tab_index, "Terrain selection tabs!") ui_set_wh_text(ui, 4.0f, 1.0f)
							{
								ui_tab(ui, "Terrains");
								ui_tab(ui, "Autoterrains");
							}

							switch(tab_index)
							{
								case tab_terrains:
									{
										ui_node *tileset_terrains_box = 0;
										ui_set_w_ppct(ui, 1.0f, 0.0f)
											tileset_terrains_box = ui_box_with_scroll(ui, "tileset terrains");
										ui_set_parent(ui, tileset_terrains_box)
											{
												if(tileset_count)
												{
													world_tileset *tileset = editor_world_get_tileset(
															game_editor, tileset_index);
													//fix the selected tile index
													if(tileset->terrain_count)
													{
														world_editor->selected_tileset_terrain_index = 
															MIN((i32)world_editor->selected_tileset_terrain_index,
																	(i32)tileset->terrain_count - 1);
													}

													//display tileset tiles
													f32 stepping = 8.0f;
													f32 tile_display_size = 32.0f;
													f32 advance_x = 0;
													u32 t = 0;
													ui_set_w_specified(ui, tile_display_size, 1.0f) ui_set_h_specified(ui, tile_display_size, 1.0f)
														while(t < tileset->terrain_count)
														{
															ui_set_row(ui)
																while(advance_x < 3 && t < tileset->terrain_count)
																{
																	s_tileset_terrain *terrain = tileset->terrain + t;
																	//get uvs for displaying image
																	model_mesh *uvs = tileset_terrain_uvs(tileset, terrain);

																	ui_node *box = ui_create_node(ui, 0, 0);
																	ui_set_wh_specified(ui, tile_display_size, 1.0f) ui_set_parent(ui, box)
																	{
																		ui_node *image = ui_image_uvs(ui,
																				tileset->image,
																				uvs->uv0,
																				uvs->uv1,
																				uvs->uv2,
																				uvs->uv3
																				);
																		ui_set_color(ui, ui_color_background, vec4_all(0))
																		{
																			ui_node *sel_box = sel_box = ui_selectable_boxf(ui, 
																					world_editor->selected_tileset_terrain_index == t,
																					"Terrain%d", t);
																			//set index if clicked
																			if(ui_node_mouse_l_up(ui, sel_box))
																			{
																				world_editor->selected_tileset_terrain_index = t;
																				world_editor->next_tile_autoterrain = 0;
																			}

																		}
																	}
																	t++;
																	advance_x += 1;
																	ui_space_specified(ui, 4.0f, 1.0f);
																}
															ui_space_specified(ui, 4.0f, 1.0f);
															advance_x = 0;
														}

												}
											}
									}break;
								case tab_autoterrains:
									{
										ui_set_w_ppct(ui, 1.0f, 0.0f)
											ui_content_box_be_ex(
													ui,
													node_scroll_y,
													"tileset autoterrains")
											{
												if(tileset_count)
												{
													world_tileset *tileset = editor_world_get_tileset(
															game_editor, tileset_index);
													//fix the selected tile index
													if(tileset->autoterrain_count)
													{
														world_editor->selected_autoterrain_index = 
															MIN((i32)world_editor->selected_autoterrain_index,
																	(i32)tileset->autoterrain_count - 1);
													}

													//display tileset tiles
													ui_set_h_text(ui, 4.0f, 1.0f)
														for(u32 t = 0;
																t < tileset->autoterrain_count;
																t++)
														{
															if(ui_selectablef(ui,
																		world_editor->next_tile_autoterrain && 
																		world_editor->selected_autoterrain_index == t,
																		"Autoerrain %d", t))
															{
																world_editor->selected_autoterrain_index = t;
																world_editor->next_tile_autoterrain = 1;
															}
														}

												}
											}
									}break;
							}
						}

						//display selected terrain
						ui_set_column(ui) if(tileset_count)
						{
							world_tileset *tileset = editor_world_get_tileset(
									game_editor, tileset_index);
							if(!world_editor->next_tile_autoterrain)
							{
								if(world_editor->selected_autoterrain_index < 
										tileset->terrain_count)
								{
									ui_set_wh(ui, ui_size_specified(32.0f, 1.0f))
									{
										model_mesh *uvs = _tileset_terrain_uvs(tileset,
												world_editor->selected_tileset_terrain_index);
										ui_image_uvs(
												ui,
												tileset->image,
												uvs->uv0,
												uvs->uv1,
												uvs->uv2,
												uvs->uv3
												);
									}
								}
							}
							else
							{
							}
						}


					}
				}break;
			case world_mode_entities:
				{
					ui_node *node_panel_right;

					ui_set_h_specified(ui, 300, 1.0f)
						ui_set_w_specified(ui, 300, 1.0f)
						{
							node_panel_right = ui_node_box(
									ui,
									0);
						}
					ui_set_parent(ui, node_panel_right)
					{
						ui_set_wh(ui, ui_size_sum_of_children(1.0f))ui_set_row_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f) 
						{
							u32 count = world_editor->entity_count;
							u32 max = world_editor->entity_max;

							ui_text(ui, "entity_count/max : {");
							ui_textf(ui, "%u", count);
							ui_textf(ui, "/%u}", max);
						}

						ui_node *entity_data_box = 0;
						ui_set_w_ppct(ui, 1.0f, 1.0f) 
						{
							ui_set_h_text(ui, 4.0f, 1.0f)
								ui_label_ex(ui, node_text | node_text_centered, "Selected entity data");

							ui_set_h_ppct(ui, 1.0f, 0.0f)
								entity_data_box = ui_node_box(ui, "entity_data_box");
						}
						if(world_editor->entity_got_selected) ui_set_parent(ui, entity_data_box)
						{
							editor_map_entity *entity = world_editor->entities + 
								world_editor->selected_entity_index;

							ui_set_row(ui) ui_set_h_text(ui, 4.0f, 1.0f)
							{
								ui_set_w_text(ui, 4.0f, 1.0f)
									ui_text(ui, "Position");
								ui_set_w_em(ui, 8.0f, 1.0f)
								{
									ui_spinner_f32(ui, 0.1f, -10000.0f, 10000.0f, &entity->position.x, 0, "entity_position_x");
									ui_spinner_f32(ui, 0.1f, -10000.0f, 10000.0f, &entity->position.y, 0, "entity_position_y");
									ui_spinner_f32(ui, 0.1f, -10000.0f, 10000.0f, &entity->position.z, 0, "entity_position_z");
								}

							}
							ui_set_row(ui)
							{
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Type:");
								}

								u8 *entity_types[] = {
									"display",
								    "event",
								"marker"};
								i32 types_count = ARRAYCOUNT(entity_types);
								u8 *entity_type_name = entity->type >= types_count ?
									"UNDEFINED" : entity_types[entity->type];

								ui_id entity_types_popup_id = ui_id_from_string("entity_types_popup");
								ui_set_wh_text(ui, 4.0f, 1.0f)
								if(ui_drop_down_beginf(ui, entity_types_popup_id, "%s##entity_type_drop_down", entity_type_name))
								{
									ui_set_w_em(ui, 8.0f, 1.0f)
									for(i32 t = 0; t < types_count; t++)
									{
										if(ui_selectablef(ui, entity->type == t,
													"%s##entity_type_selection_popup%u", entity_types[t], t))
										{
											entity->type = t;
										}
									}
								}
								ui_drop_down_end(ui);


							}
							ui_set_wh_text(ui, 4.0f, 1.0f)
							{
								ui_textf(ui, "Model:");
								u8 *model_name = "-";
								ui_id model_list_popup_id = ui_id_from_string("map_entity_set_model");

								if(world_editor->selected_entity_index < world_editor->entity_count)
								{
									editor_map_entity *ent = world_editor->entities +
										world_editor->selected_entity_index;
									ui_set_w_specified(ui, 200, 1.0f) ui_set_h_specified(ui, 300, 1.0f)
										ui_popup(ui, model_list_popup_id)
										{
											ui_content_box_be_ex(
													ui,
													node_scroll_y,
													"map_entity_model_resources")
											{
												s_game_resource_editor *re = &game_editor->asset;
												for(game_resource_attributes *r = re->first;
														r;
														r = r->next)
												{
													if(r->type == asset_type_model)
													{
														ui_set_wh_text(ui, 4.0f, 1.0f)
															if(ui_selectable(ui, 0, r->path_and_name))
															{
																editor_world_set_model_to_entity(
																		game_editor,
																		ent,
																		r);
																ui_popup_close(ui,model_list_popup_id);
															}
													}
												}
											}
										}
									if(ent->model)
									{
										model_name = ent->model->path_and_name;
									}
								}
								ui_node *button = 0;
								ui_set_w_ppct(ui, 0.9f, 1.0f)
									button = ui_button_nodef(ui, "%s##map_entity_model_dd", model_name);
								if(ui_node_mouse_l_up(ui, button))
								{
									ui_popup_open(ui,
											button->region.x0,
											button->region.y1,
											model_list_popup_id);
								}

							}
						}
					}


					ui_set_column(ui)
						ui_space_ppct(ui, 1.0f, 0.0f);
				}break;
			case world_mode_resize:
				{
					ui_node *node_panel_right;

					ui_set_wh_soch(ui, 1.0f)
					{
						node_panel_right = ui_node_box(
								ui,
								0);
					}
					ui_set_parent(ui, node_panel_right)
					{
						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_text(ui, "Map size:");
						}
						ui_set_h_em(ui, 2.0f, 1.0f) ui_set_w_em(ui, 12.0f, 1.0f)
						{
							u32 w = game_editor->world.new_map_w;
							u32 h = game_editor->world.new_map_h;
							ui_spinner_u32(ui,
									1,
									1,
									10000,
									&w,
									0,
									"new_map_w");

							ui_spinner_u32(ui,
									1,
									1,
									10000,
									&h,
									0,
									"new_map_h");
							game_editor->world.new_map_w = w;
							game_editor->world.new_map_h = h;
						}

						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_textf(ui, "Current maximum terrain capacity: %u", game_editor->world.map_terrain_max);
							u32 total_terrain_needed = 
								game_editor->world.new_map_w * game_editor->world.new_map_h;
							b32 more_terrain_allocation_needed = total_terrain_needed >= game_editor->world.map_terrain_max;
							ui_set_row(ui)
							{
								ui_text(ui, "Occuppied by new sizes:");
								if(more_terrain_allocation_needed) ui_set_color(ui, ui_color_background, V4(255, 0, 0, 255))
								{
									ui_textf(ui, "%u", total_terrain_needed);
								}
								else
								{
									ui_textf(ui, "%u", total_terrain_needed);
								}
							}
							ui_set_row(ui)
							{
								ui_push_disable_if(ui, more_terrain_allocation_needed);
								if(ui_button(ui, "Confirm##map_size"))
								{
									editor_world_change_map_size(
											game_editor);
								}
								ui_pop_disable(ui);
								ui_space_specified(ui, 4.0f, 1.0f);
								//ui_same_line(ui);
								if(ui_button(ui, "Cancel"))
								{
									game_editor->world.new_map_w = game_editor->world.map_w;
									game_editor->world.new_map_h = game_editor->world.map_h;
									game_editor->world.mode = world_mode_normal;
								}
							}
						}

						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_text(ui, "Shift amount:");
						}
						ui_set_h_em(ui, 2.0f, 1.0f) ui_set_w_em(ui, 12.0f, 1.0f)
						{
							u32 w = game_editor->world.shift_x;
							u32 h = game_editor->world.shift_y;
							b32 redraw = 
							ui_spinner_i32(ui, 1, -10000, 10000, &w, 0, "new_map_shift_x");
							redraw |= ui_spinner_i32(ui,1,-10000, 10000, &h, 0, "new_map_shift_y");
							game_editor->world.shift_x = w;
							game_editor->world.shift_y = h;

							//redraw if the shift value changes
							if(redraw)
							{
								world_editor->draw_locked = 0;
							}
						}
						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_checkbox(ui, &world_editor->resize_after_shift, "Resize after shift");
							ui_set_row(ui)
							{
								//								ui_push_disable_if(ui, more_terrain_allocation_needed);
								if(ui_button(ui, "Confirm##map_shift_confirm"))
								{
									editor_world_shift_map(game_editor);
									game_editor->world.shift_x = 0;
									game_editor->world.shift_y = 0;
									world_editor->draw_locked = 0;
								}
								//								ui_pop_disable(ui);
								ui_space_specified(ui, 4.0f, 1.0f);
								//ui_same_line(ui);
								if(ui_button(ui, "Cancel##map_shift_cancel"))
								{
									game_editor->world.shift_x = 0;
									game_editor->world.shift_y = 0;
									world_editor->draw_locked = 0;
								}
							}
						}
					}
				}break;
			case world_mode_harea:
				{
					ui_node *node_panel_right;

					ui_set_h_specified(ui, 300, 1.0f)
						ui_set_w_specified(ui, 300, 1.0f)
						{
							node_panel_right = ui_node_box(
									ui,
									0);
						}
					ui_set_parent(ui, node_panel_right) ui_set_wh_text(ui, 4.0f, 1.0f)
					{
						if(world_editor->harea_is_selected)
						{
							editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) + 
								world_editor->selected_harea_index;
							ui_checkbox(ui, &world_editor->harea_grid_edit, "Grid edit");
							ui_text(ui, "x, y, z");
							ui_set_w_em(ui, 6.0f, 1.0f)
							{
								ui_spinner_f32(ui, 1, -1000, 1000, &harea->base.x, 0, "harea_spinner_x");
								ui_spinner_f32(ui, 1, -1000, 1000, &harea->base.y, 0, "harea_spinner_y");
								ui_spinner_f32(ui, 1, -1000, 1000, &harea->base.z, 0, "harea_spinner_z");
							}
							ui_text(ui, "w, h");
							ui_set_w_em(ui, 6.0f, 1.0f)
							{
								u16 w = harea->base.w;
								u16 h = harea->base.h;
								ui_spinner_u16(ui, 1, 0, 1000, &w, 0, "harea_spinner_w");
								ui_spinner_u16(ui, 1, 0, 1000, &h, 0, "harea_spinner_h");
								if(harea->base.w != w || harea->base.h != h)
								{
									memory_dyarray_set_count(harea->tiles, w * h);
									harea->base.w = w;
									harea->base.h = h;

								}
							}
							switch(harea->base.type)
							{
								//animated tiles
								case 0:
									{
									}break;
									//models
								case 1:
									{
										if(ui_button(ui, "generate points"))
										{
											world_editor->generate_harea_poisson = 1;
										}
										//add and remove buttons
										b8 add = 0;
										b8 rem = 0;
										ui_set_row(ui)
										{
											ui_text(ui, "Models: ");
											ui_set_wh_em(ui, 1.0f, 1.0f)
											{
												add = ui_button(ui, "+#harea_model");
												ui_push_disable_if(ui, !world_editor->harea_model_selection.selected);
												rem = ui_button(ui, "x#harea_model");
												ui_pop_disable(ui);

											}
										}

										//display and select harea models
										ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_soch(ui, 1.0f)
										for(u32 m = 0; m < harea->model_count; m++)
										{
											ui_push_id_u32(ui, m);
											ui_node *box = ui_focus_box(ui, eui_selection_selected(world_editor->harea_model_selection, m), "harea_model_focus_box");
											//focus if selected
											if(ui_node_mouse_l_pressed(ui, box))
											{
												eui_selection_select(&world_editor->harea_model_selection, m);
											}
											ui_set_parent(ui, box) ui_set_wh_text(ui, 4.0f, 1.0f)
											{
												//get harea model data
												editor_map_harea_model *harea_model_data = 0;
												memory_dyarray_get_safe(harea->models, harea_model_data, m);
#if 0
												if(harea_model->model)
												{
													u8 *name = harea_model->model->path_and_name;
													ui_textf(ui, "Model: %s", name);
													ui_set_row(ui)
													{
														ui_text(ui, "weight");
														ui_spinner_u32(ui, 1, 0, 1000, &harea_model->weight, 0, "harea_model_weight");
													}
													ui_set_row(ui)
													{
														ui_text(ui, "radius");
														ui_spinner_f32(ui, .1f, 0, 1000, &harea_model->radius, 0, "harea_model_radius");
													}
												}
#else
												{
													editor_harea_model_slot *harea_model = 0;
													memory_dyarray_get_safe(harea->external_models, harea_model, harea_model_data->model_index);
													u8 *name = harea_model->r->path_and_name;
													ui_textf(ui, "Model: %s", name);
													ui_set_row(ui)
													{
														ui_text(ui, "weight");
														ui_spinner_u16(ui, 1, 0, 1000, &harea_model_data->weight, 0, "harea_model_weight");
													}
													ui_set_row(ui)
													{
														ui_text(ui, "radius");
														ui_spinner_f32(ui, .1f, 0, 1000, &harea_model_data->radius, 0, "harea_model_radius");
													}
												}
#endif
											}
											ui_pop_id(ui);
										}


										if(add)
										{
											editor_resource_explorer_set_process_reestricted(
													game_editor,
													er_explorer_load,
													asset_type_model,
													ew_harea_model_add_PROCESS);
										}
										else if(rem)
										{
											ew_remove_model_from_harea(
													game_editor,
													harea,
													world_editor->harea_model_selection.index);
										}


									}
							}
						}
					}
				}break;
			default:
				{
					ui_set_column(ui)
						ui_space_ppct(ui, 1.0f, 0.0f);
				}break;
		}
	}

	ui_node *map_name_bottom_bar;
	ui_node *bottom_bar;
	ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
	ui_set_w_ppct(ui, 1.0f, 1.0f)
	{
		map_name_bottom_bar = ui_create_node(
				ui,
				node_background,
				0);
		bottom_bar = ui_create_node(
				ui,
				node_background,
				0);
	}
	//display map name
	ui_set_parent(ui, map_name_bottom_bar)
	{
		if(world_editor->editing_map)
		{
			ui_text(ui, world_editor->editing_map->path_and_name);
		}
	}
	ui_set_parent(ui, bottom_bar)
	{
		ui_set_row(ui)
		{
			u8 *current_mode_name[] = {
				"-- Normal --",
				"-- Selection --",
				"-- Entity --",
				"-- Shift --",
				"-- Resize --",
				"-- Harea --",
				"-- View --",
			};
			ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
			ui_set_text_color(ui, V4(255, 255, 0, 255))
			{
				ui_text(ui, current_mode_name[game_editor->world.mode]);
			}
			ui_space_ppct(ui, 1.0f, 0.0f);
			ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
			{
				ui_textf(ui, "ray_tile_position : {%f, %f, %f}",
						world_editor->ray_tile_position.x,
						world_editor->ray_tile_position.y,
						world_editor->ray_tile_position.z);
				u32 ray_tile_index = world_editor->ray_tile_index;
				u32 ray_tile_x = ray_tile_index % world_editor->map_w;
				u32 ray_tile_y = ray_tile_index / world_editor->map_w;
				ui_textf(ui, "ray_tile_index: %u, {%u, %u}", 
						ray_tile_index, ray_tile_x, ray_tile_y);
				ui_space_specified(ui, 4.0f, 1.0f);
				if(game_editor->world.ray_hits_tile)
				{
					//show tile data
					world_tile *tile = game_editor->world.terrain + game_editor->world.ray_tile_index;
					ui_textf(ui, "is_autoterrain: %u", tile->is_autoterrain);
					ui_space_specified(ui, 4.0f, 1.0f);
					ui_textf(ui, "autoterrain_index: %u", tile->autoterrain_index);
					ui_textf(ui, "terrain_index: %u", tile->tileset_terrain_index);

				}
			}

		}
	}
	//new map context menu
	ui_set_wh(ui, ui_size_sum_of_children(0.0f))
	ui_popup(ui, new_map_dialogue_id)
	{
		ui_node *region;
		ui_set_height(ui, ui_size_sum_of_children(1.0f)) ui_set_width(ui, ui_size_specified(100, 1.0f))
		{
			region = ui_node_box(ui, "new_map_node_box");
		}
		ui_set_parent(ui,region)
		{

			ui_next_nodes_interaction_only_begin(ui);
			ui_set_wh_text(ui, 4.0f, 1.0f)
			{
				ui_text(ui, "Start new map?");
				ui_set_row(ui)
				{
					if(ui_button(ui, "Confirm#__new__map__"))
					{
						editor_world_reset(game_editor);
						ui_popup_close(ui, new_map_dialogue_id);
					}
					ui_space_specified(ui, 4.0f, 1.0f);
					if(ui_button(ui, "Cancel##calcel_new"))
					{
						ui_popup_close(ui, new_map_dialogue_id);
					}
				}
			}
			ui_next_nodes_interaction_only_end(ui);
		}
	}
	//new plane contenxt_menu
#if 0
	ui_popup(ui, add_plane_popup_id)
	{
		ui_next_nodes_interaction_only_begin(ui);
		ui_node *region_node = 0;
		ui_node *interact_area = 0;
		ui_set_wh_soch(ui, 1.0f)
		{
			interact_area = ui_mid_interact(ui, "Add_plane_interacion_area");
			ui_set_parent(ui, interact_area)
			{
				region_node = ui_create_node(ui, 0, 0);
			}
		}
		ui_set_parent(ui, region_node)
		{
			ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
			{
				if(ui_selectable(ui, 0, "Animated tiles#add_plane"))
				{
					ui_popup_close(ui, add_plane_popup_id);
				}
				if(ui_selectable(ui, 0, "Models#add_plane"))
				{
					ui_popup_close(ui, add_plane_popup_id);
				}
				if(ui_selectable(ui, 0, "Stay open!#add_plane"))
				{
				}
			}
		}
		if(!ui_node_mouse_hover(ui, interact_area) && input_pressed(ui->input.mouse_left))
		{
			ui_popup_close(ui, add_plane_popup_id);
		}
		ui_next_nodes_interaction_only_end(ui);
	}
#else
	ui_set_wh_soch(ui, 1.0f)
	ui_context_menu(ui, add_plane_popup_id)
	{
		ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
		{
			if(ui_selectable(ui, 0, "Animated tiles#add_plane"))
			{
				ui_popup_close(ui, add_plane_popup_id);
				world_editor->harea_to_add_type = 0;
				world_editor->add_harea = 1;
				editor_world_add_animated_tiles_harea(game_editor,
						(f32)(i32)world_editor->cached_ray_position.x,
						(f32)(i32)world_editor->cached_ray_position.y,
						(f32)(i32)world_editor->cached_ray_position.z);
			}
			if(ui_selectable(ui, 0, "Models#add_plane"))
			{
				ui_popup_close(ui, add_plane_popup_id);
				world_editor->add_harea = 1;
				editor_world_add_model_harea(game_editor,
						(f32)(i32)world_editor->cached_ray_position.x,
						(f32)(i32)world_editor->cached_ray_position.y,
						(f32)(i32)world_editor->cached_ray_position.z);
			}
			if(ui_selectable(ui, 0, "Stay open!#add_plane"))
			{
			}
		}
	}
#endif

	if(new_clicked)
	{
		ui_popup_open(ui,
				500,
				500,
				new_map_dialogue_id);
	}
	//explorer processes
#if 0
	if(ui_explorer_check_process(ui, "Add tileset to map"))
	{
		u8 *tileset_name = ui->explorer->full_process_path_and_name;
		editor_world_add_tileset(game_editor,
				game_asset_manager,
				tileset_name);
	}
	if(ui_explorer_check_process(ui, "Load map"))
	{
		u8 *name = ui->explorer->full_process_path_and_name;
		editor_world_load_map(
				editor_state,
				name);
	}
	if(ui_explorer_check_process(ui, "Save map"))
	{
		u8 *name = ui->explorer->full_process_path_and_name;
		editor_world_save_map_old(
				editor_state,
				name);
	}
#endif

	if(editor_resource_explorer_process_completed(game_editor, "Add tileset to map"))
	{
		u8 *name = editor_resource_explorer_output(game_editor);
		editor_world_reference_and_add_tileset(game_editor, name);
	}
	if(editor_resource_explorer_process_completed(game_editor, "Load map"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		editor_world_load_map(
				editor_state,
				name);
	}
	if(editor_resource_explorer_process_completed(game_editor, "Save map as"))
	{
		u8 *save_path = editor_resource_explorer_output(game_editor);
		game_resource_attributes *new_editing_map = editor_resource_create_and_save(
				editor_state, asset_type_map, 1, save_path);
		if(new_editing_map)
		{
			world_editor->editing_map = new_editing_map;
			editor_world_save_map_new(
					editor_state);
			editor_resources_reimport(editor_state, world_editor->editing_map);
		}
	}
	//harea model
	if(editor_resource_explorer_process_completed(game_editor, ew_harea_model_add_PROCESS) &&
			world_editor->harea_is_selected)
	{
		editor_map_harea *harea = editor_array_base(world_editor->hareas, editor_map_harea) +
			world_editor->selected_harea_index;
		ew_add_model_to_harea(
				game_editor,
				harea,
				er_explorer_output(game_editor));
	}

	if(world_editor->open_add_context_menu)
	{
		world_editor->open_add_context_menu = 0;
		i16 x = (i16)ui->mouse_point.x;
		i16 y = (i16)ui->mouse_point.y;
		ui_popup_open(ui, x, y, add_plane_popup_id);
	}
}
