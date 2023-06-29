

static u32
editor_coso_get_name_chunk_index(editor_name_chunks *name_chunks, u8 *name)
{
	u32 index = 0;
	if(name)
	{
		for(u32 c = 0; c < name_chunks->count; c++)
		{
			u8 *chunk_name = name_chunks->chunks[c].name;
			if(chunk_name == name)
			{
				index = c;
				break;
			}
		}
	}
	return(index);
}

static inline u8 *
ec_condition_name(s_game_editor *game_editor,
		u32 type)
{
	u8 *name = 0;
	for(u32 c = 0;!name && c < game_editor->coso.condition_values_count; c++)
	{
		e_line *condition_v = game_editor->coso.condition_values + c;
		if(condition_v->value == type)
		{
			name = condition_v->name;
		}
	}
	return(name);
}
static void
e_coso_register_condition(
		s_game_editor *game_editor,
		u32 value,
		u8 *name)
{
	Assert(game_editor->coso.condition_values_count < state_node_count);
	e_line *condition_v = game_editor->coso.condition_values + game_editor->coso.condition_values_count++;
	u32 count = string_count(name);
	Assert(count < sizeof(condition_v->name));
	//set data
	string_copy(name, condition_v->name);
	condition_v->value = value;
}

static inline void
editor_coso_allocate(
		s_game_editor *game_editor, game_renderer *game_renderer)
{
	memory_area *area = &game_editor->area;
	s_coso_editor *coso_editor = &game_editor->coso;

	coso_editor->state_max = 100;
	coso_editor->actions_max = 100;
	coso_editor->collisions_max = 100;

	coso_editor->states = memory_area_push_array(
			area, editor_state_node, coso_editor->state_max);
	coso_editor->actions = memory_area_push_array(
			area, editor_state_action, coso_editor->actions_max);
	coso_editor->collisions = memory_area_push_array(
			area, editor_collision, coso_editor->collisions_max);
	coso_editor->dyarrays_area = memory_dyarray_area_create(&game_editor->area,
			MEGABYTES(1));
	coso_editor->render_parameters = render_set_initial_parameters(game_renderer);
	coso_editor->per_frame_area = memory_area_clear_and_create_from(&game_editor->area, KILOBYTES(256));

	coso_editor->condition_values = memory_area_push_array(&game_editor->area, e_line, state_node_count);
	e_coso_register_condition(game_editor, state_node_null           , "true");
	e_coso_register_condition(game_editor, state_node_random_factor  , "random_factor");
	e_coso_register_condition(game_editor, state_node_on_air         , "on_air");
	e_coso_register_condition(game_editor, state_node_timer_at       , "timer_at");
	//e_coso_register_condition(game_editor, state_node_player_distance, "player_distance");
	e_coso_register_condition(game_editor, state_node_moved          , "moved");
	e_coso_register_condition(game_editor, state_node_jump_pressed   , "jump_pressed");
	e_coso_register_condition(game_editor, state_node_attack_pressed , "attack_pressed");


	//entity default data
	coso_editor->simulating_entity.looking_direction.x = 0;
	coso_editor->simulating_entity.looking_direction.y = -1;

	editor_hash_and_name_chunks_allocate(
			area,
			&coso_editor->states_hash,
			&coso_editor->state_names,
			coso_editor->state_max,
			48);
	editor_hash_and_name_chunks_allocate(
			area,
			&coso_editor->action_hash,
			&coso_editor->action_names,
			coso_editor->actions_max,
			48);
	editor_hash_and_name_chunks_allocate(
			area,
			&coso_editor->collisions_hash,
			&coso_editor->collision_names,
			coso_editor->collisions_max,
			48);
}

static void
editor_coso_make_entity(
		s_game_editor *game_editor,
		render_commands *commands,
		f32 dt)
{
	s_coso_editor *coso_editor = &game_editor->coso;
	//reset the per_frame area
	memory_area_reset(&coso_editor->per_frame_area);

	world_entity *ent = &coso_editor->simulating_entity;
	//create the render data struct
	if(coso_editor->editing_coso_model)
	{
		model *coso_model = &coso_editor->editing_coso_model->asset_key->model;
		ent->model.animated_pose = render_allocate_pose(&coso_editor->per_frame_area, *coso_model);
		ent->model.model = coso_model;
		ent->body = &coso_editor->body;

		model_animate_and_render(commands,
				&ent->model,
				ent->body->p,
				ent->looking_direction,
				dt);
	}
}

static void
editor_coso_remove_line_condition(editor_state_line *line, u32 condition_index)
{
	if(condition_index < line->base.trigger_count)
	{
		memory_dyarray_remove_at(line->conditions, condition_index);
		line->base.trigger_count--;
	}
}

static editor_state_line *
editor_coso_add_state_line(
		s_game_editor *game_editor,
		editor_state_node *editor_state)
{
	editor_state_line *state_line = memory_dyarray_clear_and_push(editor_state->lines);
	editor_state->lines_count++;

	//create a triggers array
	state_line->conditions = memory_dyarray_create(
			game_editor->coso.dyarrays_area,
			state_trigger,
			4,
			4);

	return(state_line);
}

static editor_state_node *
editor_coso_add_state(
		s_game_editor *game_editor)
{
	s_coso_editor *coso_editor = &game_editor->coso;
	editor_state_node *state = 0;
	if(coso_editor->state_max > coso_editor->state_count)
	{
		state = coso_editor->first_free_state;
		if(!state)
		{
			state = coso_editor->states + coso_editor->state_count;
		}
		else
		{
			coso_editor->first_free_state = state->next_free;
		}
		memory_clear(state, sizeof(*state));
		state->lines = memory_dyarray_create(coso_editor->dyarrays_area,
				editor_state_line,
				10,
				10);

		u32 n = coso_editor->state_count;
		while(!editor_hash_and_name_add_indexf(
					&coso_editor->states_hash,
					&coso_editor->state_names,
					coso_editor->state_count,
					"state %u",
					n++));

		coso_editor->state_count++;
	}
	return(state);
}

static void
editor_coso_remove_state(
		s_game_editor *game_editor,
		u32 state_index)
{
	s_coso_editor *coso_editor = &game_editor->coso;
	if(coso_editor->state_count && state_index < coso_editor->state_count)
	{
		u8 *name = coso_editor->state_names.chunks[state_index].name;
		editor_hash_remove(&coso_editor->states_hash, name);
		editor_name_chunks_remove(&coso_editor->state_names, state_index);
		editor_state_node *state = coso_editor->states + state_index;
		memory_dyarray_delete(state->lines);

		state->next_free = coso_editor->first_free_state;
		coso_editor->first_free_state = state;

		for(u32 s  = 0; s < coso_editor->state_count; s++)
		{
			editor_state_node *editor_state = coso_editor->states + s;
			for(u32 l = 0; l < editor_state->lines_count; l++)
			{
				editor_state_line *line = memory_dyarray_get(editor_state->lines, l);
				if(line->state_name == name)
				{
					line->state_name = 0;
				}
			}
		}
	}
}

static editor_collision *
editor_coso_add_collision(s_editor_state *editor_state)
{
	s_coso_editor *coso_editor = &editor_state->editor.coso;
	editor_collision *collision = 0;
	if(coso_editor->collision_count < coso_editor->collisions_max)
	{
		editor_hash_and_name_add_index_numbered(
					&coso_editor->collisions_hash,
					&coso_editor->collision_names,
					coso_editor->collision_count,
					"collision");
		collision = coso_editor->collisions + 
			coso_editor->collision_count++;
		memory_clear(collision, sizeof(*collision));
	}
	return(collision);
}

static void
editor_coso_remove_collision(s_editor_state *editor_state, u32 index)
{
	s_coso_editor *coso_editor = &editor_state->editor.coso;
	if(index < coso_editor->collision_count)
	{
		editor_hash_and_name_remove(
				&coso_editor->collisions_hash,
				&coso_editor->collision_names,
				index);
		coso_editor->collision_count = e_array_remove_and_shift(
				coso_editor->collisions,
				editor_collision,
				coso_editor->collision_count,
				index);
	}
}

static void
editor_coso_save(
		s_editor_state *editor_state)
{
	s_game_editor *game_editor = &editor_state->editor;
	stream_data *info_stream = &game_editor->info_stream;
	platform_api *platform = editor_state->platform;
	
	s_coso_editor *coso_editor = &game_editor->coso;
	game_resource_attributes *editing_coso = coso_editor->editing_entity;
	if(!editing_coso)
	{
		return;
	}
	//header (figure composite resource, attached figures composite resource)
	//stats
	//states (properties, lines(conditions))
	//state actions (properties, lines)
	editor_wr wr = editor_wr_begin_write(
			&game_editor->area,
			platform,
			platform_file_op_create_new,
			editing_coso->path_and_name);
	if(wr.file.handle)
	{
		//first header, then lines
		ppse_coso_header *header = editor_wr_put_header(&wr, ppse_coso_header);
		header->header.signature = ppse_coso_SIGNATURE;
		header->header.version = ppse_coso_version;
		//set data.
		header->action_count = coso_editor->actions_count;
		header->state_count = coso_editor->state_count;
		header->collision_count = coso_editor->collision_count;
		//get the index in order from the name chunks
		if(coso_editor->coso_default_state)
		{
			u32 name_index = 0;
			for(u32 c = 0; c < coso_editor->state_names.count; c++)
			{
				u8 *chunk_name = coso_editor->state_names.chunks[c].name;
				if(chunk_name == coso_editor->coso_default_state)
				{
					name_index = c;
					break;
				}
			}
			//set index for the game to load
			header->default_state = name_index;
			////put the default state string id for the editor 
			//header->default_state_id = editor_generate_hash_key(coso_editor->coso_default_state);
		}
		
		//first line, composite resources
		editor_wr_put_line(&wr);
		{
			//		header.composite_resource_count += coso_editor->attached_models;
			header->header.offset_to_composite_resources = wr.data_offset;
			//this should always have a model for loading.
			header->header.composite_resource_count = 1;
			//save model composite resource
			if(coso_editor->editing_coso_model)
			{
				editor_wr_write_composite_resource_header(&wr,
						coso_editor->editing_coso_model->path_and_name);
			}	header->line_to_composite_resources = wr.current_line_number;
		}


		editor_wr_put_line(&wr);
		{
			ppse_coso_stats *file_stats = editor_wr_put_struct(&wr, ppse_coso_stats);
			header->line_to_stats = (u8)wr.current_line_number;
			file_stats->speed = coso_editor->stats.speed;
			file_stats->speed_max = coso_editor->stats.speed_max;
			file_stats->z_speed = coso_editor->stats.z_speed;
			file_stats->collision_x = coso_editor->collision_size.x;
			file_stats->collision_y = coso_editor->collision_size.y;
			file_stats->collision_z = coso_editor->collision_size.z;
		}
		//states
		editor_wr_put_line(&wr);
		{
			header->line_to_states = (u8)wr.current_line_number;
			for(u32 s = 0; s < coso_editor->state_count; s++)
			{
				//pick states using a hash index
				editor_state_node *state = coso_editor->states + editor_hash_and_name_get_index(
						&coso_editor->states_hash, &coso_editor->state_names, s);
				ppse_coso_state *file_state = editor_wr_put_struct(&wr, ppse_coso_state);
				file_state->line_count = state->lines_count;
				//save state lines
				for(u32 l = 0; l < state->lines_count; l++)
				{
					ppse_coso_state_line *file_state_line = editor_wr_put_struct(&wr, ppse_coso_state_line);
					editor_state_line *line = memory_dyarray_get(state->lines, l);
					file_state_line->condition_count = line->base.trigger_count;
					file_state_line->flags = line->base.type;
					//save the state index
					file_state_line->state_index = editor_coso_get_name_chunk_index(
							&coso_editor->state_names, line->state_name);
					//save state conditions
					for(u32 c = 0; c < file_state_line->condition_count; c++)
					{
						ppse_coso_state_condition *file_condition = editor_wr_put_struct(&wr, ppse_coso_state_condition);
						state_trigger *condition = memory_dyarray_get(line->conditions, c);
						file_condition->type = condition->type;
						file_condition->not = condition->not;
						file_condition->eq = condition->eq;
						file_condition->radius = condition->radius;
					}
				}
			}
		}
		//state "actions"
		editor_wr_put_line(&wr);
		{
			header->line_to_actions = (u8)wr.current_line_number;
			for(u32 a = 0; a < coso_editor->actions_count; a++)
			{
				editor_state_action *action = coso_editor->actions + editor_hash_and_name_get_index(
						&coso_editor->action_hash, &coso_editor->action_names, a);

				ppse_coso_action *file_action = editor_wr_put_struct(&wr, ppse_coso_action);
				file_action->line_count = action->lines_count;
				for(u32 l = 0; l < action->lines_count; l++)
				{
					ppse_coso_action_line *file_action_line = editor_wr_put_struct(
							&wr, ppse_coso_action_line);
					state_action_line *action_line = memory_dyarray_get(action->action_lines, l);
					file_action_line->target_index = action_line->target_index;
					file_action_line->animation_index = action_line->animation_index;
					file_action_line->time_total = action_line->time_total;
				}
			}
		}
		//collisions
		header->line_to_collisions = editor_wr_put_line(&wr);
		{
			for(u32 c = 0; c < coso_editor->collision_count; c++)
			{
				editor_collision *collision = coso_editor->collisions + c;
				ppse_coso_collision *file_collision = editor_wr_put_struct(&wr, ppse_coso_collision);
				file_collision->offset = collision->base.offset;
				file_collision->size = collision->base.size;
			}
		}
		editor_wr_put_line(&wr);
		{
			header->line_to_state_names = (u8)wr.current_line_number;
			//state names
			editor_wr_write_name_chunks(&wr, &coso_editor->state_names, 0);
			//action names
			editor_wr_write_name_chunks(&wr, &coso_editor->action_names, 0);
			//collision names
			editor_wr_write_name_chunks(&wr, &coso_editor->collision_names, 0);
		}

		//editorwr_put_line(editor_wr, ppse_coso_header);
		//{
		//	editor_wr_put_struct(editor_wr, ppse_struct);
		//	for(u32  a = 0; a < array_count ; a++)
		//	{
		//		//...
		//		editor_wr_put_struct();
		//	}
		//}
		//editor_wr_end_line(editor_wr);
		editor_wr_end(&wr);
	}
}

static void
editor_coso_reset(
		s_game_editor *game_editor)
{
	s_coso_editor *coso_editor = &game_editor->coso;
#if 0
	coso_editor->renaming_state = 0;
	coso_editor->actions_count = 0;
	coso_editor->state_count = 0;
	coso_editor->first_free_state = 0;
	coso_editor->state_is_selected = 0;
	coso_editor->selected_state_index = 0;
	coso_editor->state_line_new_is_selected;
#else
	memory_clear(coso_editor, sizeof(struct coso_editor_reset_data));
	editor_hash_and_name_reset(
			&coso_editor->states_hash, &coso_editor->state_names);
	editor_hash_and_name_reset(
			&coso_editor->action_hash, &coso_editor->action_names);
#endif


}

static void
editor_coso_load(
		s_editor_state *editor_state, game_resource_attributes *resource)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_coso_editor *coso_editor = &game_editor->coso;
	platform_api *platform = editor_state->platform;

	editor_wr wr = editor_wr_begin_read(
			&game_editor->area,
			platform,
			resource->path_and_name);
	if(wr.file.handle)
	{
		editor_coso_reset(game_editor);
		asset_entity *loading_entity = &resource->asset_key->entity;
		//load composite model
		coso_editor->editing_coso_model = resource->coso.model->attributes;
		coso_editor->editing_entity = resource;
		//stats
		coso_editor->stats.speed = loading_entity->speed;
		coso_editor->stats.z_speed = loading_entity->z_speed;
		coso_editor->stats.speed_max = loading_entity->speed_max;
		coso_editor->collision_size = loading_entity->collision_size;
		coso_editor->collision_offset = loading_entity->collision_offset;
		//states
		state_main *loading_states = &loading_entity->states;
		for(u32 s = 0;s < loading_states->state_count; s++)
		{
			editor_state_node *editor_state = editor_coso_add_state(game_editor);
			state_node *loading_state = loading_states->states + s;
		//	editor_state->lines_count = loading_state->state_line_count;
			//load lines
			for(u32 l = 0; l < loading_state->state_line_count; l++)
			{
				state_line *line = loading_states->state_lines + l;
				editor_state_line *editor_line = editor_coso_add_state_line(game_editor, editor_state);
				if(line->type == do_run_action)
				{
					//point to the correct name chunk
					editor_line->action_name = coso_editor->action_names.chunks[line->action_index].name;
				}
				else if(line->type == do_switch_state)
				{
					//point to the correct name chunk
					editor_line->state_name = coso_editor->state_names.chunks[line->state_index].name;
				}
				//load line conditions
				for(u32 c = 0; c < line->trigger_count; c++)
				{
				}
				editor_line->base = *line;
			}

		}
		//collisions
		for(u32 c = 0; c < loading_entity->collision_count; c++)
		{
			editor_collision *editor_collision = editor_coso_add_collision(editor_state);
			collision_cube *collision = loading_entity->collisions + c;
			editor_collision->base.offset = collision->offset;
			editor_collision->base.size = collision->size;
		}
		//set the default state after adding all of them
		coso_editor->coso_default_state = coso_editor->state_names.chunks[loading_entity->default_state].name;
		//state actions
		for(u32 a = 0; a < loading_states->action_count; a++)
		{
			state_action *action = loading_states->actions + a;
			//load action lines
			for(u32 l = 0; l < action->action_lines_count; l++)
			{
				state_action_line *line = loading_states->action_lines + l;
			}
		}

		ppse_coso_header *header = editor_wr_read_struct(&wr, ppse_coso_header);
		editor_wr_read_to_line(&wr, header->line_to_state_names);
		//state names
		editor_wr_set_name_chunks(&wr, &coso_editor->state_names, 0);
		//action names
		editor_wr_set_name_chunks(&wr, &coso_editor->action_names, 0);
		//collision names
		editor_wr_set_name_chunks(&wr, &coso_editor->collision_names, 0);
		//load the model as the component resource.
		editor_wr_end(&wr);
	}
}

static void
editor_coso_update_camera(
		s_game_editor *editor,
		game_renderer *game_renderer)
{
	s_coso_editor *coso_editor = &editor->coso;
	//update the camera movement just as the game does
	coso_editor->render_parameters.camera_target = V3(0, 0, 0);
	game_update_camera(
			game_renderer,
			&coso_editor->render_parameters,
			0);

	game_renderer->camera_position = coso_editor->render_parameters.camera_position;
	game_renderer->camera_rotation = coso_editor->render_parameters.camera_rotation;
}


static void
editor_coso_update_render(
		s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input,
		f32 dt)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_coso_editor *coso_editor = &game_editor->coso;


    b32 input_text_focused = editor_state->ui->input.input_text.focused;
    u16 ui_focused = game_editor->ui_is_focused;
	u32 editor_process_input = game_editor->process_input;

	//update hotkeys
	if(!input_text_focused && editor_process_input)
	{
		   u32 ctrl_hotkeys  = editor_input->ctrl_l;
		   u32 shift_hotkeys = editor_input->shift_l;
		   u32 alt_hotkeys   = editor_input->alt;
		   //global_hotkeys
		   if(!ctrl_hotkeys)
		   {
			   if((editor_input->number_keys[1]))
			   {
				   coso_editor->mode = coso_base;
			   }
			   else if((editor_input->number_keys[2]))
			   {
				   coso_editor->mode = coso_collisions;
			   }
			   else if((editor_input->number_keys[3]))
			   {
				   coso_editor->mode = coso_states;
			   }
			   else if((editor_input->number_keys[4]))
			   {
				   coso_editor->mode = coso_state_actions;
			   }
		   }
	}



	render_commands *commands = render_commands_begin_default(game_renderer);
	//mode specific
	switch(coso_editor->mode)
	{
		case coso_collisions:
			{
				if(coso_editor->collision_is_selected)
				{
					editor_collision *collision = coso_editor->collisions + coso_editor->selected_collision_index;

					//draw origin point
					render_draw_cube(commands, collision->base.offset, vec3_all(1), V4(255, 255, 255, 255));
					//draw size
					render_draw_cube(commands, collision->base.offset, collision->base.size, vec4_all(120));
				}
			}break;
	}

	//set default camera position
	editor_draw_background(commands);

	//update and render the entity
	editor_coso_make_entity(game_editor, commands, dt);
	render_commands_end(commands);
}

static void
editor_coso_update_render_ui(
		s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input,
		f32 dt)
{
	game_ui *ui = editor_state->ui;
	s_game_editor *game_editor = &editor_state->editor;
	s_coso_editor *coso_editor = &game_editor->coso;

	b32 new_clicked = 0;
	b32 save_clicked = 0;
	b32 load_clicked = 0;
	b32 save_as_clicked = 0;
	b32 select_resource_model = 0;

	ui_node *tool_bar = 0;
	ui_set_h_em(ui, 3.0f, 1.0f)
	ui_set_w_ppct(ui, 1.0f ,1.0f)
	{
		tool_bar = ui_label(ui, "coso_editor_tool_bar");
	}
	ui_set_parent(ui, tool_bar)
	{
		ui_space(ui, ui_size_specified(4.0f, 1.0f));
		ui_set_row(ui)
		{
			ui_space(ui, ui_size_specified(4.0f, 1.0f));
			ui_set_wh_text(ui, 4.0f, 1.0f)
			{
				new_clicked = ui_button(ui, "New##new_coso");
				ui_space(ui, ui_size_specified(4.0f, 1.0f));
				load_clicked = ui_button(ui, "Load##load_coso");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_clicked = ui_button(ui, "Save##save_coso");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_as_clicked = ui_button(ui, "Save as##save_as_coso");
				ui_space_specified(ui, 16.0f, 1.0f);
			}
		}
	}
	switch(coso_editor->mode)
	{
		default:
			{
				ui_space_ppct(ui, 1.0f, 0.0f);
			}break;
		case coso_base:
			{
				ui_node *panel = 0;
				ui_set_w_specified(ui, 400, 1.0f)
					ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						panel = ui_create_node(ui, node_background | node_clickeable, 0);
						panel->padding_x = 6;
						panel->padding_y = 6;
					}
				ui_set_parent(ui, panel) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 12.0f, 1.0f)
				{
					ui_text(ui, "Model:");
					u8 *model_name = "-";
					if(coso_editor->editing_coso_model)
					{
						model_name = coso_editor->editing_coso_model->path_and_name;
					}
					if(ui_selectablef(ui, 0, "%s##select_editing_coso_model", model_name))
					{
						select_resource_model = 1;
					}
						
					f32 text_separation = 10.0f;
					ui_text(ui, "Stats:");
					ui_push_row(ui, 1.0f, 1.0f);
					ui_set_column(ui) ui_set_w_em(ui, text_separation, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
					{
						ui_text(ui, "speed");
						ui_text(ui, "z_speed");
						ui_text(ui, "max_speed");
					}
					ui_set_column(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 4.0f, 1.0f)
					{
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->stats.speed, 0, "coso_speed");
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->stats.z_speed, 0, "coso_z_speed");
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->stats.speed_max, 0, "coso_max_speed");
					}
					ui_pop_row(ui);

					ui_set_row(ui)
					{
						ui_set_w_em(ui, text_separation, 1.0f)
						{
							ui_text(ui, "default_state");
						}
						ui_set_w_em(ui, 12.0f, 1.0f)
						{
							u8 *preview = "-";
							if(!coso_editor->state_count)
							{
							}
							else if(!coso_editor->coso_default_state)
							{
								//use 0 as default
								preview = coso_editor->state_names.chunks[0].name;
							}
							else if(coso_editor->coso_default_state)
							{
								preview = coso_editor->coso_default_state; 
							}
							ui_set_w_em(ui, 12.0f, 1.0f)
							{
								ui_push_disable_if(ui, !coso_editor->state_count);
								if(ui_drop_down_begin_quick(ui, "coso_default_state_dd", preview))
								{
#if 1
									ui_set_h_specified(ui, 400.0f, 1.0f)
									{
										ui_node *box_node = ui_box_with_scroll(ui, "coso_states_for_default");
										ui_set_parent(ui, box_node) ui_set_h_text(ui, 4.0f, 1.0f)
										{
											for(u32 t = 0; t < coso_editor->state_count; t++)
											{
												u8 *preview = coso_editor->state_names.chunks[t].name;
												u32 active = coso_editor->coso_default_state == preview;
												if(ui_selectablef(ui, active, "%s##entity_def_state_selectable%u", preview, t))
												{
													coso_editor->coso_default_state = preview;
												}
											}
										}
									}
#else
									ui_set_h_soch(ui, 1.0f) 
									{
										ui_set_row(ui)
										{
											ui_node *box = ui_scroll_v_box(ui, "coso_states_for_default");
											ui_set_parent(ui, box) ui_set_w_em(ui, 12.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_row(ui)
											{
												ui_set_column(ui) 
												for(u32 t = 0; t < coso_editor->state_count; t++)
												{
													b32 active = coso_editor->simulating_entity.default_state == t;
													u8 *preview = coso_editor->state_names.chunks[t].name;
													if(ui_selectablef(ui, active, "%s##entity_def_state_selectable%u", preview, t))
													{
														coso_editor->simulating_entity.default_state = t;
													}
												}
												//scroll
												ui_set_w_specified(ui, 12.0f, 1.0f)
												{
													ui_scroll_vertical_from_node(ui, box);
												}
											}

										}
										
									}
#endif
								}
								ui_drop_down_end(ui);
								ui_pop_disable(ui);
							}

						}
					}
					ui_set_wh_text(ui, 4.0f, 1.0f)
					{
						ui_text(ui, "collision_size");
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->collision_size.x, 0, "coso_col_x");
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->collision_size.y, 0, "coso_col_y");
						ui_spinner_f32(ui, 1.0f, 0.0f, 10000.0f, &coso_editor->collision_size.z, 0, "coso_col_z");
					}
				}

			}break;
		case coso_collisions:
			{
				ui_push_row(ui, 0, 0);
				ui_node *panel = 0;
				ui_set_w_specified(ui, 200, 1.0f)
					ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						panel = ui_create_node(ui, node_background | node_clickeable, 0);
						panel->padding_x = 6;
						panel->padding_y = 6;
					}
				ui_set_parent(ui, panel) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 12.0f, 1.0f)
				{
					ui_node *box_with_scroll = 0;
					ui_node *box_top_bar = 0;
					ui_set_wh_ppct(ui, 1.0f, 0.0f)
					{
						ui_set_h_em(ui, 2.0f, 1.0f)
						{
							box_top_bar = ui_label(ui, 0);
							box_top_bar->layout_axis = 0;
							box_top_bar->padding_x = 2;
							box_top_bar->padding_y = 2;
						}
						box_with_scroll = ui_box_with_scroll(ui, "Collisions_list_box");
					}
					//top bar with add and remove button
					//collision list
					ui_set_parent(ui, box_top_bar) ui_set_wh_text(ui, 4.0f, 1.0f) ui_extra_flags(ui, node_border)
					{
						if(ui_button(ui, "+#coso_collision"))
						{
							editor_coso_add_collision(editor_state);
						}
						ui_space_specified(ui, 4.0f, 1.0f);
						ui_push_disable_if(ui, !coso_editor->collision_is_selected || !coso_editor->collision_count);
						{
							if(ui_button(ui, "x#coso_collision"))
							{
								editor_coso_remove_collision(editor_state, coso_editor->selected_collision_index);
							}
						}
						ui_pop_disable(ui);
					}
					ui_set_parent(ui, box_with_scroll)
					{
						for(u32 c = 0; c < coso_editor->collision_count; c++)
						{
							u8 *name = coso_editor->collision_names.chunks[c].name;
							b32 active = coso_editor->collision_is_selected && coso_editor->selected_collision_index == c;
							if(ui_selectablef(ui, active, "%s##collider%u", name, c))
							{
								coso_editor->collision_is_selected = 1;
								coso_editor->selected_collision_index = c;
							}
						}
					}
				}
				ui_space_ppct(ui, 1.0f, 0.0f);
				ui_node *properties_panel = 0;
				ui_set_w_specified(ui, 200.0f, 1.0f) ui_set_h_soch(ui, 1.0f)
				{
					properties_panel = ui_node_box(ui, 0);
				}
				ui_set_parent(ui, properties_panel) ui_set_wh_text(ui, 4.0f, 1.0f)
				{
					ui_text(ui, "Properties");
					if(coso_editor->collision_is_selected)
					{
						ui_text(ui, "Name:");
						//display name normally if not renaming
						if(!coso_editor->renaming_collision)
						{
							u8 *name = coso_editor->collision_names.chunks[coso_editor->selected_collision_index].name;
							ui_text(ui, name);
							ui_set_row(ui) ui_extra_flags(ui, node_border)
							{
								if(ui_button(ui, "Rename#collision"))
								{
									coso_editor->renaming_collision = 1;
									memory_clear_and_copy(name,
											coso_editor->name_change_buffer,
											sizeof(coso_editor->name_change_buffer));
								}
							}
						}
						else
						{
							ui_next_nodes_interaction_only_begin(ui);
							b32 name_avadible = 0;

							//try to rename this
							ui_set_w_em(ui, 12.0f, 1.0f)
							{
								editor_hash_check hash_name_check = 0;
								ui_input_text(ui, 0, coso_editor->name_change_buffer, 
										coso_editor->collision_names.length - 1,
										"Collision rename");
								name_avadible = editor_hash_string_is_avadible(
										&coso_editor->collisions_hash,
										&hash_name_check,
										coso_editor->name_change_buffer);
							}
							ui_set_row(ui) ui_extra_flags(ui, node_border)
							{
								ui_push_disable_if(ui, !name_avadible);
								if(ui_button(ui, "confirm#rename_col"))
								{
									coso_editor->renaming_collision = 0;
									editor_hash_and_name_modify_key_by_index(
											&coso_editor->collisions_hash,
											&coso_editor->collision_names,
											coso_editor->selected_collision_index,
											coso_editor->name_change_buffer);
								}
								ui_pop_disable(ui);
								ui_space_specified(ui, 4.0f, 1.0f);
								if(ui_button(ui, "cancel#rename_col"))
								{
									coso_editor->renaming_collision = 0;
								}
							}
							ui_next_nodes_interaction_only_end(ui);
						}

						editor_collision *collision = coso_editor->collisions + coso_editor->selected_collision_index;
						ui_text(ui, "offset");
						ui_set_row(ui) ui_set_w_em(ui, 5.0f, 1.0f)
						{
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.offset.x, 0, "col_offset_x");
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.offset.y, 0, "col_offset_y");
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.offset.z, 0, "col_offset_z");
						}
						ui_text(ui, "size");
						ui_set_row(ui) ui_set_w_em(ui, 5.0f, 1.0f)
						{
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.size.x, 0, "col_size_x");
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.size.y, 0, "col_size_y");
							ui_spinner_f32(ui, 1, 0, 1000, &collision->base.size.z, 0, "col_size_z");
						}
					}
				}
				ui_pop_row(ui);
			}break;
		case coso_states:
			{
				
				//current editing state
				editor_state_node *editor_state = 0;
				if(coso_editor->state_is_selected)
				{
					editor_state = coso_editor->states + coso_editor->selected_state_index;
				}

				ui_node *panel = 0;
				ui_node *panel_r = 0;
				ui_set_row(ui)
				ui_set_w_specified(ui, 400, 1.0f)
					ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						panel = ui_create_node(ui, node_background | node_clickeable, 0);
						panel->padding_x = 6;
						panel->padding_y = 6;
						ui_space_ppct(ui, 1.0f, 0.0f);
						panel_r = ui_create_node(ui, node_background | node_clickeable, 0);
						panel_r->padding_x = 6;
						panel_r->padding_y = 6;
					}
				//list of states
				ui_set_parent(ui, panel) ui_set_wh_text(ui, 4.0f, 1.0f)
				{
					u32 state_list_w = 200;
					ui_set_row(ui)
					{
						//list of states
						{
							ui_push_w_specified(ui, state_list_w, 1.0f);
							ui_push_h_ppct(ui, 1.0f, 0.0f);
							ui_node *state_list = ui_create_node(ui, node_background | node_clickeable | node_border, 0);
							ui_pop_width(ui);
							ui_pop_height(ui);

							state_list->padding_x = 6;
							state_list->padding_y = 6;

							ui_set_parent(ui, state_list)
							{
								ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_ppct(ui, 1.0f, 0.0f)
									for(u32 s = 0; s < coso_editor->state_count; s++)
									{
										u8 *name = coso_editor->state_names.chunks[s].name;
										b32 active = coso_editor->state_is_selected && coso_editor->selected_state_index == s;
										if(ui_selectablef(ui, active, name, "%s##coso_editor_state%u", name, s))
										{
											coso_editor->state_is_selected = 1;
											coso_editor->selected_state_index = s;
											coso_editor->renaming_state = 0;
											coso_editor->state_name_avadible = 0;
										}
									}
							}
						}
						ui_set_column(ui)
						{
							ui_text(ui, "Properties");
							if(coso_editor->state_is_selected)
							{
								ui_text(ui, "Name:");
								u8 *name = coso_editor->state_names.chunks[coso_editor->selected_state_index].name;
								if(!coso_editor->renaming_state) ui_set_column(ui)
								{
									ui_text(ui, coso_editor->state_names.chunks[coso_editor->selected_state_index].name);
									ui_extra_flags(ui, node_border)
									if(ui_button(ui, "Rename##rename state"))
									{
										coso_editor->renaming_state = 1;
										memory_clear(coso_editor->name_change_buffer, sizeof(coso_editor->name_change_buffer));
										memory_copy(name, coso_editor->name_change_buffer, sizeof(coso_editor->name_change_buffer));
									}
								}
								else
								{
									b32 modified = 0;
									editor_hash_check hash_name_check = 0;
									ui_set_w_em(ui, 16.0f, 1.0f)
									{
										modified = ui_input_text(ui, 0, coso_editor->name_change_buffer, sizeof(coso_editor->name_change_buffer) - 1, "state_name_change");
									}
									if(modified)
									{
										b32 name_avadible = editor_hash_string_is_avadible(
												&coso_editor->states_hash,
												&hash_name_check,
												coso_editor->name_change_buffer);
										coso_editor->state_name_avadible = name_avadible;
									}
									ui_set_row(ui)
									{
										ui_push_disable_if(ui, !coso_editor->state_name_avadible);
										if(ui_button(ui, "Confirm#state name"))
										{
											coso_editor->renaming_state = 0;
											editor_hash_and_name_modify_key_by_index(
													&coso_editor->states_hash,
													&coso_editor->state_names,
													coso_editor->selected_state_index,
													coso_editor->name_change_buffer);
										}
										ui_pop_disable(ui);
										ui_space_specified(ui, 4.0f, 1.0f);
										if(ui_button(ui, "Cancel#state name"))
										{
											coso_editor->renaming_state = 0;
										}
									}
								}
							}
						}
					}
					//add and remove buttons
					ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
					{
						if(ui_button(ui, "+##add_coso_state"))
						{
							editor_coso_add_state(game_editor);
						}
						ui_space_specified(ui, 4.0f, 1.0f);
						ui_push_disable_if(ui, coso_editor->state_count == 0 || !coso_editor->state_is_selected);
						{
							if(ui_button(ui, "x##remove_coso_state"))
							{
							}
						}
						ui_pop_disable(ui);
					}

					//state lines
					ui_node *state_lines_panel = 0;
					ui_node *state_lines_panel_bar = 0;
					//top bar with name with plus and remove
					ui_set_w_specified(ui, 200, 1.0f) ui_set_h_soch(ui, 1.0f)
					{
						state_lines_panel_bar = ui_label(ui, 0);
						state_lines_panel_bar->padding_x = 6;
						state_lines_panel_bar->padding_y = 6;
						state_lines_panel_bar->layout_axis = 0;
						ui_set_parent(ui, state_lines_panel_bar) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							//disable buttons is there isn't any state selected
							ui_push_disable_if(ui, !coso_editor->state_is_selected);
							ui_push_disable_if(ui, !coso_editor->state_line_is_selected);
							ui_text(ui, "Lines");
							ui_space_specified(ui, 4.0f, 1.0f);
							ui_button(ui, "+##add_state_line");
							ui_space_specified(ui, 4.0f, 1.0f);
							ui_button(ui, "x##remove_state_line");
							ui_pop_disable(ui);
							ui_pop_disable(ui);
						}
					}
					//panel with list of lines
					u32 state_lines_panel_height = 400;
					i32 padding_x = 2;
					i32 padding_y = 2;
					ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, state_lines_panel_height, 1.0f)
					{
						ui_usri line_usri = {0};
						u32 lines_height = (u32)ui_em(ui) * 2;
						ui_line_list_begin(ui, (u32)(ui_em(ui) * 2.0f), "state_lines_entity");
						if(editor_state)
						{
							for(u32 l = 0;l < editor_state->lines_count; l++)  
							{
								editor_state_line *line = memory_dyarray_get(editor_state->lines, l);
								b32 selected = coso_editor->state_line_is_selected &&
									coso_editor->selected_state_line_index == l &&
									!coso_editor->state_line_new_is_selected;
								ui_line_list_line_beginf(ui, selected, &line_usri, "entity_state_line%u", l);
								{
									//display conditions
									ui_set_h_specified(ui, lines_height, 1.0f) ui_set_w_text(ui, 4.0f, 1.0f)
									{
										ui_set_row(ui)
										{
											ui_textf(ui, "if", l);
											if(line->base.trigger_count)
											{
												for(u32 t = 0; t < line->base.trigger_count; t++)
												{
													state_trigger *trigger = memory_dyarray_get(line->conditions, t);
													u8 *type = 0;
													switch(trigger->type)
													{
														case state_node_null: 
															type = "true"; break;
														case state_node_random_factor : 
															type = "random_factor"; break;
														case state_node_on_air : 
															type = "on_air"; break;
														case state_node_timer_at : 
															type = "timer_at"; break;
														case state_node_moved : 
															type = "moved"; break;
														case state_node_jump_pressed :
															type = "jump_pressed"; break;
														case state_node_attack_pressed : 
															type = "attack_pressed"; break;
															ui_text(ui, type);
													}
												}
											}
											else
											{
												ui_textf(ui, "1");
											}
										}
									}
								}
								ui_line_list_line_end(ui);
								//select the line if clicked
								if(ui_usri_mouse_l_pressed(line_usri))
								{
									coso_editor->state_line_is_selected = 1;
									coso_editor->selected_state_line_index = l;
									coso_editor->state_line_new_is_selected = 0;
								}
							}
							b32 new_line_selected = coso_editor->state_line_is_selected && coso_editor->state_line_new_is_selected;
							ui_line_list_line(ui, new_line_selected, &line_usri, "state line add") ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_specified(ui, lines_height, 1.0f)
							{
								ui_text(ui, ">");
								b32 pressed = ui_usri_mouse_l_pressed(line_usri);
								b32 add_trigger = coso_editor->state_line_is_selected && 
									coso_editor->state_line_new_is_selected && 
									ui_usri_mouse_l_double_clicked(line_usri);
								if(pressed)
								{
									coso_editor->state_line_is_selected = 1;
									coso_editor->state_line_new_is_selected = 1;
								}
								if(add_trigger)
								{
									editor_coso_add_state_line(game_editor, editor_state);
								}
							}

						}
						ui_line_list_end(ui);

					}
				}
				//state line properties
				ui_set_parent(ui, panel_r) ui_set_wh_text(ui, 4.0f, 1.0f)
				{
					editor_state_line *editor_state_line = 0;
					if(editor_state && 
					   coso_editor->state_line_is_selected && 
					   !coso_editor->state_line_new_is_selected)
					{
						editor_state_line = memory_dyarray_get(editor_state->lines, coso_editor->selected_state_line_index);
					}
					ui_text(ui, "state line properties");
					if(editor_state_line)
					{
						ui_checkbox_flag(ui, do_run_action, &editor_state_line->base.type, "do_run_action");
						//select an action if the flag is set
						ui_push_disable_if(ui, !(editor_state_line->base.type & do_run_action));
						ui_pop_disable(ui);

						ui_checkbox_flag(ui, do_switch_state, &editor_state_line->base.type, "do_switch_state");
						//select a state if the flag is set
						ui_push_disable_if(ui, !(editor_state_line->base.type & do_switch_state));
						ui_set_w_em(ui, 16.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
						{
							u8 *preview = "-";
							if(editor_state_line->base.type & do_switch_state &&
							   editor_state_line->base.state_index < coso_editor->state_count &&
							   editor_state_line->base.state_index != coso_editor->selected_state_index)
							{
								preview = coso_editor->state_names.chunks[editor_state_line->base.state_index].name;
							}
							ui_id dd_id = ui_id_from_string("state line state_index");
							if(ui_drop_down_beginf(ui, dd_id, "%s##state_lines_state_index_dd", preview))
							{
								ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
								for(u32 s = 0; s < coso_editor->state_count; s++)
								{
									if(s == coso_editor->selected_state_index) continue;
									u8 *name = coso_editor->state_names.chunks[s].name;
									if(ui_selectablef(ui, 0, "%s##line_switch_state#u", name, s))
									{
										editor_state_line->state_name = name;
									}
//									ui_list_selectable_u32(ui, 0, name, s, &editor_state_line->state_index);
								}
							}
							ui_drop_down_end(ui);
						}
						ui_pop_disable(ui);

						ui_text(ui, "Conditions");
						u32 lines_height = (u32)ui_em(ui) * 2;
						ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 400, 1.0f)
						{
							ui_id add_c_popup_id = ui_id_from_string("condition_context_box");

							ui_line_list_begin(ui, lines_height, "state line conditions");
							{
								ui_usri line_usri = {0};
								u32 open_popup = 0;
								for(u32 c = 0; c < editor_state_line->base.trigger_count; c++)
								{
									state_trigger *condition = memory_dyarray_get(
											editor_state_line->conditions, c);
									b32 selected = !coso_editor->state_line_condition_new_is_selected &&
										coso_editor->state_line_condition_is_selected &&
										coso_editor->state_line_selected_condition_index == c;
									ui_line_list_line_beginf(ui, selected, &line_usri, "state line trigger %u", c); ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_specified(ui, lines_height, 1.0f)
									{
										ui_set_row(ui) ui_set_w_text(ui, 1.0f, 1.0f)
										{
											if(condition->not)
											{
												ui_text(ui, "!");
											}
											u8* name = ec_condition_name(game_editor, condition->type);
											ui_text(ui, name);
										}
									}
									//select if clicked
									u8 pressed_l = ui_usri_mouse_l_pressed(line_usri);
									u8 pressed_r = ui_usri_mouse_r_pressed(line_usri);
								    if(pressed_l || pressed_r)
									{
										coso_editor->state_line_condition_is_selected = 1;
										coso_editor->state_line_selected_condition_index = c;
										coso_editor->state_line_condition_new_is_selected = 0;

									}
									if(ui_usri_mouse_r_pressed(line_usri))
									{
										ui_popup_open_mouse(ui, add_c_popup_id);
									}
									ui_line_list_line_end(ui);
								}

								b32 selected = coso_editor->state_line_condition_new_is_selected;
								ui_line_list_line_begin(ui, selected, &line_usri, "state line test"); ui_set_w_text(ui, 4.0f, 1.0f) ui_set_h_specified(ui, lines_height, 1.0f)
								{
									ui_text(ui, ">");
									b32 pressed = ui_usri_mouse_l_pressed(line_usri);
									b32 add_trigger = selected && ui_usri_mouse_l_double_clicked(line_usri);
									if(pressed)
									{
										coso_editor->state_line_selected_condition_index = 
											editor_state_line->base.trigger_count;
										coso_editor->state_line_condition_new_is_selected = 1;
										coso_editor->state_line_condition_is_selected = 0;
									}
									if(add_trigger)
									{
										editor_state_line->base.trigger_count++;
										memory_dyarray_clear_and_push(editor_state_line->conditions);
									}
								}
								ui_line_list_line_end(ui);
							}
							ui_line_list_end(ui);
							//drop down for adding
							ui_set_wh_soch(ui, 1.0f)
							if(ui_context_menu_begin(ui, add_c_popup_id))
							{
								ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
								{
									if(ui_selectable(ui, 0, "Delete#line_condition"))
									{
										//remove selected condition
										ui_popup_close(ui, add_c_popup_id);
										editor_coso_remove_line_condition(editor_state_line, coso_editor->state_line_selected_condition_index);
									}
								}
							}
							ui_context_menu_end(ui);
						}
						if(coso_editor->state_line_condition_is_selected) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_text(ui, "Condition properties:");
							state_trigger *condition =  memory_dyarray_get(
									editor_state_line->conditions, coso_editor->state_line_selected_condition_index);
							ui_set_row(ui)
							{
								ui_text(ui, "type");
								u8 *cname = ec_condition_name(game_editor, condition->type);
								ui_set_w_em(ui, 12.0f, 1.0f)
								ui_drop_down_quick(ui, "current_condition_type", cname)
								{
									ui_node *box = 0;
									ui_set_h_specified(ui, 400.0f, 1.0f)
									{
	//									box = ui_box_with_scroll(ui, "current_condition_type_box");
									}
	//								ui_set_parent(ui, box)
									{
										for(u32 t = 0; t < coso_editor->condition_values_count; t++)
										{
											u8 *condition_name = coso_editor->condition_values[t].name;
											if(ui_selectablef(ui, condition->type == t, "%s##condition_type%u", condition_name, t))
											{
												condition->type = coso_editor->condition_values[t].value;
											}
										}
									}
									
								}

							}
						}
					}
				}
			}break;
		case coso_state_actions:
			{
			}break;
	}



	//bottom bar where the file name is displayed
	ui_node *bottom_bar;
	ui_node *bottom_bar_mode;
	ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
	ui_set_w_ppct(ui, 1.0f, 1.0f)
	{
		bottom_bar = ui_create_node(ui, node_background, 0);
		bottom_bar_mode = ui_create_node(ui, node_background, 0);
	}
	//display map name
	ui_set_parent(ui, bottom_bar) ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		if(coso_editor->editing_entity)
		{
			ui_text(ui, coso_editor->editing_entity->path_and_name);
		}
	}
	//display editor mode
	ui_set_parent(ui, bottom_bar_mode) ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		u8 *mode_name = "UNDEFINED";
		switch(coso_editor->mode)
		{
			case coso_base: mode_name = "-- BASE --"; break;
			case coso_collisions: mode_name = "-- COLLISIONS --"; break;
			case coso_states: mode_name = "-- STATES --"; break;
			case coso_state_actions: mode_name = "-- ACTIONS --"; break;
		}
		ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
		{
			ui_text(ui, mode_name);
		}
	}
	
	//save/load
	if(save_clicked && coso_editor->editing_entity)
	{
		editor_coso_save(editor_state);
	}
	else if(save_as_clicked || (save_clicked && !coso_editor->editing_entity))
	{
		er_explorer_set_process_reestricted(game_editor,
				er_explorer_save,
				asset_type_entity,
				"Save entity");
	}

	if(editor_resource_explorer_process_completed(game_editor, "Save entity"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		game_resource_attributes *resource = editor_resource_create_and_save(
				editor_state, asset_type_entity, 1, name);
		if(resource)
		{
			coso_editor->editing_entity = resource;
			editor_coso_save(editor_state);
			editor_resources_reimport(editor_state, resource);
		}
	}
	if(editor_resource_explorer_process_completed(game_editor, "Load entity"))
	{
		editor_coso_load(editor_state, er_look_for_resource(game_editor, editor_resource_explorer_output(game_editor)));
	}

	//select new model
	if(select_resource_model)
	{
		er_explorer_set_process_reestricted(game_editor,
				er_explorer_load,
				asset_type_model,
				"Select entity model");
	}
	if(editor_resource_explorer_process_completed(game_editor, "Select entity model"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		game_resource_attributes *resource = er_look_for_resource(game_editor, name);
		if(resource)
		{
			coso_editor->editing_coso_model = resource;
		}
	}
	if(new_clicked)
	{
		editor_coso_reset(
				game_editor);
	}
	else if(load_clicked)
	{
		er_explorer_set_process_reestricted(
				game_editor,
				er_explorer_load,
				asset_type_entity,
				"Load entity");
	}
}
