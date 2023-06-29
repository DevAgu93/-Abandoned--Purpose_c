static void
editor_world_reset(s_game_editor *game_editor)
{
	world_tile *worldTerrain = game_editor->world.loaded_world->tiles;
	world_tile emptyTerrain = {0};
	for(u32 w = 0; w < game_editor->world.loaded_world->tile_count; w++)
	{
		worldTerrain[w] = emptyTerrain;
	}

	game_editor->world.loaded_world->tile_count = 0;
	game_editor->world.loaded_world->entity_count  = 0;
	game_editor->world.loaded_world->tileset_count = 0;
	game_editor->world.cursor_memory.selected_meshes_count      = 0;

	game_editor->world.history_buffer_used = 0;
	game_editor->world.history_count       = 0;
	game_editor->world.history_cursor      = 0;
	game_editor->world.world_tilesets_count = 0;
}
//TODO(Agu) separate texture from every piece of terrain? use index instead?
static void
editor_world_load_map(s_editor_state *editor_state,
		              s_game_editor *game_editor,
			          platform_api *platform,
			          uint8 *path_and_name)
{
	game_assets *game_asset_manager = editor_state->editor_assets;

	platform_file_handle map_file_handle = platform->f_open_file(
			path_and_name,
			platform_file_op_read);

	stream_data *info_stream = &game_editor->info_stream;

	if(map_file_handle.handle)
	{
	    
		editor_world_reset(game_editor);
        game_world *editor_world = game_editor->world.loaded_world;

		//get header to read signature and version, then tilesets
        ppse_map_file_header map_file_header = {0};

        platform->f_read_from_file(map_file_handle,
	    		                      0,
	    					          sizeof(ppse_map_file_header),
	    					          &map_file_header);

	    u32 signature_check = map_file_header.signature == (ppse_map_SIGNATURE);
	    u32 version_check   = map_file_header.version == ppse_map_version;
		u32 tile_count      = map_file_header.tile_count;

		if(signature_check && version_check)
		{

			u32 tile_data_offset = sizeof(ppse_map_file_header);
			game_editor->world.tile_count = tile_count;
			for(u32 t = 0;
					t < tile_count;
					t++)
			{
				world_tile *e_t = game_editor->world.world_tiles + t;
				ppse_world_tile file_tile = {0};
				//read tile data
				platform->f_read_from_file(
						map_file_handle,
						tile_data_offset,
						sizeof(ppse_world_tile),
						&file_tile);
                tile_data_offset += sizeof(ppse_world_tile);

				//set data
				e_t->v0 = file_tile.v0;
				e_t->v1 = file_tile.v1;
				e_t->v2 = file_tile.v2;
				e_t->v3 = file_tile.v3;

				e_t->uv0 = file_tile.uv0;
				e_t->uv1 = file_tile.uv1;
				e_t->uv2 = file_tile.uv2;
				e_t->uv3 = file_tile.uv3;

				e_t->tileset_index = file_tile.tileset_index;
				//e_t->animated = file_tile.animated;
				//e_t->tileset_animation_index = file_tile.tileset_animation_index;

			}
			//get tilesets and load them optionally

			u32 map_tilesets_count      = map_file_header.tileset_count;
			u32 offset_to_tileset_paths = map_file_header.offset_to_tileset_paths;
			u32 current_path_offset     = offset_to_tileset_paths;

			for(u32 t = 0;
					t < map_tilesets_count;
					t++)
			{
				u32 tileset_path_and_name_length = 0;
				u8 path_and_name_buffer[256]     = {0};
				//read name length
				platform->f_read_from_file(
						map_file_handle,
						current_path_offset,
						sizeof(u32),
						&tileset_path_and_name_length);
                current_path_offset += sizeof(u32);

				//read name
				platform->f_read_from_file(
						map_file_handle,
						current_path_offset,
						tileset_path_and_name_length,
						&path_and_name_buffer);
				current_path_offset += tileset_path_and_name_length;

				assets_load_and_get_tileset(
						game_asset_manager,
						path_and_name_buffer);
				//tileset was successfuly loaded
				if(!editor_world_add_tileset(
						game_editor, game_asset_manager, path_and_name_buffer))
				{
#if 0



					//fill the needed data to know about the unloaded tileset anyways
					string_copy(
							path_and_name_buffer,
							game_editor->world.world_tilesets[t].path_and_name);
#endif
					stream_pushf(info_stream, "Error while reading one of the map's tileset file: %s."
							     "please check if it exists or contains a valid version", path_and_name_buffer);

				}
				else
				{
				}


				Assert(tileset_path_and_name_length < 256)

			}


		}
		else
		{
			if(!signature_check)
			{
				stream_pushf(
						info_stream,
						"Error while opening the file %s as a map file. Signature check failed.",
						path_and_name);
			}
			else
			{
				stream_pushf(
						info_stream,
						"Error while opening the file %s as a map file. Version check failed, got %d but expected %d",
						path_and_name,
						map_file_header.version,
						ppse_map_version);
			}
		}

		platform->f_close_file(map_file_handle);
	}
}

static void
editor_world_save_map(s_game_editor *game_editor,
		          game_assets *game_asset_manager,
				  platform_api *platform,
				  u8 *path_and_name)
{
	//for log
	stream_data *info_stream = &game_editor->info_stream;


   game_world *world  = game_editor->world.loaded_world;
   u32 tile_count     = world->tile_count;
   u32 colliderCount  = world->colliders_count;
   u32 entity_count   = world->entity_count;
   u32 tileset_count  = world->tileset_count;

   world_collider *colliderArray = world->colliders;
   world_entity *entityArray     = world->entities;
   world_tile *terrainArray      = world->tiles;

   //uint8 *path_and_name	  = "Data/MAP0.level";	
   ppse_map_file_header map_file_header = {0};

   //Write data
   map_file_header.signature     = (ppse_map_SIGNATURE);
   map_file_header.tile_count    = tile_count;
   map_file_header.colliderCount = colliderCount;
   map_file_header.entity_count  = entity_count;
   map_file_header.tileset_count = tileset_count;
   map_file_header.version       = ppse_map_version;

   platform_file_handle mapFile = platform->f_open_file(path_and_name, platform_file_op_CreateNew);
   //check
   colliderCount = 0;

   if(mapFile.handle)
   {

       //Offset to data
       uint32 data_offset = sizeof(ppse_map_file_header);

	   for(u32 t = 0; t < tile_count; t++)
	   {	
            world_tile *terrainFromArray = (terrainArray + t);
            ppse_world_tile terrainData = {0};


            terrainData.tileset_index   = terrainFromArray->tileset_index;
		    terrainData.v0             = terrainFromArray->v0;
		    terrainData.v1             = terrainFromArray->v1;
		    terrainData.v2             = terrainFromArray->v2;
		    terrainData.v3             = terrainFromArray->v3;

			terrainData.uv0 = terrainFromArray->uv0;
			terrainData.uv1 = terrainFromArray->uv1;
			terrainData.uv2 = terrainFromArray->uv2;
			terrainData.uv3 = terrainFromArray->uv3;

		    platform->f_write_to_file(mapFile ,data_offset, sizeof(ppse_world_tile), &terrainData);
		    data_offset += sizeof(ppse_world_tile);

	   }
       //write collidables data
       for(u32 collisionI = 0; collisionI < colliderCount; collisionI++)
       {
           world_collider *collider = colliderArray + collisionI;

           uint32 data_size = sizeof(world_collider);
           platform->f_write_to_file(mapFile, data_offset, data_size, collider);
           data_offset += data_size;
       }

	   //Entities
	   for(u32 e = 0; e < entity_count; e++)
	   {
		   //ppse_entity fileEntity = {0};
		   //world_entity *entity = entityArray + e;

		   //fileEntity.velocity          = entity->velocity;
		   //fileEntity.collision_size    = entity->collision_size;
		   //fileEntity.collision_offset  = entity->collision_offset;
		   //fileEntity.position          = entity->position;
		   //fileEntity.model_id          = entity->model_id;
		   //fileEntity.looking_direction = entity->looking_direction;

		   ////platform_write_struct_advance(platform, mapFile, &data_offset, world_entity, &fileEntity);
		   //platform->f_write_to_file(mapFile, data_offset, sizeof(ppse_entity), &fileEntity);
		   //data_offset += sizeof(ppse_entity);


	   }


	   //write header at the end with the tileset paths
       map_file_header.offset_to_tileset_paths = data_offset;
       //Write header
       platform->f_write_to_file(mapFile, 0, sizeof(ppse_map_file_header), &map_file_header);

	   //first write the length of the tileset path and name,
	   //then the path and name
	   for(u32 t = 0;
			   t < game_editor->world.world_tilesets_count;
			   t++)
	   {
		   u32 tileset_file_length = string_count(
				   game_editor->world.world_tilesets[t].path_and_name);

		   //name length
		   platform->f_write_to_file(
				   mapFile,
				   data_offset,
				   sizeof(u32),
				   &tileset_file_length);

		   data_offset += sizeof(u32);
		   platform->f_write_to_file(
				   mapFile,
				   data_offset,
				   tileset_file_length,
				   game_editor->world.world_tilesets[t].path_and_name);

		   stream_pushf(&game_editor->info_stream, "Saved tileset with name \"%s\" and length %d",
				   game_editor->world.world_tilesets[t].path_and_name,
				   tileset_file_length);
		   //advance by name length
		   data_offset += tileset_file_length;
	   }


	   stream_pushf(&game_editor->info_stream, "Saved map to: %s", path_and_name);
       platform->f_close_file(mapFile);
   }
   else //invalid file
   {
	   stream_pushf(&game_editor->info_stream, "Unable to save map to: %s", path_and_name);
   }
}

static void
editor_world_update_render_tiles(s_game_editor *game_editor, game_assets *game_asset_manager, render_commands *commands)
{

	game_world *gameWorld = game_editor->world.loaded_world;

    //
	//Draw every terrain
	//

	u32 tile_count = game_editor->world.tile_count;

    if(tile_count)
	{

        //game_editor->world.drawLocked = 0;
	    if(!game_editor->world.drawLocked)
	    {
	    	//render_refresh_locked_vertices(commands);
	    }

	    //render_push_locked_vertices(commands);
		world_tile *gridTiles = gameWorld->tiles;

		if(!game_editor->world.drawLocked) //or if the renderer needs a lock update
		{
		    //game_editor->world.drawLocked = 1;

	        //u32 tile_count = 10;

            for(u32 t = 0; t < tile_count; t++) 
            {
                world_tile *current_tile = gridTiles + t;
				u32 tileset_index = current_tile->tileset_index;
	        	//render_texture *terrainTexture = gameWorld->tilesets[terrain->tileset_index].image; 
		    	asset_tileset asset_tileset = game_editor->world.tilesets_as_asset[tileset_index];
		    	//;Clean this
		    	if(asset_tileset.key.id == assets_NULL_ID)
		    	{
		    		continue;
		    	}


		    	world_tileset_tile *tileset_tile_array = asset_tileset.tileset->tiles;

		    	asset_image tilesetTileImage       = assets_get_texture_by_id(game_asset_manager, asset_tileset.tileset->imageId);
	        	render_texture *tilesetTileTexture = tilesetTileImage.image;

	        	vec3 hOffX = {0}; 

				vec3 v0 = current_tile->v0;
				vec3 v1 = current_tile->v1;
				vec3 v2 = current_tile->v2;
				vec3 v3 = current_tile->v3;

				vec2 uv0 = current_tile->uv0;
				vec2 uv1 = current_tile->uv1;
				vec2 uv2 = current_tile->uv2;
				vec2 uv3 = current_tile->uv3;

				if(current_tile->animated)
				{
                    world_tileset_animation *tileset_animation = asset_tileset.tileset->animated_tiles + current_tile->tileset_animation_index;
				    u16 tile_uvs_index = tileset_animation->frames_current;

				    world_tileset_tile *current_tile_uvs = tileset_tile_array + tile_uvs_index;

					uv0 = current_tile_uvs->uv0;
					uv1 = current_tile_uvs->uv1;
					uv2 = current_tile_uvs->uv2;
					uv3 = current_tile_uvs->uv3;
				}

#if 0
                render_push_quad(commands,
                                 tilesetTileTexture,
                                 v0,
                                 v1,
                                 v2,
                                 v3,
                                 uv0,
                                 uv1,
                                 uv2,
                                 uv3,
                                 vec4_all(255) );
#endif
                render_draw_sprite_up0(commands,
                                 tilesetTileTexture,
                                 v0,
                                 v1,
                                 v2,
                                 v3,
                                 uv0,
                                 uv1,
                                 uv2,
                                 uv3);
	        							 

            }

		}
		//render_pop_locked_vertices(commands);
	}

}

inline void
editor_world_delete_entity(s_game_editor *game_editor, u32 index)
{
	game_world *gameWorld = game_editor->world.loaded_world;
	u32 entity_count       = gameWorld->entity_count;
	if(entity_count)
	{
		world_entity *entityArray = gameWorld->entities;
		u32 e = index;
		while(e < entity_count - 1)
		{
			entityArray[e] = entityArray[e + 1];
			e++;
		}

		gameWorld->entity_count--;
	}

}

static void
editor_world_update_render_entities(s_game_editor *game_editor,
		                            game_assets *game_asset_manager,
									render_commands *commands,
									f32 dt)
{
	game_world *gameWorld = game_editor->world.loaded_world;
	memory_area *bones_area = &game_editor->area;

	//update and render entities
	for(u32 e = 0;
			e < gameWorld->entity_count;
			e++)
	{

		world_entity *entity    = gameWorld->entities + e;
		asset_model entityModel = entity->model_key;

		asset_model_animation entity_animation_key = entity->animation_key;

		if(entityModel.key.id)
		{
			sprite_model *loaded_model   = assets_get_model_by_key(game_asset_manager, entityModel);
			s_asset_animation *animations = assets_get_model_animations_by_key(game_asset_manager, entity_animation_key);
			assets_fill_model_animations_from_asset(&entity->animation, animations);
			u32 bone_count               = entityModel.model->bone_count;
			u32 sprite_count             = entityModel.model->sprite_count;

			temporary_area temporaryModelArea = temporary_area_begin(bones_area);

			sprite_model animated_meshes = render_allocate_bind_model(
					bones_area, loaded_model);

#if 0
		    model_update_render_animated(commands,
		    							 editor_state->editor_assets,
		    							 &entityModel->model,
		    							 loaded_model,
										 animated_meshes,
		    							 &entity->animation,
		    							 entity->position,
		    							 0,
		    							 1,
		    							 dt);
#else

	        for(u32 b = 0; b < sprite_count; b++)
	        {
	        	animated_meshes.sprites[b] = loaded_model->sprites[b];
	        }
	        for(u32 b = 0; b < bone_count; b++)
	        {
	        	animated_meshes.bones[b] = loaded_model->bones[b];
	        }
	        for(u32 b = 0; b < loaded_model->uvs_count; b++)
	        {
	        	animated_meshes.uvs[b] = loaded_model->uvs[b];
	        }


			model_render(commands->gameRenderer,
					            commands,
								game_asset_manager,
								*loaded_model,
								animated_meshes,
								entity->position,
							game_editor->model.model_foward	
								);
#endif
			temporary_area_end(&temporaryModelArea);
		}

	}

}

typedef struct
{
	u32 x;
	u32 y;
}vec2_u32;

#if 0
static void
editor_world_update_bucket(s_game_editor *game_editor, world_grid *grid, world_tile originalTile, i32 hittedGridIndex)
{
	i32 tileCapacity = grid->width * grid->height;
	
    if(hittedGridIndex > 0 && hittedGridIndex < tileCapacity)
	{
	    u32 currentTilesetIndex    = game_editor->world.selected_tileset_for_painting_index;
	    u32 currentSelectedTile_x0 = game_editor->world.selectedTile_x0;

	    world_tile *clickedTile = grid->tiles + hittedGridIndex;
		world_tile *tileArray = grid->tiles;
#define ORIGINAL_TILE(t) (t.tileset_index == originalTile.tileset_index && t.tileArrayIndex == originalTile.tileArrayIndex)
#define GRID_TILE_EQUALS(t0, t1) ((t0.tileset_index == t1.tileset_index) && (t0.tileArrayIndex == t1.tileArrayIndex))

	    u32 same_tile_as_original = clickedTile->tileset_index == originalTile.tileset_index &&
	    	                        clickedTile->tileArrayIndex == originalTile.tileArrayIndex;

	    u32 same_tile_as_selected = clickedTile->tileset_index == currentTilesetIndex &&
	    	                        clickedTile->tileArrayIndex == currentSelectedTile_x0;

	    if(same_tile_as_original && !same_tile_as_selected)
	    {
			u32 at_x = hittedGridIndex % grid->width;
			u32 at_y = hittedGridIndex / grid->width;


	        temporary_area tempArea = temporary_area_begin(&game_editor->area);

			//;Cleanup
			//vec2_u32 *locationStack = memory_area_push_array(&game_editor->area ,vec2_u32, 400);
			u32 locationStackCount  = 1;
			//locationStack[0].x = at_x;
			//locationStack[0].y = at_y;
			vec2_u32 *firstTile = memory_area_push_struct(&game_editor->area, vec2_u32);

			firstTile->x = at_x;
			firstTile->y = at_y;
			u32 lStackAt = 1;
			while(locationStackCount--)
			{
				vec2_u32 tile_xy= *memory_area_PopStruct(&game_editor->area, vec2_u32);

				i32 current_x = tile_xy.x;
				i32 current_y = tile_xy.y;
				i32 x0 = current_x;
				i32 indexAt = x0 + (current_y * grid->width);
			    u32 checkedAbove = 0;
			    u32 checkedBelow = 0;

				while(x0 >= 0 && GRID_TILE_EQUALS(tileArray[indexAt], originalTile))
				{
					x0--;
					indexAt = x0 + (current_y * grid->width);
				}
				x0++;

				while(x0 < (i32)grid->width)
				{
				    i32 current_index = x0 + (current_y * grid->width);

				    grid->tiles[current_index].tileset_index   = currentTilesetIndex;
				    grid->tiles[current_index].tileArrayIndex = currentSelectedTile_x0;

	    	        i32 index_d = current_index + grid->width;
	    	        i32 index_u = current_index - grid->width;

					if(!checkedAbove && (index_u > 0))
					{

						world_tile tileAt = grid->tiles[index_u];
	                    u32 same_tile_as_original = GRID_TILE_EQUALS(tileAt, originalTile);
						if(same_tile_as_original)
						{
							vec2_u32 *newLocation = memory_area_push_struct(&game_editor->area, vec2_u32);
							newLocation->x = x0;
							newLocation->y = current_y - 1;
							locationStackCount++;

							checkedAbove = 1;

						}
						
					}
					else if(checkedAbove && (index_u > 0))
					{

						world_tile tileAt = grid->tiles[index_u];
	                    u32 same_tile_as_original = GRID_TILE_EQUALS(tileAt, originalTile);
						if(!same_tile_as_original)
						{
							checkedAbove = 0;
						}
					}

					if(!checkedBelow && (index_d < tileCapacity))
					{

						world_tile tileAt = grid->tiles[index_d];
	                    u32 same_tile_as_original = GRID_TILE_EQUALS(tileAt, originalTile);
						if(same_tile_as_original)
						{
							vec2_u32 *newLocation = memory_area_push_struct(&game_editor->area, vec2_u32);
							newLocation->x = x0;
							newLocation->y = current_y + 1;
							locationStackCount++;

							checkedBelow = 1;

						}
						
					}
					if(checkedBelow && (index_d < tileCapacity))
					{

						world_tile tileAt = grid->tiles[index_d];
	                    u32 same_tile_as_original = GRID_TILE_EQUALS(tileAt, originalTile);
						if(!same_tile_as_original)
						{
							checkedBelow = 0;
						}
					}

					x0++;
					if(x0 < (i32)grid->width)
					{
						world_tile nextTile = grid->tiles[current_index + 1];

	                    u32 same_tile_as_original = GRID_TILE_EQUALS(nextTile, originalTile);
						if(!same_tile_as_original)
						{
							break;
						}
					}
				}
			}
	        temporary_area_end(&tempArea);
	    }
	}
}
#endif


inline u32
editor_world_get_overlapping_near_tile(s_game_editor *game_editor,
		                              vec3 v0,
		                              vec3 v1,
		                              vec3 v2,
		                              vec3 v3,
									  u32 *overlapping_tile_index)
{

	u32 success = 0;
	u32 t = 0;

    while(!success && t < game_editor->world.loaded_world->tile_count)
	{
		u16 tile_index          = t;
		world_tile *current_tile = game_editor->world.loaded_world->tiles + tile_index;

	    vec3 new_tile_mid = vertices_get_mid_point(v0,
			      	        					   v1,
			      	        					   v2,
			      	        					   v3);

	    vec3 current_tile_mid = vertices_get_mid_point(current_tile->v0,
			      	        						   current_tile->v1,
			      	        						   current_tile->v2,
			      	        						   current_tile->v3);

		u32 verticesOverlap = 
			(i32)new_tile_mid.x == (i32)current_tile_mid.x &&
			(i32)new_tile_mid.y == (i32)current_tile_mid.y &&
			(i32)new_tile_mid.z == (i32)current_tile_mid.z;

		if(verticesOverlap)
		{
			success = 1;
			*overlapping_tile_index = tile_index;
		}
		t++;
	}
	return(success);
}


inline vec3
editor_world_get_delta_at_cursor(s_game_editor *game_editor,
		                         vec3 rayOrigin,
		                         vec3 rayDir,
							     vec3 pointFrom)
{
	
	vec3 paintCursor_position = game_editor->world.cursor.position;
	vec3 paintCursor_uAxis    = game_editor->world.cursor.uAxis;
	vec3 paintCursor_rAxis    = game_editor->world.cursor.rAxis;
	vec3 paintCursor_normal   = vec3_cross(paintCursor_rAxis,
				                           paintCursor_uAxis);
	f32 paintCursorTileSize   = game_editor->world.cursor.tile_displacement;
	//position against cursor grid
	ray_casted_info rayResult = cast_ray_at_plane(rayOrigin,
				                                 rayDir,
												 paintCursor_position,
												 paintCursor_normal);
	vec3 mouseToGrid = rayResult.ray_on_plane;

	vec3 mouse_cursor_delta = vec3_sub(mouseToGrid, pointFrom);
	//vec3 mouse_cursor_delta = game_editor->world.mouse_holding_position;
	//vec3 v0_last = current_tile->v0;

	vec3 move_delta_position = {0};

	move_delta_position.x = vec3_inner(mouse_cursor_delta, paintCursor_rAxis);
	move_delta_position.y = vec3_inner(mouse_cursor_delta, paintCursor_uAxis);
	move_delta_position.x = scalar_advance_by_no_minus_zero(move_delta_position.x, (i32)paintCursorTileSize);
	move_delta_position.y = scalar_advance_by_no_minus_zero(move_delta_position.y, (i32)paintCursorTileSize); 

	matrix3x3 move_delta_rotation = matrix3x3_from_vec_col(paintCursor_rAxis,
		 		                                           paintCursor_uAxis,
		 												   paintCursor_normal);

    //move_delta_position2 = matrix3x3_v3_mul_cols(move_delta_position, move_delta_positionRotation);
    move_delta_position = matrix3x3_v3_mul_rows(move_delta_rotation, move_delta_position);

	return(move_delta_position);
}

inline void
editor_world_remove_tile(s_game_editor *game_editor,
		                 u32 tile_index)
{
	// ;I might need to get rid of this tile at the end of the update to avoid errors
	
	world_tile *terrainArray = game_editor->world.loaded_world->tiles;
	u32 tile_count        = game_editor->world.loaded_world->tile_count;
	u32 t = tile_index;
	//shift the terrain array to the left by one step

	while(t < tile_count - 1)
	{
		world_tile tileA = terrainArray[t];
		terrainArray[t] = terrainArray[t + 1];
		terrainArray[t + 1] = tileA;
		t++;
	}
	game_editor->world.loaded_world->tile_count--;
}


inline void
editor_world_undo_or_redo_moved_selections(s_game_editor *game_editor,
		                                   editor_world_history_record_data *recorded_history,
		                                   u32 redo)
{

	vec3 amount_moved = vec3_round_to_int(recorded_history->moved_selections.delta_moved);
	if(!redo)
	{
		amount_moved.x = -amount_moved.x;
		amount_moved.y = -amount_moved.y;
		amount_moved.z = -amount_moved.z;
	}

	u32 tile_count   = recorded_history->moved_selections.tile_count;
	u32 ignored_tile = recorded_history->moved_selections.ignored_tile;

	world_tile *world_tile_array = game_editor->world.loaded_world->tiles;

	//go to the end of the header where the array is located

	editor_mesh_selection *selection_data_array = (editor_mesh_selection *)(recorded_history + 1);

	u32 t = 0;
	while(t < tile_count)
	{
        editor_mesh_selection tile_selection_data = selection_data_array[t];

		u32 tile_index  = tile_selection_data.index;
		world_tile *tile = world_tile_array + tile_index;

		if(ignored_tile != tile_index)
		{
		    tile->v0 = tile_selection_data.v0_selected ? vec3_add(tile->v0, amount_moved) : tile->v0;
		    tile->v1 = tile_selection_data.v1_selected ? vec3_add(tile->v1, amount_moved) : tile->v1;
		    tile->v2 = tile_selection_data.v2_selected ? vec3_add(tile->v2, amount_moved) : tile->v2;
		    tile->v3 = tile_selection_data.v3_selected ? vec3_add(tile->v3, amount_moved) : tile->v3;
		}

		//world_tile *selected_tile = 
		t++;
	}

	if(recorded_history->moved_selections.move_gizmo)
	{
	    game_editor->world.rotation_point = vec3_add(game_editor->world.rotation_point, amount_moved);
	}
}
static void
editor_world_undo(s_game_editor *game_editor)
{
	stream_data *info_stream = &game_editor->info_stream;

	if(game_editor->world.history_cursor && 
	   game_editor->world.history_count)
	{
		u32 history_index = --game_editor->world.history_cursor;

		editor_world_history_header *recorded_history_header = game_editor->world.history + history_index;
		editor_world_history_record_data *recorded_history = recorded_history_header->record;

		switch(recorded_history->type)
		{

			case world_history_tile_added:
				{
					game_editor->world.loaded_world->tile_count -= recorded_history->added_tile_count;
				}break;
			case world_history_tile_removed:
				{
					u32 removed_tiles_count  = recorded_history->tile_count;
					world_tile *removed_tiles = (world_tile *)(recorded_history + 1);
					u32 current_tiles_count  = game_editor->world.loaded_world->tile_count;
					world_tile *world_tiles   = game_editor->world.loaded_world->tiles;

					game_editor->world.loaded_world->tile_count += removed_tiles_count;

					u32 t = 0;
					while(t < removed_tiles_count)
					{
						u32 tile_index = current_tiles_count + t;
						world_tiles[tile_index] = removed_tiles[t];
						t++;
					}
				}break;
			case world_history_moved_selections:
				{
                    editor_world_undo_or_redo_moved_selections(game_editor, recorded_history, 0);
				}break;
			case world_history_paste:
				{
					u32 paste_tile_count = recorded_history->pasted_tiles.tile_count;

					game_editor->world.loaded_world->tile_count -= paste_tile_count;
				}break;
			case world_history_cut:
				{
					u32 cut_tile_count = recorded_history->cut_tiles.tile_count;
					//game_editor->world.loaded_world->tile_count += cut_tile_count;

					u8 *data_after_header        = (u8 *)(recorded_history + 1);
					u32 *tile_ascending_indices  = (u32 *)data_after_header;
					data_after_header           += cut_tile_count * sizeof(u32);

					world_tile *memory_tile_array = (world_tile *)data_after_header;
					world_tile *world_tile_array  = game_editor->world.loaded_world->tiles;

					u32 t = 0;
					while(t < cut_tile_count)
					{

						u32 tile_index      = tile_ascending_indices[t];
						world_tile tile_data = memory_tile_array[t];

						//insert tile
						u32 end_index = game_editor->world.loaded_world->tile_count++;
						//shift array one step to the right
						while(end_index > tile_index)
						{
                            world_tile_array[end_index] = world_tile_array[end_index - 1];

							end_index--;
						}
						world_tile_array[tile_index] = tile_data;
						t++;
					}
				}break;
			default:
				{
					Assert(0);
				}
		}
	}
}

static void
editor_world_redo(s_game_editor *game_editor)
{
	stream_data *info_stream = &game_editor->info_stream;
	if(game_editor->world.history_count &&
	   game_editor->world.history_cursor < game_editor->world.history_count)
	{
		u32 history_index = game_editor->world.history_cursor++;

		editor_world_history_header *recorded_history_header = game_editor->world.history + history_index;
		editor_world_history_record_data *recorded_history = recorded_history_header->record;

		switch(recorded_history->type)
		{

			case world_history_tile_added:
				{
					world_tile added_tiles = recorded_history->added_tile;

					game_editor->world.loaded_world->tiles[game_editor->world.loaded_world->tile_count] = added_tiles;
					game_editor->world.loaded_world->tile_count += recorded_history->added_tile_count;
				}break;
			case world_history_tile_removed:
				{
				}break;
			case world_history_moved_selections:
				{
                    editor_world_undo_or_redo_moved_selections(game_editor, recorded_history, 1);
				}break;
			case world_history_paste:
				{
					u32 paste_tile_count         = recorded_history->pasted_tiles.tile_count;
					u32 tile_start_index = game_editor->world.loaded_world->tile_count;
					game_editor->world.loaded_world->tile_count += paste_tile_count;

					world_tile *memory_tile_array = (world_tile *)(recorded_history + 1);
					world_tile *world_tile_array  = game_editor->world.loaded_world->tiles;

					u32 t = 0;
					while(t < game_editor->world.loaded_world->tile_count)
					{
						u32 tile_index = tile_start_index + t;
						world_tile_array[tile_index] = memory_tile_array[t];
						t++;
					}

				}break;
			case world_history_cut:
				{
					u32 cut_tile_count = recorded_history->cut_tiles.tile_count;

					u8 *data_after_header        = (u8 *)(recorded_history + 1);
					u32 *tile_ascending_indices  = (u32 *)data_after_header;

					u32 t = 0;
					//read tile indices in descending order
					while(t < cut_tile_count)
					{
						u32 tile_index = tile_ascending_indices[cut_tile_count - 1 - t];
						editor_world_remove_tile(game_editor, tile_index);
						t++;
					}
				}break;
			default:
				{
					Assert(0);
				}
		}

	}
}

static editor_world_history_header *
editor_world_record_push(s_game_editor *game_editor)
{

	u32 history_count  = game_editor->world.history_count;
	u32 history_max    = game_editor->world.history_max;
	u32 history_cursor = game_editor->world.history_cursor;
    editor_world_history_header *header_array = game_editor->world.history;

	editor_world_history_header *pushed_header = 0;
	//to cursor
	if(history_count && (history_cursor < history_count))
	{
		pushed_header = header_array + history_cursor;

		game_editor->world.history_count = history_cursor;

        game_editor->world.history_count++;
        game_editor->world.history_cursor++;
	}
	else if(history_count < history_max)
	{
		pushed_header = header_array + history_count;
        game_editor->world.history_count++;
        game_editor->world.history_cursor++;
	}
	else
	{
		//push buffers back
		u32 h = 0;
		while(h < (u32)(game_editor->world.history_max - 1))
		{
			game_editor->world.history[h] = game_editor->world.history[h + 1];
			h++;
		}
		pushed_header = game_editor->world.history + game_editor->world.history_count - 1;
	}

	return(pushed_header);
}

inline void *
editor_world_push_size_to_history_buffer(s_game_editor *game_editor,
		                                 u32 size)
{
	u32 size_used   = game_editor->world.history_buffer_used;
	u8 *buffer_base = game_editor->world.history_buffer;
	u8 *buffer_at   = buffer_base + size_used;

	//check 
	if((size_used + size) > (game_editor->world.history_buffer_total))
	{
		//get the total size to remove
		u32 total_size_removed   = 0;
		u32 history_header_index = 0;
		while(total_size_removed < size)
		{
			editor_world_history_header header0 = game_editor->world.history[history_header_index];
			editor_world_history_header header1 = game_editor->world.history[history_header_index + 1];

			total_size_removed += (u32)(header1.record - header0.record);
			history_header_index++;
		}


		u32 total_size = size_used - total_size_removed;
		game_editor->world.history_buffer_used = total_size;

		memory_Copy(buffer_base + total_size_removed, buffer_base, total_size);

		//subtract the total removed headers
		game_editor->world.history_count  -= history_header_index;
        game_editor->world.history_cursor -= history_header_index;

		u32 index_at = history_header_index;
		u32 index_to = 0;
		while(index_at < (u32)(game_editor->world.history_count - 1))
		{
			editor_world_history_header *header_at = game_editor->world.history + index_at;
			(u8 *)header_at->record -= total_size_removed;

			game_editor->world.history[index_to] = *header_at; 

			index_at++;
			index_to++;
		}

		buffer_at = buffer_base + total_size;
	}
	else
	{
		game_editor->world.history_buffer_used += size;
	}
	return(buffer_at);
}

inline void
editor_world_record_add_tile(
		s_game_editor *game_editor, 
		world_tile new_tile)
{

	editor_world_history_header *pushed_header = editor_world_record_push(game_editor);

	u32 needed_size = sizeof(editor_world_history_record_data);
	editor_world_history_record_data *pushed_record = editor_world_push_size_to_history_buffer(game_editor, needed_size);

	pushed_header->record     = pushed_record;

	pushed_record->type             = world_history_tile_added;
	pushed_record->added_tile       = new_tile;
	pushed_record->added_tile_count = 1;
}

inline void
editor_world_record_moved_selected_all(s_game_editor *game_editor,
		                               vec3 delta_moved,
									   u32 move_gizmo,
								       u32 ignored_tile)
{
	editor_world_history_header *pushed_header = editor_world_record_push(game_editor);

	u32 needed_size = (game_editor->world.cursor_memory.selected_meshes_count * sizeof(editor_mesh_selection)) + sizeof(editor_world_history_record_data);
	u8 *data        = editor_world_push_size_to_history_buffer(game_editor, needed_size);

	editor_world_history_record_data *pushed_record = (editor_world_history_record_data *)data;
	pushed_record->type = world_history_moved_selections;
	pushed_record->moved_selections.tile_count      = game_editor->world.cursor_memory.selected_meshes_count;
	pushed_record->moved_selections.delta_moved     = delta_moved;
	pushed_record->moved_selections.ignored_tile    = ignored_tile;
	pushed_record->moved_selections.move_gizmo      = move_gizmo;

	pushed_header->record = pushed_record;

	data += sizeof(editor_world_history_record_data);

	editor_mesh_selection *memory_selection_array = (editor_mesh_selection *)data;
	editor_mesh_selection *world_selection_array  = game_editor->world.cursor_memory.selected_meshes;

	u32 selection_count = game_editor->world.cursor_memory.selected_meshes_count;
	u32 t = 0;
	while(t < selection_count)
	{
		memory_selection_array[t] = world_selection_array[t];
		t++;
	}

}

inline void
editor_world_record_moved_selected_gizmo(s_game_editor *game_editor,
		                                 vec3 delta_moved,
										 u32 ignored_tile)
{
	
    editor_world_record_moved_selected_all(game_editor,
    		                               delta_moved,
    									   1,
    								       ignored_tile);
}

inline void
editor_world_record_moved_selected(s_game_editor *game_editor,
		                           vec3 delta_moved)
{
    editor_world_record_moved_selected_all(game_editor,
    		                               delta_moved,
    									   0,
    								       game_editor->world.loaded_world->tile_count);

}

static void
editor_world_record_pasted_tiles(s_game_editor *game_editor)
{
	editor_world_history_header *pushed_header = editor_world_record_push(game_editor);
	u32 pasted_tiles_count = game_editor->world.tile_clipboard_count;

	u32 needed_size = (pasted_tiles_count * sizeof(world_tile)) + sizeof(editor_world_history_record_data);
	u8 *data        = editor_world_push_size_to_history_buffer(game_editor, needed_size);

	editor_world_history_record_data *pushed_record = (editor_world_history_record_data *)data;
	pushed_record->type = world_history_paste;
	pushed_record->pasted_tiles.tile_count = pasted_tiles_count;

	pushed_header->record = pushed_record;

	data += sizeof(editor_world_history_record_data);

	world_tile *memory_tile_array = (world_tile *)data;
	world_tile *world_tile_array  = game_editor->world.loaded_world->tiles;

	u32 tile_count        = game_editor->world.loaded_world->tile_count;
	u32 pasted_tile_index = tile_count - pasted_tiles_count;
	u32 t = 0;
	while(t < pasted_tiles_count)
	{
		memory_tile_array[t] = world_tile_array[pasted_tile_index];
		pasted_tile_index++;
		t++;
	}

}

static void
editor_world_record_cut_tiles(s_game_editor *game_editor)
{
	editor_world_history_header *pushed_header = editor_world_record_push(game_editor);
	u32 selection_count = game_editor->world.cursor_memory.selected_meshes_count;

	u32 needed_size = selection_count * sizeof(u32) +
		              (selection_count * sizeof(world_tile)) +
		              sizeof(editor_world_history_record_data);

	u8 *data = editor_world_push_size_to_history_buffer(game_editor, needed_size);

	editor_world_history_record_data *pushed_record = (editor_world_history_record_data *)data;
	pushed_record->type                 = world_history_cut;
	pushed_record->cut_tiles.tile_count = selection_count;

	pushed_header->record = pushed_record;

	data += sizeof(editor_world_history_record_data);

	world_tile *world_tile_array                   = game_editor->world.loaded_world->tiles;
	editor_mesh_selection *world_selection_array  = game_editor->world.cursor_memory.selected_meshes;

	//store the order of tiles first
    u32 *tile_ascending_indices = (u32 *)data;
	data += sizeof(u32) * selection_count;

	//store tile array after indices
    world_tile *memory_tile_array = (world_tile *)data;



	u32 t = 0;
	while(t < selection_count)
	{
		u32 tile_index        = world_selection_array[t].index;
		tile_ascending_indices[t] = tile_index;

		t++;
	}
	//sort by ascending and store tiles
	u32_array_insertion_sort_ascending(tile_ascending_indices, selection_count);
    t = 0;
	while(t < selection_count)
	{
		u32 tile_index  = tile_ascending_indices[t];

		memory_tile_array[t] = world_tile_array[tile_index];

		t++;
	}
}

inline void
editor_world_copy_selected_tiles(s_game_editor *game_editor)
{
	u32 selected_tile_count = game_editor->world.cursor_memory.selected_meshes_count;
	if(selected_tile_count)
	{
		world_tile *tile_clipboard = game_editor->world.tile_clipboard;
		world_tile *tile_array     = game_editor->world.loaded_world->tiles;
		u32 t = 0;
		while(t < selected_tile_count)
		{
			u32 tile_index         = game_editor->world.cursor_memory.selected_meshes[t].index;
			world_tile *copied_tile = tile_array + tile_index;

			tile_clipboard[t] = *copied_tile;

			t++;
		}
		game_editor->world.tile_clipboard_count = selected_tile_count;

		Assert(game_editor->world.tile_clipboard_count < editor_TILE_CLIPBOARD_MAX);
	}
}


inline void
editor_world_cut_selected_tiles(s_game_editor *game_editor)
{
	editor_world_copy_selected_tiles(game_editor);
	u32 selected_tiles_count = game_editor->world.cursor_memory.selected_meshes_count;
	if(selected_tiles_count)
	{
	    editor_world_record_cut_tiles(game_editor);

	    game_editor->world.cursor_memory.selected_meshes_count = 0;

		//order the indices in descending order to correctly shift the selected indices.
		temporary_area temporary_sort_area = temporary_area_begin(&game_editor->area);
		u32 *index_descending_array = memory_area_push_array(&game_editor->area, u32, selected_tiles_count);

		u32 t = 0;
		while(t < selected_tiles_count)
		{
			index_descending_array[t] = game_editor->world.cursor_memory.selected_meshes[t].index;
			t++;
		}
		u32_array_insertion_sort_descending(index_descending_array, selected_tiles_count);

	    t = 0;
	    while(t < selected_tiles_count)
	    {
			//TODO: tile selection sort and removal 
	    	u32 tile_index = index_descending_array[t];
            editor_world_remove_tile(game_editor, tile_index);
			t++;
	    }

		temporary_area_end(&temporary_sort_area);
	}
}

inline void
editor_world_paste_selected_tiles(s_game_editor *game_editor)
{
	u32 tile_clipboard_count = game_editor->world.tile_clipboard_count;
	if(tile_clipboard_count)
	{
		editor_mesh_selection *selection_array = 
			game_editor->world.cursor_memory.selected_meshes;

		game_editor->world.cursor_memory.selected_meshes_count = tile_clipboard_count;

		world_tile *tile_clipboard    = game_editor->world.tile_clipboard;
		world_tile *tile_array        = game_editor->world.loaded_world->tiles;

		u32 new_tiles_index_start = game_editor->world.loaded_world->tile_count;
        game_editor->world.tile_count += tile_clipboard_count;

		u32 t = 0;
		while(t < tile_clipboard_count)
		{
			u32 new_tile_index = new_tiles_index_start + t;
			tile_array[new_tile_index] = tile_clipboard[t];
		    //add tile to selection array
			editor_mesh_selection *selection_data = selection_array + t;
			selection_data->index       = new_tile_index; 
			selection_data->v0_selected = 1;
			selection_data->v1_selected = 1;
			selection_data->v2_selected = 1;
			selection_data->v3_selected = 1;

			t++;
		}
		editor_world_record_pasted_tiles(game_editor);
	}
}


static void
editor_world_update_render(s_editor_state *editor_state,
		                   game_renderer *gameRenderer,
						   game_input *game_input,
						   f32 dt)
{
	render_commands *commands = render_commands_Begin(gameRenderer); 
	//render_commands_SetProjection(commands);
	commands->render_flags = 0 | render_flags_DepthTest | render_flags_DepthPeel;
	commands->camera_type = render_camera_perspective;


	s_game_editor *game_editor = &editor_state->editor;
	game_assets *game_asset_manager = editor_state->editor_assets;
	stream_data *info_stream = &game_editor->info_stream;

	editor_world_tool worldTool = game_editor->world.tool;
	editor_cursor_mode editor_world_mode = game_editor->world.cursor_mode;
	//initial data
	u32 tileset_count                 = game_editor->world.world_tilesets_count;
	u32 tile_count                    = game_editor->world.tile_count;
	u32 entity_count                  = game_editor->world.entity_count;
	world_entity *editor_entity_array = game_editor->world.world_entities;
	world_tile *tile_array            = game_editor->world.world_tiles;

	//tile ray data
	u16 tile_hitted            = 0;
	u16 tile_hitted_index       = 0;
	i32 tile_selected_hitted_index  = -1;

	f32 tile_ray_hit_distance           = 100000.0f;
	f32 tile_vertex_hit_distance        = 100000.0f;
	editor_mesh_edge tile_hovering_edge = 0;
	u32 tile_hovering_vertex            = 0;

	f32 vertices_cube_size     = game_editor->settings.vertices_cube_size;
	f32 gizmo_cube_size        = 2.0f;
	vec3 rotation_point_last   = game_editor->world.rotation_point;
	vec3 rotation_point_new    = game_editor->world.rotation_point;
	u16 record_selection_move  = 0;
	u16 started_selection_move = 0;


	//Input
	//I was about to add the bucket tool functionality
	u16 mouse_l_down     = input_down(game_input->mouse_left);
	u16 mouse_l_pressed  = input_pressed(game_input->mouse_left);
	u16 mouse_r_pressed = input_pressed(game_input->mouse_right);
	u16 mouse_r_down    = input_down(game_input->mouse_right);
	u32 ui_is_interacting = game_editor->ui_is_interacting;
	u32 ui_focused = game_editor->ui_is_focused; 
	u16 in_camera_mode = game_input->spaceBarDown;
	u16 mouse_inside_window = editor_state->platform->mouseAtWindow;

	u16 pressed_u         = input_pressed(game_input->input.w);
	u16 pressed_d         = input_pressed(game_input->input.s);
	u16 pressed_l         = input_pressed(game_input->input.a);
	u16 pressed_r         = input_pressed(game_input->input.d);

	u32 editor_process_input = game_editor->process_input;
	u32 input_text_focused   = game_input->input_text.focused || game_input->input_text.got_focus;
	vec3 paintCursor_p     = game_editor->world.cursor.position;

	//Set ray trace
	vec2 mousePoint = {game_input->mouse_clip_x, game_input->mouse_clip_y};
	vec3 mouseWorld = render_mouse_coordinates_to_world(
			gameRenderer, mousePoint, 1.0f);
	vec3 rayOrigin  = gameRenderer->camera_position;
	vec3 rayDir     = vec3_normalize_safe(vec3_sub(mouseWorld, rayOrigin));

	f32 tileRayHitDistance = 10000.0f;

	vec3 gridPlaneN = {0, 0, 1}; 
	f32 paintCursorTileSize = game_editor->world.cursor.tile_displacement; 

	//initialize frame

	memory_area_reset(&game_editor->world.per_frame_area);
	editor_cursor_memory_initialize_frame(
			&game_editor->world.per_frame_area,
			&game_editor->world.cursor_memory);

	editor_cursor_memory *cursor_memory = &game_editor->world.cursor_memory;
	editor_cursor *cursor               = &game_editor->world.cursor;

	//update the "loaded" world data
	game_world *loaded_editor_world = game_editor->world.loaded_world;
	loaded_editor_world->tile_count    = game_editor->world.tile_count;
	loaded_editor_world->entity_count  = game_editor->world.entity_count;
	loaded_editor_world->tileset_count = game_editor->world.world_tilesets_count;
	//arrays
	loaded_editor_world->tiles         = game_editor->world.world_tiles;
	loaded_editor_world->entities      = game_editor->world.world_entities;
	loaded_editor_world->tilesets_a    = game_editor->world.tilesets_as_asset;

	//run all loaded tileset animations
	for(u32 t = 0;
			t < tileset_count;
			t++)
	{

		asset_tileset asset_tileset = game_editor->world.tilesets_as_asset[t];

		world_tileset *tileset = assets_get_tileset_by_key(game_asset_manager,
				asset_tileset); 
		tileset_reproduce_animations(tileset, dt);
	}

	//
	// hotkeys
	//
	//undo/redo hotkeys


	if(game_editor->world.cursor.tile_displacement < 1)
	{
		game_editor->world.cursor.tile_displacement = 1;
	}
	//
	//
	//


	vec2 distanceCameraRayOnGrid = {0};
	vec2 rayBounds = {600, 600};

	//
	// up and right axis
	// _crosshair axes based on camera 

	editor_cursor_set_axes_by_camera(gameRenderer, &game_editor->world.cursor);

	//move cursor with keys
	//_crosshair movement with keys
	u32 paint_cursor_moved = 0;
	vec3 cursor_p_last     = game_editor->world.cursor.position;
	if(!input_text_focused)
	{
		paint_cursor_moved = editor_cursor_input_movement_by_camera(gameRenderer,
				game_input,
				&game_editor->world.cursor,
				dt);

	}
	else
	{
		int s = 0;
	}



	if(game_editor->world.frame_tile_size_x < MINIMUM_TILESIZE)
	{
		game_editor->world.frame_tile_size_x = MINIMUM_TILESIZE;
	}
	if(game_editor->world.frame_tile_size_y < MINIMUM_TILESIZE)
	{
		game_editor->world.frame_tile_size_y = MINIMUM_TILESIZE;
	}
	//
	// _crosshair movement with mouse
	//

	//Grid movement by mouse or keys
	u32 move_reference_grid = game_editor->world.move_reference_grid;
	if(move_reference_grid)
	{
		if(editor_process_input && mouse_l_pressed)
		{
			game_editor->world.move_reference_grid = 0;
		}

		vec3 worldPlaneN = {0, 0, 1};
		//Move grid towards z axis 
		if(game_input->shift_l)
		{
			worldPlaneN.z = 0;
			worldPlaneN.y = 1.0f;
		}
		//The grid always moves up in order to move it on the x and y axis
		f32 worldRayDir = vec3_inner(rayDir, worldPlaneN);
		vec3 worldPlaneP = paintCursor_p;
		if(worldRayDir > 0.001f || worldRayDir < -0.001f)
		{
			vec3 distanceGridRay    = vec3_sub(paintCursor_p, rayOrigin);
			real32 worldRayDistance = vec3_inner(distanceGridRay, worldPlaneN);
			worldRayDistance		/= worldRayDir;
			worldPlaneP				= vec3_add(rayOrigin, vec3_f32_mul(rayDir, worldRayDistance));


			//round them up
			worldPlaneP.x = (f32)((i32)((worldPlaneP.x + 0.0001f) / paintCursorTileSize) * paintCursorTileSize);
			worldPlaneP.y = (f32)((i32)((worldPlaneP.y + 0.0001f) / paintCursorTileSize) * paintCursorTileSize);
			worldPlaneP.z = (f32)((i32)((worldPlaneP.z + 0.0001f) / paintCursorTileSize) * paintCursorTileSize);
		}



		game_editor->world.cursor.position = worldPlaneP; 
		paintCursor_p = worldPlaneP; 
	}


	u32 hoveredRotationMidPoint = 0;
	//shoot ray against rotation point

	if(editor_world_mode == editor_cursor_tiles)
	{


		//to select edges, vertices...
		if(game_editor->world.tool == world_tool_selection)
		{
			//cancel movement/resize
			if(in_camera_mode || (!mouse_l_down && !game_input->shift_l))
			{
				//record last movement
				record_selection_move = game_editor->world.moving_selections;

				game_editor->world.moving_selections    = 0;
				game_editor->world.movingSelectedVertex = 0;
			}
			if(game_input->shift_l)
			{
				started_selection_move = !game_editor->world.moving_selections;

				game_editor->world.moving_selections = 1;
			}

			//update gizmo point
			if(tile_count)
			{
				vec3 mid_point = game_editor->world.rotation_point;
				ray_cube_result ray_and_rotation_point_result = ray_cube_get_result(rayOrigin,
						rayDir,
						mid_point,
						vec3_all(gizmo_cube_size));


				//hitted cube
				if(ray_and_rotation_point_result.t_min)
				{

					tile_ray_hit_distance = ray_and_rotation_point_result.t_min;
					hoveredRotationMidPoint = 1;
					if(mouse_l_pressed)
					{
						started_selection_move = !game_editor->world.moving_selections;

						game_editor->world.moving_selections = 1;
					}


				}
				//editor_gizmo_interact_ray(rayOrigin, rayDir, tile_ray_hit_distance);
			}
			if(started_selection_move)
			{
				game_editor->world.recorded_rotation_point = game_editor->world.rotation_point;
			}
			//for edge or tile de-selection
			for(u32 t = 0; t < game_editor->world.cursor_memory.selected_meshes_count; t++)
			{
				editor_mesh_selection *selection_data = game_editor->world.cursor_memory.selected_meshes + t;
				u32 tile_index                        = selection_data->index;
				world_tile *current_tile = tile_array + tile_index;

				editor_mouse_mesh_selection mesh_ray_selection_data =  editor_cursor_update_selected_mesh(
						rayOrigin,
						rayDir, 
						current_tile->v0,
						current_tile->v1,
						current_tile->v2,
						current_tile->v3,
						t,
						tile_ray_hit_distance,
						&game_editor->world.cursor_memory,
						game_editor->settings.vertices_cube_size);

				if(mesh_ray_selection_data.hitted)
				{
					tile_ray_hit_distance = mesh_ray_selection_data.hit_distance;
					tile_hovering_edge    = mesh_ray_selection_data.hovering_edge;

					tile_hitted = 1;
					tile_hitted_index = tile_index;
					tile_selected_hitted_index = t;
				}


			} //selected tiles array
		}
		//to select tiles
		for(u32 t = 0; t < tile_count; t++)
		{
			world_tile *current_tile  = tile_array + t;
			ray_hit_result hit_result = ray_quad_get_hit_result(rayOrigin,
					rayDir,
					current_tile->v0,
					current_tile->v1,
					current_tile->v2,
					current_tile->v3,
					tile_ray_hit_distance);

			if(hit_result.hitted)
			{
				tile_ray_hit_distance = hit_result.distance;
				tile_hitted = 1;
				tile_hitted_index = t;
			}

		}

		//__FOR DEBUG

		//vec3 ray_tile_end = vec3_add(rayOrigin, vec3_scale(rayDir, tile_ray_hit_distance));
		//vec3 ray_draw_origin = vec3_add(rayOrigin, vec3_add_x(rayDir, 3));
		//render_draw_line_up(commands, ray_draw_origin, ray_tile_end, V4(255, 255, 255, 255), 1);
		//render_draw_cube_colored(commands, ray_tile_end, V3(4, 4, 4), V4(255, 0, 0, 255));
		//process edges after shooting ray
		// ;cleanup?

		//FOR DEBUG__

		game_editor->world.edge_hot = tile_hovering_edge;

		if(worldTool == world_tool_paint && tileset_count)
		{

			//flip up and right axes based on orientation
			u32 tile_orientation = game_editor->world.cursor.mesh_orientation;
			//Draw tile location

			u32 processLeftClick  = editor_process_input && mouse_l_down;
			u32 processRightClick = editor_process_input && mouse_r_down;

			//get pointing tileset and texture
			u32 tileset_index     = game_editor->world.selected_tileset_for_painting_index;
			asset_tileset cursor_tileset_key = game_editor->world.tilesets_as_asset[tileset_index];
			world_tileset *tileset  = assets_get_tileset_by_key(game_asset_manager, cursor_tileset_key);
			render_texture *texture = assets_get_texture_by_id(game_asset_manager, tileset->imageId).image;


			vec3 v0 = {0};
			vec3 v1 = {0};
			vec3 v2 = {0};
			vec3 v3 = {0};
			vec2 tile_uv0 = game_editor->world.cursor.mesh.uv0;
			vec2 tile_uv1 = game_editor->world.cursor.mesh.uv1;
			vec2 tile_uv2 = game_editor->world.cursor.mesh.uv2;
			vec2 tile_uv3 = game_editor->world.cursor.mesh.uv3;

			vec2 tile_uv_min = game_editor->world.cursor.mesh.uv1; 
			vec2 tile_uv_max = game_editor->world.cursor.mesh.uv3; 

			u16 frame_w = (u16)((tile_uv_max.x - tile_uv_min.x) * texture->width);
			u16 frame_h = (u16)((tile_uv_max.y - tile_uv_min.y) * texture->height);

			u32 canAddTile = 0;

			if(game_editor->world.stick_to_near_tile) 
			{ //stick to the edge of hovering tile
			  //Note: in order to keep the tile orientation I must re-scale the tile from the mid point
			  //using it's axes
				if(tile_hitted && tile_hovering_edge)
				{
					canAddTile = frame_w > 0 && frame_h > 0;;

					world_tile *tileHot = game_editor->world.loaded_world->tiles + tile_hitted_index;
					if(tile_hovering_edge == tile_edge_u)
					{
						vec3 distance_v1_v0 = vec3_sub(tileHot->v1, tileHot->v0);
						vec3 distance_v2_v3 = vec3_sub(tileHot->v2, tileHot->v3);



						v0 = tileHot->v1;
						v3 = tileHot->v2;

						v1 = vec3_add(v0, distance_v1_v0);
						v2 = vec3_add(v3, distance_v2_v3);
					}
					else if(tile_hovering_edge == tile_edge_d)
					{
						v1 = tileHot->v0;
						v2 = tileHot->v3;

						vec3 distance_v1_v0 = vec3_sub(tileHot->v1, tileHot->v0);
						vec3 distance_v2_v3 = vec3_sub(tileHot->v2, tileHot->v3);

						v0 = vec3_sub(v1, distance_v1_v0);
						v3 = vec3_sub(v2, distance_v2_v3);
					}
					else if(tile_hovering_edge == tile_edge_l)
					{
						v2 = tileHot->v1;
						v3 = tileHot->v0;

						vec3 distance_v2_v1 = vec3_sub(tileHot->v2, tileHot->v1);
						vec3 distance_v3_v0 = vec3_sub(tileHot->v3, tileHot->v0);

						v1 = vec3_sub(v2, distance_v2_v1);
						v0 = vec3_sub(v3, distance_v3_v0);
					}
					else
					{
						v0 = tileHot->v3;
						v1 = tileHot->v2;

						vec3 distance_v2_v1 = vec3_sub(tileHot->v2, tileHot->v1);
						vec3 distance_v3_v0 = vec3_sub(tileHot->v3, tileHot->v0);

						v3 = vec3_add(v0, distance_v3_v0);
						v2 = vec3_add(v1, distance_v2_v1);
					}

				}
			}
			else if(game_editor->world.repaint_tile)
			{
				if(tile_hitted)
				{

					canAddTile = frame_w > 0 && frame_h > 0 && mouse_l_pressed;
					world_tile *current_tile = game_editor->world.loaded_world->tiles + tile_hitted_index;


					mesh_points oriented_points = quad_rotate_vertices_by_orientation(cursor->mesh_orientation,
							current_tile->v0,
							current_tile->v1,
							current_tile->v2,
							current_tile->v3);
					v0 = oriented_points.v0;
					v1 = oriented_points.v1;
					v2 = oriented_points.v2;
					v3 = oriented_points.v3;


				}

			}
			else
			{
				canAddTile = frame_w > 0 && frame_h > 0;

				editor_cursor_set_mesh_axes_by_orientation(&game_editor->world.cursor);
				vec3 next_tile_uAxis = game_editor->world.cursor.mesh_uAxis;
				vec3 next_tile_rAxis = game_editor->world.cursor.mesh_rAxis;

				//u16 frame_x = game_editor->world.tileset_frame_x;
				//u16 frame_y = game_editor->world.tileset_frame_y;



				vec3 next_tile_position = editor_cursor_translate_ray_to_tile_wh(&game_editor->world.cursor,
						rayOrigin,
						rayDir,
						frame_w,
						frame_h);

				vec3 next_tile_uAxis_scaled = vec3_scale(next_tile_uAxis, (f32)frame_h);
				vec3 next_tile_rAxis_scaled = vec3_scale(next_tile_rAxis, (f32)frame_w);
				//Future tile vertices
				v0 = next_tile_position;
				v1 = vec3_add(v0, next_tile_uAxis_scaled);
				v2 = vec3_add(v1, next_tile_rAxis_scaled);
				v3 = vec3_add(v0, next_tile_rAxis_scaled);
			}


			//uv settings
			//uv orientations
			u32 uv_flip_x = game_editor->world.next_tile_uv_flip_x;
			if(uv_flip_x)
			{
				f32 uv_min_x_copy = tile_uv_min.x;
				tile_uv_min.x = tile_uv_max.x;
				tile_uv_max.x = uv_min_x_copy;
			}

			//preview tile
			render_push_quad(commands,
					texture,
					v0,
					v1,
					v2,
					v3,
					tile_uv0,
					tile_uv1,
					tile_uv2,
					tile_uv3,
					vec4_all(255));
			//draw vertices for debug
			//render_draw_cube_colored(commands, v0, vec3_all(vertices_cube_size), V4(255, 255, 255, 255));
			//render_draw_cube_colored(commands, v1, vec3_all(vertices_cube_size), V4(255, 0, 0, 255));
			//render_draw_cube_colored(commands, v2, vec3_all(vertices_cube_size), V4(0, 255, 0, 255));
			//render_draw_cube_colored(commands, v3, vec3_all(vertices_cube_size), V4(0, 0, 255, 255));

			if(editor_process_input &&
					mouse_l_down &&
					canAddTile)
			{
				u32 tileIndex = 0;
				u32 added_tile = 0;
				u32 reeplaced_tile = 0;
				world_tile newT = {0};

				if(editor_world_get_overlapping_near_tile(game_editor,
							v0,
							v1,
							v2,
							v3,
							&tileIndex))

				{
				}
				else
				{
					added_tile = 1;
					tileIndex = game_editor->world.tile_count++;
				}

				newT.tileset_index   = game_editor->world.selected_tileset_for_painting_index;
				//newT.tileArrayIndex = game_editor->world.selectedTile_x0;
				//newT.tileArrayIndex = game_editor->world.selectedTileFromTileset;

				newT.v0 = v0;
				newT.v1 = v1;
				newT.v2 = v2;
				newT.v3 = v3;

				reeplaced_tile = (tile_uv0.x == newT.uv0.x && tile_uv0.y == newT.uv0.y) && 
					(tile_uv1.x == newT.uv1.x && tile_uv1.y == newT.uv1.y) &&
					(tile_uv2.x == newT.uv2.x && tile_uv2.y == newT.uv2.y) &&
					(tile_uv3.x == newT.uv3.x && tile_uv3.y == newT.uv3.y)
					;

				newT.uv0 = tile_uv0;
				newT.uv1 = tile_uv1;
				newT.uv2 = tile_uv2;
				newT.uv3 = tile_uv3;

				newT.animated = game_editor->world.next_tile_animated;
				newT.tileset_animation_index = game_editor->world.next_tile_animation_index;


				//grid->tiles[hittedGridIndex] = newT;
				game_editor->world.world_tiles[tileIndex] = newT;
				game_editor->world.drawLocked = 0;

				if(added_tile)
				{
					editor_world_record_add_tile(game_editor, newT);
				}

				//u32 needed_size = sizeof(editor_world_history_record_data) + sizeof(world_tile);
				//u8 *record_memory = game_editor->world.history_buffer + game_editor->world.history_buffer_used;
				//game_editor->world.history_buffer_used += needed_size;

				//editor_world_history_record_data *new_record = (editor_world_history_record_data *)record_memory;
				//new_record->type         = world_history_tile_added;
				//new_record->tile_count   = 1;
				//world_tile *recorded_tile = record_memory + sizeof(editor_world_history_record_data);
				//*recorded_tile = newT;

			}
			if(tile_hitted && editor_process_input && mouse_r_pressed)
			{
				// ;I might need to get rid of this tile at the end of the update to avoid errors
				editor_world_delete_tile(
						game_editor,
						tile_hitted_index);
				tile_hitted = 0;
			} 
		}
		else if(game_editor->world.tool == world_tool_selection)
		{

			u32 in_deselect_mode = game_input->alt;
			u32 selected_tiles_count = game_editor->world.cursor_memory.selected_meshes_count;
			if(tile_hitted && !in_deselect_mode)
			{
				world_tile *current_tile = tile_array + tile_hitted_index;

				if(mouse_l_pressed &&
						editor_process_input)
				{
					//not selected
					if(tile_selected_hitted_index < 0)
					{
						//Add to selected tiles array
						editor_mesh_selection *tileSelection = game_editor->world.cursor_memory.selected_meshes + game_editor->world.cursor_memory.selected_meshes_count;
						tileSelection->index = tile_hitted_index;


						tileSelection->v0_selected = 1;
						tileSelection->v1_selected = 1;
						tileSelection->v2_selected = 1;
						tileSelection->v3_selected = 1;
						game_editor->world.cursor_memory.selected_meshes_count++;

						//if no tile is selected, set rotation point
						if(!selected_tiles_count)
						{
							game_editor->world.rotation_point = vertices_get_mid_point(
									current_tile->v0,
									current_tile->v1,
									current_tile->v2,
									current_tile->v3
									);
						}


					}
					else
					{
						u32 selectionDataIndex = tile_selected_hitted_index;
						editor_mesh_selection *tileSelection = game_editor->world.cursor_memory.selected_meshes + selectionDataIndex;
						//position the gizmo point
						if(game_input->ctrl_l || game_input->shift_l)
						{
							vec3 final_position = game_editor->world.rotation_point;

							if(tile_hovering_edge == tile_edge_v0)
							{
								final_position = current_tile->v0;
							}
							else if(tile_hovering_edge == tile_edge_v1)
							{
								final_position = current_tile->v1;
							}
							else if(tile_hovering_edge == tile_edge_v2)
							{
								final_position = current_tile->v2;
							}
							else if(tile_hovering_edge == tile_edge_v3)
							{
								final_position = current_tile->v3;
							}
							else if(tile_hovering_edge == tile_edge_u)
							{
								final_position = vec3_add(current_tile->v1, vec3_scale(vec3_sub(current_tile->v2, current_tile->v1), 0.5f));
							}
							else if(tile_hovering_edge == tile_edge_d)
							{
								final_position = vec3_add(current_tile->v0, vec3_scale(vec3_sub(current_tile->v3, current_tile->v0), 0.5f));
							}
							else if(tile_hovering_edge == tile_edge_l)
							{
								final_position = vec3_add(current_tile->v0, vec3_scale(vec3_sub(current_tile->v1, current_tile->v0), 0.5f));
							}
							else if(tile_hovering_edge == tile_edge_r)
							{
								final_position = vec3_add(current_tile->v3, vec3_scale(vec3_sub(current_tile->v2, current_tile->v3), 0.5f));
							}
							else //clicked on the same tile, so deselect
							{
								final_position = vertices_get_mid_point(
										current_tile->v0,
										current_tile->v1,
										current_tile->v2,
										current_tile->v3
										);
							}
							game_editor->world.rotation_point = final_position;
						}
						else //update selection data
						{
							if(tile_hovering_edge == tile_edge_v0)
							{
								tileSelection->v0_selected = !tileSelection->v0_selected;
							}
							else if(tile_hovering_edge == tile_edge_v1)
							{
								tileSelection->v1_selected = !tileSelection->v1_selected;
							}
							else if(tile_hovering_edge == tile_edge_v2)
							{
								tileSelection->v2_selected = !tileSelection->v2_selected;
							}
							else if(tile_hovering_edge == tile_edge_v3)
							{
								tileSelection->v3_selected = !tileSelection->v3_selected;
							}
							else //clicked on the same tile, so deselect
							{
								editor_cursor_memory_deselect(cursor_memory, selectionDataIndex);
							}
						}

					}
				}
			}


			//move rotation_point
			if(game_editor->world.moving_selections && !game_input->shift_l)
			{
				vec3 move_delta_position = editor_cursor_move_at_axes(&game_editor->world.cursor,
						rayOrigin,
						rayDir,
						rotation_point_last);
				game_editor->world.rotation_point = vec3_add(move_delta_position, game_editor->world.rotation_point);;
			}

			rotation_point_new = game_editor->world.rotation_point;

			if(record_selection_move)
			{
				vec3 gizmo_move_delta = vec3_sub(rotation_point_new, game_editor->world.recorded_rotation_point);
				u32 ignored_tile      = game_editor->world.loaded_world->tile_count;
				if(game_input->shift_l && tile_hitted)
				{
					ignored_tile = tile_hitted_index;
				}

				editor_world_record_moved_selected_gizmo(game_editor,
						gizmo_move_delta,
						ignored_tile);
				//world_record record = editor_world_push_record();
				//record.moved_selections.moved_delta = gizmo_move_delta;
				//record.moved_selections.ignore_tile = 1;
				//record.moved_selections.ignored_tile = ignored_tile;
				//editor_world_set_record()
			}

			//
			// world selection tool hotkeys
			//
			u8 ctrl_hotkeys = (u8)game_input->ctrl_l;
			u8 shift_hotkeys = (u8)game_input->shift_l;
			u8 alt_hotkeys = (u8)game_input->alt;

			if(ctrl_hotkeys)
			{
				u8 deselect_vertices = game_input->spaceBarDown;
				u8 undo = input_pressed(game_input->input.z);
				u8 redo = input_pressed(game_input->input.y);
				if(deselect_vertices)
				{
					game_editor->world.cursor_memory.selected_meshes_count = 0;
				}
				if(redo)
				{
					editor_world_redo(game_editor);
				}
				if(undo)
				{
					editor_world_undo(game_editor);
				}
			}
			//shift left + x to flip the selected tile's diagonal
			else if(shift_hotkeys)
			{
				u8 flip_selected_tiles_diagonals = input_pressed(game_input->input.x);
				if(flip_selected_tiles_diagonals)
				{

					u32 t = 0;
					while(t < game_editor->world.cursor_memory.selected_meshes_count)
					{
						world_tile *tile = game_editor->world.loaded_world->tiles + game_editor->world.cursor_memory.selected_meshes[t].index;
						model_mesh flipped_quad = quad_flip_diagonal(tile->v0,
								tile->v1,
								tile->v2,
								tile->v3,
								tile->uv0,
								tile->uv1,
								tile->uv2,
								tile->uv3);

						tile->v0 = flipped_quad.v0;
						tile->v1 = flipped_quad.v1;
						tile->v2 = flipped_quad.v2;
						tile->v3 = flipped_quad.v3;

						tile->uv0 = flipped_quad.uv0;
						tile->uv1 = flipped_quad.uv1;
						tile->uv2 = flipped_quad.uv2;
						tile->uv3 = flipped_quad.uv3;
						t++;

					}
				}

			}
			else if(alt_hotkeys)
			{
				u8 select_hovered_vertices = mouse_l_down > 0;
				u8 deselect_hovered_vertices = mouse_r_down > 0;
				if(deselect_hovered_vertices)
				{
					editor_cursor_deselect_hot_vertices(cursor_memory);
				}
				else if(select_hovered_vertices)
				{
					editor_cursor_select_hot_vertices(cursor_memory);
				}
			}
			//move tiles along paint cursor
			if(paint_cursor_moved) //|| game_input->shift_l)
			{
				vec3 cursor_delta = vec3_sub(game_editor->world.cursor.position, cursor_p_last);
				cursor_delta      = vec3_round_to_int(cursor_delta);
				u32 t = 0;
				while(t < game_editor->world.cursor_memory.selected_meshes_count)
				{
					editor_mesh_selection *tile_selection = game_editor->world.cursor_memory.selected_meshes + t;

					world_tile *tile = game_editor->world.loaded_world->tiles + tile_selection->index;


					if(tile_selection->v0_selected)
					{
						tile->v0 = vec3_add(tile->v0 , cursor_delta);
					}
					if(tile_selection->v1_selected)
					{
						tile->v1 = vec3_add(tile->v1 , cursor_delta);
					}
					if(tile_selection->v2_selected)
					{
						tile->v2 = vec3_add(tile->v2 , cursor_delta);
					}
					if(tile_selection->v3_selected)
					{
						tile->v3 = vec3_add(tile->v3 , cursor_delta);
					}
					t++;
				}

				if(game_editor->world.cursor_memory.selected_meshes_count)
				{
					editor_world_record_moved_selected(game_editor,
							cursor_delta);
				}
			}

		} //tool == selection



		//update selections
		u32 t = 0;
		while(t < game_editor->world.cursor_memory.selected_meshes_count)
		{
			editor_mesh_selection *tileSelection = game_editor->world.cursor_memory.selected_meshes + t;
			u32 tile_index                       = tileSelection->index;
			world_tile *current_tile             = tile_array + tile_index;

			if(game_editor->world.apply_rotation)
			{

				mesh_points rotated_vertices = quad_rotate_from_degrees(game_editor->world.rotation_point,
						game_editor->world.rotation_apply_x,
						game_editor->world.rotation_apply_y,
						game_editor->world.rotation_apply_z,
						current_tile->v0,
						current_tile->v1,
						current_tile->v2,
						current_tile->v3);
				current_tile->v0 = rotated_vertices.v0;
				current_tile->v1 = rotated_vertices.v1;
				current_tile->v2 = rotated_vertices.v2;
				current_tile->v3 = rotated_vertices.v3;
			}
			//apply movement if holding click

			//move rotation point and selected vertices

			u32 move_with_selections = !game_input->shift_l || (game_input->shift_l && tile_selected_hitted_index != t);
			if(game_editor->world.moving_selections && move_with_selections)
			{
				if(tileSelection->v0_selected)
				{
					vec3 distance_rotation_point_and_v0 = vec3_sub(rotation_point_last, current_tile->v0);
					current_tile->v0                    = vec3_sub(rotation_point_new, distance_rotation_point_and_v0);
				}
				if(tileSelection->v1_selected)
				{
					vec3 distance_rotation_point_and_v1 = vec3_sub(rotation_point_last, current_tile->v1);
					current_tile->v1                    = vec3_sub(rotation_point_new, distance_rotation_point_and_v1);
				}
				if(tileSelection->v2_selected)
				{
					vec3 distance_rotation_point_and_v2 = vec3_sub(rotation_point_last, current_tile->v2);
					current_tile->v2                    = vec3_sub(rotation_point_new, distance_rotation_point_and_v2);
				}
				if(tileSelection->v3_selected)
				{
					vec3 distance_rotation_point_and_v3 = vec3_sub(rotation_point_last, current_tile->v3);
					current_tile->v3                    = vec3_sub(rotation_point_new, distance_rotation_point_and_v3);
				}
			}
			t++;
		}
		game_editor->world.apply_rotation = 0;

		//
		//world tile cursor hotkeys
		//
		if(mouse_inside_window)
		{
			//general tiles hotkeys
			u32 alt_hotkeys  = game_input->alt;
			u32 ctrl_hotkeys = game_input->ctrl_l;

			if(ctrl_hotkeys)
			{
				u8 copy = input_pressed(game_input->input.c);
				u8 cut = input_pressed(game_input->input.x);
				u8 paste = input_pressed(game_input->input.v);
				u8 reposition_cursor_to_tile = (input_pressed(game_input->input.e) && tile_hitted);

				if(copy)
				{
					editor_world_copy_selected_tiles(game_editor);
				}
				if(cut)
				{
					editor_world_cut_selected_tiles(game_editor);
				}
				if(paste)
				{
					game_editor->world.tool = world_tool_selection;
					editor_world_paste_selected_tiles(game_editor);
				}
				//reposition cursor to hitted tile
				if(reposition_cursor_to_tile)
				{
					world_tile *current_hitted_tile = tile_array + tile_hitted_index;
					cursor->position = current_hitted_tile->v0;
				}

				game_editor->world.reserved_vec.x = (f32)game_editor->world.history_count;
				game_editor->world.reserved_vec.y = (f32)game_editor->world.history_cursor;
				game_editor->world.reserved_vec.z = (f32)game_editor->world.loaded_world->tile_count;
			}
			else if(alt_hotkeys)
			{
			}
			else
			{
				if(input_pressed(game_input->input.e))
				{
					game_editor->world.cursor.mesh_orientation++;
					game_editor->world.cursor.mesh_orientation %= 4;
				}
				if(input_pressed(game_input->input.q))
				{
					game_editor->world.cursor.mesh_orientation--;
					game_editor->world.cursor.mesh_orientation %= 4;
				}

				if(input_pressed(game_input->input.h))
				{
					game_editor->world.next_tile_uv_flip_x = !game_editor->world.next_tile_uv_flip_x;
				}
			}
		}

	}//cursor == tiles__
	if(editor_world_mode == editor_cursor_collisions)
	{
#if 0
		// ;ray_at_tile used to be the result of the casting ray against the closest tile
		// I might instead manipulate the new collision position at the paintCursor location and it's axes

		vec3 ghostP = AddOffsetFromVector(ray_at_tile, gameRenderer->camera_position, 1.0f);
		// ;these axes should be set up from the editor
		vec3 collider_xAxis = {1, 0, 0};
		vec3 collider_yAxis = {0, 1, 0};
		render_draw_rectangleFilled(commands, ghostP, 
				vec3_f32_mul(collider_xAxis, (f32)game_editor->nextColliderW), 
				vec3_f32_mul(collider_yAxis, (f32)game_editor->nextColliderH), V4(0, 0, 0, 120));
		if(!ui_focused && input_pressed(game_input->mouse_left))	
		{
			world_collider newCol = {0};

			i32 newColX = (i32)ray_at_tile.x; 
			i32 newColY = (i32)ray_at_tile.y; 
			i32 newColZ = (i32)ray_at_tile.z; 
			newCol.p = V3((f32)newColX, (f32)newColY, (f32)newColZ);

			newCol.xAxis = collider_xAxis;
			newCol.yAxis = collider_yAxis;

			newCol.width  = MAX((f32)game_editor->world.cursor.tile_displacement, game_editor->nextColliderW);
			newCol.height = MAX((f32)game_editor->world.cursor.tile_displacement, game_editor->nextColliderH);

			Assert(gameWorld->colliders_count < 400);
			gameWorld->colliders[gameWorld->colliders_count++] = newCol;
		}
		//Add collider 
#endif
	}
	else if(game_editor->world.cursor_mode == editor_cursor_entities)
	{

		//; update
		//shoot ray to the cursor always looking up
		ray_casted_info rayResult = cast_ray_at_plane(rayOrigin,
				rayDir,
				paintCursor_p,
				V3(0, 0, 1));
		vec3 entity_p = rayResult.ray_on_plane;
		//add entity
		if(editor_process_input && input_pressed(game_input->mouse_left))
		{
			world_entity *newEntity = editor_entity_array + entity_count;
			game_editor->world.entity_count++;
			memory_clear(newEntity, sizeof(world_entity));

			newEntity->position       = entity_p;

		}
		//Display prop hologram 
		//place on click

	}

	editor_world_update_render_tiles(game_editor, editor_state->editor_assets, commands);
	editor_world_update_render_entities(game_editor, editor_state->editor_assets, commands, dt);


	render_commands_End(commands);

	//
	// new commands group without depth
	//
	commands = render_commands_Begin(gameRenderer);
	//render_commands_SetProjection(commands);
	commands->render_flags = render_flags_Blending;
	commands->camera_type = render_camera_perspective;


	//
	//draw cursor
	//
	f32 cursor_alpha = 255.0f;
	draw_crosshair_cursor(commands,
			paintCursor_p,
			cursor_alpha);


	//draw tile hitted
	//draw tile hitted edges
	// highlight hitted tile
	if(editor_world_mode == editor_cursor_tiles)
	{
		if(tile_hitted)
		{

			vec4 lineColor = {0xff, 0xff, 0x00, 0xff};
			world_tile *current_tile = tile_array + tile_hitted_index;

			real32 lineSz = 0.2f;
			render_vertices_edges(commands,
					current_tile->v0,
					current_tile->v1,
					current_tile->v2,
					current_tile->v3,
					lineColor, lineSz);
		}

		//render rotation point or gizmo
		//draw rotation point
		if(game_editor->world.cursor_memory.selected_meshes_count)
		{

			editor_render_gizmo(commands,
					game_editor->world.rotation_point,
					hoveredRotationMidPoint || game_editor->world.moving_selections,
					in_camera_mode);
		}


		//get indices
		u32 t = 0;
		while(t < game_editor->world.cursor_memory.selected_meshes_count)
		{
			editor_mesh_selection *tile_selection = game_editor->world.cursor_memory.selected_meshes + t;

			u32 tile_index           = tile_selection->index;
			world_tile *current_tile = tile_array + tile_index;
			u32 hide = in_camera_mode || game_editor->world.moving_selections;

			editor_highlight_selected_mesh(commands,
					tile_selection,
					current_tile->v0,
					current_tile->v1,
					current_tile->v2,
					current_tile->v3,
					hide,
					game_editor->settings.vertices_cube_size);

			t++;
		}



		//selection tool hotkeys
	}

	//render entity circles
	if(editor_world_mode == editor_cursor_entities)
	{
		for(u32 e = 0; e < entity_count; e++)
		{
			world_entity *worldEntity = editor_entity_array + e;

			render_circle_upfront(commands, worldEntity->position, 5,  0.6f, V4(255, 255, 0, 230));
			//render_draw_cube_colored(commands, worldEntity->position, V3(20, 20, 20), V4(255, 0, 0, 255));
		}
	}

	//Draw line to detected tile
	//Got a tile!

	render_commands_End(commands);



}

static void
editor_world_update_render_ui(s_editor_state *editor_state,
		                      game_renderer *gameRenderer,
							  game_input *game_input)
{
	s_game_editor *game_editor = &editor_state->editor;
    game_world *gameWorld   = game_editor->world.loaded_world;
    world_tile *worldTerrain = gameWorld->tiles;
	game_assets *game_asset_manager = editor_state->editor_assets;

    uint32 *tile_count    = &gameWorld->tile_count; 

	asset_file_info *packed_assets = game_editor->packed_assets;
	u32 packed_assets_count         = game_editor->packed_assets_count;
//
// ui start
//
   // render_texture sceneTexture = assets_get_texture_by_id(editor_state->editor_assets, game_editor->world.loaded_world->texture); 
    game_ui *ui = editor_state->ui;

	u32 tileset_count = game_editor->world.world_tilesets_count;
	asset_file_info *tileset_array = game_editor->world.world_tilesets;

	//
	//Editor panel
	//

	u32 newMap = 0;
	u32 saveMap = 0;
	u32 loadMap = 0;
	u32 addModel = 0;

	u32 open_load_tileset_explorer = 0;

	
	//World editor tool bar
	f32 topbar_y = 30.0f;
	f32 toolbar_x = 0;
	f32 toolbar_y = topbar_y;
	f32 toolbar_w = gameRenderer->os_window_width;
	f32 toolbar_h = editor_TOP_BAR_H;
	ui_panel_begin(ui, ui_panel_flags_borders | ui_panel_flags_ignore_focus,
			       toolbar_x, toolbar_y, toolbar_w, toolbar_h, 
				  "Top bar world");
	{
		ui_keep_line_push(ui);
	    ui_push_element_sizeToText(ui);


		newMap = ui_button(ui, "New");
		saveMap = ui_button(ui, "Save");
        loadMap = ui_button(ui, "Load");
		addModel = ui_button(ui, "Add model");


		ui_AddAndPushCursor(ui, 60, 0);

		u32 tab_terrain  = ui_selectable(ui, game_editor->world.tab == editor_world_graphics, "Terrain");
		u32 tab_entities = ui_selectable(ui, game_editor->world.tab == editor_world_entities, "Livings");
		

		if(tab_terrain)
		{
			game_editor->world.tab = editor_world_graphics;
			game_editor->world.cursor_mode = editor_cursor_tiles;
		}
		else if(tab_entities)
		{
			game_editor->world.tab = editor_world_entities;
			game_editor->world.cursor_mode = editor_cursor_entities;
		}
		ui_keep_line_pop(ui);
		ui_pop_cursor(ui);
	    ui_pop_element_size(ui);
	}
	ui_panel_end(ui);
	if(1)
	{
	  ui_panel_begin(ui,ui_panel_flags_front_panel, 1024, 0, 300, 300, "Editor panel");

	  
	  ui_text(ui, "Editor data:");
      ui_textf(ui, "Terrain count: %u", *tile_count);
      ui_textf(ui, "used size: %u bytes", (*tile_count * sizeof(world_tile)));

	  
      ui_separator(ui, 4);
      
      static int checkVal = 0;
      
      //
      //Selected terrain data.
      //
      ui_separator(ui ,6);
        
        
        ui_panel_end(ui);
    }



	//
	//Tileset selection panel
	//
	if(game_editor->world.tab == editor_world_graphics)
	{
		f32 paintPanel_w = 400;
		f32 paintPanel_x = gameRenderer->os_window_width - paintPanel_w;
		f32 paintPanel_y = topbar_y + editor_TOP_BAR_H;
		f32 paintPanel_h = gameRenderer->os_window_height - paintPanel_y;

		ui_set_next_panel_position(ui, paintPanel_x, paintPanel_y);
		ui_set_next_panel_size(ui, paintPanel_w, paintPanel_h);

	    ui_panel_begin(ui, ui_panel_flags_scroll_v,
				      0, 0, 0, 0, "Paint world and tileset panel");
        {
	    	u32 tileset_count       = game_editor->world.world_tilesets_count;
	    	u32 packed_assets_count = game_editor->packed_assets_count;
			u32 tileset_index        = game_editor->world.selected_tileset_for_painting_index;
	        // &game_editor->panelTileset;
	        //ui_child_panel childPanel = ui_begin_child_panel(ui, "Child_Panel", 400, 400);
            game_assets *game_asset_manager		       = editor_state->editor_assets;
			asset_file_info *tileset_for_painting_info = 0;

			if(tileset_count && (tileset_index >= tileset_count))
			{
				tileset_index = tileset_count - 1;
	            game_editor->world.selected_tileset_for_painting_index = tileset_index;
			}
			//set info after correcting
			if(tileset_count)
			{
			    tileset_for_painting_info = game_editor->world.world_tilesets + tileset_index;
			}

			ui_tab_group_begin(ui, 0, "Cursor properties tab");
			{
				if(ui_tab_push(ui, "Cursor"))
				{
					//select tileset from the loaded tileset index
	                ui_begin_child_panel(ui, 300, 200, "Tileset list");
	    	        {

	                    for(u32 t = 0;
	                      	  t < tileset_count;
	                      	  t++)
	                    {
			        		asset_file_info *tilesetInfo = game_editor->world.world_tilesets + t; 

	    	          	    u32 selectableActive = game_editor->world.selected_tileset_for_painting_index == t;
	                        u32 selected         = ui_selectable(ui, selectableActive, tilesetInfo->path_and_name);
	                        if(selected)
	                        {
	                      	    game_editor->world.selected_tileset_for_painting_index = t;
			        			tileset_index = t;
	                        }

	                    }

						open_load_tileset_explorer = ui_selectable(ui, 0, "...");
	    	        }
	                ui_end_child_panel(ui);
			        ui_push_disable_if(ui, game_editor->world.loaded_world->tileset_count);
	                ui_button(ui, "Remove tileset");
			        ui_pop_disable(ui);
	                
	                ui_text(ui, "Load tileset");
			        u8 *preview = "-";
			        if(tileset_for_painting_info)
			        {
			        	preview = tileset_for_painting_info->path_and_name;
			        }

			        ui_text(ui, "Tile frame sizes");
			        ui_keep_line_push(ui);
			        ui_push_element_width(ui, 60);

			        ui_spinner_u16(ui, 1, 1, 96, &game_editor->world.frame_tile_size_x , 0,"tile frame size x");
			        ui_spinner_u16(ui, 1, 1, 96, &game_editor->world.frame_tile_size_y , 0,"tile frame size y");

			        ui_pop_element_size(ui);
			        ui_keep_line_pop(ui);

					vec2 frame_size = ui_get_frame_size(ui);

			        ui_begin_child_panel(
							ui,
							frame_size.x,
							400, "tileset selectable image");
					{
			            if(tileset_count)
			            {
			            	asset_tileset assetTileset = game_editor->world.tilesets_as_asset[tileset_index];
			            	world_tileset *tileset     = assets_get_tileset_by_key(game_asset_manager, assetTileset);

			            	u32 tile_count      = tileset->count;
			            	u32 tilesetImage_id = tileset->imageId;
			            	u32 tileSize        = tileset->tileSize;
			            	asset_image tilesetAssetImage  = assets_get_texture_by_id(game_asset_manager, tileset->imageId);
			            	render_texture *tilesetTexture = tilesetAssetImage.image;

			            	ui_image_selection_begin(ui,
			            			                  tilesetTexture,
			            							  game_editor->world.frame_tile_size_x,
			            							  game_editor->world.frame_tile_size_y,
			            							  0,
			            							  "Next tile frames");
			            	{
			            		u32 interacted_with_image = ui_image_selection_push_uv_selection(
										ui, 
			            				&game_editor->world.cursor.mesh.uv0,
			            				&game_editor->world.cursor.mesh.uv1,
			            				&game_editor->world.cursor.mesh.uv2,
			            				&game_editor->world.cursor.mesh.uv3);

								if(interacted_with_image)
								{
									game_editor->world.next_tile_animated = 0;
								}
			            	}
			            	ui_image_selection_end(ui);
			            }
				    }
			        ui_end_child_panel(ui);
					ui_begin_child_panel_flags(
							ui,
							ui_panel_flags_title | ui_panel_flags_borders,
							frame_size.x,
							200,
							"Animated tiles");
					{
						if(tileset_count)
						{
			            	asset_tileset asset_tileset = game_editor->world.tilesets_as_asset[tileset_index];
			            	world_tileset *tileset      = assets_get_tileset_by_key(game_asset_manager, asset_tileset);
							u32 animated_tiles_count    = tileset->animated_tiles_count;
		                    asset_image tilesetImage      = assets_get_texture_by_id(game_asset_manager, tileset->imageId);

			                for(u32 t = 0;
			                		t < tileset->animated_tiles_count;
			                		t++)
			                {
			                	//pick animation from array
			                	world_tileset_animation *current_animation      = tileset->animated_tiles + t;
			                	//pick the tile depending on the frame index
			                	world_tileset_tile *current_tile_from_animation = tileset->tiles + current_animation->tiles_at + current_animation->frames_current;

			                	u32 selectable_active = game_editor->world.next_tile_animated && game_editor->world.next_tile_animation_index == t;

                                u32 selected_new_animation = ui_selectable_image_uv(
			                			ui,
                                		selectable_active,
                                		tilesetImage.image,
                                		80,
                                		80,
                                		current_tile_from_animation->uv0,
                                		current_tile_from_animation->uv1,
                                		current_tile_from_animation->uv2,
                                		current_tile_from_animation->uv3);

			                	if(selected_new_animation)
			                	{
			                		//game_editor->tileset.selected_tileset_animation_index = t;
									game_editor->world.next_tile_animation_index = t;
									game_editor->world.next_tile_animated = 1;
			                		
			                	}
			                }
						}
					}
					ui_end_child_panel(ui);
#if 0
		         	ui_begin_child_panel_flags(ui, 
		         			                 ui_panel_flags_title,
		         							 model_cursor_panel_w,
		         							 300,
		         							 "Cursor properties");
		         	{
	             	    ui_text(ui, "Selection size w/h");
	             	    ui_keep_line_push(ui);
	             	    ui_push_element_width(ui, 80);
	             	    ui_spinner_u32(ui, 1, 1, 96, &game_editor->model.cursor_selection_size_x, 0, "selection size w");
	             	    ui_spinner_u32(ui, 1, 1, 96, &game_editor->model.cursor_selection_size_y, 0, "selection size h");
	             	    ui_pop_element_size(ui);
	             	    ui_keep_line_pop(ui);


		         	    ui_text(ui, "Cursor displacement");
	             	    ui_spinner_u32(ui, 1, 1, 96, &game_editor->model.cursor.tile_displacement, 0, "selection size h");


		         	    ui_text(ui, "Rotation values");
		         	    ui_keep_line_push(ui);
		         	    ui_push_element_width(ui, 80);


		         	    ui_spinner_i32(ui, 1, -360, 360, &game_editor->model.gizmo_rotation_x, 0, "gizmo rotation x");
		         	    ui_spinner_i32(ui, 1, -360, 360, &game_editor->model.gizmo_rotation_y, 0, "gizmo rotation y");
		         	    ui_spinner_i32(ui, 1, -360, 360, &game_editor->model.gizmo_rotation_z, 0, "gizmo rotation z");

		         	    if(ui_button(ui, "Apply to selected tiles"))
		         	    {
		         	    	game_editor->model.apply_gizmo_rotation = 1;
		         	    }

		         	    ui_keep_line_pop(ui);
		         	    ui_pop_element_size(ui);
		         	}
		         	ui_end_child_panel(ui);
#endif

			        ui_keep_line_pop(ui);
			        ui_pop_element_size(ui);
				}
				if(ui_tab_push(ui, "Selected tiles"))
				{

         			//edit selection test
         			ui_text(ui, "uvs");
					u32 selected_meshes_count = game_editor->world.cursor_memory.selected_meshes_count;
         		    u32 tileset_id            = game_editor->world.selected_meshes_tileset_id;

					//get the information from the asset group
	             	asset_file_info *selected_tileset = editor_get_disk_file_info_from_id(game_editor, tileset_id);
					//load preview if the asset is loaded
	             	u8 *preview = "-";
	             	if(selected_tileset)
	             	{
	             		preview = selected_tileset->path_and_name;
	             	}


					ui_begin_drop_down(ui, "selected meshes loaded tilesets", preview);
					{
						for(u32 t = 0; t < gameWorld->tileset_count; t++)
	             	    {
	             	        asset_file_info *asset_data_and_name = game_editor->world.world_tilesets + t;
							//only select images
	             			u32 asset_id     = asset_data_and_name->data.id;
	             			u8 *path_and_name = asset_data_and_name->path_and_name;

	             	     	u32 selected_clicked =  ui_selectable(ui,
	             			 	             (tileset_id) == asset_id,
	             			 				 path_and_name);
	             	    	if(selected_clicked)
	             	    	{
						        tileset_id = asset_id;
						        game_editor->world.selected_meshes_tileset_id = tileset_id;
	             	    	}
	             	    }
					}
					ui_end_drop_down(ui);
					

					ui_begin_child_panel(ui, 512, 512, "Edit selection uvs");
					{
         			    if(tileset_id && selected_meshes_count > 0)
         			    {
					    	//get tileset and image
         		        	asset_tileset assetTileset   = assets_get_tileset_id(game_asset_manager, tileset_id);
         		        	asset_image tilesetTileImage = assets_get_texture_by_id(game_asset_manager, assetTileset.tileset->imageId);
         	            	render_texture *tileTexture  = tilesetTileImage.image;
         
					    	ui_image_selection_begin(ui,
					    			                  tileTexture,
					    							  game_editor->world.frame_tile_size_x,
					    							  game_editor->world.frame_tile_size_y,
					    							  0,
					    							  "Selected tiles uvs");
					    	{
					    	    u32 s = 0;
					    	    while(s < selected_meshes_count)
					    	    {
         			    	        u32 tile_index = game_editor->world.cursor_memory.selected_meshes[s].index;
         
         			    	        world_tile *current_tile = game_editor->world.loaded_world->tiles + tile_index;
         		        	        u32 tile_tileset_id      = game_editor->world.tilesets_as_asset[current_tile->tileset_index].key.id;
					    			if(tile_tileset_id == tileset_id)
					    			{
         
					    	    	    s++;

	             	            	    ui_image_selection_push_uv_selection(ui, 
         			    	    	                                          &current_tile->uv0,
         			    	    	                                          &current_tile->uv1,
         			    	    	                                          &current_tile->uv2,
         			    	    	                                          &current_tile->uv3);
					    			}

					    	    }

					    	}
					    	ui_image_selection_end(ui);
			            }
					}
					ui_end_child_panel(ui);
			   }
			}
			ui_tab_group_end(ui);

			
	        
	    }
	    ui_panel_end(ui);


		//
		//tool bar panel
		//tool panel
		f32 toolbar_x = 0;
		f32 toolbar_y = topbar_y + editor_TOP_BAR_H;
		f32 toolbar_w = 30;
		ui_set_next_panel_position(ui, 0, toolbar_y);
		ui_set_next_panel_size(ui, toolbar_w, 900);
		ui_panel_begin(ui, ui_panel_flags_borders | ui_panel_flags_ignore_focus, 0, 0, 0, 0, "tool bar");
		{
			ui_push_element_size(ui, 20, 20);
			u16 selected_paint     = game_editor->world.tool == world_tool_paint;
			u16 selected_selection = game_editor->world.tool == world_tool_selection;
			u16 selected_bucket    = game_editor->world.tool == world_tool_bucket;
			u16 selected_grid      = game_editor->world.tool == world_tool_grid;
			
			if(ui_selectable(ui, selected_paint, "P"))
			{
				game_editor->world.tool = world_tool_paint;
			}
			if(ui_selectable(ui, selected_selection, "S"))
			{
				game_editor->world.tool = world_tool_selection;
			}
			if(ui_selectable(ui, selected_bucket, "B"))
			{
				game_editor->world.tool = world_tool_bucket;
			}
			if(ui_selectable(ui, selected_grid, "G"))
			{
				game_editor->world.tool = world_tool_grid;
			}
			if(ui_selectablef(ui, selected_grid, "C%u", game_editor->world.camera_mode))
			{
				game_editor->world.camera_mode++;
				game_editor->world.camera_mode %= editor_world_camera_mode_total;
			}
			

			ui_pop_element_size(ui);
		}
		ui_panel_end(ui);

		//axes selection

		ui_set_next_panel_position(ui, toolbar_w, toolbar_y);
		ui_set_next_panel_size(ui, 260, 160);

		ui_panel_begin(ui, ui_panel_flags_borders | ui_panel_flags_ignore_focus, 0, 0, 0, 0, "paint tool axes");
		{
		    if(game_editor->world.tool == world_tool_paint)
		    {

		    	ui_keep_line_push(ui);
		    	ui_push_element_size(ui, 20, 20);


		    	ui_pop_element_size(ui);
		    	ui_keep_line_pop(ui);

				ui_check_box(ui, &game_editor->world.stick_to_near_tile, "Stick to tile");
				ui_check_box(ui, &game_editor->world.repaint_tile, "Repaint");
		    }
			else if(game_editor->world.tool == world_tool_grid)
			{
				//move grid when activated
				ui_selectable_toggle_u32(ui, &game_editor->world.move_reference_grid, "Move");


				ui_text(ui, "tile movement");
				ui_spinner_f32(ui, 1, 1, 96, &game_editor->world.cursor.tile_displacement, 0, "cursor tile movement");

                ui_text(ui, "Cursor settings:");
                ui_textf(ui, "Position:");
                
                ui_keep_line_push(ui);
                ui_drag_f32(ui, (f32)game_editor->world.move_reference_grid, -1.0f, 50.0f, &game_editor->world.cursor.position.x, "gridPx");
                ui_drag_f32(ui, (f32)game_editor->world.move_reference_grid, -1.0f, 50.0f, &game_editor->world.cursor.position.y, "gridPy");
                ui_drag_f32(ui, (f32)game_editor->world.move_reference_grid, -1.0f, 50.0f, &game_editor->world.cursor.position.z, "gridPz");
                ui_keep_line_pop(ui);
			}
		}
		ui_panel_end(ui);
	} //tab == graphics

	if(game_editor->world.tab == editor_world_entities)
	{
        //
        // world_entitiess model tab
        // Entity panel
		u32 deleteEntity = 0;
		f32 world_entities_panel_x = 0;
		f32 world_entities_panel_y = toolbar_y + toolbar_h;
		f32 world_entities_panel_h = gameRenderer->os_window_height - world_entities_panel_y;
		f32 world_entities_panel_w = 300;

		ui_set_next_panel_position(ui, world_entities_panel_x, world_entities_panel_y);
		ui_set_next_panel_size(ui, world_entities_panel_w, world_entities_panel_h);
        ui_panel_begin(ui, ui_panel_flags_scroll_v | ui_panel_flags_title, 0, 200, 300, 600, "World entities");
        {
	    	u32 entity_count = gameWorld->entity_count;
	    	ui_begin_child_panel(ui, 200, 200, "Entity list");
	    	{
	    		for(u32 e = 0; e < gameWorld->entity_count; e++)
	    		{
	    			u32 selected = ui_selectablef(ui,game_editor->world.selected_entity_index == e, "Entity %u", e);
	    			if(selected)
	    			{
	    				game_editor->world.selected_entity_index = e;
	    			}
	    		}
	    	}
	    	ui_end_child_panel(ui);

			//temp
			deleteEntity = ui_button(ui, "Delete");

	    	if(entity_count)
	    	{
	    		if(game_editor->world.selected_entity_index > entity_count)
	    		{
	    			game_editor->world.selected_entity_index = 0;
	    		}

	    		world_entity *selected_entity_index = gameWorld->entities + game_editor->world.selected_entity_index;

	    		//select model
	    		ui_text(ui, "Model");
				ui_text(ui, "*insert model selection");

	    		ui_keep_line_push(ui);
	    		ui_push_element_size(ui, 60, 0);

	    		//ui_drag_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.x, "entity_x");
	    		//ui_drag_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.y, "entity_y");
	    		//ui_drag_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.z, "entity_z");

	    		ui_spinner_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.x, 0, "entity_x");
	    		ui_spinner_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.y, 0, "entity_y");
	    		ui_spinner_f32(ui, 0.1f, -F32MAX, F32MAX, &selected_entity_index->position.z, 0, "entity_z");

	    		ui_pop_element_size(ui);
	    		ui_keep_line_pop(ui);

				if(deleteEntity)
				{
					editor_world_delete_entity(game_editor, game_editor->world.selected_entity_index);
				}


	    	}


	    }
	    ui_panel_end(ui);
	}
    //
    //Mini side info panel
    //

    f32 miniPanelW = 200;
	f32 miniPanelH = 100;
    ui_set_next_panel_position(ui, 40, gameRenderer->os_window_height - miniPanelH - 40);
	ui_set_next_panel_size(ui, 260, 200);
    ui_panel_begin(ui, ui_panel_flags_NoInteraction, 40, 40, 200, miniPanelH, "Editor info");
    {
		  u8 *editorCursorModes[editor_cursor_mode_Total] = {
			  "AddDelete",
			  "AddCollisions",
			  "Entity",
		  };
		  ui_keep_line_push(ui);
          ui_textf(ui, "Mouse mode: ");
		  ui_textf(ui, "%s", editorCursorModes[game_editor->world.cursor_mode]);
		  ui_keep_line_pop(ui);
		  ui_textf(ui, "Mouse timer: %f", game_editor->world.cursor.move_down_timer);


		  //FOR REFERENCE. This should be removed.
		  f32 gridInner = vec3_inner(game_editor->world.reserved_vec, game_editor->world.reserved_vec);
		  ui_textf(ui, "Mouse at grid:\n{x:%f, y:%f, z:%f}", game_editor->world.reserved_vec.x, game_editor->world.reserved_vec.y, game_editor->world.reserved_vec.z);
		  ui_textf(ui, "Hot vertices count:%u", game_editor->world.cursor_memory.hot_vertices_count);

		  ui_text(ui, "world_cursor_tile_displacement");
		  ui_same_line(ui);
		  ui_spinner_f32(ui, 1, 1, 32, &game_editor->world.cursor.tile_displacement, 0, "world_cursor_tile_displacement");

    }
    ui_panel_end(ui);

	//
	// end functionality
	//

	if(open_load_tileset_explorer)
	{
		ui_explorer_set_path(ui, DATA_PATH);
		ui_explorer_set_flags_and_process(
				ui,
				ui_explorer_process_select_file,
				ui_explorer_flags_copy_selected_file_name | ui_explorer_flags_close_on_complete,
				"Add tileset to map");
	}

	if(saveMap)
	{
       //editor_world_save_map(game_editor, editor_state->editor_assets, editor_state->platform, "DEF00.world");
	   ui_explorer_set_path(ui, DATA_FOLDER("world"));
	   ui->explorer->flags = ui_explorer_flags_copy_selected_file_name | ui_explorer_flags_close_on_complete;
	   ui_explorer_set_process(ui, ui_explorer_process_select_file | ui_explorer_process_type_file, "Save map");
	}
	if(loadMap)
	{

       //editor_world_load_map(editor_state, editor_state->editor, editor_state->platform, "DEF00.world");
	   ui_explorer_set_path(ui, DATA_FOLDER("world"));
	   ui->explorer->flags = ui_explorer_flags_copy_selected_file_name | ui_explorer_flags_close_on_complete;
	   ui_explorer_set_process(ui, ui_explorer_process_select_file , "Load map");
        
	}
	if(newMap)
	{
	    editor_world_reset(&editor_state->editor);
	}


//Process check for world editor
	if(ui_explorer_check_process(ui, "Save map"))
	{
	    u8 mapName[256] = {0};
        ui_explorer_get_file_path(ui, mapName, sizeof(mapName));
        editor_world_save_map(game_editor, editor_state->editor_assets, editor_state->platform, mapName);

	}
	if(ui_explorer_check_process(ui, "Load map"))
	{
	    u8 *mapName = ui->explorer->selectedFilePathAndName;
        editor_world_load_map(editor_state, &editor_state->editor, editor_state->platform, mapName);
	}
	if(ui_explorer_check_process(ui, "Add tileset to map"))
	{
		u8 *tileset_name = ui->explorer->selectedFilePathAndName;
		editor_world_add_tileset(game_editor,
				                 game_asset_manager,
				                 tileset_name);
	}

}

