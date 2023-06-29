static void
editor_franim_allocate(
		s_game_editor *game_editor)
{
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;

	franim_editor->animation_max = 100;
	franim_editor->animation_names = editor_name_chunks_allocate(
			&game_editor->area, franim_editor->animation_max, 64);
	franim_editor->dyarrays_area = memory_dyarray_area_create(
			&game_editor->area, franim_editor->animation_max * sizeof(editor_sprite_animation));
	franim_editor->animations = memory_area_push_array(
			&game_editor->area, editor_sprite_animation, franim_editor->animation_max);

}

static editor_sprite_animation *
editor_franim_add_animation(s_game_editor *game_editor)
{
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;
	editor_sprite_animation *new_animation = 0;
	if(franim_editor->animation_count < franim_editor->animation_max)
	{
		new_animation = franim_editor->animations +
			franim_editor->animation_count;
		//create frames dyarray
		new_animation->frames = memory_dyarray_create(
				franim_editor->dyarrays_area, sprite_animation_frame, 5, 5);
		//clear used memory
		memory_clear(new_animation, sizeof(new_animation));
		//add name
		editor_name_chunks_addf(&franim_editor->animation_names,
				"Animation %u", franim_editor->animation_count);
		franim_editor->animation_count++;
	}

	return(new_animation);
}

static void
editor_franim_remove_animation(s_game_editor *game_editor, u32 index)
{
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;
	if(index < franim_editor->animation_count)
	{
		franim_editor->animation_count = e_array_remove_and_shift(
				franim_editor->animations,
				editor_sprite_animation,
				franim_editor->animation_count,
				index);
		editor_name_chunks_remove(&franim_editor->animation_names, index);
	}
}

static void
editor_franim_update_camera(s_editor_state *editor_state,
		                   game_renderer *game_renderer,
						   editor_input *editor_input,
						   f32 dt)
{
	game_renderer->camera_position.x = 0;
	game_renderer->camera_position.y = 0;
	game_renderer->camera_position.z = 0;
}
static sprite_animation_frame *
editor_franim_add_frame(
		editor_sprite_animation *animation)
{
	sprite_animation_frame *frame = memory_dyarray_push(animation->frames);
	animation->frame_count++;
	return(frame);
}

static void
editor_franim_quick_set(
		s_game_editor *game_editor)
{
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;
	editor_sprite_animation *water0 = editor_franim_add_animation(game_editor);
	water0->base.frame_w = 12;
	water0->base.frame_h = 12;
	for(u32 f = 0; f < 6; f++)
	{
		sprite_animation_frame *frame0 = editor_franim_add_frame(water0);
		frame0->fx = 17 + f;
		frame0->fy = 6;
		frame0->totalMiliseconds = 200;
	}
}

static void
editor_franim_update_render(s_editor_state *editor_state,
		                   game_renderer *game_renderer,
						   editor_input *editor_input,
						   f32 dt)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;

	//display in 2d
	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

//	render_rectangle_2d_xywh(commands, game_renderer->back_buffer_width - 20, 200, 400, 400, V4(0, 255, 0, 255));
	if(franim_editor->animation_is_selected && franim_editor->external_image)
	{
		editor_sprite_animation *sprite_animation = franim_editor->animations + franim_editor->selected_animation_index;
		temporary_area temp_animation_area = temporary_area_begin(&game_editor->area);

		//push temp frames
		sprite_animation->base.frames = memory_area_push_array(
				&game_editor->area, sprite_animation_frame, sprite_animation->frame_count);
		sprite_animation->base.frames_total = sprite_animation->frame_count;
		for(u32 f = 0; f < sprite_animation->frame_count; f++)
		{
			sprite_animation->base.frames[f] = *(sprite_animation_frame *)memory_dyarray_get(sprite_animation->frames, f);
		}

		if(sprite_animation->frame_count)
		{
			render_reproduce_sprite_animation(&sprite_animation->base, dt);
			sprite_animation_frame *frame = sprite_animation->base.frames + sprite_animation->base.current_frame;

			u16 fw = sprite_animation->base.frame_w * 1;
			u16 fh = sprite_animation->base.frame_h * 1;
			u16 fx = (frame->fx * fw) + 1 + (2 * frame->fx);
			u16 fy = (frame->fy * fh) + 1 + (2 * frame->fy);

			render_sprite_2d(commands,
					&franim_editor->external_image->asset_key->image,
					V2(400, 400),
					V2(fw * 4.0f, fh * 4.0f),
					fx,
					fy,
					fw,
					fh,
					V4(255, 255, 255, 255));
		}
		temporary_area_end(&temp_animation_area);
	}

	render_commands_end(commands);
}


static inline void
editor_franim_update_render_ui(s_editor_state *editor_state,
		                      game_renderer *game_renderer,
							  editor_input *editor_input)
{
	game_ui *ui = editor_state->ui;
	s_game_editor *game_editor = &editor_state->editor;
	s_frame_animation_editor *franim_editor = &game_editor->frame_animation;

	b32 new_clicked = 0;
	b32 save_clicked = 0;
	b32 load_clicked = 0;
	b32 save_as_clicked = 0;

	ui_node *tool_bar;
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
				new_clicked = ui_button(ui, "New#franim");
				ui_space(ui, ui_size_specified(4.0f, 1.0f));
				load_clicked = ui_button(ui, "Load#franim");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_clicked = ui_button(ui, "Save#franim");
				ui_space_specified(ui, 4.0f, 1.0f);
				save_as_clicked = ui_button(ui, "Save as#franim");
				ui_space_specified(ui, 4.0f, 1.0f);
				if(ui_button(ui, "Quick set (test)#franim"))
				{
					editor_franim_quick_set(
							game_editor);
				}
				ui_space_ppct(ui, 1.0f, 0.0f);
				ui_extra_flags(ui, node_border | node_text_centered)
				{
					b8 base = ui_selectable(ui, franim_editor->tab == 0, "Base#franim");
					ui_space_specified(ui, 4.0f, 1.0f);
					b8 properties = ui_selectable(ui, franim_editor->tab == 1, "Properties#franim");
					ui_space_specified(ui, 4.0f, 1.0f);

					if(base)
					{
						franim_editor->tab = 0;
					}
					else if(properties)
					{
						franim_editor->tab = 1;
					}
				}
			}
		}
	}
	switch(franim_editor->tab)
	{
		case 0:
			{
				ui_set_row(ui)
				{
					//panel
					ui_node *animation_list_panel = 0;
					ui_set_w_specified(ui, 210.0f, 0.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						animation_list_panel = ui_node_box(ui, 0);
					}
					ui_set_parent(ui, animation_list_panel)
					{
						//display image name and add button
						ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							u8 *img_name = "";
							if(franim_editor->external_image)
							{
								img_name = franim_editor->external_image->path_and_name;
							}
							ui_textf(ui, "Image: %s", img_name);
							if(ui_button(ui, "Load#franim_external_image"))
							{
								er_explorer_set_process(game_editor, er_explorer_load, "Load image");
							}
						}
						//frame animation list
						ui_node *frame_animation_list_node = 0;
						ui_node *frame_animation_list_top_bar = 0;
						ui_set_w_specified(ui, 200.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
						{
							ui_set_h_em(ui, 2.0f, 1.0f)
							{
								frame_animation_list_top_bar = ui_label(ui, 0);
								frame_animation_list_top_bar->padding_x = 2;
								frame_animation_list_top_bar->padding_y = 2;
								frame_animation_list_top_bar->layout_axis = 0;
							}
							frame_animation_list_node = ui_box_with_scroll(ui, "test#franim");
							//add and remove buttons
							ui_set_parent(ui, frame_animation_list_top_bar) ui_extra_flags(ui, node_border) ui_set_wh_text(ui, 4.0f, 1.0f)
							{
								if(ui_button(ui, "+#franim"))
								{
									editor_franim_add_animation(game_editor);
								}
								ui_space_specified(ui, 4.0f, 1.0f);
								ui_push_disable_if(ui, !franim_editor->animation_is_selected || franim_editor->animation_count == 0);
								if(ui_button(ui, "x#franim") && franim_editor->animation_is_selected)
								{
									editor_franim_remove_animation(game_editor, franim_editor->selected_animation_index);
									if(franim_editor->animation_count)
									{
										franim_editor->selected_animation_index--;
									}
									else
									{
										franim_editor->animation_is_selected = 0;
									}
								}
								ui_pop_disable(ui);
							}
						}
						ui_set_parent(ui, frame_animation_list_node) ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f ,1.0f)
						{
							//		editor_ui_name_chunks_selectables(
							//				ui,
							//				&franim_editor->selected_animation_index,
							//				franim_editor->animation_names,
							//				franim_editor->animation_count);
							//	editor_ui_selectable s = editor_ui_selectable_name(
							//			ui,
							//			franim_editor->animation_names,
							//			franim_editor->animation_count)
							//	while(editor_ui_selectable_name_continue(&s))
							//	{
							//		if(s.clicked)
							//		{
							//		}
							//		if(s.index == ignored_index)
							//		{
							//			continue;
							//		}
							//	}
							for(u32 f = 0; f < franim_editor->animation_count; f++)
							{
								u8 *name = franim_editor->animation_names.chunks[f].name;
								b32 active = franim_editor->animation_is_selected && franim_editor->selected_animation_index == f;
								if(ui_selectablef(ui, active, "%s##frame_animation_list%u", name, f))
								{
									franim_editor->animation_is_selected = 1;
									franim_editor->selected_animation_index = f;
								}
							}
						}
					}

				}
			}break;
		case 1:
			{
				ui_node *animation_frames_panel = 0;
				ui_node *animation_properties_panel = 0;
				ui_set_row(ui)
				{
					ui_push_color(ui, ui_color_background, V4(15, 0, 5, 255));
					ui_set_w_specified(ui, 200.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
					{
						animation_frames_panel = ui_box_with_scroll(ui, "animation_frames_panel");
					}
					ui_pop_color(ui, ui_color_background);

					ui_space_ppct(ui, 1.0f, 0.0f);
					ui_set_wh_soch(ui, 1.0f)
					{
						animation_properties_panel = ui_node_box(ui, "animation_properties_panel");
						//ui_box_with_scroll(ui, "animation_properties_panel");
					}
				}
				editor_sprite_animation *editor_animation = 0;
				if(franim_editor->animation_is_selected)
				{
					editor_animation = franim_editor->animations + 
						franim_editor->selected_animation_index;
				}
				//animation_frames
				f32 frames_height = 16.0f;
				ui_set_parent(ui, animation_frames_panel) ui_set_h_specified(ui, frames_height, 1.0f) ui_set_w_ppct(ui, 1.0f, 0.0f)
				{
					if(editor_animation)
					{
						ui_set_row(ui)
						{
							//frames
							ui_set_column(ui)
							for(u32 f = 0; f < editor_animation->frame_count; f++)
							{
								ui_push_id_u32(ui, f);
								sprite_animation_frame *frame = memory_dyarray_get(editor_animation->frames, f);
								//test
								ui_node *frame_selectable = ui_create_node(ui, node_border | node_clickeable, 0);
								frame_selectable->layout_axis = 0;
								ui_set_parent(ui, frame_selectable)
								{
									ui_textf(ui, "%u. ms:%u", f, frame->totalMiliseconds);
									//							ui_space_ppct(ui, 1.0f, 0.0f);
									ui_set_w_specified(ui, 16.0f, 1.0f)
									{
										ui_create_node(ui, node_border, 0);
									}
								}
								ui_pop_id(ui);
							}
							//timer arrow
							ui_set_w_specified(ui, 16.0f, 1.0f) ui_set_h_specified(ui, frames_height * editor_animation->frame_count, 0.0f)
							{
								ui_node *arrow_node = ui_create_node(ui, node_border, 0);
								//arrow timer cursor
								if(editor_animation->frame_count)
								ui_set_parent(ui, arrow_node) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "<-");
								}
							}
						}
					}
				}

				//animation properties
				ui_set_parent(ui, animation_properties_panel) ui_set_wh_text(ui, 4.0f, 1.0f)
				{
					ui_text(ui, "Properties");
					if(franim_editor->animation_is_selected)
					{
						ui_set_row(ui)
						{
							//edit name
							ui_text(ui, "Name");
							ui_set_w_em(ui, 16.0f, 1.0f)
							{
								editor_ui_input_text_name(ui, &franim_editor->animation_names, franim_editor->selected_animation_index, "selected_animation_edit_name");
							}
						}
					}
				}
			}break;
	}

	if(editor_resource_explorer_process_completed(game_editor, "Load image"))
	{
		franim_editor->external_image = er_look_for_resource(game_editor, editor_resource_explorer_output(game_editor));
		if(!franim_editor->external_image->type == asset_type_image)
		{
			franim_editor->external_image = 0;
		}
	}
}
