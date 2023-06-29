
#define NOT_GAME_BUILD 0

#define Assert(Condition) \
    if(!(Condition)) {*(int*)0 = 0;}
#define NotImplemented Assert(0)
#define ARRAYCOUNT(a) (sizeof(a) / sizeof(a[0]))

//program structs
struct s_game_state;

static void
asset_map_to_world_map(struct asset_map_data *loading_map, struct s_game_world *map);

typedef enum{
	state_idle,
	state_attacking0 = 1,
	state_attacking1 = 4,
}player_states_e;

#include "purpose_crt.h"

#include "global_definitions.h"
#include "agu_random.h"
#include "purpose_math.h"
#include "global_all_use.h"
#include "purpose_memory.h"
#include "purpose_stream.h"
#include "purpose_platform.h"
#include "purpose_global.h"
#include "purpose_render.h"
#include "purpose_render.c"
#include "purpose_console.h"
#include "purpose_ui.h"
#include "purpose_ui.c"
#include "agu_timer.h"

#include "purpose_platform_layer.h"

//#include "purpose_files.c"
#include "purpose_all.h"

#include "brains.h"
#include "purpose_assets.h"
#include "purpose.h"
#include "entity.h"
#include "purpose_assets.c"
#include "purpose_geometry.c"

#include "model_render.h"
#include "model_render.c"
#include "purpose_world.h"
#include "purpose_ray.c"
#include "purpose_console_commands.c"
#include "brains.c"
#include "purpose_coso.h"
#include "purpose_physics.c"
//#include "Assets/generated_header.h"
#define	WORLD_ENTITES_MAX 100
#define WORLD_TILESETS_MAX 10
global_variable stream_data *g_info_stream;
#define Gprintf(params, ...) stream_pushf(g_info_stream, params, __VA_ARGS__)

#include "purpose_detections.c"
#include "entity.c"
#include "purpose_prototype.c"
#include "events.c"
/*
   -Events
     -Display event conditions.
	 -Add else if condition is not true.
	 -Spawn conditions ?
   -Map swapping
   -More entity behaviour
     -Add tag/group to them
   -Attacks
   -weapon/style switching


   -Agregado simples hitpoints
   -Planeo "spawnear" varias entidades.
*/

static game_world *
build_test_map(program_state *program, u32 w, u32 h);
static world_tileset *
build_test_tileset(program_state *program);
static model *
build_test_player_model(program_state *program);
static void
transition_map(program_state *program, render_commands *commands);
static void
apply_damage_to_targets(program_state *program, world_entity *ent);

static void
apply_damage_to_targets(program_state *program, world_entity *ent)
{
	for(world_entity *ent0 = program->first_entity; ent0; ent0 = ent0->next)
	{
		if(!(ent0->flags & coso_damage))
		{
			continue;
		}

		for(u32 d = 0; d < ent0->detections_count; d++)
		{
			cosos_detection detection = ent0->detections[d];

			world_entity *ent1 = detection.id1;
			//ignore parent
			if(ent1 == ent0->parent) continue;

			if(detection.overlap)
			{

				//look if this target got hit
				b32 cancel_hit = 0;
				u32 contact_index = 0;
				for(u32 d = 0; d < ent0->damage_contact_count; d++)
				{
					damage_contact contact_data = ent0->damage_contact[d];
					if(contact_data.id1.id0 == ent1->id.id0)
					{
						cancel_hit = 1;
						contact_index = d;
						break;
					}
				}
				if(!cancel_hit)
				{
					ent1->body->v.x += 1;
					ent1->damage_contact_count++;
					//NOTA: underflow
					ent1->hitpoints--;
					//add this to the list of contacts
					damage_contact cd = {0};
					//puede que esta entidad desaparezca y esta id sea usada por otra.
					cd.id1 = ent1->id;
					cd.frame_timer = ent0->dmg.frame_timer;
					cd.frames_before_hit = ent0->dmg.frames_before_hit;
					ent0->damage_contact[ent0->damage_contact_count] = cd;
					ent0->damage_contact_count++;
					Assert(ent0->damage_contact_count < 10);
				}
			}
		}
		//countdown
		u32 index = 0;
	    while(index < ent0->damage_contact_count)
		{
			damage_contact *cd = ent0->damage_contact + index;
			if(cd->frame_timer)
			{
				if(cd->frames_before_hit == 0)
				{
					//get last one
					ent0->damage_contact[index] = ent0->damage_contact[ent0->damage_contact_count - 1];
					ent0->damage_contact_count--;
				}
				else
				{
					index++;
				}
				cd->frames_before_hit--;
			}
			else
			{
				index++;
			}
		}
	}
}

static void
transition_map(program_state *program, render_commands *commands)
{
	if(program->fade_speed < 6.0f) program->fade_speed = 6.0f;

	b32 cancel_transition = program->switch_map && program->current_map_index == program->next_map_index; 
	if(cancel_transition)
	{
		program->switch_map = 0;
	}
	f32 fade_speed = program->fade_speed;

	if(program->switch_map)
	{
		if(!program->faded)
		{
			program->fade_alpha += fade_speed;
			if(program->fade_alpha > 255.0f)
			{
				program->faded = 1;
				program->fade_alpha = 255.0f;
			}
		}
		if(program->fade_alpha)
		{
			vec4 fade_color = {0, 0, 0, program->fade_alpha};
			render_rectangle_2d(commands,
					0,
					0,
					commands->gameRenderer->back_buffer_width,
					commands->gameRenderer->back_buffer_height,
					fade_color);
		}
	}
	else
	{
		program->fade_alpha = 0;
	}


	if(program->faded)
	{
		if(program->switch_map && program->current_map_index != program->next_map_index)
		{
			program->current_map_index = program->next_map_index;
			Assert(program->next_map_index < program->map_count);

			world_entity *player = program->player_entity;
			Assert(player);

			player->body->p = program->map_swap_p;

		}
		program->switch_map = 0;
		program->faded = 0;
	}
}

static model * 
build_test_player_model(program_state *program)
{
	game_assets *assets = program->game_asset_manager;
	model *test_model = memory_area_clear_and_push_struct(program->area, model);
	test_model->bone_count = 1;
	test_model->uvs_count = 1;
	test_model->sprite_count = 1;
	test_model->bones = memory_area_clear_and_push_array(program->area, model_bone, 1);
	test_model->sprites = memory_area_clear_and_push_array(program->area, model_sprite, 1);
	test_model->orientation_amount = 1;
//	test_model->view_direction = V2(0, -1);
	//sprite shetts
	test_model->sprite_sheet_count = 1;
	test_model->sprite_sheets_a = memory_area_clear_and_push_array(program->area, render_texture *, 1);
	{
		test_model->sprite_sheets_a[0] = assets_load_and_get_image(assets, "data/images/model_human.png"); 
		Assert(test_model->sprite_sheets_a[0] != 0);
	}
	test_model->uvs = memory_area_push_array(program->area, sprite_orientation, 1);
	{
		model_bone *bone = test_model->bones + 0;
		bone->parent = 0; 
//		bone->p      = 
	//	bone->displacement = file_model_bone->displacement;
		bone->q = quaternion_identity(); 
		bone->two_dim = 0; 
		bone->sprite_count = 1; 
		bone->frame_key_count = 0;
		bone->sprites_at = 0; 
		//bones point to a list of sprites.
		{
			model_sprite *model_sprite = test_model->sprites + 0;

			model_sprite->type          = model_sprite_billboard;
			model_sprite->type          = model_sprite_mesh;
			model_sprite->p             = V3(0, 0, 0); 
			model_sprite->depth_x       = 0;
			model_sprite->depth_y       = 0;
			model_sprite->depth_z       = 0;
			model_sprite->pivotX        = 0;
			model_sprite->pivotY        = 0;
			model_sprite->pivotZ        = 0;
			model_sprite->texture_index = 0;//file_model_sprite->texture_index;
			model_sprite->extra_frame_count = 0;
			model_sprite->frame_list_index = 0;
			model_sprite->frame_at = 0;

			model_sprite->face_axis = billboard_face_x;
			model_sprite->size.x = 16;
			model_sprite->size.z = 20;

			//uvs
			{
			    sprite_orientation *uvs = test_model->uvs + 0;
				render_uvs fuvs = render_frames_to_uv(512, 512, 0, 0, 16, 20);
				uvs->uv0 = fuvs.uv0;
				uvs->uv1 = fuvs.uv1;
				uvs->uv2 = fuvs.uv2;
				uvs->uv3 = fuvs.uv3;

//				adjust_vertices_to_uvs(
//						test_model->sprite_sheets_a[0],
//						&model_sprite->v0,
//						&model_sprite->v1,
//						&model_sprite->v2,
//						&model_sprite->v3,
//						uvs->uv0,
//						uvs->uv1,
//						uvs->uv2,
//						uvs->uv3);
			}
			
			//model_sprite->size = file_model_sprite->v0;
			//model_sprite->size2 = file_model_sprite->v1;
			////model_sprite->v0 = file_model_sprite->v0;
			////model_sprite->v1 = file_model_sprite->v1;
			//model_sprite->v2 = file_model_sprite->v2;
			//model_sprite->v3 = file_model_sprite->v3;
			//model_sprite->frame_at = loaded_uvs;
		}
	}
	return(test_model);
}

static world_tileset *
build_test_tileset(program_state *program)
{
	game_assets *assets = program->game_asset_manager;
	world_tileset *result = memory_area_push_struct(program->area, world_tileset);
	memory_clear(result, sizeof(*result));
	result->image = assets_load_and_get_image(assets, "data/images/selene12_b.png");
	Assert(result->image);
	//setup terrain
	result->terrain_count = 1;
	result->terrain = memory_area_push_array(program->area, s_tileset_terrain, result->terrain_count);
	{
		s_tileset_terrain *terrain = result->terrain + 0;
		terrain->shape = 0;
		terrain->capacity = 1;
		terrain->use_wall = 1;
		terrain->wall_index = 0;

		//render_uvs uvs = render_frames_to_uv(512, 512, 1, 127, 12, 12);
		render_uvs uvs = render_frames_to_uv(512, 512, 1, 113, 12, 12);
		terrain->uv0 = uvs.uv0;
		terrain->uv1 = uvs.uv1;
		terrain->uv2 = uvs.uv2;
		terrain->uv3 = uvs.uv3;

	}
	//wall
	result->wall_count = 1;
	result->walls = memory_area_push_array(program->area, tileset_wall, result->wall_count);
	{
		tileset_wall *wall = result->walls + 0;
		wall->uvs_at = 0;
		wall->uvs_count = 1;
		wall->repeat = 0;
		wall->extra_frames_x = 0;
		wall->extra_frames_y = 0;

//		render_uvs uvs = render_frames_to_uv(512, 512, 29, 127, 12, 12);
		render_uvs uvs = render_frames_to_uv(512, 512, 29, 113, 12, 12);
		wall->uv0 = uvs.uv0;
		wall->uv1 = uvs.uv1;
		wall->uv2 = uvs.uv2;
		wall->uv3 = uvs.uv3;
	}
	return(result);
}

static game_world * 
build_test_map(program_state *program, u32 w, u32 h)
{
	//for now this is the current map, later this should be
	//on a cached array.
	game_world *current_map = program->maps + program->map_count;
	program->map_count++;
	memory_area *area = program->area;

	current_map->tiles = memory_area_clear_and_push_array(area, world_tile, 100000); 
	current_map->entities = memory_area_push_array(area, world_entity, WORLD_ENTITES_MAX);
	//world->tilesets   = memory_area_push_array(main_memory, u32, 10);
	current_map->tilesets_a = memory_area_push_array(area, world_tileset *, WORLD_TILESETS_MAX);
	current_map->colliders = memory_area_push_array(area, world_collider, 50000);
	current_map->marks = memory_area_push_array(area, map_mark, 100);
	current_map->events.events = memory_area_push_array(area, game_event, 100);
	current_map->events.conditions = memory_area_push_array(area, event_condition, 100);
	current_map->events.lines = memory_area_push_array(area, event_line, 100);

	//add test tileset
	current_map->tileset_count = 1;
	current_map->tilesets_a[0] = build_test_tileset(program);

//	u32 w = 128;
//	u32 h = 128;
	u32 tile_count = w * h;
	current_map->w = w;
	current_map->h = h;

	for(u32 t = 0; t < tile_count; t++)
	{
		world_tile *tile = current_map->tiles;
		tile->tileset_index = 0;
		tile->tileset_terrain_index = 0;
		tile->height = 0;
		current_map->tile_count++;
	}
	return(current_map);
}


static vec3
game_mouse_in_world(program_state *program, game_renderer *game_renderer, program_input *input)
{
	vec3 mouse_world = render_mouse_coordinates_to_world(game_renderer, input->mouse_clip, 1.0f);
	vec3 ray_origin = game_renderer->camera_position;
	vec3 ray_dir = vec3_normalize_safe(vec3_sub(mouse_world, ray_origin));
	ray_casted_info mouse_p = cast_ray_at_plane(
			ray_origin, ray_dir, V3(0, 0, 0), V3(0, 0, 1));
	return(mouse_p.ray_on_plane);
}



static void
asset_map_to_world_map(struct asset_map_data *loading_map, struct s_game_world *map)
{
	u32 loaded_entity_count = loading_map->entity_count;
	u32 tileset_count       = loading_map->tileset_count;
	u32 tile_count          = loading_map->tile_count;

	//tilesets
	for(u32 t = 0;
			t < tileset_count;
			t++)
	{
		world_tileset *loaded_tileset = loading_map->tilesets_a[t];
		if(loaded_tileset)
		{
			map->tilesets_a[t] = loaded_tileset;
		}
		else
		{
			Assert(0);
		}
	}

	//map files
	for(u32 t = 0;
			t < tile_count;
			t++)
	{
		map->tiles[t] = loading_map->tiles[t];
	}

	//"colliders" from tiles
	for(u32 t = 0;
			t < tile_count;
			t++)
	{
		map->colliders[t] = loading_map->colliders[t];
		world_collider *col = map->colliders+t;
		col->width = 8;
		col->height = 8;
		col->z = (f32)(loading_map->tiles[t].height * 1);
	}
	//entities depending on types
	for(u32 e = 0; e < loading_map->entity_count; e++)
	{
		map_entity_data *map_entity = loading_map->entities + e;
		switch(map_entity->type)
		{
			case map_entity_mark:
				{
					map_mark *mark = map->marks + map->mark_count++;
					mark->position = map_entity->position;
				}break;
		}
	}


	//fill data
	map->colliders_count = tile_count;
	map->tile_count    = tile_count;
	map->tileset_count = tileset_count;
	map->w = loading_map->map_w;
	map->h = loading_map->map_h;

//	//add the count for the player
//	map->entity_count++;

}

static void
asset_entity_to_game_entity(asset_entity *asset_entity, world_entity *entity)
{
	//stats
	entity->body->speed = asset_entity->speed;
	entity->body->z_speed = asset_entity->z_speed;
	entity->body->speed_max= asset_entity->speed_max;
	//states
	//state actions
}

#define PLAYERTEXTUREID 'WC00'


typedef enum{
	player_idle,
	player_walking,
	player_attack,
}player_animation_index;

static vec3
AddOffsetFromVector(vec3 p0, vec3 p1, f32 bias)
{
	vec3 distanceP0P1 = vec3_sub(p1, p0);
	distanceP0P1      = vec3_normalize_safe(( distanceP0P1));
	distanceP0P1	  = vec3_f32_mul(distanceP0P1 ,bias);
	vec3 result       = vec3_add(p0, distanceP0P1);
    return(result);
}





//
// Reserved space__
//

static vec3 
GetCollisionCorrectionFromSides(world_entity *player_entity,
		                        vec3 currentCorrection,
								f32 distance_from_y_max,
								f32 distance_from_y_min,
								f32 distance_from_x_min,
								f32 distance_from_x_max,
								u32 inside_x,
								u32 inside_y)
{
#if 0

	vec3 velocity = player_entity->velocity;
	vec3 collision_size_h = vec3_f32_mul(player_entity->collision_size, 0.5f);
	vec3 correction_deltas = currentCorrection;

	int32 at_x_positive = velocity.x > 0;
	int32 at_y_positive = velocity.y > 0;
	int s = 0;
	//Get the formula from past!
	int16 moving_y = velocity.y != 0;
	int16 moving_x = velocity.x != 0;
	if(inside_y && moving_y)
	{
		real32 correctionY = 0;
		int32 direction = 0;
		if(at_y_positive)
		{
			//push to -y
			correctionY = -distance_from_y_min;
			direction = -1;
		}
		else
		{
			correctionY = distance_from_y_max;
			direction = 1;
		}
		correctionY += collision_size_h.y;
		if(correctionY > correction_deltas.y * direction)
		{
			correction_deltas.y = correctionY * direction; 
		}
	}
	else if(inside_x && moving_x)
	{

		real32 correctionX = 0;
		int32 direction = 0;
		//Distance from left is positive
		if(at_x_positive)
		{
			//push to -x
			correctionX = -distance_from_x_min;
			direction = -1;
		}
		else
		{
			correctionX = distance_from_x_max;
			direction = 1;
		}
		correctionX += collision_size_h.x;
		if(correctionX > correction_deltas.x * direction)
		{
			correction_deltas.x = correctionX * direction;
		}
	}
	return(correction_deltas);
#endif
}

static inline game_contact_points 
game_cubes_collision_data(
		cubes_overlap_result overlap_results,
		vec3 p0,
		vec3 v0,
		vec3 sz0,
		vec3 p1,
		vec3 sz1,
		game_contact_points current_data)
{

	vec3 size0_h = vec3_f32_mul(sz0, 0.5f);
	vec3 size1_h = vec3_f32_mul(sz1, 0.5f);

	vec3 current_correction_values = current_data.correction;
	vec3 reflection = {0};
	vec3 normal0 = vec3_normalize_safe(v0);
	vec3 normal1 = {0};
	b8 updated_correction = 0;

	//inside y axis
	u32 inside_x = overlap_results.side_y == 0;
	//inside x axis
	u32 inside_y = overlap_results.side_x == 0;
	i32 at_x_positive = v0.x > 0;
	i32 at_y_positive = v0.y > 0;
	int s = 0;
	//Get the formula from past!
	b16 moving_y = v0.y != 0;
	b16 moving_x = v0.x != 0;
	if(inside_y && moving_y)
	{
		//distance from mid points
		f32 distance_from_y = p1.y - p0.y;
		f32 distance_from_y_min = (distance_from_y - size1_h.y);
		f32 distance_from_y_max = (distance_from_y + size1_h.y);
		real32 correctionY = 0;
		int32 direction = 0;
		if(at_y_positive)
		{
			normal1.y = -1;
			//push to -y
			correctionY = -distance_from_y_min;
			direction = -1;
		}
		else
		{
			normal1.y = -1;
			correctionY = distance_from_y_max;
			direction = 1;
		}
		correctionY += size0_h.y;
		//pick this correction if the value is bigger than the current
		if(correctionY > (current_correction_values.y * direction))
		{
			updated_correction = 1;
			current_correction_values.y = correctionY * direction; 
		}
	}
	else if(inside_x && moving_x)
	{

		f32 distance_from_x = p1.x - p0.x;
		real32 correctionX = 0;
		int32 direction = 0;
		//Distance from left is positive
		if(at_x_positive)
		{
			f32 distance_from_x_min = (distance_from_x - size1_h.x);
			//push to -x
			normal1.x = -1;
			correctionX = -distance_from_x_min;
			direction = -1;
		}
		else
		{
			f32 distance_from_x_max = (distance_from_x + size1_h.x);
			normal1.x = 1;
			correctionX = distance_from_x_max;
			direction = 1;
		}
		correctionX += size0_h.x;
		if(correctionX > current_correction_values.x * direction)
		{
			updated_correction = 1;
			current_correction_values.x = correctionX * direction;
		}
	}
	//inside, but not moving
	if(inside_x && inside_y)
	{
		int s = 0;
	}

	if(updated_correction)
	{
		current_data.correction = current_correction_values;
		current_data.penetration = vec3_length(current_correction_values);
		reflection = vec3_reflect(normal0, normal1);
		current_data.normal = reflection;
		current_data.got_contact = 1;
//		stream_pushf(g_info_stream, "normal0 {%f, %f}, 1 {%f, %f}, reflection {%f, %f, %f}",
//				normal0.x,
//				normal0.y,
//				normal1.x,
//				normal1.y,
//				reflection.x,
//				reflection.y,
//				reflection.z
//				);
	}
	return(current_data);
}

inline vec3
get_plane_size(
		vec3 x_axis,
		vec3 y_axis,
		f32 w,
		f32 h)
{
	vec3 plane_size = {
			(x_axis.x * w) + (y_axis.x * h),
			(x_axis.y * w) + (y_axis.y * h),
			(x_axis.z * w) + (y_axis.z * h)
	};

	return(plane_size);
}




inline void
game_world_render_entities(render_commands *commands,
		                   memory_area *bones_area,
		                   game_assets *assets,
						   game_world *current_map,
						   f32 dt)
{
	//update and render entities
	for(u32 e = 0;
			e < current_map->entity_count;
			e++)
	{

		world_entity *entity         = current_map->entities + e;
		//get keys for model and animation
		// ;cleanup
		model *entity_model = entity->model.model;

		model_animations *entity_model_animation = &entity->animation;


		if(entity_model)
		{
			u32 bone_count   = entity_model->bone_count;
			u32 sprite_count = entity_model->sprite_count;


//			model_render(commands->game_renderer,
//					            commands,
//								animated_meshes,
//								entity->position,
//								V2(0, -1));
			model_animate_and_render(
					commands,
					&entity->model,
					entity->body->p,
					entity->looking_direction,
					dt);
		}

	}

}

inline void
debug_update_camera(
		game_renderer *game_renderer,
		program_input *game_input_state,
		program_state *main_game)
{

	render_debug_displace_camera(
			&main_game->debug.camera,
			game_input_state,
			game_renderer->camera_x,
			game_renderer->camera_y,
			game_renderer->camera_z);

}

inline void
debug_display_map_colliders_at(
		render_commands *debug_commands,
		game_world *current_map,
		vec3 at,
		u32 range)
{
	i32 x0 = (i32)(at.x / GAME_TILESIZE) - range;
	i32 x1 = (i32)(at.x / GAME_TILESIZE) + range;
	i32 y0 = (i32)(at.y / GAME_TILESIZE) - range;
	i32 y1 = (i32)(at.y / GAME_TILESIZE) + range;
	i32 w_end = current_map->w * GAME_TILESIZE;
	i32 h_end = current_map->h * GAME_TILESIZE;

	x0 = x0 < 0 ? 0 : x0;
	x1 = x1 >= (i32)w_end ? w_end : x1;
	y0 = y0 < 0 ? 0 : y0;
	y1 = y1 >= (i32)h_end ? h_end : y1;
#if 1
	for(i32 y = y0; y < y1; y++)
	{
		for(i32 x = x0; x < x1; x++)
		{
			u32 c = x + (y * current_map->w);
			world_collider w_collider = current_map->colliders[c]; 
			//get collider positions
			u32 c_x = (x % current_map->w) * GAME_TILESIZE;
			u32 c_y = y * GAME_TILESIZE;
			//for now I assume that z will always be at 0
			vec3 collider_p = {(f32)c_x, (f32)c_y, 0};

			//Assumming it's a plane
			real32 w_collider_w = w_collider.size.x;
			real32 w_collider_h = w_collider.size.y;

			//Only detects if the position is inside


			//Inner product with only the Z since it always looks down.
			//real32 normalFromBottom = vec3_inner(collisionNormal, w_collider_normal);
			vec3 w_collider_size;
			if(w_collider.shape == shape_plane)
			{

				w_collider_size = vec3_scale(w_collider.size, 1); 
				w_collider_size.z *= GAME_TILESIZE;
				w_collider_size.z += 4; 
				collider_p.z += w_collider_size.z / 2;
				collider_p.x += w_collider_size.x / 2;
				collider_p.y += w_collider_size.y / 2;

				render_draw_cube(
						debug_commands,
						collider_p,
						w_collider_size,
						V4(0, 0, 0, 128));

			}
		}
	}
#endif
}

inline void
game_update_camera(program_state *program,
		game_renderer *game_renderer)
{


	vec3 camera_target = program->render_parameters.camera_target;



//	program->distance_camera_target = 160.0f;
	f32 distance_camera_target = program->render_parameters.distance_camera_target;
	//limit the camera target

	vec3 final_camera_distance = {0, 0, distance_camera_target};


	quaternion camera_rotation_m = quaternion_from_rotations_scale(
			program->render_parameters.camera_rotation_x,
			program->render_parameters.camera_rotation_y,
			program->render_parameters.camera_rotation_z);

	final_camera_distance = quaternion_v3_mul_foward_inverse(
			                       camera_rotation_m,
			                       final_camera_distance);

	//vec3 prev = program->camera_position;
	//make the camera follow the player
	program->render_parameters.camera_position = vec3_add(final_camera_distance,
			V3(camera_target.x, camera_target.y, 0));
//	vec3 delta = vec3_sub(program->camera_position, prev);

#if 1
	if(program->debug.use_camera_bounds)
	{
		vec3 vx = game_renderer->camera_x; 
		vec3 vy = game_renderer->camera_y; 
		vec3 vz = game_renderer->camera_z; 

		matrix4x4_data cameraMatrices = 
			matrix4x4_camera_transform(
					vx,
					vy,
					vz,
					program->render_parameters.camera_position);
		matrix4x4 projection = matrix4x4_mul(
				game_renderer->projections.foward,
				cameraMatrices.foward);
		vec4 plane_b = vec4_PlaneBottom(projection); 
		vec4 plane_t = vec4_PlaneTop(projection); 
		//extraer los planos

		f32 cam_top_limit = 12 * 32 * -plane_t.y;
		f32 cam_bot_limit = 12;

		if(plane_b.w >= cam_bot_limit)
		{
			program->render_parameters.camera_position.y += plane_b.w / plane_b.y;
		}

		//480 es un limite hardcodeado final del mapa
		//dif es la diferencia entre plane_t.w y el limite
		//plane_t.y dado a la inclinación de la cámara, es un número negativo
		else if(plane_t.w >= cam_top_limit)
		{
			f32 dif = plane_t.w - cam_top_limit;
			program->render_parameters.camera_position.y += dif / plane_t.y;
		}
	}
#else
	if(program->debug.use_camera_bounds)
	{
		matrix4x4 projection = matrix4x4_mul(
				game_renderer->projections.foward,
				cameraMatrices.foward);
		vec4 plane_b = vec4_PlaneBottom(projection); 
		vec4 plane_t = vec4_PlaneTop(projection); 
		//extraer los planos

		f32 cam_top_limit = 12 * 32 * -plane_t.y;
		f32 cam_bot_limit = 12;

		if(plane_b.w >= cam_bot_limit)
		{
			program->render_parameters.camera_position.y += plane_b.w / plane_b.y;
		}

		//480 es un limite hardcodeado final del mapa
		//dif es la diferencia entre plane_t.w y el limite
		//plane_t.y dado a la inclinación de la cámara, es un número negativo
		else if(plane_t.w >= cam_top_limit)
		{
			f32 dif = plane_t.w - cam_top_limit;
			program->render_parameters.camera_position.y += dif / plane_t.y;
		}
	}
#endif

//	program->render_parameters.camera_normal = vec3_normalize(vec3_sub(camera_target, program->render_parameters.camera_position));
//	program->camera_position.x = f32_round_to_int(program->camera_position.x);
//	program->camera_position.y = f32_round_to_int(program->camera_position.y);
//	program->camera_position.z = f32_round_to_int(program->camera_position.z);



}

/*
camera: Para la camara se tiene lo siguiente:
Posicion, rotacion en escapa (se multiplica por PI), zoom, distance_camera_target y z_fix
z_fix lo que hace es multiplicar a zoom y distance_camera_target por este número y
se utiliza para no ver la parte de atras de los objectos cuando la cámara apuntando
desde arriba pasa por ellos.
Para calcular los límites de la cámara, se debe tomar la posición rotada del mismo
y aplicarle la otra rotación del quaternion al mismo (si se le hizo mul_foward_inverse
a la poción, entonces a este se le aplica mul_inverse_foward) y de ahí, sumarle
o restarle el límite que se busque.
*/

static void
update_render_debug_ui(program_state *program,
		              game_renderer *game_renderer,
					  program_input *game_input_data,
					  f32 dt)
{
	if(game_input_data->f_keys[10])
	{
		program->debug_display_console = !program->debug_display_console;
	}
	if(game_input_data->f_keys[9])
	{
		program->debug_display_ui = !program->debug_display_ui;
	}

   game_ui *ui = ui_begin_frame(
		   program->ui,
		   program->default_font,
		   program->platform,
		   game_renderer,
		   dt);

   if(program->debug_display_console)
   {
	   ui_set_color_a(ui, ui_color_background, 0.5f)
       if(ui_panel_begin(
        	   ui,
        	   ui_panel_flags_front_panel,
        	   0,
        	   0,
        	   1024,
        	   512,
        	   "debug panel"))
       {
		   ui_set_wh_ppct(ui, 1.0f, 0.0f)
           ui_console(
        		   ui,
        		   &program->debug_console,
				   "game_debug_console");
       }
       ui_panel_end(ui);
   }
   if(program->debug_display_ui)
   {
       if(ui_window_begin(
        	   ui,
        	   ui_panel_flags_front_panel,
        	   0,
        	   0,
        	   1024,
        	   512,
        	   "Game variables"))
       {
		   ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		   {
			   ui_textf(ui, "target_framerate");
			   ui_spinner_u16(ui, 5, 5, 60, &program->target_framerate, 0, "target_framerate_p");
			   ui_textf(ui, "elapsed_ms: %f", program->elapsed_ms);
			   ui_textf(ui, "mc_per_frame: %f", program->mc_per_frame);
			   ui_textf(ui, "cycles_elapsed : %u", program->cycles_elapsed);
			   ui_textf(ui, "camera_position {%f, %f, %f}",
					   program->render_parameters.camera_position.x,
					   program->render_parameters.camera_position.y,
					   program->render_parameters.camera_position.z);

			   f32 limit_x = 80;
			   f32 limit_z = program->render_parameters.camera_position.z;
			   f32 game_w = game_renderer->back_buffer_width;
			   f32 game_h = game_renderer->back_buffer_height;
			   vec4 plane_l = vec4_PlaneLeft(game_renderer->projection); 
			   vec4 plane_b = vec4_PlaneBottom(game_renderer->projection); 
			   vec4 plane_t = vec4_PlaneTop(game_renderer->projection); 
			   vec4 plane_r = vec4_PlaneRight(game_renderer->projection); 
			   vec3 v_limit_x = {80, 0, 0};

			   //plane_l.x *= plane_l.w;
			   //plane_l.y *= plane_l.w;
			   //plane_l.z *= plane_l.w;

			   quaternion qcam = quaternion_from_rotations_scale(
				   game_renderer->camera_rotation.x,
				   game_renderer->camera_rotation.y,
				   game_renderer->camera_rotation.z
				   );
	//		   vec4 plane_n = vec4_PlaneNear(game_renderer->projection);
			   vec3 tvec2 = {0, 0, 80};
			   tvec2 = quaternion_v3_mul_foward_inverse(qcam, tvec2);
			   vec3 tvec = {0, 0, 0};
			   f32 result_x = ((tvec.x * plane_l.x) + (tvec.y * plane_l.y) +(tvec.z * plane_l.z) + (plane_l.w)); 
			   f32 result_y = ((tvec.x * plane_b.x) + (tvec.y * plane_b.y) +(tvec.z * plane_b.z) + (plane_b.w)); 

			   f32 cam_x = program->render_parameters.camera_position.x;// + program->camera_position.z;
			   f32 cam_y = program->render_parameters.camera_position.y;// + program->camera_position.z;
			   b32 outside_x0 = result_x > 0.0f; 
			   b32 outside_x1 = 0;
			   b32 outside_y0 = cam_y < tvec.z ; 
			   b32 outside_y1 = 0;
			   ui_textf(ui, "outside_x0_x1 {%u, %u} outside_y0_y1 {%u, %u} tvec2 {%f, %f, %f}",
					  outside_x0, outside_x1, outside_y0, outside_y1, tvec2.x, tvec2.y, tvec2.z);
			   ui_textf(ui, "plane_t {%f, %f, %f, %f}",
					   plane_t.x,
					   plane_t.y,
					   plane_t.z,
					   plane_t.w
					   );
			   ui_textf(ui, "plane_b {%f, %f, %f, %f}",
					   plane_b.x,
					   plane_b.y,
					   plane_b.z,
					   plane_b.w
					   );
			   ui_textf(ui, "plane_l {%f, %f, %f, %f}",
					   plane_l.x,
					   plane_l.y,
					   plane_l.z,
					   plane_l.w);
			   ui_textf(ui, "plane_r {%f, %f, %f, %f}",
					   plane_r.x,
					   plane_r.y,
					   plane_r.z,
					   plane_r.w);

#if 1
			   ui_set_row(ui)
			   {
				   ui_text(ui, "distance_camera_target");
				   ui_spinner_f32(
						   ui, 1.00f, F32MIN, F32MAX, &program->render_parameters.distance_camera_target, 0, "distance_camera_target");
			   }

			   ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
			   {
				   ui_checkbox(ui, &program->debug.free_camera, "debug free_camera");
				   ui_checkbox(ui, &program->debug.use_camera_bounds, "camera_bounds");
				   ui_checkbox(ui, &program->debug.debug_display_colliders, "debug_display_colliders");
				   ui_checkbox(ui, &program->debug.debug_display_entity_colliders, "debug_display_entity_colliders");
			   }

			   ui_set_row(ui)
			   {
				   ui_text(ui, "camera_rotation.x");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   ui_spinner_f32(
						   ui, 0.01f, F32MIN, F32MAX, &program->render_parameters.camera_rotation_x, 0, "camera_rotation.x");
			   }

			   ui_set_row(ui)
			   {
				   ui_text(ui, "camera_rotation.y");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   ui_spinner_f32(
						   ui, 0.01f, F32MIN, F32MAX, &program->render_parameters.camera_rotation_y, 0, "camera_rotation.y");
			   }

			   ui_set_row(ui)
			   {
				   ui_text(ui, "camera_rotation.z");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   ui_spinner_f32(
						   ui, 0.01f, F32MIN, F32MAX, &program->render_parameters.camera_rotation_z, 0, "camera_rotation.z");
			   }

			   ui_set_row(ui)
			   {
				   ui_text(ui, "camera_zoom");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   ui_spinner_f32(
						   ui, 0.01f,
						   F32MIN,
						   F32MAX,
						   &program->render_parameters.camera_zoom, 0, "camera_zoom");
			   }


			   ui_set_row(ui)
			   {
				   ui_text(ui, "sprite_skew");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   if(ui_spinner_f32(
						   ui, 0.1f,
						   0,
						   1.0f,
						   &game_renderer->sprite_skew, 0, "sprite_skew"))
				   {
					   program->maps[program->current_map_index].geometry_locked = 0;
				   }
			   }
			   ui_set_row(ui)
			   {
				   ui_text(ui, "fov");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
				   ui_spinner_f32(
						   ui, 1,
						   0,
						   360.0f,
						   &game_renderer->fov, 0, "game_fov");
			   }

			   ui_set_row(ui)
			   {
				   ui_text(ui, "z_fix");
				   ui_space(ui, ui_size_specified(2.0f, 0.0f));
				   ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
					   ui_spinner_f32(
							   ui, 0.01f, F32MIN, F32MAX, &program->render_parameters.z_fix, 0, "z_fix");
			   }

			   ui_textf(ui, "camera_x: {%f, %f, %f}", game_renderer->camera_x.x, game_renderer->camera_x.y, game_renderer->camera_x.z);
			   ui_textf(ui, "camera_y: {%f, %f, %f}", game_renderer->camera_y.x, game_renderer->camera_y.y, game_renderer->camera_y.z);
			   ui_textf(ui, "camera_z: {%f, %f, %f}", game_renderer->camera_z.x, game_renderer->camera_z.y, game_renderer->camera_z.z);

			   world_entity *player = program->player_entity;
			   ui_textf(ui, "player looking_direction {%f, %f}",
					   player->looking_direction.x,
					   player->looking_direction.y);

			   ui_textf(ui, "player position {x:%f, y:%f, z:%f}",
					   player->body->p.x, player->body->p.y, player->body->p.z);
			   f32 looking_angle = arctan232(player->looking_direction.x,
					   player->looking_direction.y);
			   f32 looking_angle_whole = (PI - looking_angle) / PI;
			   ui_textf(ui, "player looking_direction angle {%f} or {%f}",
					   looking_angle, looking_angle_whole);
			   ui_textf(ui, "player velocity {%f, %f, %f}",
					   player->body->v.x, player->body->v.y, player->body->v.z);
#endif
		   }
		   ui_space_specified(ui, 4.0f, 1.0f);
		   if(program->test_focused_brain)
		   {
			   state_main *brain = program->test_focused_brain;

			   state_node *current_state = brain->states + brain->current;
			   ui_textf(ui, "focused_state frame step: %u", current_state->frame_step);

		   }
       }
       ui_panel_end(ui);
   }

   ui_end_frame(
		   ui,
		   program->platform->window_is_focused);

   u32 ui_is_interacting = ui_any_interaction(ui);
   u32 ui_focused = ui_is_interacting;
   program->debug.interacting_with_ui = ui_focused || ui_is_interacting;
}

inline cubes_overlap_result 
cubes_overlap_xy(
		vec3 p0,
		vec3 sz0,
		vec3 p1,
		vec3 sz1)
{ 
	//Detect if on side
	f32 distance_from_x = p0.x - p1.x;
	f32 distance_from_y = p0.y - p1.y;

	f32 inside_formula_x = (distance_from_x) / ((sz1.x + sz0.x) * 0.5f);
	f32 inside_formula_y = (distance_from_y) / ((sz1.y + sz0.y) * 0.5f);

	cubes_overlap_result overlap_result = {0};
	overlap_result.side_x = ((i32)inside_formula_x);
	overlap_result.side_y = ((i32)inside_formula_y);
	return(overlap_result);

}


#define get_entity_collision_position(entity) V3(entity->position.x + collision_offset.x,\
	    					      entity->position.y + collision_offset.y,\
	    					      entity->position.z + collision_offset.z);


static b32 
game_switch_map(
		program_state *program,
		u8 *map_path_and_name)
{
	b32 result = 1;
	return(result);
}

static void
game_update_render(program_state *program,
		           game_renderer *game_renderer,
				   program_input *program_input,
				   f32 dt)
{
	//in case of pausing when debugging.
	if(dt > (0.0625f * 2))
	{
		dt = 0;
	}
	game_assets *assets = program->game_asset_manager;
	stream_data *info_stream = &program->info_stream;
	s_game_console *game_console = &program->debug_console;

	if(program->reload_game)
	{
		//allocate data
		memory_area *area = program->area;
		//fix rotation for now
		program->render_parameters = render_set_initial_parameters(game_renderer);
		//slots for damage detection

		//below is basically what this function should do
		u8 *default_map_name = "data/selene_test.ppmp";
		b32 success = game_switch_map(program, default_map_name);
		//for now this is the current map, later this should be
		//on a cached array.


		//build the map
		u32 map_w = 32;
		u32 map_h = 32;
		game_world *current_map = build_test_map(program, map_w, map_h);//assets_load_and_get_map(assets, default_map_name);
		//create second test map
		build_test_map(program, 16, 16);//assets_load_and_get_map(assets, default_map_name);

		{
			//setup the map
			{
				u8 heights[32 * 32] =
				{
					1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					1, 0, 0, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
				};
				for(u32 y = 0; y < map_h; y++)
				{
					for(u32 x = 0; x < map_w; x++)
					{
						u32 index = x + y * map_w;
						u32 index2 = x + ((map_h - 1 - y) * map_w);
						world_tile *tile = current_map->tiles + index;
						tile->height = heights[index2];
					}
				}
			}
			//create events
			{
				event_creation e_creation = begin_event_creation(program);
				event_line *line = add_line_to_event(&e_creation, 0);
				line->next_map_index = 1;
				line->vector3 = V3(15, 15, 0);

				//don't run this if the map is swapping, might just use a flag
				event_condition *cond = add_condition_to_event(&e_creation, event_condition_not_map_swapping);
				cond = add_condition_to_event(&e_creation, event_condition_player_in_area);
				cond->vector3 = V3(300, 300, 20);
				cond->size3 = V3(40, 40, 40);
				game_event *event = end_event_creation(program, current_map, &e_creation);
//				event->eliminate_when_finished = 1;
			}
			program->focused_entity = game_create_player_entity(program);
			program->focused_entity->model = model_allocate_render_data(program->area, build_test_player_model(program));
			program->focused_entity->flags |= coso_render;
			world_entity *enemy_ent = game_create_test_entity(program);
			enemy_ent->body->p.x = 20 * GAME_TILESIZE;
			enemy_ent->body->p.y = 28 * GAME_TILESIZE;

			//Activate to spawn entity
			if(0)
			{
				entity_spawn_parameters sp = {0};
				sp.flags = coso_body | coso_traversable | coso_no_gravity;
				sp.life_time = 5.0f;
				sp.shape = shape_cube;
				sp.speed = 0;
				sp.p = V3(20, 20, 10);
				sp.size = V3(10, 10, 10);
				sp.dir = V2(0, -1);
				request_entity_spawn(program, sp);
			}
//			game_create_test_entity(program);
            program->game_loaded = 1;

			if(current_map->mark_count)
			{
				world_entity *player_entity = current_map->entities + 0;
				player_entity->body->p = current_map->marks[current_map->default_mark].position;

			}
		}

		asset_map_data *loaded_map = 0;//assets_load_and_get_map(assets, default_map_name);
		if(loaded_map)
		{
		    asset_map_to_world_map(loaded_map, current_map);
			//add the count for the player


			//set data
			//	    program->distance_camera_target = 80.0f;

			program->focused_entity = game_create_player_entity(program);
//			game_create_test_entity(program);
            program->game_loaded = 1;

			if(current_map->mark_count)
			{
				world_entity *player_entity = current_map->entities + 0;
				player_entity->body->p = current_map->marks[current_map->default_mark].position;

			}
			// ;unfinished

			//create "enemy"
		//	game_make_test_entity(program, assets);

		}
		else
		{
			stream_pushf(
					info_stream,
					"Error! could not load the default map \"%s\" the program will not load",
					default_map_name);
			//log errors
		}
	    //run program
		program->reload_game = 0;
	}
	
	if(program->game_loaded)
	{
		memory_area *main_memory = program->area;
		game_world *current_map  = program->maps + program->current_map_index;
		//for now take the first slot
		world_entity *player_entity = program->entities + program->player_entity_index;

		//VARIABLES
		player_entity->body->z_speed = 2.7f;
		//		player_entity->looking_direction = V2(0, -1); 

		//set data

		//player_entity->model_key  = assets_load_and_get_model(assets, "data/models/cb.ppmo");

		ASSERT_stream_log(game_console, &assets->info_stream,
				player_entity->model.model != 0);

		player_entity->body->shape.size = V3(12.0f, 12.0f, 24.0f);
		player_entity->body->shape.p = V3(0, 0, player_entity->body->shape.size.z * 0.5f - 0.0f);
		// vec3 collision_offset = player_entity->collision_offset;
		real32 player_jump_speed  = 3.3f;
		// this should be multiplied by delta time and then with the target framerate

		int32 pausePlayer = 0;

		render_commands *commands = render_commands_begin_default(game_renderer);



		//Draw line to nearest floor
		if(commands)
		{
			if(program->debug.debug_display_colliders)
			{
//				debug_display_map_colliders_at(
//						commands,
//						current_map,
//						player_entity->body->p,
//						10);
			}
		}

		update_render_bodies(program, program_input, game_renderer, dt);
		//camera after entity update
		{
			//update and set camera to program
			if(program->debug.free_camera)
			{

				if(!program->debug.interacting_with_ui)
				{
					debug_update_camera(
							game_renderer,
							program_input,
							program);
				}
				game_renderer->camera_position = program->debug.camera.position;
				game_renderer->camera_rotation = program->debug.camera.rotation;
			}
			else
			{
				//normal program camera
				program->render_parameters.camera_target = player_entity->body->p;
				game_update_camera(program,	game_renderer);
				game_renderer->camera_position = program->render_parameters.camera_position;
				game_renderer->camera_rotation = program->render_parameters.camera_rotation;
			}
			render_update_camera_rotation(game_renderer);

			game_renderer->camera_zoom = program->render_parameters.camera_zoom;
		}
		update_render_entities(program, program_input, game_renderer, commands, dt);
		apply_damage_to_targets(program, 0);

		read_events(program);
		//Draw player point
		render_draw_cube(commands,
				player_entity->body->p, V3(1.0f, 1.0f, 1.0f), vec4_all(255));
		//update danios
		//Draw world
		//		game_world_render_tiles(commands, assets, current_map);

		//events



		spawn_requested_entities(program);
		game_build_world_geometry(
				current_map,
				assets,
				commands);
		render_commands_end(commands);

		commands= render_commands_begin_2d(game_renderer);
		game_renderer->camera_zoom_2d = 1.0f;

		render_texture *t = assets_load_and_get_image(program->game_asset_manager, "data/images/model_human.png");
		static f32 px = 20;
		px += 1.0f;
		render_sprite_2d(commands, t, V2(px,  20), V2(400, 400), 0, 0, 16, 20, V4(255, 255, 255, 255));
		transition_map(program, commands);
		render_commands_end(commands);
	}

    update_render_debug_ui(
			program,
			game_renderer,
			program_input,
			dt);
}


GAMEPROGRESS(GameProgress)
{

	memory_area *main_memory = area->main_memory;
	program_state *program = area->game_state_data;

	if(area->dll_reloaded)
	{
		g_info_stream = &program->info_stream;
	}
	if(!program)
	{ 
		//
		// allocate program features 
		//

		area->game_state_data = memory_area_clear_and_push_struct(main_memory, struct s_game_state);
		program = area->game_state_data;
		program->area = main_memory;
		//set platfom
		program->platform   = area->platformApi;
		//set input
		program->input = program_input;
		//if debug
		program->info_stream_area = memory_area_create_from(
				main_memory, KILOBYTES(256));
		program->info_stream = stream_Create(&program->info_stream_area);
		g_info_stream = &program->info_stream;

		//set up the settings to initialize the program
		game_resource_initial_settings game_resource_settings = {0};

		game_resource_settings.total_slots = assets_MAX_ASSET_CAPACITY;
		game_resource_settings.allocation_size = MEGABYTES(12);
		game_resource_settings.operations_size = MEGABYTES(4);
		game_resource_settings.dev_build = 1;
		game_resource_settings.resource_folder_path = "data";
		//start up without allocating any initial assets
		game_resource_settings.initial_assets_count = 0;
		game_resource_settings.use_pack = 0;
		game_resource_settings.resource_pack_file_name = "data/gameassets.pack";//"data/gameassets.pack";
		program->game_asset_manager = assets_allocate_game_assets(
				main_memory,
				program->platform,
				&game_renderer->texture_operations,
				game_resource_settings,
				&program->info_stream_area);

		//load the default font to the ui
		//   program->default_font = assets_GetFont(program->game_asset_manager,
		//		                                         assets_generate_id("data/fonts/roboto.font"),
		//												 3).font;

		program->default_font = assets_load_and_get_font(program->game_asset_manager,
				"data/fonts/roboto.pfnt");
		Assert(program->default_font);

		program->reload_game = 1;
		//program->world = assets_GetMap(program->game_asset_manager, area->platformApi, main_memory,'MAP0');


		//initialize world
		//initialize program
		//
		program->random_s = random_series_create(40);
		u32 bindex = 0;
//		game_body *test_body = game_spawn_body(program, game_get_coso_id(program), &bindex);

		//for debug
		program->ui = area->ui;



		//
		//Debug and console
		//

		program->debug_console = console_allocate(
				main_memory,
				program->platform,
				KILOBYTES(5),
				256,
				256);
		s_game_console *game_console = &program->debug_console;




		//
		// camera parameters
		//
		game_renderer->camera_position = V3(0, 0, 200.0f);
		game_renderer->camera_rotation.x = 0;// -0.5f;

		program = area->game_state_data;

		//program prototype stuff
		{
			allocate_physics_world(program, main_memory);
			allocate_brains(program, main_memory);


			program->entities_max = 100;
			program->entities_count = 0;
			program->entities = memory_area_push_array(main_memory, world_entity,
					program->entities_max);
//			program->focused_entity = game_create_test_entity(program);
		//	program->focused_entity = game_create_player_entity(program);
			program->detections_max = 1000;
			program->detections = memory_area_push_array(main_memory, cosos_detection, program->detections_max);
			program->maps = memory_area_push_array(main_memory, game_world, 100);
		}
		program->area = main_memory;

	}
	program->elapsed_ms = area->elapsed_ms;
	program->mc_per_frame = area->mc_per_frame;
	program->cycles_elapsed = area->cycles_elapsed;
	if(!program->target_framerate) program->target_framerate = 60;
	//	global_target_framerate = program->target_framerate;

	area->target_ms = 1.0f / program->target_framerate;//calc_target_ms(program->target_framerate);//1.0f / (program->target_framerate * 2);
//	program->elapsed_ms = program_memory->elapsed_ms;


	render_texture himtexture  = {2, 0,0, 512, 512};
	render_texture fonttexture = {3, 256,0, 512, 512};

	//
	// begin commands frame
	//

	game_renderer->clear_color[0] = 0;
	game_renderer->clear_color[1] = 0;
	game_renderer->clear_color[2] = 0;
	render_commands *commands = render_commands_begin_default(game_renderer);
	commands->camera_type = render_camera_perspective;

	render_commands_clear_graphics(commands);

	//render_commands_SetProjection(commands);


	render_commands_end(commands);




	//update program
	game_update_render(program, game_renderer, program_input, dt);
	

	//post-update

	//RUN CAMERA
	render_update_camera_values(game_renderer, 16, 9);

	//Push command
	//
	//Update_Render_Debug
	//
	//get console
	s_game_console *game_console = &(program->debug_console);
	console_command_chunk *next_command = 0;
	while(next_command = console_consume_command(game_console))
	{
		game_console_push_command(
				program,
				game_console,
				next_command->data);
	}
	game_console_end_frame(game_console);

	//Asset log
	game_console_push_and_clear_stream(game_console, &program->game_asset_manager->info_stream); 
	game_console_push_and_clear_stream(game_console, &program->info_stream);
	memory_area_reset(&program->info_stream_area);

}

int dll_main()
{
	return(1);
}
//Never used
int _DllMainCRTStartup()
{
	void * ap = GameProgress;
	return(1);
}
