typedef struct{
	temporary_area temp_area;
	state_main brain;
}game_brain_creation;

static void
allocate_brains(program_state *program, memory_area *main_area);
static state_main *
create_brain(program_state *program, state_definition definition);
static void
remove_brain(program_state *program, state_main *brain);
static void
fragment_brains(program_state *program);
static game_brain_creation 
begin_brain_creation(program_state *program);
static state_main *
end_brain_creation(program_state *program, game_brain_creation bc);

static state_action_line *
brain_add_action_line(state_main *brain, state_action *action, state_action_line_type type);
static state_action *
brain_add_action(state_main *brain);
static state_node *
brain_add_state(state_main *brain);
static state_line *
brain_add_state_line(state_main *brain, state_node *state);
static state_trigger *
brain_add_line_condition(state_main *brain, state_line *sdo, state_do_flags type);
static void
brains_update(program_state *program, program_input *input, world_entity *ent,f32 dt);
static b32
brain_read_trigger(world_entity *ent, state_node *current_state, state_trigger *trigger, program_input *input);
static b32
run_timer(f32 *timer, f32 total_ms, f32 dt);
static b32
run_frame_timer(f32 *timer, f32 total_ms, f32 dt);

static void
brains_updateV0(world_entity *ent, program_input *input, random_series *random, f32 dt);
static void
brains_updateV1(program_state *program, program_input *input, world_entity *ent,f32 dt);

#define brain_current_state(brain) brain->states + brain->current
//lines
static void
brain_input_move(world_entity *ent, program_input *input, f32 dt);

//brains
static void
brain_switch_state(world_entity *ent, u32 state_index);
static void
run_brain(program_state *program,program_input *input, world_entity *ent, f32 dt);
//conditions
static b32
brain_frame_step_between(world_entity *ent, u32 frame_at, u32 frame_duration);
static void
brain_add_speed_to_facing(world_entity *ent, f32 speed, f32 dt);
//spawn attack sweep
static void
brain_spawn_sweep(program_state *program, world_entity *ent, f32 distance, f32 z, vec3 size, b32 relative_to_dir, i32 side, f32 start_angle);
static void
brain_spawn_front(program_state *program, world_entity *ent, f32 distance, f32 z, vec3 size, b32 relative_to_dir, i32 side);
static void
brain_face_random(program_state *program, world_entity *ent);

static void
player_state_idle_run(program_state *program, program_input *input, world_entity *ent, f32 dt);
static void
player_state_attack0_run(program_state *program, program_input *input, world_entity *ent, f32 dt);
static void
player_state_attack1_run(program_state *program, program_input *input, world_entity *ent, f32 dt);
#define STATE_F(name) name(program_state *program, program_input *input, world_entity *ent, f32 dt)
STATE_F(ent0_state0);
STATE_F(ent0_state1);

static b32 
brain_wait(world_entity *ent, f32 time, f32 dt)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);

	b32 finished = run_timer(&brain->timer0, brain->timer0_total, dt);
	if(finished)
	{
		brain->timer0 = 0;
	}

	return(finished);

}

static b32
brain_timer_finished(world_entity *ent, f32 dt)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);

	b32 finished = run_timer(&brain->timer0, brain->timer0_total, dt);
	if(finished)
	{
		brain->timer0 = 0;
	}
	return(finished);
}

static void
brain_reset_frame_step_for_frame(world_entity *ent)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);

	current_state->frame_step = 0;
	brain->restarted_step = 1;
}

static void
brain_reset_frame_step(world_entity *ent)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);

	current_state->frame_step = 0;
}

static b32
brain_player_distance_less_than(program_state *program, world_entity *ent, f32 radius)
{
	//entity_detection_data *nearest_entity = &ent->nearest_entity;
	//detect player!
	b32 trigger_true = 0;
	for(u32 d = 0; d < ent->detections_count; d++)
	{
		cosos_detection *detection = ent->detections + d;
		if(detection->id1->id.id0 == 0)
		{
			trigger_true = (radius * radius) > detection->distance_squared;
			break;
		}
	}

	return(trigger_true);
}


static void
brain_tackle(program_state *program, world_entity *ent)
{
	//aparecer un hitbox
	//mantenerlo vivo en el transcurso de este ataque

}


static void
brain_face_random(program_state *program, world_entity *ent)
{
	random_series *random_series = &program->random_s;

	f32 angle = random_get_f32_between(random_series, -PI, PI);
	f32 angle16 = PI / 16.0f;
	angle = (f32)((i32)(angle / angle16) * angle16);
	ent->looking_direction.x = cos32(angle);
	ent->looking_direction.y = sin32(angle);

}

static void
brain_spawn_sweep(program_state *program, world_entity *ent, f32 distance, f32 z, vec3 size, b32 relative_to_dir, i32 side, f32 start_angle)
{
	game_body *body = ent->body;

	entity_spawn_parameters spawn_params = {0};
	spawn_params.flags = coso_damage | 
		coso_body | 
		coso_traversable | 
		coso_no_gravity |
		coso_move_with_parent;

	spawn_params.parent = ent;
	spawn_params.shape = shape_cube;
	spawn_params.size = size;
	spawn_params.dmg_frame_time = 1;
	spawn_params.frames_before_hit = 10;
	spawn_params.sweep = 1;
	f32 a2 = start_angle;
	spawn_params.end_angle = PI / 1.4f;
	spawn_params.start_angle = a2;
	spawn_params.distance = distance;
	spawn_params.side = side;
	if(relative_to_dir)
	{
		f32 angle = arctan232(ent->looking_direction.x, ent->looking_direction.y);
		a2 = spawn_params.start_angle;
		vec3 bp = {0, distance};
		vec2 p2 = {cos32(angle - a2), sin32(angle - a2)};
		vec3 p = { 0, 0, z};
		//rotate
		p.x = (bp.x * p2.x) + (bp.y * p2.y);
		p.y = (bp.x * p2.y) + (bp.y * p2.x);

		spawn_params.relative_p = p;

		p = vec3_add(ent->body->p, p);
		spawn_params.p = p;
	}
	else
	{
		vec3 p = {0, 0, z};
		spawn_params.p = vec3_add(ent->body->p, p);
	}

	request_entity_spawn(program, spawn_params);
}

static void
brain_spawn_front(program_state *program, world_entity *ent, f32 distance, f32 z, vec3 size, b32 relative_to_dir, i32 side)
{
	game_body *body = ent->body;

	entity_spawn_parameters spawn_params = {0};
	spawn_params.flags = coso_damage | 
		coso_body | 
		coso_traversable | 
		coso_no_gravity |
		coso_move_with_parent;

	spawn_params.parent = ent;
	spawn_params.shape = shape_cube;
	spawn_params.size = size;
	spawn_params.dmg_frame_time = 1;
	spawn_params.frames_before_hit = 10;
	spawn_params.sweep = 0;
	spawn_params.distance = distance;
	if(relative_to_dir)
	{
		f32 angle = arctan232(ent->looking_direction.x, ent->looking_direction.y);
		vec3 bp = {0, distance};
		vec2 p2 = {cos32(angle), sin32(angle)};
		vec3 p = { 0, 0, z};
		//rotate
		p.x = (bp.x * p2.x) + (bp.y * p2.y);
		p.y = (bp.x * p2.y) + (bp.y * p2.x);

		spawn_params.relative_p = p;

		p = vec3_add(ent->body->p, p);
		spawn_params.p = p;
	}
	else
	{
		vec3 p = {0, 0, z};
		spawn_params.p = vec3_add(ent->body->p, p);
	}

	request_entity_spawn(program, spawn_params);
}

static void
brain_add_speed_to_facing(world_entity *ent, f32 speed, f32 dt)
{
	game_body *body = ent->body;

	vec2 speed_angles = vec2_normalize(
			ent->looking_direction);

	f32 dt_s = DELTA_SPEED(dt);
	body->v.y += (speed * speed_angles.y) * dt_s;
	body->v.x += (speed * speed_angles.x) * dt_s;

}

static void
brain_sub_speed_to_facing(world_entity *ent, f32 speed, f32 dt)
{
	game_body *body = ent->body;

	vec2 speed_angles = vec2_normalize(
			ent->looking_direction);

	f32 dt_s = DELTA_SPEED(dt);
	body->v.y -= (speed * speed_angles.y) * dt_s;
	body->v.x -= (speed * speed_angles.x) * dt_s;

}

static b32
brain_frame_step_between(world_entity *ent, u32 frame_at, u32 frame_duration)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);
	
	u32 fs = current_state->frame_step;
	u32 fa = frame_at;
	u32 fd = frame_duration;
	b32 trigger_true = fs >= fa && fs < (fa + fd);
	return(trigger_true);
}

static void
brain_switch_state(world_entity *ent, u32 state_index)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain_current_state(brain);
	//reset current line
	current_state->current_line = 0;
	current_state->timer = 0;
	current_state->frame_step = 0;
	//reset action timers
	brain->first_running_action = 0;
	brain->action_is_running = 0;
	brain->current = state_index;
	brain->state_transition = 2;

	current_state->step = 0;
	current_state->step_transition++;
//	state_transition = 1;

}



static void
run_brain(program_state *program,program_input *input, world_entity *ent, f32 dt)
{

	game_body *body = ent->body;
	state_main *brain = ent->brain;
	state_node *current_state = brain->states + brain->current;
	brain->state_transition = brain->state_transition ? brain->state_transition - 1 : 0;
	brain->restarted_step = 0;
	current_state->step_transition = 0;

	if(run_timer(&current_state->timer, 1000, dt))
	{
		current_state->frame++;
		current_state->timer = 0;
	}

	//
	switch(brain->type)
	{
		case brain_player:
			{
				//
				switch(brain->current)
				{
					case 0: player_state_idle_run(program, input, ent, dt); break;
					case 2: player_state_attack0_run(program, input, ent, dt); break;
					case 3: player_state_attack1_run(program, input, ent, dt); break;
					default:
						{
							brain_switch_state(ent, 0);
							Assert(0);
						}break;
				}break;
				//
			}break;
		case brain_ent0:
			{
				//switch(brain->current)
				//{
				//	case 0: ent0_state0(program, input, ent, dt); break;
				//}
				ent0_state0(program, input, ent, dt);
			}break;
	}
	//

	if(!brain->state_transition && !brain->restarted_step)
	{
		current_state->frame_step++;
	}
}

static void
player_state_idle_run(program_state *program,program_input *input, world_entity *ent, f32 dt)
{
	game_body *body = ent->body;

	brain_input_move(ent, input, dt);
	//just jump
	if(body->grounded && input_pressed(input->jump))
	{
		body->v.z += body->z_speed;
	}
	//attack
	if(input_pressed(input->attack))
	{
		brain_switch_state(ent, 2);
	}
}

static void
player_state_attack0_run(program_state *program,program_input *input, world_entity *ent, f32 dt)
{
	if(brain_frame_step_between(ent, 0, 1))
	{
		//spawn sweep
		//add speed
		brain_spawn_sweep(program, ent, 20, 4, V3(5, 5, 5), 1, 1, PI * -0.5f);
		brain_add_speed_to_facing(ent, 2.0f, dt);
	}
	//switch to attack 2 between these frames
	if(brain_frame_step_between(ent, 5, 5) && input_pressed(input->attack))
	{
		brain_switch_state(ent, 3);
	}
	if(brain_frame_step_between(ent, 10, 1))
	{
		brain_switch_state(ent, 0);
	}
}

static void
player_state_attack1_run(program_state *program,program_input *input, world_entity *ent, f32 dt)
{
	if(brain_frame_step_between(ent, 0, 1))
	{
		//spawn sweep
		//add speed
		brain_spawn_sweep(program, ent, 20, 4, V3(5, 5, 5), 1, -1, PI * .5f);
		brain_add_speed_to_facing(ent, 2.0f, dt);
	}
	if(brain_frame_step_between(ent, 5, 1))
	{
		brain_switch_state(ent, 0);
	}
}

static void
brain_generate_timer_between(program_state *program, world_entity *ent, f32 start, f32 end)
{
	state_main *brain = ent->brain;
	random_series *random = &program->random_s;
	f32 n = random_get_f32_between(random, start, end);

	//in miliseconds
	brain->timer0_total = n * 1000;
	brain->timer0 = 0;
}

static void
brain_generate_timer(world_entity *ent, f32 time)
{
	state_main *brain = ent->brain;
	f32 n = time;

	//in miliseconds
	brain->timer0_total = n * 1000;
	brain->timer0 = 0;
}

static void
brain_advance_step(world_entity *ent)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain->states + brain->current;
	current_state->step++;
	current_state->step_transition++;
}

static void
brain_round_player(program_state *program, world_entity *ent)
{
	world_entity *player = program->player_entity;
	if(player)
	{
		game_body *body0 = player->body;
		game_body *body1 = ent->body;
		vec3 distance_vector = vec3_sub(body0->p, body1->p);
		f32 dist = vec3_inner_squared(distance_vector);
	}
}

static void
brain_face_player(program_state *program, world_entity *ent)
{
	if(program->player_entity)
	{
		world_entity *player = program->player_entity;
		vec3 p1 = player->body->p;
		vec3 p0 = ent->body->p;

		vec2 face_dir = {
			p1.x - p0.x,
			p1.y - p0.y};
		face_dir = vec2_normalize(face_dir);
		ent->looking_direction = face_dir;
	}

}

static void
brain_move_to_facing(world_entity *ent, f32 dt)
{
//	b32 finished = run_timer(&sdo->line_timer, sdo->time_total, dt);
//	if(!finished)
//	{
//		keep_reading = 0;
//	}
//	else
//	{
//		sdo->line_timer= 0;
//		break;
//	}

	f32 dt_s = DELTA_SPEED(dt);
	vec2 moving_dir = ent->looking_direction;
	vec2 speed_angles = vec2_normalize(
			ent->looking_direction);
	game_body *body = ent->body;
	f32 speed = ent->body->speed;
	if(moving_dir.y > 0)
	{
		body->v.y += (speed * speed_angles.y) * dt_s;

	}
	else if(moving_dir.y < 0)
	{
		body->v.y += (speed * speed_angles.y) * dt_s;

	}
	if(moving_dir.x < 0)
	{
		body->v.x += (speed * speed_angles.x) * dt_s;

	}
	else if(moving_dir.x > 0)
	{
		body->v.x += (speed * speed_angles.x) * dt_s;

	}
}

STATE_F(ent0_state0)
{
	state_main *brain = ent->brain;
	state_node *current_state = brain->states + brain->current;

	switch(brain->current)
	{
		//idle
		case 0:
			{
				if(brain_frame_step_between(ent, 0, 1))
				{
					brain_face_random(program, ent);
				}
				if(brain_frame_step_between(ent, 100, 1))
				{
					brain_face_random(program, ent);
				}
				if(brain_frame_step_between(ent, 200, 1))
				{
					brain_face_random(program, ent);
					brain_reset_frame_step_for_frame(ent);
				}
				if(brain_player_distance_less_than(program, ent, 100))
				{
					brain_switch_state(ent, 1);
				}
			}break;
			//round player
		case 1:
			{
				if(current_state->step == 0)
				{
					if(current_state->step_transition || brain->state_transition)
					{
						brain_generate_timer_between(program, ent, 1.0f, 2.0f);
					}

					if(!brain_timer_finished(ent, dt))
					{
						//brain_round_player(program, ent);
						brain_face_player(program, ent);
					}
					else
					{
						//game_body *body = ent->body;
						//body->v.z += 2;
						//brain_switch_state(ent, 0);
						brain_advance_step(ent);
						brain_reset_frame_step(ent);
					}
				}
				if(current_state->step == 1)
				{
					if(brain_frame_step_between(ent, 0, 1))
					{
//						brain_sub_speed_to_facing(ent, 5.0f, dt);
					}
					if(brain_frame_step_between(ent, 5, 1))
					{
						//jump sideways towards player
						game_body *body0 = program->player_entity->body;
						game_body *body1 = ent->body;
						vec3 dv = vec3_sub(body0->p, body1->p);
						i32 side_x = SIGN(dv.x);
						i32 side_y = SIGN(dv.y);
						f32 speed = 5.0f;
						if(ABS(dv.x) < ABS(dv.y))
						{
							ent->body->v.x += side_x * speed * DELTA_SPEED(dt);
						}
						else
						{
							ent->body->v.y += side_y * speed * DELTA_SPEED(dt);
						}
					}
					if(brain_frame_step_between(ent, 20, 1))
					{
						f32 advance = random_f32(&program->random_s);
						if(!brain_player_distance_less_than(program, ent, 100))
						{
							brain_switch_state(ent, 0);
						}
						else if(advance >= 0.8f)
						{
							ent->body->v.z += 2.0f;
							brain_advance_step(ent);
							brain_reset_frame_step_for_frame(ent);
						}
						else
						{
							brain_reset_frame_step_for_frame(ent);
						}
					}
				}

				if(current_state->step == 2)
				{
					if(brain_frame_step_between(ent, 0, 1))
					{
						brain_face_player(program, ent);
					}
					if(brain_frame_step_between(ent, 0, 20))
					{
						brain_move_to_facing(ent, dt);
					}
					else
					{
						brain_switch_state(ent, 0);
					}
				}
			}break;
	}
}

static void
brain_input_move(world_entity *ent, program_input *input, f32 dt)
{

	game_body *body = ent->body;
	//move by input and speed, might use another value ?
	//if keyboard
	vec2 moving_dir = {0};
	f32 dt_s = DELTA_SPEED(dt);
	if(input_down(input->up))
	{
		moving_dir.y = 1;
	}
	else if(input_down(input->down))
	{
		moving_dir.y = -1;
	}
	if(input_down(input->left))
	{
		moving_dir.x = -1;
	}
	else if(input_down(input->right))
	{
		moving_dir.x = 1;
	}
	//Also gives the sign
	vec2 speed_angles = vec2_normalize(
			moving_dir);
	f32 speed = ent->body->speed;

	if(moving_dir.y > 0)
	{
		body->v.y += (speed * speed_angles.y) * dt_s;
		ent->looking_direction.y = 1;

	}
	else if(moving_dir.y < 0)
	{
		body->v.y += (speed * speed_angles.y) * dt_s;
		ent->looking_direction.y = -1;

	}
	if(moving_dir.x < 0)
	{
		body->v.x += (speed * speed_angles.x) * dt_s;
		ent->looking_direction.x = -1;

	}
	else if(moving_dir.x > 0)
	{
		body->v.x += (speed * speed_angles.x) * dt_s;
		ent->looking_direction.x = 1;

	}
	//
	if(moving_dir.x != 0 || moving_dir.y != 0)
	{
		ent->looking_direction = vec2_normalize(moving_dir);
	}
}

static b32 
brain_read_trigger(
		world_entity *ent,
		state_node *current_state,
		state_trigger *trigger,
		program_input *input)
{

	u32 state_frame = current_state->frame;
	f32 state_timer = current_state->timer;

//	state_trigger *trigger = triggers + sdo->triggers_at + t;
	game_body *body = ent->body;
	b32 trigger_true = 0;
	switch(trigger->type)
	{
		case state_node_frame_timer_comp:
			{
				if(trigger->eq && trigger->more) trigger_true = state_frame >= trigger->frame_at;
				else if(trigger->more) trigger_true = state_frame > trigger->frame_at;
				else trigger_true = state_frame == trigger->frame_at;
			}break;
		case bt_frame_step_comp:
			{
				u32 fs = current_state->frame_step;
				u32 fa = trigger->frame_at;

				if(trigger->eq && trigger->more)
					trigger_true = fs >= fa;
				else if(trigger->more) 
					trigger_true = fs > fa;
				else 
					trigger_true = fs == fa;
			}break;
		case bt_frame_step_duration:
			{
				u32 fs = current_state->frame_step;
				u32 fa = trigger->frame_at;
				u32 fd = trigger->frame_duration;
				trigger_true = fs >= fa && fs < (fa + fd);

			}break;
		case state_node_moved:
			{
				//detect analog value or arrows pressed
				//if keyboard instead, detect up, down, left or right
				trigger_true |= input_down(input->up);
				trigger_true |= input_down(input->down);
				trigger_true |= input_down(input->left);
				trigger_true |= input_down(input->right);
			}break;
		case state_node_attack_pressed:
			{
				//detect analog value or arrows pressed
				//if keyboard instead, detect up, down, left or right
				trigger_true |= input_pressed(input->attack);
			}break;
		case state_node_jump_pressed:
			{
				trigger_true |= input_pressed(input->jump);
			}break;
		case state_node_on_air:
			{
				trigger_true |= !(body->grounded);
				if(body->grounded)
				{
					int s = 0;
				}
			}break;
		case state_node_timer_at:
			{
				//time 0 should probably be on trigger
				trigger_true |= (state_timer >= trigger->time0);
			}break;
		case bt_player_distance_less_than:
			{
				//entity_detection_data *nearest_entity = &ent->nearest_entity;
				//trigger_true |= ((nearest_entity->distance_squared) < (trigger->radius * trigger->radius));
				//detect player!
				for(u32 d = 0; d < ent->detections_count; d++)
				{
					cosos_detection *detection = ent->detections + d;
					if(detection->id1->id.id0 == 0)
					{
						trigger_true |= (trigger->radius * trigger->radius) > detection->distance_squared;
						break;
					}
				}
#if 0
				Assert(program->player_entity);
				world_entity *player = program->player_entity;
				vec3 p0 = player->body.p;
				vec3 p1 = body->p;
				vec3 dist = vec3_sub(p1, p0);
				f32 distance = vec3_inner_squared(dist, dist);
				f32 r0 = distance * distance;
				f32 r1 = trigger->radius * trigger->radius;

				trigger_true |= r0 < r1;
#endif
			}break;
		default:
			{
				trigger_true = 1;
			}break;
	}
	if(trigger->not)
	{
		trigger_true = !trigger_true;
	}
	//execute_do &= trigger_true;
	return(trigger_true);
}


static b32
run_timer(f32 *timer, f32 total_ms, f32 dt)
{
	b32 loop = 0;
//	total_ms = (total_ms * target_ff) / target_max;
    *timer += dt * 1000;
    if(*timer > total_ms)
    {
        (*timer) = 0;
		loop = 1;
    }
	return(loop);
}

static state_action_line *
brain_add_action_line(
		state_main *brain,
		state_action *action,
		state_action_line_type type)
{
	action->action_lines_count++;
	state_action_line *result = brain->action_lines + brain->action_lines_count++;
	result->type = type;
	result->type = type;

	return(result);
}

static state_action *
brain_add_action(
		state_main *brain)
{
	state_action *action = brain->actions + brain->action_count++;
	action->action_lines_at = brain->action_lines_count;
	return(action);
}

static state_node *
brain_add_state(state_main *brain)
{
	state_node *new_state = brain->states + brain->state_count++;
	new_state->state_do_at = brain->state_line_count;

	return(new_state);
}

static state_line *
brain_add_state_line(
		state_main *brain,
		state_node *state)
{
	brain->state_line_count++;
	state_line *result = brain->state_lines + state->state_do_at + state->state_line_count++;
	result->triggers_at = brain->triggers_count;
	return(result);
}

static state_trigger *
brain_add_line_condition(
		state_main *brain,
		state_line *sdo,
		state_do_flags type)
{
	state_trigger *result = brain->triggers + sdo->triggers_at + sdo->trigger_count++;
	result->type = type;
	brain->triggers_count++;
	return(result);
}



static void
allocate_brains(program_state *program, memory_area *main_area)
{
	u32 allocation_size = MEGABYTES(1);
	program->states_memory = memory_blocks_create(allocation_size, memory_area_push_size(main_area, allocation_size));

}

static state_main *
create_brain(program_state *program, state_definition definition)
{
	u32 memory_needed = 
		sizeof(state_main) +
		definition.state_count * sizeof(state_node) +
		definition.action_count * sizeof(state_action) +
		definition.action_lines_count * sizeof(state_action_line) +
		definition.state_line_count * sizeof(state_line) +
		definition.triggers_count * sizeof(state_trigger);

	memory_block *block = memory_block_get(&program->states_memory, memory_needed);
	state_main *result = memory_block_push_struct(block, state_main);
	//memory_clear(result, sizeof(*result));
	result->memory_block = block;
	result->states       = memory_block_push_array(block, state_node, definition.state_count);
	result->actions      = memory_block_push_array(block, state_action, definition.action_count);
	result->action_lines = memory_block_push_array(block, state_action_line, definition.action_lines_count);
	result->state_lines  = memory_block_push_array(block, state_line, definition.state_line_count);
	result->triggers     = memory_block_push_array(block, state_trigger, definition.triggers_count);

	result->state_count        = definition.state_count;
	result->action_count       = definition.action_count;
	result->action_lines_count = definition.action_lines_count;
	result->state_line_count   = definition.state_line_count;
	result->triggers_count     = definition.triggers_count;

	if(!program->first_brain)
	{
		program->first_brain = result;
	}

	return(result);
}

static void
remove_brain(program_state *program, state_main *brain)
{
	memory_block_free(&program->states_memory, brain->memory_block);
}

static game_brain_creation 
begin_brain_creation(program_state *program)
{
	temporary_area temp_area = temporary_area_begin(program->area);
	game_brain_creation bc = {0};
	bc.temp_area = temp_area;
	state_main *brain = &bc.brain; 
	brain->states       = memory_area_clear_and_push_array(program->area, state_node, 100);
	brain->actions      = memory_area_clear_and_push_array(program->area, state_action, 100);
	brain->action_lines = memory_area_clear_and_push_array(program->area, state_action_line, 100);
	brain->state_lines  = memory_area_clear_and_push_array(program->area, state_line, 100);
	brain->triggers     = memory_area_clear_and_push_array(program->area, state_trigger, 100);
	return(bc);
}

static state_main *
end_brain_creation(program_state *program, game_brain_creation bc)
{
	state_definition def = {0};

	state_main *brain = &bc.brain;
	def.state_count        = brain->state_count;
	def.action_count       = brain->action_count;
	def.action_lines_count = brain->action_lines_count;
	def.state_line_count   = brain->state_line_count;
	def.triggers_count     = brain->triggers_count;

	state_main *new_brain = create_brain(program, def);
	memory_copy(brain->states, new_brain->states, brain->state_count * sizeof(*brain->states));
	memory_copy(brain->actions, new_brain->actions, brain->action_count * sizeof(*brain->actions));
	memory_copy(brain->action_lines, new_brain->action_lines, brain->action_lines_count * sizeof(*brain->action_lines));
	memory_copy(brain->state_lines, new_brain->state_lines, brain->state_line_count * sizeof(*brain->state_lines));
	memory_copy(brain->triggers, new_brain->triggers, brain->triggers_count * sizeof(*brain->triggers));

	temporary_area_end(&bc.temp_area);
	new_brain->type = brain->type;
	return(new_brain);
}


static void
brains_updateV1(
		program_state *program,
		program_input *input,
		world_entity *ent,
		f32 dt)
{
	game_body *body = ent->body;
	state_main *brain = ent->brain;
	state_node *current_state = brain->states + brain->current;
	state_trigger *triggers = brain->triggers;
	random_series *random_series = &program->random_s;
	//might make this a parameter
	stream_data *info_stream = 0;//&program_state->info_stream;


	f32 dt_s = DELTA_SPEED(dt);
	//	ent->speed = 1.2f;

	/*

	   state
	   |-> line0
	   |   line1 => execute_action => index
	   |   ...
	   |
	   |
	   action[index]
	   |-> line0
	   |   line1
	   |   {
	   |      line2
	   |	   line3
	   |   }

*/
	//read state lines
	if(1)
	{
		if(run_timer(&current_state->timer, 1000, dt))
		{
			current_state->frame++;
			current_state->timer = 0;
		}
		brain->action_transition_count = 0;
		u32 index = current_state->current_line;
		b32 keep_reading = brain->state_line_count != 0;
		b32 switched_action = 0;
		b32 state_transition = 0;
		b32 restarted_step = 0;
		while(keep_reading)
		{
			u32 line_advance_count = 1;
			b32 inside_scope = 0;
			u32 scope_count = 0;
			u32 local_index = 0;

			//read dos and their conditions
			state_line *sdo = brain->state_lines + current_state->state_do_at + 
				current_state->current_line;
			b32 execute_do = 1;
			if(sdo->type == do_condition)
			{
				//conditions
				for(u32 t = 0; t < sdo->trigger_count; t++)
				{
					state_trigger *trigger = triggers + sdo->triggers_at + t;
					b32 trigger_true = brain_read_trigger(ent, current_state, trigger, input);

					execute_do &= trigger_true;
				}
				if(!execute_do)
				{
					line_advance_count += sdo->next_line_count;
				}
			}
			else if(sdo->type == do_switch_state)
			{
				//conditions
				for(u32 t = 0; t < sdo->trigger_count; t++)
				{
					state_trigger *trigger = triggers + sdo->triggers_at + t;
					b32 trigger_true = brain_read_trigger(ent, current_state, trigger, input);

					execute_do &= trigger_true;
				}
				if(execute_do)
				{
					//reset current line
					current_state->current_line = 0;
					current_state->timer = 0;
					current_state->frame_step = 0;
					//reset action timers
					brain->first_running_action = 0;
					brain->action_is_running = 0;
					//set new state
					keep_reading = 0;
					brain->current = sdo->state_index;
					line_advance_count = 0;
					state_transition = 1;
				}
			}
			else
			{
				switch(sdo->type)
				{
					case do_spawn_hurtbox:
						{
							//hurtbox life time
//							line0->time0 = 10.0f;
//							//relative position from looking_direction
//							line0->p = V3(0, 20, 4);
//							line0->relative_to_dir = 1;
//							line0->size = V3(5, 5, 5);
							entity_spawn_parameters spawn_params = {0};
							spawn_params.flags = coso_damage | 
								coso_body | 
								coso_lifetime | 
								coso_traversable | 
								coso_no_gravity |
								coso_move_with_parent;
							spawn_params.parent = ent;
							spawn_params.shape = shape_cube;
							spawn_params.life_time = sdo->time0;
							spawn_params.size = sdo->size;
//							spawn_params.speed = sdo->speed;
							if(sdo->relative_to_dir)
							{
								f32 angle = arctan232(ent->looking_direction.x, ent->looking_direction.y);
								vec3 bp = sdo->p;
								vec2 p2 = {cos32(angle), sin32(angle)};
								vec3 p = { 0, 0, sdo->p.z};
								//rotate
								p.x = (bp.x * p2.x) + (bp.y * p2.y);
								p.y = (bp.x * p2.y) + (bp.y * p2.x);

								spawn_params.relative_p = p;

								p = vec3_add(ent->body->p, p);
								spawn_params.p = p;
							}
							else
							{
								spawn_params.p = vec3_add(ent->body->p, sdo->p);
							}

							request_entity_spawn(program, spawn_params);
						}break;
					case do_spawn_melee_hurtbox:
						{
							//hurtbox life time
//							line0->time0 = 10.0f;
//							//relative position from looking_direction
//							line0->p = V3(0, 20, 4);
//							line0->relative_to_dir = 1;
//							line0->size = V3(5, 5, 5);
							u32 id1 = generate_entity_id(123124);
							world_entity *attack = get_child_with_id1(ent, id1);
							if(attack)
							{
								attack->life_time++;
							}
							else
							{
								entity_spawn_parameters spawn_params = {0};
								spawn_params.flags = coso_damage | 
									coso_body | 
									coso_frame_lifetime | 
									coso_traversable | 
									coso_no_gravity |
									coso_move_with_parent;
								spawn_params.parent = ent;
								spawn_params.shape = shape_cube;
								spawn_params.life_time = sdo->time0;
								spawn_params.size = sdo->size;
								spawn_params.id1 = id1;
								spawn_params.dmg_frame_time = 1;
								spawn_params.frames_before_hit = 10;
								//							spawn_params.speed = sdo->speed;
								if(sdo->relative_to_dir)
								{
									f32 angle = arctan232(ent->looking_direction.x, ent->looking_direction.y);
									vec3 bp = sdo->p;
									vec2 p2 = {cos32(angle), sin32(angle)};
									vec3 p = { 0, 0, sdo->p.z};
									//rotate
									p.x = (bp.x * p2.x) + (bp.y * p2.y);
									p.y = (bp.x * p2.y) + (bp.y * p2.x);

									spawn_params.relative_p = p;

									p = vec3_add(ent->body->p, p);
									spawn_params.p = p;
								}
								else
								{
									spawn_params.p = vec3_add(ent->body->p, sdo->p);
								}

								request_entity_spawn(program, spawn_params);
							}
						}break;
					case do_spawn_sweep:
						{
							//hurtbox life time
//							line0->time0 = 10.0f;
//							//relative position from looking_direction
//							line0->p = V3(0, 20, 4);
//							line0->relative_to_dir = 1;
//							line0->size = V3(5, 5, 5);
							u32 id1 = generate_entity_id(123124);
							{
								entity_spawn_parameters spawn_params = {0};
								spawn_params.flags = coso_damage | 
									coso_body | 
									coso_traversable | 
									coso_no_gravity |
									coso_move_with_parent;

								spawn_params.parent = ent;
								spawn_params.shape = shape_cube;
								spawn_params.life_time = sdo->time0;
								spawn_params.size = sdo->size;
								spawn_params.id1 = id1;
								spawn_params.dmg_frame_time = 1;
								spawn_params.frames_before_hit = 10;
								spawn_params.sweep = 1;
								f32 a2 = PI / 4;
								spawn_params.end_angle = PI + a2;
								spawn_params.start_angle = a2;
								spawn_params.distance = sdo->distance;
								//							spawn_params.speed = sdo->speed;
								if(sdo->relative_to_dir)
								{
									f32 angle = arctan232(ent->looking_direction.x, ent->looking_direction.y);
//									vec2 bp = {cos32(PI/4), sin32(PI/4)};
//									bp = vec2_scale(bp, sdo->distance);
									a2 = spawn_params.start_angle;
									vec3 bp = {0, sdo->distance};
									vec2 p2 = {cos32(angle + a2), sin32(angle + a2)};
									spawn_params.end_angle += angle;
									vec3 p = { 0, 0, sdo->p.z};
									//rotate
									p.x = (bp.x * p2.x) + (bp.y * p2.y);
									p.y = (bp.x * p2.y) + (bp.y * p2.x);

									spawn_params.relative_p = p;

									p = vec3_add(ent->body->p, p);
									spawn_params.p = p;
								}
								else
								{
									spawn_params.p = vec3_add(ent->body->p, sdo->p);
								}

								request_entity_spawn(program, spawn_params);
							}
						}break;
					case do_face_dir:
						{
							//sdo
							vec2 *ld = &ent->looking_direction;
							switch(sdo->face_dir)
							{
								case saf_s : ld->x = 0; ld->y = -1; break;
								case saf_se: ld->x = 1; ld->y = -1; break;
								case saf_e : ld->x = 1; ld->y = 0; break;
								case saf_ne: ld->x = 1; ld->y = 1; break;
								case saf_n : ld->x = 0; ld->y = 1; break;
								case saf_nw: ld->x = -1; ld->y = 1; break;
								case saf_w : ld->x = -1; ld->y = 0; break;
								case saf_sw: ld->x = -1; ld->y = -1; break;
							}
						}break;
					case do_face_player:
						{
							if(program->player_entity)
							{
								world_entity *player = program->player_entity;
								vec3 p1 = player->body->p;
								vec3 p0 = body->p;

								vec2 face_dir = {
								p1.x - p0.x,
								p1.y - p0.y};
								face_dir = vec2_normalize(face_dir);
								ent->looking_direction = face_dir;
							}
						}break;
					case do_wait:
						{
							b32 finished = run_timer(&sdo->line_timer, sdo->time_total, dt);
							if(!finished)
							{
								keep_reading = 0;
							}
							else
							{
								sdo->line_timer = 0;
							}

						}break;
					case do_face_random_dir:
						{
							f32 angle = random_get_f32_between(random_series, -PI, PI);
							f32 angle16 = PI / 16.0f;
							angle = (f32)((i32)(angle / angle16) * angle16);
							ent->looking_direction.x = cos32(angle);
							ent->looking_direction.y = sin32(angle);
						}
						break;
					case do_face_target:
						{
							entity_detection_data *nearest_entity = &ent->nearest_entity;
							vec3 dir = vec3_normalize(vec3_sub(nearest_entity->position, ent->body->p));
							f32 angle = arctan232(dir.y, dir.x);
							f32 angle16 = PI / 16.0f;
							angle = (f32)((i32)(angle / angle16) * angle16);
							ent->looking_direction.x = cos32(angle);
							ent->looking_direction.y = sin32(angle);

							//						ent->looking_direction.x = dir.x;
							//						ent->looking_direction.y = dir.y;
						}break;
					case do_set_speed:
						{
							vec3 *vptr = &ent->body->v;
							vec3 v = sdo->vec;

							vptr->x = v.x;
							vptr->y = v.y;
							vptr->z = v.z;
						}break;
					case do_add_speed:
						{
							vec3 *vptr = &ent->body->v;
							vec3 speed_v = sdo->vec;

							vptr->x += dt_s * speed_v.x;
							vptr->y += dt_s * speed_v.y;
							vptr->z += dt_s * speed_v.z;
						}break;
					case do_move_to_facing:
						{
							b32 finished = run_timer(&sdo->line_timer, sdo->time_total, dt);
							if(!finished)
							{
								keep_reading = 0;
							}
							else
							{
								sdo->line_timer= 0;
								break;
							}

							vec2 moving_dir = ent->looking_direction;
							vec2 speed_angles = vec2_normalize(
									ent->looking_direction);
							game_body *body = ent->body;
							f32 speed = ent->body->speed;
							if(moving_dir.y > 0)
							{
								body->v.y += (speed * speed_angles.y) * dt_s;

							}
							else if(moving_dir.y < 0)
							{
								body->v.y += (speed * speed_angles.y) * dt_s;

							}
							if(moving_dir.x < 0)
							{
								body->v.x += (speed * speed_angles.x) * dt_s;

							}
							else if(moving_dir.x > 0)
							{
								body->v.x += (speed * speed_angles.x) * dt_s;

							}
						}break;
					case do_add_speed_to_facing:
						{

							vec2 speed_angles = vec2_normalize(
									ent->looking_direction);
							game_body *body = ent->body;
							f32 speed = sdo->speed;

							body->v.y += (speed * speed_angles.y) * dt_s;
							body->v.x += (speed * speed_angles.x) * dt_s;
						}break;
					case do_move:
						{
							//move by input and speed, might use another value ?
							//if keyboard
							vec2 moving_dir = {0};
							if(input_down(input->up))
							{
								moving_dir.y = 1;
							}
							else if(input_down(input->down))
							{
								moving_dir.y = -1;
							}
							if(input_down(input->left))
							{
								moving_dir.x = -1;
							}
							else if(input_down(input->right))
							{
								moving_dir.x = 1;
							}
							//Also gives the sign
							vec2 speed_angles = vec2_normalize(
									moving_dir);
							f32 speed = ent->body->speed;

							if(moving_dir.y > 0)
							{
								body->v.y += (speed * speed_angles.y) * dt_s;
								ent->looking_direction.y = 1;

							}
							else if(moving_dir.y < 0)
							{
								body->v.y += (speed * speed_angles.y) * dt_s;
								ent->looking_direction.y = -1;

							}
							if(moving_dir.x < 0)
							{
								body->v.x += (speed * speed_angles.x) * dt_s;
								ent->looking_direction.x = -1;

							}
							else if(moving_dir.x > 0)
							{
								body->v.x += (speed * speed_angles.x) * dt_s;
								ent->looking_direction.x = 1;

							}
							//
							if(moving_dir.x != 0 || moving_dir.y != 0)
							{
								ent->looking_direction = vec2_normalize(moving_dir);
							}
						}break;
					case do_jump:
						{
							body->v.z += body->z_speed;
						}break;
					case do_switch_animation:
						{
							model_switch_animation(
									&ent->model, sdo->animation_index);
						}break;
					case do_reset_frame_step:
						{
							current_state->frame_step = 0;
							restarted_step = 1;
						}break;

				}
				//for now, always advance the current line
			}
			if(!state_transition)
			{
				current_state->current_line += line_advance_count;
				//loop
				if(current_state->current_line >= current_state->state_line_count)
				{
					keep_reading = 0;
					current_state->current_line = 0;
				}
			}
			current_state->current_line = current_state->current_line >= 
				current_state->state_line_count ? 0 : current_state->current_line;
		}
		//only advance a step if not transitioning
		if(!state_transition && !restarted_step)
		{
			current_state->frame_step++;
		}
	}

}

static void
brains_update(program_state *program, program_input *input, world_entity *ent,f32 dt)
{
	brains_updateV1(program, input, ent, dt);
}
