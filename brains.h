typedef enum{
	state_node_null             = 0,
	state_node_random_factor    = 1,
	state_node_on_air           = 4,
	state_node_timer_at         = 5,
	bt_player_distance_less_than  = 6,
	state_node_moved            = 10,
	state_node_jump_pressed     = 11,
	state_node_attack_pressed   = 12,
	state_node_frame_timer_comp = 13,
	bt_frame_step_comp = 14,
	bt_frame_step_duration = 15,

	state_node_count
}state_trigger_type;

//c0 && c1 || c2
//c0 || c1 && c2
//(c0->c1) || c2
//next == or
//first == and
typedef enum{
	al_wait = 0,
	al_spawn_hurtbox       = 1,
	al_switch_animation    = 2,
	al_move_to_facing      = 3,
	al_face_random_dir     = 4,
	al_face_target         = 5,
	al_face_dir            = 6,
	al_face_player         = 7,
	al_spawn_melee_hurtbox = 8,
	al_add_speed = 20,
	//input
	al_move = 10,
	al_jump = 11,
}state_action_line_type;

typedef enum{
	brain_player,
	brain_ent0,
}brain_type;

static void
state_action_line_name(u8 *dest, u32 dest_size, state_action_line_type type)
{
	u8 *name = 0;
	switch(type)
	{
		case al_wait:             name = "wait"; break;
		case al_spawn_hurtbox:    name = "spawn_hurtbox"; break;
		case al_switch_animation: name = "switch_animation"; break;
		case al_move_to_facing:   name = "move_to_facing"; break;
		case al_face_random_dir:  name = "face_random_dir"; break;
		case al_face_target:      name = "face_target"; break;
		case al_face_dir:         name = "face_dir"; break;
		case al_move:             name = "move"; break;
		case al_jump:             name = "jump"; break;
		default:                  name = "unknown"; break;
	}
	memory_copy(name, dest, dest_size);
}

typedef enum{
	do_run_action = 0,
	do_switch_state = 1,
	do_condition = 2,

	do_wait = 10,
	do_spawn_hurtbox = 11,
	do_switch_animation = 12,
	do_move_to_facing = 13,
	do_face_random_dir = 14,
	do_face_target = 15,
	do_face_dir = 16,
	do_face_player = 17,
	do_add_speed = 20,
	do_set_speed = 21,
	do_spawn_melee_hurtbox = 22,
	do_add_speed_to_facing = 23,
	do_spawn_sweep = 24,
	do_reset_frame_step = 30,
	do_move = 100,
	do_jump = 101,
}state_do_flags;

typedef enum{
	saf_s,
	saf_se,
	saf_e,
	saf_ne,
	saf_n,
	saf_nw,
	saf_w,
	saf_sw,
}state_action_face_dir;

typedef enum{
	stc_eq,
	stc_more,
	stc_more_eq
}state_trigger_comparison;

typedef struct{
     u16 triggers_at;
	 u16 trigger_count;

	 b16 active;
	 u16 state_index;
}brain_reaction;

typedef struct{
	state_do_flags type;
	u16 trigger_count;
	u16 triggers_at;


	//do data
	union{
		u16 state_index;
		b16 relative_to_dir;
	};
	u16 action_index;
	u32 next_line_count;
	f32 time0;
	f32 time1;
	f32 line_timer;
	u16 target_index;
	u16 animation_index;
	f32 time_total;
	union{
		f32 distance;
		f32 speed;
		vec3 vec;
		vec3 p;
	};
	union{
		vec3 vec2;
		vec3 size;
	};
	enum32(state_action_face_dir) face_dir;
}state_line;

typedef struct{
	state_action_line_type type;
	u16 target_index;
	u16 animation_index;
	union{
		f32 life_time;
		f32 time_total;
	};
	vec3 vec;
	enum32(state_action_face_dir) face_dir;
	
}state_action_line;

typedef struct state_action{
	struct state_action *prev;
	struct state_action *next;
	u16 action_lines_at;
	u16 action_lines_count;

	b16 running;
	u16 current_line;
	f32 timer;
	f32 line_timer;
}state_action;

//change entity variables
typedef struct state_trigger{
	state_trigger_type type;
	enum32(state_trigger_comparison) comparison_option;
	void *value;
	f32 dt;

	b16 not;
	b8 eq;
	b8 more;

	f32 radius;
	f32 time0;
	u16 frame_at;
	u16 frame_duration;
}state_trigger;
//if type == distance_less_than: radius = 60
//< ()
//> (with not)
//<= (with eq)
//>= (with not and eq)

typedef struct state_node{
	f32 timer;
	u16 frame;
	//advances every time the state reads the current lines before stopping
	u16 frame_step;

	u16 step;
	u16 step_transition;

	u16 current_action_index;
	b16 run_action;
	u16 action_count;
	u16 actions_at;

	u16 state_line_count;
	u16 state_do_at;

	u32 current_line;
	//the next states after this count only 
	//will get executed if this one is true.
	u32 follow_state_do_count;
}state_node;

typedef struct state_main{
	struct state_main *next;
	struct state_main *prev;

	brain_type type;

	u32 memory_used;
	memory_block *memory_block;

	f32 timer;
	u32 current;

	f32 timer0;
	f32 timer0_total;

	u32 reaction_count;
	brain_reaction *reactions;

	u16 state_count;
	u16 state_index;
	state_node *states;


	u16 action_count;
	state_action *actions;
	state_action *first_running_action;

	u16 action_lines_count;
	state_action_line *action_lines;

	u16 state_line_count;
	state_line *state_lines;

	u16 triggers_count;
	state_trigger *triggers;

	u32 current_action_index;
	b32 action_is_running;

	u32 action_transition_count;
	b16 state_transition;
	b8 restarted_step;
	b8 keep_reading;

	union{
		u64 count;
		struct{
			u8 counts[4];
			
		};
	};

}state_main;

typedef struct{
	u16 state_count;
	u16 action_count;
	u16 action_lines_count;
	u16 state_line_count;
	u16 triggers_count;
}state_definition;








static state_action_line *
test_set_action_line(
		state_main *brain,
		state_action *action,
		state_action_line_type type)
{
	action->action_lines_count++;
	state_action_line *result = brain->action_lines + brain->action_lines_count++;
	result->type = type;

	return(result);
}

static state_action *
test_add_action(
		state_main *brain)
{
	state_action *action = brain->actions + brain->action_count++;
	action->action_lines_at = brain->action_lines_count;
	return(action);
}

static state_line *
test_add_state_line(
		state_main *brain,
		state_node *state)
{
	brain->state_line_count++;
	state_line *result = brain->state_lines + state->state_do_at + state->state_line_count++;
	result->triggers_at = brain->triggers_count;
	return(result);
}

static state_trigger *
test_add_line_condition(
		state_main *brain,
		state_line *sdo)
{
	state_trigger *result = brain->triggers + sdo->triggers_at + sdo->trigger_count++;
	brain->triggers_count++;
	return(result);
}

static state_node *
test_add_state(state_main *brain)
{
	state_node *new_state = brain->states + brain->state_count++;
	new_state->state_do_at = brain->state_line_count;

	return(new_state);
}
