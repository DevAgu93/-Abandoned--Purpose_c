typedef struct{
	temporary_area temp_area;
	u32 line_count;
	game_event *event;
}event_creation;

typedef enum{
	event_condition_null,
	event_condition_player_in_area,
	event_condition_not_map_swapping,
}event_condition_type;
typedef enum{
	event_line_null,
	event_line_map_swap
}event_line_type;

static void
read_events(program_state *program);
static event_creation 
begin_event_creation(program_state *program);
static event_line *
add_line_to_event(event_creation *creation, u32 type);
static game_event * 
end_event_creation(program_state *map, game_world *, event_creation *creation);
static event_condition *
add_condition_to_event(event_creation *creation, u32 type);
static void
activate_event(program_state *program, game_event *event);
static void
deactivate_event(program_state *program, game_event *event);
static void
debug_display_events(program_state *program);

static void
debug_display_events(program_state *program)
{
	game_world *current_map = program->maps + program->current_map_index;
	event_main *main_events = &current_map->events;

	game_event *event = main_events->inactive_events;
}

static event_creation
begin_event_creation(program_state *program)
{
	event_creation creation = {0};
	creation.temp_area = temporary_area_begin(program->area);

	game_event *event = memory_area_clear_and_push_struct(program->area, game_event);
	event->lines = memory_area_push_array(program->area, event_line, 1000);
	event->conditions = memory_area_push_array(program->area, event_condition, 1000);

	creation.event = event;
	return(creation);
}

static game_event * 
end_event_creation(program_state *program, game_world *map, event_creation *creation)
{
	event_main *main_events = &map->events;
	game_event *temp_event = creation->event;

	//get actual event
	game_event *result_event = main_events->events + main_events->event_count;
	memory_clear(result_event, sizeof(*result_event));
	*result_event = *temp_event;
	main_events->event_count++;

	//set line array
	result_event->lines = main_events->lines + main_events->line_count;
	for(u32 l = 0; l < temp_event->line_count; l++)
	{
		result_event->lines[l] = temp_event->lines[l];
	}
	main_events->line_count += temp_event->line_count;

	result_event->conditions = main_events->conditions + main_events->condition_count;
	for(u32 l = 0; l < temp_event->condition_count; l++)
	{
		result_event->conditions[l] = temp_event->conditions[l];
	}
	main_events->condition_count += temp_event->condition_count;

	//set to the inactive list
	deactivate_event(program, result_event);
	temporary_area_end(&creation->temp_area);
	return(result_event);
}

static event_line *
add_line_to_event(event_creation *creation, u32 type)
{
	game_event *event = creation->event;
	event_line *line = event->lines + event->line_count;
	line->type = type;
	event->line_count++;
	return(line);
}

static event_condition *
add_condition_to_event(event_creation *creation, u32 type)
{
	game_event *event = creation->event;
	event_condition *condition = event->conditions + event->condition_count;
	condition->type = type;
	event->condition_count++;
	return(condition);
}

static void
activate_event(program_state *program, game_event *event)
{
	//pick current map
	game_world *current_map = program->maps + program->current_map_index;
	event_main *main_events = &current_map->events;

	if(event->prev_ad)
	{
		event->prev_ad->next_ad = event->next_ad;
	}
	if(event->next_ad)
	{
		event->next_ad->prev_ad = event->prev_ad;
	}
	if(event == main_events->inactive_events)
	{
		main_events->inactive_events = 0;
	}

	if(program->active_events)
	{
		program->active_events->prev_ad = event;
	}
	event->next_ad = program->active_events;
	program->active_events = event;
}

static void
deactivate_event(program_state *program, game_event *event)
{
	//pick current map
	game_world *current_map = program->maps + program->current_map_index;
	event_main *main_events = &current_map->events;
	if(event->prev_ad)
	{
		event->prev_ad->next_ad = event->next_ad;
	}
	if(event->next_ad)
	{
		event->next_ad->prev_ad = event->prev_ad;
	}
	if(event == program->active_events)
	{
		program->active_events = event->next_ad;
	}

	if(main_events->inactive_events)
	{
		main_events->inactive_events->prev_ad;
	}
	if(!event->eliminate_when_finished)
	{
		event->next_ad = main_events->inactive_events;
		main_events->inactive_events = event;
	}
}


static void
read_events(program_state *program)
{
	//pick current map
	game_world *current_map = program->maps + program->current_map_index;
	event_main *main_events = &current_map->events;

	game_event *event = main_events->inactive_events;
	//trigger events if conditions are satisfied
	while(event)
	{
		b32 run_event = 0;
		b32 condition_true = 1;

		for(u32 c = 0;condition_true && c < event->condition_count; c++)
		{
			u32 index = c;
			event_condition *condition = event->conditions + index;
			switch(condition->type)
			{
				//check if player is inside area
				case event_condition_player_in_area:
					{
						world_entity *player = program->player_entity;
						Assert(player);

						vec3 p0 = player->body->p;
						vec3 s0 = player->body->shape.size;

						vec3 p1 = condition->vector3;
						vec3 s1 = condition->size3;
						cubes_overlap_result overlap = cubes_overlap(p0, s0, p1, s1);
						condition_true &= (!overlap.value);
					}break;
				case event_condition_not_map_swapping:
					{
						condition_true &= (!program->switch_map);
					}break;
			}
		}
		if(condition_true)
		{
			//collided
			//queue this to the active events!
			game_event *bak = event->next_ad;
			activate_event(program, event);
			event = bak;
			//insert on the active events list

		}
		else
		{
			event = event->next_ad;
		}

	}
	//run active events
	event = program->active_events;
	while(event)
	{
		b32 keep_reading = 1;
		while(keep_reading)
		{
			event_line *line = event->lines + event->current_line;
			b32 advance_line = 1;
			switch(line->type)
			{
				//switch map
				default:
					{
						program->switch_map = 1;
						program->next_map_index = line->next_map_index;
						program->map_swap_p = line->vector3;

	//					world_entity *player = program->player_entity;
	//					Assert(player);

	//					player->body->p = line->vector3;

					}break;
			}

			if(advance_line)
			{
				event->current_line++;
			}
			else
			{
				keep_reading = 0;
			}

			if(event->current_line == event->line_count)
			{
				keep_reading = 0;
			}
		}

		if(event->current_line == event->line_count)
		{
			game_event *bak = event->next_ad;
			deactivate_event(program, event);
			event = bak;
		}
		else
		{
			event = event->next_ad;
		}
	}

}

