#define em_BONE_RADIUS 0.5f
//data
enum properties_ui_focus_at{
	properties_focus_nothing = 0,
	properties_focus_sprite_sheets,
	properties_focus_frame_lists,
};
/*

   Displaying edges should be a check for view mode.
   Animation editor:
keyframes:
the editor uses editor_animation_keyframe which contains connections to their
next sibling at its current "row" (based on sprite or bone index) and "column" 
(based on frame)
        prev <=>  column <=> next 
                   ^
                   ||
prev              prev
^                  ^
||                 ||
row <=> prev <==> keyframe <==> next
||                 ||
.   			   .
next               next 

The reason of this is because If I want to move a group of keyframes, then I can drag a 
column and move or inherit (to other empty columns) the keyframes connected to them
*/

#define editor_ANIMATIONCAPACITY 100
#define editor_KEYFRAMECAPACITY 4000
#define editor_CLIPCAPACITY 4000
#define editor_UV_CAPACITY 4000

#define editor_LOADEDMODEL_KEYFRAMECAPACITY 50
#define editor_LOADEDMODEL_CLIPCAPACITY 50
#define editor_LOADEDMODEL_UVS_CAPACITY 40
#define editor_model_selected_sprites_count(model_editor) (model_editor->cursor_memory.selected_meshes_count)

#define em_bone_name(model_editor, index) (model_editor->bone_name_chunks.chunks[index].name)
#define em_bone_from_keyframe(model_editor, keyframe) (model_editor->bones + keyframe->base.bone_index)
//
//general useful functions
//
static quaternion
editor_rotate_quaternion(
		quaternion q,
		f32 r,
		u32 axis)
{
	f32 axis_to_apply[3] = {0};
	axis %= 3;
	axis_to_apply[axis] = 1;
	quaternion qr = quaternion_rotated_at(
			axis_to_apply[0],
			axis_to_apply[1],
			axis_to_apply[2],
			r * PI);
	q = quaternion_mul(q, qr);
	return(q);
}

static void
em_display_and_select_model_nodes(
		s_game_editor *game_editor,
		render_commands *commands,
		vec3 ray_origin,
		vec3 ray_dir,
		f32 bone_ray_hit_distance,
		b32 *bone_selected,
		u32 *selected_bone_index)
{
	s_model_editor *model_editor = &game_editor->model;
	vec4 node_selected     = {160, 0, 0, 255};
	vec4 node_normal_color = {25, 45, 0, 255};
	u32 bone_count = model_editor->bone_count;
	//render nodes and linked nodes
	b16 bone_hit_by_ray = 0;
	b16 bone_hit_index = 0;
	f32 bone_radius = em_BONE_RADIUS;
	u32 in_camera_mode = model_editor->in_camera_mode; 
	// display and select nodes
	for(u32 b = 0;
			b < bone_count;
			b++)
	{
		model_bone *current_bone = model_editor->loaded_model_pose.bones + b;
		vec3 bone_position = current_bone->transformed_p;

		//if in transforms preview mode.
		vec3 node_rotation = {current_bone->rotation_x,
			current_bone->rotation_y,
			current_bone->rotation_z};

		vec3 node_x_axis = {1, 0, 0};
		vec3 node_y_axis = {0, 1, 0};
		vec3 node_z_axis = {0, 0, 1};
		matrix3x3 hierarchy_rotation_matrix = matrix3x3_rotation_scale(
				node_rotation.x, node_rotation.y, node_rotation.z);
		node_x_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 0);
		node_y_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 1);
		node_z_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 2);

		ray_casted_info ray_node_result = ray_circle_upfront_result(ray_origin,
				ray_dir,
				bone_position,
				bone_radius);
		//display
		//bone is closer and got hit
		u32 hit_bone = ray_node_result.hits != 0;
		u32 bone_hot    = 0;
		if(hit_bone && ray_node_result.distance_to_plane < bone_ray_hit_distance)
		{
			bone_hit_by_ray = 1;
			bone_hit_index = b;
			bone_ray_hit_distance = ray_node_result.distance_to_plane;
			bone_hot = 1;
		}

		vec4 node_color = node_normal_color;
		//select or modify node color based on the current state or mode
		if(*bone_selected && (*selected_bone_index) == b)
		{
			node_color = node_selected;
		}
		else if(bone_hot)
		{
			node_color.w = 220;
		}
		if(in_camera_mode)
		{
			node_color.w = 40;
		}

		//if this is not the root bone
		if(b != 0)
		{
			model_bone *linked_node = model_editor->loaded_model_pose.bones + current_bone->parent;
			vec3 linked_node_position = linked_node->transformed_p;
			f32 line_alpha = 255;
			if(in_camera_mode)
			{
				line_alpha = 40;
			}

			render_draw_line_up(commands,
					bone_position,
					linked_node_position,
					node_color,
					0.08f);
		}

		//render cube after line
		//	render_draw_cube(commands,
		//			bone_position, 
		//			vec3_all(bone_radius),
		//			node_color);
		render_circle_upfront(commands,
				bone_position, 
				bone_radius,
				.2f,
				node_color);

	}
}

static b32
em_ui_display_node_list(
		game_ui *ui,
		s_game_editor *game_editor,
		u32 *selected_bone_index)
{
	s_model_editor *model_editor = &game_editor->model;
	ui_node *node_list_node = 0;
	b32 clicked = 0;

	ui_set_h_em(ui, 12.0f, 1.0f) ui_set_w_ppct(ui, 1.0f, 0.0f)
	{
		node_list_node = ui_content_box_ex(ui, node_scroll_y, "select bone");
	}
	ui_set_parent(ui, node_list_node)
	{
		f32 advance_x = 0;

		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_set_row(ui) ui_extra_flags(ui, node_text_centered)
				for(u32 b = 0; b < model_editor->bone_count; b++)
				{
					u8 *bone_name = game_editor->model.bone_name_chunks.chunks[b].name;
					f32 text_w = ui_get_text_size(ui, bone_name).x;
					//count the padding
					advance_x +=  12 + 8 + text_w;
					if(advance_x > node_list_node->size_x)
					{
						ui_pop_row(ui);
						ui_push_row(ui, 1.0f, 1.0f);
						advance_x = 0;
					}

					b32 active = selected_bone_index && b == *selected_bone_index;
					if(ui_selectablef(ui, active, "%s##model_node_selection#%u", bone_name, b))
					{
						if(selected_bone_index) *selected_bone_index = b;
						clicked = 1;
					}
					ui_space_specified(ui, 4.0f, 1.0f);
				}
		}

		//
	}
	return(clicked);
}

static void
editor_model_dump_to_file(
		s_editor_state *editor_state)
{
	s_game_editor *editor = &editor_state->editor;
	s_model_editor *model_editor = &editor->model;
	temporary_area temp_area = temporary_area_begin(&editor->area);
	stream_data file_stream = stream_Create(&editor->area);
	{
		//save data
		stream_pushf(&file_stream,"sprite_sheets (%u):\n ", model_editor->sprite_sheets_count);
		stream_pushf(&file_stream, "{\n");
		for(u32 s = 0; s < model_editor->sprite_sheets_count; s++)
		{
			game_resource_attributes *sprite_sheet = model_editor->sprite_sheets[s];
			stream_pushf(&file_stream, "    %s\n", sprite_sheet->path_and_name);
		}
		stream_pushf(&file_stream, "}");
		stream_pushf(&file_stream, "frame_lists (%u)", model_editor->frame_list_count);
		stream_pushf(&file_stream, "{\n");
		for(u32 f = 0; f < model_editor->frame_list_count; f++)
		{
			editor_model_frame_list *frame_list = model_editor->frame_lists + f;
			stream_pushf(&file_stream, "%u.\n{\n", f);
			u8 *name = model_editor->frame_list_names.chunks[f].name;
			stream_pushf(&file_stream, "name:%s\n", name);
			stream_pushf(&file_stream, "uvs_count:%u\n", frame_list->uvs_count);
			stream_pushf(&file_stream, "sprite_index:%u\n", frame_list->uvs_count);
			stream_pushf(&file_stream, "uvs:\n");
			for(u32 u = 0; u < frame_list->uvs_count; u++)
			{
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, u);
				u32 fx = 0;
				u32 fy = 0;
				u32 fw = 0;
				u32 fh = 0;
				render_fill_frames_from_uvs(
						512, 512, uvs->uv0, uvs->uv1, uvs->uv2, uvs->uv3,
						&fx, &fy, &fw, &fh);

				stream_pushf(&file_stream, "%u. {%u, %u, %u, %u}\n", u, fx, fy, fw, fh);
			}
			stream_pushf(&file_stream, "}");

		}
		stream_pushf(&file_stream, "{\n");
//
//		ui_textf(ui, "animations(%u)", model_editor->animation_count);
//		for(u32 a = 0; a < model_editor->animation_count; a++)
//		{
//			editor_animation *editor_animation = model_editor->animations + a;
//			model_animation *animation = &model_editor->animations[a].base;
//			ui_text(ui, "{");
//			ui_set_row(ui) 
//			{
//				ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
//				{
//					ui_set_row(ui)
//					{
//						ui_text(ui, "name: "); 
//						ui_text(ui, model_editor->animation_name_chunks.chunks[a].name);
//					}
//					ui_textf(ui, "transform keyframes (%u):", animation->keyframe_count);
//					for(editor_animation_keyframe_group *group = editor_animation->first_keyframe_group;
//							group;
//							group = group->next)
//					{
//						for(u32 k = 0; k < group->keyframe_count; k++)
//						{
//							ui_text(ui, "{");
//							ui_set_row(ui) 
//							{
//								ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
//								{
//									model_animation_keyframe *kf = memory_dyarray_get(
//											group->keyframes, k);
//									ui_textf(ui, "frame_start: %u", kf->frame_start);
//									ui_set_row(ui)
//									{
//										u8 *types[] = {"mesh", "billboard"};
//										u8 *type = kf->type > 1 ? "MORE THAN SPECIFIED" : types[kf->type];
//										ui_text(ui, "Type: ");
//										ui_text(ui, type);
//									}
//									ui_textf(ui, "bone_index: %u", kf->bone_index);
//								}
//							}
//							ui_text(ui, "}");
//						}
//					}
//					ui_textf(ui, "frame_groups");
//					for(editor_animation_keyframe_group *group = editor_animation->first_frame_keyframe_group;
//							group;
//							group = group->next)
//					{
//						for(u32 k = 0; k < group->keyframe_count; k++)
//						{
//							ui_text(ui, "{");
//							ui_set_row(ui) 
//							{
//								ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
//								{
//									model_animation_keyframe *kf = memory_dyarray_get(
//											group->keyframes, k);
//									ui_textf(ui, "frame_start: %u", kf->frame_start);
//									ui_textf(ui, "sprite_index: %u", kf->mesh_index);
//									ui_textf(ui, "frame_list_index: %u", kf->frame_list_index);
//								}
//							}
//							ui_text(ui, "}");
//						}
//					}
//				}
//			}
//			ui_text(ui, "}");
//		}
	}
	u8 *model_name = model_editor->editing_model ?
		model_editor->editing_model->path_and_name : "unnamed_model";
	u8 file_name[256] = {0};
	string_concadenate(model_name, ".txt",file_name, sizeof(file_name));
	platform_api *platform = editor_state->platform;
	platform_write_stream_to_file(
			platform,
			&file_stream,
			file_name);
	//write all data to file
	temporary_area_end(&temp_area);
}

static render_texture *
editor_model_get_texture(
		s_model_editor *model_editor,
		u32 index)
{
	render_texture *result = 0;
	if(index < model_editor->sprite_sheets_count)
	{
		result = &model_editor->sprite_sheets[index]->asset_key->image;
	}
	return(result);
}

static inline void
editor_model_change_frame_list_frame_count(
		editor_model_frame_list *frame_list,
		u32 orientation_count)
{
	memory_dyarray_set_count(frame_list->mesh_frames, orientation_count);
	frame_list->total_frames_count = orientation_count;
}

static inline void
editor_model_add_frame_list_frame(
		editor_model_frame_list *frame_list)
{
	editor_model_change_frame_list_frame_count(
			frame_list, frame_list->total_frames_count + 1);
}
static inline void
editor_model_remove_frame_list_frame(
		editor_model_frame_list *frame_list,
		u32 index)
{
	if(index < frame_list->total_frames_count)
	{
		memory_dyarray_remove_at(frame_list->mesh_frames, index);
		frame_list->total_frames_count--;
	}
}


static editor_model_frame_list * 
editor_model_add_frame_list(
		s_game_editor *game_editor)
{
	s_model_editor *model_editor = &game_editor->model;
	editor_model_frame_list *frame_list = 0;
	if(model_editor->frame_list_max > model_editor->frame_list_count)
	{
		frame_list = model_editor->frame_lists +
			model_editor->frame_list_count;

		frame_list->sprite_index = 0;
		frame_list->uvs_count = 1;
		frame_list->mesh_frames = memory_dyarray_create(model_editor->dyarrays_area,
				sprite_orientation,
				1,
				4);
		frame_list->total_frames_count = 1;
		frame_list->editor_selected_frame = 0;
		//add one as default
		memory_dyarray_clear_and_push(frame_list->mesh_frames);
		//frame_list->base = model_editor->model_frame_lists + model_editor->frame_list_count;
		editor_name_chunks_addf(&model_editor->frame_list_names, "Frame list %u", model_editor->frame_list_count);
		model_editor->frame_list_count++;
	}

	return(frame_list);
}

static void
editor_model_remove_frame_list(
		s_game_editor *game_editor,
		u32 index)
{
	s_model_editor *model_editor = &game_editor->model;
	if(index < model_editor->frame_list_count)
	{
		editor_model_frame_list *frame_list = model_editor->frame_lists + index;
		memory_dyarray_delete(frame_list->mesh_frames);
		editor_name_chunks_remove(
				&model_editor->frame_list_names, index);
		//shift array
		for(i32 f = index; f < model_editor->frame_list_count - 1; f++)
		{
			model_editor->frame_lists[f] = model_editor->frame_lists[f + 1];
		}
		model_editor->frame_list_count--;

		//make sure all keyframes point to the correct bone for every animation.
		if(model_editor->frame_list_count)
		{
			for(u32 a = 0; a < model_editor->animation_count; a++)
			{
				editor_animation *editor_animation = model_editor->animations + a;
				//go through rows
				for(editor_animation_keyframe_row *keyframe_row = editor_animation->first_frame_row;
						keyframe_row; keyframe_row = keyframe_row->next)
				{
					//go through all keyframes
					for(editor_animation_keyframe *keyframe = keyframe_row->first_keyframe;
							keyframe; keyframe = keyframe->next_in_row)
					{
						if(keyframe->base.frame_list_index >= index)
						{
							keyframe->base.frame_list_index--;
							keyframe->base.frame_list_frame_index = 0;
						}
					}
				}
			}
		}
	}
}

static inline void
editor_model_copy_frame_list(
		s_game_editor *game_editor,
		editor_model_frame_list *frame_list)
{
	s_model_editor *model_editor = &game_editor->model;
	editor_clipboard *clipboard = &model_editor->frame_list_clipboard;
	editor_clipboard_reset(clipboard);
	editor_clipboard_chunk *chunk = editor_clipboard_copy_begin(clipboard);

	editor_clipboard_chunk_push_and_copy_size(chunk, sizeof(*frame_list), frame_list);
	for(u32 u = 0; u < frame_list->uvs_count; u++)
	{
		sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, u);
		editor_clipboard_chunk_push_and_copy_size(chunk, sizeof(*uvs), uvs);
	}

	editor_clipboard_copy_end(clipboard, chunk);
}

static inline void
editor_model_paste_frame_lists(
		s_game_editor *game_editor)
{
#if 0
	s_model_editor *model_editor = &game_editor->model;
	editor_clipboard *clipboard = &model_editor->frame_list_clipboard;
	for(editor_clipboard_chunk *chunk = clipboard->first_chunk;
			chunk; chunk = chunk->next)
	{
		u32 read_offset = 0;
		editor_model_frame_list *frame_list_clipboard = editor_clipboard_chunk_read_size(chunk, sizeof(editor_model_frame_list), &read_offset);
		editor_model_frame_list *frame_list_slot = editor_model_add_frame_list(
				game_editor);
		//not enough space
		if(!frame_list_slot)
		{
			break;
		}
		editor_model_change_frame_list_frame_count(frame_list_slot, frame_list_clipboard->uvs_count);
		{
			memory_dyarray *uvs_bak = frame_list_slot->mesh_frames;
			*frame_list_slot = *frame_list_clipboard;
			frame_list_slot->mesh_frames = uvs_bak;
		}
		//set uvs
		for(u32 u = 0; u < frame_list_slot->uvs_count; u++)
		{
			uvs *uvs_clipboard = editor_clipboard_chunk_read_size(chunk, sizeof(uvs), &read_offset);
			uvs *uvs = memory_dyarray_get(frame_list_slot->mesh_frames, u);
			*uvs = *uvs_clipboard;
		}
	}
#endif
}

static inline model
editor_model_make_model(s_game_editor *game_editor)
{
	s_model_editor *model_editor = &game_editor->model;
	model result_model = {0};
	//set the dummy editing model data
	result_model.sprite_sheets_a = game_editor->model.sprite_sheets_as_asset;
	result_model.bone_count = game_editor->model.bone_count;
	result_model.sprite_count = game_editor->model.sprite_count;
	result_model.sprite_sheet_count = game_editor->model.sprite_sheets_count;
	result_model.uvs_count = game_editor->model.uvs_count;
	result_model.mesh_frame_list = game_editor->model.model_frame_lists;
	result_model.frame_list_count = game_editor->model.frame_list_count;
	result_model.animation_count = game_editor->model.animation_count;
	result_model.orientation_amount = model_editor->model_orientation_count;

	result_model.keyframe_count = model_editor->keyframe_count;
	result_model.frame_keyframe_count = model_editor->frame_keyframe_count;
	//bones

	result_model.uvs = memory_area_clear_and_push_array(&game_editor->model.per_frame_area,
			sprite_orientation, 100); 
	//push sprite array
	result_model.sprites = memory_area_push_array(&game_editor->model.per_frame_area,
			model_sprite, model_editor->sprite_count);
	//push bone array
	result_model.bones = memory_area_push_array(&game_editor->model.per_frame_area,
			model_bone, model_editor->bone_count);
	model_editor->virtual_node_count = 0;
	u32 loaded_sprites = 0;
	u32 current_sprite_index = 0;
	u32 frames_loaded = 0;
	for(u32 b = 0; b < game_editor->model.bone_count; b++)
	{
		editor_model_bone *editor_bone = game_editor->model.bones + b;
		result_model.bones[b] = editor_bone->base;
		model_bone *bone = result_model.bones + b;
	    *bone = editor_bone->base;
		bone->sprite_count = editor_bone->sprite_count;
		bone->frame_key_count = editor_bone->frame_key_count;
		bone->sprites_at = loaded_sprites;
		//pick sprites and attach them to this node
		for(u32 s = 0; s < editor_bone->sprite_count; s++)
		{
			editor_model_sprite *e_sprite = 0;
			memory_dyarray_get_safe(editor_bone->sprites, e_sprite, s);
			model_sprite *sprite = result_model.sprites + bone->sprites_at + s;
			*sprite = e_sprite->base;
			sprite->frame_at = frames_loaded;
			result_model.sprites[loaded_sprites + s] = *sprite;

			for(u32 u = 0; u < (u32)(sprite->extra_frame_count + 1); u++)
			{
				sprite_orientation *frame = result_model.uvs + (sprite->frame_at + u);
				sprite_orientation *e_frame = 0;
				memory_dyarray_get_safe(e_sprite->uvs, e_frame, u);
				*frame = *e_frame;
			}
			frames_loaded += 1 + sprite->extra_frame_count;
		}
		loaded_sprites += editor_bone->sprite_count;


		if(bone->virtual)
		{
			//pick bone from attached model and mark is as a modificable from this bone
			//model_attachment_data
			game_resource_attributes *attached_model = model_editor->attachments[bone->attached_model_index].model;
			u32 bi = bone->virtual_attachment_bone;
			if(bone->virtual_attachment_bone < model_editor->attachment_count)
			{
				model *a_model = er_model(attached_model);
				if(bi < a_model->bone_count)
				{
					bone->virtual = 1;

					model_virtual_node *vnode = model_editor->virtual_nodes + model_editor->virtual_node_count;
					model_editor->virtual_node_count++;
					vnode->parent = b;
					vnode->child = bi;
				}
			}
		}
	}
	//	for(u32 f = 0; f < loaded_model->bone_count; f++)
	//	{
	//		editor_model_bone *bone = model_editor->bones + f;
	//		for(u32 s = 0; s < bone->sprite_count; f++)
	//		{
	//		}
	//	}


	u32 uvs_used = 0;
	for(u32 f = 0; f < model_editor->frame_list_count; f++)
	{
		editor_model_frame_list *editor_frame_list = game_editor->model.frame_lists + f;
		model_mesh_frame_list *frame_list = model_editor->model_frame_lists + f;
		frame_list->uvs_at = uvs_used;
		frame_list->uvs_count = editor_frame_list->uvs_count;
		frame_list->sprite_index = editor_frame_list->sprite_index;
		frame_list->total_frames_count = editor_frame_list->total_frames_count;

		for(u32 u = 0; u < frame_list->total_frames_count; u++)
		{
			uvs_used++;
			u32 i = frame_list->uvs_at + u;
	//		sprite_orientation editor_frame_list_uvs = 
	//			*(sprite_orientation *)memory_dyarray_get(
	//					editor_frame_list->mesh_frames, u);
			//result_model.uvs[i].uv0 = editor_frame_list_uvs.uv0;
			//result_model.uvs[i].uv1 = editor_frame_list_uvs.uv1;
			//result_model.uvs[i].uv2 = editor_frame_list_uvs.uv2;
			//result_model.uvs[i].uv3 = editor_frame_list_uvs.uv3;
			//result_model.uvs[i].offset = editor_frame_list_uvs.offset;
			//result_model.uvs[i].option = editor_frame_list_uvs.option;
			//result_model.uvs[i].x_rot_index = editor_frame_list_uvs.x_rot_index;
			//result_model.uvs[i].y_rot_index = editor_frame_list_uvs.y_rot_index;
		}
	}


	return(result_model);
}

static inline void
editor_graphics_allocate(s_game_editor *game_editor)
{
	game_editor->model.per_frame_area = memory_area_create_from(&game_editor->area,
			KILOBYTES(256));
	memory_area *editor_area = &game_editor->area;
	s_model_editor *model_editor = &game_editor->model;

	u32 model_sprite_max        = 300;
	u32 bone_max                = 200;
	u32 uvs_max                 = 300;
	u32 animation_max          = editor_ANIMATIONCAPACITY;
	u32 animation_keyframe_max  = editor_KEYFRAMECAPACITY;
	u32 animation_clip_max      = editor_CLIPCAPACITY;

	game_editor->model.cameraDistance = 160;
	game_editor->model.sprite_max = model_sprite_max;
	game_editor->model.boneCapacity = bone_max;
	game_editor->model.camera_rotation_x = 0.250f;


	game_editor->model.animation_max = animation_max;
	game_editor->model.keyframe_max = animation_keyframe_max;
	game_editor->model.uvs_max = editor_UV_CAPACITY;

	game_editor->model.animation_count   = 0;
	game_editor->model.model_foward.y = -1;

	//push for clipboard 
	game_editor->model.keyframe_clipboard_max = 1000;
	game_editor->model.keyframe_clipboard = memory_area_push_array(
			&game_editor->area, model_animation_keyframe, game_editor->model.keyframe_clipboard_max);
	u32 frame_list_clipboard_size = KILOBYTES(256);
	game_editor->model.frame_list_clipboard = editor_clipboard_create(
			memory_area_push_size(&game_editor->area, frame_list_clipboard_size), frame_list_clipboard_size);


	game_editor->model.sprite_clipboard = memory_area_push_array(&game_editor->area, model_sprite, editor_SPRITE_CLIPBOARD_MAX);

	//bones or nodes
	game_editor->model.bones = memory_area_clear_and_push_array(
			&game_editor->area,
			editor_model_bone,
			bone_max);
	//skin or sprites
	game_editor->model.sprites = memory_area_clear_and_push_array(
			&game_editor->area,
			model_sprite,
			model_sprite_max);
	//skin uvs
	game_editor->model.uvs = memory_area_clear_and_push_array(
			&game_editor->area,
			sprite_orientation,
			uvs_max);

	game_editor->model.virtual_node_max = 5;
	game_editor->model.virtual_nodes = memory_area_clear_and_push_array(
			&game_editor->area,
			model_virtual_node,
			game_editor->model.virtual_node_max);
	//sprite sheets
	game_editor->model.sprite_sheets_max = editor_model_sprite_sheets_MAX;
	game_editor->model.sprite_sheets = memory_area_clear_and_push_array(
			&game_editor->area,
			game_resource_attributes *,
			game_editor->model.sprite_sheets_max);
	//keyframe groups
//	game_editor->model.keyframe_group_max = 1000;
//	game_editor->model.keyframe_groups = memory_area_clear_and_push_array(
//			&game_editor->area,
//			editor_animation_keyframe_group,
//			game_editor->model.keyframe_group_max);
	game_editor->model.animation_keyframes = memory_area_clear_and_push_array(
			&game_editor->area,
			editor_animation_keyframe,
			game_editor->model.keyframe_max);
	//timeline "row"
	game_editor->model.timeline_row_max = 1000;
	game_editor->model.timeline_rows = memory_area_clear_and_push_array(
			&game_editor->area,
			editor_animation_keyframe_row,
			game_editor->model.timeline_row_max);
	//timeline "column"
	game_editor->model.timeline_column_max = 1000;
	game_editor->model.timeline_columns = memory_area_clear_and_push_array(
			&game_editor->area,
			editor_animation_keyframe_column,
			game_editor->model.timeline_column_max);

	//for displaying the model
	game_editor->model.sprite_sheets_as_asset = memory_area_clear_and_push_array(
			&game_editor->area, render_texture *, game_editor->model.sprite_sheets_max);


	game_editor->model.dyarrays_area = memory_dyarray_area_create(
			&game_editor->area, MEGABYTES(1));




	game_editor->model.animations = memory_area_clear_and_push_array(
			&game_editor->area, editor_animation, animation_max);

	game_editor->model.frame_list_max = 100;
	game_editor->model.frame_lists = memory_area_push_array(
			&game_editor->area, editor_model_frame_list, game_editor->model.frame_list_max);
	game_editor->model.model_frame_lists = memory_area_push_array(
			&game_editor->area, model_mesh_frame_list, game_editor->model.frame_list_max);

	//   game_editor->model.animations = memory_area_push_array(
	//		   &game_editor->area, model_animation, animation_max);
	//   game_editor->model.clips      = memory_area_push_array(
	//		   &game_editor->area, model_animation_clip, animation_clip_max); 
	//   game_editor->model.keyframes  = memory_area_push_array(
	//		   &game_editor->area, model_animation_keyframe, animation_keyframe_max);

	//	   game_editor->model.uvs = memory_area_push_array(
	//			   &game_editor->area, model_uvs, editor_UV_CAPACITY);

	//default model
	//   game_editor->model.loaded_animations.animations   = game_editor->model.animations;
	//attachment data
	model_editor->attachment_max = 10;
	model_editor->attachments = memory_area_clear_and_push_array(&game_editor->area,
			editor_model_attachment, model_editor->attachment_max);

	game_editor->model.cursor.tile_displacement = 1;
	game_editor->model.cursor_selection_size_x = 1;
	game_editor->model.cursor_selection_size_y = 1;

	game_editor->model.bone_name_chunks = editor_name_chunks_allocate(editor_area,
			game_editor->model.boneCapacity,
			editor_BONE_NAME_MAX);
	game_editor->model.sprite_name_chunks = editor_name_chunks_allocate(editor_area,
			game_editor->model.sprite_max,
			editor_BONE_NAME_MAX);
	game_editor->model.animation_name_chunks = editor_name_chunks_allocate(editor_area,
			game_editor->model.animation_max,
			editor_ANIMATION_NAME_LENGTH);
	game_editor->model.frame_list_names = editor_name_chunks_allocate(editor_area,
			100,
			editor_ANIMATION_NAME_LENGTH);
	game_editor->model.frame_key_names = editor_name_chunks_allocate(editor_area, 100, editor_ANIMATION_NAME_LENGTH);
}

static inline model
editor_model_allocate_copy(memory_area *area, model *target_model)
{
	model bind_model = *target_model;
	bind_model.sprites = memory_area_push_array(
			area,
			model_sprite,
			target_model->sprite_count);
	bind_model.bones = memory_area_push_array(
			area,
			model_bone,
			target_model->bone_count);

	bind_model.uvs = target_model->uvs;
	//bind_model.uvs = memory_area_push_array(
	//		area,
	//		sprite_orientation,
	//		target_model->uvs_count);

	//bones
	for(u32 s = 0; s < target_model->bone_count; s++)
	{
	    bind_model.bones[s] = target_model->bones[s];	
	}
	//sprites
	for(u32 s = 0; s < target_model->sprite_count; s++)
	{
	    bind_model.sprites[s] = target_model->sprites[s];	
	}
	//frame list
	//for(u32 s = 0; s < target_model->frame_list_count; s++)
	//{
	//    bind_model.mesh_frame_list[s] = target_model->mesh_frame_list[s];	
	//	u32 at = target_model->mesh_frame_list[s].uvs_at;

	//	for(u32 s = 0; s < target_model->mesh_frame_list[s].uvs_count; s++)
	//	{
	//		bind_model.uvs[at + s] = target_model->uvs[at + s];	
	//	}
	//}
	////keyframes
	//memory_copy(target_model->keyframes, bind_model.keyframes, target_model->keyframe_count * sizeof(model_animation_keyframe));
	////frame keyframes
	//memory_copy(target_model->keyframes, bind_model.keyframes, target_model->keyframe_count * sizeof(model_animation_keyframe));
	////animations
	//memory_copy(target_model->animations, bind_model.animations, target_model->animation_count * sizeof(model_animation));
	//uvs

	return(bind_model);
}

inline void
editor_update_and_render_model_animated(
		s_game_editor *game_editor,
		render_commands *render_commands,
		game_assets *game_asset_manager,
		vec3 model_world_position,
		f32 dt)
{
	s_model_editor *model_editor = &game_editor->model;

	u32 animation_count = model_editor->animation_count;
	if(model_editor->bone_count && 
	   model_editor->sprite_count &&
	   model_editor->animation_is_selected)
	{
		temporary_area tempArea = temporary_area_begin(&game_editor->area);
		model *render_model = &model_editor->loaded_model;
		//Temp meshes
//		model temporaryMeshes = 
//			render_allocate_bind_model(&game_editor->area, &model_editor->loaded_model);


		render_model->animations = memory_area_push_array(&game_editor->area,
				model_animation, model_editor->animation_count);
		render_model->keyframes = memory_area_push_array(&game_editor->area,
				model_animation_keyframe, model_editor->keyframe_count);
		render_model->frame_keyframes = memory_area_push_array(&game_editor->area,
				model_animation_keyframe, model_editor->frame_keyframe_count);


		u32 clip_slots_used = 0;
		u32 keyframe_slots_used = 0;
		u32 keyframe_frame_slots_used = 0;
		//copy the animations
		for(u32 a = 0; a < render_model->animation_count; a++)
		{
			model_animation *animation = render_model->animations + a;

			editor_animation *editor_animation =
				model_editor->animations + a;

			*animation = editor_animation->base;
			//copy clips onto the render model array
			animation->keyframes_at = keyframe_slots_used;
			animation->frame_keyframes_at = keyframe_frame_slots_used;
			keyframe_slots_used += animation->keyframe_count;
			keyframe_frame_slots_used += animation->frame_keyframe_count;

#if 0
			//transform keyframes

			editor_animation_keyframe_group *group = editor_animation->first_keyframe_group;
			u32 animation_keyframe_index = 0;
			while(group)
			{
				for(u32 k = 0; k < group->keyframe_count; k++)
				{
					model_animation_keyframe *keyframe = render_model->keyframes +
						animation->keyframes_at + animation_keyframe_index;
					model_animation_keyframe *editor_keyframe = memory_dyarray_get(group->keyframes, k);
					*keyframe = *editor_keyframe;
					animation_keyframe_index++;
				}
				group = group->next;

			}
			//frame keyframes
			group = editor_animation->first_frame_keyframe_group;
			animation_keyframe_index = 0;
			while(group)
			{
				for(u32 k = 0; k < group->keyframe_count; k++)
				{
					model_animation_keyframe *keyframe = render_model->frame_keyframes +
						animation->frame_keyframes_at + animation_keyframe_index;
					model_animation_keyframe *editor_keyframe = memory_dyarray_get(group->keyframes, k);
					*keyframe = *editor_keyframe;
					animation_keyframe_index++;

				}
				group = group->next;
			}
#endif
			//transform keyframes

			editor_animation_keyframe_row *group = editor_animation->first_row;
			u32 animation_keyframe_index = 0;
			while(group)
			{
				for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
						editor_keyframe;
						editor_keyframe = editor_keyframe->next_in_row)
				{
					model_animation_keyframe *keyframe = render_model->keyframes +
						animation->keyframes_at + animation_keyframe_index;
					*keyframe = editor_keyframe->base;
					animation_keyframe_index++;
				}
				group = group->next;

			}
			//frame keyframes
			group = editor_animation->first_frame_row;
			animation_keyframe_index = 0;
			while(group)
			{
				for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
						editor_keyframe;
						editor_keyframe = editor_keyframe->next_in_row)
				{
					model_animation_keyframe *keyframe = render_model->frame_keyframes +
						animation->frame_keyframes_at + animation_keyframe_index;
					*keyframe = editor_keyframe->base;
					animation_keyframe_index++;
				}
				group = group->next;
			}
		}

		model_pose animated_pose = model_editor->loaded_model_pose;
		model_animation *animation = render_model->animations + model_editor->selected_animation_index;
		//pose to animate


		f32 current_time = model_editor->timer.dt_transcurred;
		if(model_editor->reproduce_animation)
		{
			model_animation_timer_run(animation, &model_editor->timer, 1, dt);
		}

		model_animation_animate_model_new(
				render_commands->gameRenderer,
				model_editor->loaded_model,
				animated_pose,
				animation,
				current_time,
				1,
				model_editor->model_foward,
				dt);
		model_render(render_commands->gameRenderer,
				render_commands,
				model_editor->loaded_model,
				animated_pose,
				model_world_position,
				game_editor->model.model_foward);

//		model_update_render_animated(
//				render_commands,
//				bind_model,
//				model_copy,
//				animation,
//				&game_editor->model.timer,
//				model_world_position,
//				game_editor->model.model_foward,
//				game_editor->model.reproduce_animation,
//				game_editor->model.runAnimation,
//				dt);

		//temp area
		temporary_area_end(&tempArea);

	}

}


static editor_animation *
editor_model_add_animation(s_game_editor *game_editor)
{
	model_animation zero_animation = {0};

	editor_animation *new_animation_data = game_editor->model.animations + game_editor->model.animation_count;
	game_editor->model.animation_count++;

	u32 anim_count = game_editor->model.animation_count;

	if(anim_count >= game_editor->model.animation_max)
	{
	    Assert(0);
	}
	//Reset to default
//	new_animation_data->clips = memory_dyarray_create(
//			game_editor->model.clip_arrays_expandable_area,
//			10,
//			10);
	new_animation_data->first_column = 0;
	new_animation_data->first_row = 0;
	new_animation_data->first_frame_row = 0;
	//new_animation_data->keyframes = memory_dyarray_create(
	//		game_editor->model.dyarrays_area, 20, 20);
	//new_animation_data->frame_keyframes = memory_dyarray_create(
	//		game_editor->model.dyarrays_area, 10, 10);


	//set initial data
	new_animation_data->base = zero_animation;
	new_animation_data->base.repeat = 1;

	editor_name_chunks_addf(&game_editor->model.animation_name_chunks,
			                "Animation %u",
							game_editor->model.animation_count);

	return(new_animation_data);
}

inline u32 
editor_animation_switch_animation_index(
		s_game_editor *game_editor,
		u32 i0,
		u32 i1)
{
	u32 anim_c = game_editor->model.animation_count;
	u32 success = 
		i0 != i1 && 
		(i0 > 0 && i1 > 0) &&
		(i0 < (anim_c) && i1 < (anim_c));

	if(success)
	{
		editor_animation animation_copy0 = game_editor->model.animations[i0];
		game_editor->model.animations[i0] = game_editor->model.animations[i1];
		game_editor->model.animations[i1] = animation_copy0;

		u8 name_buffer[editor_ANIMATION_NAME_LENGTH];

		//set name to the buffer
		string_copy(
				game_editor->model.animation_name_chunks.chunks[i1].name,
				name_buffer);
		//set name to other animation
		string_copy_and_clear(
				game_editor->model.animation_name_chunks.chunks[i0].name,
				game_editor->model.animation_name_chunks.chunks[i1].name,
				editor_ANIMATION_NAME_LENGTH);

		string_copy_and_clear(
				name_buffer,
				game_editor->model.animation_name_chunks.chunks[i0].name,
				editor_ANIMATION_NAME_LENGTH);
	}
	return(success);
}

static void
editor_animation_remove(s_game_editor *game_editor, u32 animation_index)
{
	u32 animation_count = game_editor->model.animation_count;
	game_editor->model.animation_count--;

	u32 a = animation_index;
	//shift array
	while(a <  animation_count - 1)
	{
		game_editor->model.animations[a] =
			game_editor->model.animations[a + 1];

		a++;
	}
	//remove name
	editor_name_chunks_remove(&game_editor->model.animation_name_chunks, animation_index);

	//set a valid selected animation
	if(game_editor->model.selected_animation_index < game_editor->model.animation_count)
	{
		game_editor->model.animation_is_selected = 0;
		if(game_editor->model.animation_count)
		{
		    game_editor->model.selected_animation_index = game_editor->model.animation_count - 1;
		}
		else
		{
		    game_editor->model.selected_animation_index = 0; 
		}
	}
}

#if 0
static void
editor_animation_sort_keyframes(editor_animation_keyframe_group *keyframe_group)
{
	u32 keyframe_count = keyframe_group->keyframe_count;
	//get base from dyarray
	model_animation_keyframe *kf_array = (model_animation_keyframe *)keyframe_group->keyframes->base_v;

	//first sort by bone index
	for(u32 y = 0; y < keyframe_count; y++)
	{
		for(u32 x = 0; x < keyframe_count; x++)
		{
			model_animation_keyframe *kf0 = kf_array + y;
			model_animation_keyframe *kf1 = kf_array + x;

			if(kf0->frame_start < kf1->frame_start)
			{
				model_animation_keyframe kf_copy = *kf0;
				*kf0 = *kf1;
				*kf1 = kf_copy;
			}
		}
	}
	//sort by time
	//u32 y = 0;
	//while(y + 1 < keyframe_count)
	//{
	//	model_animation_keyframe *kf0 = kf_array + y;
	//	model_animation_keyframe *kf1 = kf_array + y + 1;
	//	while(y + 1 < keyframe_count && kf1->bone_index == kf0->bone_index)
	//	{
	//		if(kf0->frame_start < kf1->frame_start)
	//		{
	//			model_animation_keyframe kf_copy = *kf0;
	//			*kf0 = *kf1;
	//			*kf1 = kf_copy;
	//		}
	//		kf0 = kf_array + y;

	//		y++;
	//		model_animation_keyframe *kf0 = kf_array + y;
	//		model_animation_keyframe *kf1 = kf_array + y + 1;
	//	}
	//}
}
#endif

static inline void
editor_animation_add_keyframe_to_column(
		editor_animation_keyframe_column *column,
		editor_animation_keyframe *keyframe)
{
	//remove old connections
	if(keyframe->prev_in_column)
	{
		keyframe->prev_in_column->next_in_column = keyframe->next_in_column;
	}
	if(keyframe->next_in_column)
	{
		keyframe->next_in_column->prev_in_column = keyframe->prev_in_column;
	}
	if(keyframe->column)
	{
		if(keyframe->column->first_keyframe == keyframe)
		{
			keyframe->column->first_keyframe = keyframe->next_in_column;
		}
	}
	keyframe->next_in_column = 0;
	keyframe->prev_in_column = 0;

	if(column->first_keyframe)
	{
		column->first_keyframe->prev_in_column = keyframe;
	}
	keyframe->next_in_column = column->first_keyframe;
	column->first_keyframe = keyframe;
	keyframe->column = column;
}

static inline void
editor_animation_add_keyframe_to_row(
		editor_animation_keyframe_row *row,
		editor_animation_keyframe *keyframe,
		u32 frame)
{
	//get rid of row connections if any
	if(keyframe->prev_in_row)
	{
		keyframe->prev_in_row->next_in_row = keyframe->next_in_row;
	}
	if(keyframe->next_in_row)
	{
		keyframe->next_in_row->prev_in_row = keyframe->prev_in_row;
	}
	if(keyframe->row && keyframe->row->first_keyframe == keyframe)
	{
		keyframe->row->first_keyframe = keyframe->next_in_row;
	}
	keyframe->next_in_row = 0;
	keyframe->prev_in_row = 0;

	if(!row->first_keyframe)
	{
		row->first_keyframe = keyframe;
	}
	else
	{
		for(editor_animation_keyframe *row_kf = row->first_keyframe;
				row_kf;
				row_kf = row_kf->next_in_row)
		{
			//add between
			if(row_kf->base.frame_start > frame)
			{
				if(row_kf->prev_in_row)
				{
					row_kf->prev_in_row->next_in_row = keyframe;
				}
				keyframe->prev_in_row = row_kf->prev_in_row;
				keyframe->next_in_row = row_kf;
				row_kf->prev_in_row = keyframe;
				break;
			}
			else if(row_kf->next_in_row && row_kf->next_in_row->base.frame_start > frame)
			{
				row_kf->next_in_row->prev_in_row = keyframe;
				keyframe->prev_in_row = row_kf;
				keyframe->next_in_row = row_kf->next_in_row;
				row_kf->next_in_row = keyframe;
				break;
			}
			else if(!row_kf->next_in_row)
			{
				row_kf->next_in_row = keyframe;
				keyframe->prev_in_row = row_kf;
				break;
			}
		}
	}

	if(row->first_keyframe != keyframe &&
	   row->first_keyframe->base.frame_start > frame)
	{
		row->first_keyframe = keyframe;
	}
	keyframe->row = row;
}




static editor_animation_keyframe * 
editor_animation_get_keyframe_at_column(editor_animation_keyframe_column *column,
		u32 bone_index,
		u32 bs_array_index)
{

	editor_animation_keyframe *keyframe = column->first_keyframe;
	model_animation_keyframe_type types[] =  {
	model_animation_keyframe_transform,
	model_animation_keyframe_frame};
	b32 found = 0;
	while(keyframe && !found)
	{
		while(keyframe && keyframe->base.bone_index != bone_index)
		{
			keyframe = keyframe->next_in_column;
		}
		if(keyframe && keyframe->base.type != types[bs_array_index])
		{
			keyframe = keyframe->next_in_column;
		}
		else
		{
			found = 1;
		}
	}
	return(keyframe);
}

static editor_animation_keyframe * 
editor_animation_get_keyframe_at_row(
		editor_animation_keyframe_row *row,
		u32 frame)
{

	editor_animation_keyframe *keyframe = row->first_keyframe;
	while(keyframe && keyframe->base.frame_start != frame)
	{
		keyframe = keyframe->next_in_row;
	}
	return(keyframe);
}


static editor_animation_keyframe_column *
editor_animation_get_column(
		editor_animation *target_animation,
		u32 frame)
{
	editor_animation_keyframe_column *column = target_animation->first_column;
	while(column && column->frame != frame)
	{
		column = column->next;
	}
	return(column);
}


static editor_animation_keyframe_column *
editor_animation_get_or_allocate_column(s_game_editor *game_editor,
		editor_animation *target_animation,
		u32 frame)
{
	editor_animation_keyframe_column *column = target_animation->first_column;
	while(column && column->frame != frame)
	{
		column = column->next;
	}
	if(!column)
	{
		s_model_editor *model_editor = &game_editor->model;
		column = model_editor->first_free_column;
		//allocate new
		if(!column)
		{
			column = model_editor->timeline_columns + model_editor->timeline_column_count++;
			Assert(model_editor->timeline_column_count < model_editor->timeline_column_max);
		}
		else
		{
			model_editor->first_free_column = column->next;
		}
		column->next = 0;
		column->prev = 0;
		column->first_keyframe = 0;
		if(target_animation->first_column)
		{
			target_animation->first_column->prev = column;
		}
		column->next = target_animation->first_column;
		target_animation->first_column = column;
		column->frame = frame;
	}

	return(column);
}


static inline editor_animation_keyframe_row *
editor_animation_get_row_slot(s_game_editor *game_editor)
{
	s_model_editor *model_editor = &game_editor->model;
	editor_animation_keyframe_row *row = model_editor->first_free_row;
	//allocate new
	if(!row)
	{
		row = model_editor->timeline_rows + model_editor->timeline_row_count++;
		Assert(model_editor->timeline_row_count < model_editor->timeline_row_max);
	}
	else
	{
		model_editor->first_free_row = row->next;
	}
	row->next = 0;
	row->prev = 0;
	row->first_keyframe = 0;
	return(row);
}

static  editor_animation_keyframe_row *
editor_animation_get_row(
		editor_animation *target_animation,
		u32 bone_index,
		u32 bs_array_index)
{
	editor_animation_keyframe_row *bs_rows[] =
	{target_animation->first_row,
		target_animation->first_frame_row};
	editor_animation_keyframe_row *row = bs_rows[bs_array_index]; 
	while(row && row->bone_index != bone_index)
	{
		row = row->next;
	}
	return(row);
}
static editor_animation_keyframe_row *
editor_animation_get_or_allocate_row(s_game_editor *game_editor,
		editor_animation *target_animation,
		u32 bone_index,
		u32 bs_array_index)
{
	editor_animation_keyframe_row *bs_rows[] =
	{target_animation->first_row,
		target_animation->first_frame_row};
	editor_animation_keyframe_row *row = bs_rows[bs_array_index]; 
	while(row && row->bone_index != bone_index)
	{
		row = row->next;
	}
	if(!row)
	{
		row = editor_animation_get_row_slot(game_editor);
		if(target_animation->first_rows[bs_array_index])
		{
			target_animation->first_rows[bs_array_index]->prev = row;
		}
		row->next = target_animation->first_rows[bs_array_index];
		target_animation->first_rows[bs_array_index] = row;
		row->first_keyframe = 0;
	}
	row->bone_index = bone_index;
	row->type = bs_array_index ? model_animation_keyframe_frame : 
		model_animation_keyframe_transform;
	return(row);
}

static inline void
editor_animation_change_keyframe_frame(
		s_game_editor *game_editor,
		editor_animation *target_animation,
		editor_animation_keyframe *keyframe,
		u32 frame)
{
	if(keyframe->base.frame_start != frame)
	{
		b32 index_is_avadible = 1;
		//look for an existing keyframe on this frame
		if(frame < keyframe->base.frame_start)
		{
			editor_animation_keyframe *other_keyframe = keyframe->prev_in_row;
			while(index_is_avadible && other_keyframe)
			{
				if(other_keyframe->base.frame_start == frame)
				{
					index_is_avadible = 0;
				}
				other_keyframe = other_keyframe->prev_in_row;
			}
		}
		else
		{
			editor_animation_keyframe *other_keyframe = keyframe->next_in_row;
			while(index_is_avadible && other_keyframe)
			{
				if(other_keyframe->base.frame_start == frame)
				{
					index_is_avadible = 0;
				}
				other_keyframe = other_keyframe->next_in_row;
			}
		}

		if(index_is_avadible)
		{
			keyframe->base.frame_start = frame;
			editor_animation_keyframe_column *new_column = editor_animation_get_or_allocate_column(
						game_editor, target_animation, frame);
			editor_animation_add_keyframe_to_column(
					new_column,
					keyframe);
			editor_animation_add_keyframe_to_row(
					keyframe->row,
					keyframe,
				    frame);
		}

	}
}

static inline editor_animation_keyframe *
editor_animation_get_keyframe_slot(
		s_game_editor *game_editor)
{
	s_model_editor *model_editor = &game_editor->model;
	editor_animation_keyframe *keyframe = model_editor->first_free_keyframe;
	if(!keyframe)
	{
		keyframe = model_editor->animation_keyframes + model_editor->keyframe_count++;
		//assert for now
		Assert(model_editor->keyframe_max > model_editor->keyframe_count);
	}
	else
	{
		model_editor->first_free_keyframe = keyframe->next_free;
	}
	keyframe->column = 0;
	keyframe->row = 0;
	keyframe->next_in_row = 0;
	keyframe->next_in_column = 0;
	keyframe->prev_in_row = 0;
	keyframe->prev_in_column = 0;
	memory_clear(keyframe, sizeof(*keyframe));

	return(keyframe);
}

static void
editor_animation_free_keyframe(
		s_game_editor *game_editor,
		editor_animation *animation,
		editor_animation_keyframe *keyframe)
{
	s_model_editor *model_editor = &game_editor->model;
	keyframe->next_free = model_editor->first_free_keyframe;
	model_editor->first_free_keyframe = keyframe;

	if(keyframe->base.type == model_animation_keyframe_transform)
	{
		animation->base.keyframe_count--;
	}
	else
	{
		animation->base.frame_keyframe_count--;
	}
	if(game_editor->model.selected_keyframe == keyframe)
	{
		game_editor->model.keyframe_is_selected = 0;
	}
}

static editor_animation_keyframe * 
editor_animation_add_keyframe(s_game_editor *game_editor,
		editor_animation *target_animation,
		u32 bone_index,
		u32 bs_array_index,
		u32 frame)
{
	editor_animation_keyframe_row *row = editor_animation_get_or_allocate_row(
			game_editor, target_animation, bone_index, bs_array_index);
	editor_animation_keyframe_column *column = editor_animation_get_or_allocate_column(
			game_editor, target_animation, frame);

	//make sure this keyframe doesn't exist
	editor_animation_keyframe *new_keyframe = editor_animation_get_keyframe_at_column(column,
			bone_index,
			bs_array_index);
	if(!new_keyframe)
	{
		new_keyframe = editor_animation_get_keyframe_slot(game_editor);
		new_keyframe->base.q = QUAT(1, 0, 0, 0);
		new_keyframe->base.frame_list_index = 0;
		new_keyframe->base.frame_start = frame;
		//add to row, look for a free slot between frames 
		new_keyframe->base.bone_index = bone_index;
		new_keyframe->base.type = !bs_array_index ? 
			model_animation_keyframe_transform : model_animation_keyframe_frame;
		new_keyframe->base.spline = model_animation_spline_linear;
		//add to row and sort
		if(!bs_array_index)
		{
			game_editor->model.keyframe_count++;
			target_animation->base.keyframe_count++;
		}
		else
		{
			game_editor->model.frame_keyframe_count++;
			target_animation->base.frame_keyframe_count++;
		}

		editor_animation_add_keyframe_to_column(
				column,
				new_keyframe);
		editor_animation_add_keyframe_to_row(
				row,
				new_keyframe,
				frame);
	}

	return(new_keyframe);
}

//static model_animation_keyframe *
//editor_animation_add_ke



static void
editor_animation_remove_keyframe(
		s_game_editor *game_editor,
		editor_animation *animation,
		editor_animation_keyframe *r_keyframe)
{

	u32 frame = r_keyframe->base.frame_start;
	if(r_keyframe->prev_in_column)
	{
		r_keyframe->prev_in_column->next_in_column = r_keyframe->next_in_column;
	}
	if(r_keyframe->next_in_column)
	{
		r_keyframe->next_in_column->prev_in_column = r_keyframe->prev_in_column;
	}
	if(r_keyframe->prev_in_row)
	{
		r_keyframe->prev_in_row->next_in_row = r_keyframe->next_in_row;
	}
	if(r_keyframe->next_in_row)
	{
		r_keyframe->next_in_row->prev_in_row = r_keyframe->prev_in_row;
	}
	//free in rows and columns
	if(r_keyframe->row->first_keyframe == r_keyframe)
	{
		r_keyframe->row->first_keyframe = r_keyframe->next_in_row;
	}
	if(r_keyframe->column->first_keyframe == r_keyframe)
	{
		r_keyframe->column->first_keyframe = r_keyframe->next_in_column;
	}
	//put in free slots
	editor_animation_free_keyframe(game_editor, animation, r_keyframe);


}

static void
editor_animation_copy_at_current_frame(
		s_game_editor *editor)
{
	s_model_editor *model_editor = &editor->model;
	if(model_editor->focused_on_keyframe_only &&
			model_editor->keyframe_is_selected)
	{
		model_editor->keyframe_clipboard_count = 1;
		model_editor->keyframe_clipboard[0] = model_editor->selected_keyframe->base;
	}
	else if(model_editor->selected_column)
	{
		editor_animation_keyframe_column *column = model_editor->selected_column;
		model_editor->keyframe_clipboard_count = 0;
		editor_animation_keyframe *keyframe = column->first_keyframe;
		while(keyframe && model_editor->keyframe_clipboard_count < model_editor->keyframe_clipboard_max)
		{
			model_editor->keyframe_clipboard[model_editor->keyframe_clipboard_count] = keyframe->base;
			model_editor->keyframe_clipboard_count++;
			keyframe = keyframe->next_in_column;
		}
	}
}

static void
editor_animation_paste_at_current_frame(
		s_game_editor *editor,
		editor_animation *animation)
{
	s_model_editor *model_editor = &editor->model;
	if(model_editor->timeline_frame_is_selected)
	{
		u32 selected_frame = model_editor->timeline_selected_frame;
		for(u32 k = 0; k < model_editor->keyframe_clipboard_count; k++)
		{
			model_animation_keyframe *keyframe = model_editor->keyframe_clipboard + k;
			u32 type = keyframe->type;
			b32 avadible = type ? 
				keyframe->mesh_index < model_editor->sprite_count :
				keyframe->bone_index < model_editor->bone_count;
			if(avadible)
			{
				editor_animation_keyframe *animation_keyframe = editor_animation_add_keyframe(
						editor,
						animation,
						keyframe->bone_index,
						type,
						selected_frame);
				animation_keyframe->base = *keyframe;
				animation_keyframe->base.frame_start = selected_frame;
			}
		}
	}
}








//static editor_animation_clip * 
//editor_animation_add_clip_if_frame_avadible(s_game_editor *game_editor,
//		                  u32 target_frame)
//{
//	editor_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//	memory_dyarray(editor_clip) *clip_array = current_animation->clips;
//	editor_animation_clip *new_clip = 0;
//	
//	u32 clip_count = memory_dyarray_count(current_animation->clips);
//
//	if(current_animation->base.frames_total > clip_count)
//	{
//	    u32 f = 0;
//	    u32 found_frame = 1;
//
//		//look for a clip that goes after the target frame
//	    while(f < clip_count)
//	    {
//			//cancel adding if a clip is already on this frame
//			editor_animation_clip *current_clip = memory_dyarray_get(
//					clip_array, f);
//	    	if(current_clip->base.frameStart == target_frame)
//	    	{
//				found_frame = 0;
//				break;
//	    	}
//	    	else if(current_clip->base.frameStart > target_frame)
//	    	{
//				found_frame = 1;
//				break;
//	    	}
//	    	f++;
//	    }
//		if(found_frame)
//		{
//			new_clip->base.frameStart = target_frame;
//		}
//		
//	}
//	return(new_clip);
//}
//
//static void
//editor_animation_add_clip_and_sort(s_game_editor *game_editor,
//		                           u32 target_frame)
//{
//	editor_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//	editor_animation_clip *new_clip = editor_animation_add_clip_if_frame_avadible(game_editor, target_frame);
//	if(new_clip)
//	{
//        editor_animation_sort_clips_by_frame(current_animation);
//	}
//}
//
//
//static void
//editor_animation_remove_clip_from_index(s_game_editor *game_editor,
//		                     u32 target_animation_index,
//		                     u32 target_clip_index)
//{
//	editor_animation *current_animation = game_editor->model.animations + target_animation_index;
//	editor_animation_clip *target_clip = memory_dyarray_get(current_animation->clips, target_clip_index);
//
//	if(current_animation->base.clip_count && 
//			(target_clip_index < current_animation->base.clip_count))
//	{
//		//delete the keyframes array from this clip
//		memory_dyarray_delete(target_clip->keyframes);
//		//delete clip
//		memory_dyarray_remove_at(current_animation->clips, target_clip_index);
//		game_editor->model.clip_count--;
//        current_animation->base.clip_count--;
//
//	}
//
//}
//
//inline void
//editor_animation_remove_clip_from_editing(s_game_editor *game_editor)
//{
//	editor_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//
//	if(current_animation->base.clip_count &&
//	   game_editor->model.selected_clip_index < current_animation->base.clip_count)
//	{
//        editor_animation_remove_clip_from_index(game_editor,
//    		                                    game_editor->model.selected_animation_index,
//    		                                    game_editor->model.selected_clip_index);
//		//set the selected clip index to another
//        game_editor->model.selected_clip_index = 0;
//		if(current_animation->base.clip_count)
//		{
//			game_editor->model.selected_clip_index = current_animation->base.clip_count - 1;
//		}
//	}
//}
//
////used to move clips on the timeline
//inline void
//editor_animation_change_editing_clip_frame(s_game_editor *game_editor,
//		                                   u32 new_frame)
//{
//	editor_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//	editor_animation_clip *clip_array   = current_animation->clips->base_v;
//	editor_animation_clip *editing_clip = clip_array + game_editor->model.selected_clip_index;
//
//	u32 clip_count = current_animation->base.clip_count;
//	u32 editing_clip_index = game_editor->model.selected_clip_index;
//
//	if(new_frame != editing_clip->base.frameStart)
//	{
//		u32 increased = new_frame > editing_clip->base.frameStart;
//		if(increased)
//		{
//			//looks for how many slots this clip advanced
//			u32 switched_slots = 0;
//			u32 c              = editing_clip_index + 1;
//			while(c < clip_count)
//			{
//				if(clip_array[c].base.frameStart <= new_frame)
//				{
//					switched_slots++;
//				}
//				else
//				{
//					break;
//				}
//				c++;
//			}
//			//switch places with the clips ahead on the array
//			u32 s = 0;
//			c = editing_clip_index;
//			while(s < switched_slots) 
//			{
//				editor_animation_clip clip_copy = clip_array[c];
//				clip_array[c] = clip_array[c + 1];
//				clip_array[c + 1] = clip_copy;
//
//				s++;
//				c++;
//			}
//			editing_clip_index += switched_slots;
//		}
//		else
//		{
//			//looks for how many slots this clip advanced
//			u32 switched_slots = 0;
//			u32 c               = editing_clip_index;
//			while(c > 0)
//			{
//				if(clip_array[c - 1].base.frameStart >= new_frame)
//				{
//					switched_slots++;
//				}
//				else
//				{
//					break;
//				}
//				c--;
//			}
//			//switch places with the clips on the array
//			u32 s = 0;
//			c = editing_clip_index;
//			while(s < switched_slots) 
//			{
//				editor_animation_clip clip_copy = clip_array[c];
//				clip_array[c] = clip_array[c - 1];
//				clip_array[c - 1] = clip_copy;
//
//				s++;
//				c--;
//			}
//			editing_clip_index -= switched_slots;
//		}
//
//		//set the frame
//		clip_array[editing_clip_index].base.frameStart = new_frame;
//		game_editor->model.selected_clip_index = editing_clip_index;
//	}
//}
//
//static void
//editor_animation_copy_clip(s_game_editor *game_editor)
//{
//	if(game_editor->model.clip_count)
//	{
//		editor_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//		model_animation_clip *clip_array = current_animation->clips->base_v;
//		editor_animation_clip *current_clip        = memory_dyarray_get(current_animation->clips, game_editor->model.selected_clip_index);
//		model_animation_keyframe *keyframe_array  = current_clip->keyframes->base_v;
//
//		u32 clip_keyframe_count = current_clip->base.keyframe_count;
//
//		game_editor->model.clip_clipboard_count = 1;
//		game_editor->model.clip_clipboard[0] = current_clip->base;
//
//		u32 k = 0;
//		while(k < clip_keyframe_count)
//		{
//			u32 keyframe_index = k;
//			game_editor->model.keyframe_clipboard[k] = keyframe_array[keyframe_index];
//			k++;
//		}
//	}
//}
//
//static void
//editor_animation_cut_clip(s_game_editor *game_editor)
//{
//	editor_animation_copy_clip(game_editor);
//
//	editor_animation_remove_clip_from_editing(game_editor);
//}
//
//static void
//editor_animation_paste_clip(s_game_editor *game_editor,
//		                    u32 target_frame)
//{
//#if 0
//	model_animation *current_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
//	model_animation_clip *clip_array          = current_animation->clips;
//	model_animation_clip *current_clip        = clip_array + game_editor->model.selected_clip_index;
//	model_animation_keyframe *keyframe_array  = current_animation->keyframes;
//
//	u32 copied_clips_count = game_editor->model.clip_clipboard_count;
//	if(copied_clips_count)
//	{
//		model_animation_clip *copied_clip = game_editor->model.clip_clipboard + 0;
//        model_animation_clip *new_clip = editor_animation_add_clip_if_frame_avadible(game_editor,
//		                                                           target_frame);
//		//clip got successfully added, so fill data and sort after
//		if(new_clip)
//		{
//		    u32 new_clip_index = current_animation->clip_count - 1;
//
//			copied_clip->keyframes_at = new_clip->keyframes_at;
//			*new_clip = *copied_clip;
//			new_clip->frameStart     = target_frame;
//			//reset since the count increases by calling the add function
//			new_clip->keyframe_count = 0;
//			//add the key frames
//			u32 k = 0;
//			while(k < copied_clip->keyframe_count)
//			{
//				model_animation_keyframe *copied_keyframe = game_editor->model.keyframe_clipboard + k;
//
//				model_animation_keyframe *new_keyframe = editor_animation_add_keyframe_to_clip(game_editor,
//						                              game_editor->model.selected_animation_index,
//													  new_clip_index);
//
//				*new_keyframe = *copied_keyframe;
//				k++;
//			}
//		    editor_animation_sort_clips_by_frame(current_animation);
//		}
//
//
//	}
//#endif
//	//;finish
//
//}



inline u32
editor_model_get_overlapping_sprite(s_game_editor *game_editor,
		                              vec3 v0,
		                              vec3 v1,
		                              vec3 v2,
		                              vec3 v3,
									  u32 *overlapping_index)
{

	u32 success = 0;
	u32 t       = 0;

	u32 mesh_count             = game_editor->model.bone_count;
	model_sprite *sprite_array = game_editor->model.sprites;

    while(!success && t < mesh_count)
	{
		u16 sprite_index = t;
		model_sprite *current_sprite = sprite_array + sprite_index;
		u32 mesh_type = current_sprite->type;

		if(mesh_type != model_sprite_mesh)
		{
		    t++;
			continue;
		}

	    vec3 new_sprite_mid = vertices_get_mid_point(v0,
			      	        					   v1,
			      	        					   v2,
			      	        					   v3);

	    vec3 current_sprite_mid = vertices_get_mid_point(current_sprite->v0,
			      	        						   current_sprite->v1,
			      	        						   current_sprite->v2,
			      	        						   current_sprite->v3);

		u32 verticesOverlap = 
			(i32)new_sprite_mid.x == (i32)current_sprite_mid.x &&
			(i32)new_sprite_mid.y == (i32)current_sprite_mid.y &&
			(i32)new_sprite_mid.z == (i32)current_sprite_mid.z;

		if(verticesOverlap)
		{
			success = 1;
			*overlapping_index = sprite_index;
		}
		t++;
	}
	return(success);
}

#define model_sprite_MINIMUMORIENTATION 1

static model_sprite *
editor_model_add_sprite(s_game_editor *game_editor)
{

	Assert(game_editor->model.bone_count + 1 < game_editor->model.boneCapacity);

	model_sprite *sprite = game_editor->model.sprites + game_editor->model.sprite_count;
	//Reset to default
	model_sprite zeroSprite = {0};
	*sprite = zeroSprite;

	editor_name_chunks_addf(&game_editor->model.sprite_name_chunks,
			               "Sprite %u",
						   game_editor->model.sprite_count);

	game_editor->model.sprite_count++;

	return(sprite);
}

static editor_model_attachment * 
em_add_attachment(
		s_game_editor *game_editor)
{
	s_model_editor *model_editor = &game_editor->model;
	editor_model_attachment *result = 0;
	if(model_editor->attachment_max > model_editor->attachment_count)
	{
		result = model_editor->attachments + model_editor->attachment_count;
		model_editor->attachment_count++;
		memory_clear(result, sizeof(*result));
	}
	else
	{
		Gprintf("Model editor error: could not add another attachment! the maximum amount allowed is %u!", 
				model_editor->attachment_max);
	}
	return(result);
}

#define em_remove_selected_attachment(game_editor) \
	em_remove_attachment(game_editor, game_editor->model.attachment_selection.index)
static b32
em_remove_attachment(
		s_game_editor *game_editor, u32 index)
{
	b32 success = 0;
	s_model_editor *model_editor = &game_editor->model;
	if(index < model_editor->attachment_max)
	{
		//remove and shift
		//get removing thing to check for type
		editor_model_attachment *attachment = model_editor->attachments + index;
		model_editor->attachment_count = e_array_remove_and_shift(model_editor->attachments, sizeof(*attachment), model_editor->attachment_count, index);
	}
	else
	{
		Gprintf("Model editor error: could not remove attachment at %u because it's an invalid index (max is %u)",
				index, model_editor->attachment_max);
	}

	if(eui_selection_selected(model_editor->attachment_selection, index))
	{
		if(model_editor->attachment_count)
		{
			u32 new_index = model_editor->attachment_selection.index;
			new_index = index ? index - 1 : index;
			eui_selection_select(&model_editor->attachment_selection, new_index);
		}
		else
		{
			eui_selection_deselect(model_editor->attachment_selection);
		}
	}
	return(success);
}

static editor_frame_key *
em_add_frame_key_to_bone(
		s_game_editor *game_editor, editor_model_bone *editor_bone)
{

	editor_frame_key *fk = memory_dyarray_push(editor_bone->frame_keys);
	fk->name = editor_name_chunks_addf(&game_editor->model.frame_key_names,"key %u",editor_bone->frame_key_count)->name;
	editor_bone->frame_key_count++;
	game_editor->model.frame_key_count++;
	return(fk);
}

static void
em_remove_frame_key_from_bone(
		s_game_editor *game_editor,
		editor_model_bone *editor_bone,
		u32 index)
{
	if(index < editor_bone->frame_key_count)
	{
		memory_dyarray_remove_at(editor_bone->frame_keys, index);
		editor_bone->frame_key_count--;
		game_editor->model.frame_key_count--;

		s_model_editor *model_editor = &game_editor->model;
		//deselect in case of being the same selection
		if(eui_selection_selected(model_editor->bone_selected_frame_key, index))
		{
			model_editor->bone_selected_frame_key.selected = 0;
		}
	}
}

static editor_model_sprite *
em_add_sprite_to_bone(
		s_game_editor *game_editor, editor_model_bone *editor_bone)
{

	//push the sprite
	memory_dyarray_push(editor_bone->sprites);
	//careful here, the sprite could be re-pointed
	memory_dyarray(sprite_orientation, uvs) dyarray;
	dyarray.uvs = memory_dyarray_create(
			game_editor->model.dyarrays_area, sprite_orientation, 3, 3);
	//get the sprite
	editor_model_sprite *sprite = 0;
	memory_dyarray_last_safe(editor_bone->sprites, sprite);
	//Reset to default
	memory_clear(sprite, sizeof(*sprite));
	//push a default one
	sprite->uvs = dyarray.uvs;
	memory_dyarray_clear_and_push(sprite->uvs);

	editor_name_chunks_addf(&game_editor->model.sprite_name_chunks,
			               "Sprite %u",
						   game_editor->model.sprite_count);

	editor_bone->sprite_count++;
	game_editor->model.sprite_count++;

	return(sprite);
}

static sprite_orientation *
em_add_frame_to_sprite(s_game_editor *game_editor, editor_model_sprite *sprite)
{
	sprite_orientation *frame = 0;
	memory_dyarray_push_safe(sprite->uvs, frame);
	sprite->base.extra_frame_count++;
	memory_clear(frame, sizeof(*frame));
	return(frame);
}

static void 
em_remove_frame_from_sprite(s_game_editor *game_editor,
		editor_model_sprite *sprite,
		u32 index)
{
	if(index && index <= sprite->base.extra_frame_count)
	{
		sprite->base.extra_frame_count--;
		memory_dyarray_remove_at(sprite->uvs, index);
	}
}

static void
em_remove_sprite_from_bone(
		s_game_editor *game_editor,
		editor_model_bone *editor_bone,
		u32 index)
{
	editor_model_sprite *sprite = 0;
	s_model_editor *model_editor = &game_editor->model;
	if(index < editor_bone->sprite_count)
	{
		editor_bone->sprite_count--;
		model_editor->sprite_count--;
		
		//get sprite and get rid of its dyarrays
		memory_dyarray_get_safe(editor_bone->sprites, sprite, index);
		memory_dyarray_delete(sprite->uvs);
		//now remove it
		memory_dyarray_remove_at(editor_bone->sprites, index);
	}
	
}

static void
editor_model_free_row(
		s_game_editor *game_editor,
		editor_animation *animation,
		editor_animation_keyframe_row *row)
{
	//free keyframes
	for(editor_animation_keyframe *keyframe = row->first_keyframe;
			keyframe;
	   )	
	{
		editor_animation_keyframe *next = keyframe->next_in_row;
		editor_animation_free_keyframe(
				game_editor,
				animation,
				keyframe);
		keyframe = next;
	}
	//free row
	if(row->prev)
	{
		row->prev->next = row->next;
	}
	if(row->next)
	{
		row->next->prev = row->prev;
	}
	if(animation->first_row == row)
	{
		animation->first_row = row->next;

	}
	//put in free slots
	row->next = game_editor->model.first_free_row;
	game_editor->model.first_free_row = row;
}

static inline void
editor_animation_free_column(
		s_game_editor *game_editor,
		editor_animation *animation,
		editor_animation_keyframe_column *column)
{
	//free keyframes
	for(editor_animation_keyframe *keyframe = column->first_keyframe;
			keyframe;)	
	{
		editor_animation_keyframe *next = keyframe->next_in_column;
		editor_animation_remove_keyframe(
				game_editor,
				animation,
				keyframe);
		keyframe = next;
	}
	//free column 
	if(column->prev)
	{
		column->prev->next = column->next;
	}
	if(column->next)
	{
		column->next->prev = column->prev;
	}
	if(animation->first_column == column)
	{
		animation->first_column = column->next;

	}
	//put in free slots
	column->next = game_editor->model.first_free_column;
	game_editor->model.first_free_column = column;
	if(game_editor->model.selected_column == column)
	{
		game_editor->model.selected_column = 0;
		game_editor->model.column_is_selected = 0;
	}
}

static inline void
editor_animation_remove_row(
		s_game_editor *game_editor,
		u32 bone_index,
		u32 bs_array_index)
{
	s_model_editor *model_editor = &game_editor->model;
	for(u32 a = 0; a < model_editor->animation_count; a++)
	{
		editor_animation *animation = model_editor->animations + a;
		//free group pointing to this sprite 
		editor_animation_keyframe_row *row_to_free = editor_animation_get_row(
				animation,
				bone_index,
				bs_array_index);
		if(row_to_free)
		{
			editor_model_free_row(
					game_editor,
					animation,
					row_to_free);
		}
		for(editor_animation_keyframe_row *group = animation->first_rows[bs_array_index];
				group;
				group = group->next)
		{
			//just decrease index
			if(group->bone_index > bone_index)
			{
				for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
						editor_keyframe;
						editor_keyframe = editor_keyframe->next_in_row)
				{
					model_animation_keyframe *kf = &editor_keyframe->base; 
					kf->bone_index--;
				}
				group->bone_index--;
			}
		}
	}
}


static void
editor_model_remove_sprite(s_game_editor *game_editor, u32 sprite_index)
{
	Assert(sprite_index < game_editor->model.sprite_max);
	u32 sprite_count = game_editor->model.sprite_count;
	s_model_editor *model_editor = &game_editor->model;

	if(sprite_count)
	{

		editor_animation_remove_row(
				game_editor,
				sprite_index,
				1);

			model_sprite *removing_sprite = game_editor->model.sprites + sprite_index;

		//sort
	    if(sprite_index < sprite_count)
	    {
	    	//Move deleting bone to the end of the array
	    	u32 s = sprite_index;
	    	model_sprite *sprite_array = game_editor->model.sprites;

	    	while(s < sprite_count)
	    	{
				//Move the deleting bone foward
	    		sprite_array[s] = sprite_array[s + 1];
				s++;
	    	}
	    }
		editor_name_chunks_remove(&game_editor->model.sprite_name_chunks, sprite_index);
	    game_editor->model.sprite_count--;

		//for the individual sprites mode
		if(model_editor->selected_sprite_index == sprite_index)
		{
			model_editor->sprite_is_selected = 0;
		}
	}
}

inline void
editor_model_copy_selected_sprites(s_game_editor *game_editor)
{
    editor_cursor_memory *cursor_memory = &game_editor->model.cursor_memory;

	u32 selected_sprite_count = cursor_memory->selected_meshes_count;
	if(selected_sprite_count)
	{
		model_sprite *sprite_clipboard = game_editor->model.sprite_clipboard;
		model_sprite *sprite_array     = game_editor->model.sprites;
		u32 t = 0;
		while(t < selected_sprite_count)
		{
			u32 sprite_index = cursor_memory->selected_meshes[t].index;
			sprite_clipboard[t] = sprite_array[sprite_index];

			t++;
		}
		game_editor->model.sprite_clipboard_count = selected_sprite_count;

		Assert(game_editor->model.sprite_clipboard_count < editor_SPRITE_CLIPBOARD_MAX);
	}
}

inline void
editor_model_cut_selected_sprites(s_game_editor *game_editor)
{
	editor_model_copy_selected_sprites(game_editor);
    editor_cursor_memory *cursor_memory = &game_editor->model.cursor_memory;

	u32 selected_sprites_count = cursor_memory->selected_meshes_count;
	if(selected_sprites_count)
	{
	    //editor_model_record_cut_sprites(game_editor);

	    cursor_memory->selected_meshes_count = 0;

		//order the indices in descending order to correctly shift the selected indices.
		temporary_area temporary_sort_area = temporary_area_begin(&game_editor->area);

		u32 *index_descending_array = memory_area_push_array(&game_editor->area, u32, selected_sprites_count);

		u32 t = 0;
		while(t < selected_sprites_count)
		{
			index_descending_array[t] = cursor_memory->selected_meshes[t].index;
			t++;
		}
		u32_array_insertion_sort_descending(index_descending_array, selected_sprites_count);

		//remove sprites in descending order
	    t = 0;
	    while(t < selected_sprites_count)
	    {
	    	u32 sprite_index = index_descending_array[t];
            editor_model_remove_sprite(game_editor, sprite_index);
			t++;
	    }

		temporary_area_end(&temporary_sort_area);
	}
}

inline void
editor_model_paste_selected_sprites(s_game_editor *game_editor)
{
    editor_cursor_memory *cursor_memory = &game_editor->model.cursor_memory;

	u32 sprite_clipboard_count = game_editor->model.sprite_clipboard_count;

	if(sprite_clipboard_count)
	{

		editor_mesh_selection *selection_array = cursor_memory->selected_meshes;
		cursor_memory->selected_meshes_count   = sprite_clipboard_count;

		model_sprite *sprite_clipboard    = game_editor->model.sprite_clipboard;
		model_sprite *sprite_array        = game_editor->model.sprites;

		u32 new_sprites_index_start    = game_editor->model.sprite_count;
        game_editor->model.sprite_count += sprite_clipboard_count;

		u32 t = 0;
		while(t < sprite_clipboard_count)
		{
			u32 new_sprite_index = new_sprites_index_start + t;
			sprite_array[new_sprite_index] = sprite_clipboard[t];
		    //add sprite to selection array
			editor_mesh_selection *selection_data = selection_array + t;
			selection_data->index       = new_sprite_index; 
			selection_data->v0_selected = 1;
			selection_data->v1_selected = 1;
			selection_data->v2_selected = 1;
			selection_data->v3_selected = 1;

			t++;
		}
		//editor_model_record_pasted_sprites(game_editor);
	}
}


static void
editor_model_add_texture(s_game_editor *editor,
		                       game_resource_attributes *sprite_sheet)
{

	s_model_editor *model_editor = &editor->model;
	stream_data *info_stream = &editor->info_stream;
	if(sprite_sheet && sprite_sheet->type == asset_type_image)
	{
		//look if it got already added.
		b32 already_added = 0;
		for(u32 s = 0;
				s < model_editor->sprite_sheets_count && !already_added;
				s++)
		{
			already_added = model_editor->sprite_sheets[s] == sprite_sheet;
		}
		//not added yet, so go for it!
		if(!already_added)
		{
			model_editor->sprite_sheets[model_editor->sprite_sheets_count] = sprite_sheet;
			model_editor->sprite_sheets_as_asset[model_editor->sprite_sheets_count] = &sprite_sheet->asset_key->image;
			model_editor->sprite_sheets_count++;
		}
	}
}

static editor_model_bone *
editor_model_add_bone(s_game_editor *game_editor)
{
	Assert(game_editor->model.bone_count + 1 < game_editor->model.boneCapacity);

	editor_model_bone *editor_bone = game_editor->model.bones + game_editor->model.bone_count;
	memory_clear(editor_bone, sizeof(*editor_bone));
	//create sprites and frame keys arrays
	memory_dyarray_create_safe(
			game_editor->model.dyarrays_area, editor_bone->sprites, editor_model_sprite, 5, 5);
	editor_bone->frame_keys = memory_dyarray_create(
			game_editor->model.dyarrays_area, editor_frame_key, 1, 2);

	//Clear and set default name
	editor_name_chunks_addf(&game_editor->model.bone_name_chunks,
			               "Bone %u",
						   game_editor->model.bone_count);

	//Reset to default
	model_bone zeroBone = {0};
	editor_bone->base = zeroBone;
	editor_bone->base.q = QUAT(1, 0, 0, 0);
	//bone->uvs_count = model_sprite_MINIMUMORIENTATION;

	game_editor->model.bone_count++;

	return(editor_bone);

}

static void
editor_remove_bone(s_game_editor *game_editor, u32 bone_index)
{
	Assert(bone_index < game_editor->model.boneCapacity);
	s_model_editor *model_editor = &game_editor->model;


	if(bone_index < game_editor->model.bone_count)
	{
        u32 deletingBoneIndex = bone_index;

	    if((i32)bone_index < game_editor->model.bone_count - 1)
	    {
			editor_animation_remove_row(
					game_editor,
					bone_index,
					0);
	    	//Move deleting bone to the end of the array
	    	i32 b = bone_index;
	    	editor_model_bone *bone_array = game_editor->model.bones;

	    	while(b < game_editor->model.bone_count - 1)
	    	{
				//Move the deleting bone foward
	    		bone_array[b] = bone_array[b + 1];

				//Make sure the data stays correct
         	    editor_model_bone *boneAt = bone_array + b;
         	    //Subtract one to the parent index
         	    if(boneAt->base.parent > deletingBoneIndex)
         	    {
         	        boneAt->base.parent--;
         	    }
         	    else if(boneAt->base.parent == deletingBoneIndex)
         	    {
         	        //Put parent back to root
         	        boneAt->base.parent = 0;
         	    }
	    		b++;
	    	}

	    	u32 s = 0;
	        u32 sprite_count = game_editor->model.sprite_count;
	    	model_sprite *sprite_array = game_editor->model.sprites;

	    	while(s < game_editor->model.sprite_count)
	    	{
				//read by descending
				u32 sprite_index = sprite_count - 1 - s;
				model_sprite *sprite = sprite_array + s;

				if(sprite->bone_index > deletingBoneIndex)
				{
					sprite->bone_index--;
				}
				else if(sprite->bone_index == deletingBoneIndex)
				{
                    editor_model_remove_sprite(game_editor, s);
				}
	    		s++;
	    	}

	    }
		editor_name_chunks_remove(&game_editor->model.bone_name_chunks, bone_index);
	    game_editor->model.bone_count--;
	}
}

inline void
editor_model_reset(s_game_editor *editor)
{
	editor->model.model_orientation_count = 1;
	editor->model.first_free_column = 0;
	//reset animation timer
	editor->model.timer.dt_transcurred = 0;
	editor->model.timer.dt_current = 0;
	editor->model.timer.frame_current = 0;

	//reset name chunks
	editor->model.bone_name_chunks.count = 0;
	editor->model.sprite_name_chunks.count = 0;
	editor->model.animation_name_chunks.count = 0;
	editor->model.frame_list_names.count = 0;
	editor->model.frame_key_names.count = 0;

	editor_cursor_memory *selections_memory = &editor->model.cursor_memory;
	selections_memory->hot_vertices = 0;
	selections_memory->selected_meshes_count = 0;

	editor->model.cursor.position = vec3_all(0);

	memory_clear(&editor->model.reset, sizeof(editor->model.reset));


//	memory_expandable_zones_wipe(editor->model.dyarrays_area);
	memory_expandable_zones_wipe(editor->model.dyarrays_area);


}

inline void
editor_animation_check_mesh_frames_array(s_game_editor *game_editor)
{
}

static void
editor_model_save_new(s_editor_state *editor_state)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_model_editor *model_editor = &game_editor->model;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &game_editor->info_stream;

	u32 bone_count = model_editor->bone_count;
	u32 sprite_count = model_editor->sprite_count;
	u32 sprite_sheet_count = model_editor->sprite_sheets_count;
	//u32 uvs_count            = game_editor->model.uvs_count;
	//u32 animation_count = model->animation_count;
	if(!model_editor->editing_model)
	{
		return;
	}
	Assert(model_editor->editing_model->type == asset_type_model);

	u8 *path_and_name = model_editor->editing_model->path_and_name;
	editor_wr wr = editor_wr_begin_write(
			&game_editor->area,
			platform,
			platform_file_op_create_new,
			path_and_name);
	if(wr.file.handle)
	{
		//header
		ppse_model_header_new *model_file_header = editor_wr_put_header(&wr, ppse_model_header_new);

		model_file_header->orientation_amount = (u8)model_editor->model_orientation_count;
		model_file_header->bone_count = bone_count;
		model_file_header->sprite_count = sprite_count;
		model_file_header->sprite_sheet_count = sprite_sheet_count;
		model_file_header->animation_count = model_editor->animation_count;
		model_file_header->keyframe_count = model_editor->keyframe_count;
		model_file_header->frame_keyframe_count = model_editor->frame_keyframe_count;
		model_file_header->frame_list_count = model_editor->frame_list_count;

		model_file_header->header.signature = (ppse_model_SIGNATURE);
		model_file_header->header.version = ppse_model_next_version;

		editor_wr_put_line(&wr);
		{
			//model_file_header.animation_count = animation_count;
			//composite resources after header.
			model_file_header->header.offset_to_composite_resources = wr.data_offset;
			model_file_header->header.composite_resource_count = model_editor->sprite_sheets_count;
			model_file_header->line_to_sheet_headers = wr.current_line_number;
			for(u32 s = 0;
					s < sprite_sheet_count;
					s++)
			{
				game_resource_attributes *texture = model_editor->sprite_sheets[s];
				editor_wr_write_composite_resource_header(
						&wr,
						texture->path_and_name);
			}
		}
		//save bones and mesh first

		editor_wr_put_line(&wr);
		{
			model_file_header->line_to_bones = wr.current_line_number;
			//
			// save model bones 
			//
			for(u32 b = 0; b < bone_count; b++)
			{
				editor_model_bone *editor_model_bone = model_editor->bones + b;
				model_bone *model_bone = &editor_model_bone->base;
				ppse_model_bone *file_model_bone = editor_wr_put_struct(&wr, ppse_model_bone);

				//ppse_model_sprite_depth depth = DC(ppse_model_sprite_depth, f32 x, f32 y, f32 y);
				//fill data

				file_model_bone->parent = model_bone->parent;
				file_model_bone->p = model_bone->p;
				file_model_bone->displacement = model_bone->displacement;
				file_model_bone->two_dim = model_bone->two_dim;
				file_model_bone->q = model_bone->q;
				file_model_bone->frame_key_count = editor_model_bone->frame_key_count;
				file_model_bone->sprite_count = editor_model_bone->sprite_count;
				//save bone sprites
				for(u32 s = 0; s < editor_model_bone->sprite_count; s++)
				{
					editor_model_sprite *e_sprite = 0;
					//get sprite from bone
					memory_dyarray_get_safe(editor_model_bone->sprites, e_sprite, s);
					model_sprite *save_model_sprite = &e_sprite->base;
					ppse_model_sprite *file_sprite = editor_wr_put_struct(&wr, ppse_model_sprite);

					//ppse_model_sprite_depth depth = DC(ppse_model_sprite_depth, f32 x, f32 y, f32 y);

					file_sprite->texture_index  = save_model_sprite->texture_index;
					file_sprite->type = save_model_sprite->type;
					file_sprite->bone_index = save_model_sprite->bone_index;
					file_sprite->p = save_model_sprite->p;
					file_sprite->depth_x = save_model_sprite->depth_x;
					file_sprite->depth_y = save_model_sprite->depth_y;
					file_sprite->depth_z = save_model_sprite->depth_z;
					file_sprite->pivotX = save_model_sprite->pivotX;
					file_sprite->pivotY = save_model_sprite->pivotY;
					file_sprite->pivotZ = save_model_sprite->pivotZ;
					file_sprite->frame_list_index = save_model_sprite->frame_list_index;
					file_sprite->extra_frame_count = save_model_sprite->extra_frame_count;

					file_sprite->face_axis = save_model_sprite->face_axis;


					file_sprite->v0 = save_model_sprite->size;
					file_sprite->v1 = save_model_sprite->size2;
					file_sprite->v2 = save_model_sprite->v2;
					file_sprite->v3 = save_model_sprite->v3;
					//file_sprite.uvs_count = save_model_sprite->uvs_count;

					//Load dimension data for billboard sprite.
					//Use the maximum orientations since the array is a fixed sized one
					if(file_sprite->type != model_sprite_billboard)
					{
						file_sprite->uvs_count = 1;
					}
					//save uvs
					for(u32 u = 0; u <= file_sprite->extra_frame_count; u++)
					{
						sprite_orientation *uvs = 0;
						memory_dyarray_get_safe(
								e_sprite->uvs, uvs, u);
						ppse_frame_list_uvs *file_uvs = editor_wr_put_struct(&wr, ppse_frame_list_uvs);

						file_uvs->uv0 = uvs->uv0;
						file_uvs->uv1 = uvs->uv1;
						file_uvs->uv2 = uvs->uv2;
						file_uvs->uv3 = uvs->uv3;
						file_uvs->offset_x = uvs->offset.x;
						file_uvs->offset_y = uvs->offset.y;
						file_uvs->offset_z = uvs->offset.z;
						file_uvs->option = uvs->option;
						file_uvs->x_rot_index = (i8)uvs->x_rot_index;
						file_uvs->y_rot_index = (i8)uvs->y_rot_index;
						file_uvs->skin_index = uvs->skin_index;
						//increase the header's total uvs count
						model_file_header->uvs_count++;

					}
				}
				////save bone frame keys
				//for(u32 k = 0; k < editor_model_bone->frame_key_count; k++)
				//{
				//	editor_frame_key *fk = 0;
				//	memory_dyarray_get_safe(editor_model_bone->frame_keys, fk, k);
				//	//save names for the editor.
				//	editor_wr_put_string(&wr, fk->name);
				//}
			}
		}
#if 0
		editor_wr_put_line(&wr);
		{
			model_file_header->line_to_sprites = wr.current_line_number;
			for(u32 b = 0; b < sprite_count; b++)
			{
				model_sprite *save_model_sprite = model_editor->sprites + b;
				ppse_model_sprite *file_sprite = editor_wr_put_struct(&wr, ppse_model_sprite);

				//ppse_model_sprite_depth depth = DC(ppse_model_sprite_depth, f32 x, f32 y, f32 y);

				file_sprite->texture_index  = save_model_sprite->texture_index;
				file_sprite->type = save_model_sprite->type;
				file_sprite->bone_index = save_model_sprite->bone_index;
				file_sprite->p = save_model_sprite->p;
				file_sprite->depth_x = save_model_sprite->depth_x;
				file_sprite->depth_y = save_model_sprite->depth_y;
				file_sprite->depth_z = save_model_sprite->depth_z;
				file_sprite->pivotX = save_model_sprite->pivotX;
				file_sprite->pivotY = save_model_sprite->pivotY;
				file_sprite->pivotZ = save_model_sprite->pivotZ;
				file_sprite->frame_list_index = save_model_sprite->frame_list_index;

				file_sprite->face_axis = save_model_sprite->face_axis;


				file_sprite->v0 = save_model_sprite->size;
				file_sprite->v1 = save_model_sprite->size2;
				file_sprite->v2 = save_model_sprite->v2;
				file_sprite->v3 = save_model_sprite->v3;
				//file_sprite.uvs_count = save_model_sprite->uvs_count;

				//Load dimension data for billboard sprite.
				//Use the maximum orientations since the array is a fixed sized one
				if(file_sprite->type != model_sprite_billboard)
				{
					file_sprite->uvs_count = 1;
				}

			}
		}
		editor_wr_put_line(&wr);
		{
			model_file_header->line_to_frame_lists = wr.current_line_number;
			//pre-defined frames.
			for(u32 f = 0; f < model_editor->frame_list_count; f++)
			{
#if 1
				//first write the frame list data
				editor_model_frame_list *current_list = game_editor->model.frame_lists + f;

				ppse_animation_sprite_frame_list *frame_list = editor_wr_put_struct(&wr, ppse_animation_sprite_frame_list);

				frame_list->sprite_index = current_list->sprite_index;
				frame_list->uvs_count = current_list->uvs_count;
				frame_list->total_frames_count = current_list->total_frames_count;

				//now write the frames uvs
				for(u32 fr = 0;
						fr < frame_list->total_frames_count;
						fr++)
				{
					sprite_orientation *uvs = memory_dyarray_get(
							current_list->mesh_frames, fr);
					ppse_frame_list_uvs *file_uvs = editor_wr_put_struct(&wr, ppse_frame_list_uvs);

					file_uvs->uv0 = uvs->uv0;
					file_uvs->uv1 = uvs->uv1;
					file_uvs->uv2 = uvs->uv2;
					file_uvs->uv3 = uvs->uv3;
					file_uvs->offset_x = uvs->offset.x;
					file_uvs->offset_y = uvs->offset.y;
					file_uvs->offset_z = uvs->offset.z;
					file_uvs->option = uvs->option;
					file_uvs->x_rot_index = (i8)uvs->x_rot_index;
					file_uvs->y_rot_index = (i8)uvs->y_rot_index;
					//increase the header's total uvs count
					model_file_header->uvs_count++;

				}
#endif
			}
		}
#endif

		//animations line
		editor_wr_put_line(&wr);
		{
			model_file_header->line_to_animations = wr.current_line_number;

			for(u32 a = 0; a < model_editor->animation_count; a++)
			{
				editor_animation *editor_animation = model_editor->animations + a;
				model_animation *saving_animation = &editor_animation->base;
				ppse_model_animation *file_model_animation = editor_wr_put_struct(&wr, ppse_model_animation);

				file_model_animation->keyframe_count     = saving_animation->keyframe_count;
				file_model_animation->frame_keyframe_count = saving_animation->frame_keyframe_count;
				file_model_animation->frames_total       = saving_animation->frames_total;
				file_model_animation->loop               = saving_animation->loop;
				file_model_animation->frame_loop_start   = saving_animation->frame_loop_start;
				file_model_animation->frame_loop_end     = saving_animation->frame_loop_end;
				file_model_animation->frames_per_ms      = saving_animation->frames_per_ms;
				file_model_animation->frame_timer        = saving_animation->frame_timer;
				file_model_animation->frame_timer_repeat = saving_animation->frame_timer_repeat;
				file_model_animation->keep_timer_on_transition = saving_animation->keep_timer_on_transition;
				file_model_animation->repeat = saving_animation->repeat;

				stream_pushf(
						info_stream,
						"%u. Saving animation.\n"
						"keyframes: %u\n"
						"frame_keyframes: %u\n",
						a,
						file_model_animation->keyframe_count,
						file_model_animation->frame_keyframe_count);

				//save transform keyframes
				for(editor_animation_keyframe_row *group = editor_animation->first_row;
						group; group = group->next)
				{
					for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
							editor_keyframe;
							editor_keyframe = editor_keyframe->next_in_row)
					{
						ppse_model_animation_keyframe *file_keyframe = editor_wr_put_struct(&wr, ppse_model_animation_keyframe);
						model_animation_keyframe *keyframe = &editor_keyframe->base;

						file_keyframe->bone_index = keyframe->bone_index;
						file_keyframe->spline = keyframe->spline;

						file_keyframe->offset = keyframe->offset;
						file_keyframe->q = keyframe->q;
						file_keyframe->timer_frame = keyframe->timer_frame;
						file_keyframe->frame_repeat = keyframe->timer_frame_repeat;
						file_keyframe->frame_start = keyframe->frame_start;
						file_keyframe->switch_parent = keyframe->switch_parent;
						file_keyframe->parent_index = keyframe->parent_index;

						file_keyframe->rotation_x = keyframe->rotation_x;
						file_keyframe->rotation_y = keyframe->rotation_y;
						file_keyframe->rotation_z = keyframe->rotation_z;
					}
				}
				//save frame keyframes
				for(editor_animation_keyframe_row *group = editor_animation->first_frame_row;
						group; group = group->next)
				{
					for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
							editor_keyframe;
							editor_keyframe = editor_keyframe->next_in_row)
					{
						ppse_model_animation_keyframe *file_keyframe = editor_wr_put_struct(&wr, ppse_model_animation_keyframe);
						//clip keyframeo
						model_animation_keyframe *keyframe = &editor_keyframe->base;


						file_keyframe->mesh_index = keyframe->mesh_index;
						file_keyframe->spline = keyframe->spline;
						file_keyframe->offset = keyframe->offset;
						file_keyframe->frame_start = keyframe->frame_start;
						file_keyframe->frame_key = keyframe->frame_key;
						file_keyframe->flip_h = keyframe->flip_h;
					}
				}
			} //animation_count loop
		}
		  //attachment data

		//editor data line
		
		editor_wr_put_line(&wr);
		{
			model_file_header->line_to_names = wr.current_line_number;

			//bone names
			editor_wr_write_name_chunks(
					&wr,
					&model_editor->bone_name_chunks,
					info_stream);
			//frame key names
			editor_wr_write_name_chunks(
					&wr,
					&model_editor->frame_key_names,
					info_stream);
			
			////mesh names
			//editor_wr_write_name_chunks(
			//		&wr,
			//		&model_editor->sprite_name_chunks,
			//		info_stream);
			////sprite list names
			//editor_wr_write_name_chunks(
			//		&wr,
			//		&model_editor->frame_list_names,
			//		info_stream);
			//animation names
			editor_wr_write_name_chunks(
					&wr,
					&model_editor->animation_name_chunks,
					info_stream);
		}
	}

	editor_wr_end(&wr);
	game_console_PushLinef(game_editor->game_console, "Saved model file to %s", path_and_name);
}




static u32 
editor_model_load(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	s_game_editor *game_editor = &editor_state->editor;
	s_model_editor *model_editor = &game_editor->model;
	u32 success = 0;
	if(!resource)
	{
		return(success);
	}
	Assert(resource->type == asset_type_model);

	platform_api *platform = editor_state->platform;
	editor_wr wr = editor_wr_begin_read(
			&game_editor->area,
			platform,
			resource->path_and_name);

	if(wr.file.handle)
	{
		editor_model_reset(game_editor);

		model *loaded_model = &resource->asset_key->model;
		game_editor->model.uvs_count = loaded_model->uvs_count;
		game_editor->model.editing_model = resource;

		u32 bone_count = loaded_model->bone_count; 
		u32 sprite_count = loaded_model->sprite_count; 
		model_editor->model_orientation_count = loaded_model->orientation_amount;
		model_editor->model_orientation_count = loaded_model->orientation_amount == 0 ?
			1 : loaded_model->orientation_amount;

		//load bones
		for(u32 b = 0; b < loaded_model->bone_count; b++)
		{
			editor_model_bone *e_bone = editor_model_add_bone(game_editor);
			model_bone *loaded_bone = loaded_model->bones + b;
			e_bone->base = loaded_model->bones[b];
			//add frame keys
			for(u32 f = 0; f < e_bone->base.frame_key_count; f++)
			{
				em_add_frame_key_to_bone(game_editor, e_bone);
			}
			//load bone sprites
			for(u32 b = 0; b < loaded_bone->sprite_count; b++)
			{
//				model_sprite *editor_model_sprite = editor_model_add_sprite(game_editor); 
				model_sprite *sprite = loaded_model->sprites + loaded_bone->sprites_at + b;
				editor_model_sprite *e_sprite = em_add_sprite_to_bone(game_editor, e_bone);
				e_sprite->base = *sprite;
				u32 frame_count = e_sprite->base.extra_frame_count;
				e_sprite->base.extra_frame_count = 0;
				//load frames
				for(u32 u = 0; u <= frame_count; u++)
				{
					sprite_orientation *e_frame = 0;
					sprite_orientation *frame = loaded_model->uvs + sprite->frame_at + u;
					if(!u)
					{
						memory_dyarray_get_safe(e_sprite->uvs,
								e_frame, u);
					}
					else
					{
						e_frame = em_add_frame_to_sprite(game_editor, e_sprite);
					}
					*e_frame = *frame;
				}
			}
		}
		//temp load sprites
		if(0)
		{
			for(u32 b = 0; b < loaded_model->sprite_count; b++)
			{
//				model_sprite *editor_model_sprite = editor_model_add_sprite(game_editor); 
				model_sprite *sprite = loaded_model->sprites + b;
				editor_model_bone *e_bone = model_editor->bones + sprite->bone_index;
				editor_model_sprite *e_sprite = em_add_sprite_to_bone(game_editor, e_bone);
				e_sprite->base = *sprite;
				u32 frame_count = e_sprite->base.extra_frame_count;
				e_sprite->base.extra_frame_count = 0;
				//load frames
				for(u32 u = 0; u <= frame_count; u++)
				{
					sprite_orientation *e_frame = 0;
					sprite_orientation *frame = loaded_model->uvs + sprite->frame_at + u;
					if(u)
					{
						memory_dyarray_get_safe(e_sprite->uvs,
								e_frame, u);
					}
					else
					{
						e_frame = em_add_frame_to_sprite(game_editor, e_sprite);
					}
					*e_frame = *frame;
				}
			}
		}
		//uvs
		//for(u32 u = 0; u < loaded_model->uvs_count; u++)
		//{
		//	sprite_orientation *editor_uvs = game_editor->model.uvs + u;
		//	*editor_uvs = loaded_model->uvs[u];
		//}
		////frame lists
		//for(u32 f = 0; f < loaded_model->frame_list_count; f++)
		//{
		//	model_mesh_frame_list *frame_list = loaded_model->mesh_frame_list + f;
		//	editor_model_frame_list *editor_frame_list = editor_model_add_frame_list(game_editor);
		//	u32 frame_amount = frame_list->total_frames_count;
		//	//set data
		//	editor_frame_list->uvs_count = frame_list->uvs_count;
		//	editor_frame_list->sprite_index = frame_list->sprite_index;
//		//	editor_frame_list->total_frames_count = frame_amount;
		//	editor_model_change_frame_list_frame_count(editor_frame_list,
		//			frame_amount);
		//	//set uvs
		//	for(u32 u = 0; u < frame_amount; u++)
		//	{
		//		sprite_orientation *model_orientation = &loaded_model->uvs[frame_list->uvs_at + u];
		//		sprite_orientation *uvs = memory_dyarray_get(editor_frame_list->mesh_frames, u);
		//		uvs->uvs = model_orientation->uvs;
		//		uvs->offset = model_orientation->offset;
		//		uvs->option = model_orientation->option;
		//		uvs->x_rot_index = model_orientation->x_rot_index;
		//		uvs->y_rot_index = model_orientation->y_rot_index;
		//	}
		//}

		//animations
#if 1
		for(u32 a = 0; a < loaded_model->animation_count; a++)
		{
			editor_animation *editor_animation = editor_model_add_animation(game_editor);
			model_animation *animation = loaded_model->animations + a;
			editor_animation->base = *animation;
			//counts increase by adding keyframes
			editor_animation->base.keyframe_count = 0;
			editor_animation->base.frame_keyframe_count = 0;

			//keyframes
			for(u32 c = 0; c < animation->keyframe_count; c++)
			{
				model_animation_keyframe *kf = loaded_model->keyframes + 
					animation->keyframes_at + c;

				editor_animation_keyframe *editor_kf = editor_animation_add_keyframe(
						game_editor,
						editor_animation,
						kf->bone_index,
						0,
						kf->frame_start);
				editor_kf->base = *kf;
			}
			//frame keyframes
			for(u32 c = 0; c < animation->frame_keyframe_count; c++)
			{
				model_animation_keyframe *kf = loaded_model->frame_keyframes + 
					animation->frame_keyframes_at + c;

				editor_animation_keyframe *editor_kf = editor_animation_add_keyframe(
						game_editor,
						editor_animation,
						kf->mesh_index,
						1,
						kf->frame_start);
				editor_kf->base = *kf;
				editor_kf->base.type = model_animation_keyframe_frame;
			}
		}
#endif

		//composite sprite sheets
		for(u32 s = 0; s < resource->model.texture_count; s++)
		{
			editor_model_add_texture(game_editor, resource->model.textures[s].attributes);
		}
		//get file header
		ppse_model_header_new *header = editor_wr_read_struct(&wr, ppse_model_header_new);
		editor_wr_read_to_line(&wr, header->line_to_names);
		//load names
		//bone names
		editor_wr_set_name_chunks(
				&wr,
				&model_editor->bone_name_chunks,
				0);
		//frame key names
		editor_wr_set_name_chunks(
				&wr,
				&model_editor->frame_key_names,
				0);
		//frame list names
	//	editor_wr_set_name_chunks(
	//			&wr,
	//			&model_editor->frame_list_names,
	//			0);
		//animation names
		editor_wr_set_name_chunks(
				&wr,
				&model_editor->animation_name_chunks,
				0);

		editor_wr_end(&wr);
	}
	return(success);
}

static void
editor_model_fill_sprite_vertices(
		s_game_editor *game_editor,
		game_renderer *game_renderer,
		u32 sprite_index,
		vec3 *v0_ptr,
		vec3 *v1_ptr,
		vec3 *v2_ptr,
		vec3 *v3_ptr
		)
{
	model_sprite *model_sprite_array = game_editor->model.sprites;
    model_sprite *current_sprite = model_sprite_array + sprite_index;

	model_bone *parent_bone = game_editor->model.loaded_model_pose.bones + current_sprite->bone_index;

	vec3 v0 = {0, 0, 0};
	vec3 v1 = {0, 0, 4};
	vec3 v2 = {4, 0, 4};
	vec3 v3 = {4, 0, 0};

			vec3 pivot_offset = {
				current_sprite->pivotX,
				current_sprite->pivotY,
				current_sprite->pivotZ,
			};
    if(current_sprite->type == model_sprite_billboard)
	{
		if(current_sprite->face_axis == billboard_face_x)
		{
			mesh_points points = get_mesh_billboard_points_x(game_renderer, v0, v1, v2, v3, pivot_offset);
			v0 = points.v0;
			v1 = points.v1;
			v2 = points.v2;
			v3 = points.v3;
		}
		else
		{
			mesh_points points = get_mesh_billboard_points(game_renderer, v0, v1, v2, v3, pivot_offset);
			v0 = points.v0;
			v1 = points.v1;
			v2 = points.v2;
			v3 = points.v3;
		}
	}


	v0 = vec3_add(parent_bone->transformed_p, v0);
	v1 = vec3_add(parent_bone->transformed_p, v1);
	v2 = vec3_add(parent_bone->transformed_p, v2);
	v3 = vec3_add(parent_bone->transformed_p, v3);

	*v0_ptr = v0;
	*v1_ptr = v1;
	*v2_ptr = v2;
	*v3_ptr = v3;
}
inline ray_hit_result
editor_model_ray_hits_sprite(
		s_game_editor *game_editor,
		game_renderer *game_renderer,
		vec3 ray_origin,
		vec3 ray_dir,
		u32 sprite_index,
		f32 model_sprite_hit_distance)
{
	model_sprite *model_sprite_array = game_editor->model.sprites;
    model_sprite *current_sprite = model_sprite_array + sprite_index;

	vec3 v0 = {0};
	vec3 v1 = {0};
	vec3 v2 = {0};
	vec3 v3 = {0};
	editor_model_fill_sprite_vertices(
			game_editor,
			game_renderer,
			sprite_index,
			&v0,
			&v1,
			&v2,
			&v3
			);
	ray_hit_result hit_result = ray_quad_get_hit_result(ray_origin,
			                                  ray_dir,
											  v0,
											  v1,
											  v2,
											  v3,
											  model_sprite_hit_distance);

	return(hit_result);
}

static void
editor_model_select_sprites_against_ray(
		s_game_editor *game_editor,
		game_renderer *game_renderer,
		vec3 ray_origin,
		vec3 ray_dir)
{
	u32 sprite_count = game_editor->model.sprite_count;
	model_sprite *model_sprite_array = game_editor->model.sprites;
//cast ray against sprites (selected and unselected)
    for(u32 s = 0;
			s < sprite_count;
			s++)
    {

       ray_hit_result hit_result = editor_model_ray_hits_sprite(
		game_editor,
		game_renderer,
		ray_origin,
		ray_dir,
		s,
		1000000);
		//closest sprite hit
		if(hit_result.hit)
		{
            editor_cursor_memory_add_selection(
					&game_editor->model.cursor_memory,
					s);
		}
    
    }
}
static void
editor_model_update_camera(
		s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input)
{
    b32 input_text_focused   = editor_state->ui->input.input_text.focused;
	u32 lock_camera_rotation = input_text_focused;
	s_game_editor *game_editor = &editor_state->editor;
	f32 camera_rotation_x = game_editor->model.camera_rotation_x;
	f32 camera_rotation_z = game_editor->model.camera_rotation_z;
    u32 mouse_r_down = input_down(editor_input->mouse_right);
    u32 mouse_l_down = input_down(editor_input->mouse_left);
    u32 mouse_m_down = editor_input->mouse_middle; 
    u16 ui_focused = game_editor->ui_is_focused;
	//model editor camera
	//rotate camera
	if(!lock_camera_rotation && 
		editor_state->platform->window_is_focused &&
		!ui_focused)
	{
		f32 mouse_delta_x = editor_input->mouse_clip_x_last - editor_input->mouse_clip_x;
		f32 mouse_delta_y = editor_input->mouse_clip_y_last - editor_input->mouse_clip_y;
		camera_rotation_x = game_editor->model.camera_rotation_x;
		camera_rotation_z = game_editor->model.camera_rotation_z;
		if(game_editor->model.tool == model_mode_view || editor_input->spaceBarDown)
		{
			//rotate model camera
			if(mouse_l_down)
			{
				game_editor->model.in_camera_mode = 1;

				camera_rotation_z += ((f32)mouse_delta_x * PI * 0.0005f); 
				camera_rotation_z = camera_rotation_z < 0 ? 2.0f : camera_rotation_z >= 2.0f ? 0 : camera_rotation_z;
			}
			if(mouse_m_down)
			{
				camera_rotation_x += ((f32)mouse_delta_y * PI * 0.0005f); 
				camera_rotation_x = camera_rotation_x < 0 ? 2.0f : camera_rotation_x >= 2.0f ? 0 : camera_rotation_x;
			}
			else if(mouse_r_down)
			{
				f32 mouse_delta_x = editor_input->mouse_x - editor_input->mouse_x_last;
				f32 mouse_delta_y = editor_input->mouse_y - editor_input->mouse_y_last;

				f32 paddingSpeed = 0.02f * game_editor->model.cameraDistance * 0.5f;
				paddingSpeed = (paddingSpeed > 0.2f) ? 
					0.2f : (paddingSpeed < 0.02f) ? 0.02f : paddingSpeed;

				vec3 add_xAxis = vec3_scale(game_renderer->camera_x, mouse_delta_x * paddingSpeed);
				vec3 add_yAxis = vec3_scale(game_renderer->camera_y, mouse_delta_y * paddingSpeed);

				game_editor->model.camera_position.x -= add_xAxis.x - add_yAxis.x;
				game_editor->model.camera_position.y -= add_xAxis.y - add_yAxis.y;
				game_editor->model.camera_position.z -= add_xAxis.z - add_yAxis.z;

				//game_editor->model.cameraDistance = vec3_length(game_editor->model.camera_position);
			}
		}


		game_editor->model.camera_rotation_x = camera_rotation_x;
		game_editor->model.camera_rotation_z = camera_rotation_z;
		game_renderer->camera_rotation.x     = camera_rotation_x;
		game_renderer->camera_rotation.z     = camera_rotation_z;

	}
	//set position by rotation
	if(!ui_focused)
	{

		i32 mouseWheelValue = editor_input->mouse_wheel * 10;
		game_editor->model.cameraDistance -= mouseWheelValue;

		if(game_editor->model.cameraDistance < 0)
		{
			game_editor->model.cameraDistance = 0;

			//instead, add the new value to the pad location
			vec3 add_zAxis = vec3_scale(game_renderer->camera_z, (f32)mouseWheelValue);

			game_editor->model.camera_position.x -= add_zAxis.x;
			game_editor->model.camera_position.y -= add_zAxis.y;
			game_editor->model.camera_position.z -= add_zAxis.z;
		}
	}

	vec3 model_editor_origin = {0, 0, 0};
	vec3 distanceFromCamera = {0, 0, game_editor->model.cameraDistance};

	matrix4x4 camera_rotation = 
		matrix4x4_rotation_scale(game_renderer->camera_rotation.x,
				game_renderer->camera_rotation.y,
				game_renderer->camera_rotation.z);

	distanceFromCamera           = matrix4x4_v3_mul_rows(camera_rotation, distanceFromCamera, 0);
	game_renderer->camera_position = vec3_add(game_editor->model.camera_position,
			distanceFromCamera);

	game_renderer->camera_rotation.x = camera_rotation_x;
	game_renderer->camera_rotation.z = camera_rotation_z;
	//game_renderer->camera_position = game_editor->model.camera_position;


 
}


static void
editor_model_apply_ik_fabrik(
		s_game_editor *editor,
		game_renderer *game_renderer,
		editor_animation *editing_animation,
		editor_animation_keyframe *editor_keyframe,
		vec3 target)
{
	s_model_editor *model_editor = &editor->model;

	temporary_area temp_bones_area = temporary_area_begin(&editor->area);
	//allocate a pose at this frame
	model_editor->loaded_model_ik_pose = render_allocate_pose(
			&editor->area, model_editor->loaded_model);
	//transform bones using the current frame
	for(editor_animation_keyframe *kf = editor_keyframe->column->first_keyframe;
			kf; kf = kf->next_in_column)
	{
		model_bone *bone = model_editor->loaded_model_ik_pose.bones + kf->base.bone_index;
		bone->q = quaternion_mul(bone->q, kf->base.q);
		bone->p = vec3_add(bone->p, kf->base.offset);
	}
	//Apply the transformations
	model_fill_bone_transformed_data(
			game_renderer,
			model_editor->loaded_model_ik_pose,
			model_editor->loaded_model.bone_count,
			model_editor->model_foward);
	model_pose transformed_pose = render_allocate_pose(&
			editor->area, model_editor->loaded_model);
	model_fill_bone_transformed_data(
			game_renderer,
			transformed_pose,
			model_editor->loaded_model.bone_count,
			model_editor->model_foward);

	model_bone *bones = model_editor->loaded_model_ik_pose.bones;
	//this is needed because later it is needed to take que original rotation into acount
	model_bone *bones_og = transformed_pose.bones;//transformed_pose.bones;
	u32 target_bone_index = editor_keyframe->base.bone_index;
	//current target bone
	model_bone *bone = bones + editor_keyframe->base.bone_index;
	target = vec3_add(target, bone->transformed_p);
	//	target = model_editor->ik_vec;
	model_bone *parent_bone = bones + bone->parent;
	f32 b = 0;
	//distance between target and end position
	u32 last_parent_index = bone->parent;
	u32 prev_parent_index = last_parent_index;
	//no ascendants
	if(parent_bone == bone || !bone->dof)
	{
		temporary_area_end(&temp_bones_area);
		return;
	}
	u32 hierarchy_count = 1;
	{

		u32 prev_parent_index = bone->parent;
		u32 last_parent_index = prev_parent_index;
		model_bone *before_root = 0;
		model_bone *ascending_bone = parent_bone;
		do
		{
			hierarchy_count++;
			//			if(last_parent_index == 17)
			if(!ascending_bone->dof)
			{
				break;
			}

			prev_parent_index = last_parent_index;
			last_parent_index = ascending_bone->parent;

			ascending_bone = bones + ascending_bone->parent;
		}while(prev_parent_index != last_parent_index);
	}

	u32 number_of_passes = 1000;
	//backward_points
	vec3 *points_b = memory_area_clear_and_push_array(&editor->area,
			vec3, hierarchy_count);
	//foward points
	vec3 *points_f = memory_area_clear_and_push_array(&editor->area,
			vec3, hierarchy_count);
	//lengths
	f32 *distances = memory_area_push_array(&editor->area, f32 ,hierarchy_count);
	//indices for last foward pass
	u16 *indices = memory_area_push_array(&editor->area, u16,hierarchy_count + 1);
	//for every keyframe that is part of the hierarchy
	editor_animation_keyframe **hierarchy_keyframes = memory_area_push_array(
			&editor->area, editor_animation_keyframe *, hierarchy_count);



	f32 total_hierarchy_length = 0;
	u32 index = hierarchy_count - 1;
	distances[index] = vec3_length(bone->p);
	indices[index + 1] = target_bone_index;
	indices[index] = bone->parent;
	points_f[index] = bone->transformed_p;
	hierarchy_keyframes[index] = editor_keyframe;
	index--;
	total_hierarchy_length = vec3_length(bone->p);

	model_bone *root_bone = parent_bone;
	{

		u32 prev_parent_index = bone->parent;
		u32 last_parent_index = prev_parent_index;
		model_bone *before_root = 0;
		model_bone *ascending_bone = parent_bone;
		do
		{

			//fill all of the bone points on this keyframe
			points_f[index] = ascending_bone->transformed_p;
			//distance between points
			distances[index] = vec3_length(ascending_bone->p);
			//the "root" of the hierarchy doesn't have a length
			total_hierarchy_length += index ? distances[index] : 0;
			indices[index] = ascending_bone->parent;
			//add keyframe to this column if needed
			editor_animation_keyframe *kf = editor_animation_add_keyframe(editor,
					editing_animation,
					last_parent_index,
					model_animation_keyframe_transform,
					editor_keyframe->base.frame_start);
			hierarchy_keyframes[index] = kf;

			index--;
			if(!ascending_bone->dof)
			{
				break;
			}

			prev_parent_index = last_parent_index;
			last_parent_index = ascending_bone->parent;

			ascending_bone = bones + ascending_bone->parent;
		}while(prev_parent_index != last_parent_index);

		root_bone = ascending_bone;
	}
	vec3 distance_target_root = vec3_sub(target, root_bone->transformed_p);
	f32 distance_tr_length = vec3_length(distance_target_root);
	if(total_hierarchy_length < distance_tr_length)
	{
		//set the vector target inside the reach
		target = vec3_scale(vec3_normalize(distance_target_root), total_hierarchy_length);
		target = vec3_add(target, root_bone->transformed_p);
	}
	index = hierarchy_count - 1;
	//can't reach target!
	points_b[index] = target;
	vec3 f_end = points_f[index];
	{
		u32 passes_index = 0;
		f32 distance_end_target = vec3_length(vec3_sub(target, f_end));
		while(passes_index < 1000 && distance_end_target > 0.000001f)
		{
			//loop back and forth
			index = hierarchy_count - 1;
			vec3 end = points_b[index];
			//backward pass
			while(index)
			{
				u32 parent_i = index - 1;
				//backwards, start from end and adjust coordinates
				vec3 p0 = points_b[index];
				vec3 p1 = points_f[parent_i];
				vec3 distance_parent_bone = vec3_sub(p1, p0);
				vec3 distance_parent_bone_n = vec3_normalize(distance_parent_bone);
				f32 dpb_length = distances[index];
				//new position of parent
				vec3 p_parent = vec3_scale(distance_parent_bone_n, dpb_length);
				p_parent = vec3_add(end, p_parent);
				end = p_parent;
				points_b[parent_i] = p_parent;

				index--;
			}
			index = 0;
			//foward pass
			while(index < hierarchy_count - 1)
			{
				u32 child_i = index + 1;
				vec3 p0 = points_f[index];
				vec3 p1 = points_b[child_i];
				vec3 distance_p10 = vec3_normalize(vec3_sub(p1, p0));
				f32 dpb_length = distances[child_i];
				vec3 p2 = vec3_add(p0, vec3_scale(distance_p10, dpb_length));
				points_f[child_i] = p2;
				index++;
			}
			passes_index++;

			f_end = points_f[hierarchy_count - 1];
			distance_end_target = vec3_length(vec3_sub(target, f_end));
		}
		index = 0;

		//temp
		memory_copy(points_f, model_editor->temp_points,hierarchy_count * sizeof(vec3));
		model_editor->tp_count = hierarchy_count;
	}
	//now fill the rotations
	{
		//fill rotations
		f32 total_angle = 0;
		//total rotation of quaternions
		quaternion q_total = {1, 0, 0, 0};
		quaternion prev_q = {1, 0, 0, 0};
		vec3 total_displacement = {0};
		for(u32 ni = 1; ni < hierarchy_count;
				ni++)
		{
			model_bone *bone = bones + indices[ni];
			model_bone *child = bones + indices[ni + 1];
			model_bone *boneog = bones_og + indices[ni];
			editor_animation_keyframe *kf = hierarchy_keyframes[ni - 1];
			//hierarchy_p = vec2_add(child->p);
			vec3 p0 = points_f[ni - 1];
			vec3 p1 = points_f[ni];

			vec3 pp = bone->p;
			vec3 cp = vec3_add(bone->p, child->p);
			vec3 d_rp = vec3_normalize(vec3_sub(p1, p0));
			//			d_rp = quaternion_v3_mul_foward_inverse(prev_q, d_rp);
			//convert unit vectors to quaternions (function on gmmath, explanation on math study)
			vec3 normal = vec3_normalize_safe(vec3_sub(cp, pp)); 
			//			normal = quaternion_v3_mul_inverse_foward(quaternion_conjugate(prev_q), normal);
			vec3 up = normal;//{0, 0, 1}; 
			prev_q = quaternion_mul(boneog->q, prev_q);
			kf->base.q = quaternion_from_vectors(up, d_rp);
			kf->base.q = quaternion_mul(quaternion_conjugate(prev_q), kf->base.q);
			//vec3 cross = vec3_cross(up, d_rp);
			//f32 w = vec3_inner(up, d_rp);
			//if(w == -1)
			//{
			//	kf->base.q = QUAT(1, 0, 0, 0);
			//}
			//else
			//{
			//	//				quaternion q = quaternion_from_vectors(up, d_rp);
			//	kf->base.q.vector = cross;
			//	kf->base.q.w = 1.0f + w;
			//	kf->base.q = quaternion_normalize_safe(kf->base.q);
			//	//cancel total rotation
			kf->base.q = quaternion_mul(quaternion_conjugate(q_total), kf->base.q);
			//}
			q_total = quaternion_mul(q_total, kf->base.q);
			//	q_total = quaternion_mul(q_total, quaternion_conjugate(bone->q));
		}
	}

	temporary_area_end(&temp_bones_area);
}



	static void
editor_animation_update_render(s_editor_state *editor_state, game_renderer *game_renderer, editor_input *editor_input, f32 dt)
{
}
//not even functions
	static void
editor_model_update_render(s_editor_state *editor_state, game_renderer *game_renderer, editor_input *editor_input, f32 dt)
{
	//	editor_model_update_camera(editor_state, game_renderer, editor_input);

#if 1
	s_game_editor *game_editor = &editor_state->editor;
	game_assets *game_asset_manager = editor_state->editor_assets;

	u32 packed_assets_count         = game_editor->packed_assets_count;

	u32 mouseAtWindow = editor_state->platform->mouseAtWindow;
	

//
// Model editor or editor_model_update_render
// 

	

	s_model_editor *model_editor = &game_editor->model;
	//per-frame data
	//current editing_animation
	editor_animation *editor_animation = model_editor->animation_is_selected && model_editor->selected_animation_index < model_editor->animation_count ?
		model_editor->animations + model_editor->selected_animation_index : 0;
	//make model
	{
		//selection memory
		memory_area_reset(&game_editor->model.per_frame_area);
		editor_cursor_memory_initialize_frame(&game_editor->model.per_frame_area, 
				&game_editor->model.cursor_memory);

		model_editor->loaded_model = editor_model_make_model(game_editor);
		model_editor->loaded_model_pose = render_allocate_pose(
				&model_editor->per_frame_area, model_editor->loaded_model);
		model_fill_bone_transformed_data(
				game_renderer,
				model_editor->loaded_model_pose,
				model_editor->loaded_model.bone_count,
				model_editor->model_foward);
	}


	editor_cursor_memory *cursor_memory = &game_editor->model.cursor_memory;
	editor_cursor *cursor = &game_editor->model.cursor;

	u32 selected_meshes_count = game_editor->model.cursor_memory.selected_meshes_count;
	editor_mesh_selection *selected_meshes_array = game_editor->model.cursor_memory.selected_meshes;

	//initial data

    u8 mouse_l_down = input_down(editor_input->mouse_left);
	u8 mouse_r_down = input_down(editor_input->mouse_right);
    u8 mouse_l_pressed = input_pressed(editor_input->mouse_left);
    u8 mouse_r_pressed = input_pressed(editor_input->mouse_right);

	u8 pressed_u = input_pressed(editor_input->w);
	u8 pressed_d = input_pressed(editor_input->s);
	u8 pressed_l = input_pressed(editor_input->a);
	u8 pressed_r = input_pressed(editor_input->d);

    u32 ui_is_interacting = game_editor->ui_is_interacting; 
    u32 ui_focused        = game_editor->ui_is_focused; 
	u32 in_camera_mode = editor_input->spaceBarDown;

	u32 editor_process_input = game_editor->process_input;
    u32 input_text_focused  = editor_state->ui->input.input_text.focused || editor_state->ui->input.input_text.got_focus;
	u32 cursor_render_and_update = 
		model_editor->tool == model_mode_selection ||
		model_editor->tool == model_mode_nodes;

	//for node and animation modes
	vec4 node_selected     = {160, 0, 0, 255};
	vec4 node_normal_color = {25, 45, 0, 255};

	vec3 cursor_position = game_editor->model.cursor.position;
	//model editor data
	vec3 model_world_position = {0, 0, 0};

	model_sprite *model_sprite_array = model_editor->sprites;
	u32 bone_count = model_editor->bone_count;
	u32 sprite_count = model_editor->sprite_count;
	u32 selected_bone_index = game_editor->model.selected_bone_index;
	u32 selected_sprite_index = game_editor->model.selected_sprite_index;

	u32 individual_sprite_editing       = game_editor->model.individual_sprite_editing;
	u32 individual_sprite_selecting_uvs = game_editor->model.individual_sprite_selecting_uvs;

	if(selected_bone_index >= bone_count)
	{
		selected_bone_index = bone_count;
		if(bone_count)
		{
			selected_bone_index -= 1;
		}
		game_editor->model.selected_bone_index = selected_bone_index;
	}
	if(selected_sprite_index >= sprite_count)
	{
		selected_sprite_index = sprite_count;
		if(selected_sprite_index)
		{
			selected_sprite_index -= 1;
		}
		game_editor->model.selected_sprite_index = selected_sprite_index;
	}
	//I don't know where to put this. set reproduce_animation to 0 if the animation
	//doesn't loop and it's over
//	if(editing_animation && !editing_animation->base.repeat)
//	{
//		model_editor->reproduce_animation = 0;
//	}

	//Set up ray data
	//
    vec3 mouse_world = render_mouse_coordinates_to_world(
			game_renderer, editor_input->mouse_clip, 1.0f);
    vec3 ray_origin = game_renderer->camera_position;
    vec3 ray_dir = vec3_normalize_safe(vec3_sub(mouse_world, ray_origin));
	struct s_ray ray_od = {ray_origin, ray_dir};
	f32 bone_radius = em_BONE_RADIUS;
    f32 modelBoneHitDistance = 1e6f;
	f32 model_sprite_hit_distance = 100000.0f;
	f32 bone_ray_hit_distance = model_sprite_hit_distance;
	
	u16 sprite_hit = 0;
	u16 sprite_hit_index  = 0;

	u16 sprite_hit_is_selected       = 0;
	u16 sprite_selection_hit_index = 0;
	u32 sprite_hovering_edge = 0;

	game_editor->settings.vertices_cube_size = 0.9f;
	f32 vertices_cube_size = game_editor->settings.vertices_cube_size;
	f32 gizmo_cube_size = 1.0f;

	u32 model_cursor_moved = 0;
	//for record
	u32 started_selection_move = 0;
	u32 record_selection_move = 0;
	u32 gizmo_hot = 0;
    // if draw model UP then offset everything to the origin foward, not the z from camera
	f32 cursor_alpha = 255;
	if(in_camera_mode)
	{
		cursor_alpha = 60;
	}

	//non-depth test commands
	render_commands *commands = render_commands_begin(game_renderer, render_flags_Blending);

	//draw origin point
	render_draw_cube(commands,
			vec3_all(0),
			vec3_all(1),
			V4(0, 0, 255, 180));

	//cancel movement/resize
	if(in_camera_mode || (!mouse_l_down && !editor_input->shift_l))
	{
		//record last movement
		record_selection_move = game_editor->model.moving_gizmo;

	    game_editor->model.moving_gizmo = 0;
	}

	vec3 gizmo_position      = cursor->gizmo_position;
	vec3 gizmo_position_last = gizmo_position;

	// update gizmo
	if(!game_editor->model.individual_sprite_editing && selected_meshes_count)
	{
	    //update gizmo interaction
	    //interact with the gizmo's center as a cube
	    ray_cube_result ray_and_gizmo_position_result = ray_cube_get_result(
				ray_origin,
				ray_dir,
				gizmo_position,
				vec3_all(gizmo_cube_size));


	    //hit cube
	    if(ray_and_gizmo_position_result.t_min && !editor_input->shift_l)
	    {
	    	model_sprite_hit_distance = ray_and_gizmo_position_result.t_min;
	    	gizmo_hot = 1;
	    }

	    //holding shift or interacted with gizmo
		//the gizmo counts as moved when holding shift or just by clicking it
	    if((gizmo_hot && mouse_l_pressed))
	    {
	    	started_selection_move = !game_editor->model.moving_gizmo;

	        game_editor->model.moving_gizmo = 1;
	    }
	    //render gizmo
	    editor_render_gizmo(commands,
				cursor->gizmo_position,
				gizmo_hot || game_editor->model.moving_gizmo,
				in_camera_mode);
	}


	//set up and right cursor axes depending on the camera
     editor_cursor_set_axes_by_camera(game_renderer, &game_editor->model.cursor);
	 //record cursor movement for sprites or bones
	 vec3 cursor_position_last = cursor_position;
	 //move cursor with keys
	 if(cursor_render_and_update && !ui_focused && !in_camera_mode)
	 {

        draw_crosshair_cursor(commands,
        		              cursor_position,
	    				      cursor_alpha);

         model_cursor_moved = editor_cursor_input_movement_by_camera(game_renderer,
				                                               editor_input,
															   &game_editor->model.cursor,
															   dt);
		 cursor_position = game_editor->model.cursor.position;
	 }

	 

	 b32 cast_ray_at_sprites =
		 (model_editor->tool == model_mode_selection ||
			 model_editor->tool == model_mode_sprite_editing ||
			 model_editor->tool == model_mode_paint ||
			 (model_editor->tool == model_mode_nodes && model_editor->link_sprites_to_bone));
    //real32 worldRayDir = vec3_inner(ray_dir, worldPlaneN);


	 //first process the selected sprites, then the unselected ones 
	 if(cast_ray_at_sprites)
	 {
		 for(u32 t = 0; t < selected_meshes_count; t++)
		 {
			 editor_mesh_selection *selection_data = selected_meshes_array + t;

			 u32 mesh_index = selection_data->index;
			 model_sprite *current_sprite = model_sprite_array + mesh_index;

			 editor_mouse_mesh_selection mesh_ray_selection_data = 
				 editor_cursor_update_selected_mesh(ray_origin,
						 ray_dir, 
						 current_sprite->v0,
						 current_sprite->v1,
						 current_sprite->v2,
						 current_sprite->v3,
						 t,
						 model_sprite_hit_distance,
						 &game_editor->model.cursor_memory,
						 game_editor->settings.vertices_cube_size);

			 if(mesh_ray_selection_data.hit && 
					 mesh_ray_selection_data.hit_distance < model_sprite_hit_distance)
			 {
				 //set this as the closest hit distance by the ray
				 model_sprite_hit_distance = mesh_ray_selection_data.hit_distance;
				 sprite_hovering_edge = mesh_ray_selection_data.hovering_edge;

				 sprite_hit = 1;
				 sprite_hit_index = mesh_index;
				 sprite_selection_hit_index = t;
				 sprite_hit_is_selected = 1;
			 }


		 } //selected tiles array
		   //cast ray against sprites (selected and unselected)
		 for(u32 s = 0;
				 s < sprite_count;
				 s++)
		 {
			 ray_hit_result hit_result = editor_model_ray_hits_sprite(
					 game_editor,
					 game_renderer,
					 ray_origin,
					 ray_dir,
					 s,
					 model_sprite_hit_distance);
			 //closest sprite hit
			 if(hit_result.hit && hit_result.distance < model_sprite_hit_distance)
			 {
				 model_sprite_hit_distance = hit_result.distance;
				 sprite_hit = 1;
				 sprite_hit_index = s;
				 sprite_hit_is_selected = 0;
			 }

		 }
	 }
	   

	 //"paint" sprites or meshes
	 if(game_editor->model.tool == model_mode_paint &&
		game_editor->model.bone_count &&
		game_editor->model.sprite_sheets_count)
	 {
		 u32 fl_index = model_editor->selected_frame_list_paint_mode;
		 editor_model_frame_list *frame_list = model_editor->frame_list_is_selected_paint_mode ? 
			 model_editor->frame_lists + fl_index : 0; 
		 render_texture *selected_texture = 0;
		 if(frame_list && frame_list->sprite_index < model_editor->sprite_sheets_count)
		 {
			 selected_texture = editor_model_get_texture(model_editor, frame_list->sprite_index);
		 }
		 if(selected_texture)
		 {
			 sprite_orientation *frame_list_uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
			 vec3 v0 = {0};
			 vec3 v1 = {0};
			 vec3 v2 = {0};
			 vec3 v3 = {0};
			 vec2 uv0 = frame_list_uvs->uv0;
			 vec2 uv1 = frame_list_uvs->uv1;
			 vec2 uv2 = frame_list_uvs->uv2;
			 vec2 uv3 = frame_list_uvs->uv3;

			 vec2 uv_min = uv1;
			 vec2 uv_max = uv3;

			 //used to adjust vertices to frame's uvs
			 u16 frame_w = (u16)(ABS(uv_max.x - uv_min.x) * selected_texture->width);
			 u16 frame_h = (u16)(ABS(uv_max.y - uv_min.y) * selected_texture->height);

			 editor_cursor_set_mesh_axes_by_orientation(&game_editor->model.cursor);
			 vec3 next_sprite_position = editor_cursor_translate_ray_to_tile_wh(&game_editor->model.cursor,
					 ray_origin,
					 ray_dir,
					 1,
					 1);
			 //model_mesh_axes mesh_axes = rotate_axes_by_orientation();

			 vec3 model_uAxis = game_editor->model.cursor.mesh_uAxis;
			 vec3 model_rAxis = game_editor->model.cursor.mesh_rAxis;
			 vec3 model_uAxis_scaled = vec3_scale(model_uAxis, (f32)frame_h);
			 vec3 model_rAxis_scaled = vec3_scale(model_rAxis, (f32)frame_w);
			 ////Future tile vertices
			 v0 = next_sprite_position;
			 v1 = vec3_add(v0, model_uAxis_scaled);
			 v2 = vec3_add(v1, model_rAxis_scaled);
			 v3 = vec3_add(v0, model_rAxis_scaled);



			 u32 next_sprite_index = 0;
				 u32 can_add_sprite = game_editor->model.selected_cursor_texture_index < game_editor->model.sprite_sheets_count && 
				 (frame_w > 0) && (frame_h > 0);
			 u32 mesh_click_mode = 0;
			 u32 show_preview = 0;

			 u32 replacing_sprite = 0;


			 model_sprite *sprite_to_add = 0;


			 if(editor_process_input)
			 {
				 if(can_add_sprite)
				 {
					 if(game_editor->model.cursor_repaint_mode)
					 {
						 if(sprite_hit)
						 {
							 show_preview = 1;
							 mesh_click_mode = mouse_l_pressed;

							 model_sprite *current_sprite = model_sprite_array + sprite_hit_index;


							 mesh_points oriented_points = quad_rotate_vertices_by_orientation(cursor->mesh_orientation,
									 current_sprite->v0,
									 current_sprite->v1,
									 current_sprite->v2,
									 current_sprite->v3);
							 v0 = oriented_points.v0;
							 v1 = oriented_points.v1;
							 v2 = oriented_points.v2;
							 v3 = oriented_points.v3;

							 //set the next sprite to repaint
							 sprite_to_add = current_sprite;
						 }
					 }
					 else
					 {
						 show_preview = 1;
						 mesh_click_mode = mouse_l_pressed;

						 //returns true if overlaps a sprite
						 if(mesh_click_mode)
						 {
							 if(editor_model_get_overlapping_sprite(game_editor,
										 v0,
										 v1,
										 v2,
										 v3,
										 &next_sprite_index))
							 {
								 sprite_to_add = model_sprite_array + next_sprite_index;
								 replacing_sprite = 1;
							 }
							 else
							 {
								 sprite_to_add = editor_model_add_sprite(game_editor);

							 }
						 }

						 //record
					 }
				 }
				 if(mouse_r_pressed && sprite_hit)
				 {
					 editor_model_remove_sprite(game_editor, sprite_hit_index);
				 }
				 if(show_preview)
				 {
					 //draw next tile preview
					 render_push_quad(commands,
							 selected_texture,
							 v0,
							 v1,
							 v2,
							 v3,
							 uv0,
							 uv1,
							 uv2,
							 uv3,
							 vec4_all(255));
				 }

				 if(model_editor->selected_bone_index < model_editor->bone_count)
				 {
					// if(mesh_click_mode)
					// {
					//	 model_bone *bone = model_editor->bones + model_editor->selected_bone_index;

					//	 sprite_to_add->bone_index = model_editor->selected_bone_index;
					//	 sprite_to_add->texture_index = game_editor->model.selected_cursor_texture_index;

					//	 sprite_to_add->type = model_sprite_mesh;
					//	 sprite_to_add->v0 = vec3_sub(v0, bone->p);
					//	 sprite_to_add->v1 = vec3_sub(v1, bone->p);
					//	 sprite_to_add->v2 = vec3_sub(v2, bone->p);
					//	 sprite_to_add->v3 = vec3_sub(v3, bone->p);

					//	 sprite_to_add->uv0 = uv0;
					//	 sprite_to_add->uv1 = uv1;
					//	 sprite_to_add->uv2 = uv2;
					//	 sprite_to_add->uv3 = uv3;
					//	 sprite_to_add->frame_list_index = fl_index;
					// } 
				 }
			 }
		 }
	 }
	 if(game_editor->model.tool == model_mode_selection)
	 {
		 if(sprite_hit)
		 {
			 model_sprite *current_mesh = model_sprite_array + sprite_hit_index;
			 vec3 v0 = current_mesh->v0;
			 vec3 v1 = current_mesh->v1;
			 vec3 v2 = current_mesh->v2;
			 vec3 v3 = current_mesh->v3;

			 if(mouse_l_pressed &&
					 editor_process_input)
			 {
#if 1
				 //hit sprite is not selected
				 if(!sprite_hit_is_selected)
				 {

					 //Add to selected tiles array
					 u32 selected_tiles_count = game_editor->model.cursor_memory.selected_meshes_count;

					 editor_cursor_memory_add_selection(
							 &game_editor->model.cursor_memory,
							 sprite_hit_index);
					 //if no tile is selected, put the gizmo on the mesh's center
					 //if(!selected_tiles_count)
					 //{
					 //    gizmo_position = vertices_get_mid_point(v0, v1, v2, v3);
					 //}
				 }
				 else
				 {
					 u32 selectionDataIndex = sprite_selection_hit_index;
					 if(editor_input->ctrl_l)
					 {
						 vec3 final_position = gizmo_position;

						 if(sprite_hovering_edge & tile_edge_v0)
						 {
							 final_position = v0;
						 }
						 else if(sprite_hovering_edge & tile_edge_v1)
						 {
							 final_position = v1;
						 }
						 else if(sprite_hovering_edge & tile_edge_v2)
						 {
							 final_position = v2;
						 }
						 else if(sprite_hovering_edge & tile_edge_v3)
						 {
							 final_position = v3;
						 }
						 else if(sprite_hovering_edge & tile_edge_u)
						 {
							 final_position = vec3_add(v1, vec3_scale(vec3_sub(v2, v1), 0.5f));
						 }
						 else if(sprite_hovering_edge & tile_edge_d)
						 {
							 final_position = vec3_add(v0, vec3_scale(vec3_sub(v3, v0), 0.5f));
						 }
						 else if(sprite_hovering_edge & tile_edge_l)
						 {
							 final_position = vec3_add(v0, vec3_scale(vec3_sub(v1, v0), 0.5f));
						 }
						 else if(sprite_hovering_edge & tile_edge_r)
						 {
							 final_position = vec3_add(v3, vec3_scale(vec3_sub(v2, v3), 0.5f));
						 }
						 else
						 {
							 final_position = vertices_get_mid_point(v0, v1, v2, v3);
						 }
						 gizmo_position = final_position;
					 }
					 //move the selected vertices to another clicked one
					 else if(editor_input->shift_l && cursor->vertex_to_move_with_gizmo <  cursor_memory->selected_meshes_count)
					 {
						 //look for the closest hot vertex
						 editor_hot_vertices_info *hv = cursor_memory->hot_vertices;
						 u32 closest_vertex_index     = cursor_memory->hot_vertices_count;
						 f32 h_distance_to_camera = 1e6;
						 for(u32 h = 0;
								 h < cursor_memory->hot_vertices_count;
								 h++)
						 {
							 editor_hot_vertices_info *current_h = cursor_memory->hot_vertices + h;
							 if(current_h->mesh_index != cursor->vertex_to_move_with_gizmo)
							 {
								 if(current_h->distance_to_ray < h_distance_to_camera)
								 {
									 h_distance_to_camera = current_h->distance_to_ray;
									 closest_vertex_index = h;
								 }

							 }
						 }
						 if(closest_vertex_index < cursor_memory->hot_vertices_count)
						 {
							 editor_hot_vertices_info *closest_h = cursor_memory->hot_vertices + closest_vertex_index;
							 u32 h_sv = closest_h->selection_value;

							 vertices *closest_hot_mesh = (vertices *)&model_sprite_array[closest_h->mesh_index].v0;
							 vec3 v0 = closest_hot_mesh->v0;
							 vec3 v1 = closest_hot_mesh->v1;
							 vec3 v2 = closest_hot_mesh->v2;
							 vec3 v3 = closest_hot_mesh->v3;

							 vec3 final_position = gizmo_position;

							 if(h_sv & tile_edge_v0)
							 {
								 final_position = v0;
							 }
							 else if(h_sv & tile_edge_v1)
							 {
								 final_position = v1;
							 }
							 else if(h_sv & tile_edge_v2)
							 {
								 final_position = v2;
							 }
							 else if(h_sv & tile_edge_v3)
							 {
								 final_position = v3;
							 }
							 else if(h_sv & tile_edge_u)
							 {
								 final_position = vec3_add(v1, vec3_scale(vec3_sub(v2, v1), 0.5f));
							 }
							 else if(h_sv & tile_edge_d)
							 {
								 final_position = vec3_add(v0, vec3_scale(vec3_sub(v3, v0), 0.5f));
							 }
							 else if(h_sv & tile_edge_l)
							 {
								 final_position = vec3_add(v0, vec3_scale(vec3_sub(v1, v0), 0.5f));
							 }
							 else if(h_sv & tile_edge_r)
							 {
								 final_position = vec3_add(v3, vec3_scale(vec3_sub(v2, v3), 0.5f));
							 }
							 else
							 {
								 final_position = vertices_get_mid_point(v0, v1, v2, v3);
							 }
							 gizmo_position = final_position;

							 //read the mesh's selection data
							 editor_mesh_selection *mesh_to_move_selection = cursor_memory->selected_meshes + cursor->vertex_to_move_with_gizmo;
							 vec3 gizmo_delta = vec3_sub(gizmo_position, gizmo_position_last);

							 //selected vertices to move
							 vertices *m_vertices = (vertices *)&game_editor->model.sprites[mesh_to_move_selection->index];
							 //move selected vertices by delta
							 if(mesh_to_move_selection->v0_selected)
							 {
								 m_vertices->v0 = vec3_add(m_vertices->v0, gizmo_delta);
							 }
							 if(mesh_to_move_selection->v1_selected)
							 {
								 m_vertices->v1 = vec3_add(m_vertices->v1, gizmo_delta);
							 }
							 if(mesh_to_move_selection->v2_selected)
							 {
								 m_vertices->v2 = vec3_add(m_vertices->v2, gizmo_delta);
							 }
							 if(mesh_to_move_selection->v3_selected)
							 {
								 m_vertices->v3 = vec3_add(m_vertices->v3, gizmo_delta);
							 }

						 }
					 }
					 else
					 {
						 //sprite is already selected,
						 //so update selection data.
						 editor_mesh_selection *selection_data = selected_meshes_array + selectionDataIndex;
						 if(sprite_hovering_edge == tile_edge_v0)
						 {
							 selection_data->v0_selected = !selection_data->v0_selected;
						 }
						 else if(sprite_hovering_edge == tile_edge_v1)
						 {
							 selection_data->v1_selected = !selection_data->v1_selected;
						 }
						 else if(sprite_hovering_edge == tile_edge_v2)
						 {
							 selection_data->v2_selected = !selection_data->v2_selected;
						 }
						 else if(sprite_hovering_edge == tile_edge_v3)
						 {
							 selection_data->v3_selected = !selection_data->v3_selected;
						 }
						 else //clicked on the same tile, so deselect
						 {
							 editor_cursor_memory_deselect(cursor_memory, selectionDataIndex);
						 }
					 }
				 }

				 //position the gizmo point
				 if(editor_input->ctrl_l)
				 {
					 game_editor->model.cursor.vertex_to_move_with_gizmo = sprite_selection_hit_index;
				 }
				 cursor->gizmo_position = gizmo_position;
#endif
			 }
		 }

	 }// if tool is selection



	 //highlight sprite hit by ray if not editing nodes
	 if(cast_ray_at_sprites && sprite_hit)
	 {
		 model_sprite *current_mesh = model_sprite_array + sprite_hit_index;

		 vec3 v0 = {0}; 
		 vec3 v1 = {0}; 
		 vec3 v2 = {0}; 
		 vec3 v3 = {0}; 
		 vec4 lineColor = {0xff, 0xff, 0x00, 0xff};
		 editor_model_fill_sprite_vertices(
				 game_editor,
				 game_renderer,
				 sprite_hit_index,
				 &v0,
				 &v1,
				 &v2,
				 &v3
				 );

		 real32 lineSz = 0.2f;
		 render_vertices_edges(commands,
				 v0,
				 v1,
				 v2,
				 v3,
				 lineColor, lineSz);
	 }

	       //highlight individual selected mesh
		   if(model_editor->tool == model_mode_sprite_editing)
		   {
			   if(model_editor->sprite_is_selected)
			   {
				   model_sprite *current_mesh = game_editor->model.sprites + 
					   game_editor->model.selected_sprite_index;

				   vec4 selected_mesh_color   = {0xff, 0x00, 0x00, 60};
				   vec4 borders_color         = {0xff, 0xff, 0xff, 0xff};

				   vec3 v0 = {0}; 
				   vec3 v1 = {0}; 
				   vec3 v2 = {0}; 
				   vec3 v3 = {0}; 
				   editor_model_fill_sprite_vertices(
						   game_editor,
						   game_renderer,
						   game_editor->model.selected_sprite_index,
						   &v0,
						   &v1,
						   &v2,
						   &v3
						   );


				   u32 hide_highlight = in_camera_mode || game_editor->model.moving_gizmo;
				   if(hide_highlight)
				   {
					   selected_mesh_color.w = 20;
					   borders_color.w       = 20;
				   }

				   render_vertices_colored(commands,
						   v0,
						   v1,
						   v2,
						   v3,
						   selected_mesh_color);
				   render_vertices_edges(commands,
						   v0,
						   v1,
						   v2,
						   v3,
						   borders_color,
						   0.2f);
			   }

			   //select the hit sprite if clicked
			   if(sprite_hit && mouse_l_pressed && editor_process_input)
			   {
				   game_editor->model.selected_sprite_index = sprite_hit_index;
				   game_editor->model.sprite_is_selected = 1;
			   }

		   }
		   else if(model_editor->tool == model_mode_selection)
		   {
			   //highlight selected meshes
	           u32 t = 0;
			   f32 vertices_cube_size =  game_editor->settings.vertices_cube_size;
			   f32 border_line_thickness = 0.1f;
	           while(t < selected_meshes_count) 
	           {
	               editor_mesh_selection *mesh_selection = game_editor->model.cursor_memory.selected_meshes + t;
	           	   u32 mesh_index = mesh_selection->index;
                   model_sprite *current_mesh = game_editor->model.sprites + mesh_index;

	               u32 hide = in_camera_mode || game_editor->model.moving_gizmo;
				   vec3 v0 = {0};
				   vec3 v1 = {0};
				   vec3 v2 = {0};
				   vec3 v3 = {0};
				   editor_model_fill_sprite_vertices(
						   game_editor,
						   game_renderer,
						   mesh_index,
						   &v0,
						   &v1,
						   &v2,
						   &v3
						   );

                //   editor_highlight_selected_mesh(commands,
	            //								  mesh_selection,
	            //								  current_mesh->v0,
	            //								  current_mesh->v1,
	            //								  current_mesh->v2,
	            //								  current_mesh->v3,
	            //								  hide,
				//								  game_editor->settings.vertices_cube_size);

				   vec4 selected_meshesColor = {0xff, 0x00, 0x00, 60};
				   vec4 verticesColor = {0xff, 0xff, 0xff, 0xff};
				   vec4 verticesSelectedColor = {0x00, 0x00, 0xff, 0xff};
				   vec4 verticesHotColor = {0xff, 0x00, 0x00, 0xff};
				   vec4 nextLineColor = {0};
				   //For more visibility 
				   if(hide)
				   {
					   verticesColor.w         = 20;
					   verticesSelectedColor.w = 20;
					   verticesHotColor.w      = 20;
					   selected_meshesColor.w    = 20;
				   }

				   render_vertices_colored(commands,
						   v0,
						   v1,
						   v2,
						   v3,
						   selected_meshesColor);

				   render_draw_line_up(commands,
						   v0,
						   v3,
						   verticesColor,
						   border_line_thickness);

				   render_draw_line_up(commands,
						   v0,
						   v1,
						   verticesColor,
						   border_line_thickness);

				   render_draw_line_up(commands,
						   v1,
						   v2,
						   verticesColor,
						   border_line_thickness);

				   render_draw_line_up(commands,
						   v3,
						   v2,
						   verticesColor,
						   border_line_thickness);

				   vec4 nextVertexColor = mesh_selection->v0_selected ?
					   verticesSelectedColor : 
					   verticesColor;
				   render_draw_cube(commands, v0, vec3_all(vertices_cube_size), nextVertexColor);

				   nextVertexColor = mesh_selection->v1_selected ? verticesSelectedColor : verticesColor;
				   render_draw_cube(commands, v1, vec3_all(vertices_cube_size), nextVertexColor);

				   nextVertexColor = mesh_selection->v2_selected ? verticesSelectedColor : verticesColor;
				   render_draw_cube(commands, v2, vec3_all(vertices_cube_size), nextVertexColor);

				   nextVertexColor = mesh_selection->v3_selected ? verticesSelectedColor : verticesColor;
				   render_draw_cube(commands, v3, vec3_all(vertices_cube_size), nextVertexColor);
	              t++;
	           }
		   }
	   	   
	   

	   //update model tab hotkeys
	   if(!input_text_focused)
	   {

		   u32 ctrl_hotkeys  = editor_input->ctrl_l;
		   u32 shift_hotkeys = editor_input->shift_l;
		   u32 alt_hotkeys   = editor_input->alt;

		   if(game_editor->graphics_tab == graphics_tab_model)
		   {

			   //model editor hotkeys
			   // for ctrl +
			   //  a: Select all
			   //  c: Copy
			   //  x: Cut
			   //  v: paste
			   //  space: deselect all
			   //

			   //  shift +
			   //   x: flip quad diagonals
			   //   h: flip uv horizontally
			   //   r: apply gizmo rotation

			   // alt +
			   //  left click: select vertices
			   //  right click: deselect vertices
			   //  f: select faces

			   //global hotkeys
			   if(!ctrl_hotkeys && !shift_hotkeys && !alt_hotkeys)
			   {
				   if(input_down(editor_input->esc))
				   {
					   model_editor->tool = model_mode_view;
				   }
				   if(input_down(editor_input->v))
				   {
					   model_editor->tool = model_mode_selection;
				   }
				   if(input_down(editor_input->b))
				   {
					   model_editor->tool = model_mode_paint;
				   }
				   if(editor_input->number_keys[1])
				   {
					   model_editor->tool = model_mode_nodes;
				   }
				   if(editor_input->number_keys[2])
				   {
					   model_editor->tool = model_mode_sprite_editing;
				   }
				   if(input_down(editor_input->x))
				   {
					   model_editor->tool = model_mode_properties;
				   }
				   if(editor_input->number_keys[3])
				   {
					   model_editor->tool = model_mode_animation;
				   }
			   }
			   else if(ctrl_hotkeys)
			   {
				   b8 reset_camera_x = input_down(editor_input->r) && editor_input->spaceBarDown;
				   if(reset_camera_x)
				   {
					   model_editor->camera_rotation_x = 0.2f;
					   model_editor->cameraDistance = 300;
					   
				   }
			   }



			   if(model_editor->tool == model_mode_selection)
			   {
				   //ctrl hotkeys
				   if(ctrl_hotkeys)
				   {
					   u32 copy         = input_pressed(editor_input->c);
					   u32 cut          = input_pressed(editor_input->x);
					   u32 paste        = input_pressed(editor_input->v);
					   u32 deselect_all = editor_input->spaceBarDown;
					   u32 select_all   = input_pressed(editor_input->a);
					   //hotkeys for the model sprites
						   if(copy)
						   {
							   editor_model_copy_selected_sprites(game_editor);
						   }
						   else if(cut)
						   {
							   editor_model_cut_selected_sprites(game_editor);
						   }
						   else if(paste)
						   {
							   editor_model_paste_selected_sprites(game_editor);
						   }
						   else if(deselect_all)
						   {
							   game_editor->model.cursor_memory.selected_meshes_count = 0;
						   }
						   else if(select_all)
						   {
							   editor_cursor_memory_select_amount(cursor_memory, sprite_count);
						   }
						   if(input_pressed(editor_input->e) && sprite_hit)
						   {
							   model_sprite *current_hit_sprite = model_sprite_array + sprite_hit_index;
							   cursor->position = current_hit_sprite->v0;
						   }
				   }
				   else if(shift_hotkeys)
				   {
					   u32 flip_selected_diagonals = input_pressed(editor_input->x);
					   u32 flip_uv_horizontal      = input_pressed(editor_input->h);
					   u32 apply_gizmo_rotation    = input_pressed(editor_input->r);

					   if(apply_gizmo_rotation)
					   {
						   game_editor->model.apply_gizmo_rotation = 1;
					   }
					   else if(flip_selected_diagonals)
					   {
						   u32 t = 0;
						   while(t < selected_meshes_count)
						   {
							   // ;flip the diagonals of the billboards?
							   u32 sprite_index     = selected_meshes_array[t].index;
							   model_sprite *sprite = model_sprite_array + sprite_index; 
							   model_mesh flipped_quad = quad_flip_diagonal(sprite->v0,
									   sprite->v1,
									   sprite->v2,
									   sprite->v3,
									   sprite->uv0,
									   sprite->uv1,
									   sprite->uv2,
									   sprite->uv3);
							   sprite->v0 = flipped_quad.v0;
							   sprite->v1 = flipped_quad.v1;
							   sprite->v2 = flipped_quad.v2;
							   sprite->v3 = flipped_quad.v3;

							   sprite->uv0 = flipped_quad.uv0;
							   sprite->uv1 = flipped_quad.uv1;
							   sprite->uv2 = flipped_quad.uv2;
							   sprite->uv3 = flipped_quad.uv3;
							   t++;

						   }
					   }
					   else if(flip_uv_horizontal)
					   {      
						   u32 t = 0;
						   while(t < selected_meshes_count)
						   {
							   // ;flip the diagonals of the billboards?
							   u32 sprite_index     = selected_meshes_array[t].index;
							   model_sprite *sprite = model_sprite_array + sprite_index; 

#if 0
							   vec2 uv0_copy = sprite->uv0;
							   vec2 uv2_copy = sprite->uv2;
							   sprite->uv0 = sprite->uv1; 
							   sprite->uv1 = uv0_copy; 
							   sprite->uv2 = sprite->uv3;
							   sprite->uv3 = uv2_copy;
#else
							   vec2 uv0_copy = sprite->uv0;
							   vec2 uv1_copy = sprite->uv1;
							   sprite->uv0 = sprite->uv3; 
							   sprite->uv1 = sprite->uv2; 
							   sprite->uv2 = uv1_copy;
							   sprite->uv3 = uv0_copy;
#endif
							   t++;
						   }

					   }

				   }//<-- shift hotkeys
				   else if(alt_hotkeys)
				   {

					   if(game_editor->model.tool == model_mode_selection)
					   {
						   //in deselect mode
						   if(mouse_r_down)
						   {
							   editor_cursor_deselect_hot_vertices(cursor_memory);
						   }
						   else if(mouse_l_down) //select mode
						   {
							   editor_cursor_select_hot_vertices(cursor_memory);
						   }
						   else if(input_down(editor_input->r))
						   {
							   editor_model_select_sprites_against_ray(
									   game_editor,
									   game_renderer,
									   ray_origin,
									   ray_dir);
						   }
					   }
				   }
				   else
				   {
				   }

			   }
			   else if(model_editor->tool == model_mode_animation)
			   {
				   if(ctrl_hotkeys)
				   {
					   b32 copy         = input_pressed(editor_input->c);
					   b32 cut          = input_pressed(editor_input->x);
					   b32 paste        = input_pressed(editor_input->v);
					   b32 deselect_all = editor_input->spaceBarDown;
					   b32 select_all   = input_pressed(editor_input->a);
					   //hotkeys for the model sprites
					   //hotkeys for the individual sprite editing mode
					   if(copy)
					   {
						   if(model_editor->ui_focus ==  ea_focus_timeline)
						   {
							   //copy keyframes
							   editor_animation_copy_at_current_frame(
									   game_editor);
						   }
						   else if(model_editor->ui_focus ==  ea_focus_animation_list)
						   {
							   //Implement: copy selected animation
						   }
					   }
					   else if(cut)
					   {
					   }
					   else if(paste && editor_animation)
					   {
						   editor_animation_paste_at_current_frame(
								   game_editor, editor_animation);
					   }
				   }
				   else
				   {
					   b32 delete = input_pressed(editor_input->del);
					   //delete and an individual keyframe is focused
					   if(delete)
					   {
						   if(model_editor->focused_on_keyframe_only)
						   {
							   editor_animation_remove_keyframe(
									   game_editor,
									   editor_animation,
									   model_editor->selected_keyframe);
						   }
						   else if(model_editor->selected_column)
						   {
							   editor_animation_free_column(
									   game_editor,
									   editor_animation,
									   model_editor->selected_column);

						   }
					   }
				   }
			   }
			   else if(model_editor->tool == model_mode_paint)
			   {
				   //UNUSED?
				   if(input_pressed(editor_input->e))
				   {
					   cursor->mesh_orientation++;
					   cursor->mesh_orientation %= 4;
				   }
				   if(input_pressed(editor_input->q))
				   {
					   cursor->mesh_orientation--;
					   cursor->mesh_orientation %= 4;
				   }

				   if(input_pressed(editor_input->h))
				   {
					   //game_editor->model.next_tile_uv_flip_x = !game_editor->world.next_tile_uv_flip_x;
				   }
			   }
			   else if(model_editor->tool == model_mode_properties)
			   {
				   if(ctrl_hotkeys)
				   {
					   b8 copy = input_pressed(editor_input->c);
					   b8 paste = input_pressed(editor_input->v);

					   if(model_editor->properties_ui_focus == properties_focus_frame_lists)
					   {
						   if(copy && model_editor->frame_list_is_selected)
						   {
							   editor_model_copy_frame_list(
									   game_editor,
									   model_editor->frame_lists + model_editor->selected_frame_list_index);
						   }
						   else if(paste)
						   {
							   editor_model_paste_frame_lists(
									   game_editor);
						   }
					   }
				   }
				   else
				   {
				   }
			   }
			   //move rotation_point
			   if(game_editor->model.moving_gizmo && !editor_input->shift_l)
			   {
				   vec3 move_delta_position = editor_cursor_move_at_axes(&game_editor->model.cursor,
						   ray_origin,
						   ray_dir,
						   gizmo_position);
				   gizmo_position = vec3_add(move_delta_position, gizmo_position);
				   cursor->gizmo_position = gizmo_position;
			   }
			   if(model_editor->tool == model_mode_selection)
			   {
				   u32 t = 0;
				   while(!individual_sprite_editing && t < selected_meshes_count)
				   {
					   editor_mesh_selection *mesh_selection = selected_meshes_array + t;
					   u32 mesh_index             = mesh_selection->index;
					   model_sprite *current_mesh = model_sprite_array + mesh_index;

					   if(game_editor->model.apply_gizmo_rotation)
					   {
						   i32 rotation_x = game_editor->model.gizmo_rotation_x;
						   i32 rotation_y = game_editor->model.gizmo_rotation_y;
						   i32 rotation_z = game_editor->model.gizmo_rotation_z;

						   mesh_points rotated_vertices = quad_rotate_from_degrees(gizmo_position,
								   rotation_x,
								   rotation_y,
								   rotation_z,
								   current_mesh->v0,
								   current_mesh->v1,
								   current_mesh->v2,
								   current_mesh->v3);
						   current_mesh->v0 = rotated_vertices.v0;
						   current_mesh->v1 = rotated_vertices.v1;
						   current_mesh->v2 = rotated_vertices.v2;
						   current_mesh->v3 = rotated_vertices.v3;
					   }
					   //apply movement if holding click

					   //move rotation point and selected vertices

					   u32 move_with_selections = !editor_input->shift_l || (editor_input->shift_l && sprite_hit_index != mesh_index);
					   if(game_editor->model.moving_gizmo && move_with_selections)
					   {
						   if(mesh_selection->v0_selected)
						   {
							   vec3 distance_gizmo_position_and_v0 = vec3_sub(gizmo_position_last, current_mesh->v0);

							   current_mesh->v0 = vec3_sub(gizmo_position, distance_gizmo_position_and_v0);
						   }
						   if(mesh_selection->v1_selected)
						   {
							   vec3 distance_gizmo_position_and_v1 = vec3_sub(gizmo_position_last, current_mesh->v1);

							   current_mesh->v1 = vec3_sub(gizmo_position, distance_gizmo_position_and_v1);
						   }
						   if(mesh_selection->v2_selected)
						   {
							   vec3 distance_gizmo_position_and_v2 = vec3_sub(gizmo_position_last, current_mesh->v2);

							   current_mesh->v2 = vec3_sub(gizmo_position, distance_gizmo_position_and_v2);
						   }
						   if(mesh_selection->v3_selected)
						   {
							   vec3 distance_gizmo_position_and_v3 = vec3_sub(gizmo_position_last, current_mesh->v3);

							   current_mesh->v3 = vec3_sub(gizmo_position, distance_gizmo_position_and_v3);
						   }
					   }
					   //move selected meshes with cursor
					   if(model_cursor_moved)
					   {
						   vec3 cursor_delta = vec3_sub(cursor_position, cursor_position_last);
						   //cursor_delta      = vec3_round_to_int(cursor_delta);
						   u32 t = 0;


						   if(mesh_selection->v0_selected)
						   {
							   current_mesh->v0 = vec3_add(current_mesh->v0 , cursor_delta);
						   }
						   if(mesh_selection->v1_selected)
						   {
							   current_mesh->v1 = vec3_add(current_mesh->v1 , cursor_delta);
						   }
						   if(mesh_selection->v2_selected)
						   {
							   current_mesh->v2 = vec3_add(current_mesh->v2 , cursor_delta);
						   }
						   if(mesh_selection->v3_selected)
						   {
							   current_mesh->v3 = vec3_add(current_mesh->v3 , cursor_delta);
						   }

						   //if(game_editor->world.cursor_memory.selected_meshes_count)
						   //{
						   //    editor_world_record_moved_selected(game_editor,
						   //    		                           cursor_delta);
						   //}
					   }

					   t++;
				   }//<-- selected meshes loop
			   }
			   game_editor->model.apply_gizmo_rotation = 0;
		   }// <- model hotkeys

	   }

	render_commands_end(commands);


	//depth test commands
	//render model and scenario
	commands = render_commands_begin_default(game_renderer); 

	if(game_editor->model.display_edges)
	{
		for(u32 e = 0;
				e < game_editor->model.sprite_count;
				e++)
		{
			model_sprite *current_sprite = game_editor->model.sprites + e;

			vec3 v0 = current_sprite->v0;
			vec3 v1 = current_sprite->v1;
			vec3 v2 = current_sprite->v2;
			vec3 v3 = current_sprite->v3;

			vec3 z_bias = vec3_scale(game_renderer->camera_z, 0);

			v0 = vec3_add(v0, z_bias);
			v1 = vec3_add(v1, z_bias);
			v2 = vec3_add(v2, z_bias);
			v3 = vec3_add(v3, z_bias);
			vec4 faces_color = { 0xff, 0xff, 0xff, 0xff};
			f32 edges_thickness = 0.1f;

			render_draw_line_up(commands, v0, v1, faces_color, edges_thickness);
			render_draw_line_up(commands, v0, v3, faces_color, edges_thickness);
			render_draw_line_up(commands, v1, v2, faces_color, edges_thickness);
			render_draw_line_up(commands, v2, v3, faces_color, edges_thickness);
			render_draw_line_up(commands, v0, v2, faces_color, edges_thickness);
		}
	}
	//editor background
	vec4 backRectangleColor = {20, 20, 20, 255};
	render_draw_rectangle_colored_axes_sized(commands,
			                     V3(0, 0, 0),
								 V3(1024, 0, 0),
								 V3(0, 1024, 0),
								 backRectangleColor);
	f32 model_bk_size = 1024.0f;
	vec3 background_edge = {
	0 - 1024 * 0.5f, 0 - 1024 * 0.5f, 0};

	f32 line_x = background_edge.x;
	f32 line_y = background_edge.y;
	for(u32 y = 0; y  < 32; y++)
	{
		render_draw_line(
				commands,
				V3(line_x, line_y, 0.5f),
				V3(line_x + model_bk_size, line_y , 0.3f),
				V3(0, 0, 1.0f),
				V4(120, 120, 120, 120),
				0.5f,
				1);
		line_y += 32;
	}
	line_x = background_edge.x;
	line_y = background_edge.y;
	for(u32 x = 0; x < 32; x++)
	{
		//f32 line_x 
		render_draw_line(
				commands,
				V3(line_x, line_y, 0.5f),
				V3(line_x, line_y + model_bk_size, 0.3f),
				V3(0, 0, 1.0f),
				V4(120, 120, 120, 120),
				0.5f,
				1);
		line_x += 32;
	}
	//red: south
	//yellow: north
	//green: east
	//blue: west
	f32 directional_lines_thickness = 1;
	render_draw_line(
			commands,
			V3(0, 0, 0.5f),
			V3(0, -1024, 0.5f),
			V3(0, 0, 1.0f),
			V4(255, 0, 0, 255),
			directional_lines_thickness,
			1);

	//Maybe move to top or another place?

	//render meshes without transform
	if(model_editor->tool != model_mode_animation)
	{
			model_render(
					commands->gameRenderer,
					commands,
					model_editor->loaded_model,
					model_editor->loaded_model_pose,
					model_world_position,
					game_editor->model.model_foward);
	}
	else
	{
		if(!bone_count)
		{
           model_render_meshes(commands,
				   editor_state->editor_assets,
				   model_editor->loaded_model,
				   model_world_position,
				   game_editor->model.model_foward);
		}
		else
		{
            editor_update_and_render_model_animated(
					game_editor,
					commands,
					editor_state->editor_assets,
					model_world_position,
					dt);

		}
	}

	temporary_area tempArea = temporary_area_begin(&game_editor->area);
	{
		//simulate the model rendering
		model_render_data render_data = {0};
		render_data.animated_pose = model_editor->loaded_model_pose;
		render_data.model = &model_editor->loaded_model;
		render_data.attach_data = memory_area_push_array(&game_editor->area, model_attachment_data, model_editor->attachment_count);
		u32 index = 0;
		for(u32 m = 0; m < model_editor->attachment_count; m++)
		{
			editor_model_attachment *e_attachment = model_editor->attachments + m;
			model_attachment_data *attachment_data = render_data.attach_data + index;
			if(e_attachment->model)
			{
				index++;
				attachment_data->bone_index = e_attachment->base.bone_index;
				attachment_data->model = &model_editor->attachments[m].model->asset_key->model;
				attachment_data->animated_pose = render_allocate_pose(&game_editor->area, *attachment_data->model);

				//set same as model editor for now
				//attachment_data->virtual_nodes = memory_area_push_array(&game_editor->area, model_virtual_node, model_editor->virtual_node_count);
				//TEMP
				attachment_data->virtual_node_count = model_editor->virtual_node_count;
				attachment_data->virtual_nodes = model_editor->virtual_nodes;
				//increase count if avadible
				render_data.attached_models_count++;
				//look for virtual bones who reference this attachment
				for(u32 v = 0; v < model_editor->virtual_node_count; v++)
				{
					model_virtual_node *vnode = model_editor->virtual_nodes + v;
					if(vnode->attached_model == m)
					{
						attachment_data->virtual_nodes[attachment_data->virtual_node_count] = *vnode;
						attachment_data->virtual_node_count++;
					}
				}
			}
			
		}
		//render attached models
		model_render_attached_models(commands, &render_data, 
				model_world_position,
				game_editor->model.model_foward);
	}
	temporary_area_end(&tempArea);


	render_commands_end(commands);
#endif

	//post-non-depth test commands
	commands = render_commands_begin(game_renderer, render_flags_Blending);

	if((model_editor->tool == model_mode_animation ||
		model_editor->tool == model_mode_nodes ||
		model_editor->tool == model_mode_sprite_editing) && !ui_focused)
	{
		//render nodes and linked nodes
//		em_display_and_select_model_nodes(game_editor,
//				commands,
//				ray_origin,
//				ray_dir,
//				bone_ray_hit_distance,
//				model_editor->animation_bone_is_selected,
//				model_editor->animation_selected_bone_index
//				);
		b16 bone_hit_by_ray = 0;
		b16 bone_hit_index = 0;
		b32 bone_selected = 0;
		u32 selected_bone_index = 0;
		if(model_editor->tool == model_mode_animation)
		{
			bone_selected = model_editor->animation_bone_is_selected;
			selected_bone_index = model_editor->animation_selected_bone_index;
		}
		else if(model_editor->tool == model_mode_nodes)
		{
			bone_selected = 1;
			selected_bone_index = model_editor->selected_bone_index;
		}
		else if(model_editor->tool == model_mode_sprite_editing)
		{
			bone_selected = model_editor->sprite_selection_bone.selected;
			selected_bone_index = model_editor->sprite_selection_bone.index;
		}
		// display and select nodes
		for(u32 b = 0;
				b < bone_count;
				b++)
		{
			model_bone *current_bone = model_editor->loaded_model_pose.bones + b;
			vec3 bone_position = current_bone->transformed_p;

			//if in transforms preview mode.
			vec3 node_rotation = {current_bone->rotation_x,
				current_bone->rotation_y,
				current_bone->rotation_z};

			vec3 node_x_axis = {1, 0, 0};
			vec3 node_y_axis = {0, 1, 0};
			vec3 node_z_axis = {0, 0, 1};
			matrix3x3 hierarchy_rotation_matrix = matrix3x3_rotation_scale(
					node_rotation.x, node_rotation.y, node_rotation.z);
			node_x_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 0);
			node_y_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 1);
			node_z_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 2);

			ray_casted_info ray_node_result = ray_circle_upfront_result(ray_origin,
					ray_dir,
					bone_position,
					bone_radius);
			//display
			//bone is closer and got hit
			u32 hit_bone = ray_node_result.hits != 0;
			u32 bone_hot    = 0;
			if(hit_bone && ray_node_result.distance_to_plane < bone_ray_hit_distance)
			{
				bone_hit_by_ray = 1;
				bone_hit_index = b;
				bone_ray_hit_distance = ray_node_result.distance_to_plane;
				bone_hot = 1;
			}

			vec4 node_color = node_normal_color;
			//select or modify node color based on the current state or mode
			if(bone_selected && selected_bone_index == b)
			{
				node_color = node_selected;
				//display rotation axis
				vec3 vx = {1, 0, 0};
				vec3 vy = {0, 1, 0};
				vec3 vz = {0, 0, 1};
				vx = quaternion_v3_mul_foward_inverse(current_bone->transformed_q, vx);
				vy = quaternion_v3_mul_foward_inverse(current_bone->transformed_q, vy);
				vz = quaternion_v3_mul_foward_inverse(current_bone->transformed_q, vz);
				f32 vscale = 10.0f;
				vx = vec3_add(bone_position, vec3_scale(vx, vscale));
				vy = vec3_add(bone_position, vec3_scale(vy, vscale));
				vz = vec3_add(bone_position, vec3_scale(vz, vscale));
				render_draw_line_up(commands,
						bone_position,
						vx,
						V4(255, 255, 255, 255),
						0.08f);
				render_draw_line_up(commands,
						bone_position,
						vy,
						V4(255, 255, 255, 255),
						0.08f);
				render_draw_line_up(commands,
						bone_position,
						vz,
						V4(255, 255, 255, 255),
						0.08f);
			}
			else if(bone_hot)
			{
				node_color.w = 220;
			}
			if(in_camera_mode)
			{
				node_color.w = 40;
			}

			//if this is not the root bone
			if(b != 0)
			{
				model_bone *linked_node = model_editor->loaded_model_pose.bones + current_bone->parent;
				vec3 linked_node_position = linked_node->transformed_p;
				f32 line_alpha = 255;
				if(in_camera_mode)
				{
					line_alpha = 40;
				}

				render_draw_line_up(commands,
						bone_position,
						linked_node_position,
						node_color,
						0.08f);
			}

			//render cube after line
		//	render_draw_cube(commands,
		//			bone_position, 
		//			vec3_all(bone_radius),
		//			node_color);
			render_circle_upfront(commands,
					bone_position, 
					bone_radius,
					.2f,
					node_color);

		}
		//rotate the selected bone



		//select the bone hit by the ray
		if(bone_hit_by_ray && editor_process_input && mouse_l_pressed)
		{
			if(model_editor->tool == model_mode_animation && editor_animation)
			{
				model_editor->animation_selected_bone_index = bone_hit_index;
				model_editor->animation_bone_is_selected = 1;
				selected_bone_index = bone_hit_index;

				//select keyframe if any
				model_editor->selected_timeline_row = selected_bone_index;
				editor_animation_keyframe_row *row = editor_animation_get_row(
						editor_animation, selected_bone_index, 0);
				model_editor->selected_row = row;
				model_editor->row_is_selected = row != 0;
				model_editor->timeline_row_is_selected = 1;

				if(row)
				{
					if(!model_editor->timeline_frame_is_selected)
					{
						model_editor->timeline_frame_is_selected = 1;
						model_editor->timeline_selected_frame = 0;
					}
					editor_animation_keyframe *keyframe = editor_animation_get_keyframe_at_row(
							row, model_editor->timeline_selected_frame);
					model_editor->selected_keyframe = keyframe;
					model_editor->keyframe_is_selected = keyframe != 0;
				}
				else
				{
					model_editor->selected_keyframe = 0;
					model_editor->keyframe_is_selected = 0;
				}
			}
			else if(model_editor->tool == model_mode_nodes)
			{
				model_editor->selected_bone_index = bone_hit_index;
				selected_bone_index = bone_hit_index;
			}
			else if(model_editor->tool == model_mode_sprite_editing)
			{
				eui_selection_select(&model_editor->sprite_selection_bone, bone_hit_index);
			}
		}
		if(model_editor->tool == model_mode_nodes)
		{
			//move nodes with cursor 
			if(model_cursor_moved && bone_count)
			{
				vec3 editor_cursor_delta = vec3_sub(game_editor->model.cursor.position, cursor_position_last);

				editor_model_bone *selected_bone_data = model_editor->bones + selected_bone_index;
				model_bone *bone = &selected_bone_data->base;

				bone->p.x += editor_cursor_delta.x;
				bone->p.y += editor_cursor_delta.y;
				bone->p.z += editor_cursor_delta.z;
			}
		}

	}


	//draw ik vec
	//render_draw_cube(commands, model_editor->ik_vec, vec3_all(2), V4(255, 0, 0, 255));



	render_commands_end(commands);
}

static void 
editor_graphics_db_panel(s_editor_state *editor_state, game_renderer *game_renderer, editor_input *editor_input)
{
	game_assets *game_asset_manager = editor_state->editor_assets;
	s_game_editor *game_editor = &editor_state->editor;
	game_ui *ui = editor_state->ui;
	s_model_editor *model_editor = &game_editor->model;

	if(editor_input->f_keys[2])
	{
		ui_open_or_close_panel(ui, "editor_graphics_debug_panel");
	}
	if(_ui_window_begin(ui,
				ui_panel_flags_front_panel | ui_panel_flags_init_closed,
				200,
				200,
				600,
				600,
				1,
				&game_editor->explorer_is_opened,
				"editor_graphics_debug_panel"))
	{
		if(ui_button(ui, "dump to file"))
		{
			editor_model_dump_to_file(
					editor_state);
		}
		if(ui_button(ui, "quick set#model"))
		{
			editor_model_reset(game_editor);
			editor_model_add_texture(game_editor,
					er_look_for_resource(game_editor, "data/images/model_human.png"));

			//node or bone
			editor_model_add_bone(game_editor);
			//sprite
			model_sprite *sprite = editor_model_add_sprite(game_editor);
			sprite->v0.x = 16 * -0.5f;

			sprite->v1.x = 16 * -0.5f;
			sprite->v1.z = 16;

			sprite->v2.x = 16 * 0.5f;
			sprite->v2.z = 16;

			sprite->v3.x = 16 * 0.5f;
			//frame list
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 0, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 96, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 120, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 144, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 168, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 192, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			{
				editor_model_frame_list *frame_list = editor_model_add_frame_list(game_editor);
				sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, 0);
				render_fill_uvs_from_frames(
						512, 512, 216, 0, 24, 24,
						&uvs->uv0,
						&uvs->uv1,
						&uvs->uv2,
						&uvs->uv3);
			}
			//add empty default animation
			editor_model_add_animation(
					game_editor);

			//another to test
			editor_animation *animation1 = editor_model_add_animation(
					game_editor);
			//adding keyframes to animation 0
			editor_animation_keyframe *kf0 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 0);
			editor_animation_keyframe *kf1 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 1);
			editor_animation_keyframe *kf2 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 2);
			editor_animation_keyframe *kf3 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 3);
			editor_animation_keyframe *kf4 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 4);
			editor_animation_keyframe *kf5 = editor_animation_add_keyframe(game_editor, animation1, 0, 1, 5);
			kf0->base.type = model_animation_keyframe_frame;
			kf1->base.type = model_animation_keyframe_frame;
			kf2->base.type = model_animation_keyframe_frame;

			kf0->base.frame_list_index = 1;
			kf1->base.frame_list_index = 2;
			kf2->base.frame_list_index = 3;
			kf3->base.frame_list_index = 4;
			kf4->base.frame_list_index = 5;
			kf5->base.frame_list_index = 6;

		    

		}
		u8 *model_name = "None loaded";
		if(model_editor->editing_model)
		{
			model_name = model_editor->editing_model->path_and_name;
		}
		ui_set_wh_text(ui, 4.0f, 1.0f)
		{
			ui_set_row(ui)
			{
				ui_text(ui, "editing_model: ");
				ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
				{
					ui_text(ui, model_name);
				}
			}
			{
				ui_textf(ui, "sprite_sheets (%u): ", model_editor->sprite_sheets_count);
				ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
				for(u32 s = 0; s < model_editor->sprite_sheets_count; s++)
				{
					game_resource_attributes *sprite_sheet = model_editor->sprite_sheets[s];
					ui_text(ui, sprite_sheet->path_and_name);
					ui_space_specified(ui, 6.0f, 1.0f);
				}
				ui_textf(ui, "frame_lists (%u)", model_editor->frame_list_count);
				for(u32 f = 0; f < model_editor->frame_list_count; f++)
				{
					editor_model_frame_list *frame_list = model_editor->frame_lists + f;
					ui_text(ui, "{");
						u8 *name = model_editor->frame_list_names.chunks[f].name;
						ui_text(ui, "name:");
						ui_text(ui, name);
						ui_textf(ui, "uvs_count:%u", frame_list->uvs_count);
						ui_textf(ui, "sprite_index:%u", frame_list->uvs_count);
						ui_text(ui, "uvs");
						for(u32 u = 0; u < frame_list->uvs_count; u++)
						{
							sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, u);
							u32 fx = 0;
							u32 fy = 0;
							u32 fw = 0;
							u32 fh = 0;
							render_fill_frames_from_uvs(
									512, 512, uvs->uv0, uvs->uv1, uvs->uv2, uvs->uv3,
									&fx, &fy, &fw, &fh);

							ui_textf(ui, "%u. {%u, %u, %u, %u}", u, fx, fy, fw, fh);
						}
					ui_text(ui, "}");
					
				}

				ui_textf(ui, "animations(%u)", model_editor->animation_count);
				for(u32 a = 0; a < model_editor->animation_count; a++)
				{
					editor_animation *editor_animation = model_editor->animations + a;
					model_animation *animation = &model_editor->animations[a].base;
					ui_text(ui, "{");
					ui_set_row(ui) 
					{
						ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
						{
							ui_set_row(ui)
							{
								ui_text(ui, "name: "); 
								ui_text(ui, model_editor->animation_name_chunks.chunks[a].name);
							}
							ui_textf(ui, "transform keyframes (%u):", animation->keyframe_count);
							for(editor_animation_keyframe_row *group = editor_animation->first_row;
									group;
									group = group->next)
							{
								for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
										editor_keyframe;
										editor_keyframe = editor_keyframe->next_in_row)
								{
									ui_text(ui, "{");
									ui_set_row(ui) 
									{
										ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
										{
												model_animation_keyframe *kf = &editor_keyframe->base;
											ui_textf(ui, "frame_start: %u", kf->frame_start);
											ui_set_row(ui)
											{
												u8 *types[] = {"mesh", "billboard"};
												u8 *type = kf->type > 1 ? "MORE THAN SPECIFIED" : types[kf->type];
												ui_text(ui, "Type: ");
												ui_text(ui, type);
											}
											ui_textf(ui, "bone_index: %u", kf->bone_index);
										}
									}
									ui_text(ui, "}");
								}
							}
							ui_textf(ui, "frame_groups");
							for(editor_animation_keyframe_row *group = editor_animation->first_frame_row;
									group;
									group = group->next)
							{
								for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
										editor_keyframe;
										editor_keyframe = editor_keyframe->next_in_row)
									{
										ui_text(ui, "{");
										ui_set_row(ui) 
										{
											ui_space_specified(ui, 8.0f, 1.0f); ui_set_column(ui)
											{
												model_animation_keyframe *kf = &editor_keyframe->base; 
												ui_textf(ui, "frame_start: %u", kf->frame_start);
												ui_textf(ui, "sprite_index: %u", kf->mesh_index);
												ui_textf(ui, "frame_list_index: %u", kf->frame_list_index);
											}
										}
										ui_text(ui, "}");
									}
							}
						}
					}
					ui_text(ui, "}");
				}
			}
		}
	}
	ui_panel_end(ui);


}

static void
editor_graphics_update_render(s_editor_state *editor_state, game_renderer *game_renderer, editor_input *editor_input, f32 dt)
{
	s_game_editor *game_editor = &editor_state->editor;
	if(game_editor->graphics_tab == graphics_tab_model)
	{
	}
	else if(game_editor->graphics_tab == graphics_tab_animation)
	{
	}
}

inline render_texture *
editor_get_model_texture(
		s_game_editor *editor,
		game_assets *game_asset_manager,
		u32 sprite_index)
{
	model_sprite *spr = editor->model.sprites + sprite_index;
	render_texture *bone_texture = editor->model.sprite_sheets_as_asset[spr->texture_index];

	return(bone_texture);

}

static void
editor_model_select_frame(
		s_game_editor *game_editor,
		editor_animation *animation,
		u32 frame)
{
	s_model_editor *model_editor = &game_editor->model;
	model_editor->timeline_selected_frame = frame; 
	model_editor->timeline_frame_is_selected = 1;
	if(animation)
	{
		editor_animation_keyframe_column *column = editor_animation_get_column(
				animation,
				frame);
		model_editor->selected_column = column;
	}
}

static void
editor_model_update_render_ui(s_editor_state *editor_state, game_renderer *game_renderer, editor_input *editor_input)
{
	game_assets *game_asset_manager = editor_state->editor_assets;
	s_game_editor *game_editor = &editor_state->editor;
	game_ui *ui = editor_state->ui;
	s_model_editor *model_editor = &game_editor->model;

	//initial data
	editor_cursor_memory *cursor_memory = &game_editor->model.cursor_memory;
	u32 selected_sprites_count = cursor_memory->selected_meshes_count;

	u16 texture_array_w = game_renderer->texture_array_w;
	u16 texture_array_h = game_renderer->texture_array_h;


	u32 packed_assets_count = game_editor->packed_assets_count;
	u32 new_clicked = 0;
	u32 load_clicked = 0;
	u32 save_clicked = 0;
	b32 save_as_clicked = 0;

	u32 newAnimation = 0;
	u32 loadAnimation = 0;
	u32 saveAnimation = 0;
	f32 topbar_y = editor_TOP_BAR_H;

	b8 add_bone = 0;
	b8 remove_bone = 0;
	b8 add_sprite = 0;
	b8 remove_sprite = 0;
	bool32 open_selectable_panel = 0;

	u32 open_load_sheet_explorer = 0;

	u32 selected_sprite_sheet_index = game_editor->model.selected_sprite_sheet_index;

	model *loaded_model = &model_editor->loaded_model;
	editor_animation *editor_animation = model_editor->animation_is_selected && model_editor->selected_animation_index < model_editor->animation_count ?
		model_editor->animations + model_editor->selected_animation_index : 0;

	u8 properties_ui_focus = model_editor->properties_ui_focus;
	u32 bone_count = game_editor->model.bone_count;
	u32 selected_bone_index = game_editor->model.selected_bone_index;
	editor_model_bone *e_selected_bone = model_editor->bones + model_editor->selected_bone_index;
	model_bone *selectedBone = &e_selected_bone->base;
//	model_sprite *selected_sprite = loaded_model->sprites + game_editor->model.selected_sprite_index;
	//
	// Model editor ui or editor_model_update_render
	//
	//model editor top bar
	ui_node *top_bar;
	ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
	{
		top_bar = ui_create_node(ui, node_background | node_border | node_clickeable, "Top bar model");
	}

	ui_set_parent(ui, top_bar)
	{
#if 1
		u32 current_graphics_tab = game_editor->graphics_tab;
		u8 *graphic_tab_names[graphics_tab_total];
		graphic_tab_names[graphics_tab_model] = "Model";
		graphic_tab_names[graphics_tab_animation] = "animation";

		ui_space_specified(ui, 4.0f, 1.0f);
		ui_set_height(ui, ui_size_text(1.0f, 1.0f))
			ui_set_width(ui, ui_size_em(ui, 8, 1.0f))
			ui_set_row(ui) ui_extra_flags(ui, node_text_centered)
			{

						new_clicked = ui_button(ui, "New##new_model");
						ui_space_specified(ui, 4.0f, 1.0f);
						save_clicked = ui_button(ui, "Save##save_model");
						ui_space_specified(ui, 4.0f, 1.0f);
						save_as_clicked = ui_button(ui, "Save as##save_as_model");
						ui_space_specified(ui, 4.0f, 1.0f);
						load_clicked = ui_button(ui, "Load##load_as_model");

				if(ui_selectable(ui, current_graphics_tab == graphics_tab_model, graphic_tab_names[graphics_tab_model]))
				{
					game_editor->graphics_tab = graphics_tab_model;
				}
				if(ui_selectable(ui, current_graphics_tab == graphics_tab_animation, graphic_tab_names[graphics_tab_animation]))
				{
					game_editor->graphics_tab = graphics_tab_animation;
				}

				ui_space_specified(ui, 8.0f, 1.0f);

			}

#endif
	}


	//model tool bar
	f32 toolbar_x = 0;
	f32 toolbar_y = topbar_y + editor_TOP_BAR_H;
	f32 toolbar_w = 900;
	f32 toolbar_h = 30;

	if(game_editor->graphics_tab == graphics_tab_model)
	{
		ui_node *tool_bar;
		ui_node *data_bar;

		ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
		ui_set_w_ppct(ui, 0.5f, 1.0f)
		ui_set_h_specified(ui, 30, 1.0f)
		{
			tool_bar = ui_create_node(ui, node_clickeable | node_background | node_border, "ttool_bar");
			data_bar = ui_create_node(ui, node_clickeable | node_background | node_border, "top_data_bar");
		}
		ui_set_parent(ui, tool_bar)
		{

		}

		ui_set_parent(ui, data_bar)
		{
			ui_space_specified(ui, 4.0f, 1.0f);
			ui_set_row(ui)
			{
				ui_space_specified(ui, 4.0f, 1.0f);

				u32 camera_orientation_index16 = 
					get_orientation_index_foward(game_renderer,
							V2(0, 1),
							16);
				ui_textf(ui, "| Camera orientation index: %d|", camera_orientation_index16);
				ui_set_wh_text(ui, 4.0f, 1.0f)
				{
				//nodes
					{
						u32 max = model_editor->boneCapacity;
						u32 count = model_editor->bone_count;
						ui_text(ui, "node_count/max : {");
						f32 needed = count ? 
							(f32)count / (f32)max : 0;
						if(needed > 0.6f)
						{
							vec4 color = {255, 255, 0, 255};
							color.g = needed > 0.9f ? 0.0f : 255.0f;
							ui_set_color(ui, ui_color_text, color)
								ui_textf(ui, "%u", count);
						}
						else
						{
							ui_textf(ui, "%u", count);
						}
						ui_textf(ui, "/%u}", max);
					}

					//mesh
					{
						u32 max = model_editor->sprite_max;
						u32 count = model_editor->sprite_count;
						ui_text(ui, "mesh_count/max : {");
						f32 needed = count ? 
							(f32)count / (f32)max: 0;
						if(needed > 0.6f)
						{
							vec4 color = {255, 255, 0, 255};
							color.g = needed > 0.9f ? 0.0f : 255.0f;
							ui_set_color(ui, ui_color_text, color)
								ui_textf(ui, "%u", count);
						}
						else
						{
							ui_textf(ui, "%u", count);
						}
						ui_textf(ui, "/%u}", max);
					}
				}
			
			}
		}
	}
	//ui_space(ui, ui_size_percent_of_parent(1.0f, 0.0f));
	//Bottom bar
	//ui_set_h_specified(ui, 30.0f, 1.0f)
	//ui_set_w_ppct(ui, 1.0f, 1.0f)
	//{
	//	ui_node *bottom_bar = ui_create_node(ui, node_background, 0);

	//	ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
	//	ui_text(ui, "Data!");
	//}
	if(game_editor->graphics_tab == graphics_tab_model)
	{
		ui_set_row(ui)
		{
			ui_node *panels_node = 0;
			ui_set_w_soch(ui, 0.0f) ui_set_h_soch(ui, 0.0f)
			{
				panels_node = ui_create_node(ui, 0, 0);
			}

			//only active when the sprite properties tab is active
			game_editor->model.individual_sprite_editing = 0;
			ui_push_parent(ui, panels_node);
			switch(model_editor->tool)
			{
				case model_mode_view:
					{
				        ui_node *panel = 0;
						ui_set_w_specified(ui, 200, 1.0f) ui_set_h_soch(ui, 1.0f)
						{
							panel = ui_node_box(ui, 0);
						}
						ui_set_parent(ui, panel) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_checkbox(ui, &game_editor->model.display_edges, "Display edges");

							//select the total amount of orientations for this model
							ui_set_wh_specified(ui, 40, 1.0f)
							{
								u32 selected_orientation = 0;
								u32 orientations[ ] = {1 ,2, 4, 6, 8, 16};
								u32 orientations_count = ARRAYCOUNT(orientations);
								ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
								for(u32 o = 0; o < orientations_count; o++)
								{
									if(ui_selectablef(ui, 
												model_editor->model_orientation_count == orientations[o],
												"%u#model_view_orientation", orientations[o]))
									{
										model_editor->model_orientation_count = orientations[o];
									}
									//space between buttons
									ui_space_specified(ui, 2.0f, 1.0f);
								}

								b32 interacted = ui_game_orientation_selection(ui, 
										model_editor->model_orientation_count,
										&model_editor->model_orientation_index);

								ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 8.0f, 1.0f) ui_extra_flags(ui, node_text)
								{
#if 0
									ui_text(ui, "Attach model:");
									u8 *preview = model_editor->attachments[0] ? 
										model_editor->attachments[0]->path_and_name : "-";
									{
										ui_node *selectable = ui_selectable_boxf(ui, 0, "%s##invoke_attach_model", preview);
										if(ui_node_mouse_l_up(ui, selectable))
										{
											er_explorer_set_process(game_editor,
													er_explorer_load,
													"Select model to attach");
										}
									}
									ui_text(ui, "bone to attach");
#endif
								}
								if(interacted)
								{
									f32 pi_div = (PI * 2) / model_editor->model_orientation_count;
									f32 angle = (PI / 2) + (pi_div * model_editor->model_orientation_index);
									game_editor->model.model_foward.x = -cos32(angle);
									game_editor->model.model_foward.y = -sin32(angle);
								}

								//primero se debe crear un pequeño struct en el array
								//de attachments indicando que un modelo extra se le
								//puede colocar a este. Luego el editor dejará que a
								//aca uno de estos se le pueda poner cualquier
								//modelo que se desee
								ui_node *attachment_list_node = 0;
								ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_em(ui, 7.0f, 1.0f);
								{
									attachment_list_node = ui_node_box(ui, "Model attachment list");
								}
								ui_set_parent(ui, attachment_list_node) ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
								{
									for(u32 a = 0; a < model_editor->attachment_count; a++)
									{
									}
								}
								
								//attachment_boxes
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Attachments");
								}
								ui_node *box_bar = 0;
								ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_em(ui, 2.0f, 1.0f)
								{
									//top bar
									box_bar = ui_label(ui, 0);
									box_bar->padding_x = 2;
									box_bar->padding_y = 2;
									box_bar->layout_axis = 0;
									ui_set_parent(ui, box_bar) ui_set_wh_em(ui, 2.0f, 0.0f) ui_extra_flags(ui, node_border)
									{
										if(ui_button(ui, "+##add_attachment_data"))
										{
											em_add_attachment(game_editor);
										}
										ui_space_specified(ui, 4.0f, 1.0f);
										ui_set_disable_if(ui, !model_editor->attachment_selection.selected)
										if(ui_button(ui, "x##remove_attachment_data"))
										{
											em_remove_selected_attachment(game_editor);
										}
									}
									//attachment boxes
									for(u32 a = 0; a < model_editor->attachment_count; a++) ui_set_h_soch(ui, 1.0f)
									{
										ui_push_id_u32(ui, a);
										ui_node *box = ui_focus_box(ui, eui_selection_selected(model_editor->attachment_selection, a), "bone_attachment_box");
										//select if clicked
										if(ui_node_mouse_l_up(ui, box))
										{
											eui_selection_select(&model_editor->attachment_selection, a);
										}
										ui_set_parent(ui, box) ui_set_wh_text(ui, 4.0f, 1.0f)
										{
											editor_model_attachment *e_attachment = model_editor->attachments + a;

											ui_text(ui, "Attach model:");
											u8 *preview = "-";
											if(e_attachment->model)
											{
												preview = e_attachment->model->path_and_name;
											}
											ui_set_w_em(ui, 12.0f, 1.0f)
											{
												b32 clicked = ui_selectablef(ui, 0, "%s##invoke_attach_model", preview);
												if(clicked)
												{
													er_explorer_set_process(game_editor,
															er_explorer_load,
															"Select model to attach");
													model_editor->attachment_to_model = a;
												}
											}
											{
												ui_text(ui, "bone_to_attach");
												u8 *prev = "-";
												if(e_attachment->base.bone_index < model_editor->bone_count)
												{
													//get name of bone
													prev = model_editor->bone_name_chunks.chunks[e_attachment->base.bone_index].name;
												}
												else
												{
													e_attachment->base.bone_index = 0;
												}
												//drop down to select a parent bone
												ui_id pop_id = ui_popup_id(ui, "attachment_parent_bone");
												ui_set_w_em(ui, 12.0f, 1.0f)
												ui_drop_down(ui, pop_id, prev)
												{
													for(u32 s = 0; s < model_editor->bone_count; s++)
													{
														ui_push_id_u32(ui, s);
														u8 *name = model_editor->bone_name_chunks.chunks[s].name;
														if(ui_selectablef(ui, e_attachment->base.bone_index == s, "%s##attachment_parent_bone_sel", name))
														{
															e_attachment->base.bone_index = s;
															ui_popup_close(ui, pop_id);
														}
														ui_pop_id(ui);
													}
												}
											}
										}
										ui_pop_id(ui);

									}
								}
							}
						}
						//space to keep the bottom bar down.
						ui_space_ppct(ui, 1.0f, 0.0f);
					}break;
				case model_mode_paint: case model_mode_selection:
					{
						ui_node *cursor_paint_panel;
						ui_set_row(ui)
						{
							ui_space(ui, ui_size_percent_of_parent(1.0f, 0.0f));
							ui_set_height(ui, ui_size_percent_of_parent(1.0f, 1.0f))
							ui_set_width(ui, ui_size_specified(300, 1.0f))
							{
								cursor_paint_panel = ui_node_box(ui, "Cursor paint panel");
							}
						}

						ui_set_parent(ui, cursor_paint_panel)
						{
							ui_set_wh_text(ui, 4.0f, 1.0f)
							{
								ui_text(ui, "Frame lists");
							}
							//list of frame lists
							ui_node *frame_lists_node = 0;
							ui_set_wh_ppct(ui, 1.0f, 1.0f)
							{
								frame_lists_node = ui_box_with_scroll(ui, "paint_mode_frame_lists");
							}
							ui_set_parent(ui, frame_lists_node)
							{
								ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
								for(u32 l = 0; l < model_editor->frame_list_count; l++)
								{
									if(ui_selectablef(ui,
												model_editor->frame_list_is_selected_paint_mode &&
												model_editor->selected_frame_list_paint_mode == l,
												"%s##paint_mode_frame_list%u",
												model_editor->frame_list_names.chunks[l].name,
												l))
									{
										model_editor->selected_frame_list_paint_mode = l;
										model_editor->frame_list_is_selected_paint_mode = 1;
									}
								}
							}
						}
						ui_space_ppct(ui, 1.0f, 0.0f);

					}break;
				case model_mode_sprite_editing:
					{

						editor_model_bone *e_selected_bone = 0;
						editor_model_sprite *e_selected_sprite = 0;
						model_sprite *selected_sprite = 0;
						if(model_editor->sprite_selection_bone.selected)
						{
							e_selected_bone = model_editor->bones + model_editor->sprite_selection_bone.index;
							//deselect if index unavadible
							if(model_editor->bone_selected_sprite.selected && 
							   model_editor->bone_selected_sprite.index >= e_selected_bone->sprite_count)
							{
								eui_selection_deselect(model_editor->bone_selected_sprite);
							}
						}
						if(e_selected_bone && model_editor->bone_selected_sprite.selected)
						{
							memory_dyarray_get_safe(e_selected_bone->sprites, e_selected_sprite, model_editor->bone_selected_sprite.index);
							selected_sprite = &e_selected_sprite->base;
						}

						b32 open_sprite_properties_popup = model_editor->bone_selected_sprite.selected 
							&& model_editor->sprite_selection_bone.selected;


						ui_node *sprite_editing_panel = 0;
						ui_node *sprite_properties_panel = 0;
						ui_id sprite_sprites_popup_id = ui_id_from_string("sprite_sprites_popup");
						b32 remove_this_sprite = 0;
						ui_set_h_ppct(ui, 1.0f, 0.0f) ui_set_row(ui)
						{
							ui_set_width(ui, ui_size_specified(300, 1.0f))
								sprite_editing_panel = ui_node_box(ui, "sprite_editing_panel");
							ui_space_ppct(ui, 1.0f, 0.0f);

							if(open_sprite_properties_popup && ui_look_for_opened_popup(ui, sprite_sprites_popup_id) != 0)
							{
								ui_set_w_specified(ui, 600, 1.0f)
									sprite_properties_panel = ui_node_box(ui, "sprite_properties");
							}
						}

						ui_set_parent(ui, sprite_editing_panel)
						{
							u32 selected_index = model_editor->sprite_selection_bone.index;
							//bone list
							ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
							{
								ui_set_w_ppct(ui, 0.4f, 0.0f) ui_set_h_specified(ui, 300, 1.0f)
								{
									ui_node *box0 = ui_box_with_scroll(ui, "sprite_mode_bone_list");

									//bone list
									ui_set_parent(ui, box0)
									{
										ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
											for(u32 b = 0; b < model_editor->bone_count; b++)
											{
												u8 *n = em_bone_name(model_editor, b);
												b32 active = eui_selection_selected(model_editor->sprite_selection_bone, b);
												if(ui_selectablef(ui, active, "%s##sprite_bone_%u", n, b))
												{
													eui_selection_select(&model_editor->sprite_selection_bone, b);
													eui_selection_deselect(model_editor->bone_selected_sprite);
												}
											}
									}

								}
							}
							if(e_selected_bone)
							{
								ui_node *box1 = 0;
								ui_node *box1_bar = 0;
								ui_set_column(ui) ui_set_w_ppct(ui, 1.0f, 0.0f)
								{
									ui_set_h_em(ui, 2.0f, 1.0f)
									{
										box1_bar = ui_label(ui, 0);
										box1_bar->padding_x = 4;
										box1_bar->padding_y = 4;
										box1_bar->layout_axis = 0;
										ui_set_parent(ui, box1_bar) ui_set_wh_text(ui, 4.0f, 0.0f)
										{
											ui_push_disable_if(ui, !model_editor->sprite_selection_bone.selected);
											if(ui_button(ui, "+#sprite_to_bone"))
											{
												em_add_sprite_to_bone(game_editor, e_selected_bone);
											}
											ui_space_specified(ui, 4.0f, 1.0f);
											ui_set_disable_if(ui, !model_editor->bone_selected_sprite.selected)
											if(ui_button(ui, "x#sprite_to_bone"))
											{
												remove_this_sprite = 1;
											}
											ui_pop_disable(ui);
										}
									}
									box1 = ui_box_with_scroll(ui, "sprite_mode_bone_sprite_list");
								}
								//bone sprite list
								ui_set_parent(ui, box1)
								{
									editor_model_bone *e_bone = model_editor->bones + model_editor->sprite_selection_bone.index;
									ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
										for(u32 b = 0; b < e_bone->sprite_count; b++)
										{
											b32 active = eui_selection_selected(model_editor->bone_selected_sprite, b);
											if(ui_selectablef(ui, active, "%u#selected_bone_sprite", b))
											{
												eui_selection_select(&model_editor->bone_selected_sprite, b);
											}
										}
									//add new button
								}
								//sprite properties
								if(e_selected_sprite)
								{
									
									//select mesh or billboard
									ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
									{
										ui_text(ui, "Sprite type");
										ui_node *types_box = 0;
										ui_set_wh_soch(ui, 1.0f)
										{
											types_box = ui_node_box(ui, 0);
										}
										ui_set_parent(ui, types_box) ui_set_w_em(ui, 12.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
										{
											if(ui_selectable(ui, selected_sprite->type == model_sprite_mesh, "Mesh"))
											{
												(u32)selected_sprite->type = model_sprite_mesh;
											}
											if(ui_selectable(ui, selected_sprite->type == model_sprite_billboard, "Billboard"))
											{
												(u32)selected_sprite->type = model_sprite_billboard;
											}
										}
									}
									//Select avadible texture ids
									game_resource_attributes *selected_texture = 0;
									u8 *preview = "-";
									if(selected_sprite->texture_index < game_editor->model.sprite_sheets_count)
									{
										selected_texture = game_editor->model.sprite_sheets[selected_sprite->texture_index];
										preview = selected_texture->path_and_name;
									}
									ui_id sprite_texture_ctm_id = ui_id_from_string("sprite_texture_id");
									ui_set_height(ui, ui_size_em(ui, 2.0f * game_editor->model.sprite_sheets_count, 1.0f))
										ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))//ui_size_specified(60, 1.0f))
										{
											if(ui_context_menu_begin(
														ui,
														sprite_texture_ctm_id
														))
											{
												ui_set_h_text(ui, 4.0f, 1.0f)
													ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
													{
														for(u32 t = 0; t < game_editor->model.sprite_sheets_count; t++)
														{
															game_resource_attributes *currentAsset =
																game_editor->model.sprite_sheets[t];
															game_resource_attributes *resource = currentAsset;
															u8 *path_and_name = resource->path_and_name;

															u32 selected_id = ui_selectable(ui,
																	selected_sprite->texture_index == t,
																	path_and_name);
															if(selected_id)
															{
																selected_sprite->texture_index = t;
																ui_popup_close(ui, sprite_texture_ctm_id);
															}
														}
													}
											}
											ui_context_menu_end(ui);
										}
#if 1

									if(model_editor->sprite_sheets_count)
									{
										ui_set_h_text(ui, 4.0f, 1.0f) ui_set_width(ui, ui_size_em(ui, 20.0f, 1.0f))
										{
											game_resource_attributes *current_asset =
												game_editor->model.sprite_sheets[selected_sprite->texture_index];
											ui_node *dd = ui_drop_down_node(ui,
													current_asset->path_and_name, "Sprite type");
											if(ui_node_mouse_l_up(ui, dd))
											{
												ui_popup_open(ui,
														dd->region.x0,
														dd->region.y1,
														sprite_texture_ctm_id);
											}

										}
									}
									ui_set_wh_text(ui, 4.0f, 1.0f)
									{
										ui_text(ui, "Position");
										ui_set_w_em(ui, 4.0f, 1.0f)
										{
											ui_set_row(ui)
											{
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->p.x, "selected_sprite_px");
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->p.y, "selected_sprite_py");
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->p.z, "selected_sprite_pz");

											}
											ui_set_row(ui)
											{
												ui_input_f32(ui, 0, &selected_sprite->p.x, "selected_sprite_px_input");
												ui_input_f32(ui, 0, &selected_sprite->p.y, "selected_sprite_py_input");
												ui_input_f32(ui, 0, &selected_sprite->p.z, "selected_sprite_pz_input");
											}
										}
										ui_text(ui, "Pivot");
										ui_set_w_em(ui, 4.0f, 1.0f)
										{
											ui_set_row(ui)
											{
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->pivotX, "selected_sprite_pivot_x");
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->pivotY, "selected_sprite_pivot_y");
												ui_drag_f32(ui, 0.1f, -10000, F32MAX, &selected_sprite->pivotZ, "selected_sprite_pivot_z");

											}
											ui_set_row(ui)
											{
												ui_input_f32(ui, 0, &selected_sprite->pivotX, "selected_sprite_pivot_x_input");
												ui_input_f32(ui, 0, &selected_sprite->pivotY, "selected_sprite_pivot_y_input");
												ui_input_f32(ui, 0, &selected_sprite->pivotZ, "selected_sprite_pivot_z_input");
											}
										}
									}

									ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
									{
										//v0v1_x
										//v1v2_zy
										ui_text(ui, "bone_index: ");
										//drop down to select frame list index
										u8 *preview = "INVALID";
										if(selected_sprite->bone_index < model_editor->bone_count)
										{
											preview = model_editor->bone_name_chunks.chunks[selected_sprite->bone_index].name;
										}
										ui_id sprite_bone_list_dd_id = ui_id_from_string("sprite_bone_index_dd_id");
										ui_set_w_em(ui, 12.0f, 1.0f)
											if(ui_drop_down_beginf(ui, sprite_bone_list_dd_id, "%s##sprite_bone_index_dd", preview))
											{
												ui_node *box = 0;
												ui_set_h_em(ui, 12.0f, 1.0f)
												{
													box = ui_box_with_scroll(ui, "bone_parent_index_dd_scroll");
												}

												ui_set_parent(ui, box)
												{
#if 1
													for(u32 f = 0; f < model_editor->bone_count; f++)
													{
														ui_selectable_set_u16f(ui,
																&selected_sprite->bone_index,
																f,
																"%s##sprite_bone_parent_list%u",
																model_editor->bone_name_chunks.chunks[f].name,
																f);
													}
												}
#endif
											}
										ui_drop_down_end(ui);
									}


									if(selected_sprite->type == model_sprite_billboard)
									{

										ui_set_wh_text(ui, 4.0f, 1.0f)
											ui_text(ui, "post-rotations");
										ui_set_row(ui)
											ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
											{
											}


										ui_set_wh_text(ui, 4.0f, 1.0f)
											ui_text(ui, "depth x/y/z");

										ui_set_row(ui)
											ui_set_wh(ui, ui_size_text(4.0f, 1.0f)) ui_extra_flags(ui, node_border | node_hover_animation | node_active_animation)
											{
												ui_input_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selected_sprite->depth_x, 0, "depth x");
												ui_input_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selected_sprite->depth_y, 0, "depth y");
												ui_input_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selected_sprite->depth_z, 0, "depth z");
											}

										ui_set_wh_text(ui, 4.0f, 1.0f)
										{
											//ui_checkbox(ui, &selected_sprite->face_x, "Face X");
											//ui_checkbox(ui, &selected_sprite->face_xy, "Face XY");
											billboard_face_axis options[] = {
												billboard_face_x,
												billboard_face_xy
											};
											ui_radio_button_u32(ui, &selected_sprite->face_axis, billboard_face_x, "face_x");
											ui_radio_button_u32(ui, &selected_sprite->face_axis, billboard_face_xy, "face_xy");
										}


										//get current texture
										render_texture *bone_texture = game_editor->model.sprite_sheets_as_asset[selected_sprite->texture_index];

										ui_id orientations_popup_id = ui_id_from_string("sprite_orientations_count");

										ui_set_wh(ui, ui_size_sum_of_children(1.0f))
											ui_context_menu(ui, orientations_popup_id)
											{
												ui_set_w_em(ui, 4.0f, 1.0f)
													ui_set_h_text(ui, 4.0f, 1.0f)
													{
														u32 orientations[5] = {1 ,2, 4, 8, 16};
														u32 orientations_count = ARRAYCOUNT(orientations);
													}
											}

										ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
										{
										}

										//
										// orientations
										//
										u32 orientations[5]       = {1 ,2, 4, 8, 16};
										u32 orientationArrayCount = ARRAYCOUNT(orientations);


										ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
										{
											ui_set_row(ui)
											{
												ui_text(ui, "frame_list: ");
												//drop down to select frame list index
												u8 *preview = "INVALID";
												if(selected_sprite->frame_list_index < model_editor->frame_list_count)
												{
													preview = model_editor->frame_list_names.chunks[selected_sprite->frame_list_index].name;
												}
												ui_id sprite_frame_list_dd_id = ui_id_from_string("sprite_frame_list_dd_id");
												ui_set_w_em(ui, 12.0f, 1.0f)
													if(ui_drop_down_beginf(ui, sprite_frame_list_dd_id, "%s##sprite_frame_list_index", preview))
													{
														for(u32 f = 0; f < model_editor->frame_list_count; f++)
														{
															ui_selectable_set_u16f(ui,
																	&selected_sprite->frame_list_index,
																	f,
																	"%s##sprite_frame_list%u",
																	model_editor->frame_list_names.chunks[f].name,
																	f);
														}
													}
												ui_drop_down_end(ui);
											}

										}

									}
									else
									{

										ui_set_wh_text(ui, 4.0f, 1.0f)
										{
											ui_text(ui, "vertices");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->v0.x,ui_text_input_confirm_on_enter, "uv0_x");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->v0.y,ui_text_input_confirm_on_enter, "uv0_y");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->v0.z,ui_text_input_confirm_on_enter, "uv0_z");

											ui_text(ui, "size");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size.x, 0, "uv0_size_x");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size.y, 0, "uv0_size_y");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size.z, 0, "uv0_size_z");
											ui_text(ui, "size2");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size2.x, 0, "uv0_size_x2");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size2.y, 0, "uv0_size_y2");
											ui_spinner_f32(ui, 1, -10000, 10000.0f, &selected_sprite->size2.z, 0, "uv0_size_z2");
										}


									}

									ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
										if(ui_selectable(ui, 0, "sprites >"))
										{
											ui_popup_open_or_close(ui, 600, 100, sprite_sprites_popup_id);
										}
#if 0
									ui_popup(ui, sprite_sprites_popup_id)
									{
										ui_node *sprites_box = 0;
										ui_node *sprites_box_top_bar = 0;
										ui_set_w_specified(ui, 600, 1.0f)
										{
											//top bar with title and close button
											ui_set_h_em(ui, 2.0f, 1.0f)
											{
												sprites_box_top_bar = ui_label(ui, 0);
												sprites_box_top_bar->padding_x = 2;
												sprites_box_top_bar->padding_y = 2;
												ui_set_parent(ui, sprites_box_top_bar) ui_set_wh_text(ui, 4.0f, 0.0f)
												{
													if(ui_button(ui, "x#close_sprites_top_bar"))
													{
														ui_popup_close(ui, sprite_sprites_popup_id);
													}
												}
											}

											ui_set_color_a(ui, ui_color_background, 0.2f)
												ui_set_h_soch(ui, 1.0f)
												{
													sprites_box = ui_node_box(ui, "sprite_frames_box_");
												}
										}
										ui_set_parent(ui, sprites_box)
										{
											ui_id sprite_frame_uv_selection_popup_id = ui_id_from_string("popup_sprite_frame_sel_id");
											render_texture *texture = editor_model_get_texture(model_editor, selected_sprite->sprite_sheet_index);
											//image button if selected
											sprite_orientation *ori = 0;
											ui_set_wh_specified(ui, 64, 1.0f)
											{
												if(model_editor->sprite_frame_selection.selected)
												{
													dyarr_get_safe(e_selected_sprite->uvs, ori, model_editor->sprite_frame_selection.index);
													if(ui_button_image_uv(ui, texture,
																ori->uv0,
																ori->uv1,
																ori->uv2,
																ori->uv3,
																"open_popup_for_frame_selection"))
													{
														ui_popup_open(ui,
																20, sprites_box->region.y0, sprite_frame_uv_selection_popup_id);
													}

												}
												else
												{
													ui_create_node(ui, node_border | node_background | node_clickeable, 0);
												}
											}
											//selection popup
											if(ori) ui_popup(ui, sprite_frame_uv_selection_popup_id)
											{
												ui_node *box = 0;
												ui_set_w_specified(ui, 512, 1.0f) ui_set_h_specified(ui, 512, 1.0f)
												{
													box = ui_popup_box(ui, sprite_frame_uv_selection_popup_id, 0);
												}
												ui_set_parent(ui, box) ui_set_wh_ppct(ui, 1.0f, 0.0f)
												{
													ui_image_selection_data image_selection_data = ui_image_selection_begin(
															ui,
															texture,
															&model_editor->image_selection_down,
															&model_editor->image_selection_hot,
															&model_editor->image_selection_zoom,
															"sprite_frame_uvs_selection");
													{
														ui_image_selection_uvs(
																ui,
																image_selection_data,
																1,
																&ori->uv0,
																&ori->uv1,
																&ori->uv2,
																&ori->uv3);
														u32 orientation_count = orientation_count_from_option(ori->option);

														for(u32 f = 1; f < orientation_count; f++)
														{
															u32 fx = 0;
															u32 fy = 0;
															u32 fw = 0;
															u32 fh = 0;
															render_fill_frames_from_uvs(
																	texture->width,
																	texture->height,
																	ori->uv0,
																	ori->uv1,
																	ori->uv2,
																	ori->uv3,
																	&fx,
																	&fy,
																	&fw,
																	&fh
																	);
															vec2 uv0 = ori->uv0;
															vec2 uv1 = ori->uv1;
															vec2 uv2 = ori->uv2;
															vec2 uv3 = ori->uv3;
															uv0.x += (f32)fw / texture->width * f;
															uv1.x += (f32)fw / texture->width * f;
															uv2.x += (f32)fw / texture->width * f;
															uv3.x += (f32)fw / texture->width * f;
															ui_image_selection_draw_uvs(
																	ui,
																	image_selection_data,
																	uv0,
																	uv1,
																	uv2,
																	uv3,
																	V4(255, 0, 0, 200));
														}
													}
												}
											}
											//add and remove buttons
											ui_set_row(ui) ui_extra_flags(ui, node_border) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 2.0f, 1.0f)
											{
												if(ui_button(ui, "+#_sprite_frame"))
												{
													em_add_frame_to_sprite(game_editor, e_selected_sprite);
												}
												ui_set_disable_if(ui, !model_editor->sprite_frame_selection.selected)
													if(ui_button(ui, "x#_sprite_frame"))
													{
														em_remove_frame_from_sprite(game_editor, e_selected_sprite, model_editor->sprite_frame_selection.index);
													}
											}
											//display all uvs in this sprite
											ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_soch(ui, 1.0f)
												for(u32 u = 0; u < (u32)(selected_sprite->extra_frame_count + 1); u++)
												{
													sprite_orientation *ori = 0;
													memory_dyarray_get_safe(e_selected_sprite->uvs, ori, u);
													sprite_orientation *uvs = ori;

													u32 orientation_count = orientation_count_from_option(ori->option);

													ui_push_id_u32(ui, u);
													ui_node *box = ui_focus_box(ui, eui_selection_selected(model_editor->sprite_frame_selection, u), "sprite_frame");
													ui_pop_id(ui);

													ui_set_parent(ui, box) ui_set_wh_text(ui, 4.0f, 1.0f)
													{
														ui_push_id_node(ui, box);
														ui_node *box_area = 0;
														//ui_set_wh_ppct(ui, 1.0f, 0.0f) box_area = ui_interact_area(ui, ui_interaction_layer_top1);
														//ui_pop_id(ui);
														//select and focus this frame if clicked
														//if(ui_node_mouse_l_pressed(ui, box_area))
														//{
														//	eui_selection_select(&model_editor->sprite_frame_selection, u);
														//}

														ui_text(ui, "Properties:");
														ui_set_row(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_text(ui, 5.0f, 1.0f)
														{
															//orientations drop down
															ui_text(ui, "orientation_option");
															u8 *orientation_options[] ={
																"orientation_1",
																"orientation_4",
																"orientation_8",
																"orientation_16",
																"orientation_8_flip",
																"orientation_16_flip",
															};
															ui_push_id_u32(ui, u);
															u8 *preview = "UNDEFINED";
															if(uvs->option < orientation_option_count)
															{
																preview = orientation_options[uvs->option];
															}
															ui_id orientations_dd_id = ui_node_id_from_string(ui, "sprite_ori_option_dd_id");
															ui_set_w_em(ui, 12.0f, 1.0f)
																if(ui_drop_down_beginf(ui, orientations_dd_id, "%s##orientation_option_dd%u", preview, u))
																{
																	for(u32 o = 0; o < orientation_option_count; o++)
																	{
																		if(ui_selectablef(ui, uvs->option == o, "%s##orientation_option%u", orientation_options[o], o))
																		{
																			uvs->option = o;
																			ui_popup_close_last(ui);
																		}
																	}

																}
															ui_drop_down_end(ui);
															//x and y rot index
															ui_text(ui, "x/y_rot_index");
															{
																ui_set_w_em(ui, 5.0f, 1.0f)  ui_set_h_soch(ui, 1.0f) ui_set_column_n(ui) ui_set_h_text(ui, 4.0f, 1.0f)
																{
																	ui_spinner_i16(ui, 1, -8, 8, &ori->x_rot_index, 0, "ori_x_rot_index");
																	ui_spinner_i16(ui, 1, -8, 8, &ori->y_rot_index, 0, "ori_y_rot_index");
																}
																ui_space_specified(ui, 26.0f, 1.0f);

																//frame key
																{
																	ui_text(ui, "frame_key");
																	u8 *prev = "default";
																	b32 no_keys = selected_sprite->extra_frame_count == 0;
																	if(ori->skin_index && ori->skin_index < e_selected_bone->frame_key_count + 1)
																	{
																		prev = model_editor->frame_key_names.chunks[ori->skin_index - 1].name;
																	}
																	ui_id popid = ui_node_id_from_string(ui, "sprite_frame_f_key");
																	ui_set_w_em(ui, 12.0f, 1.0f)
																		ui_drop_down(ui, popid, prev) ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
																		{
																			//default
																			if(ui_selectable(ui, no_keys, "default#sprite_fkey"))
																			{
																				ori->skin_index = 0;
																				ui_popup_close_last(ui);
																			}
																			//others
																			for(u32 k = 0; k < e_selected_bone->frame_key_count; k++)
																			{
																				u8 *n = model_editor->frame_key_names.chunks[k].name;
																				if(ui_selectablef(ui, ori->skin_index == k + 1, "%s##sprite_fkey%u", n, k))
																				{
																					ori->skin_index = k + 1;
																					ui_popup_close_last(ui);
																				}
																			}
																		}
																}
															}
															ui_pop_id(ui);
														}
														ui_set_row(ui)
														{
															ui_push_id_u32(ui, u);
															f32 angle_div = TWOPI / orientation_count;
															f32 angle_current = 0;
															//size of displaying images
															ui_set_wh_specified(ui, 32.0f + 2, 1.0f) 
															{
																if(selected_sprite->sprite_sheet_index < model_editor->sprite_sheets_count)
																	for(u32 u = 0; u < orientation_count; u++)
																	{
																		//get uvs
																		ui_node *orientation_button = 0;
																		{
																			orientation_button = ui_create_nodef(ui, node_clickeable, "spr_orientation%u", u);

																			ui_node_push_image(
																					ui,
																					orientation_button,
																					editor_model_get_texture(model_editor, selected_sprite->sprite_sheet_index),
																					0,
																					0,
																					uvs->uv0,
																					uvs->uv1,
																					uvs->uv2,
																					uvs->uv3);
																			vec4 selection_color = {0, 160, 160, 160};
																			//smoothly change border color for selection
																			ui_node_push_hollow_rectangle(
																					ui,
																					orientation_button,
																					0,
																					0,
																					orientation_button->size_x,
																					orientation_button->size_y,
																					2,
																					selection_color);

																			vec2 mid = {
																				orientation_button->size_x * 0.5f,
																				orientation_button->size_y * 0.5f
																			};
																			vec2 p1 = mid;
																			p1.x = sin32(angle_current);
																			p1.y = cos32(angle_current);
																			f32 inner_rl = 0;
																			f32 inner_ud = 0;
																			//mid in this case is used as half of the size of the orientation button
																			if(p1.x > 0)
																			{
																				inner_rl = mid.x / p1.x;
																			}
																			else if(p1.x < 0)
																			{
																				inner_rl = -mid.x / p1.x;
																			}

																			if(p1.y > 0)
																			{
																				inner_ud = mid.y / p1.y;
																			}
																			else if(p1.y < 0)
																			{
																				inner_ud = -mid.y / p1.y;
																			}
																			if(!p1.x || inner_ud < inner_rl)
																			{
																				p1.x *= inner_ud;
																				p1.y *= inner_ud;
																			}
																			else
																			{
																				p1.x *= inner_rl;
																				p1.y *= inner_rl;
																			}
																			p1.x += mid.x;
																			p1.y += mid.y;
																			angle_current += angle_div;

																			ui_node_push_line(ui,
																					orientation_button,
																					mid,
																					p1,
																					2.0f,
																					V4(0, 255, 0, 255));
																			//ui_button_image_uvs_nodef(
																			//	ui,
																			//	editor_model_get_texture(model_editor, frame_list->sprite_index),
																			//	uvs->uv0,
																			//	uvs->uv1,
																			//	uvs->uv2,
																			//	uvs->uv3,
																			//	"current_frame_list_orientation_sel%u", u);
																		}
																	}
																else
																{
																	ui_selectable_boxf(ui, 0, 0, "current_frame_list_orientation_sel%u", u);
																}
															}
															ui_pop_id(ui);
														}
													}
													if(ui_node_mouse_l_pressed(ui, box))
													{
														eui_selection_select(&model_editor->sprite_frame_selection, u);
													}
												}

										}
									}
#endif
#endif
								}
							}
							//								if(em_ui_display_node_list(ui, game_editor, &selected_index))
							//								{
							//									eui_selection_select(&model_editor->sprite_selection_bone, selected_index);
							//								}
						}
						if(sprite_properties_panel && open_sprite_properties_popup) ui_set_parent(ui, sprite_properties_panel)
						{
							{
								ui_node *sprites_box = 0;
								ui_set_w_specified(ui, 600, 1.0f)
								{
									ui_set_color_a(ui, ui_color_background, 0.2f)
										ui_set_h_soch(ui, 1.0f)
										{
											sprites_box = ui_node_box(ui, "sprite_frames_box_");
										}
								}
								ui_set_parent(ui, sprites_box) if(selected_sprite->sprite_sheet_index < model_editor->sprite_sheets_count)
								{
									ui_id sprite_frame_uv_selection_popup_id = ui_id_from_string("popup_sprite_frame_sel_id");
									render_texture *texture = editor_model_get_texture(model_editor, selected_sprite->sprite_sheet_index);
									//image button if selected
									sprite_orientation *ori = 0;
									ui_set_wh_specified(ui, 64, 1.0f)
									{
										if(model_editor->sprite_frame_selection.selected)
										{
											mdy_get_safe(e_selected_sprite->uvs, ori, model_editor->sprite_frame_selection.index);
											if(ui_button_image_uv(ui, texture,
														ori->uv0,
														ori->uv1,
														ori->uv2,
														ori->uv3,
														"open_popup_for_frame_selection"))
											{
												ui_popup_open(ui,
														20, sprites_box->region.y0, sprite_frame_uv_selection_popup_id);
											}

										}
										else
										{
											ui_create_node(ui, node_border | node_background | node_clickeable, 0);
										}
									}
									//selection popup
									if(ori) ui_popup(ui, sprite_frame_uv_selection_popup_id)
									{
										ui_node *box = 0;
										ui_set_w_specified(ui, 512, 1.0f) ui_set_h_specified(ui, 512, 1.0f)
										{
											box = ui_popup_box(ui, sprite_frame_uv_selection_popup_id, 0);
										}
										ui_set_parent(ui, box) ui_set_wh_ppct(ui, 1.0f, 0.0f)
										{
											ui_image_selection_data image_selection_data = ui_image_selection_begin(
													ui,
													texture,
													&model_editor->image_selection_down,
													&model_editor->image_selection_hot,
													&model_editor->image_selection_zoom,
													"sprite_frame_uvs_selection");
											{
												ui_image_selection_uvs(
														ui,
														image_selection_data,
														1,
														&ori->uv0,
														&ori->uv1,
														&ori->uv2,
														&ori->uv3);
												u32 orientation_count = orientation_count_from_option(ori->option);

												for(u32 f = 1; f < orientation_count; f++)
												{
													u32 fx = 0;
													u32 fy = 0;
													u32 fw = 0;
													u32 fh = 0;
													render_fill_frames_from_uvs(
															texture->width,
															texture->height,
															ori->uv0,
															ori->uv1,
															ori->uv2,
															ori->uv3,
															&fx,
															&fy,
															&fw,
															&fh
															);
													vec2 uv0 = ori->uv0;
													vec2 uv1 = ori->uv1;
													vec2 uv2 = ori->uv2;
													vec2 uv3 = ori->uv3;
													uv0.x += (f32)fw / texture->width * f;
													uv1.x += (f32)fw / texture->width * f;
													uv2.x += (f32)fw / texture->width * f;
													uv3.x += (f32)fw / texture->width * f;
													ui_image_selection_draw_uvs(
															ui,
															image_selection_data,
															uv0,
															uv1,
															uv2,
															uv3,
															V4(255, 0, 0, 200));
												}
											}
										}
									}
									//add and remove buttons
									ui_set_row(ui) ui_extra_flags(ui, node_border) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 2.0f, 1.0f)
									{
										if(ui_button(ui, "+#_sprite_frame"))
										{
											em_add_frame_to_sprite(game_editor, e_selected_sprite);
										}
										ui_set_disable_if(ui, !model_editor->sprite_frame_selection.selected)
											if(ui_button(ui, "x#_sprite_frame"))
											{
												em_remove_frame_from_sprite(game_editor, e_selected_sprite, model_editor->sprite_frame_selection.index);
											}
									}
									//display all uvs in this sprite
									ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_soch(ui, 1.0f)
										for(u32 u = 0; u < (u32)(selected_sprite->extra_frame_count + 1); u++)
										{
											sprite_orientation *ori = 0;
											memory_dyarray_get_safe(e_selected_sprite->uvs, ori, u);
											sprite_orientation *uvs = ori;

											u32 orientation_count = orientation_count_from_option(ori->option);

											ui_push_id_u32(ui, u);
											ui_node *box = ui_focus_box(ui, eui_selection_selected(model_editor->sprite_frame_selection, u), "sprite_frame");
											ui_pop_id(ui);

											ui_set_parent(ui, box) ui_set_wh_text(ui, 4.0f, 1.0f)
											{
												ui_push_id_node(ui, box);
												ui_node *box_area = 0;
												//ui_set_wh_ppct(ui, 1.0f, 0.0f) box_area = ui_interact_area(ui, ui_interaction_layer_top1);
												//ui_pop_id(ui);
												//select and focus this frame if clicked
												//if(ui_node_mouse_l_pressed(ui, box_area))
												//{
												//	eui_selection_select(&model_editor->sprite_frame_selection, u);
												//}

												ui_text(ui, "Properties:");
												ui_set_row(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_text(ui, 5.0f, 1.0f)
												{
													//orientations drop down
													ui_text(ui, "orientation_option");
													u8 *orientation_options[] ={
														"orientation_1",
														"orientation_4",
														"orientation_8",
														"orientation_16",
														"orientation_8_flip",
														"orientation_16_flip",
													};
													ui_push_id_u32(ui, u);
													u8 *preview = "UNDEFINED";
													if(uvs->option < orientation_option_count)
													{
														preview = orientation_options[uvs->option];
													}
													ui_id orientations_dd_id = ui_node_id_from_string(ui, "sprite_ori_option_dd_id");
													ui_set_w_em(ui, 12.0f, 1.0f)
														if(ui_drop_down_beginf(ui, orientations_dd_id, "%s##orientation_option_dd%u", preview, u))
														{
															for(u32 o = 0; o < orientation_option_count; o++)
															{
																if(ui_selectablef(ui, uvs->option == o, "%s##orientation_option%u", orientation_options[o], o))
																{
																	uvs->option = o;
																	ui_popup_close_last(ui);
																}
															}

														}
													ui_drop_down_end(ui);
													//x and y rot index
													ui_text(ui, "x/y_rot_index");
													{
														ui_set_w_em(ui, 5.0f, 1.0f)  ui_set_h_soch(ui, 1.0f) ui_set_column_n(ui) ui_set_h_text(ui, 4.0f, 1.0f)
														{
															ui_spinner_i16(ui, 1, -8, 8, &ori->x_rot_index, 0, "ori_x_rot_index");
															ui_spinner_i16(ui, 1, -8, 8, &ori->y_rot_index, 0, "ori_y_rot_index");
														}
														ui_space_specified(ui, 26.0f, 1.0f);

														//frame key
														{
															ui_text(ui, "frame_key");
															u8 *prev = "default";
															b32 no_keys = selected_sprite->extra_frame_count == 0;
															if(ori->skin_index && ori->skin_index < e_selected_bone->frame_key_count + 1)
															{
																prev = model_editor->frame_key_names.chunks[ori->skin_index - 1].name;
															}
															ui_id popid = ui_node_id_from_string(ui, "sprite_frame_f_key");
															ui_set_w_em(ui, 12.0f, 1.0f)
																ui_drop_down(ui, popid, prev) ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
																{
																	//default
																	if(ui_selectable(ui, no_keys, "default#sprite_fkey"))
																	{
																		ori->skin_index = 0;
																		ui_popup_close_last(ui);
																	}
																	//others
																	for(u32 k = 0; k < e_selected_bone->frame_key_count; k++)
																	{
																		u8 *n = model_editor->frame_key_names.chunks[k].name;
																		if(ui_selectablef(ui, ori->skin_index == k + 1, "%s##sprite_fkey%u", n, k))
																		{
																			ori->skin_index = k + 1;
																			ui_popup_close_last(ui);
																		}
																	}
																}
														}
													}
													ui_pop_id(ui);
												}
												ui_set_row(ui)
												{
													ui_push_id_u32(ui, u);
													f32 angle_div = TWOPI / orientation_count;
													f32 angle_current = 0;
													//size of displaying images
													ui_set_wh_specified(ui, 32.0f + 2, 1.0f) 
													{
														if(selected_sprite->sprite_sheet_index < model_editor->sprite_sheets_count)
															for(u32 u = 0; u < orientation_count; u++)
															{
																//get uvs
																ui_node *orientation_button = 0;
																{
																	orientation_button = ui_create_nodef(ui, node_clickeable, "spr_orientation%u", u);

																	ui_node_push_image(
																			ui,
																			orientation_button,
																			editor_model_get_texture(model_editor, selected_sprite->sprite_sheet_index),
																			0,
																			0,
																			uvs->uv0,
																			uvs->uv1,
																			uvs->uv2,
																			uvs->uv3);
																	vec4 selection_color = {0, 160, 160, 160};
																	//smoothly change border color for selection
																	ui_node_push_hollow_rectangle(
																			ui,
																			orientation_button,
																			0,
																			0,
																			orientation_button->size_x,
																			orientation_button->size_y,
																			2,
																			selection_color);

																	vec2 mid = {
																		orientation_button->size_x * 0.5f,
																		orientation_button->size_y * 0.5f
																	};
																	vec2 p1 = mid;
																	p1.x = sin32(angle_current);
																	p1.y = cos32(angle_current);
																	f32 inner_rl = 0;
																	f32 inner_ud = 0;
																	//mid in this case is used as half of the size of the orientation button
																	if(p1.x > 0)
																	{
																		inner_rl = mid.x / p1.x;
																	}
																	else if(p1.x < 0)
																	{
																		inner_rl = -mid.x / p1.x;
																	}

																	if(p1.y > 0)
																	{
																		inner_ud = mid.y / p1.y;
																	}
																	else if(p1.y < 0)
																	{
																		inner_ud = -mid.y / p1.y;
																	}
																	if(!p1.x || inner_ud < inner_rl)
																	{
																		p1.x *= inner_ud;
																		p1.y *= inner_ud;
																	}
																	else
																	{
																		p1.x *= inner_rl;
																		p1.y *= inner_rl;
																	}
																	p1.x += mid.x;
																	p1.y += mid.y;
																	angle_current += angle_div;

																	ui_node_push_line(ui,
																			orientation_button,
																			mid,
																			p1,
																			2.0f,
																			V4(0, 255, 0, 255));
																	//ui_button_image_uvs_nodef(
																	//	ui,
																	//	editor_model_get_texture(model_editor, frame_list->sprite_index),
																	//	uvs->uv0,
																	//	uvs->uv1,
																	//	uvs->uv2,
																	//	uvs->uv3,
																	//	"current_frame_list_orientation_sel%u", u);
																}
															}
														else
														{
															ui_selectable_boxf(ui, 0, "current_frame_list_orientation_sel%u", u);
														}
													}
													ui_pop_id(ui);
												}
											}
											if(ui_node_mouse_l_pressed(ui, box))
											{
												eui_selection_select(&model_editor->sprite_frame_selection, u);
											}
										}

								}
							}
						}

						//never true if any sprite isn't selected
						if(remove_this_sprite)
						{
							em_remove_sprite_from_bone(
									game_editor, e_selected_bone, model_editor->bone_selected_sprite.index);

						}
					}break;
				case model_mode_nodes:
					{

						ui_node *nodes_panel;
						ui_set_height(ui, ui_size_sum_of_children(1.0f))
							ui_set_width(ui, ui_size_specified(400, 1.0f))
							{
								nodes_panel = ui_node_box(ui, "sprite_editing_panel");
							}
						ui_set_parent(ui, nodes_panel)
						{

							ui_set_wh_text(ui, 1.0f, 1.0f)
							{
								ui_checkbox(ui, &game_editor->model.node_preview_mode, "Preview transforms");
								ui_textf(ui, "bone_count: %u", loaded_model->bone_count);
							}
							//model_editor->loaded_model =  gameWorld->playerEntity.model;

							//ui_text(ui, "Selected model bone: %u");


							//List the model parts in this mini child
							{
								u32 selected_index = game_editor->model.selected_bone_index;
								em_ui_display_node_list(ui, game_editor, &selected_index);
								game_editor->model.selected_bone_index = selected_index;
							}

							ui_set_wh_text(ui, 4.0f, 1.0f) ui_set_row(ui)
							{
								add_bone = ui_button(ui, "+##add_bone");
								ui_set_disable_if(ui, model_editor->bone_count == 0);
								{
									remove_bone = ui_button(ui, "X##remove_bone") && loaded_model->bone_count;
								}
							}

							if(loaded_model->bone_count) ui_set_wh_text(ui, 4.0f, 1.0f)
							{

								ui_text(ui, "Bone name");
								ui_input_text(ui,
										0,
										game_editor->model.bone_name_chunks.chunks[game_editor->model.selected_bone_index].name,
										editor_BONE_NAME_MAX,
										"Bone name");

								ui_text(ui, "Parent");
								u32 parent_spinner_max = loaded_model->bone_count ? loaded_model->bone_count - 1 : 0;
								u32 bone_parent_last = selectedBone->parent;

								ui_push_disable_if(ui, game_editor->model.selected_bone_index == 0);
								ui_id bone_parent_dd_id = ui_id_from_string("bone_parent_dd_id");
								u8 *preview = "-";
								if(selectedBone->parent > model_editor->bone_count)
								{
									preview = "INVALID_INDEX";
								}
								else
								{
									preview = model_editor->bone_name_chunks.chunks[selectedBone->parent].name;
								}
								if(ui_drop_down_beginf(ui, bone_parent_dd_id, "%s##bone_parent_drop_down_b", preview))
								{
									ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
									for(u32 b = 0; b < model_editor->bone_count; b++)
									{
										b32 selected = selectedBone->parent == b;
										u8 *name = model_editor->bone_name_chunks.chunks[b].name;
										if(ui_selectablef(ui, selected, "%s##bone_parent_s%u", name, b))
										{
											selectedBone->parent = b;
										}
									}
								}
								ui_drop_down_end(ui);
								ui_pop_disable(ui);

								ui_extra_flags(ui, node_background | node_active_animation | node_hover_animation) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Position");
									ui_set_row(ui)
									{
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->p.x, "bonePosX");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->p.y, "bonePosY");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->p.z, "bonePosZ");
									}
									ui_set_row(ui)
									{
										ui_input_f32(ui, 0, &selectedBone->p.x, "bonePosX_input");
										ui_input_f32(ui, 0, &selectedBone->p.y, "bonePosY_input");
										ui_input_f32(ui, 0, &selectedBone->p.z, "bonePosZ_input");
									}


									ui_set_row(ui)
									{
										ui_text(ui, "displacement");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->displacement.x, "ref bonePosX");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->displacement.y, "ref bonePosY");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->displacement.z, "ref bonePosZ");
									}
									ui_set_row(ui)
									{
										ui_text(ui, "Pivot");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->pivot.x, "node_mode_pivot_x");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->pivot.y, "node_mode_pivot_y");
										ui_drag_f32(ui, 0.1f, -100.0f, 100.0f, &selectedBone->pivot.z, "node_mode_pivot_z");
									}

												ui_text(ui, "rotation");
												quaternion *quat = &selectedBone->q;
												ui_set_wh_text(ui, 4.0f, 1.0f) ui_set_row(ui)
												{
													ui_textf(ui, "{x: %f", quat->x);
													ui_textf(ui, "y: %f", quat->y);
													ui_textf(ui, "z: %f", quat->z);
													ui_textf(ui, "w: %f}", quat->w);
												}
												if(ui_button(ui, "Reset##reset bone quaternion"))
												{
													quat->w = 1;
													quat->x = 0;
													quat->y = 0;
													quat->z = 0;
												}
												ui_set_w_em(ui, 6.0f, 1.0f) ui_set_row(ui) ui_extra_flags(ui, node_border)
												{
													f32 rx = 0;
													f32 ry = 0;
													f32 rz = 0;

													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &rx, "selected_bone_q_x");
													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &ry, "selected_bone_q_y");
													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &rz, "selected_bone_q_z");
													if(rx)
													{
														quaternion qr = quaternion_rotated_at(
																1, 0, 0, rx * PI);
														*quat = quaternion_mul(*quat, qr);
													}
													else if(ry)
													{
														quaternion qr = quaternion_rotated_at(
																0, 1, 0, ry * PI);
														*quat = quaternion_mul(*quat, qr);
													}
													else if(rz)
													{
														quaternion qr = quaternion_rotated_at(
																0, 0, 1, rz * PI);
														*quat = quaternion_mul(*quat, qr);
													}
												}
								}
								ui_checkbox(ui, &selectedBone->two_dim, "two_dim");


								ui_text(ui, "r_angle");
								ui_checkbox(ui, &selectedBone->dof, "dof");
								//ui_drag_f32(ui, 0.0174532925199432957f, -100.0f, 100.0f, &selectedBone->r_angle, "r_angle");
								//ui_drag_radians(ui, 1, -180.0f, 180.0f, &selectedBone->r_angle, "r_angle");
								//ui_same_line(ui);
								ui_text(ui, "r_degree");
								ui_set_row(ui) ui_set_wh_text(ui, 4.0f ,1.0f)
								{
									ui_text(ui, "yaw/pitch/roll");
									ui_set_w_em(ui, 4.0f, 1.0f)
									{
#if 1
										f32 rx = radians_to_degrees_f32(selectedBone->rotation_x);
										f32 ry = radians_to_degrees_f32(selectedBone->rotation_y);
										f32 rz = radians_to_degrees_f32(selectedBone->rotation_z);
										//ui_spinner_f32(ui, 1, -100000, 100000, &rx, 0, "bone_rot_x");
										//ui_spinner_f32(ui, 1, -100000, 100000, &ry, 0, "bone_rot_y");
										//ui_spinner_f32(ui, 1, -100000, 100000, &rz, 0, "bone_rot_z");
										ui_input_drag_f32(ui, 1, -100000, 100000, &rx, 0, "bone_rot_x");
										ui_input_drag_f32(ui, 1, -100000, 100000, &ry, 0, "bone_rot_y");
										ui_input_drag_f32(ui, 1, -100000, 100000, &rz, 0, "bone_rot_z");

										selectedBone->rotation_x = degrees_to_radians_f32(rx);
										selectedBone->rotation_y = degrees_to_radians_f32(ry);
										selectedBone->rotation_z = degrees_to_radians_f32(rz);
#else
										f32 angle = 0;
										f32 rx = 0;
										f32 ry = 0;
										f32 rz = 0;
										quaternion_fill_rotations_degrees(
												selectedBone->q, &angle, &rx, &ry, &rz);
										b32 changed = 0;
										changed |= ui_spinner_f32(ui, 1, -100000, 100000, &rx, 0, "bone_rot_x");
										changed |= ui_spinner_f32(ui, 1, -100000, 100000, &ry, 0, "bone_rot_y");
										changed |= ui_spinner_f32(ui, 1, -100000, 100000, &rz, 0, "bone_rot_z");
										if(changed)
										{
											selectedBone->q = quaternion_from_rotations_degrees(rx, ry, rz);
										}
#endif
									}
								}

								u32 edited_quaternion = 0;
								//				selectedBone->q = quaternion_normalize_safe(selectedBone->q);
		//						quaternion editor_quaternion = quaternion_unit_to_angle_and_vector(selectedBone->q);
								ui_checkbox(ui, &model_editor->link_sprites_to_bone, "Link to sprites");
								ui_checkbox(ui, &selectedBone->virtual, "virtual#bone");
								if(selectedBone->virtual)
								{
									ui_spinner_u16(ui, 1, 0, 100, &selectedBone->virtual_attachment_bone, 0, "target_attached_bone_index");
								}
								ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 200, 1.0f)
								{
									ui_node *frame_keys_box = ui_scroll_box(ui, 0, "bone_frame_keys");
									ui_set_parent(ui, frame_keys_box)
									{
										ui_set_h_soch(ui, 1.0f)
										{
											//display frame keys
											for(u32 t = 0; t < e_selected_bone->frame_key_count; t++)
											{
												editor_frame_key *fk = memory_dyarray_get(e_selected_bone->frame_keys, t);
												ui_push_id_u32(ui, t);
												//focus box
												ui_node *frame_key_box = ui_focus_box(ui, eui_selection_selected(model_editor->bone_selected_frame_key, t), "test_frame_key");
												//select if clicked
												if(ui_node_mouse_l_pressed(ui, frame_key_box))
												{
													eui_selection_select(&model_editor->bone_selected_frame_key, t);
												}

												ui_set_parent(ui, frame_key_box) ui_set_wh_text(ui, 4.0f, 1.0f)
												{
													ui_set_row(ui) ui_set_w_em(ui, 16.0f, 0.0f)
													{
														ui_text(ui, "Name:");
														ui_input_text(ui, 0, fk->name, model_editor->frame_key_names.length - 1, "bone_frame_key_name");
													}
												}
												ui_pop_id(ui);
											}
											ui_node *new_frame_key_box = ui_node_box(ui, "new_frame_key_box");
											ui_set_parent(ui, new_frame_key_box) ui_set_wh_em(ui, 2.0f, 1.0f) ui_extra_flags(ui, node_border) ui_set_row(ui)
											{
												if(ui_button(ui, "+#bone_frame_key"))
												{
													em_add_frame_key_to_bone(game_editor, e_selected_bone);
												}
												//delete button
												ui_set_disable_if(ui, !model_editor->bone_selected_frame_key.selected)
												if(ui_button(ui, "x#bone_frame_key"))
												{
													em_remove_frame_key_from_bone(game_editor, e_selected_bone, model_editor->bone_selected_frame_key.index);
													model_editor->bone_selected_frame_key.selected = 0;
												}
											}
										}
									}
								}
							}

						}
						ui_space_ppct(ui, 1.0f, 0.0f);
					}break;
				case model_mode_properties:
					{
						//sprte_list0: texture:data/images/him.png orientations:16
						ui_node *list_nodes = 0;
						ui_node *nodes_panel = 0;
						ui_set_row(ui)
						{
							ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
							{
								ui_set_w_ppct(ui, 1.0f, 0.0f)
								{
									nodes_panel = ui_node_box(ui, "test_list");
								}
							}
						}
						ui_set_parent(ui, nodes_panel)
						{
							ui_node *frame_list_node = 0;
							ui_node *next_row_node = 0;
							ui_set_row(ui)
							{
								ui_set_h_ppct(ui, 1.0f, 0.0f)
								{
									ui_set_w_soch(ui, 1.0f) ui_set_column_n(ui) ui_set_w_em(ui, 24.0f, 1.0f) 
									{
										b32 pushed_color = model_editor->properties_ui_focus == properties_focus_sprite_sheets;
										if(pushed_color)
										{
											ui_push_color(ui, ui_color_border, V4(0, 200, 200, 255));
										}
										ui_node *frame_list_node_label = ui_create_node(ui, node_clip | node_border | node_background, 0);
										if(pushed_color)
										{
											ui_pop_color(ui, ui_color_border);
										}


										ui_set_wh_text(ui, 4.0f, 1.0f)
											ui_text(ui, "Sprite sheets");
										//sprite sheet listui_content_box_ex(ui, node_scroll_y, "model_sprite_sheets_list");
										ui_set_parent(ui, frame_list_node_label) ui_set_row(ui) ui_set_w_ppct(ui, 1.0f, 0.0f)
										{

											list_nodes = ui_create_node(ui,
													node_scroll_y | node_clickeable | node_clip,
													"model_sprite_sheets_list"); 
											list_nodes->padding_x = 6;
											list_nodes->padding_y = 6;
											ui_set_w_specified(ui, 14.0f, 1.0f)
											{
												f32 scroll_value = ui_scroll_vertical1(
														ui,
														list_nodes->size_y,
														list_nodes->content_size[1],
														list_nodes->scroll_y,
														"sprite_sheets_scroll_v");
												if(scroll_value)
												{
													ui_node_set_scroll_y(list_nodes, scroll_value);
												}
											}
										}
										ui_scroll_area_vertical(ui, list_nodes);
										if(ui_node_mouse_l_pressed(ui, list_nodes))
										{
											model_editor->properties_ui_focus = properties_focus_sprite_sheets;
										}
									}
								}
								ui_space_specified(ui, 12.0f, 1.0f);
								ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f) ui_set_column_n(ui)
								{
									ui_set_wh_text(ui, 4.0f, 1.0f)
									{
										ui_text(ui, "Frame lists");
									}
									b32 pushed_color = model_editor->properties_ui_focus == properties_focus_frame_lists;
									if(pushed_color)
									{
										ui_push_color(ui, ui_color_border, V4(0, 200, 200, 255));
									}
									frame_list_node = ui_box_with_scroll(ui, "model_frame_lists_list");
									if(pushed_color)
									{
										ui_pop_color(ui, ui_color_border);
									}
								}
								if(ui_node_mouse_l_pressed(ui, frame_list_node))
								{
									model_editor->properties_ui_focus = properties_focus_frame_lists;
								}

								ui_space_specified(ui, 4.0f, 1.0f);
								ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
								{
									next_row_node = ui_create_node(ui, 0, 0);
								}
							}
							//sprite sheet list
							ui_set_parent(ui, list_nodes)
							{
								ui_set_w_ppct(ui, 1.0f, 0.0f)
									ui_set_h_text(ui, 4.0f, 1.0f)
									{
										open_load_sheet_explorer = (ui_selectable(ui, 0, "..."));
										for(u32 s = 0;
												s < game_editor->model.sprite_sheets_count;
												s++)
										{
											game_resource_attributes *current_sheet = 
												game_editor->model.sprite_sheets[s];

											u32 selectable_active = game_editor->model.selected_sprite_sheet_index == s;

											if(ui_selectable(ui,
														selectable_active,
														current_sheet->path_and_name))
											{
												game_editor->model.selected_sprite_sheet_index = s;
												game_editor->model.selected_cursor_texture_index = s;
												model_editor->properties_ui_focus = properties_focus_sprite_sheets;
											}

										}
									}
							}
							//frame lists
							ui_set_parent(ui, frame_list_node) ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
							{
								for(u32 s = 0; s < model_editor->frame_list_count; s++)
								{
									if(ui_selectablef(ui, 
												model_editor->frame_list_is_selected &&
												s == model_editor->selected_frame_list_index, 
												"%s#select_frame_list%u", 
												model_editor->frame_list_names.chunks[s].name, s))
									{
										model_editor->frame_list_is_selected = 1;
										model_editor->selected_frame_list_index = s;
										model_editor->frame_list_selected_frame = 
											model_editor->frame_lists[s].editor_selected_frame;
										model_editor->properties_ui_focus = properties_focus_frame_lists;
									}
								}
							}
							ui_set_parent(ui, next_row_node)
							{
								ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									if(ui_button(ui, "+#add_frame_list"))
									{
										editor_model_add_frame_list(game_editor);
									}
									ui_set_disable_if(ui, !model_editor->frame_list_is_selected)
										if(ui_button(ui, "x#remove_frame_list"))
										{
											model_editor->frame_list_is_selected = 0;
											model_editor->frame_list_is_selected_paint_mode >= 
												model_editor->selected_frame_list_index ? 0 : model_editor->frame_list_is_selected_paint_mode;
											editor_model_remove_frame_list(
													game_editor,
													model_editor->selected_frame_list_index);
										}
								}
								ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Properties:");
									if(model_editor->frame_list_is_selected)
									{
											editor_model_frame_list *frame_list = model_editor->frame_lists +
												model_editor->selected_frame_list_index;
										ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
										{
											ui_set_column_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
											{
												ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui) ui_set_w_em(ui, 6.0f, 1.0f)
												{
													ui_text(ui, "Name:");
													ui_push_disable_if(ui, !model_editor->frame_list_is_selected);
													ui_set_w_em(ui, 30.0f, 0.0f) ui_set_h_text(ui, 4.0f, 1.0f)
													{
														u8 *name = "-";
														if(model_editor->frame_list_is_selected)
														{
															name = model_editor->frame_list_names.chunks[model_editor->selected_frame_list_index].name;
														}

														ui_input_text(ui,
																0,
																name,
																model_editor->frame_list_names.length - 1,
																"frame_list_name");
													}
													ui_pop_disable(ui);
												}
												//freme list uvs count
												ui_set_row(ui)
												{
													ui_text(ui, "uvs_count:");
													{
														ui_id frame_list_uvs_id = ui_id_from_string("frame_list_uvs_count");

														u32 orientations[] = {1, 4, 8, 16};
														ui_set_w_em(ui, 4.0f, 1.0f) if(ui_drop_down_beginf(ui, frame_list_uvs_id, "%u##frame_list_uvs_dd", frame_list->uvs_count))
														{
															ui_set_h_text(ui, 4.0f, 1.0f)
																for(u32 o = 0; o < 4; o++)
																{
																	if(ui_selectablef(ui, frame_list->uvs_count == orientations[o], "%u##frame_list_orientation%u", orientations[o], o))
																	{
																		frame_list->uvs_count = orientations[o];
																	}
																}
														}
														ui_drop_down_end(ui);
													}
												}
												//sprite sheets drop down
												{
													ui_id sprite_sheet_dd_id = ui_id_from_string("frame_list_sprite_sheet_dd");
													ui_text(ui, "sprite_sheet");
													u8 *dd_preview = frame_list->sprite_index < model_editor->sprite_sheets_count ?
														model_editor->sprite_sheets[frame_list->sprite_index]->path_and_name : "-";


													if(!model_editor->sprite_sheets_count)
													{
														ui_popup_close(ui, sprite_sheet_dd_id);
													}
													ui_set_w_em(ui, 20.0f, 1.0f)
														if(ui_drop_down_beginf(ui,
																	sprite_sheet_dd_id,
																	"%s##frame_list_selected_sprite_sheet%u",
																	dd_preview,
																	frame_list->sprite_index))
														{

															ui_set_h_text(ui, 4.0f, 1.0f)
															{

																for(u32 s = 0; s < model_editor->sprite_sheets_count; s++)
																{
																	if(ui_selectablef(ui,
																				frame_list->sprite_index == s,
																				"%s##frame_list_sprite_sheet_dd%u",
																				model_editor->sprite_sheets[s]->path_and_name, s))
																	{
																		frame_list->sprite_index = s;
																	}
																}

															}
														}
													ui_drop_down_end(ui);
												}
											}
											ui_space_specified(ui, 4.0f, 1.0f);
											//display uvs
											//get uvs
											sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames,
													model_editor->frame_list_selected_frame);
											ui_set_column(ui)
											{
												u32 indices[4] = {1, 2, 0, 3};
												ui_set_row(ui) for(u32 u = 0; u < 4; u++)
												{
													u32 index = indices[u];
													if(u == 2)
													{
														ui_pop_row(ui);
														ui_push_row(ui, 1.0f, 1.0f);
													}
													i32 uv0_x = (i32)(uvs->u[index].x * texture_array_w);
													i32 uv0_y = (i32)(uvs->u[index].y * texture_array_h);
													ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 2.0f, 1.0f)
													{
														ui_textf(ui, "uv%u", index);
													}
													ui_space_specified(ui, 4.0f, 1.0f);
													ui_set_w_em(ui, 3.0f, 1.0f) ui_set_row(ui) ui_set_h_text(ui, 4.0f, 1.0f)
													{
														ui_push_id_u32(ui, u);

														b8 cx = ui_spinner_i32(ui, 1, -1000, 1000, &uv0_x, 0, "uv0_x_scaled");
														b8 cy = ui_spinner_i32(ui, 1, -1000, 1000, &uv0_y, 0, "uv0_y_scaled");
														uvs->u[index].x = ((f32)uv0_x / texture_array_w);
														uvs->u[index].y = ((f32)uv0_y / texture_array_h);

														ui_pop_id(ui);
													}
												}
												ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
												{
													if(ui_button(ui, "Copy##copy uvs selection"))
													{
														model_editor->selection_uvs_copied = 1;
														model_editor->c_uv0 = uvs->uv0;
														model_editor->c_uv1 = uvs->uv1;
														model_editor->c_uv2 = uvs->uv2;
														model_editor->c_uv3 = uvs->uv3;
													}
													ui_set_disable_if(ui, !model_editor->selection_uvs_copied)
													{
														if(ui_button(ui, "Paste##paste uvs selection"))
														{
															uvs->uv0 = model_editor->c_uv0;
															uvs->uv1 = model_editor->c_uv1;
															uvs->uv2 = model_editor->c_uv2;
															uvs->uv3 = model_editor->c_uv3;
														}
													}
												}
											}
											ui_set_column(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 5.0f, 1.0f)
											{
												ui_textf(ui, "offset");
												ui_set_row(ui)
												{
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &uvs->offset.x, "selected_orientation_offset_x");
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &uvs->offset.y, "selected_orientation_offset_y");
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &uvs->offset.z, "selected_orientation_offset_z");
												}
												ui_set_row(ui)
												{
													ui_input_f32(ui, 0, &uvs->offset.x, "selected_orientation_offset_x_it");
													ui_input_f32(ui, 0, &uvs->offset.y, "selected_orientation_offset_y_it");
													ui_input_f32(ui, 0, &uvs->offset.z, "selected_orientation_offset_z_it");
												}
											}
											ui_set_column(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 5.0f, 1.0f)
											{
												ui_text(ui, "orientation_option");
												u8 *orientation_options[] ={
													"orientation_1",
													"orientation_4",
													"orientation_8",
													"orientation_16",
													"orientation_8_flip",
													"orientation_16_flip",
												};
												u8 *preview = "UNDEFINED";
												if(uvs->option < orientation_option_count)
												{
													preview = orientation_options[uvs->option];
												}
												ui_id orientations_dd_id = ui_id_from_string("orientation_option_dd_id");
												ui_set_w_em(ui, 12.0f, 1.0f)
												if(ui_drop_down_beginf(ui, orientations_dd_id, "%s##orientation_option_dd", preview))
												{
													for(u32 o = 0; o < orientation_option_count; o++)
													{
														if(ui_selectablef(ui, uvs->option == o, "%s##orientation_option%u", orientation_options[o], o))
														{
															uvs->option = o;
														}
													}

												}
												ui_drop_down_end(ui);
											}
											ui_set_column(ui) ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_em(ui, 5.0f, 1.0f)
											{
												ui_text(ui, "x/y_rot_index");
												u8 *orientation_options[] ={
													"8",
													"7",
													"6",
													"5",
													"4",
													"3",
													"2",
													"1",
												};
												ui_spinner_i16(ui, 1, -8, 8, &uvs->x_rot_index, 0, "uvs_x_rot_index");
												ui_spinner_i16(ui, 1, -8, 8, &uvs->y_rot_index, 0, "uvs_y_rot_index");
#if 0
												u8 *preview = "UNSUPPORTED";
												if(uvs->x_rot_index >= -8 && uvs->x_rot_index <= 8)
												{
													preview = orientation_options[uvs->x_rot_index];
												}
												ui_id orientations_dd_id = ui_id_from_string("pitch_index_dd_id");
												ui_set_w_em(ui, 12.0f, 1.0f)
												if(ui_drop_down_beginf(ui, orientations_dd_id, "%u##pitch_index_dd", preview))
												{
												}
												ui_drop_down_end(ui);
#endif
											}

										}

										//selectable image for selected uvs
										ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
										{

											//side images for selection
											{
												ui_node *frame_list_orientations_list;
												ui_node *frame_list_orientations_top_bar;
												//ui_set_w_specified(ui, 64 + 2 + 14 + 12, 1.0f) ui_set_h_soch(ui, 0.0f)
												//{
												//	frame_list_orientations_list = ui_box_with_scroll(ui, "frame_list_orientations_list_scroll");
												//}
												ui_set_row(ui)
												{
													//column for top bar and main contents node
													ui_node *list_label = 0;
													ui_set_column(ui)
													{
														ui_set_w_specified(ui, 96.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
														{
															//top bar with add and remove buttons
															ui_set_h_em(ui, 2.0f ,1.0f)
															{
																frame_list_orientations_top_bar = ui_label(ui, 0);
																frame_list_orientations_top_bar->padding_x = 2;
																frame_list_orientations_top_bar->padding_y = 2;
																frame_list_orientations_top_bar->layout_axis = ui_axis_x;
																ui_set_parent(ui, frame_list_orientations_top_bar)
																{
																	//add and remove buttons
																	ui_set_wh_text(ui, 4.0f, 0.0f)
																	{
																		if(ui_button(ui, "+#frame_list_frame"))
																		{
																			editor_model_add_frame_list_frame(frame_list);
																		}
																		ui_space_specified(ui, 4.0f, 1.0f);
																		ui_push_disable_if(ui, !model_editor->frame_list_frame_is_selected || frame_list->total_frames_count <= 1);
																		if(ui_button(ui, "x#frame_list_frame"))
																		{
																			editor_model_remove_frame_list_frame(frame_list,
																					frame_list->editor_selected_frame);
																			model_editor->frame_list_frame_is_selected = 0;
																		}
																		ui_pop_disable(ui);
																	}
																}
															}
															list_label = ui_label(ui, 0);
														}
														ui_set_parent(ui, list_label) ui_set_row(ui)
														{
															frame_list_orientations_list = ui_create_node(ui, node_scroll_y | node_clip, "frame_list_orientations_list");
															ui_space_ppct(ui, 1.0f, 0.0f);
															ui_set_w_specified(ui, 14.0f, 1.0f)
															{
																f32 scroll_delta = ui_scroll_vertical1(
																		ui,
																		frame_list_orientations_list->size_y,
																		frame_list_orientations_list->content_size[1],
																		frame_list_orientations_list->scroll_y,
																		"frame_list_orientations_list_scroll_v");
																if(scroll_delta != frame_list_orientations_list->scroll_y)
																{
																	ui_node_set_scroll_y(frame_list_orientations_list, scroll_delta);
																}
															}
															ui_scroll_area_vertical(ui, frame_list_orientations_list);
														}
													}
												}
												ui_set_parent(ui, frame_list_orientations_list)
												{
													f32 angle_div = TWOPI / frame_list->uvs_count;
													f32 angle_current = 0;
													//size of displaying images
													ui_set_wh_specified(ui, 64.0f + 2, 1.0f) 
													{
														for(u32 u = 0; u < frame_list->total_frames_count; u++)
														{
															//get uvs
															sprite_orientation *uvs = memory_dyarray_get(frame_list->mesh_frames, u);
															ui_node *orientation_button = 0;
															if(frame_list->sprite_index < model_editor->sprite_sheets_count)
															{
																orientation_button = ui_create_nodef(ui, node_clickeable, "fl_orientation%u", u);

																ui_node_push_image(
																		ui,
																		orientation_button,
																		editor_model_get_texture(model_editor, frame_list->sprite_index),
																		0,
																		0,
																		uvs->uv0,
																		uvs->uv1,
																		uvs->uv2,
																		uvs->uv3);
																vec4 selection_color = {0, 160, 160, 160};
																ui_usri ob_usri = ui_usri_from_node(ui, orientation_button);
																//if selected highlight
																if(model_editor->frame_list_selected_frame == u)
																{
																	selection_color.a = 255;
																}
																//smoothly change border color for selection
																else if(ui_usri_mouse_hover(ob_usri))
																{
																	selection_color.a = f32_lerp(160, orientation_button->hot_time, 240);
																}
																//select frame
																if(ui_usri_mouse_l_up(ob_usri))
																{
																	//model_editor->frame_list_selected_frame = u;
																	//model_editor->frame_list_frame_is_selected = 1;
																}
																ui_node_push_hollow_rectangle(
																		ui,
																		orientation_button,
																		0,
																		0,
																		orientation_button->size_x,
																		orientation_button->size_y,
																		2,
																		selection_color);

																vec2 mid = {
																	orientation_button->size_x * 0.5f,
																	orientation_button->size_y * 0.5f
																};
																vec2 p1 = mid;
																p1.x = sin32(angle_current);
																p1.y = cos32(angle_current);
																f32 inner_rl = 0;
																f32 inner_ud = 0;
																//mid in this case is used as half of the size of the orientation button
																if(p1.x > 0)
																{
																	inner_rl = mid.x / p1.x;
																}
																else if(p1.x < 0)
																{
																	inner_rl = -mid.x / p1.x;
																}

																if(p1.y > 0)
																{
																	inner_ud = mid.y / p1.y;
																}
																else if(p1.y < 0)
																{
																	inner_ud = -mid.y / p1.y;
																}
																if(!p1.x || inner_ud < inner_rl)
																{
																	p1.x *= inner_ud;
																	p1.y *= inner_ud;
																}
																else
																{
																	p1.x *= inner_rl;
																	p1.y *= inner_rl;
																}
																p1.x += mid.x;
																p1.y += mid.y;
																angle_current += angle_div;

																ui_node_push_line(ui,
																		orientation_button,
																		mid,
																		p1,
																		2.0f,
																		V4(0, 255, 0, 255));
																//ui_button_image_uvs_nodef(
																//	ui,
																//	editor_model_get_texture(model_editor, frame_list->sprite_index),
																//	uvs->uv0,
																//	uvs->uv1,
																//	uvs->uv2,
																//	uvs->uv3,
																//	"current_frame_list_orientation_sel%u", u);
															}
															else
															{
																orientation_button = ui_selectable_boxf(ui, 0, 0, "current_frame_list_orientation_sel%u", u);
															}
															if(ui_node_mouse_l_up(ui, orientation_button))
															{
																model_editor->frame_list_selected_frame = u;
																model_editor->frame_list_frame_is_selected = 1;
																frame_list->editor_selected_frame = u;
															}
														}
													}
												}
											}
											if(frame_list->sprite_index < model_editor->sprite_sheets_count) 
											{
												ui_set_wh_ppct(ui, 1.0f, 0.0f)
												{

													sprite_orientation *uvs = memory_dyarray_get(
															frame_list->mesh_frames,
															model_editor->frame_list_selected_frame);
													ui_set_column(ui)
													{
														ui_node *top_bar_node = 0;
														ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
														ui_set_color(ui, ui_color_background, V4(0x2c, 0x2c, 0x30, 0xff))
														{
																top_bar_node = ui_create_node(
																		ui,
																		node_background,
																		0);
														}
#if 1
														render_texture *texture = editor_model_get_texture(model_editor, frame_list->sprite_index);
														ui_image_selection_data image_selection_data = ui_image_selection_begin(
																ui,
																texture,
																&model_editor->image_selection_down,
																&model_editor->image_selection_hot,
																&model_editor->image_selection_zoom,
																"frame_list_uvs_orientation_selection");
														{
															ui_image_selection_uvs(
																	ui,
																	image_selection_data,
																	1,
																	&uvs->uv0,
																	&uvs->uv1,
																	&uvs->uv2,
																	&uvs->uv3);
															u32 orientation_count = orientation_count_from_option(uvs->option);

															for(u32 f = 1; f < orientation_count; f++)
															{
																u32 fx = 0;
																u32 fy = 0;
																u32 fw = 0;
																u32 fh = 0;
																render_fill_frames_from_uvs(
																		texture->width,
																		texture->height,
																		uvs->uv0,
																		uvs->uv1,
																		uvs->uv2,
																		uvs->uv3,
																		&fx,
																		&fy,
																		&fw,
																		&fh
																		);
																vec2 uv0 = uvs->uv0;
																vec2 uv1 = uvs->uv1;
																vec2 uv2 = uvs->uv2;
																vec2 uv3 = uvs->uv3;
																uv0.x += (f32)fw / texture->width * f;
																uv1.x += (f32)fw / texture->width * f;
																uv2.x += (f32)fw / texture->width * f;
																uv3.x += (f32)fw / texture->width * f;
																ui_image_selection_draw_uvs(
																		ui,
																		image_selection_data,
																		uv0,
																		uv1,
																		uv2,
																		uv3,
																		V4(255, 0, 0, 200));
															}
														}
#else
														ui_node *n = ui_image_selection_begin(
																ui,
																editor_model_get_texture(model_editor, frame_list->sprite_index),
																"frame_list_uvs_orientation_selection");
														{
															ui_image_selection_uvs(
																	ui,
																	editor_model_get_texture(model_editor, frame_list->sprite_index),
																	n,
																	1,
																	&uvs->uv0,
																	&uvs->uv1,
																	&uvs->uv2,
																	&uvs->uv3);
														}
#endif
														ui_image_selection_end(ui);
														//top bar data
#if 1
														ui_set_parent(ui, top_bar_node)
														{
															f32 zoom = model_editor->image_selection_zoom;
															ui_set_row(ui) 
															{
																//buttons and stuff
																ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
																{
																	ui_node *bar_button = ui_create_node(
																			ui,
																			node_clickeable |
																			node_text |
																			node_text_centered |
																			node_hover_animation |
																			node_active_animation,
																			"Flip selection horizontally#S_IMAGE_FLIP_H");
																	if(ui_node_mouse_l_up(ui, bar_button))
																	{
																	//	flip_uvs_h = 1;
																		render_flip_and_fill_uvs_horizontally(
																				&uvs->uv0,
																				&uvs->uv1,
																				&uvs->uv2,
																				&uvs->uv3);
																	}
																}
																//display grid check box
																{
																	ui_node *cb_region;
																	ui_set_wh(ui, ui_size_sum_of_children(1.0f))
																	{
																		cb_region = ui_create_node(
																				ui,
																				node_clickeable | node_hover_animation | node_active_animation,
																				"__SHOW_GRID_CB__");
																	}
																	ui_set_parent(ui, cb_region) 
																	{
																		//padding
																		ui_space_specified(ui, 2.0f, 1.0f);
																		ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
																		{
																			ui_text(ui, "Show grid");
																			ui_node *cb_node = 0;
																			ui_set_wh(ui, ui_size_specified(14.0f, 1.0f))
																				cb_node = ui_create_node(ui, 0, 0);
																			ui_style_node_push_checkbox_check(
																					ui,
																					cb_node,
																					0,
																					0,
																					12,
																					model_editor->image_selection_display_grid);
																		}
																	}
																	if(ui_node_mouse_l_up(ui, cb_region))
																	{
																		model_editor->image_selection_display_grid = !model_editor->image_selection_display_grid;
																	}
																}

																ui_space_ppct(ui, 1.0f, 0.0f);

																//data
																ui_set_h_text(ui, 4.0f, 1.0f)
																	ui_set_w_em(ui, 40.0f, 1.0f)
																	{
																		i32 cursor_x = image_selection_data.image_cursor_x;
																		i32 cursor_y = image_selection_data.image_cursor_y;
																		ui_set_row(ui)
																		{
																			ui_textf(ui, 
																					"Cursor {%d, %d}",
																					cursor_x, cursor_y);
#if 0

																			u32 frame_x = 0;
																			u32 frame_y = 0;
																			u32 frame_w = 0;
																			u32 frame_h = 0;
																			if(n->uvs_group_selection_index < total_uvs_count && texture)
																			{
																				u32 uvs_group_index = n->uvs_group_selection_index * 4;
																				vec2 uv0 = *uvs[uvs_group_index];
																				vec2 uv1 = *uvs[uvs_group_index + 1];
																				vec2 uv2 = *uvs[uvs_group_index + 2];
																				vec2 uv3 = *uvs[uvs_group_index + 3];
																				render_fill_frames_from_uvs(
																						texture->width, texture->height, uv0, uv1, uv2, uv3,
																						&frame_x,
																						&frame_y,
																						&frame_w,
																						&frame_h
																						);
																			}
																			ui_textf(ui,
																					"Selected frames: {%u, %u, %u, %u}",
																					frame_x, frame_y, frame_w, frame_h);
#endif
																		}
																	}
															}
														}
#endif
													}
#if 0
													ui_image_selection_region(
															ui,
															editor_model_get_texture(model_editor, frame_list->sprite_index),
															model_editor->selection_grid_w,
															model_editor->selection_grid_h,
															model_editor->display_image_selection_grid,
															1,
															selectable_uvs,
															"frame_list_uvs_orientation_selection");
#endif
												}
											}
											else
											{
												//ui_space_ppct(ui, 1.0f, 0.0f);
											}

										}
									}
								}
							}
						}
						//uvs: 0:{x, x, x, x}, 1:
					}
					break;
				case model_mode_animation:
					{
#if 1
						model_animation_timer *animation_timer = &game_editor->model.timer;

						u32 animation_count = game_editor->model.animation_count;

						model_animation *animation = 0;

						u32 selected_animation_frame_duration = 0;
						if(editor_animation)
						{
							//editor_animation = game_editor->model.animations + game_editor->model.selected_animation_index;
							animation = &editor_animation->base;
							selected_animation_frame_duration = animation->frames_total;

							if(!animation->frames_per_ms)
							{
								animation->frames_per_ms = 1;
							}
							if(!animation->frame_timer_repeat)
							{
								animation->frame_timer_repeat = 1;
							}

						}
						//animation list, properties
						ui_set_row(ui)
						{
							ui_set_w_specified(ui, 400, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
								ui_content_box_be(ui,
										"Model animations")
								{
									ui_node *animation_list_node = 0;
									ui_usri animation_list_node_usri = {0};
									ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 300, 0.0f)
									{
										if(model_editor->ui_focus == ea_focus_animation_list)
										{
											ui_set_color(ui, ui_color_border, V4(0, 200, 200, 255))
											{
												animation_list_node = ui_scroll_box(ui, &animation_list_node_usri, "figure_animation_list");
											}
										}
										else
										{
												animation_list_node = ui_scroll_box(ui, &animation_list_node_usri, "figure_animation_list");
										}
									}
									ui_set_parent(ui, animation_list_node) ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
									{
										if(ui_usri_mouse_l_pressed(animation_list_node_usri))
										{
											model_editor->ui_focus = ea_focus_animation_list;
										}
										//Select animation
										for(u32 o = 0; o < animation_count; o++)
										{
											u8 *animation_name = game_editor->model.animation_name_chunks.chunks[o].name;
											b32 selected = model_editor->animation_is_selected && 
												model_editor->selected_animation_index == o;
											if(ui_selectable(ui, selected, animation_name))
											{
												game_editor->model.selected_animation_index = o;
												model_editor->animation_is_selected = 1;
												model_editor->keyframe_is_selected = 0;
												model_editor->timeline_row_is_selected = 0;
												model_editor->timer.dt_transcurred = 0;
												model_editor->timer.dt_current = 0;
												model_editor->timer.frame_current = 0;
											}

										}
									}

									b8 add_animation = 0;
									b8 remove_animation = 0;
									b8 move_animation_l = 0;
									b8 move_animation_r = 0;
									ui_set_wh_text(ui, 4.0f, 1.0f) ui_set_row(ui)
									{
										add_animation = ui_button(ui, "+##add_animation");
										ui_space_specified(ui, 4.0f, 1.0f);
										ui_push_disable_if(ui, game_editor->model.selected_animation_index >= animation_count);
										remove_animation = ui_button(ui, "x##remove_animation");
										ui_space_specified(ui, 4.0f, 1.0f);
										/*
										   kf1 = kf0 + 1;
										   while(kf1->bone_index == kf0->bone_index)
										   {
										       kf1++;
										       if(kf1->type == keyframe_type_transform)
												  break;
										   }
										*/

										ui_push_disable_if(ui, game_editor->model.selected_animation_index == 0);
										move_animation_l = (ui_button(ui, "<"));
										ui_space_specified(ui, 4.0f, 1.0f);
										ui_pop_disable(ui);

										ui_push_disable_if(ui, game_editor->model.selected_animation_index >= (animation_count - 1));
										move_animation_r = (ui_button(ui, ">"));
										ui_space_specified(ui, 4.0f, 1.0f);
										ui_pop_disable(ui);
										ui_pop_disable(ui);
									}


									if(add_animation)
									{
										editor_model_add_animation(game_editor);
									}
									if(remove_animation)
									{
										editor_animation_remove(game_editor, game_editor->model.selected_animation_index);
									}
									if(move_animation_r)
									{
										u32 i0 = game_editor->model.selected_animation_index;
										u32 i1 = i0 + 1;

										editor_animation_switch_animation_index(
												game_editor,
												i0,
												i1);
										game_editor->model.selected_animation_index = i1;

									}
									else if(move_animation_l)
									{
										u32 i0 = game_editor->model.selected_animation_index;
										u32 i1 = i0 - 1;

										editor_animation_switch_animation_index(
												game_editor,
												i0,
												i1);
										game_editor->model.selected_animation_index = i1;
									}

									//Set cursor at the side of the panel
									if(animation) ui_set_wh_text(ui, 4.0f, 1.0f)
									{
										//Select bone depending on which one the key frame is pointing at 

										ui_text(ui, "Settings:");
										//				ui_content_box_be(ui, ui_panel_flags_borders, ui_current_panel_w)
										if(animation_count)
										{
											ui_set_row(ui)
											{
												ui_text(ui, "name");
												ui_input_text(ui,
														0,
														game_editor->model.animation_name_chunks.chunks[game_editor->model.selected_animation_index].name,
														model_editor->animation_name_chunks.length - 1,
														"text_input_animation_name"
														);
											}
											ui_set_row(ui)
											{
												ui_textf(ui, "Total frames (%f seconds)", animation->frames_total * 0.1f);
												ui_spinner_u32(ui, 1, 0, 100, &animation->frames_total,ui_text_input_confirm_on_enter , "frames_total");
											}
											ui_set_row(ui)
											{
												ui_text(ui, "Frames per ms");
												ui_spinner_u32(ui, 1, 1, 10, &animation->frames_per_ms, 0, "frames per ms");
											}

											ui_checkbox(ui, &animation->loop, "Loop");
											ui_text(ui, "Loop start/end");
											ui_set_w_specified(ui, 80, 1.0f)
												ui_set_row(ui)
												{
													//ui_spinner_u16(ui, 1, 0, animation->frames_total, &selected_animation_index->frame_loop_start,ui_text_input_confirm_on_enter , "frame_loop_s");
													//ui_spinner_u16(ui, 1, 0, animation->frames_total, &animation->frame_loop_end  ,ui_text_input_confirm_on_enter , "frame_loop_e");
												}

											ui_checkbox(ui, &animation->keep_timer_on_transition, "keep_timer_on_transition");
											ui_checkbox(ui,&animation->frame_timer ,"Use frame timer");
											ui_checkbox(ui,&animation->keep_timer_on_transition ,"keep_timer_on_transition");
											ui_checkbox(ui,&animation->repeat,"repeat#repeat_current_animation");


											ui_set_row(ui)
											{
												ui_text(ui, "Frame repeat");
												ui_spinner_u16(ui, 1, 1, 10, &animation->frame_timer_repeat, 0, "animation frame repeat");
											}

											ui_checkbox(ui, &model_editor->reproduce_animation, "reproduce_animation");
											ui_text(ui, "Timer data:");
											ui_textf(ui, "frame_current %u", model_editor->timer.frame_current);
											ui_textf(ui, "frame_transition: %u", model_editor->timer.frame_transition);
											ui_textf(ui, "dt_current: %f", model_editor->timer.dt_current);
											ui_textf(ui, "dt_transcurred: %f", model_editor->timer.dt_transcurred);


										}
									}
								}
							ui_space_ppct(ui, 1.0f, 0.0f);
							ui_node *keyframe_data_panel = 0;
							ui_set_w_specified(ui, 400, 1.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
							{
								keyframe_data_panel = ui_node_box(ui, "selected_animation_clips_keyframes");
							}

							ui_set_parent(ui, keyframe_data_panel)
							{
								u32 tab_index = 0;

								u32 animation_count = game_editor->model.animation_count;

								ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									ui_text(ui, "Selected frame:");
									if(model_editor->timeline_frame_is_selected)
									{
										ui_textf(ui, "%u", model_editor->timeline_selected_frame);
									}
								}
								if(animation) ui_set_wh_text(ui, 4.0f, 1.0f)
								{
									//Select bone depending on which one the key frame is pointing at 
									ui_set_row(ui)
									{
										ui_push_disable_if(ui, model_editor->keyframe_is_selected);
										if(ui_button(ui, "+##add_keyframe"))
										{
											u32 i = model_editor->selected_timeline_row;
											if(i >= model_editor->bone_count)
											{
									//			u32 index = i - model_editor->bone_count;
									//			editor_animation_add_keyframe(game_editor,
									//					editor_animation,
									//					index,
									//					1,
									//					model_editor->timeline_selected_frame);
											}
											else
											{
												editor_animation_add_keyframe(game_editor,
														editor_animation,
														i,
														0,
														model_editor->timeline_selected_frame);
											}
												
										}
										ui_pop_disable(ui);
										ui_push_disable_if(ui, !model_editor->keyframe_is_selected);
										if(ui_button(ui, "x##remove_keyframe"))
										{
											editor_animation_remove_keyframe(
													game_editor,
													editor_animation,
													model_editor->selected_keyframe);
										}
										ui_pop_disable(ui);
									}
									if(model_editor->keyframe_is_selected)
									{
										editor_animation_keyframe *editor_keyframe = model_editor->selected_keyframe;
										if(editor_keyframe)
										{
											model_animation_keyframe *keyframe = &editor_keyframe->base;
											i32 frame = editor_keyframe->base.frame_start;
											ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
											{
												ui_textf(ui, "frame_start:");
												//frame = keyframe->frame_start;
												ui_spinner_i32(ui, 1, 0, 1000, &frame, 0, "keyframe_frame");
												if(frame != keyframe->frame_start)
												{
													editor_animation_change_keyframe_frame(
															game_editor,
															editor_animation,
															editor_keyframe,
															frame);
												}
											}
											if(keyframe->type == model_animation_keyframe_transform)
											{
												u8 *name = "INVALID_SPRITE";
												if(keyframe->bone_index < model_editor->bone_count)
												{
													name = model_editor->bone_name_chunks.chunks[keyframe->bone_index].name;
												}
												ui_textf(ui, "node : %s", name);

												ui_text(ui, "position");
												if(ui_button(ui, "reset#keyframe_p"))
												{
													keyframe->offset.x = 0;
													keyframe->offset.y = 0;
													keyframe->offset.z = 0;
												}
												ui_set_w_em(ui, 6.0f, 1.0f)
												{
													b32 changed = 0;
													vec3 before = model_editor->ik_vec;
													ui_set_row(ui)
													{
														changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.x, "selected_keyframe_pos_x");
														changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.y, "selected_keyframe_pos_y");
														changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.z, "selected_keyframe_pos_z");
													}
													ui_set_row(ui)
													{
														ui_input_f32(ui, 0, &keyframe->offset.x, "selected_keyframe_pos_it_x");
														ui_input_f32(ui, 0, &keyframe->offset.y, "selected_keyframe_pos_it_y");
														ui_input_f32(ui, 0, &keyframe->offset.z, "selected_keyframe_pos_it_z");
													}
													ui_text(ui, "Ik test");
													{
														ui_set_row(ui)
														{
															changed |= ui_input_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &model_editor->ik_vec.x, 0, "selected_ik_pos_x");
															changed |= ui_input_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &model_editor->ik_vec.y, 0, "selected_ik_pos_y");
															changed |= ui_input_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &model_editor->ik_vec.z, 0, "selected_ik_pos_z");
														}
													}
													if(model_editor->apply_ik && changed)
													{
														vec3 delta = vec3_sub(model_editor->ik_vec, before);
														//delta.x = 0;
														//delta.y = 0;
														//delta.z = 0;
														editor_model_apply_ik_fabrik(game_editor, game_renderer, editor_animation, editor_keyframe, delta);
													}
												}
												ui_text(ui, "offset (for now same as position)");
												ui_set_w_em(ui, 6.0f, 1.0f) ui_set_row(ui)
												{
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.x, "selected_keyframe_offset_x");
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.y, "selected_keyframe_offset_y");
													ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &keyframe->offset.z, "selected_keyframe_offset_z");
												}
												ui_set_wh_text(ui, 4.0f, 1.0f)
												{
													ui_checkbox(ui, &model_editor->lock_descendants, "lock_descendants");
													ui_checkbox(ui, &model_editor->apply_ik, "apply_ik");
												}
												ui_set_row(ui) ui_extra_flags(ui, node_border)
												{
													ui_text(ui, "rotation");
													if(ui_button(ui, "Reset##reset keyframe quaternion"))
													{
														keyframe->q.w = 1;
														keyframe->q.x = 0;
														keyframe->q.y = 0;
														keyframe->q.z = 0;
													}
												}
												ui_set_wh_text(ui, 4.0f, 1.0f) ui_set_row(ui)
												{
													ui_textf(ui, "{x: %f", keyframe->q.x);
													ui_textf(ui, "y: %f", keyframe->q.y);
													ui_textf(ui, "z: %f", keyframe->q.z);
													ui_textf(ui, "w: %f}", keyframe->q.w);
												}
												ui_set_w_em(ui, 6.0f, 1.0f) ui_set_row(ui) ui_extra_flags(ui, node_border)
												{
													f32 rx = 0;
													f32 ry = 0;
													f32 rz = 0;

													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &rx, "selected_keyframe_q_x");
													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &ry, "selected_keyframe_q_y");
													ui_drag_f32(ui, 0.01f, -10000.0f, 10000.0f, &rz, "selected_keyframe_q_z");
													if(rx)
													{
														quaternion qr = quaternion_rotated_at(
																1, 0, 0, rx * PI);
														keyframe->q = quaternion_mul(keyframe->q, qr);
													}
													else if(ry)
													{
														quaternion qr = quaternion_rotated_at(
																0, 1, 0, ry * PI);
														keyframe->q = quaternion_mul(keyframe->q, qr);
													}
													else if(rz)
													{
														quaternion qr = quaternion_rotated_at(
																0, 0, 1, rz * PI);
														keyframe->q = quaternion_mul(keyframe->q, qr);
													}
													if(model_editor->lock_descendants && (rx || ry || rz))
													{
														//find descendant bones
														editor_animation_keyframe *root_keyframe = editor_keyframe->column->first_keyframe;
#if 0
														for(editor_animation_keyframe *kf = root_keyframe;
																kf; kf = kf->next_in_column)
														{
															model_bone *bone = model_editor->bones + kf->base.bone_index;
															//found descendant
															if(bone->parent == keyframe->bone_index)
															{
																quaternion qx = quaternion_rotated_at(1, 0, 0, -rx * PI);
																quaternion qy = quaternion_rotated_at(0, 1, 0, -ry * PI);
																quaternion qz = quaternion_rotated_at(0, 0, 1, -rz * PI);
																kf->base.q = quaternion_mul(kf->base.q, qx);
																kf->base.q = quaternion_mul(kf->base.q, qy);
																kf->base.q = quaternion_mul(kf->base.q, qz);
															}
														}
#else
														//look for descendants in order to invert the applied parent rotation,
														//add a new keyframe if necessary.
														editor_model_bone *editor_bone = model_editor->bones + keyframe->bone_index;
														model_bone *bone = &editor_bone->base;
														for(u32 b = 0; b < model_editor->bone_count; b++)
														{
															model_bone *bone1 = &model_editor->bones[b].base;
															if(bone1 != bone && bone1->parent == keyframe->bone_index)
															{
																//add or get keyframe of this bone.
																editor_animation_keyframe *kf = editor_animation_add_keyframe(
																		game_editor, editor_animation, b, model_animation_keyframe_transform, keyframe->frame_start);
																quaternion qx = quaternion_rotated_at(1, 0, 0, -rx * PI);
																quaternion qy = quaternion_rotated_at(0, 1, 0, -ry * PI);
																quaternion qz = quaternion_rotated_at(0, 0, 1, -rz * PI);
																kf->base.q = quaternion_mul(kf->base.q, qx);
																kf->base.q = quaternion_mul(kf->base.q, qy);
																kf->base.q = quaternion_mul(kf->base.q, qz);
																//kf->base.offset = quaternion_v3_mul_foward_inverse(qx, bone1->p);
																//kf->base.offset = quaternion_v3_mul_foward_inverse(qy, kf->base.offset);
																//kf->base.offset = quaternion_v3_mul_foward_inverse(qz, kf->base.offset);
															}
														}
#endif
													}
												}
												ui_text(ui, "transformed_p:");
												{
													model_bone *bone = model_editor->loaded_model_pose.bones + keyframe->bone_index;
													ui_textf(ui, "{%f, %f, %f}", bone->transformed_p.x, bone->transformed_p.y, bone->transformed_p.z);
												}

												ui_set_row(ui)
												{
													ui_text(ui, "parent");
													u8 *parent_name = "-";
													if(keyframe->switch_parent && 
													   keyframe->parent_index < model_editor->bone_count)
													{
														parent_name = model_editor->bone_name_chunks.chunks[
														keyframe->parent_index].name;
													}
													else
													{
														keyframe->switch_parent = 0;
													}
													ui_id drop_down_id = ui_id_from_string("dd_keyframe_bone_parent");
													ui_set_w_em(ui, 12.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
													if(ui_drop_down_beginf(ui, drop_down_id, "%s##dd_keyframe_bone_parent_bton", parent_name))
													{
														if(ui_selectable(ui, !keyframe->switch_parent,
																		"-##keyframe_parent_index_cancel"))
														{
															keyframe->switch_parent = 0;
														}
														for(u32 b = 0; b < model_editor->bone_count; b++)
														{
															if(ui_selectablef(ui,
																		keyframe->switch_parent && keyframe->parent_index == b,
																		"%s##keyframe_parent_index_at%u",
																		model_editor->bone_name_chunks.chunks[b].name, b))
															{
																keyframe->switch_parent = 1;
																keyframe->parent_index = b;
															}
														}
													}
													ui_drop_down_end(ui);
												}
												ui_set_row(ui)
												{
													ui_text(ui, "spline");
													//splines
													u8 *spline_names[model_animation_spline_total] = {
														"near",
														"linear",
														"smooth_in",
														"smooth_out"};
													u8 *spline_name = keyframe->spline >= model_animation_spline_total ? "OUT OF BOUNDS" : spline_names[keyframe->spline];
													ui_id keyframe_splines_dd_id = ui_id_from_string("keyframe_splines_dd");
													ui_set_w_em(ui, 8.0f, 1.0f)
														if(ui_drop_down_beginf(ui, keyframe_splines_dd_id, "%s##keyframe_splines", spline_name))
														{
															for(u32 s = 0; s < model_animation_spline_total; s++)
															{
																b32 active = keyframe->spline == s;
																if(ui_selectablef(ui, active, "%s##keyframe_spline%u", spline_names[s], s))
																{
																	keyframe->spline = s;
																	ui_popup_close(ui, keyframe_splines_dd_id);
																}
															}
														}
													ui_drop_down_end(ui);
												}
											}
											else if(keyframe->type == model_animation_keyframe_frame)
											{
												ui_text(ui, "sub_key properties:");
												if(keyframe->bone_index < model_editor->bone_count)
												{
													ui_text(ui, "frame_key");
													editor_model_bone *ekf_bone = model_editor->bones + keyframe->bone_index;
													model_bone *kf_bone = &ekf_bone->base;

													ui_checkbox(ui, &keyframe->flip_h, "flip_h#keyframe");
													ui_set_w_em(ui, 12.0f, 1.0f)
													{
														u8 *preview = "default";

														ui_drop_down_quick(ui, "frame_key_select_frame", preview) ui_set_h_text(ui, 4.0f, 1.0f)
														{
															if(ui_selectable(ui, !keyframe->frame_key, "default##kf_frame_key"))
															{
																keyframe->frame_key = 0;
															}

															for(u32 k = 0; k < ekf_bone->frame_key_count; k++)
															{
																editor_frame_key *fk = 0;
																memory_dyarray_get_safe(ekf_bone->frame_keys, fk, k);
																b32 selected = keyframe->frame_key == (k + 1);
																if(ui_selectablef(ui, selected, "%s##frame_extra_kf%u", fk->name, k))
																{
																	keyframe->frame_key = k + 1;
																}
															}
														}
													}
												}
												else
												{
													ui_text(ui, "Invalid node selected.");
												}
#if 0
												ui_text(ui, "frame_list_index");
												u32 max_list = model_editor->frame_list_count ? model_editor->frame_list_count - 1 : 0;
												editor_model_frame_list *frame_list = keyframe->frame_list_index < model_editor->frame_list_count ?
													model_editor->frame_lists + keyframe->frame_list_index : 0;
												u32 max_frames = frame_list ? frame_list->total_frames_count - 1 : 0;
												ui_spinner_u16(ui, 1, 0, max_list, &keyframe->frame_list_index, 0, "selected_keyframe_frame_list_index");
												ui_text(ui, "frame_list_frame");
												ui_spinner_u16(ui, 1, 0, max_frames, &keyframe->frame_list_frame_index, 0, "selected_keyframe_frame_list_frame_index");
#endif
											}
										}
									}
									//sub key properties
									ui_text(ui, "sub_key:");
									editor_animation_keyframe *sub_keyframe = 0;
									editor_animation_keyframe_row *sub_r = 0;
									if(model_editor->timeline_row_is_selected)
									{
										sub_r = editor_animation_get_row(editor_animation, model_editor->selected_timeline_row, 1);
									}
									if(sub_r)
									{
										sub_keyframe = editor_animation_get_keyframe_at_row(sub_r, model_editor->timeline_selected_frame);
									}
									
									ui_set_row(ui)
									{
										ui_set_disable_if(ui, sub_keyframe != 0)
										if(ui_button(ui, "+#sub_key"))
										{
											u32 i = model_editor->selected_timeline_row;
											u32 index = i;
											editor_animation_add_keyframe(game_editor,
													editor_animation,
													index,
													1,
													model_editor->timeline_selected_frame);
										}
										ui_space_specified(ui, 4.0f, 1.0f);

										ui_set_disable_if(ui, !sub_keyframe)
										if(ui_button(ui, "x#sub_key"))
										{
											editor_animation_remove_keyframe(
													game_editor, editor_animation, sub_keyframe);
										}
									}
									//display sub key properties
									if(sub_keyframe)
									{
										//change starting frame
										ui_set_row(ui)
										{
											ui_text(ui, "frame_start");
											ui_set_w_em(ui, 3.0f, 1.0f)
											{
												i32 frame = sub_keyframe->base.frame_start;
												ui_spinner_i32(ui, 1, 0, 1000, &frame, 0, "sub_keyframe_frame");
												if(frame != sub_keyframe->base.frame_start)
												{
													editor_animation_change_keyframe_frame(game_editor,
															editor_animation,
															sub_keyframe,
															frame);
												}
											}
										}
										//select avadible frame key
										u8 *preview = "default";
										editor_model_bone *bone = em_bone_from_keyframe(model_editor, sub_keyframe);
										//set display name if index is not 0
										if(sub_keyframe->base.frame_key && sub_keyframe->base.frame_key <= bone->frame_key_count)
										{
											editor_frame_key *fk = 0;
											memory_dyarray_get_safe(bone->frame_keys, fk, sub_keyframe->base.frame_key - 1);
											preview = fk->name;
										}
										ui_set_w_em(ui, 12.0f, 1.0f)
										ui_drop_down_quick(ui, "sub_keyframe_frame_key", preview)
										{
											if(ui_selectable(ui, 0, "default#sub_keyframe"))
											{
												sub_keyframe->base.frame_key = 0;
												ui_popup_close_last(ui);
											}
											//frame keys
											for(u32 f = 0; f < bone->frame_key_count; f++)
											{
												editor_frame_key *fk = 0;
												memory_dyarray_get_safe(bone->frame_keys, fk, f);
												if(ui_selectablef(ui, 0, "%s##sub_keyframe_frame_key_at%u", fk->name, f))
												{
													sub_keyframe->base.frame_key = f + 1;
													ui_popup_close_last(ui);
												}
											}
										}
										
									}
								}

							}
						}
						//timeline
						ui_node *timeline_node = 0;
						ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 160, 1.0f)
						{
							timeline_node = ui_create_node(ui, node_background | node_clickeable, 0);
						}
						ui_set_parent(ui, timeline_node)
						{
							ui_node *timeline_display = 0;
							//where keyframes go and uses scroll
							ui_node *timeline_display_content = 0;
							ui_node *playback_numbers = 0;
							ui_node *playback_numbers_display = 0;
							//bone and sprite names
							ui_node *timeline_display_names = 0;
							ui_node *timeline_top_display_node = 0;
							ui_node *timeline_top_interaction_node = 0;
							f32 playback_top_height = 40;
							f32 timeline_zoom = model_editor->timeline_zoom;
							model_editor->timeline_zoom = MAX(1.0f, model_editor->timeline_zoom);
							f32 timeline_advance = 20.0f * timeline_zoom;
							u32 frames_per_ms = editor_animation ? editor_animation->base.frames_per_ms :
								1;
							u32 lines_per_ms = 6 / frames_per_ms;
							f32 timeline_precision = 1.0f / lines_per_ms;
							f32 scroll_delta_x = 0;
							f32 scroll_delta_y = 0;
							f32 name_boxes_w = 120.0f;
							f32 mouse_at_timeline = 0;
							f32 mouse_at_timeline_time = 0;
							f32 timeline_scroll_x = 0;
							f32 draw_end_x = 0;
							ui_set_wh_soch(ui, 0.0f) ui_set_row_n(ui)
							{
									ui_set_h_ppct(ui, 1.0f, 0.0f) ui_set_w_specified(ui, name_boxes_w, 1.0f) ui_set_column_n(ui)
									{
										ui_set_w_specified(ui, name_boxes_w, 1.0f) ui_set_h_specified(ui, playback_top_height, 1.0f)
										{
											//buttons
											ui_set_column_n(ui)
											{
												ui_set_h_soch(ui, 1.0f) ui_set_row_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
												{
													ui_space_specified(ui, 8.0f, 1.0f);
													u8 *pr_preview = model_editor->reproduce_animation ? "||" : ">";
													if(ui_button(ui, "<<##reset_animation0"))
													{
														model_editor->timer.dt_transcurred = 0;
														model_editor->timer.dt_current = 0;
														model_editor->timer.frame_current = 0;
													}
													if(ui_buttonf(ui, "%s##pause_resume_animation", pr_preview))
													{
														model_editor->reproduce_animation = !model_editor->reproduce_animation;
													}
													ui_button(ui, ">>##got_to_end_animation");
													ui_space_specified(ui, 8.0f, 1.0f);
													if(ui_button(ui, "flip##flip_keyframes") && model_editor->timeline_frame_is_selected)
													{
														editor_animation_keyframe_column *column = editor_animation_get_column(
																editor_animation,
																model_editor->timeline_selected_frame);
														if(column)
														{
															for(editor_animation_keyframe *keyframe = column->first_keyframe;
																	keyframe; keyframe = keyframe->next_in_column)
															{
																//															keyframe->base.q.w *= -1;
																keyframe->base.q.x *= -1;
																keyframe->base.q.y *= -1;
																keyframe->base.q.z *= -1;
															}
														}
													}
												}
												ui_set_row(ui)
												{
													ui_set_w_em(ui, 7.0f, 1.0f)
													{
														ui_textf(ui, "zoom %f", timeline_zoom);
													}
													ui_set_w_ppct(ui, 1.0f, 0.0f)
													ui_slider_f32(ui, 1.0f, 4.0f, &model_editor->timeline_zoom, "timeline_zoom_slider");
												}
											}
										}
										//space where names go, only scroll on y and shares the value with the 
										//timeline keyframes contents node
										ui_set_w_specified(ui, name_boxes_w, 1.0f)
										{
											timeline_display_names = ui_create_node(ui, 
													node_scroll_y | node_border | node_clip, "timeline_display_names");
										}
									}
									//next column where the playback numbers display goes and the box with the
									//keyframes and scrolls
									ui_set_h_ppct(ui, 1.0f, 0.0f) ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_column_n(ui)
									{
										//lines and numbers
										ui_set_h_specified(ui, playback_top_height, 1.0f)
										{
											playback_numbers = ui_create_node(ui, node_clip | node_scroll_x | node_scroll_x_skip_bounds | node_clickeable, "playback_numbers");
											ui_set_parent(ui, playback_numbers) ui_set_wh_specified(ui, 0.0f, 0.0f)
											{
												playback_numbers_display = ui_create_node(ui, node_clickeable, "playback_numbers_interaction");
											}
										}
										//"floating node" to push some render stuff (but below keyframes)
										timeline_top_display_node = ui_create_node(ui, node_skip_layout_x | node_skip_layout_y, 0);
										ui_set_interaction_layer(ui, ui_interaction_layer_mid)
										{
											timeline_top_interaction_node = ui_create_node(ui, node_skip_layout_x | node_skip_layout_y | node_clickeable, "timeline_interaction_at_top");;
										}
										//node display and scroll bars go here.
										timeline_display = ui_create_node(ui,
												node_border, "timeline_display_keyframes_line");

									}

									timeline_scroll_x = (f32)(playback_numbers->region.x0 - playback_numbers_display->region.x0);
									mouse_at_timeline = ui->mouse_point.x - playback_numbers->region.x0;
									mouse_at_timeline_time = ((mouse_at_timeline + timeline_scroll_x) / (lines_per_ms * timeline_advance));

								//display numbers
#if 1
								 ui_set_parent_and_id(ui, timeline_display) ui_set_row(ui)
								{

									ui_node *column = 0;
									ui_set_wh_soch(ui, 0.0f)
									column = ui_create_node(ui, 0, "Column!");
									ui_set_parent(ui, column) ui_set_wh_ppct(ui, 1.0f, 0.0f)
									{
										ui_set_wh_ppct(ui, 1.0f, 0.0f)
										{
											timeline_display_content = ui_create_node(ui,
													node_scroll_x | node_scroll_y | node_border | node_clip,
													"timeline_display_content");
										}
										ui_set_h_specified(ui, 10.0f, 1.0f) ui_set_w_ppct(ui, 1.0f, 1.0f)
										{
											scroll_delta_x = ui_scroll_horizontal(ui,
													timeline_display_content->size_x,
													timeline_display_content->content_size[ui_axis_x],
													timeline_display_content->scroll_x);
										}
										//create horizontal scroll
									}

									//scroll vertical
									ui_set_w_specified(ui, 10.0f, 1.0f) ui_set_h_ppct(ui, 1.0f, 1.0f)
									{
										scroll_delta_y = ui_scroll_vertical(ui, 
												timeline_display_content->size_y,
												timeline_display_content->content_size[ui_axis_y],
												timeline_display_content->scroll_y);
									}
									//show timeline
									ui_set_parent(ui, timeline_display_content)
									{
										f32 timeline_selectable_h = 14.0f;
										

										//show sprite list
										u32 _counts[2] = {
											model_editor->bone_count,
											model_editor->bone_count};
										vec4 _bk_colors[2] = {
											V4(0, 200, 200, 160), V4(0, 200, 0, 160)};
										editor_name_chunks _names[2] = {
										model_editor->bone_name_chunks,
										model_editor->sprite_name_chunks};
										f32 keyframe_wh = 12.0f;
										f32 sub_keyframe_h = 8.0f;
										u32 group_index = 0;
										for(u32 c = 0; c < 1; c++)
										for(u32 b = 0; b < _counts[c]; b++)
										{
											ui_set_w_soch(ui, 1.0f) ui_set_h_specified(ui, 20.0f, 1.0f)
											{
												//si quisiera añadir un border, puedo ponerlo en timeline_top_interaction porque cubre toda el área
												ui_node *timeline_box = ui_create_node(ui, 0, 0);
												ui_node *timeline_top_interaction  = 0;
												ui_set_parent(ui, timeline_box) ui_set_w_ppct(ui, 1.0f, 0.0f)
												{
													timeline_top_interaction = ui_create_nodef(ui, node_skip_layout_x | node_skip_layout_y | node_clickeable | node_border, "tl_box_%u%u", c, b);
												}
												//interacted for group and keyframe selection
												b32 interacted = ui_node_mouse_l_up(ui, timeline_top_interaction);

												editor_animation_keyframe_row *group = 0;
												editor_animation_keyframe_row *sub_group = 0;
												if(editor_animation)
												{
													group = editor_animation_get_row(editor_animation, b, 0);
													sub_group = editor_animation_get_row(editor_animation, b, 1);
												}
												//if clicked on this row, select timeline group and check avadible group
												if(interacted)
												{
													model_editor->selected_timeline_row = group_index;
													model_editor->selected_row = group;
													model_editor->row_is_selected = group != 0;
													model_editor->keyframe_is_selected = 0;
													model_editor->timeline_row_is_selected = 1;

													editor_model_select_frame(
															game_editor,
															editor_animation,
															(u32)mouse_at_timeline_time);
													if(!c)
													{
														model_editor->animation_bone_is_selected = 1;
														model_editor->animation_selected_bone_index = b;
													}
													else
													{
														model_editor->animation_bone_is_selected = 0;
													}
												}
												vec4 background_color = _bk_colors[0];
												//set alpha if selected
												if(model_editor->timeline_row_is_selected && 
												   model_editor->selected_timeline_row == group_index)
												{
													background_color.a = 255;
												}
												//ui_set_parent(ui, timeline_box) ui_set_row(ui)
												//{
												//}
												//select this group if clicked

												//display names
												ui_set_w_specified(ui, name_boxes_w, 1.0f) ui_set_parent(ui, timeline_display_names)
												{
														ui_push_color(ui, ui_color_background, background_color);
														ui_node *text_node = ui_create_node(ui, node_background | node_text | node_border, 0);
														ui_pop_color(ui, ui_color_background);
														ui_node_set_display_string(ui, text_node, _names[c].chunks[b].name);
												}
												//get and display keyframes
												if(group) ui_set_parent(ui, timeline_box)
												{
													u32 k = 0;
													ui_set_row(ui) ui_set_wh_specified(ui, keyframe_wh, 1.0f)
													for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
															editor_keyframe;
															editor_keyframe = editor_keyframe->next_in_row)
													{
														model_animation_keyframe *keyframe = &editor_keyframe->base;
														u16 frame_at = (u16)(keyframe->frame_start * timeline_advance * lines_per_ms);
														ui_node *keyframe_node = ui_create_nodef(ui, node_skip_layout_x | node_clickeable, "timeline_keyframe%u%u%u", c, b, k);
														keyframe_node->added_x = frame_at;
														ui_space_specified(ui, timeline_advance * lines_per_ms, 1.0f);
														vec4 color = V4(255, 255, 0, 160);

														//draw if not outside bounds
														if(keyframe_node->region.x1 > playback_numbers->region.x0 &&
														   keyframe_node->region.x0 < playback_numbers->region.x1)
														{
															if(model_editor->selected_timeline_row == group_index &&
																	model_editor->keyframe_is_selected && 
																	model_editor->selected_keyframe == editor_keyframe)
															{
																color.a = 255;
															}

															ui_node_push_rectangle(
																	ui, keyframe_node, 0, 0,  12, 12, color);
														}
														//select keyframe
														if(ui_node_mouse_l_up(ui, keyframe_node))
														{
															model_editor->selected_row = group;
															model_editor->row_is_selected = 1;
															model_editor->selected_timeline_row = group_index;
															model_editor->keyframe_is_selected = 1;
															model_editor->selected_keyframe = editor_keyframe;
															model_editor->timeline_row_is_selected = 1;
															//used for dragging
															model_editor->focused_on_keyframe_only = 1;

															model_editor->ui_focus = ea_focus_timeline;
//															model_editor->timeline_selected_frame = (u16)mouse_at_timeline_time;
//															model_editor->timeline_frame_is_selected = 1;
															editor_model_select_frame(
																	game_editor,
																	editor_animation,
																	(u32)mouse_at_timeline_time);
															if(!c)
															{
																model_editor->animation_bone_is_selected = 1;
																model_editor->animation_selected_bone_index = b;
															}
															else
															{
																model_editor->animation_bone_is_selected = 0;
															}
														}
														if(interacted && model_editor->timeline_selected_frame == keyframe->frame_start)
														{
															model_editor->keyframe_is_selected = 1;
															model_editor->selected_keyframe = editor_keyframe;
															model_editor->ui_focus = ea_focus_timeline;
														}

														k++;
													}
												}
											}
											group_index++;
										}
										//show sprite list
									}

									//check interactions

									//check scroll
									if(scroll_delta_y)
									{
										ui_node_set_target_scroll(
												timeline_display_content,
												scroll_delta_y,
												ui_axis_y);

										ui_node_set_target_scroll(
												timeline_display_names,
												scroll_delta_y,
												ui_axis_y);
									}
									if(scroll_delta_x)
									{
										ui_node_change_target_scroll(
												timeline_display_content,
												scroll_delta_x,
												ui_axis_x);
										ui_node_change_target_scroll(
												playback_numbers,
												scroll_delta_x,
												ui_axis_x);
										f32 scx = playback_numbers->target_scroll[ui_axis_x];
										f32 content_outside_bounds_x = (f32)(timeline_display_content->content_size[ui_axis_x] - timeline_display_content->size[ui_axis_x]);
										playback_numbers->target_scroll[ui_axis_x] = scx < 0 ? 0 : scx; 
										if(content_outside_bounds_x > 0)
										{
											playback_numbers->target_scroll[ui_axis_x] = MIN(playback_numbers->target_scroll[ui_axis_x], content_outside_bounds_x);
										}
										else
										{
											playback_numbers->target_scroll[ui_axis_x] = 0; 
										}
									}
									//hovers top
									ui_usri playback_interaction = ui_usri_from_node(ui, playback_numbers);
									b32 hovers_playback_top = ui_usri_mouse_hover(playback_interaction);
									b32 playback_top_mouse_l_down = ui_usri_mouse_l_down(playback_interaction);
									if(playback_top_mouse_l_down)
									{
										f32 time = mouse_at_timeline_time;
										time = time < 0 ? 0 : time;
										//model_editor->timeline_selected_frame = (u16)time;
										//model_editor->timeline_frame_is_selected = 1;
										editor_model_select_frame(
												game_editor,
												editor_animation,
												(u32)mouse_at_timeline_time);
										model_editor->timer.dt_transcurred = time / 10.0f;
										//pause timer
										model_editor->reproduce_animation = 0;
										model_editor->selected_keyframe = 0;
										if(model_editor->row_is_selected)
										{
											editor_animation_keyframe_row *group = model_editor->selected_row;
											b32 found = 0;
											editor_animation_keyframe *keyframe_to_select = 0;
											u32 frame = model_editor->timeline_selected_frame;

											for(editor_animation_keyframe *editor_keyframe = group->first_keyframe;
													!keyframe_to_select && editor_keyframe;
													editor_keyframe = editor_keyframe->next_in_row)
											{
												model_animation_keyframe *keyframe = &editor_keyframe->base; 
												//found keyframe!
												if(keyframe->frame_start == frame)
												{
													keyframe_to_select = editor_keyframe;
												}
											}
											model_editor->selected_keyframe = keyframe_to_select;
										}
										model_editor->keyframe_is_selected = model_editor->selected_keyframe != 0;
									}


									//Drawing


									//draw numbers

									f32 playback_draw_at = (f32)(playback_numbers->region.x0 - playback_numbers_display->region.x0);
									f32 sdt = 0; 
									f32 duration_h = 9.0f;
									f32 time = 0.0f;
									f32 timeline_scale = (lines_per_ms * timeline_advance);
									u32 frame_to_start = (u32)(playback_draw_at / timeline_advance);
									u32 frame_to_end = (u32)(frame_to_start + (playback_numbers->size[ui_axis_x] / timeline_advance));

									f32 line_x = -playback_draw_at + ((u32)(frame_to_start / lines_per_ms) * timeline_scale);
									f32 line_y = duration_h;
									u32 lines_count = 0;
									u32 time_frame = (u32)(frame_to_start / lines_per_ms);
									for(u32 n = frame_to_start; n < frame_to_end + (1 * lines_per_ms); n++)
									{
										time = (f32)time_frame / 10.0f;//n * timeline_precision;//(f32)lines_per_ms / 100.0f;
										f32 line_size = 10.0f;
										if(!lines_count)
										{
											line_size = 20.0f;
											time_frame++;

											ui_node_push_textf(ui,
													playback_numbers,
													line_x,
													(line_y + line_size),
													0,
													vec4_all(255),
													"%.2f", time);
										}
										ui_node_push_rectangle(ui,
												playback_numbers,
												(i16)line_x,
												(i16)duration_h,
												(i16)line_x + 2,
												(i16)(line_y +line_size),
												vec4_all(255));
										line_x += timeline_advance;
										lines_count++;
										lines_count %= lines_per_ms;
									}

									//display the animation duration at the top
									i16 playback_numbers_end = animation ? 
										(i16)(animation->frames_total * lines_per_ms * timeline_advance) : 0;
									ui_node_push_rectangle(ui,
											playback_numbers_display,
											(i16)0,
											(i16)0,
											(i16)playback_numbers_end,
											(i16)duration_h,
											V4(0, 255, 255, 200));

									//cursor to display the current time
									{
										i16 playback_line_at = (i16)(model_editor->timer.dt_transcurred * lines_per_ms * 10 * timeline_advance - timeline_scroll_x);
										ui_node_push_rectangle(ui,
												playback_numbers,
												(i16)playback_line_at,
												(i16)0,
												(i16)playback_line_at + 2,
												(i16)40,
												V4(0, 255, 255, 255));
									}
									//cursor to display selection
									if(hovers_playback_top)
									{
										ui_node_push_rectangle(ui,
												playback_numbers,
												(i16)mouse_at_timeline,
												(i16)0,
												(i16)mouse_at_timeline + 2,
												(i16)40,
												V4(0, 255, 255, 255));
									}
									//render timeline columns
									if(editor_animation)
									{
										i32 cube_size = 16;
										u32 index = 0;
										ui_set_parent(ui,timeline_top_display_node)
										for(editor_animation_keyframe_column *column = editor_animation->first_column;
												column;
												column = column->next)
										{
											//create a node to select this column
											//only render if it actually has something
											if(column->first_keyframe)
											{
												u32 selected_frame = model_editor->timeline_selected_frame;
												f32 x0 = (f32)(i32)(column->frame * timeline_advance * lines_per_ms - timeline_scroll_x);
												f32 x1 = (f32)(i32)(x0 + cube_size);
												f32 y0 = playback_numbers->size_y - (f32)cube_size; 
												f32 y1 = y0 + cube_size; 
												if(x1 >= 0)
												{
													ui_push_wh_specified(ui, cube_size, 1.0f);
													ui_node *column_node = ui_create_nodef(ui, node_skip_layout_x | node_skip_layout_y | node_clickeable, "timeline_time_column%u", index);
													ui_pop_wh(ui);
													column_node->added_x = (u16)x0;
													column_node->added_y = (u16)y0;
													vec4 color = {160, 160, 160, 160};
													if(model_editor->selected_column == column)
													{
														color.a = 255;
														color.r = 200;
														color.g = 200;
														color.b = 200;
													}
													ui_node_push_rectangle(ui,
															column_node,
															0,
															0,
															cube_size,
														    cube_size,
															color);
													//interact, select and drag
													if(ui_node_mouse_l_down(ui, column_node))
													{
														model_editor->focused_on_keyframe_only = 0;
														i32 frame_to_drag = (i32)mouse_at_timeline_time;
														u32 old_frame = column->frame;
														if(frame_to_drag != column->frame)
														{
															b32 found_frame = 1;
															for(editor_animation_keyframe_column *other_column = editor_animation->first_column;
																	other_column;
																	other_column = other_column->next)
															{
																if(other_column == column) continue;
																if(other_column->frame == frame_to_drag)
																{
																	if(!other_column->first_keyframe )
																	{
																		other_column->first_keyframe = column->first_keyframe;
																		column->first_keyframe = 0;
																		found_frame = 1;
																		model_editor->selected_column = other_column;
																		column = other_column;
																		found_frame = 1;
																		break;
																	}
																	else
																	{
																		found_frame = 0;
																	}
																}
																else
																{
																	found_frame = 1;
																}
															}
															if(found_frame)
															{
																column->frame = frame_to_drag;
																for(editor_animation_keyframe *editor_keyframe = column->first_keyframe;
																		editor_keyframe;
																		editor_keyframe = editor_keyframe->next_in_column)
																{
																	model_animation_keyframe *kf = &editor_keyframe->base;
																	kf->frame_start = column->frame;
																	editor_animation_add_keyframe_to_row(
																			editor_keyframe->row,
																			editor_keyframe,
																			column->frame);

																}
															}
														}
														editor_model_select_frame(game_editor, editor_animation, column->frame);
													}
												}
											}
											index++;
										}
									}

									//render the current selected frame
									if(model_editor->timeline_frame_is_selected)
									{
										u32 selected_frame = model_editor->timeline_selected_frame;
										u32 x0 = (u32)(selected_frame * timeline_advance * lines_per_ms - timeline_scroll_x);
										u32 x1 = (u32)(x0 + (timeline_advance * lines_per_ms));
										ui_node_push_rectangle(ui,
												timeline_top_display_node,
												x0,
												0,
												x1,
												(i16)200,
												V4(0, 100, 100, 100));
									}
									//used to test scrolling
									//add and invisible widget depending on the duration
									ui_set_parent(ui, timeline_display_content)
									{
										u32 w = editor_animation ? 
											(u32)(editor_animation->base.frames_total * (lines_per_ms * timeline_advance) * 3) : 0;
										ui_set_h_specified(ui, 0.0f, 1.0f) ui_set_w_specified(ui, w, 1.0f)
										{
											ui_create_node(ui, 0, 0);
										}
									}

									if(ui_node_mouse_l_pressed(ui, timeline_top_interaction_node))
									{
										model_editor->ui_focus = ea_focus_timeline;
									}
								//	ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
								//	{
								//		for(u32 d = 0; d < 40; d++) ui_set_wh_text(ui, 4.0f, 1.0f)
								//		{
								//			ui_textf(ui, "Text! %u", d);
								//		}
								//	}
								}
								 //node on top for interactions
#endif
								
							}
						}

#endif
					}break;
				default:
					{
						ui_space_ppct(ui, 1.0f, 0.0f);
					}
					break;
			}

			ui_pop_parent(ui, panels_node);

		}
	}


	//
	// Animation editor
	//

		ui_node *name_bottom_bar;
		ui_node *bottom_bar;
		ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
		{
			name_bottom_bar = ui_create_node(ui, node_background, 0);
			bottom_bar = ui_create_node(ui, node_background, 0);
		}
		ui_set_parent(ui, name_bottom_bar)
		{
			if(model_editor->editing_model)
			{
				ui_text(ui, model_editor->editing_model->path_and_name);
			}
		}
		ui_set_parent(ui, bottom_bar) ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
		{
			u8 *modes[] = {
			"-- View --",
			"-- Draw --",
			"-- Selection --",
			"-- Nodes --",
			"-- Sprite editing --",
			"-- Properties --",
			"-- Animation --"
			};
			i32 modes_count = ARRAYCOUNT(modes);
			u8 *mode = model_editor->tool > modes_count ?
				"Undefined" : modes[model_editor->tool];
			ui_text(ui, mode);
		}


		//post ui stuff
		if(remove_sprite)
		{
			editor_model_remove_sprite(game_editor, model_editor->selected_sprite_index);
		}

	//cursor values tab
	if(ui_window_begin(
				ui,
				0,
				60,
				game_renderer->os_window_height - 200 - 60.0f,
				200,
				200,
				"Cursor properties"))
	{
		ui_text(ui, "Cursor displacement");
		ui_spinner_f32(ui, 0.1f, 0.1f, 96, &game_editor->model.cursor.tile_displacement, 0, "selection size h");
//		ui_checkbox(ui, &game_editor->model.display_edges,"Display edges");

		ui_selectable_directions_vec2(
				ui,
				32,
				&game_editor->model.model_foward);
	}
	ui_panel_end(ui);

	if(model_editor->tool == model_mode_nodes)
	{
		if(add_bone)
		{
			editor_model_add_bone(game_editor);
		}
		//Delete bone
		else if(remove_bone)
		{
			editor_remove_bone(game_editor, game_editor->model.selected_bone_index);
		}
	}


	if(newAnimation)
	{
	}

	if(new_clicked)
	{
		editor_model_reset(game_editor);
	}
	else if(save_clicked && model_editor->editing_model)
	{
		editor_model_save_new(editor_state);
	}
	else if(save_as_clicked || (save_clicked && !model_editor->editing_model))
	{
		er_explorer_set_process(game_editor,
				er_explorer_save,
				"Save model");
	}
	else if(load_clicked)
	{
		er_explorer_set_process(game_editor,
				er_explorer_load,
				"Load model");
	}

	if(open_load_sheet_explorer)
	{
		er_explorer_set_process(game_editor,
				er_explorer_load,
				"Add spritesheet to model");

	}



	if(editor_resource_explorer_process_completed(game_editor, "Add spritesheet to model"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		editor_model_add_texture(game_editor,
				er_look_for_resource(game_editor, name));
	}

	if(editor_resource_explorer_process_completed(game_editor, "Save model"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		game_resource_attributes *resource = editor_resource_create_and_save(
				editor_state, asset_type_model, 1, name);
		if(resource)
		{
			model_editor->editing_model = resource;
			editor_model_save_new(editor_state);
			editor_resources_reimport(editor_state, resource);
		}
	}
	if(editor_resource_explorer_process_completed(game_editor, "Load model"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		//Reset timer
		game_editor->tipTimer = 0;
		editor_model_load(
				editor_state, er_look_for_resource(game_editor, name));
	}
	if(editor_resource_explorer_process_completed(game_editor, "Select model to attach"))
	{
		u8 *name = editor_resource_explorer_output(game_editor); 
		model_editor->attachments[model_editor->attachment_to_model].model = er_look_for_resource(game_editor, name);
	}

	if(ui_explorer_check_process(ui, "Save animation"))
	{

	}
	if(ui_explorer_check_process(ui, "Load animation"))
	{
		u8 *animationPathAndName = ui->explorer->full_process_path_and_name;
		//editor_animation_loadtemp(game_editor, editor_state->editor_assets, editor_state->platform, animationPathAndName); 
		//Reset timer
		game_editor->tipTimer = 0;
	}

	editor_graphics_db_panel(editor_state, game_renderer, editor_input);

}

