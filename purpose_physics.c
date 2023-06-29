#define physics_debug_BUILD 0
#define PHYSICS_PASSES 1
//physics

//typedef struct{
//	union{
//		i32 value;
//		struct{
//			i8 side_x;
//			i8 side_y;
//			i8 side_z;
//		};
//	};
//}cubes_overlap_result;
//
//typedef struct{
//	vec3 correction;
//	vec3 normal;
//	f32 penetration;
//	b32 got_contact;
//}game_contact_points;
//
//typedef struct game_body{
//	struct game_body *next;
//	struct game_body *prev;
//	game_shape shape;
//	u32 id;
//	b16 collides;
//	b8 grounded;
//	u8 collided_count;
//
//	//position of the closest collider below this body
//	f32 nearest_z_min;
//	f32 nearest_z_max;
//	vec3 p;
//	vec3 v;
//	f32 z_speed;
//	f32 speed;
//	f32 speed_max;
//	f32 bounce_factor;
//}game_body;

static void
allocate_physics_world(program_state *program, memory_area *main_area);
static void
update_render_bodies(program_state *program, program_input *input, game_renderer *game_renderer, f32 dt);
static void
update_render_simulated_world(program_state *program, program_input *input, game_renderer *game_renderer);
static void
update_render_ui_physics(program_state *program, program_input *input, game_renderer *game_renderer);
static game_body *
body_init(game_body *body);
static void
bodies_reset(program_state *program);
static game_body *
body_create(program_state *program);
static void
body_remove(program_state *program, game_body *body);
static inline vec3
body_shape_p(game_body *body);
static inline vec3
body_shape_p_past(game_body *body);
static inline game_contact_points
bodies_cubes_col_data_dynamic(game_body *body0, game_body *body1);
inline cubes_overlap_result 
cubes_overlap(vec3 p0, vec3 sz0, vec3 p1, vec3 sz1);
static vec2
bodies_error(game_body *body0, game_body *body1);
static vec2
body_cube_error(game_body *body0, vec3 cp, vec3 cs);
static physics_zone *
physics_zone_create(program_state *program);

static inline game_contact_points 
cubes_col_data_dynamic_static(game_body *body0, vec3 p1, vec3 sz1);

static vec2
bodies_error(game_body *body0, game_body *body1)
{
	f32 mdx = body1->p.x - body0->p.x;
	f32 mdy = body1->p.y - body0->p.y;
	f32 sizes_hx = (body1->shape.size.x * 0.5f) + (body0->shape.size.x * 0.5f);
	f32 sizes_hy = (body1->shape.size.y * 0.5f) + (body0->shape.size.y * 0.5f);
	f32 error_x = sizes_hx - ABS(mdx);
	f32 error_y = sizes_hy - ABS(mdy);
	vec2 error = {error_x, error_y};
	return(error);
}

static vec2
body_cube_error(game_body *body0, vec3 cp, vec3 cs)
{
	f32 mdx = cp.x - body0->p.x;
	f32 mdy = cp.y - body0->p.y;
	f32 sizes_hx = (cs.x * 0.5f) + (body0->shape.size.x * 0.5f);
	f32 sizes_hy = (cs.y * 0.5f) + (body0->shape.size.y * 0.5f);
	f32 error_x = sizes_hx - ABS(mdx);
	f32 error_y = sizes_hy - ABS(mdy);
	vec2 error = {error_x, error_y};
	return(error);
}

static void
update_render_ui_physics(program_state *program, program_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_textf(ui, "target_framerate");
		ui_spinner_u16(ui, 15, 15, 60, &program->target_framerate, 0, "target_framerate_p");
	}
}

static game_body *
body_init(game_body *body)
{
	body->weight = 0;
	body->nearest_z_max = 100000;
	body->nearest_z_min = 0;
	return(body);
}

static void
bodies_reset(program_state *program)
{
	program->body_count = 0;
	program->first_free_body = 0;
	program->first_body = 0;
}

static game_body *
body_create(program_state *program)
{
	game_body *result = program->first_free_body;
	if(result)
	{
		program->first_free_body = result->next;
	}
	else
	{
		result = program->bodies + program->body_count;
		program->body_count++;
	}
	memory_clear(result, sizeof(*result));

	if(program->first_body)
	{
		program->first_body->prev = result;
	}
	result->next = program->first_body;
	program->first_body = result;
	result->id = program->body_id++;
	body_init(result);
	return(result);
}

static void
body_remove(program_state *program, game_body *body)
{
	body->id = 0;
	body->user_data = 0;

	//disconnect from other bodies
	if(body->prev) body->prev->next = body->next;
	if(body->next) body->next->prev = body->prev;

	if(program->first_body == body)
	{
		program->first_body = body->next;
	}

	body->next = program->first_free_body;
	program->first_free_body = body;
}

static void
allocate_physics_world(program_state *program, memory_area *main_area)
{
	program->body_max = 1000;
	program->bodies = memory_area_push_array(main_area, game_body, program->body_max);
	program->body_solve_max = 100;
	program->body_solves = memory_area_push_array(main_area, game_body_solve, program->body_solve_max);

	program->body_tile_solve_max = 100;
	program->body_tile_solves = memory_area_push_array(main_area, game_tile_solve, program->body_tile_solve_max);

	program->collision_signal_max = 100;
	program->collision_signals = memory_area_push_array(main_area, game_collision_signal,
			program->collision_signal_max);

	//allocate moving bodies
#if physics_debug_BUILD
	{
		game_body *body = body_create(program);
		program->controlling_body = body;
		//set stats
		body->speed = 1.2f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 25; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 0;
		body->p.y = 0;
	}
	{
		game_body *body = body_create(program);
		//set stats
		body->speed = 1.2f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 25; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 60;
		body->p.y = 0;
	}
	{
		game_body *body =body_create(program); 
		//set stats
		body->speed = 1.2f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 105; 
		body->shape.size.x = 20;
		body->shape.size.y = 20;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 100;
		body->p.y = 40;
	}
	//allocate static bodies
	for(u32 b = 0; b < 20; b++)
	{
		game_body *body =body_create(program); 
		//set stats
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->weight = 0;
		body->p.y = 100;
		body->p.x = body->shape.size.x * b;
	}
#endif
}

    static vec2
    get_vector_angles(f32 x, f32 y)
    {
    	vec2 moving_dir = {0};
    	if(y > 0)
    	{
    		moving_dir.y = 1;
    	}
    	else if(y < 0)
    	{
    		moving_dir.y = -1;
    	}
    	if(x < 0)
    	{
    		moving_dir.x = -1;
    	}
    	else if(x > 0)
    	{
    		moving_dir.x = 1;
    	}
    	//Also gives the sign
    	vec2 speed_angles = vec2_normalize_zero(
    			moving_dir);
    
    	return(speed_angles);
    }
    
    inline cubes_overlap_result 
    cubes_overlap(
    		vec3 p0,
    		vec3 sz0,
    		vec3 p1,
    		vec3 sz1)
    { 
    	//Detect if on side
    	f32 distance_from_x = p0.x - p1.x;
    	f32 distance_from_y = p0.y - p1.y;
    	f32 distance_from_z = p0.z - p1.z;
    
    	f32 sizes_sum_x = ((sz1.x + sz0.x) * 0.5f);
    	f32 sizes_sum_y = ((sz1.y + sz0.y) * 0.5f);
    	f32 sizes_sum_z = ((sz1.z + sz0.z) * 0.5f);
    	f32 inside_formula_x = (distance_from_x) / sizes_sum_x;
    	f32 inside_formula_y = (distance_from_y) / sizes_sum_y;
    	f32 inside_formula_z = (distance_from_z) / sizes_sum_z;

    
    	cubes_overlap_result overlap_result = {0};
    	//0 means inside this axis
    	overlap_result.side_x = ((i32)inside_formula_x);
    	overlap_result.side_y = ((i32)inside_formula_y);
    	overlap_result.side_z = ((i32)inside_formula_z);
    	return(overlap_result);
    
    }
    
    static inline cubes_overlap_result
    body_cubes_overlap(
    		game_body *body0, game_body *body1)
    {
    	vec3 body0_col_p = {
    		body0->p.x + body0->shape.p.x,
    		body0->p.y + body0->shape.p.y,
    		body0->p.z + body0->shape.p.z,
    	};
    	vec3 body1_col_p = {
    		body1->p.x + body1->shape.p.x,
    		body1->p.y + body1->shape.p.y,
    		body1->p.z + body1->shape.p.z,
    	};
    
    	cubes_overlap_result result = cubes_overlap(
    			body0_col_p, body0->shape.size,
    			body1_col_p, body1->shape.size);
    	return(result);
    }

	static inline cubes_overlap_result
    body_cubes_overlap_past(
    		game_body *body0, game_body *body1)
    {
    	vec3 body0_col_p = {
    		body0->p_past.x + body0->shape.p.x,
    		body0->p_past.y + body0->shape.p.y,
    		body0->p_past.z + body0->shape.p.z,
    	};
    	vec3 body1_col_p = {
    		body1->p_past.x + body1->shape.p.x,
    		body1->p_past.y + body1->shape.p.y,
    		body1->p_past.z + body1->shape.p.z,
    	};
    
		f32 red_pct = 1.0f;//.9f;
    	cubes_overlap_result result = cubes_overlap(
    			body0_col_p, vec3_scale(body0->shape.size, red_pct),
    			body1_col_p, vec3_scale(body1->shape.size, red_pct));
    	return(result);
    }

    
    //assumes the vectors are normalized
    static inline vec3
    vec3_reflect(vec3 v0, vec3 v1)
    {
    	vec3 v0n = {-v0.x, -v0.y, -v0.z};
    	f32 n_dot = vec3_inner(v0n, v1);
    	//v0 + 2(n_dot)v1 
    	vec3 result = vec3_normalize_safe(vec3_add(v0, vec3_scale(v1, 2 * n_dot)));
//    	vec3 result = vec3_add(v0, vec3_scale(v1, 2 * n_dot));
    
    	return(result);
    }
    static inline vec3
	body_shape_p(game_body *body)
    {
		vec3 p = {
			body->p.x + body->shape.p.x,
			body->p.y + body->shape.p.y,
			body->p.z + body->shape.p.z,
		};
		return(p);
    }

    static inline vec3
	body_shape_p_past(game_body *body)
    {
		vec3 p = {
			body->p_past.x + body->shape.p.x,
			body->p_past.y + body->shape.p.y,
			body->p_past.z + body->shape.p.z,
		};
		return(p);
    }

    static inline game_contact_points 
    bodies_cubes_col_data_dynamic(
			game_body *body0,
			game_body *body1)
    {
		vec3 p0  = body_shape_p(body0);
		vec3 v0  = body0->v;
		vec3 sz0 = body0->shape.size;
		vec3 p1  = body_shape_p(body1);
		vec3 sz1 = body1->shape.size;
    
		cubes_overlap_result overlap_results = body_cubes_overlap_past(
				body0, body1);
		cubes_overlap_result overlap_results_now = body_cubes_overlap(
				body0, body1);
		game_contact_points current_data = {0};

    	vec3 size0_h = vec3_f32_mul(sz0, 0.5f);
    	vec3 size1_h = vec3_f32_mul(sz1, 0.5f);
    
    	vec3 reflection = {0};
    	vec3 normal0 = vec3_normalize_safe(v0);
    	vec3 normal1 = {0};
		vec3 collision_normal = {0};
    	b8 updated_correction = 0;
		f32 depth = 0;
    
    	//inside y axis
    	u32 inside_x = overlap_results.side_y == 0;
    	//inside x axis
    	u32 inside_y = overlap_results.side_x == 0;
    	i32 at_x_positive = v0.x > 0;
    	int s = 0;
    	//Get the formula from past!
    	b16 moving_y = v0.y != 0;
    	b16 moving_x = v0.x != 0;
		//lado de body0 con 1
		//a que dirección se mueven
		//si inside_y y body 0 se mueven por abajo
		b32 got_contact = 0;
		i32 side_x_p = SIGN(overlap_results.side_x);
			i32 side_x_f = SIGN(overlap_results_now.side_x);
			if(side_x_p != side_x_f)
			{
				int s = 0;
			}
#if 1
    	if(inside_y && !inside_x)
		{

			i32 side_y_p = SIGN_OR_ZERO(overlap_results.side_y);
			i32 side_y_f = SIGN_OR_ZERO(overlap_results_now.side_y);

			//Distance from left is positive
			if(side_y_p != side_y_f)
			{
				normal1.y = (f32)side_y_p;
				got_contact = 1;

				f32 distance_from_y = p1.y - p0.y;
				f32 extend_distance_y = size0_h.y + size1_h.y - ABS(distance_from_y);
				depth = extend_distance_y;
				collision_normal.y = (f32)-side_y_p;
			}
		}
    	else if(inside_x && !inside_y)
		{

			i32 side_x_p = SIGN_OR_ZERO(overlap_results.side_x);
			i32 side_x_f = SIGN_OR_ZERO(overlap_results_now.side_x);

			//Distance from left is positive
			if(side_x_p != side_x_f)
			{
				normal1.x = (f32)side_x_p;
				got_contact = 1;

				f32 distance_from_x = p1.x - p0.x;
				f32 extend_distance_x = size0_h.x + size1_h.x - ABS(distance_from_x);
				depth = extend_distance_x;
					collision_normal.x = (f32)-side_x_p;
			}
		}
#endif
    	//inside, but not moving; so correct based on location of centers
    	if(1 && inside_x && inside_y && !got_contact)
    	{
			f32 distance_from_y = p1.y - p0.y;
			f32 distance_from_x = p1.x - p0.x;
			f32 extend_distance_y = size0_h.y + size1_h.y - ABS(distance_from_y);
			f32 extend_distance_x = size0_h.x + size1_h.x - ABS(distance_from_x);
			//collides if positive
			//if(extend_distance > 0)
			//{
			//}
			got_contact = 1;
			if(extend_distance_x < extend_distance_y)
			{
				depth = extend_distance_x;
				//move a to <-
				if(distance_from_x > 0)
				{
					normal1.x = -1;
					normal0.x = 1;
					collision_normal.x = 1;
				}
				else
				{
					normal1.x = 1;
					normal0.x = -1;
					collision_normal.x = -1;
				}
			}
			else
			{
				//move a to <-
				if(distance_from_y > 0)
				{
					normal1.y = -1;
					normal0.y = 1;
					collision_normal.y = 1;
				}
				else
				{
					normal1.y = 1;
					normal0.y = -1;
					collision_normal.y = -1;
				}
				depth = extend_distance_y;
			}
    	}
    
		if(got_contact)
		{
			current_data.correction = vec3_scale(normal1, depth);
			current_data.penetration = depth;
			reflection = vec3_reflect(normal0, normal1);
			current_data.normal = collision_normal;//normal1;//reflection;
			if(reflection.x > 1)
			{
				int s = 0;
			}
			current_data.got_contact = got_contact;
		}
		//		stream_pushf(g_info_stream, "normal0 {%f, %f}, 1 {%f, %f}, reflection {%f, %f, %f}",
		//				normal0.x,
		//				normal0.y,
		//				normal1.x,
		//				normal1.y,
		//				reflection.x,
		//				reflection.y,
		//				reflection.z
		//				);
    	return(current_data);
    }
    
    static inline game_contact_points 
    cubes_col_data_dynamic_static(
			game_body *body0,
    		vec3 p1,
    		vec3 sz1)
    {
		vec3 p0  = body_shape_p(body0);
		vec3 v0  = body0->v;
		vec3 sz0 = body0->shape.size;
    
    	cubes_overlap_result overlap_results_now = cubes_overlap(
    			body_shape_p(body0), body0->shape.size,
				p1, sz1);
		cubes_overlap_result overlap_results = cubes_overlap(
    			body_shape_p_past(body0), body0->shape.size,
				p1, sz1);
		game_contact_points current_data = {0};

    	vec3 size0_h = vec3_f32_mul(sz0, 0.5f);
    	vec3 size1_h = vec3_f32_mul(sz1, 0.5f);
    
    	vec3 reflection = {0};
    	vec3 normal0 = vec3_normalize_safe(v0);
    	vec3 normal1 = {0};
		vec3 collision_normal = {0};
    	b8 updated_correction = 0;
		f32 depth = 0;
    
    	//inside y axis
    	u32 inside_x = overlap_results.side_y == 0 && overlap_results.side_z == 0;
    	//inside x axis
    	u32 inside_y = overlap_results.side_x == 0 && overlap_results.side_z == 0;
    	i32 at_x_positive = v0.x > 0;
    	int s = 0;
    	//Get the formula from past!
    	b16 moving_y = v0.y != 0;
    	b16 moving_x = v0.x != 0;
		//lado de body0 con 1
		//a que dirección se mueven
		//si inside_y y body 0 se mueven por abajo
		b32 got_contact = 0;
		i32 side_x_p = SIGN(overlap_results.side_x);
			i32 side_x_f = SIGN(overlap_results_now.side_x);
			if(side_x_p != side_x_f)
			{
				int s = 0;
			}
#if 1
    	if(inside_y && !inside_x)
		{

			i32 side_y_p = SIGN_OR_ZERO(overlap_results.side_y);
			i32 side_y_f = SIGN_OR_ZERO(overlap_results_now.side_y);

			//Distance from left is positive
			if(side_y_p != side_y_f)
			{
				normal1.y = (f32)side_y_p;
				got_contact = 1;

				f32 distance_from_y = p1.y - p0.y;
				f32 extend_distance_y = size0_h.y + size1_h.y - ABS(distance_from_y);
				depth = extend_distance_y;
				collision_normal.y = (f32)-side_y_p;
			}
		}
    	else if(inside_x && !inside_y)
		{

			i32 side_x_p = SIGN_OR_ZERO(overlap_results.side_x);
			i32 side_x_f = SIGN_OR_ZERO(overlap_results_now.side_x);

			//Distance from left is positive
			if(side_x_p != side_x_f)
			{
				normal1.x = (f32)side_x_p;
				got_contact = 1;

				f32 distance_from_x = p1.x - p0.x;
				f32 extend_distance_x = size0_h.x + size1_h.x - ABS(distance_from_x);
				depth = extend_distance_x;
					collision_normal.x = (f32)-side_x_p;
			}
		}
#endif
    	//inside, but not moving; so correct based on location of centers
    	if(1 && inside_x && inside_y && !got_contact)
    	{
			f32 distance_from_y = p1.y - p0.y;
			f32 distance_from_x = p1.x - p0.x;
			f32 extend_distance_y = size0_h.y + size1_h.y - ABS(distance_from_y);
			f32 extend_distance_x = size0_h.x + size1_h.x - ABS(distance_from_x);
			//collides if positive
			//if(extend_distance > 0)
			//{
			//}
			got_contact = 1;
			if(extend_distance_x < extend_distance_y)
			{
				depth = extend_distance_x;
				//move a to <-
				if(distance_from_x > 0)
				{
					normal1.x = -1;
					normal0.x = 1;
					collision_normal.x = 1;
				}
				else
				{
					normal1.x = 1;
					normal0.x = -1;
					collision_normal.x = -1;
				}
			}
			else
			{
				//move a to <-
				if(distance_from_y > 0)
				{
					normal1.y = -1;
					normal0.y = 1;
					collision_normal.y = 1;
				}
				else
				{
					normal1.y = 1;
					normal0.y = -1;
					collision_normal.y = -1;
				}
				depth = extend_distance_y;
			}
    	}
    
		if(got_contact)
		{
			current_data.correction = vec3_scale(normal1, depth);
			current_data.penetration = depth;
			reflection = vec3_reflect(normal0, normal1);
			current_data.normal = collision_normal;//normal1;//reflection;
			if(reflection.x > 1)
			{
				int s = 0;
			}
			current_data.got_contact = got_contact;
		}
    	return(current_data);
    }
    static void
	add_collision_signal(
			program_state *program,
			game_body *body0, game_body *body1)
    {
		u32 count = program->collision_signal_count;
		b32 add_new = 1;
		for(u32 c = 0; c < count; c++)
		{
			game_collision_signal *callback = program->collision_signals + c;
			b32 a = body0 == callback->body0 || body0 == callback->body1;
			b32 b = body1 == callback->body0 || body1 == callback->body1;
			if(a && b)
			{
				add_new = 0;
				break;
			}
		}
		if(add_new)
		{
			game_collision_signal *new_callback = program->collision_signals + program->collision_signal_count;
			program->collision_signal_count++;
			new_callback->body0 = body0;
			new_callback->body1 = body1;
		}
		Assert(program->collision_signal_max > program->collision_signal_count);
    }

/*
   Orden de actualizacion:
   1. Las fuerzas y rotaciones se integran utilizando dt
   2. Luego de su movimiento se tratará de detectar nuevos contactos generados por ellos.
   Se necesita saber si se perdieron o generaron nuevos contactos.
   3. Se resuelven las colisiones
   4. Se corrige cualquier error.

   Luego, puede que las nuevas posiciones corregidas dejen al cuerpo dentro de otro por lo que
   se genera una nueva conexión. Normalmente la resolución de estos cuerpos se repite hasta que
   todos ellos terminen correctamente corregidos o se alcanza un número máximo de repeticiones.

   En un simple ejemplo, cuando dos cuerpos chocan y se empujan entre si, estos serán resueltos
   y luego se buscará nuevos cuerpos con quienes estos contacten, pero como no hay otros, el
   loop se detiene sin hacer nada mas.

   acceleration = force / mass
   force = mass * acceleration
*/
static void
update_render_bodies(program_state *program, program_input *input, game_renderer *game_renderer, f32 dt)
{
//	render_commands *debug_commands = render_commands_begin_default(game_renderer);


	//set up arbitrari parameters
	if(0)
	{
		if(program->body_count > 1)
		{
			game_body *body = program->bodies + 1;
			body->weight = 10;
		}
	}

			static game_contact_points static_contact_data = {0};
			static game_body static_b0 = {0};
			static game_body static_b1 = {0};
	
	//1.
	//get active zone
	physics_zone *physics_zone = 0;
//	for(active_body *ab = physics_zone->active_bodies;
//			ab; ab = ab->next)
	{
//		game_body *body = ab->body;

	f32 last_frame_nz = 0;
	for(game_body *body = program->first_body; body; body = body->next)
	{
		b8 skip_decceleration[4] = {0};
		body->collides = 0;
		//decceleration
		{
			//only apply decceleration
			f32 acceleration = 0.3f;
			f32 decceleration = 0.16f;
			//	f32 speed_cap = 4.2f;
			f32 speed_cap = body->speed_max;
			f32 dt_s = dt * 49;
			f32 deccel = decceleration * dt * 90;
			if(!skip_decceleration[0] && body->v.y > 0)
			{
				body->v.y -= (deccel);
				body->v.y = MAX(0, body->v.y);

			}
			else if(!skip_decceleration[1] && body->v.y < 0)
			{
				body->v.y += (deccel);
				body->v.y = MIN(0, body->v.y);
			}


			if(!skip_decceleration[2] && body->v.x > 0)
			{
				body->v.x -= (deccel);
				body->v.x = MAX(0, body->v.x);

			}
			else if(!skip_decceleration[3] && body->v.x < 0)
			{
				body->v.x += (deccel);
				body->v.x = MIN(0, body->v.x);
			}
			//apply speed cap
			vec2 speed_angles = get_vector_angles(body->v.x, body->v.y);
			vec2 cap = {speed_cap * speed_angles.x, speed_cap * speed_angles.y};
			if(body->v.y > 0)
			{
				body->v.y = MIN(speed_cap * speed_angles.y, body->v.y);
			}
			else if(body->v.y < 0)
			{
				body->v.y = MAX(speed_cap * speed_angles.y, body->v.y);
			}
			if(body->v.x < 0)
			{
				body->v.x = MAX(speed_cap * speed_angles.x, body->v.x);
				//body->v.x = body->v.x > cap.x ? cap.x : body->v.x;
			}
			else if(body->v.x > 0)
			{
				body->v.x = MIN(speed_cap * speed_angles.x, body->v.x);
				//			body->v.x = body->v.x < cap.x ? cap.x : body->v.x;
			}
		}
		//gravity and ground
		last_frame_nz = body->nearest_z_min;
		if(1)
		{
			//position of shape
			vec3 collision_offset = body->shape.p;
			vec3 collision_size_h = vec3_f32_mul(body->shape.size, 0.5f);
			b32 was_grounded = body->grounded;
			//Move nearest floor to match the position.
			f32 ent_bottom = collision_offset.z - collision_size_h.z; 
			f32 ent_top = collision_offset.z + collision_size_h.z; 
			//in body space
			f32 nearestFloorFromP = body->nearest_z_min - ent_bottom;
			f32 nearestRoofFromP = body->nearest_z_max - ent_top;
			f32 bounce_factor = body->bounce_factor;

			body->grounded = 0;

			//Apply gravity
			if(!body->ignore_gravity)
			{
				f32 bounce_value = body->v.z;
				f32 tGravity = 0.20f;
				f32 gravity = tGravity * dt * 50;

				f32 body_z = body->p.z + body->v.z;
				if(body_z > nearestFloorFromP)
				{
					body->v.z -= gravity;
				}

				if(body_z <= nearestFloorFromP)
				{
					if(!was_grounded)
					{
						body->collided_count++;
					}
					//Get distance after gravity
					//entity->position.z    = nearest_z_min - collision_size_h.z;
					//Note(Agu): this may have some precision problems.
					if(0 && body->v.z < 0)
					{
						real32 bottom_z = (body->p.z + collision_offset.z - 
								((f32)body->shape.size_z * 0.5f));

						f32 v = ABS(bottom_z - nearestFloorFromP);
						f32 rt = bottom_z / body->v.z;
						if(v)
						{
							int s = 0;
						}

#if 1
						body->v.z = -body->v.z * bounce_factor;
						//subtract gravity with the difference
						body->v.z -= gravity * ABS(rt);
						body->v.z = body->v.z < 0 ? 0 : body->v.z;
#else
						body->v.z = v;
						body->v.z -= tGravity * dt * 50;
						body->v.z = body->v.z < 0 ? 0 : body->v.z;
#endif
					}
					else
					{
						body->v.z = body->v.z < 0 ? 0 : body->v.z;
					}
					body->p.z = nearestFloorFromP;
					//		body->p_past.z = body->p.z;
					body->grounded = 1;
				}
				if(body->p.z > nearestRoofFromP)
				{
					//Get distance after gravity
					//entity->position.z    = nearest_z_min - collision_size_h.z;
					//Note(Agu): this may have some precision problems.
					body->p.z = nearestRoofFromP;
					body->v.z = 0;
				}
			}
		}


		static_b1 = static_b0;
		static_b0 = *body;
		body->p_past = body->p;
		body->p.x += body->v.x;
		body->p.y += body->v.y;
		body->p.z += body->v.z;

//		body->v.x = 0;
//		body->v.y = 0;
//		body->v.z = 0;
		//reset collided count for this frame
	    real32 z_minimum = 0;
	    real32 z_max = 1000000;
		body->collided_count = 0;
	    body->nearest_z_min = z_minimum;
	    body->nearest_z_max = z_max;
	}
	}
		if(program->player_entity->body->p.z < 0)
		{
			int s = 0;
		}

	//2.collision detection
	//number of passes for correction
	for(u32 t = 0; t < PHYSICS_PASSES; t++)
	{
		//This might work inside the same loop
		//get current map colliders
		//grids can be used to divide colliders on the future.
		game_world *map = program->maps + program->current_map_index;
		u32 tz = GAME_TILESIZE;
		f32 tz_h = tz * .5f;
		for(game_body *body0 = program->first_body; body0; body0 = body0->next)
		{
			game_contact_points contact_data = {0};
			vec3 contacted_tile_p = {0};
			vec3 contacted_tile_s = {0};
			//collision with tiles.
			//DON'T DELETE
#if 1
			for(u32 y = 0; y < map->h; y++)
			{
				for(u32 x = 0; x < map->w; x++)
				{
					u32 i = x + y * map->w;
					//tile position and size
					world_tile *tile = map->tiles + i;
					vec3 size1 = {
					GAME_TILESIZE,
					GAME_TILESIZE,
					0};
					vec3 p = {(f32)x * GAME_TILESIZE, (f32)y * GAME_TILESIZE, 0};
					p.x += size1.x * .5f;
					p.y += size1.y * .5f;
					//adjust position

					if(tile->height > 0)
					{
						//size1.z = (f32)GAME_TILESIZE + (tile->height * GAME_TILESIZE);
						//p.z = (f32)(tile->height * tz_h) - tz_h;
						size1.z = (f32)(tile->height * GAME_TILESIZE);
						p.z += size1.z * 0.5f;;
					}
					else
					{
						size1.z = 0;
						p.z = (f32)tile->height * tz;
					}

					//detect if it's above or below
					cubes_overlap_result overlap_results = cubes_overlap(
							body_shape_p_past(body0), body0->shape.size,
							p, size1);
					cubes_overlap_result overlap_results_future = cubes_overlap(
							body_shape_p(body0), body0->shape.size,
							p, size1);
//				    b32 is_inside = overlap_results.side_x == 0 && overlap_results.side_y == 0;
				    b32 is_inside = overlap_results_future.side_x == 0 && overlap_results_future.side_y == 0;
					//check if it's above or below the tile.
					//above
					if(is_inside && overlap_results.side_z != 0)
					{
						if(overlap_results.side_z > 0)
						{
							f32 top = (f32)tile->height * GAME_TILESIZE;
							body0->nearest_z_min = top > body0->nearest_z_min ? top : body0->nearest_z_min;
						}
						//below
						else if(overlap_results.side_z < 0)
						{
						}
					}
					else
					{
						game_contact_points current_contact_data = cubes_col_data_dynamic_static(
								body0,
								p,
								size1);
						if(current_contact_data.got_contact)
						{
							int s = 0;
						}
						if(current_contact_data.penetration && current_contact_data.penetration > contact_data.penetration)
						{
							contact_data = current_contact_data;
							contacted_tile_p = p;
							contacted_tile_s = size1;

							if(body0->nearest_z_min > 0)
							{
								int s = 0;
							}
						}
					}
				}
			}
#endif
			//only solve if collided
			if(contact_data.got_contact)
			{
				game_tile_solve *tsolve = program->body_tile_solves + program->body_tile_solve_count++;
				tsolve->body0 = body0;
				tsolve->tp = contacted_tile_p;
				tsolve->ts = contacted_tile_s;
				tsolve->contact_data = contact_data;
				static_contact_data = contact_data;
			}
		}
		//dynamic bodies
		for(game_body *body0 = program->first_body; body0; body0 = body0->next)
		{
			for(game_body *body1 = program->first_body; body1; body1 = body1->next)
			{
				if(body1 == body0) break;

				b32 collides = 0;

				vec3 body0_col_p = {
					body0->p.x + body0->shape.p.x,
					body0->p.y + body0->shape.p.y,
					body0->p.z + body0->shape.p.z,
				};
				vec3 body1_col_p = {
					body1->p.x + body1->shape.p.x,
					body1->p.y + body1->shape.p.y,
					body1->p.z + body1->shape.p.z,
				};

				game_contact_points contact_data = {0};
				//	if(b == 0)
				//	contact_data = cubes_col_data_dynamic_static(
				//			overlap_result_p,
				//			body0_col_p,
				//			body0->v,
				//			body0->shape.size,
				//			body1_col_p,
				//			body1->shape.size,
				//			contact_data
				//			);
				contact_data = bodies_cubes_col_data_dynamic(
						body0,
						body1);
				if(contact_data.got_contact)
				{
					body0->collides = 1;
					body1->collides = 1;
					//add solve
					game_body_solve *solve = program->body_solves + program->body_solve_count;
					program->body_solve_count++;
					solve->body0 = body0;
					solve->body1 = body1;
					solve->contact_data = contact_data;
				}
			}
		}
		//3.collision solve 

		//sort
		//Solve first those who collided first
		for(u32 s = 0; s < program->body_solve_count; s++)
		{
			game_body_solve *solve0 = program->body_solves + s;
			f32 depth0 = ABS(solve0->contact_data.penetration);
			for(u32 d = 0; d < program->body_solve_count; d++)
			{
				game_body_solve *solve1 = program->body_solves + d;
				f32 depth1 = ABS(solve1->contact_data.penetration);
				if(depth0 > depth1)
				{
					game_body_solve copy = program->body_solves[s];
					program->body_solves[s] = program->body_solves[d];
					program->body_solves[d] = copy;
				}
			}

		}
		//body-tile resolution
		for(u32 s = 0; s < program->body_tile_solve_count; s++)
		{
			game_tile_solve *tile_solve = program->body_tile_solves + s;
			game_body *body0 = tile_solve->body0;
			game_contact_points contact_data = tile_solve->contact_data;
			//calculate impulse, based on the body's bounciness
			vec3 rel_v = vec3_inverse(body0->v);
			f32 rel_vn = vec3_inner(contact_data.normal, rel_v);
			f32 restitution = 0.0f;
			f32 j = rel_vn * -(1 + restitution);
			f32 iw0 = body0->weight ? 1.0f / body0->weight : 0.0f;
			j = j / iw0;
			vec3 impulse = vec3_scale(contact_data.normal, j);

			f32 pn0 = -(contact_data.penetration);
			body0->p = vec3_add(body0->p, (vec3_scale(contact_data.normal, pn0)));
			body0->v = vec3_add(body0->v, vec3_scale(impulse, -iw0));

			vec2 error = body_cube_error(body0, tile_solve->tp, tile_solve->ts);
			error.x *= contact_data.normal.x;
			error.y *= contact_data.normal.y;

			if(error.x || error.y)
			{
				int s = 0;
			}

		}
		program->body_tile_solve_count = 0;
		//bodies resolution
		for(u32 s = 0; s < program->body_solve_count; s++)
		{
			game_body_solve *solve = program->body_solves + s;
			game_contact_points contact_data = solve->contact_data;
			game_body *body0 = solve->body0;
			game_body *body1 = solve->body1;

			//stupid skip
			if(body0->traversable || body1->traversable)
			{
				add_collision_signal(program, body0, body1);
				continue;
			}

			//re-check their penetration after previous solves
			f32 mdx = body1->p.x - body0->p.x;
			f32 mdy = body1->p.y - body0->p.y;
			f32 sizes_hx = (body1->shape.size.x * 0.5f) + (body0->shape.size.x * 0.5f);
			f32 sizes_hy = (body1->shape.size.y * 0.5f) + (body0->shape.size.y * 0.5f);
			f32 error_x = sizes_hx - ABS(mdx);
			f32 error_y = sizes_hy - ABS(mdy);
			body0->p_past = body0->p;
			body1->p_past = body1->p;

			vec3 penetration_v = vec3_mul(contact_data.normal, V3(error_x, error_y, 0));
			//no contact
			if(error_x < 0)
			{
				penetration_v.x = 0;
			}
			if(error_y < 0)
			{
				penetration_v.y = 0;
			}
			contact_data.penetration = vec3_length(penetration_v);
			//no contact was made
			if(!contact_data.penetration)
			{
				continue;
			}
			//add contact signal
			add_collision_signal(program, body0, body1);
			f32 restitution = 0.0f * 0.0f; //from body a and b
										   //f32 restitution = MIN(4.0f, 1.0f); //from body a and b
			vec3 rel_v = vec3_sub(body1->v, body0->v);
			f32 rel_vn = vec3_inner(contact_data.normal, rel_v);
			f32 j = rel_vn * -(1 + restitution);
			f32 iw0 = body0->weight ? 1.0f / body0->weight : 0.0f;
			f32 iw1 = body1->weight ? 1.0f / body1->weight : 0.0f;

			f32 sum_of_w = iw0 + iw1;//body0->weight + body1->weight;
			f32 w0 = 0;
			f32 w1 = 0;
			if(sum_of_w)
			{
				w0 = iw0 / sum_of_w;
				w1 = iw1 / sum_of_w;		
				j = j / sum_of_w;
			}
			else
			{
				j = 0;
			}
			vec3 impulse = vec3_scale(contact_data.normal, j);

			f32 pn0 = -contact_data.penetration;
			f32 pn1 = contact_data.penetration;
			//fix a little "ping-pong" issue
			if(body0->weight != 0 && ABS(pn0) < 0.00001f)
			{
				pn0 = pn0 < 0 ? -0.00001f : 0.00001f;
			}
			if(body1->weight != 0 && ABS(pn1) < 0.00001f)
			{
				pn1 = pn1 < 0 ? -0.00001f : 0.00001f;
			}

			body0->p = vec3_add(body0->p, (vec3_scale(contact_data.normal, pn0 * w0)));
			body1->p = vec3_add(body1->p, (vec3_scale(contact_data.normal, pn1 * w1)));
			body0->v = vec3_add(body0->v, vec3_scale(impulse, -iw0));
			body1->v = vec3_add(body1->v, vec3_scale(impulse, iw1));

			//solve smaller errors if needed
			mdx = body1->p.x - body0->p.x;
			mdy = body1->p.y - body0->p.y;
			error_x = sizes_hx - ABS(mdx);
			error_y = sizes_hy - ABS(mdy);
			while(error_x > 0.0f && contact_data.normal.x)
			{
				int s = 0;
				body0->p.x += .5f * error_x * -contact_data.normal.x;
				body1->p.x += .5f * error_x * contact_data.normal.x;

				mdx = body1->p.x - body0->p.x;
				error_x = sizes_hx - ABS(mdx);
			}
			while(error_y > 0.0f && contact_data.normal.y)
			{
				body0->p.y += .5f * error_y * -contact_data.normal.y;
				body1->p.y += .5f * error_y * contact_data.normal.y;

				mdy = body1->p.y - body0->p.y;
				error_y = sizes_hy - ABS(mdy);
			}


		}

		program->body_solve_count = 0;
	}
	//read signals?
	program->collision_signal_count = 0;
	//4.
#if physics_debug_BUILD
	for(u32 b = 0; b < program->body_count; b++)
	{
		game_body *body = program->bodies + b;

		vec4 color = {0, 0, 0, 255};
		if(b == 0) color.r = 255;
		if(b > 0) color.g = 255;
		if(body->weight < 0)
		{
			color.b = 0;
		}
		//render the body
	//	render_draw_cube(debug_commands,
	//			body->p,
	//			body->shape.size,
	//			V4(255, 0, 0, 255));
//		render_cube_borders(debug_commands,
//				body->p,
//				body->shape.size,
//				0.5f,
//				V4(255, 255, 255, 255));
	}
	//move the body 0

	if(program->controlling_body)
	{
		game_body *body = program->controlling_body;
		body->z_speed = 2.7f;

		if(input_down(input->w)) body->v.y += body->speed;
		else if(input_down(input->s)) body->v.y -= body->speed;

		if(input_down(input->a)) body->v.x -= body->speed;
		else if(input_down(input->d)) body->v.x += body->speed;

		if(input_down(input->z) && body->grounded) body->v.z += body->z_speed;
	}
#endif
//	render_commands_end(debug_commands);
}

static void
update_render_simulated_world(program_state *program, program_input *input, game_renderer *game_renderer)
{
}

