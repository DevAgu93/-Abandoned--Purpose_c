static inline void
game_render_cube_sides(
		render_texture texture,
		vec3 p,
		u32 tile_size,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
}

typedef enum{
	terrain_shape_cube,
	terrain_shape_slope_u
}world_terrain_shape;

static inline void
game_render_cube(
		render_commands *render_commands,
		render_texture texture,
		vec3 displacement,
		u32 tile_size,
		vec2 t_uv0,
		vec2 t_uv1,
		vec2 t_uv2,
		vec2 t_uv3,
		vec2 wall_uv0,
		vec2 wall_uv1,
		vec2 wall_uv2,
		vec2 wall_uv3
		)
{

	//top
	vec3 v0 = vec3_add(displacement, V3(0, 0, GAME_TILESIZE));
	vec3 v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v3 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));

	vec2 uv0 = t_uv0;
	vec2 uv1 = t_uv1;
	vec2 uv2 = t_uv2;
	vec2 uv3 = t_uv3;

	render_push_quad(render_commands,
			&texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255));

	uv0 = wall_uv0;
	uv1 = wall_uv1;
	uv2 = wall_uv2;
	uv3 = wall_uv3;

	//front
	vec3 yp_v0 = vec3_add(displacement, V3(0, 0, 0));
	vec3 yp_v1 = vec3_add(displacement, V3(0, 0, GAME_TILESIZE));
	vec3 yp_v2 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));
	vec3 yp_v3 = vec3_add(displacement, V3(GAME_TILESIZE, 0, 0));

	vec3 xn_v0 = vec3_add(displacement, V3(0, GAME_TILESIZE, 0));
	vec3 xn_v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xn_v2 = vec3_add(displacement, V3(0, 0, GAME_TILESIZE));
	vec3 xn_v3 = vec3_add(displacement, V3(0, 0, 0));

	vec3 xp_v0 = vec3_add(displacement, V3(GAME_TILESIZE, 0, 0));
	vec3 xp_v1 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));
	vec3 xp_v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xp_v3 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, 0));


	render_push_quad(render_commands,
			&texture,
			yp_v0,
			yp_v1,
			yp_v2,
			yp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );

	render_push_quad(render_commands,
			&texture,
			xp_v0,
			xp_v1,
			xp_v2,
			xp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );

	render_push_quad(render_commands,
			&texture,
			xn_v0,
			xn_v1,
			xn_v2,
			xn_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );
}

static inline void
game_render_cube_triangular(
		render_commands *render_commands,
		render_texture texture,
		vec3 displacement,
		u32 tile_size,
		vec2 t_uv0,
		vec2 t_uv1,
		vec2 t_uv2,
		vec2 t_uv3,
		vec2 wall_uv0,
		vec2 wall_uv1,
		vec2 wall_uv2,
		vec2 wall_uv3
		)
{

	//top
	vec3 v0 = vec3_add(displacement, V3(8, 0, GAME_TILESIZE));
	vec3 v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v3 = vec3_add(displacement, V3(8, 0, GAME_TILESIZE));

	vec2 uv0 = t_uv0;
	vec2 uv1 = t_uv1;
	vec2 uv2 = t_uv2;
	vec2 uv3 = t_uv3;

	render_push_quad(render_commands,
			&texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255));

	uv0 = wall_uv0;
	uv1 = wall_uv1;
	uv2 = wall_uv2;
	uv3 = wall_uv3;

	vec3 xn_v0 = vec3_add(displacement, V3(0, GAME_TILESIZE, 0));
	vec3 xn_v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xn_v2 = vec3_add(displacement, V3(8, 0, GAME_TILESIZE));
	vec3 xn_v3 = vec3_add(displacement, V3(8, 0, 0));

	vec3 xp_v0 = vec3_add(displacement, V3(8, 0, 0));
	vec3 xp_v1 = vec3_add(displacement, V3(8, 0, GAME_TILESIZE));
	vec3 xp_v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xp_v3 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, 0));

	render_push_quad(render_commands,
			&texture,
			xp_v0,
			xp_v1,
			xp_v2,
			xp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );

	render_push_quad(render_commands,
			&texture,
			xn_v0,
			xn_v1,
			xn_v2,
			xn_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );
}
static inline void
game_render_cube_custom(
		render_commands *render_commands,
		render_texture texture,
		vec3 displacement,
		u32 tile_size,
		vec2 t_uv0,
		vec2 t_uv1,
		vec2 t_uv2,
		vec2 t_uv3,
		vec2 wall_uv0,
		vec2 wall_uv1,
		vec2 wall_uv2,
		vec2 wall_uv3
		)
{

	vec3 v0 = vec3_add(displacement, V3(0, 0, GAME_TILESIZE));
	vec3 v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 v3 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));

	vec2 uv0 = t_uv0;
	vec2 uv1 = t_uv1;
	vec2 uv2 = t_uv2;
	vec2 uv3 = t_uv3;

	render_push_quad(render_commands,
			&texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255));

	uv0 = wall_uv0;
	uv1 = wall_uv1;
	uv2 = wall_uv2;
	uv3 = wall_uv3;

	vec3 yp_v0 = vec3_add(displacement, V3(3, 3, 0));
	vec3 yp_v1 = vec3_add(displacement, V3(3, 3, GAME_TILESIZE));
	vec3 yp_v2 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));
	vec3 yp_v3 = vec3_add(displacement, V3(GAME_TILESIZE, 0, 0));

	vec3 xn_v0 = vec3_add(displacement, V3(0, GAME_TILESIZE, 0));
	vec3 xn_v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xn_v2 = vec3_add(displacement, V3(3, 3, GAME_TILESIZE));
	vec3 xn_v3 = vec3_add(displacement, V3(3, 3, 0));

	vec3 xp_v0 = vec3_add(displacement, V3(GAME_TILESIZE, 0, 0));
	vec3 xp_v1 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));
	vec3 xp_v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
	vec3 xp_v3 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, 0));


	render_push_quad(render_commands,
			&texture,
			yp_v0,
			yp_v1,
			yp_v2,
			yp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );

	render_push_quad(render_commands,
			&texture,
			xp_v0,
			xp_v1,
			xp_v2,
			xp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );

	render_push_quad(render_commands,
			&texture,
			xn_v0,
			xn_v1,
			xn_v2,
			xn_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			vec4_all(255) );
}














static inline void
game_draw_terrain(
		game_world *game_world,
		game_assets *game_asset_manager,
		render_commands *render_commands,
		u32 x,
		u32 y,
		i32 offset_x,
		i32 offset_y,
		vec4 color)
{
	//calculate the index
	u32 t = x + (y * game_world->w);
	//calculate location
	vec3 displacement = 
	{((i32)x + offset_x) * (f32)GAME_TILESIZE,
	 ((i32)y + offset_y) * (f32)GAME_TILESIZE, 0};

	world_tile *current_tile = game_world->tiles + t;
	//select a tileset
	u32 tileset_index = current_tile->tileset_index;
	//select terrain from tileset
	u32 tileset_terrain_index = current_tile->tileset_terrain_index;
	world_tileset *tileset = 
		game_world->tilesets_a[tileset_index];
	//;Clean this
	if(!tileset)
	{
		return;
	}

	if(y == 10 && x == 12)
	{
		//tileset_terrain_index = 1;
		//current_tile->height = 1;
	}

	s_tileset_terrain *tileset_terrain = 
		tileset->terrain + tileset_terrain_index;
	render_texture *tileset_texture = tileset->image;

	i32 height = current_tile->height;
	u32 shape = tileset_terrain->shape;
	shape = terrain_shape_cube;

	if(y == 10 && x == 12)
	{
		//shape = 20;
	}


	//if(y == 10 && x > 12)
	//{
	//	height = 4;
	//}
	//if(y == 9 && x > 10)
	//{
	//	height = 3;
	//}
	////For testing
	//if(y == 20 && x == 4)
	//{
	//	height = 20;
	//}
	//if(y == 20 && x == 5)
	//{
	//	height = 1;
	//	shape = terrain_shape_slope_u;
	//}
	//if(y == 20 && x == 6)
	//{
	//	height = 1;
	//	shape = 2;
	//}

	//start at the top

	model_mesh *top_mesh = tileset->meshes + tileset_terrain->uvs_at_vertices_at + 0;
	//the vertices
	b32 repeat = 0;
	i32 top_side = 0;
	//get to the top
	//I might not need to subtract another tile size?
	displacement.z = height * (f32)GAME_TILESIZE - GAME_TILESIZE;

	//set up top vertices
	vec3 v0 = displacement;
	vec3 v1 = displacement;
	vec3 v2 = displacement;
	vec3 v3 = displacement;

	vec2 uv0 = tileset_terrain->uv0;
	vec2 uv1 = tileset_terrain->uv1;
	vec2 uv2 = tileset_terrain->uv2;
	vec2 uv3 = tileset_terrain->uv3;
	u32 tw = 0;
	u32 th = 0;
	b32 use_ground = 0;
	{
		if(current_tile->tileset_terrain_frame > 0)
		{
			int s = 0;
		}
		et_fill_capacity_data(tileset_terrain->capacity, &tw, &th);
		f32 tx = (f32)((current_tile->tileset_terrain_frame) % tw);
		f32 ty = (f32)((current_tile->tileset_terrain_frame) / tw);
		tx = (tx * GAME_TILESIZE) + (2 * tx);
		ty = (ty * GAME_TILESIZE) + (2 * ty);
		tx /= tileset_texture->width;
		ty /= tileset_texture->height;

		uv0.x += tx;
		uv0.y += ty;
		uv1.x += tx;
		uv1.y += ty;
		uv2.x += tx;
		uv2.y += ty;
		uv3.x += tx;
		uv3.y += ty;
	}
	switch(tileset_terrain->shape)
	{
		default:
			{
				v0 = vec3_add(v0, V3(0, 0, GAME_TILESIZE));
				v1 = vec3_add(v1, V3(0, GAME_TILESIZE, GAME_TILESIZE));
				v2 = vec3_add(v2, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
				v3 = vec3_add(v3, V3(GAME_TILESIZE, 0, GAME_TILESIZE));

			}break;
//		case slope_floor_b:
//			{
//				v0 = vec3_add(v0, V3(0, 0, 0));
//				v1 = vec3_add(v1, V3(0, GAME_TILESIZE, GAME_TILESIZE));
//				v2 = vec3_add(v2, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
//				v3 = vec3_add(v3, V3(GAME_TILESIZE, 0, 0));
//			}break;
		case slope_wall_bl:
			{
				v0 = vec3_add(v0, V3(0, GAME_TILESIZE, GAME_TILESIZE));
				v1 = vec3_add(v1, V3(0, GAME_TILESIZE, GAME_TILESIZE));
				v2 = vec3_add(v2, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
				v3 = vec3_add(v3, V3(GAME_TILESIZE, 0, GAME_TILESIZE));

				f32 add_uv = 12.0f / tileset_texture->height;
				uv0.y -= add_uv * 1;
				use_ground = 1;
			}break;
	}
	add_y_bias_to_vertices(
			&v0, &v1, &v2, &v3, render_commands->gameRenderer->sprite_skew);
	//render the top
	render_push_quad(
			render_commands,
			tileset_texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);
	//draw walls if needed
	switch(tileset_terrain->shape)
	{
		default:
			{
			}break;
	}

	if(tileset_terrain->shape != cube && tileset_terrain->uvs_count > 1)
	{
	}


	if(tileset_terrain->use_wall && tileset_terrain->wall_index < tileset->wall_count)
	{
		tileset_wall *wall = tileset->walls + tileset_terrain->wall_index;
		vertices wall_vertices[4];
		u32 wall_vertices_count = 3;
		u8 wall_heights[3] = {0};
		u8 wall_l = 0;
		u8 wall_b = 1;
		u8 wall_r = 2;
		//detect side height maps
		{
			i32 ox = x - 1;
			i32 oy = y;
			//left tile
			if(ox >= 0)
			{
				u32 i = ox + (oy * game_world->w);
				world_tile *other_tile = game_world->tiles + i;
				if(current_tile->height > other_tile->height)
				{
					wall_heights[wall_l] = current_tile->height - other_tile->height;
				}
			}
			ox += 2;
			//right tile
			if(ox < (i32)game_world->w)
			{
				u32 i = ox + (oy * game_world->w);
				world_tile *other_tile = game_world->tiles + i;
				if(current_tile->height > other_tile->height)
				{
					wall_heights[wall_r] = current_tile->height - other_tile->height;
				}
			}
			//bottom tile
			ox = x;
			oy = y - 1;
			if(oy >= 0)
			{
				u32 i = ox + (oy * game_world->w);
				world_tile *other_tile = game_world->tiles + i;
				if(current_tile->height > other_tile->height)
				{
					wall_heights[wall_b] = current_tile->height - other_tile->height;
				}
			}

		}
			
		//determine how many walls will be used
		switch(tileset_terrain->shape)
		{
			case slope_wall_only_bl: case slope_wall_bl:
				{

					wall_vertices[0].v0 = V3(GAME_TILESIZE, 0, 0);
					wall_vertices[0].v1 = V3(GAME_TILESIZE, 0, GAME_TILESIZE);
					wall_vertices[0].v2 = V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[0].v3 = V3(GAME_TILESIZE, GAME_TILESIZE, 0);

					wall_vertices[1].v0 = V3(0, GAME_TILESIZE, 0);
					wall_vertices[1].v1 = V3(0, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[1].v2 = V3(GAME_TILESIZE, 0, GAME_TILESIZE);
					wall_vertices[1].v3 = V3(GAME_TILESIZE, 0, 0);
					wall_vertices_count = 2;
					use_ground = 1;
				}break;
			case slope_wall_only_br: case slope_wall_br:
				{

					wall_vertices[1].v0 = V3(0, 0, 0);
					wall_vertices[1].v1 = V3(0, 0, GAME_TILESIZE);
					wall_vertices[1].v2 = V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[1].v3 = V3(GAME_TILESIZE, GAME_TILESIZE, 0);

					wall_vertices[2].v0 = V3(0, GAME_TILESIZE, 0);
					wall_vertices[2].v1 = V3(0, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[2].v2 = V3(0, 0, GAME_TILESIZE);
					wall_vertices[2].v3 = V3(0, 0, 0);
					wall_vertices_count = 3;
					use_ground = 1;
				}break;
			//cube
			default:
				{
					wall_vertices[0].v0 = V3(0, GAME_TILESIZE, 0);
					wall_vertices[0].v1 = V3(0, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[0].v2 = V3(0, 0, GAME_TILESIZE);
					wall_vertices[0].v3 = V3(0, 0, 0);

					wall_vertices[1].v0 = V3(0, 0, 0);
					wall_vertices[1].v1 = V3(0, 0, GAME_TILESIZE);
					wall_vertices[1].v2 = V3(GAME_TILESIZE, 0, GAME_TILESIZE);
					wall_vertices[1].v3 = V3(GAME_TILESIZE, 0, 0);


					wall_vertices[2].v0 = V3(GAME_TILESIZE, 0, 0);
					wall_vertices[2].v1 = V3(GAME_TILESIZE, 0, GAME_TILESIZE);
					wall_vertices[2].v2 = V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE);
					wall_vertices[2].v3 = V3(GAME_TILESIZE, GAME_TILESIZE, 0);
				}
		}
		u32 wall_uvs_count = wall->uvs_count;
		repeat = wall->repeat;
		u32 uvs_index = 0;
		f32 z_start = displacement.z;
		u32 wall_frame_x = (current_tile->wall_frame * GAME_TILESIZE + (current_tile->wall_frame * 2));
		for(u32 v = 0; v < wall_vertices_count; v++)
		{
			//reset z
			u32 i[3] = {0, 1, 2};
			displacement.z = z_start;
			uvs_index = 0;
			for(u32 h = 0; h < wall_heights[v]; h++)
			{
				if(wall->extra_frames_y > 0)
				{
					int s = 0;
				}
				//pick uvs
#if 0
				model_mesh *wall_uvs = tileset->meshes + wall->uvs_at;
				vec2 uv0 = wall_uvs->uv0;
				vec2 uv1 = wall_uvs->uv1;
				vec2 uv2 = wall_uvs->uv2;
				vec2 uv3 = wall_uvs->uv3;
#endif

				vec2 uv0 = wall->uv0;
				vec2 uv1 = wall->uv1;
				vec2 uv2 = wall->uv2;
				vec2 uv3 = wall->uv3;
				vec3 v0 = vec3_add(displacement, wall_vertices[i[v]].v0);
				vec3 v1 = vec3_add(displacement, wall_vertices[i[v]].v1);
				vec3 v2 = vec3_add(displacement, wall_vertices[i[v]].v2);
				vec3 v3 = vec3_add(displacement, wall_vertices[i[v]].v3);
				add_y_bias_to_vertices(
						&v0, &v1, &v2, &v3, render_commands->gameRenderer->sprite_skew);
				u32 fx, fy, fw, fh;
				render_fill_frames_from_uvs(
						tileset_texture->width,
						tileset_texture->height,
						uv0, uv1, uv2, uv3,
						&fx, &fy, &fw, &fh);
			
				u32 frame_y = (uvs_index * GAME_TILESIZE + (uvs_index * 2));
				f32 frame_add = frame_y / (f32)tileset_texture->height;
				f32 frame_add_x = wall_frame_x / (f32)tileset_texture->width;
				uv0.y += frame_add;
				uv1.y += frame_add;
				uv2.y += frame_add;
				uv3.y += frame_add;
				uv0.x += frame_add_x;
				uv1.x += frame_add_x;
				uv2.x += frame_add_x;
				uv3.x += frame_add_x;
				uvs_index++;
				if(uvs_index > wall->extra_frames_y)
				{
					uvs_index = repeat ? 0 : wall->extra_frames_y;
				}
#if 0
				if(wall_x >= wall_w)
				{
					wall_x = 0;
				}
				if(wall_y >= wall_h)
				{
					wall_y = repeat ? 0 : wall_h - 1;
				}
#endif
				render_push_quad(
						render_commands,
						tileset_texture,
						v0,
						v1,
						v2,
						v3,
						uv0,
						uv1,
						uv2,
						uv3,
						color);
				displacement.z -= GAME_TILESIZE;
			}
		}
		//draw ground if needed
		f32 lowest_height = 0;
		for(u32 w = 0; w < 3; w++)
		{
			lowest_height = wall_heights[w] > lowest_height ? wall_heights[w] : lowest_height;
		}
		if(use_ground)
		{
			displacement.z = z_start;
			displacement.z -= lowest_height * GAME_TILESIZE;
			//this should start from the "lowest" wall on tileset
#if 0
			model_mesh *wall_uvs = tileset->meshes + wall->uvs_at;
			vec2 guv0 = wall_uvs->uv0;
			vec2 guv1 = wall_uvs->uv1;
			vec2 guv2 = wall_uvs->uv2;
			vec2 guv3 = wall_uvs->uv3;
#endif

			vec2 guv0 = wall->uv0;
			vec2 guv1 = wall->uv1;
			vec2 guv2 = wall->uv2;
			vec2 guv3 = wall->uv3;
			//advance one frame below
			f32 frame_x = (f32)wall_frame_x / tileset_texture->width;
			u32 frame_y = wall->extra_frames_y + 1;
			f32 frame = ((frame_y * GAME_TILESIZE + (frame_y * 2)) / (f32)tileset_texture->width);
			guv0.y += frame;
			guv1.y += frame;
			guv2.y += frame;
			guv3.y += frame;
			guv0.x += frame_x;
			guv1.x += frame_x;
			guv2.x += frame_x;
			guv3.x += frame_x;

			vec3 v0 = vec3_add(displacement, V3(0, 0, GAME_TILESIZE));
			vec3 v1 = vec3_add(displacement, V3(0, GAME_TILESIZE, GAME_TILESIZE));
			vec3 v2 = vec3_add(displacement, V3(GAME_TILESIZE, GAME_TILESIZE, GAME_TILESIZE));
			vec3 v3 = vec3_add(displacement, V3(GAME_TILESIZE, 0, GAME_TILESIZE));
			render_push_quad(
					render_commands,
					tileset_texture,
					v0,
					v1,
					v2,
					v3,
					guv0,
					guv1,
					guv2,
					guv3,
					color);
		}

	}
}


static void
game_build_world_geometry(
		game_world *game_world,
		game_assets *game_asset_manager,
		render_commands *render_commands)
{

	//game_world *gameWorld = game_world->loaded_world;

    //
	//Draw every terrain
	//
	//prevent locking if the texture is not yet allocated
	for(u32 t = 0;
			t < game_world->tileset_count;
			t++)
	{
		world_tileset *tileset = 
			game_world->tilesets_a[t];
		//;Clean this
		if(!tileset)
		{
			continue;
		}
		render_texture *tilesetTileImage = tileset->image; 
		if(!tilesetTileImage || tilesetTileImage->index == 0)
		{
			game_world->geometry_locked = 0;
			return;	
		}
	}


	//Might need more!
	u32 tile_count = game_world->w * game_world->h;

    if(tile_count)
	{

//        game_world->geometry_locked = 0;
	    if(!game_world->geometry_locked)
	    {
	    	render_refresh_locked_vertices(render_commands);
	    }

	    render_push_locked_vertices(render_commands);
		world_tile *gridTiles = game_world->tiles;

		if(!game_world->geometry_locked) //or if the renderer needs a lock update
		{
		    game_world->geometry_locked = 1;

	        //u32 tile_count = 10;
			for(u32 y = 0;
					y < game_world->h;
					y++)
			{
				for(u32 x = 0;
						x < game_world->w;
						x++)
				{

					game_draw_terrain(
							game_world,
							game_asset_manager,
							render_commands,
							x,
							y,
							0,
							0,
							V4(255, 255, 255, 255));
				}
			}

		}
		render_pop_locked_vertices(render_commands);
	}

}
