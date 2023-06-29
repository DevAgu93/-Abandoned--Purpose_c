
static void 
request_entity_delete(program_state *program, world_entity *entity)
{
	program->entities_to_delete[program->deleting_entities_count] = entity;
	program->deleting_entities_count++;
	Assert(program->deleting_entities_count < delete_entities_MAX);
}

static void
request_entity_spawn(program_state *program, entity_spawn_parameters params)
{
	program->requested_entities[program->requested_entities_count] = params;
	program->requested_entities_count++;
	Assert(program->requested_entities_count < requested_entities_MAX);
}

static void 
spawn_requested_entities(program_state *program)
{
	//delete requested entities
	for(u32 d = 0; d < program->deleting_entities_count; d++)
	{
		world_entity *ent = program->entities_to_delete[d];
		game_remove_entity(program, ent);
	}
	program->deleting_entities_count = 0;

	//spawn requested entities.
	for(u32 r = 0; r < program->requested_entities_count; r++)
	{
		entity_spawn_parameters sp = program->requested_entities[r];
		//cancel bad request
		if((sp.flags & coso_hitpoints) && sp.hitpoints == 0)
		{
			Assert(0);
			continue;
		}
		if((sp.flags & coso_lifetime) && sp.life_time == 0)
		{
			Assert(0);
			continue;
		}

		//get entity slot
		world_entity *new_entity = game_create_entity(program);
		new_entity->flags = sp.flags;
		new_entity->life_time = sp.life_time;
		//looking_direction
		new_entity->looking_direction = sp.dir;
		new_entity->parent = sp.parent;
		new_entity->relative_p = sp.relative_p;
		new_entity->id.id1 = sp.id1;
		new_entity->dmg.frame_timer = sp.dmg_frame_time;
		new_entity->dmg.frames_before_hit = sp.frames_before_hit;
		//sweep
		new_entity->dmg.sweep = sp.sweep;
		new_entity->distance = sp.distance;
		new_entity->dmg.sweep_angle = 0;
		new_entity->end_angle = sp.end_angle;
		new_entity->start_angle = arctan232(sp.relative_p.y, sp.relative_p.x);
		new_entity->side = sp.side >= 0 ? 1 : -1;

		new_entity->hitpoints = sp.hitpoints;


		if(sp.dir.x + sp.dir.y == 0)
		{
			new_entity->looking_direction.y = -1;
		}
		//body
		if(sp.flags & coso_body)
		{
			new_entity->body = body_create(program);
			new_entity->body->shape.type = sp.shape;
			new_entity->body->shape.size = sp.size;
			new_entity->body->p = sp.p;
			new_entity->body->speed = sp.speed;
			if(sp.flags & coso_traversable)
			{
				new_entity->body->traversable = 1;
			}
			if(sp.flags & coso_no_gravity)
			{
				new_entity->body->ignore_gravity = 1;
			}
		}

		//connect with siglings
		if(new_entity->parent)
		{
			world_entity *parent = new_entity->parent;
			if(parent->first)
			{
				parent->first->prev_sibling = new_entity;
				new_entity->next_sibling = new_entity;
			}
			parent->first = new_entity;
		}
		if(new_entity->flags & coso_frame_lifetime)
		{
			new_entity->life_time = 1.0f;
		}
		
	}
	program->requested_entities_count = 0;
}

static coso_id 
game_get_coso_id(program_state *program)
{
	coso_id id = {0};
	id.id0 = program->next_entity_id++;
	return(id);
}

static world_entity *
game_create_entity(program_state *program)
{
	world_entity *entity = program->first_free_entity;
	if(entity)
	{
		program->first_free_entity = entity->next;
	}
	else
	{
		entity = program->entities + program->entities_count;
		entity->id = game_get_coso_id(program);
		//generate an id
		program->entities_count++;
		Assert(program->entities_count < program->entities_max);
	}
	coso_id copy_id = entity->id;
	memory_clear(entity, sizeof(*entity));
	entity->id = copy_id;
	entity->id.id1 = 0;
	entity->looking_direction.x = 0;
	entity->looking_direction.y = -1;

	if(!program->first_entity) program->first_entity = entity;
	if(program->last_entity) program->last_entity->next = entity;

	entity->prev = program->last_entity;
	program->last_entity = entity;


	//set up stuff needed from it
	return(entity);
}

static void
game_remove_entity(program_state *program, world_entity *entity)
{
	//cancel
	if(entity->id.id != 0){
		//disconnect from parent
		/*
		   if(entity->parent)
		   {
		   world_entity *parent = entity->parent;
		   if(parent->first == entity)
		   {
		   parent->first = entity->next;
		   }
		   }
		   */
		if(entity == program->first_entity)
		{
			program->first_entity = entity->next;
		}
		if(entity == program->last_entity)
		{
			program->last_entity = entity->prev;
		}

		//disconnect from siblings
		if(entity->prev) entity->prev->next = entity->next;
		if(entity->next) entity->next->prev = entity->prev;
		if(entity->prev_sibling) 
		{
			entity->prev_sibling->next_sibling = entity->next_sibling;
		}
		if(entity->next_sibling) 
		{
			entity->next_sibling->prev_sibling = entity->prev_sibling;
		}
		if(entity->parent && entity->parent->first == entity)
		{
			entity->parent->first = entity->next_sibling;
		}
		//disconnect from childs
		//get rid of child entities if specified
		/*
		   if(entity->first)
		   {
		   for(world_entity *child = entity->first; child; child = child->next)
		   {
		   if(child->remove_with_parent)
		   {
		   child->marked_for_remove = 1;
		   }
		   }
		   }
		   */

		entity->id.id1 = 0;
		entity->parent = 0;
		entity->prev = 0;
		entity->next = program->first_free_entity;
		entity->next_sibling = 0;
		entity->prev_sibling = 0;
		program->first_free_entity = entity;

		//eliminate brain if any
		if(entity->brain)
		{
			remove_brain(program, entity->brain);
		}
		if(entity->body)
		{
			body_remove(program, entity->body);
		}
	}
}

static world_entity * 
game_create_player_entity(program_state *program)
{
	game_assets *assets = program->game_asset_manager;
	program->player_entity_index = program->entities_count;
	world_entity *ent = game_create_entity(program);

	ent->body = body_create(program);
	program->player_entity = ent;

	u8 *model_pan = "data/test_model_sprite.ppmo";
	model *asset_model = assets_load_and_get_model(
			assets, model_pan);
	if(asset_model)
	{
		ent->model = model_allocate_render_data(
				program->area, asset_model);

		if(0)
		{
			//temp allocate weapon
			model_render_data *model_data = &ent->model;
			//		model_data->attached_models_count++;
			model_data->attached_models = memory_area_push_array(program->area, model * , 2);
			model_data->attached_models[0] = assets_load_and_get_model(assets, "data/model_sword.ppmo");

			model_data->attach_data = memory_area_push_array(program->area, model_attachment_data, 2);
			model_attachment_data *attachment_data = model_data->attach_data + 0;
			attachment_data->bone_index = 1;
			attachment_data->model = 	model_data->attached_models[0];
			Assert(attachment_data->model);
			attachment_data->animated_pose = render_allocate_pose(program->area, *attachment_data->model);
		}
//		entity *child = assets_load_and_get_coso(assets, "sword.pcos");
	}
	else
	{
		Assert(0);
	}
	//body params
	{
		game_body *body = ent->body;
//		program->controlling_body = body;
		//set stats
		body->speed = 1.2f;
		body->z_speed = 2.7f;
		body->speed_max = 2.2f;
		body->weight = 25; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 0;
		body->p.y = 0;
	}
	enum{
		ps_idle = 0,
		ps_air = 1,
	};
	//brain
	{
		//frame container: ejecuta una secuencia de acciones dependiendo del "frame" actual
		//ejemplo: un container con 40 frames en total, entre los frames 0-20 ejecuta una lista
		//de acciones, y entre 20-40 ejecuta otra
		//||0------20------40
		//     a0       a1
		//Lo que hacía antes
		//state: atk1
		/*
		   at frames 0-1 switch animation to attack
		   at frames 0-15, spawn hitbox at specified location
		   at frames 10-15, allow switching to atk2 by pressing atk
		   at frames 25-26 cancel state, switch to any
		*/
		game_brain_creation bc = begin_brain_creation(program);
		state_main *brain = &bc.brain;

		//0.floor/idle
		{
			state_node *state0 = brain_add_state(brain);
			state_line *line0 = brain_add_state_line(brain, state0);
			line0->type = do_move;


			line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, state_node_on_air);
				condition->not = 1;
				condition = brain_add_line_condition(brain, line0, state_node_jump_pressed);
				line0 = brain_add_state_line(brain, state0);
				line0->type = do_jump;
			}
			//attack
#if 0
			line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, state_node_attack_pressed);

				line0 = brain_add_state_line(brain, state0);
				//line0->type = do_spawn_hurtbox;
				line0->type = do_spawn_melee_hurtbox;
				//hurtbox life time
				line0->time0 = 10.0f;
				//relative position from looking_direction
				line0->p = V3(0, 20, 4);
				line0->relative_to_dir = 1;
				line0->size = V3(5, 5, 5);
			}
#else
			//attack state if attack pressed
			line0 = brain_add_state_line(brain, state0);
			line0->type = do_switch_state;
			line0->state_index = 2;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, state_node_attack_pressed);
			}
#endif

			//if on air and frame > 10, switch
			line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, state_node_on_air);
				condition = brain_add_line_condition(brain, line0, state_node_jump_pressed);

				line0 = brain_add_state_line(brain, state0);
				line0->type = do_switch_state;
				line0->state_index = 1;
			}

			
		}
		//1.air
		{
			state_node *state0 = brain_add_state(brain);
			state_line *line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 2;
			{
				//if frame >= 0 && frame < 1 add speed
				state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
				condition->frame_at = 0;

				line0 = brain_add_state_line(brain, state0);
				line0->type = do_set_speed;
				line0->vec.z = 0.0f;

				line0 = brain_add_state_line(brain, state0);
				line0->type = do_add_speed;
				line0->vec.z = 4.0f;
			}

			//switch idle state
			line0 = brain_add_state_line(brain, state0);
			line0->type = do_switch_state;
			line0->state_index = ps_idle;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, state_node_on_air);
				condition->not = 1;
			}

			
		}

		//2. attack
		{
			state_node *state0 = brain_add_state(brain);


			state_line *line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 1;

			{
				state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
				condition->frame_at = 0;
				condition->frame_duration = 1;

				line0 = brain_add_state_line(brain, state0);
				line0->type = do_spawn_sweep;
				//hurtbox life time
				line0->time0 = 10.0f;
				//relative position from looking_direction
				line0->p = V3(0, 20, 4);
				line0->distance = 20;
				line0->relative_to_dir = 1;
				line0->size = V3(5, 5, 5);
			}

			//NOTA: Las condiciones podrían ser parte de todas las líneas
			//y do_condition usarse cuando se quieran capturar multiples de estas.
			line0 = brain_add_state_line(brain, state0);
			line0->type = do_condition;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
				condition->frame_at = 0;
				condition->frame_duration = 1;

				line0 = brain_add_state_line(brain, state0);
				line0->type = do_add_speed_to_facing;
				line0->speed = 2.0f;
			}

			line0 = brain_add_state_line(brain, state0);
			line0->type = do_switch_state;
			line0->state_index = 0;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
				condition->frame_at = 5;
				condition->frame_duration = 1;
			}


		}
		//slash_attack2()
		//{
		//	spawn_melee_hurtbox(params);
		//	if(frame_step_duration(0, 1))
		//	{
		//		add_speed_to_facing(ent, 2.0f);
		//	}
		//	if(frame_step_duration(5, 1))
		//	{
		//		switch_state(ent, player_state_idle);
		//	}
		//}
		//3. attack 2
		{
			state_node *state0 = brain_add_state(brain);

			state_line *line0 = brain_add_state_line(brain, state0);
			line0->type = do_spawn_melee_hurtbox;
			//hurtbox life time
			line0->time0 = 10.0f;
			//relative position from looking_direction
			line0->p = V3(0, 20, 4);
			line0->relative_to_dir = 1;
			line0->size = V3(5, 5, 5);

			line0 = brain_add_state_line(brain, state0);
			line0->type = do_switch_state;
			line0->state_index = 0;
			line0->next_line_count = 1;
			{
				state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
				condition->frame_at = 5;
				condition->frame_duration = 1;
			}


		}
		/*

		   -idle
		   {
		       -switch_animation:(idle)
		   }
		   -loop
		   {
		       -if(hit_stronk) switch_state
			   -if(hit_very_stronk) switch_state
			   -switch_anim(charge)
			   -wait 0.5
		   }
		   -switch_state(normal)
		*/
		//actions
		{
			//0.move
			{
				state_action *action = brain_add_action(brain);
				state_action_line *action_line = 0;
				//face south
				action_line = brain_add_action_line(brain, action, al_move);
				action_line->face_dir = saf_s;
				////wait
				//f32 ms = 1000;
				//action_line = brain_add_action_line(brain, action, al_wait);
				//action_line->time_total = 2.0f * ms;
				////move
				//action_line = brain_add_action_line(brain, action, al_move_to_facing);
				//action_line->time_total = 1.0f * ms;
				////face north 
				//action_line = brain_add_action_line(brain, action, al_face_dir);
				//action_line->face_dir = saf_n;
				////wait
				//action_line = brain_add_action_line(brain, action, al_wait);
				//action_line->time_total = 2.0f * ms;
				////move
				//action_line = brain_add_action_line(brain, action, al_move_to_facing);
				//action_line->time_total = 1.0f * ms;
			}
			//1.jump
			{
				state_action *action = brain_add_action(brain);
				state_action_line *action_line = 0;
				//face south
				action_line = brain_add_action_line(brain, action, al_jump);
				////wait
				//f32 ms = 1000;
				//action_line = brain_add_action_line(brain, action, al_wait);
				//action_line->time_total = 2.0f * ms;
				////move
				//action_line = brain_add_action_line(brain, action, al_move_to_facing);
				//action_line->time_total = 1.0f * ms;
				////face north 
				//action_line = brain_add_action_line(brain, action, al_face_dir);
				//action_line->face_dir = saf_n;
				////wait
				//action_line = brain_add_action_line(brain, action, al_wait);
				//action_line->time_total = 2.0f * ms;
				////move
				//action_line = brain_add_action_line(brain, action, al_move_to_facing);
				//action_line->time_total = 1.0f * ms;
			}
		}
		//reaction
		{
			//1.jump transition
		}

		ent->brain = end_brain_creation(program, bc);
	}

	return(ent);
}

static world_entity * 
game_create_test_entity(program_state *program)
{
	world_entity *ent = game_create_entity(program);
	ent->flags |= coso_hitpoints;
	ent->hitpoints = 3;
	ent->body = body_create(program);
	ent->looking_direction.y = -1;
	//body params
	{
		game_body *body = ent->body;
//		program->controlling_body = body;
		//set stats
		body->speed = 1.2f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 55; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 0;
		body->p.y = 0;
	}
	//brain
	{
		game_brain_creation bc = begin_brain_creation(program);
		state_main *brain = &bc.brain;
		brain->type = 1;

		{
			//state 0,look at random directions
			{
				state_node *state0 = brain_add_state(brain);

				//face random dir at
				state_line *line0 = brain_add_state_line(brain, state0);
				line0->type = do_condition;
				line0->next_line_count = 1;
				{
					//if frame >= 0 && frame < 1 add speed
					state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
					condition->frame_at = 0;
					condition->frame_duration = 1;

					line0 = brain_add_state_line(brain, state0);
					line0->type = do_face_random_dir;
				}
				//face random dir at
				line0 = brain_add_state_line(brain, state0);
				line0->type = do_condition;
				line0->next_line_count = 1;
				{
					//if frame >= 0 && frame < 1 add speed
					state_trigger *condition = brain_add_line_condition(brain, line0, bt_frame_step_duration);
					condition->frame_at = 100;
					condition->frame_duration = 1;

					line0 = brain_add_state_line(brain, state0);
					line0->type = do_reset_frame_step;
				}
				line0 = brain_add_state_line(brain, state0);
				line0->type = do_switch_state;
				line0->state_index = 1;
				{
					//if frame >= 0 && frame < 1 add speed
					state_trigger *condition = brain_add_line_condition(brain, line0, bt_player_distance_less_than);
					condition->radius = 200;
				}
			}
			//state 1, face player if close
			{
				state_node *state1 = brain_add_state(brain);

				//switch to default state if player is far enough
				state_line *line0 = brain_add_state_line(brain, state1);
				line0->type = do_switch_state;
				line0->state_index = 0;
				{
					//
					state_trigger *condition = brain_add_line_condition(brain, line0, bt_player_distance_less_than);
					condition->radius = 200;
					condition->not = 1;
				}
				//walk away from player if close
				{
				}
				line0 = brain_add_state_line(brain, state1);
				line0->type = do_face_player;
			}

			
		}

		ent->brain = end_brain_creation(program, bc);
		program->test_focused_brain = ent->brain;
	}

	return(ent);
}

static world_entity * 
game_create_test_entity2(program_state *program)
{
	world_entity *ent = game_create_entity(program);
	ent->body = body_create(program);
	ent->looking_direction.y = -1;
	//body params
	{
		game_body *body = ent->body;
//		program->controlling_body = body;
		//set stats
		body->speed = 0.3f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 55; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 0;
		body->p.y = 0;
	}
	//brain
	{
		game_brain_creation bc = begin_brain_creation(program);
		state_main *brain = &bc.brain;

		

		ent->brain = end_brain_creation(program, bc);
	}

	return(ent);
}

static world_entity * 
game_create_new_entity(program_state *program, coso_flags flags)
{
	world_entity *ent = game_create_entity(program);
	ent->body = body_create(program);
	ent->looking_direction.y = -1;
	ent->flags = flags;
	//body params
	{
		game_body *body = ent->body;
//		program->controlling_body = body;
		//set stats
		body->speed = 0.3f;
		body->z_speed = 2.7f;
		body->speed_max = 4.2f;
		body->weight = 55; 
		body->shape.size.x = 10;
		body->shape.size.y = 10;
		body->shape.size.z = 10;
		body->shape.p.z = body->shape.size.z * 0.5f;
		body->p.x = 0;
		body->p.y = 0;
	}
	//brain
	{
		game_brain_creation bc = begin_brain_creation(program);
		state_main *brain = &bc.brain;

		

		ent->brain = end_brain_creation(program, bc);
	}

	return(ent);
}


static void
update_render_entities(program_state *program, program_input *input, game_renderer *game_renderer, render_commands *debug_commands, f32 dt)
{
//	render_commands *debug_commands = render_commands_begin_default(game_renderer);

	for(world_entity *ent = program->first_entity; ent; ent = ent->next)
	{
		vec3 ent_p = body_shape_p(ent->body);
		if(!(ent->flags & coso_render))
		{
			render_draw_cube(debug_commands,
					ent_p,
					ent->body->shape.size,
					V4(255, 255, 0, 255));
		}
		else
		{
			model_animate_and_render(debug_commands, &ent->model, ent_p, ent->looking_direction, dt);
		}
		if(ent->flags & coso_move_with_parent && ent->parent)
		{
	//		vec3 p0 = ent->body->p;
	//		vec3 p1 = ent->parent->body->p;
			vec3 pr = vec3_add(ent->relative_p, ent->parent->body->p);
			ent->body->p = pr;

			//sweeep attack!
			if(ent->flags & coso_damage)
			{
				if(ent->dmg.sweep)
				{

					if(ent->dmg.sweep_angle >= ent->end_angle)
					{
						request_entity_delete(program, ent);
					}

					f32 angular_speed = 0.2f;
					ent->dmg.sweep_angle += angular_speed;
					f32 a = (ent->dmg.sweep_angle * ent->side) + ent->start_angle;
					vec2 relative_p2 = {cos32(a) * ent->distance, sin32(a) * ent->distance};

					ent->relative_p.x = relative_p2.x;
					ent->relative_p.y = relative_p2.y;

				}
			}
		}
		//draw looking direction from origin to looking direciton
		{
			vec2 ld = vec2_scale(ent->looking_direction, 20.0f);
			vec3 p0 = ent_p;
			vec3 p1 = vec3_vec2_add_xy(p0, ld);
			render_draw_line_up(
					debug_commands,
					p0,
					p1,
					V4(255, 255, 255, 255),
					0.5f);

		}
		detect_near_entities(program, ent);
		if(ent->brain)
		{
//			brains_update(program, input, ent, dt);
			run_brain(program, input, ent ,dt);
		}
		render_Circle(debug_commands,
				ent->body->p,
				V3(1, 0, 0),
				V3(0, 1, 0),
				100,
				.5f,
				V4(255, 255, 255, 255));
		if(ent->flags & coso_lifetime)
		{
			if(ent->life_time <= 0.0f)
			{
				request_entity_delete(program, ent);
			}
			ent->life_time -= dt;
		}
		else if(ent->flags & coso_frame_lifetime)
		{
			if(ent->life_time <= 0.0f)
			{
				request_entity_delete(program, ent);
			}
			ent->life_time = (f32)((i32)ent->life_time - 1);

		}
		//hitpoints
		if(ent->flags & coso_hitpoints)
		{
			if(ent->hitpoints <= 0)
			{
				request_entity_delete(program, ent);
			}
		}
	}
	program->detections_count = 0;

//	render_commands_end(debug_commands);
}

static world_entity *
get_child_with_id1(world_entity *parent, u32 id1)
{
	world_entity *result = 0;
	for(world_entity *child = parent->first;
			child;
			child = child->next_sibling)
	{
		if(child->id.id1 == id1)
		{
			result = child;
		}
	}
	return(result);
}
