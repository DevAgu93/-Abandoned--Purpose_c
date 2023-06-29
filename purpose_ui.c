#include "purpose_ui_render.h"
#define ui_INPUT_CURSOR_TIMER_TOTAL (0.1f * 20.0f)

#define ui_COMMANDID(ui) ui_id_specified(0, 3)
#define ui_id_READONLY ui_id_specified(0, 1)
#define ui_id_ZERO ui_id_specified(0, 0)

#define ui_id_NUMBER(n) ui_id_from_number(n)

#define ui_id_EQUALS(id1, id2) ui_id_equals(id1, id2) 


#define ui_element_interacting(ui, elId) ui_id_equals(ui->node_interacting, elId)
#define ui_element_interacted(ui, elId) ui_id_equals(ui->node_last_interact, elId)

#define ui_element_interacting_flags(ui, elementId, flags) (ui_element_interacting(ui, elementId) && (ui->interacting_flags & (flags)))
#define ui_element_interacted_flags(ui, elementId, flags) (ui_element_interacted(ui, elementId) && (ui->interacted_flags & (flags)))

#define ui_FontHeight(ui) (ui->fontp.font_height * ui->font_scale)

#define ui_push_clip_inside_last_XYWH(ui, x, y, w, h) ui_push_clip_inside_last(ui, x, y, (x + w), (y + h))

#define ui_ScaleHeight(ui) (ui->renderCommands->projection.m[1][1]);
#define ui_ScaleWidth(ui) (ui->renderCommands->projection.m[0][0]);

#define ui_disabled(ui) (ui->next_node_disabled)

#define ui_SZ_SCROLL_WH 14


#define ui_mouse_delta(ui) (V2(ui->mouse_point.x - ui->mouse_point_last.x,\
			ui->mouse_point.y - ui->mouse_point_last.y))
#define ui_mouse_delta_from_node(ui, node) (\
		V2(ui->mouse_point.x - node->region.x0,\
			ui->mouse_point.y - node->region.y0))

#define ui_mouse_hold_delta_from_node(ui, node) (\
		V2(ui->mouse_point_hold.x - node->region.x0,\
			ui->mouse_point_hold.y - node->region.y0))

#define ui_mouse_hold_delta(ui) (\
		V2(ui->mouse_point.x - ui->mouse_point_hold.x,\
			ui->mouse_point.y - ui->mouse_point_hold.y))

#define ui_ANYF(text_buffer, tb_size, text)\
	va_list args;\
	va_start_m(args, text);\
	format_text_list(text_buffer, tb_size, text, args);\
	va_end_m(args);

#define ui_node_b(ui, node) V2I16(node->x0, node->y1)
#define ui_DEFER_LOOP(start, end) for(int __dfl__= ((start), 0);\
		!__dfl__;\
		__dfl__ += 1, (end))
#define ui_DEFER_CONDITION(begin, end)\
	for(u8 __I__ = (u8)(begin);\
		   __I__ == 1 || (!__I__ ? (end) : 0);\
		   __I__ += 1, (end))

#define ui_current_background_color(ui) (ui->theme_colors[ui_color_background][ui->theme_colors_indices[ui_color_background]].color)

typedef struct{
	int dfl;
	ui_node *n;
}ui_dfs;

#define ui_any_interaction(ui) (ui_any_node_hot(ui) || ui_any_node_interacting(ui))

static void
ui_inc_dec_f32(
		f32 *value, f32 inc_dec, f32 min_value, f32 max_value)
{

	f32 value_before = *value;
	f32 v = inc_dec;
	f32 va = ABS(v);
	if(v > 0 && f32_Add_OVERFLOWS(*value, va))
	{
		*value = F32MAX;
	}
	else if(v < 0 && f32_Sub_UNDERFLOWS(*value, va))
	{
		*value = F32MIN;
	}
	else
	{
		*value += v;
	}

	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);
}

static void
ui_inc_dec_i32(
		i32 *value, i32 inc_dec, i32 min_value, i32 max_value)
{

	i32 value_before = *value;
	i32 v = inc_dec;
	i32 va = ABS(v);
	if(v > 0 && i32_Add_OVERFLOWS(*value, va))
	{
		*value = I32MAX;
	}
	else if(v < 0 && i32_Sub_UNDERFLOWS(*value, va))
	{
		*value = I32MIN;
	}
	else
	{
		*value += v;
	}

	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);
}

static inline void
ui_clear_input_text_buffer(game_ui *ui)
{
	memory_clear(ui->input_text_buffer, sizeof(ui->input_text_buffer));
}
	static inline void
ui_keep_current_interaction(game_ui *ui)
{
	ui->keep_interaction = 1;
	ui->keep_interaction_countdown = 1;
}

#define ui_interaction_only(ui) ui_DEFER_LOOP(ui_next_nodes_interaction_only_begin(ui), ui_next_nodes_interaction_only_end(ui))
	static inline void
ui_next_nodes_interaction_only_begin(game_ui *ui)
{
	ui->next_node_readonly_countdown = 1;
	ui->next_node_readonly = 0;
}

	static inline void
ui_next_nodes_interaction_only_end(game_ui *ui)
{
	ui->next_node_readonly = ui->next_node_readonly_countdown >= 1;
}

#define ui_ignore_interaction_only(ui) ui_DEFER_LOOP(\
		ui_ignore_interaction_only_begin(ui), ui_ignore_interaction_only_end(ui))
	static inline void
ui_ignore_interaction_only_begin(game_ui *ui)
{
	ui->next_node_readonly = 0;
}
	static inline void
ui_ignore_interaction_only_end(game_ui *ui)
{
	ui->next_node_readonly = ui->next_node_readonly_countdown > 0;
}

//custom commands
	static void
ui_push_set_value_command(game_ui *ui, f32 *value_to_set, f32 set_value)
{
	ui_command_node command = {0};
	command.value_to_set = value_to_set;
	command.set_value = set_value;
	command.type = 0;

	ui->command_nodes[ui->command_node_count++] = command;
	Assert(ui->command_node_count < ui->command_node_max);
}

	static void
ui_push_add_value_command(game_ui *ui, f32 *value_to_set, f32 add_value)
{
	ui_command_node command = {0};
	command.value_to_set = value_to_set;
	command.set_value = add_value;
	command.type = 1;

	ui->command_nodes[ui->command_node_count++] = command;
	Assert(ui->command_node_count < ui->command_node_max);
}

	static inline void
ui_read_command_nodes(game_ui *ui)
{
	for(u32 c = 0;
			c < ui->command_node_count;
			c++)
	{
		ui_command_node *command = ui->command_nodes + c;
		switch(command->type)
		{
			case 0:
				{
					(*command->value_to_set) = command->set_value;
				}break;
			case 1:
				{
					(*command->value_to_set) += command->set_value;
				}break;
		}
	}
	ui->command_node_count = 0;
}

static inline u32
ui_hash_u32(u32 h, u32 seed) {
	h ^= h >> 16;
	h += seed;
	h *= 0x3243f6a9U;
	h ^= h >> 16;
	return(h);
}

	inline u32
ui_kinda_hash(u8 *text, u32 seed)
{
	u8 c = 0;
	u32 i = 0;
	u32 kindaHash = 0;
	while((c = text[i++]) != '\0')
	{
		u32 c32 = (u32)c;
		kindaHash += noise_u32(c32, i + seed);
	}

	return(kindaHash);
}

	static inline ui_id
ui_id_from_string(u8 *string)
{
	ui_id result = {0};
	if(string)
	{
		result.value0 = ui_kinda_hash(string, 0);
		result.value1 = result.value0; 
	}
	return(result);
}

//hashes a new id and reserves value1
//for values on the id stack
	static inline u32
ui_hash_offset(u8 *string)
{
	u8 c = 0;
	u32 i = 0;
	u32 offset = 0;
	while((c = string[i]) != '\0')
	{
		if(c == '#' && string[i + 1] == '#')
		{
			offset = i;
			break;
		}
		i++;
	}
	return(offset);
}

#define ui_popup_id ui_node_id_from_string
static inline ui_id
ui_node_id_from_string(game_ui *ui, u8 *string)
{
	ui_id result = {0};
	if(string)
	{
		u32 final_value = 0;
		ui_id_stack_slot *stack_slot = ui->id_stack_last;
		while(stack_slot)
		{
			final_value = ui_hash_u32(stack_slot->id, final_value);
			stack_slot = stack_slot->next;
		}
		result.value0 = ui_kinda_hash(string + ui_hash_offset(string), 0);
		result.value1 = final_value;
	}
	return(result);
}

#define ui_push_id_node(ui, node) ui_push_id_u32(ui, node->id.value1)
#define ui_push_id(ui, id) ui_push_id_u32(ui, id.value0)
#define ui_push_id_string(ui, id) ui_push_id_u32(ui, ui_kinda_hash(id, 0))

	static inline void
ui_push_id_u32(game_ui *ui, u32 id)
{
	//TODO: in case of overflow, push to remaining.
	ui_id_stack_slot *sl = ui->id_stack + ui->id_stack_count;

	sl->id = id;
	sl->next = ui->id_stack_last;

	ui->id_stack_last = sl;
	ui->id_stack_count++;

	Assert(ui->id_stack_count < ui->id_stack_max);
}

	static inline void
ui_pop_id(game_ui *ui)
{
	if(ui->id_stack_count)
	{
		ui->id_stack_count--;
		ui->id_stack_last = ui->id_stack_last->next;
	}
}

	static inline ui_id
ui_id_from_stringf(game_ui *ui, u8 *string, ...)
{
	u8 buffer[256] = {0};
	va_list args;
	va_start_m(args, string);
	format_text_list(buffer, sizeof(buffer), string, args);
	va_end_m(args);

	ui_id result = ui_node_id_from_string(ui, buffer);
	return(result);
}

	static inline ui_id
ui_id_from_number(u32 v)
{
	ui_id result = {0};
	result.value0 = v; 
	return(result);
}

	static inline ui_id
ui_id_specified(u32 v0, u32 v1)
{
	ui_id result = {0};
	result.value0 = v0;
	result.value1 = v1;
	return(result);
}

	static inline ui_node_size
ui_default_node_size()
{
	ui_node_size result;

	result.type = size_null;
	result.size_strictness = 0;
	result.amount = 0;

	return(result);
}

#define ui_em(ui)\
	font_em(&ui->fontp, ui->font_scale)
#define ui_size_em(ui, amount, strictness)\
	ui_size_specified(ui_em(ui) * (amount), strictness)
	static inline ui_node_size
ui_default_node_w(game_ui *ui)
{
	ui_node_size result;

	result.type = size_specified;
	result.size_strictness = 1.0f;
	result.amount = font_em(&ui->fontp, ui->font_scale) * 20;

	return(result);
}


	static inline ui_node_size
ui_default_node_h(game_ui *ui)
{
	ui_node_size result;

	result.type = size_specified;
	result.size_strictness = 1.0f;
	result.amount = font_em(&ui->fontp, ui->font_scale) * 5;

	return(result);
}

#define ui_set_interaction_layer(ui, layer) ui_DEFER_LOOP(ui_next_interaction(ui, layer), ui_prev_interaction(ui, ui_interaction_layer_default))

	static inline void
ui_next_interaction(game_ui *ui, ui_interaction_layer layer)
{
	ui->interaction_layer = layer;
}
	static inline void
ui_prev_interaction(game_ui *ui, ui_interaction_layer layer)
{
	ui->interaction_layer = layer;
}

	static inline ui_per_frame_settings
ui_platform_initialize(
		memory_area *program_memory,
		memory_size allocation_size,
		platform_api *platform,
		platform_renderer *platform_renderer,
		platform_renderer_init_functions *renderer_initializing_functions,
		stream_data *info_stream)
{
	ui_per_frame_settings initialization_settings = {0};
	//allocation size can be zero, not while I use it!
	initialization_settings.ui_area = memory_area_create_from(
			program_memory, allocation_size);

	//allocate a display buffer if avadible.
	if(renderer_initializing_functions && 
			renderer_initializing_functions->f_allocate_frame_buffer)
	{
		initialization_settings.display_buffer_allocated = 1;
		initialization_settings.display_buffer_index = renderer_initializing_functions->f_allocate_frame_buffer(
				platform_renderer,
				1600,
				900);
	}
	else
	{
		stream_pushf(info_stream,
				"No display buffer allocation function was allowed, so the"
				"ui will be displayed on the current display buffer avadible.");
	}
}

	inline void
ui_push_clip_inside_last(game_ui *ui, i32 x0, i32 y0 , i32 x1, i32 y1)
{

	matrix4x4 uiMatrix = ui->projection;

	vec3 x0y0 = {(f32)x0, (f32)y0, 1};
	vec3 x1y1 = {(f32)x1, (f32)y1, 1};
	x0y0 = matrix4x4_v3_mul_rows(uiMatrix, x0y0, 0);
	x1y1 = matrix4x4_v3_mul_rows(uiMatrix, x1y1, 0);

	x0 = (i32)x0y0.x;
	y0 = (i32)x0y0.y;
	x1 = (i32)x1y1.x;
	y1 = (i32)x1y1.y;
	if(x1 > 1680)
	{
		int s = 0;
	}
	//render_commands_PushClipInsideLast_GameCoords(ui->renderCommands, (i32)x0, (i32)y0, (i32)(x1), (i32)(y1));
	render_commands_push_clip_inside_last(ui->renderCommands, (i32)x0, (i32)y0, (i32)(x1), (i32)(y1));
}

#define ui_DISABLED_ALPHA 120
ui_render_text(game_ui *ui,
		i32 x0,
		i32 y0,
		i32 x1,
		i32 y1,
		vec4 color,
		u8 *text)
{
	if(text)
	{
		render_text_2d(
				ui->renderCommands,
				&ui->fontp,
				(f32)x0,
				(f32)y0,
				(f32)x1,
				(f32)y1,
				ui->font_scale,
				color,
				text);
	}

}

#define ui_mouse_inside_rec_clipped_xywh(ui, x, y, w, h) ui_mouse_inside_rec_clipped(ui, x, y, (x + w), (y + h))
	inline i32
ui_mouse_inside_rec_clipped(game_ui *ui, real32 x0, real32 y0, real32 x1, real32 y1)
{
	render_commands *renderCommands = ui->renderCommands;
	vec2 mouse_position              = ui->mouse_point;
	//if(ui->flags & ui_flags_DisplayScaled)
	//{
#if 1
	//Scale coordinates to correctly detect positions inside clip.
	matrix4x4 uiMatrix = ui->projection;
	f32 scaleX = uiMatrix.m[0][0];
	f32 scaleY = uiMatrix.m[1][1];

	x0 *= scaleX;
	y0 *= scaleY;
	x1 *= scaleX;
	y1 *= scaleY;

	mouse_position.x *= scaleX;
	mouse_position.y *= scaleY;
#endif
	return(render_mouse_over_rec_clipped(renderCommands, mouse_position, x0, y0, x1, y1));
	//}

}

#define ui_mouse_inside_rec_xywh(ui, x, y, w, h) ui_mouse_inside_rec_clipped(ui, x, y, (x + w), (y + h))
	inline i32
ui_mouse_inside_rec(game_ui *ui, real32 x0, real32 y0, real32 x1, real32 y1)
{
	render_commands *renderCommands = ui->renderCommands;
	vec2 mouse_position              = ui->mouse_point;
	//if(ui->flags & ui_flags_DisplayScaled)
	//{
#if 1
	//Scale coordinates to correctly detect positions inside clip.
	matrix4x4 uiMatrix = ui->projection;
	f32 scaleX = uiMatrix.m[0][0];
	f32 scaleY = uiMatrix.m[1][1];

	x0 *= scaleX;
	y0 *= scaleY;
	x1 *= scaleX;
	y1 *= scaleY;

	mouse_position.x *= scaleX;
	mouse_position.y *= scaleY;
#endif
	b32 inside = 
		(mouse_position.x >= x0 && mouse_position.x <= x1) &&
		(mouse_position.y >= y0 && mouse_position.y <= y1);
	return(inside);
	//}

}

	static inline bool32
ui_mouse_inside_baycentric(game_ui *ui,
		vec2 v0,
		vec2 v1,
		vec2 v2)
{
	f32 w1 = 0;
	f32 w2 = 0;

	vec2 p = ui->mouse_point;
	f32 v2_v0_y_difference = v2.y - v0.y;
	f32 v2_v0_x_difference = v2.x - v0.x;
	f32 v1_v0_x_difference = v1.x - v0.x;
	f32 v1_v0_y_difference = v1.y - v0.y;
	f32 p_v0_y_difference = p.y - v0.y;

	if(!v2_v0_y_difference)
	{
		v2_v0_y_difference = 1;
	}

	w1 = (v0.x * v2_v0_y_difference) +
		(p_v0_y_difference * v2_v0_x_difference) -
		(p.x * v2_v0_y_difference);

	w1 /= (v1_v0_y_difference * v2_v0_x_difference) -
		(v1_v0_x_difference * v2_v0_y_difference);
	w2 = ((p.y - v0.y) - (w1 * v1_v0_y_difference)) / v2_v0_y_difference;

	u32 mouse_inside_uvw = (w1 > 0 && w2 > 0) && (w1 + w2 < 1);
	//probably will remove these
	return(mouse_inside_uvw);
}

	inline u32
ui_mouse_inside_clip(game_ui *ui)
{
	render_commands *renderCommands = ui->renderCommands;
	vec2 mouse_position_scaled  = ui->mouse_point;

	matrix4x4 uiMatrix = ui->projection;
	f32 scaleX = uiMatrix.m[0][0];
	f32 scaleY = uiMatrix.m[1][1];

	mouse_position_scaled.x *= scaleX;
	mouse_position_scaled.y *= scaleY;
	u32 is_inside = render_point_inside_clip(renderCommands, mouse_position_scaled);

	return(is_inside);
}

	inline i32
ui_id_equals(ui_id ID1, ui_id ID2)
{
	return(
			ID1.value0 == ID2.value0 &&
			ID1.value1 == ID2.value1
		  );
}

//
// __ui utilities
//

#define ui_get_text_size_until(ui, endX, text) font_get_text_size_wrapped_scaled(&ui->fontp, endX, text, ui->font_scale)
#define ui_get_text_size(ui, text) ui_get_text_size_until(ui, F32MAX, text)

	inline vec2
ui_get_text_padded_offset_vec2(game_ui *ui, vec2 size, u8 *text, font_text_pad padOptions)
{
	vec2 textOff = font_get_text_pad_offset_vec2(
			&ui->fontp,
			size,
			ui->font_scale,
			padOptions,
			text);

	textOff.x = (f32)(i32)textOff.x;
	textOff.y = (f32)(i32)textOff.y;
	return(textOff);
}

#define ui_node_add_scroll_y(n, delta) ui_node_change_target_scroll(n, delta, ui_axis_y)
static inline void
ui_node_change_target_scroll(ui_node *n, f32 delta, ui_axis axis)
{
	if(delta)
	{
		n->target_scroll[axis] += delta;
		n->dt_scroll[axis] = 0.0f;
	}
}

#define ui_node_set_scroll_y(n, delta) ui_node_set_target_scroll(n, delta, ui_axis_y)
static inline void
ui_node_set_target_scroll(ui_node *n, f32 scroll, ui_axis axis)
{
	if(scroll)
	{
		n->target_scroll[axis] = scroll;
		n->dt_scroll[axis] = 0.0f;
	}
}

#define ui_usri ui_interaction_info
#define ui_IFN(ui, node) (ui_interaction_from_node(ui, node).flags)
#define ui_node_mouse_l_double_clicked(ui, node) (ui_IFN(ui, node) & ui_interaction_mouse_left_double_click)
#define ui_node_mouse_l_down(ui, node) (ui_IFN(ui, node) & ui_interaction_mouse_left_down)
#define ui_node_mouse_l_pressed(ui, node) (ui_IFN(ui, node) & ui_interaction_mouse_left_pressed)
#define ui_node_mouse_l_up(ui, node) (ui_interaction_from_node(ui, node).flags & ui_interaction_mouse_left_up)
#define ui_node_mouse_hover(ui, node) (ui_IFN(ui, node) & ui_interaction_mouse_hover)
#define ui_node_mouse_lr_down(ui, node) (ui_IFN(ui, node) & (ui_interaction_mouse_left_down | ui_interaction_mouse_right_down ))

#define ui_node_interacting(ui, node) (ui_id_EQUALS(node->id, ui->interactions[node->interaction_index].node_interacting))

#define ui_usri_mouse_l_double_clicked(usri) (usri.flags & ui_interaction_mouse_left_double_click)
#define ui_usri_mouse_l_up(usri) (usri.flags & ui_interaction_mouse_left_up)
#define ui_usri_mouse_l_down(usri) (usri.flags & ui_interaction_mouse_left_down)
#define ui_usri_mouse_l_pressed(usri) (usri.flags & ui_interaction_mouse_left_pressed)
#define ui_usri_mouse_m_down(usri) (usri.flags & ui_interaction_mouse_middle_down)
#define ui_usri_mouse_hover(usri) (usri.flags & ui_interaction_mouse_hover)

#define ui_usri_mouse_r_double_clicked(usri) (usri.flags & ui_interaction_mouse_right_double_click)
#define ui_usri_mouse_r_up(usri) (usri.flags & ui_interaction_mouse_right_up)
#define ui_usri_mouse_r_down(usri) (usri.flags & ui_interaction_mouse_right_down)
#define ui_usri_mouse_r_pressed(usri) (usri.flags & ui_interaction_mouse_right_pressed)

#define ui_int_layer(ui, layer)
#define ui_usri_from_node ui_interaction_from_node

#define ui_input_text_focused(ui) (ui->keep_input_text_interaction)
static b32
ui_input_text_is_focused(game_ui *ui, ui_id input_text_id)
{
	b32 focused = ui->keep_input_text_interaction && 
		ui_id_EQUALS(input_text_id, ui->input_text_interacting_id);
	return(focused);
}

static void
ui_input_text_keep_interaction(
		game_ui *ui, ui_id input_text_id)
{
	ui->keep_input_text_interaction = 0;
	ui->input_text_interaction_countdown = 0;
	if(!ui_id_EQUALS(input_text_id, ui_id_ZERO))
	{
		if(!ui_id_EQUALS(ui->input_text_interacting_id, input_text_id))
		{
			ui->input_text_interaction_transition += 2;
		}
		ui->keep_input_text_interaction = 1;
		ui->input_text_interaction_countdown++;
		ui->input_text_interacting_id = input_text_id;
	}
}

static void
ui_cancel_input_text_interaction(
		game_ui *ui)
{
	ui->keep_input_text_interaction = 0;
	ui->input_text_interaction_countdown = 0;
}

static inline ui_interaction_info
ui_interaction_from_node(game_ui *ui, ui_node *node)
{
	ui_interaction_info result = {0};
	if(ui_id_equals(ui->interactions[node->interaction_index].node_interacting, node->id))
	{
		result.flags = ui->interactions[node->interaction_index].interacting_flags;
	}
	if(ui_id_equals(ui->interactions[node->interaction_index].node_hot, node->id))
	{
		result.flags |= ui_interaction_mouse_hover;
	}
	if(ui_id_equals(ui->interactions[node->interaction_index].node_last_interact, node->id))
	{
		result.flags |= ui->interactions[node->interaction_index].interacted_flags;
	}
	return(result);
}

	static inline bool32
ui_any_node_interacting(game_ui *ui)
{
	bool32 any = 0;
	for(u32 n = 0;
			n < ui_interaction_layer_COUNT;
			n++)
	{
		any |= (ui_id_EQUALS(ui->interactions[n].node_interacting, ui_id_ZERO) == 0);
	}
	return(any);
}

//#define ui_any_node_hot(ui) _ui_any_node_interaction(ui, 0)
//#define ui_any_node_interacting(ui) _ui_any_node_interaction(ui, 0)
#define ui_any_node_interacted(ui) _ui_any_node_interaction(ui, 2)

	static inline bool32
_ui_any_node_interaction(game_ui *ui, u32 interaction_index)
{
	bool32 any_hot = 0;
	for(u32 n = 0;
			n < ui_interaction_layer_COUNT;
			n++)
	{
		bool32 eq = ui_id_EQUALS(ui->interactions[n].interaction_ids[interaction_index], ui_id_ZERO);
		any_hot |= (eq == 0);
	}
	return(any_hot);
}

	static inline bool32
ui_any_node_hot(game_ui *ui)
{
	bool32 any_hot = 0;
	for(u32 n = 0;
			n < ui_interaction_layer_COUNT;
			n++)
	{
		bool32 eq = ui_id_EQUALS(ui->interactions[n].node_hot, ui_id_ZERO);
		any_hot |= (eq == 0);
	}
	return(any_hot);
}
	static void
ui_cancel_interaction(game_ui *ui, ui_interaction_layer layer)
{
	ui->interactions[layer].node_last_interact = ui->interactions[layer].node_interacting; 
	ui->interactions[layer].node_interacting = ui_id_ZERO; 
}

#define ui_get_remaining_text_width_at(ui, textStart, textEnd, text) font_get_remaining_width_at(&ui->fontp, textStart, textEnd, ui->font_scale, text)
//
//ui utilities__ 
//

	inline void
ui_update_node_interaction(game_ui *ui,
		ui_id id,
		f32 x0,
		f32 y0,
		f32 x1,
		f32 y1,
		u32 interaction_index)
{
	//probably will remove these
	u32 elementHovered = ui_mouse_inside_rec_clipped(
			ui, x0, y0, x1, y1);

	//u32 disabled = ui_disabled(ui);
	//if(disabled)
	//{
	//	id = ui_id_READONLY;
	//}

	if(elementHovered)
	{
		ui->interactions[interaction_index].node_last_hot = id;
	}
}

	inline void
ui_create_update_element_at_baycentric(game_ui *ui,
		ui_id id,
		vec2 v0,
		vec2 v1,
		vec2 v2,
		u32 interaction_index)
{
	f32 w1 = 0;
	f32 w2 = 0;

	vec2 p     = ui->mouse_point;
	// vec2 p_end = v0 + 
	//              w1 * (v1 - v0) +
	// 		     w2 * (v2 - v0);


	//p.x = v0.x + 
	//      w1 * (v1.x - v0.x) +
	//	  ((p.y - v0.y -
	//	  w1 * (v1.y - v0.y)) / (v2.y - v0.y)) * (v2.x - v0.x);
#if 1
	f32 v2_v0_y_difference = v2.y - v0.y;
	f32 v2_v0_x_difference = v2.x - v0.x;
	f32 v1_v0_x_difference = v1.x - v0.x;
	f32 v1_v0_y_difference = v1.y - v0.y;
	f32 p_v0_y_difference = p.y - v0.y;

	if(!v2_v0_y_difference)
	{
		v2_v0_y_difference = 1;
	}
	//if(!s2)
	//{
	//	s2 = 1;
	//}
	//if(!s3)
	//{
	//	s3 = 1;
	//}
	//if(!s4)
	//{
	//	s4 = 1;
	//}

	w1 = (v0.x * v2_v0_y_difference) +
		(p_v0_y_difference * v2_v0_x_difference) -
		(p.x * v2_v0_y_difference);

	w1 /= (v1_v0_y_difference * v2_v0_x_difference) -
		(v1_v0_x_difference * v2_v0_y_difference);
	w2 = ((p.y - v0.y) - (w1 * v1_v0_y_difference)) / v2_v0_y_difference;
#else
	vec2 A = v0;
	vec2 B = v1;
	vec2 C = v2;
	vec2 P = p;
	f32 s1 = C.y - A.y;
	f32 s2 = C.x - A.x;
	f32 s3 = B.y - A.y;
	f32 s4 = P.y - A.y;
	if(!s1)
	{
		s1 = 1;
	}
	if(!s2)
	{
		s2 = 1;
	}
	if(!s3)
	{
		s3 = 1;
	}
	if(!s4)
	{
		s4 = 1;
	}

	w1 = (A.x * s1 + s4 * s2 - P.x * s1) / (s3 * s2 - (B.x-A.x) * s1);
	w2 = (s4- w1 * s3) / s1;
#endif

	u32 mouse_inside_uvw = (w1 > 0 && w2 > 0) && (w1 + w2 < 1);
	//probably will remove these
	u32 disabled       = ui_disabled(ui);
	u32 elementHovered = mouse_inside_uvw && ui_mouse_inside_clip(ui);


	if(!disabled && elementHovered)
	{
		ui->interactions[interaction_index].node_last_hot = id;
	}
}

inline void *
ui_push_size_to_reserved(game_ui *ui, u32 size)
{
	//Temp assert
	Assert(ui->reserved_space_used + size < ui->reserved_space_total);

	u8 *at = ui->reserved_space + ui->reserved_space_used;
	void *data = at;
	ui->reserved_space_used += size;
	return(data);
}

	inline void *
ui_push_size_to_reserved_and_clear(game_ui *ui, u32 size)
{
	void *data = ui_push_size_to_reserved(ui, size);
	memory_clear(data, size);
	return(data);
}

inline u8 *
ui_push_string_to_reserved(game_ui *ui, uint8 *text, u32 textLength)
{
	u8 *result = (u8 *)ui_push_size_to_reserved(ui, textLength);
	string_copy(text, result);
	return(result);
}


static inline u8 *
ui_count_and_push_string_to_reserved(game_ui *ui, uint8 *text)
{
	u32 text_length = string_count(text);
	u8 *result = (u8 *)ui_push_size_to_reserved(ui, text_length);
	string_copy(text, result);
	return(result);
}
#define ui_allocate_string(ui, text) ui_count_and_push_string_to_reserved(ui, text)
	static inline u8 *
ui_allocate_stringf(game_ui *ui, u8 *text, ...)
{
	u8 tb[256] = {0};
	ui_ANYF(tb, sizeof(tb), text);
	u8 *result = ui_count_and_push_string_to_reserved(ui, tb);
}

	static inline ui_node * 
ui_get_persistent_node_from_id(game_ui *ui, ui_id id)
{
	ui_node *result = 0;
	if(id.value0 == 0)
	{
		result = ui_push_size_to_reserved_and_clear(
				ui, sizeof(ui_node));
	}
	else
	{
		u32 index = id.value0 % ui->persistent_nodes_max;
		if(ui->persistent_nodes[index].id.value0 != 0)
		{
			while(!ui_id_equals(ui->persistent_nodes[index].id, id) &&
					ui->persistent_nodes[index].id.value0 != 0)
			{
				index++;
				index %= ui->persistent_nodes_max;
			}
		}
		result = ui->persistent_nodes + index;
	}
	return(result);
}


	static inline u8 *
ui_format(game_ui *ui, u8 *text, ...)
{
	//TODO: push better size.
	u8 *buffer = ui_push_size_to_reserved_and_clear(ui, 128);

	va_list args;
	va_start_m(args, text);
	format_text_list(buffer, 128, text, args);
	va_end_m(args);

	return(buffer);
}

	static inline void 
ui_clamp_text_for_rendering(
		game_ui *ui, 
		ui_node *node)
{
	u8 *text = node->display_string;
	if(text)
	{
		u8 c = text[0];
		u32 i = 0;
		u32 end_index = 0;
		while((c = text[i++]) != '#' &&
				c != '\0')
		{
			end_index++;
		}
		text[end_index] = '\0';
	}
}

#define ui_vec2i16_to_vec2(v) V2(v.x, v.y)

static inline ui_render_command *
ui_push_new_render_command_to_node(game_ui *ui, ui_node *node)
{
	ui_render_command *result = 0;
	if(ui->render_commands_count < ui->render_commands_max)
	{
		result = ui->render_commands_buffer + ui->render_commands_count;
		memory_clear(result, sizeof(ui_render_command));
	}
	else
	{
		result = ui_push_size_to_reserved_and_clear(
				ui, sizeof(ui_render_command));
	}
	ui->render_commands_count++;


	if(!node->first_render_command)
	{
		node->first_render_command = result;
	}
	else
	{
		node->last_render_command->next = result; 
	}
	node->last_render_command = result;

	return(result);
}

	static inline void
ui_node_push_image_specified(game_ui *ui,
		ui_node *node,
		render_texture *texture,
		i16 x,
		i16 y,
		i16 w,
		i16 h,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->type = type_image;

	rcm->p0.x = (f32)x;
	rcm->p0.y = (f32)y + h;
	rcm->p1.x = (f32)x;
	rcm->p1.y = (f32)y;
	rcm->p2.x = (f32)x + w;
	rcm->p2.y = (f32)y;
	rcm->p3.x = rcm->p2.x;
	rcm->p3.y = rcm->p0.y;

	rcm->uv0 = uv0;
	rcm->uv1 = uv1;
	rcm->uv2 = uv2;
	rcm->uv3 = uv3;
	rcm->texture = texture;

}

	static inline void
ui_node_push_image(game_ui *ui,
		ui_node *node,
		render_texture *texture,
		i16 x,
		i16 y,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->type = type_image;

	render_scale_vertices_by_uvs_2d(
			texture,
			&rcm->p0,
			&rcm->p1,
			&rcm->p2,
			&rcm->p3,
			uv0,
			uv1,
			uv2,
			uv3);
	//some vertices will get scaled to negative coordinates, add half of the
	//texture to correctly place them
	u32 wh = texture->width / 2;
	u32 hh = texture->height / 2;
	rcm->p0.x += x + wh;
	rcm->p0.y += y + hh;
	rcm->p1.x += x + wh;
	rcm->p1.y += y + hh;
	rcm->p2.x += x + wh;
	rcm->p2.y += y + hh;
	rcm->p3.x += x + wh;
	rcm->p3.y += y + hh;

	rcm->uv0 = uv0;
	rcm->uv1 = uv1;
	rcm->uv2 = uv2;
	rcm->uv3 = uv3;
	rcm->texture = texture;

}

#define ui_node_push_text(ui, node, x, y, centered, color, text)\
	_ui_node_push_text(ui, node, (i16)x, (i16)y, centered, color, text)
#define ui_node_push_textf(ui, node, x, y, centered, color, text, ...)\
	_ui_node_push_textf(ui, node, (i16)x, (i16)y, centered, color, text, __VA_ARGS__)
	static inline void
_ui_node_push_text(game_ui *ui,
		ui_node *node,
		i16 x,
		i16 y,
		bool32 centered,
		vec4 color,
		u8 *text)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->type = type_text;
	rcm->p0.x = x;
	rcm->p0.y = y;
	rcm->p1.x = 10000;
	rcm->p1.y = 10000;
	rcm->text = ui_count_and_push_string_to_reserved(ui, text);
	if(centered)
	{
		vec2 text_offset = {0};
		text_offset = font_get_text_pad_offset(
				&ui->fontp,
				node->size_x,
				node->size_y,
				ui->font_scale,
				font_text_pad_center,
				text
				);
		rcm->p0.x += (i16)text_offset.x;
		rcm->p0.y += (i16)text_offset.y;
	}
}

	static inline void
_ui_node_push_textf(game_ui *ui,
		ui_node *node,
		i16 x,
		i16 y,
		bool32 centered,
		vec4 color,
		u8 *text,
		...)
{
	u8 text_buffer[256] = {0};

	va_list args;
	va_start_m(args, text);
	format_text_list(text_buffer, sizeof(text_buffer), text, args);
	va_end_m(args);

	ui_node_push_text(ui,
			node,
			x,
			y,
			centered,
			color,
			text_buffer);	
}

#define ui_node_push_rectangle_wh(ui, node, x, y, w, h, color)\
	ui_node_push_rectangle(ui, node, x, y, ((x) + (w)), ((y) + (h)), color)
#define ui_node_push_rectangle(ui, node, x0, y0, x1, y1, color) \
	_ui_node_push_rectangle(ui, node, (f32)x0, (f32)y0, (f32)x1, (f32)y1, color)
	static inline void
_ui_node_push_rectangle(game_ui *ui,
		ui_node *node,
		f32 x0,
		f32 y0,
		f32 x1,
		f32 y1,
		vec4 color)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->color = color;
	rcm->type = type_rectangle;
	rcm->x0 = x0;
	rcm->y0 = y0;
	rcm->x1 = x1;
	rcm->y1 = y1;
}

#define ui_node_push_hollow_rectangle_wh(ui, node, x, y, w, h, thickness, color)\
	ui_node_push_hollow_rectangle(ui, node, (i16)x, (i16)y, (i16)(0 + w), (i16)(0 + h), thickness, color)
	static inline void
ui_node_push_hollow_rectangle(game_ui *ui,
		ui_node *node,
		i16 x0,
		i16 y0,
		i16 x1,
		i16 y1,
		f32 thickness,
		vec4 color)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->color = color;
	rcm->type = type_hollow_rectangle;
	rcm->x0 = x0;
	rcm->y0 = y0;
	rcm->x1 = x1;
	rcm->y1 = y1;
	rcm->border_thickness = thickness;
}

	static inline void
ui_node_push_triangle(game_ui *ui,
		ui_node *node,
		vec2 p0,
		vec2 p1,
		vec2 p2,
		vec4 color)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->color = color;
	rcm->type = type_triangle;
	rcm->p0 = p0;
	rcm->p1 = p1;
	rcm->p2 = p2;
}

#if 1
	static inline void
ui_node_push_line(game_ui *ui,
		ui_node *node,
		vec2 p0,
		vec2 p1,
		f32 thickness,
		vec4 color)
{
	ui_render_command *rcm = ui_push_new_render_command_to_node(ui, node);
	rcm->color = color;
	rcm->type = type_line;
	rcm->p0 = p0;
	rcm->p1 = p1;
	rcm->border_thickness = thickness;
}
#endif
	static inline void
ui_free_node(game_ui *ui,
		ui_node *freeing_node)
{
	Assert(freeing_node);

	freeing_node->next = ui->first_free_node;
	ui->first_free_node = freeing_node;
}

	static inline ui_node *
ui_get_or_push_node_id(game_ui *ui, ui_id node_id)
{
	ui_node *new_node = ui->first_free_node;
	//got node from the free list
	if(new_node)
	{
		ui->first_free_node = new_node->next;
	}
	else
	{
		new_node = ui_get_persistent_node_from_id(ui, node_id); 
		//this node got already called on this frame, so return the same one...
	}

	return(new_node);
}


	inline ui_node *
ui_create_node_id(game_ui *ui, ui_node_flags flags, ui_id node_id)
{
	//get node
	ui_node *new_node = ui->first_free_node;
	//got node from the free list
	if(new_node)
	{
		ui->first_free_node = new_node->next;
	}
	else
	{
		new_node = ui_get_persistent_node_from_id(ui, node_id); 
		//this node got already called on this frame, so return the same one...
		if(new_node->last_touched_frame == ui->current_frame &&
				new_node->id.value0 != 0)
		{
			//	   new_node = ui_push_size_to_reserved_and_clear(
			//			ui, sizeof(ui_node));
			return(new_node);
		}
	}

	new_node->id = node_id;
	new_node->flags = flags;
	new_node->flags |= (flags & node_use_extra_flags) ? ui->next_node_extra_flags : 0;
	new_node->last_touched_frame = ui->current_frame;

	//current pushed size types for x and y
	new_node->size_type_x = ui->next_nodes_size_x;
	new_node->size_type_y = ui->next_nodes_size_y;

	//reset tree links
	new_node->parent = 0;
	new_node->first = 0;
	new_node->next = 0;
	new_node->prev = 0;
	new_node->last = 0;
	new_node->first_render_command = 0;
	new_node->last_render_command = 0;
	new_node->color_indices = ui->color_indices;
	new_node->layout_axis = ui_axis_y;
	new_node->interaction_index = ui->interaction_layer;
	new_node->added_x = 0;
	new_node->added_y = 0;
	new_node->padding_x = 0;
	new_node->padding_y = 0;

	if(ui->next_node_disabled)
	{
		new_node->flags |= node_disabled;
	}
	if(ui->next_node_readonly)
	{
		new_node->flags |= node_readonly;
	}

	//set a parent for the next node, if the stack count is 0,
	//then the parent will be the "global" node.
	new_node->parent = ui->parent_stack[ui->parent_stack_count];

	if(!new_node->parent->first)
	{
		new_node->parent->first = new_node;
	}
	//this node contains more than one sibling.
	if(new_node->parent->last)
	{
		new_node->parent->last->next = new_node;
		new_node->prev = new_node->parent->last;
	}
	new_node->parent->last = new_node;



	return(new_node);
}

	static inline void
ui_node_set_current_colors(game_ui *ui, ui_node *node)
{
	node->color_indices = ui->color_indices;
}

	static inline ui_node *
ui_create_node(game_ui *ui, ui_node_flags flags, u8 *text)
{

	ui_id node_id = ui_node_id_from_string(ui, text);
	ui_node *new_node = ui_create_node_id(ui, flags, node_id);

	if(text)
	{
		new_node->string = ui_count_and_push_string_to_reserved(ui, text);
		new_node->display_string = ui_count_and_push_string_to_reserved(ui, text); 
		ui_clamp_text_for_rendering(
				ui, new_node);
	}
	return(new_node);
}

	static inline ui_node *
ui_create_nodef(
		game_ui *ui,
		ui_node_flags node_flags,
		u8 *text,
		...)
{
	u8 text_buffer[256] = {0};

	va_list args;
	va_start_m(args, text);
	format_text_list(text_buffer, sizeof(text_buffer), text, args);
	va_end_m(args);

	return(ui_create_node(ui, node_flags, text_buffer));
}

	static inline void
ui_node_set_display_string(game_ui *ui, ui_node *target_node, u8 *string)
{
	target_node->display_string = 
		ui_count_and_push_string_to_reserved(ui, string);
}


	static inline void
ui_node_set_display_stringf(game_ui *ui, ui_node *target_node, u8 *string, ...)
{
	u8 string_buffer[256] = {0};
	ui_ANYF(string_buffer, 256, string);
	target_node->display_string = ui_count_and_push_string_to_reserved(ui, string_buffer);
}

//
//stacks
//


#define ui_set_parent(ui, parent) \
	ui_DEFER_LOOP(ui_push_parent(ui, parent), ui_pop_parent(ui, parent))
	static inline void
ui_push_parent(game_ui *ui, ui_node *parent_node)
{
	u32 parent_index = ui->parent_stack_count + 1;
	ui->parent_stack[parent_index] = parent_node;
	ui->current_layout_axis = ui->parent_stack[parent_index]->layout_axis;
	Assert(ui->parent_stack_max > ui->parent_stack_count);
	ui->parent_stack_count++;
}

	static inline void
ui_pop_parent(game_ui *ui, ui_node *node)
{
	if(ui->parent_stack_count)
	{
		bool32 pop_success = 0;
		u32 cached_count = ui->parent_stack_count;
		while(ui->parent_stack_count && 
				ui->parent_stack[ui->parent_stack_count] != node)
		{
			ui->parent_stack_count--;
		}
		//0 is reserved for the "root" parent node,
		//so if the count is 0, it means this parent was never pushed to the stack.
		pop_success = ui->parent_stack_count > 0; 
		if(pop_success)
		{
			ui->parent_stack_count--;
		}
		else
		{
			ui->parent_stack_count = cached_count;
		}
		ui->current_layout_axis = ui->parent_stack[ui->parent_stack_count]->layout_axis;
	}
}

static inline void
ui_pop_last_parent(game_ui *ui)
{
	if(ui->parent_stack_count)
	{
		ui->parent_stack_count--;
		ui->current_layout_axis = ui->parent_stack[ui->parent_stack_count]->layout_axis;
	}
}


	static inline void
ui_pop_parent_id(game_ui *ui, ui_id id)
{
}

#define ui_set_parent_and_id(ui, parent_node)\
	ui_DEFER_LOOP(ui_push_parent_and_id(ui, parent_node), ui_pop_parent_and_id(ui, parent_node))
	static inline void
ui_push_parent_and_id(game_ui *ui, ui_node *parent_node)
{
	ui_push_parent(ui, parent_node);
	ui_push_id(ui, parent_node->id);
}

	static inline void
ui_pop_parent_and_id(game_ui *ui, ui_node *parent_node)
{
	ui_pop_parent(ui, parent_node);
	ui_pop_id(ui);
}


#define ui_push_axis_node_size(ui, node_size, index) \
	ui_push_axis_size(ui, node_size.type, node_size.amount, node_size.size_strictness, index)
	static inline void
ui_push_axis_size(
		game_ui *ui,
		ui_node_size_type type,
		f32 amount,
		f32 size_strictness,
		u32 axis_index
		)
{
	if((ui->node_size_stacks_counts[axis_index] + 1) < ui->node_size_stacks_max[axis_index])
	{
		u32 stack_index = ui->node_size_stacks_counts[axis_index] + 1;

		ui->node_size_stacks[axis_index][stack_index].type = type;
		ui->node_size_stacks[axis_index][stack_index].size_strictness = size_strictness;
		ui->node_size_stacks[axis_index][stack_index].amount = amount;
		ui->next_node_sizes[axis_index] = ui->node_size_stacks[axis_index][stack_index];
	}
	ui->node_size_stacks_counts[axis_index]++;
}


	static inline void
ui_pop_prefered_axis_size(
		game_ui *ui,
		u32 axis_index)
{
	if(ui->node_size_stacks_counts[axis_index])
	{
		ui->node_size_stacks_counts[axis_index]--;
	}
	ui->next_node_sizes[axis_index] = ui->node_size_stacks[axis_index][ui->node_size_stacks_counts[axis_index]];
}


	static inline void
ui_push_prefered_width(
		game_ui *ui,
		ui_node_size_type type,
		f32 amount,
		f32 size_strictness)
{
	ui_push_axis_size(ui, type, amount, size_strictness, 0);
}


	static inline void
ui_push_prefered_height(
		game_ui *ui,
		ui_node_size_type type,
		f32 amount,
		f32 size_strictness
		)
{
	ui_push_axis_size(ui, type, amount, size_strictness, 1);
}
//Note: ppct = parent percent; soch = sum of children.

	static inline void 
ui_push_width(game_ui *ui, ui_node_size node_size)
{
	ui_push_axis_size(
			ui,
			node_size.type,
			node_size.amount,
			node_size.size_strictness,
			ui_axis_x	
			);
}
	static inline void 
ui_push_height(game_ui *ui, ui_node_size node_size)
{
	ui_push_axis_size(
			ui,
			node_size.type,
			node_size.amount,
			node_size.size_strictness,
			ui_axis_y
			);
}

	static inline void 
ui_push_width_height(game_ui *ui, ui_node_size node_size)
{
	ui_push_axis_size(
			ui,
			node_size.type,
			node_size.amount,
			node_size.size_strictness,
			ui_axis_x	
			);
	ui_push_axis_size(
			ui,
			node_size.type,
			node_size.amount,
			node_size.size_strictness,
			ui_axis_y
			);
}

	static inline void
ui_pop_width_height(game_ui *ui)
{
	ui_pop_prefered_axis_size(
			ui,
			0);
	ui_pop_prefered_axis_size(
			ui,
			1);
}

//#define ui_push_width(ui, node_size) \
//	ui_push_axis_size(ui, node_size.type, node_size.amount, node_size.size_strictness, 0)
//
//#define ui_push_height(ui, node_size) \
//	ui_push_axis_size(ui, node_size.type, node_size.amount, node_size.size_strictness, 1)

#define ui_push_wh_specified(ui, size, strictness) \
	ui_push_width_height(ui, ui_size_specified(size, strictness))
#define ui_pop_wh(ui)\
	ui_pop_width_height(ui)

#define ui_current_size_w(ui) (ui->node_size_stacks[ui_axis_x][ui->node_size_stacks_counts[ui_axis_x]])
#define ui_current_size_h(ui) (ui->node_size_stacks[ui_axis_y][ui->node_size_stacks_counts[ui_axis_y]])


#define ui_set_specified_width(ui, type, amount, strictness) ui_DEFER_LOOP(ui_push_prefered_width(ui, type, amount, strictness), ui_pop_width(ui))
#define ui_set_specified_height(ui, type, amount, strictness) ui_DEFER_LOOP(ui_push_prefered_height(ui, type, amount, strictness), ui_pop_height(ui))
//same as above, but the ui_size struct is used instead
#define ui_set_width(ui, s_size) ui_DEFER_LOOP(ui_push_width(ui, s_size), ui_pop_width(ui))
#define ui_set_height(ui, s_size) ui_DEFER_LOOP(ui_push_height(ui, s_size), ui_pop_height(ui))

#define ui_set_w_strictness(ui, strictness) ui_DEFER_LOOP(ui_push_prefered_width(ui, ui_current_size_w(ui).type, ui_current_size_w(ui).amount, strictness), ui_pop_width(ui))
#define ui_set_h_strictness(ui, strictness) ui_DEFER_LOOP(ui_push_prefered_height(ui, ui_current_size_h(ui).type, ui_current_size_h(ui).amount, strictness), ui_pop_height(ui))
#define ui_set_wh_strictness(ui, strictness) ui_set_w_strictness(ui, strictness) ui_set_h_strictness(ui, strictness)

#define ui_push_w_em(ui, amount, strictness)\
	ui_push_width(ui, ui_size_em(ui, amount, strictness))
#define ui_push_w_specified(ui, amount, strictness)\
	ui_push_width(ui, ui_size_specified(amount, strictness))
#define ui_push_w_text(ui, space_between, strictness)\
	ui_push_width(ui, ui_size_text(space_between, strictness))
#define ui_push_w_ppct(ui, percent, strictness)\
	ui_push_width(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_push_w_soch(ui, strictness)\
	ui_push_width(ui, ui_size_sum_of_children(strictness))

#define ui_push_h_em(ui, amount, strictness)\
	ui_push_height(ui, ui_size_em(ui, amount, strictness))
#define ui_push_h_specified(ui, amount, strictness)\
	ui_push_height(ui, ui_size_specified(amount, strictness))
#define ui_push_h_text(ui, space_between, strictness)\
	ui_push_height(ui, ui_size_text(space_between, strictness))
#define ui_push_h_ppct(ui, percent, strictness)\
	ui_push_height(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_push_h_soch(ui, strictness)\
	ui_push_height(ui, ui_size_sum_of_children(strictness))


#define ui_set_width_height(ui, s_size) \
	ui_set_width(ui, s_size) ui_set_height(ui, s_size)
#define ui_set_wh ui_set_width_height
#define ui_set_wh_ppct(ui, percent, strictness)\
	ui_set_width_height(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_set_wh_text(ui, spacing, strictness)\
	ui_set_width_height(ui, ui_size_text(spacing, strictness))
#define ui_set_wh_soch(ui, strictness)\
	ui_set_width_height(ui, ui_size_sum_of_children(strictness))
#define ui_set_wh_specified(ui, size, strictness)\
	ui_set_width_height(ui, ui_size_specified(size, strictness))
#define ui_set_wh_em(ui, amount, strictness)\
	ui_set_width_height(ui, ui_size_em(ui, amount, strictness))

#define ui_push_wh(ui, s_size) ui_push_width(ui, s_size) ui_push_height(ui, s_size)


#define ui_set_w_em(ui, amount, strictness)\
	ui_set_width(ui, ui_size_em(ui, amount, strictness))
#define ui_set_w_specified(ui, amount, strictness)\
	ui_set_width(ui, ui_size_specified(amount, strictness))
#define ui_set_w_text(ui, space_between, strictness)\
	ui_set_width(ui, ui_size_text(space_between, strictness))
#define ui_set_w_ppct(ui, percent, strictness)\
	ui_set_width(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_set_w_soch(ui, strictness)\
	ui_set_width(ui, ui_size_sum_of_children(strictness))
#define ui_set_w_null(ui)\
	ui_set_width(ui, ui_size_null())

#define ui_set_h_em(ui, amount, strictness)\
	ui_set_height(ui, ui_size_em(ui, amount, strictness))
#define ui_set_h_specified(ui, amount, strictness)\
	ui_set_height(ui, ui_size_specified(amount, strictness))
#define ui_set_h_text(ui, space_between, strictness)\
	ui_set_height(ui, ui_size_text(space_between, strictness))
#define ui_set_h_ppct(ui, percent, strictness)\
	ui_set_height(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_set_h_soch(ui, strictness)\
	ui_set_height(ui, ui_size_sum_of_children(strictness))
#define ui_set_h_null(ui)\
	ui_set_height(ui, ui_size_null())


	static inline void
ui_pop_width(
		game_ui *ui)
{
	ui_pop_prefered_axis_size(ui, 0);
}

	static inline void
ui_pop_height(
		game_ui *ui)
{
	ui_pop_prefered_axis_size(ui, 1);
}


#define ui_push_text_color(ui, c) ui_push_color(ui, ui_color_text, c)
#define ui_pop_text_color(ui, c) ui_pop_color(ui, ui_color_text)
#define ui_set_text_color(ui, c) ui_DEFER_LOOP(ui_push_text_color(ui, c), ui_pop_text_color(ui, c))

#define ui_set_color(ui, color_enum, c) ui_DEFER_LOOP(ui_push_color(ui, color_enum, c), ui_pop_color(ui, color_enum))
#define ui_set_color_a(ui, color_enum, a) ui_DEFER_LOOP(ui_push_color_a(ui, color_enum, a), ui_pop_color(ui, color_enum))

static inline void
ui_push_color(game_ui *ui, ui_theme_colors color_index, vec4 color)
{
	if((ui->theme_colors_counts[color_index] + 1) < ui->theme_colors_max[color_index])
	{
		u32 stack_index = ui->theme_colors_counts[color_index] + 1;
		ui->theme_colors[color_index][stack_index].color = color;
		ui->theme_colors[color_index][stack_index].previous = ui->theme_colors_indices[color_index];

		ui->theme_colors_indices[color_index] = stack_index;
		ui->color_indices.i[color_index] = ui->theme_colors_indices[color_index];
	}
	ui->theme_colors_counts[color_index]++;
}

static inline void
ui_pop_color(game_ui *ui, ui_theme_colors color_index)
{
	if(ui->theme_colors_counts[color_index])
	{
		u32 current_index = ui->theme_colors[color_index][ui->theme_colors_indices[color_index]].previous;
		ui->theme_colors_indices[color_index] = current_index; 
		ui->color_indices.i[color_index] = current_index; 
	}
}


	static inline vec4
ui_get_current_color(game_ui *ui, ui_theme_colors color_index)
{
	vec4 result = ui->theme_colors[color_index][ui->theme_colors_counts[color_index]].color;
	return(result);
}

static inline void
ui_push_color_a(game_ui *ui, ui_theme_colors color_index, f32 alpha)
{
    vec4 color = ui_get_current_color(ui, color_index);
	color.a = alpha * 255;
	ui_push_color(ui, color_index, color);
}
#define ui_pop_color_a ui_pop_color



//
//__ui push commands
//

typedef struct{
	u32 interacted;
	u32 assigned_direction;
}ui_selectable_directions_info;

#define ui_selectable_directions_TYPE(type) static void\
	ui_selectable_directions_##type(\
			game_ui *ui, u32 orientation_count, type *assignable_orientation)

	static ui_selectable_directions_info
_ui_push_selectable_directions(game_ui *ui,
		u32 orientation_count)
{
	ui_selectable_directions_info d = {0};
	return(d);
}

ui_selectable_directions_TYPE(u32)
{
	ui_selectable_directions_info info = _ui_push_selectable_directions(ui, orientation_count);

	if(info.interacted)
	{
		*assignable_orientation = info.assigned_direction;
	}
}

ui_selectable_directions_TYPE(u16)
{
	ui_selectable_directions_info info = _ui_push_selectable_directions(ui, orientation_count);

	if(info.interacted)
	{
		*assignable_orientation = info.assigned_direction;
	}
}

ui_selectable_directions_TYPE(vec2)
{
	ui_selectable_directions_info info = _ui_push_selectable_directions(ui, orientation_count);

	if(info.interacted)
	{
		f32 mo =  (f32)info.assigned_direction / (f32)orientation_count;
		assignable_orientation->y = -cos32(mo * (PI * 2));
		assignable_orientation->x = sin32(mo * (PI * 2));
	}
}

#define ui_text_colored(ui, color, text) ui_text_All(ui, 0, color, text)
#define ui_textWrapped(ui, text)        ui_text_All(ui, 1, vec4_all(255),text)

#define ui_textf_colored(ui, color, text, ...) ui_textf_All(ui, 0, color, text, __VA_ARGS__)

	inline void
ui_text_All(game_ui *ui, u32 wrap, vec4 color, u8 *text)
{

}

	inline void
ui_textf_All(game_ui *ui, u32 wrap, vec4 color, u8 *text, ...)
{
}

#define ui_size_specified(amount, strictness) _ui_size_specified((f32)amount, (f32)strictness)
	static inline ui_node_size
_ui_size_specified(f32 amount, f32 size_strictness)
{
	ui_node_size result = {0};
	result.type = size_specified;
	result.amount = amount;
	result.size_strictness = size_strictness;
	return(result);
}

	static inline ui_node_size
ui_size_text(f32 padding ,f32 strictness)
{
	ui_node_size result = {0};
	result.type = size_text;
	result.amount = padding;
	result.size_strictness =  strictness;
	return(result);
}

static inline ui_node_size
ui_size_null()
{
	ui_node_size result = {0};
	result.type = size_null;
	result.size_strictness = 1;
	return(result);
}

static inline ui_node_size
ui_size_sum_of_children(f32 strictness)
{
	ui_node_size result = {0};
	result.type = size_sum_of_children;
	result.size_strictness = strictness;
	return(result);
}

	static inline ui_node_size
ui_size_percent_of_parent(f32 percent, f32 strictness)
{
	ui_node_size result = {0};
	result.type = size_percent_of_parent;
	result.amount = percent;
	result.size_strictness = strictness;

	return(result);
}

//pre_defined helper functions for widgets

	static inline ui_node 
ui_tree_node_begin(
		game_ui *ui,
		b32 *opened,
		ui_node *tree_button)
{
	//	ui_node *box;
	////	f32 strictness_x = tree_button->size_type_x.strictness;
	////	f32 strictness_y = tree_button->size_type_y.strictness;
	//	ui_set_parent(ui, tree_button)
	//	{
	//		u8 *text = tree_button->tree_opened ? "-" : "+";
	//		ui_text(ui, ">");
	//	}
	//	if(tree_button->tree_opened)
	//	{
	//	}
	//	ui_push_parent(ui, box);
	//	return(tree_button);
}

	static inline void
ui_style_node_push_checkbox_check(
		game_ui *ui,
		ui_node *node,
		i16 x,
		i16 y,
		u32 c_size,
		bool32 active)
{
	ui_set_width_height(ui, ui_size_specified(c_size, 1.0f))
	{

		//borders
		ui_node_push_hollow_rectangle(ui,
				node,
				0,
				0,
				c_size,
				c_size,
				1,
				V4(255, 255, 255, 255));

		//active rectangle inside
		if(active)
		{
			ui_node_push_rectangle(
					ui,
					node,
					2,
					2,
					c_size - 2,
					c_size - 2,
					V4(255, 255, 255, 255));
		}
	}
}


	inline void
ui_pop_root(game_ui *ui)
{
	Assert(ui->current_panel);
	Assert(ui->panel_stack_max > 0);

	ui_pop_id(ui);
	ui_pop_parent(ui, ui->current_panel->root_node);
	ui_panel *current_panel = ui->current_panel;

	ui->current_panel = ui->current_panel->prev; 
}


	static inline ui_panel *
ui_look_for_panel_on_stack(game_ui *ui, ui_id panel_id)
{
	ui_panel *panel = 0;
	u32 i = 0;
	u32 found_panel_on_stack = 0;
	//linear search a new panel on the stack
	while(!found_panel_on_stack && i < ui->panel_last_avadible_slot)
	{
		found_panel_on_stack = ui_id_EQUALS(ui->panel_stack[i].id, panel_id);
		if(found_panel_on_stack)
		{
			panel = ui->panel_stack + i;
		}
		i++;
	}

	return(panel);
}


#define ui_open_panel(ui, title)\
	ui_open_panel_id(ui, ui_id_from_string(title))
#define ui_open_panel_ptr(ui, panel)\
	ui_open_panel_id(ui, panel->id)
	static inline u32 
ui_open_panel_id(game_ui *ui, ui_id panel_id)
{
	ui_panel *panel_to_open = ui->first_panel_closed; 
	ui_panel *previous = 0;
	while(panel_to_open && !ui_id_EQUALS(panel_to_open->id, panel_id))
	{
		previous = panel_to_open;
		panel_to_open = panel_to_open->next;
	}
	if(panel_to_open)
	{
		panel_to_open->closed = 0;

		if(previous)
		{
			previous->next = panel_to_open->next;
		}

		//		ui->first_panel_closed = panel_to_open->next;
		if(panel_to_open == ui->first_panel_closed)
		{
			ui->first_panel_closed = panel_to_open->next;
		}
		panel_to_open->next = 0;
	}
	return(panel_to_open != 0);
}

	static inline void
ui_close_panel_ptr(game_ui *ui, ui_panel *panel)
{
	if(!panel->closed )
	{
		panel->closed = 1;
		if(!(panel->flags & ui_panel_flags_from_overflow))
		{
			//if(ui->first_panel_closed)
			//{
			//	ui->first_panel_closed->next = panel;
			//}
			panel->next = ui->first_panel_closed;
			ui->first_panel_closed = panel;
		}
	}
}

static b32 
ui_close_panel(game_ui *ui, u8 *title)
{
	ui_panel *panel_to_close = ui_look_for_panel_on_stack(ui, ui_id_from_string(title));
	if(panel_to_close)
	{
		ui_close_panel_ptr(ui, panel_to_close);
	}
	return(panel_to_close != 0);
}

	static inline u32 
ui_open_or_close_panel(game_ui *ui, u8 *title)
{
	ui_id panel_id = ui_id_from_string(title);
	ui_panel *panel_to_open = ui_look_for_panel_on_stack(ui, panel_id);
	if(panel_to_open)
	{
		if(panel_to_open->closed)
		{
			ui_open_panel_ptr(ui, panel_to_open);
		}
		else
		{
			ui_close_panel_ptr(ui, panel_to_open);
		}
	}
	return(panel_to_open != 0);
}


//
#define ui_push_or_get_root(ui, flags, label)\
	ui_push_or_get_root(ui, flags , ui_id_from_string(title))
	static inline ui_panel *
ui_push_or_get_panel_id(game_ui *ui, ui_panel_flags panel_flags, ui_id panel_id)
{
	ui_panel *panel = 0;
	//look if it exists
	panel = ui_look_for_panel_on_stack(ui, panel_id);
	//Detect overflow
	if(!panel)
	{
		if(ui->panel_last_avadible_slot >= ui->panel_stack_max)
		{
			//Temporary push panel to buffer
			panel = ui_push_size_to_reserved_and_clear(ui, sizeof(ui_panel));
			if(panel_flags & ui_panel_flags_init_closed)
			{
				panel->closed = 1;
			}
			panel_flags |= ui_panel_flags_from_overflow;

		}
		else
		{
			panel = ui->panel_stack + ui->panel_last_avadible_slot;
			ui->panel_last_avadible_slot++;

			if(panel_flags & ui_panel_flags_init_closed)
			{
				//panel->closed = 1;
				ui_close_panel_ptr(ui, panel);
			}
		}
	}
	ui->panel_stack_count++;
	panel->id = panel_id;
	panel->flags = panel_flags;

	return(panel);
}

	static inline void
ui_prepare_panel_for_rendering(
		game_ui *ui,
		ui_panel *panel,
		f32 x,
		f32 y,
		f32 w,
		f32 h
		)
{
	if(!panel->closed)
	{
		ui->panel_stack_order_ptr[ui->panel_stack_generated++] = panel;
	}

	bool32 not_initialized = panel->frame_touched == 0;
	ui_panel_flags panel_flags = panel->flags;

	panel->frame_touched = ui->current_frame;
	ui_push_id(ui, panel->id);

	if(panel_flags & ui_panel_flags_init_once)
	{
		if(not_initialized)
		{
			panel->p.x  = x;
			panel->p.y  = y;
			panel->sz.x = w;
			panel->sz.y = h;
		}
	}
	else
	{
		panel->p.x  = x;
		panel->p.y  = y;
		panel->sz.x = w;
		panel->sz.y = h;
	}
	//create a new node tree for nodes inside this panel
	if(!panel->closed)
	{

		ui_node_flags main_panel_node_flags = 0;
		if(panel_flags & ui_panel_flags_move)
		{
			main_panel_node_flags = 
				node_skip_layout_x | 
				node_skip_layout_y;
		}



		ui_node *main_panel_node;

		ui_set_parent(ui, &ui->root_node)
		{
			//use a specified size and not the panel size
			if(panel_flags & ui_panel_flags_respect_layout)
			{
				main_panel_node = ui_create_node(
						ui, 
						main_panel_node_flags,
						"__MAIN__");
			}
			else
			{
				//use panel size
				ui_set_w_specified(ui, panel->sz.x, 1.0f)
					ui_set_h_specified(ui, panel->sz.y, 1.0f)
					{
						main_panel_node = ui_create_node(
								ui, 
								main_panel_node_flags,
								"__MAIN__");
					}
			}
		}
		main_panel_node->added_x = (i32)panel->p.x;
		main_panel_node->added_y = (i32)panel->p.y;


		//set for rendering
		ui_push_parent(ui, main_panel_node);
		panel->root_node = main_panel_node;
	}
	else
	{
		//skip whatever goes inside this panel
		ui_push_parent(ui, &ui->void_node);
		panel->root_node = &ui->void_node;
	}

	if(ui->current_panel)
	{
		panel->prev = ui->current_panel;
	}
	ui->current_panel = panel;
	panel->call_order = ui->root_order++;
}

	static inline void
ui_re_position_panel(ui_panel *panel, f32 x, f32 y)
{
	panel->p.x = x;
	panel->p.y = y;
	panel->root_node->added_x = (i16)x;
	panel->root_node->added_y = (i16)y;
}

//more specifically, this is an invisible "root node"
//get panel from stack, prepare it for command processing
#define ui_push_root_for_rendering(ui, x, y, w, h, flags, label) ui_push_root_for_rendering_id(\
		ui,\
		x, y, w, h,\
		flags,\
		ui_id_from_string(label))
	inline ui_panel *
ui_push_root_for_rendering_id(game_ui *ui,
		f32 x, f32 y, f32 w, f32 h,
		ui_panel_flags panel_flags,
		ui_id id)
{

	ui_panel *root = ui_push_or_get_panel_id(ui, panel_flags, id);

	ui_prepare_panel_for_rendering(
			ui,
			root,
			x,
			y,
			w,
			h
			);

	return(root);
}



#define ui_space_ppct(ui, percent, strictness)\
	ui_space(ui, ui_size_percent_of_parent(percent, strictness))
#define ui_space_specified(ui, amount, strictness) \
	ui_space(ui, ui_size_specified(amount, strictness))
#define ui_space_em(ui, amount, strictness) \
	ui_space(ui, ui_size_em(ui, amount, strictness))
#define ui_space_between(ui, node_size) ui_DEFER_LOOP(ui_space(ui, node_size), ui_space(ui, node_size))
	static inline ui_node * 
ui_space(game_ui *ui, ui_node_size node_size)
{
	ui_push_axis_size(
			ui,
			size_null,
			0,
			0,
			!ui->current_layout_axis);

	ui_push_axis_node_size(
			ui,
			node_size,
			ui->current_layout_axis);

	ui_node *s = ui_create_node(ui, 0, 0);
	ui_pop_width(ui);
	ui_pop_height(ui);

	return(s);
}

#define ui_set_row(ui) ui_DEFER_LOOP(ui_push_row(ui, 0, 0), ui_pop_row(ui))
#define ui_set_column(ui) ui_DEFER_LOOP(ui_push_column(ui, 0, 0), ui_pop_column(ui))
#define ui_set_row_n(ui) ui_DEFER_LOOP(ui_push_row_n(ui), ui_pop_row(ui))
#define ui_set_column_n(ui) ui_DEFER_LOOP(ui_push_column_n(ui), ui_pop_column(ui))

	static inline void 
ui_push_column(game_ui *ui, f32 w_strictness, f32 h_strictness)
{
	ui_node *n;
	ui->current_layout_axis = 1;
	ui_set_width(ui, ui_size_sum_of_children(w_strictness))
		ui_set_height(ui, ui_size_sum_of_children(h_strictness))
		{
			n = ui_create_node(ui,
					0,
					0);
			n->layout_axis = 1;
		}
	ui->column_stack[ui->column_stack_count++] = n;
	ui_push_parent(ui, n);
}

	static inline ui_node * 
ui_push_column_n(game_ui *ui)
{
	ui_node *n;
	ui->current_layout_axis = 1;
	n = ui_create_node(ui,
			0,
			0);
	n->layout_axis = 1;
	ui->column_stack[ui->column_stack_count++] = n;
	ui_push_parent(ui, n);

	return(n);
}

	static inline void
ui_pop_column(game_ui *ui)
{
	ui_pop_parent(ui, ui->column_stack[--ui->column_stack_count]);
	ui->current_layout_axis = 0;
}

	static inline ui_node * 
ui_push_row_n(game_ui *ui)
{
	ui_node *n;

	n = ui_create_node(ui,
			0,
			0);
	n->layout_axis = 0;

	ui->row_stack[ui->row_stack_count++] = n;
	ui_push_parent(ui, n);

	return(n);
}

	static inline ui_node *
ui_push_row(game_ui *ui, f32 w_strictness, f32 h_strictness)
{
	ui_node *n;

	//	ui->current_layout_axis = 0;
	ui_set_width(ui, ui_size_sum_of_children(w_strictness))
		ui_set_height(ui, ui_size_sum_of_children(h_strictness))
		{
			n = ui_create_node(ui,
					0,
					0);
			n->layout_axis = 0;
		}

	ui->row_stack[ui->row_stack_count++] = n;
	ui_push_parent(ui, n);

	return(n);
}

	static inline void
ui_pop_row(game_ui *ui)
{
	ui_pop_parent(ui, ui->row_stack[--ui->row_stack_count]);
	//	ui->current_layout_axis = 1;
}

static inline ui_node * 
ui_interact_area(game_ui *ui, ui_interaction_layer layer)
{
	ui_node *area;
	ui_set_interaction_layer(ui, layer)
	{
		area = ui_create_nodef(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y,
				"__INTERACT_AREA__"
				);
	}
	return(area);
}

#define ui_interact_top(ui, label) ui_interact_node(ui, ui_interaction_layer_top, label)
#define ui_interact_mid(ui, label) ui_interact_node(ui, ui_interaction_layer_mid, label)
static inline ui_node *
ui_interact_node(
		game_ui *ui, ui_interaction_layer layer, u8 *label)
{
	ui_node *area;
	ui_set_interaction_layer(ui, layer)
	{
		area = ui_create_nodef(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y,
				label
				);
	}
	return(area);
}

static inline ui_node *
ui_mid_interact(
		game_ui *ui, u8 *label)
{
	ui_node *area;
	ui_set_interaction_layer(ui, ui_interaction_layer_mid)
	{
		area = ui_create_nodef(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y,
				label
				);
	}
	return(area);
}

static inline ui_usri
ui_top_region_usri(
		game_ui *ui,
		u8 *label)
{
	ui_node *region;
	ui_set_interaction_layer(ui, ui_interaction_layer_top)
	{
		region= ui_create_nodef(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y,
			    label);
	}
	return(ui_usri_from_node(ui, region));
}

static inline ui_node *
ui_mid_region(
		game_ui *ui)
{
	ui_node *scroll_region;
	ui_set_interaction_layer(ui, ui_interaction_layer_mid)
	{
		scroll_region = ui_create_node(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y |
				node_skip_content_size_x |
				node_skip_content_size_y,
				"__SCROLL_REGION__"
				);
	}
	return(scroll_region);
}


#define ui_invisible_region(ui, label) (ui_create_node(ui, 0, label))

//tooltip in mouse position
	static inline void
ui_tool_tip_mouse(game_ui *ui, u8 *text)
{

	ui_panel *panel = ui_push_root_for_rendering(
			ui, 
			ui->mouse_point.x, ui->mouse_point.y, 0, 0,
			ui_panel_flags_keep_on_front | ui_panel_flags_respect_layout | ui_panel_flags_move,
			text);
	//panel->id = panel_id;

	ui_push_width(ui, ui_size_text(1.0f, 1.0f));
	ui_push_height(ui, ui_size_text(1.0f, 1.0f));
	ui_node *tooltip_node = ui_create_node(ui,
			node_text |
			node_text_centered |
			node_background |
			node_border |
			node_skip_layout_x |
			node_skip_layout_y,
			text);
	//tooltip_node->added_x = (i16)ui->mouse_point.x - 50;
	//tooltip_node->added_y = (i16)ui->mouse_point.y;

	ui_pop_width(ui);
	ui_pop_height(ui);

	ui_pop_root(ui);

}

	static inline void
ui_tool_tip_mouse_begin(game_ui *ui, u8 *text)
{
	ui_panel *panel = ui_push_root_for_rendering(
			ui, 
			ui->mouse_point.x, ui->mouse_point.y, 0, 0,
			ui_panel_flags_keep_on_front | 
			ui_panel_flags_respect_layout | 
			ui_panel_flags_move,
			text);

}
#define ui_tool_tip_end(ui) ui_pop_root(ui)


//simple text without background nor borders
#define ui_QUICK_FORMAT(tb, tb_sz, text)\
	va_list args;\
	va_start_m(args, text);\
	format_text_list(text_buffer, tb_sz, text, args);\
	va_end_m(args);

	static inline void 
ui_text(game_ui *ui, u8 *text)
{
	ui_node *node = ui_create_node(ui,
			node_text,
			0);
	ui_node_set_display_string(ui, node, text);
}

	static inline void
ui_textf(game_ui *ui, u8 *text, ...)
{
	u8 text_buffer[256] = {0};
	va_list args;
	va_start_m(args, text);
	format_text_list(text_buffer, sizeof(text_buffer), text, args);
	va_end_m(args);

	ui_text(ui, text_buffer); 
}


	static inline ui_node *
ui_selectable_box(game_ui *ui,
		bool32 active,
		u8 *label)
{
	ui_node *selectable_node;
	vec4 active_color = ui->theme.background_color;
	ui_node_flags sel_flags =
		node_use_extra_flags |
		node_clickeable |
		node_background |
		node_active_animation |
		node_hover_animation;

	if(active)
	{
		active_color = ui->theme.hot_color;
		ui_set_color(ui,
				ui_color_background,
				active_color)
		{
			selectable_node = ui_create_node(ui, sel_flags, label);
		}
	}
	else
	{
		selectable_node = ui_create_node(ui, sel_flags, label);
	}

	return(selectable_node);
}

	static inline ui_node *
ui_selectable_boxf(game_ui *ui,
		bool32 active,
		u8 *label,
		...)
{
	u8 text_buffer[256] = {0};

	ui_ANYF(text_buffer, sizeof(text_buffer), label);

	return(ui_selectable_box(ui,
				active,
				text_buffer));
}

	static inline u32
ui_selectable(game_ui *ui, bool32 active, u8 *label)
{
	ui_node *selectable_node;
	vec4 active_color = ui_current_background_color(ui);

	if(active)
	{
		active_color = ui->theme.hot_color;
	}

	ui_set_color(ui,
			ui_color_background,
			active_color)
	{
		selectable_node = ui_create_node(ui,
				node_background |
				node_clickeable |
				node_active_animation |
				node_hover_animation |
				node_text |
				node_use_extra_flags,
				label);
	}
	ui_usri ite = ui_interaction_from_node(ui, selectable_node);
	u32 up = 0;

	if(ite.flags & ui_interaction_mouse_left_up)
	{
		up = 1;
	}
	return(up);
}

	static inline u32
ui_selectablef(game_ui *ui, bool32 active, u8 *label, ...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, sizeof(text_buffer), label);
	return(ui_selectable(ui, active, text_buffer));
}

#define ui_selectable_set_PARAMS(type) (game_ui *ui, type *value_to_set, type set_value, u8 *label)
	static inline u32 
ui_selectable_set_u32 ui_selectable_set_PARAMS(u32)
{
	if(ui_selectable(ui, (*value_to_set) == set_value, label))
	{
		*value_to_set = set_value;
		return(1);
	}
	return(0);
}

	static inline b32 
ui_selectable_set_u32f(game_ui *ui,
		u32 *value_to_set,
		u32 set_value,
		u8 *label,
		...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, sizeof(text_buffer), label);
	return(ui_selectable_set_u32(ui, value_to_set, set_value, text_buffer));
}

	static inline u16 
ui_selectable_set_u16 ui_selectable_set_PARAMS(u16)
{
	if(ui_selectable(ui, (*value_to_set) == set_value, label))
	{
		*value_to_set = set_value;
		return(1);
	}
	return(0);
}

	static inline b16 
ui_selectable_set_u16f(game_ui *ui,
		u16 *value_to_set,
		u16 set_value,
		u8 *label,
		...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, sizeof(text_buffer), label);
	return(ui_selectable_set_u16(ui, value_to_set, set_value, text_buffer));
}

	static ui_node * 
ui_radio_button_u32(game_ui *ui, void *value_to_set, u32 set_value, u8 *label)
{
	b32 checked = value_to_set && *(u32 *)value_to_set == set_value;
	ui_push_id_string(ui, label);
	ui_node *row_node = 0;
	ui_set_wh_soch(ui, 0.0f)
	{
		row_node = ui_create_node(ui, node_clickeable, label);
	}
	row_node->layout_axis = ui_axis_x;
	ui_set_parent(ui, row_node)
	{
		ui_node *cb_node = 0;

		cb_node = ui_create_node(ui, 0, 0);
		ui_style_node_push_checkbox_check(
				ui,
				cb_node,
				0,
				0,
				row_node->size_y,
				checked);
		ui_space_specified(ui, row_node->size_y + 4, 0.0f);
		ui_text(ui, label);

	}
	ui_pop_id(ui);

	if(ui_node_mouse_l_up(ui, row_node) && value_to_set)
	{
		*(u32 *)value_to_set = set_value;
	}

	return(row_node);
}

//static inline ui_node*
//ui_radio_button_u32(game_ui *ui, void *value_to_set, u32 set_value, u8 *label)

#define ui_extra_flags(ui, flags) ui_DEFER_LOOP(ui_extra_flags_begin(ui, flags), ui_extra_flags_end(ui))
	static inline void
ui_extra_flags_begin(game_ui *ui, ui_node_flags extra_flags)
{
	ui->next_node_extra_flags = extra_flags;
}
	static inline void
ui_extra_flags_end(game_ui *ui)
{
	ui->next_node_extra_flags = 0;
}
#define ui_label_exf(ui, extra_flags, label, ...) ui_create_nodef(ui, extra_flags | node_background | node_border, label, __VA_ARGS__)
#define ui_label_ex(ui, extra_flags, label) ui_create_node(ui, extra_flags | node_background | node_border, label)
#define ui_label(ui, label) ui_label_ex(ui, 0, label) 
#define ui_labelf(ui, label, ...) ui_label_exf(ui, 0, label, __VA_ARGS__) 

static inline ui_node * 
ui_node_box(game_ui *ui, u8 *label)
{
	ui_node *background;

	background = ui_create_node(ui,
			node_clickeable | node_use_extra_flags | node_background | node_border,
			label);
	background->padding_x = 6;
	background->padding_y = 6;
	return(background);
}

static inline ui_node *
ui_scroll_v_box(game_ui *ui, u8 *label)
{
	ui_node *background;

	background = ui_create_node(ui,
			node_clickeable | node_use_extra_flags | node_background | node_border | node_scroll_y,
			label);
	background->padding_x = 6;
	background->padding_y = 6;
	return(background);
}

#define ui_zero_node(ui) ui_create_node(ui, 0, 0)

	static inline ui_node *
_ui_start_invisible_parent(game_ui *ui)
{
	ui_node *parent = ui_zero_node(ui);
	ui_push_parent(ui, parent);
	return(parent);
}
	static inline void
_ui_end_invisible_parent(game_ui *ui, ui_node **parent)
{
	ui_pop_parent(ui, *parent);
}
#define ui_invisible_parent(ui) for(ui_node *p = _ui_start_invisible_parent(ui);\
		p;\
		_ui_end_invisible_parent(ui, &p))

	static inline ui_node *
ui_button_node(game_ui *ui, u8 *text)
{

	ui_push_color(ui, ui_color_background, V4(0, 60, 80, 255));
	ui_node *node = ui_create_node(ui,
			node_clickeable |
			node_background |
			node_hover_animation |
			node_active_animation |
			node_text |
			node_text_centered |
			node_use_extra_flags,
			text);
	ui_pop_color(ui, ui_color_background);

	return(node);
}

	static inline ui_node * 
ui_button_nodef(game_ui *ui, u8 *text, ...)
{
	u8 text_buffer[256] = {0};

	va_list args;
	va_start_m(args, text);
	format_text_list(text_buffer, sizeof(text_buffer), text, args);
	va_end_m(args);

	return(ui_button_node(ui, text_buffer));
}

	inline u32
ui_button(game_ui *ui, u8 *text)
{

	ui_node *node = ui_create_node(ui,
			node_clickeable |
			node_background |
			node_hover_animation |
			node_active_animation |
			node_text |
			node_text_centered |
			node_use_extra_flags,
			text);

	i32 interacting = ui_node_mouse_l_up(ui, node); 
	return(interacting);
}

	static inline u32
ui_buttonf(game_ui *ui, u8 *text, ...)
{
	u8 text_buffer[256] = {0};

	va_list args;
	va_start_m(args, text);
	format_text_list(text_buffer, sizeof(text_buffer), text, args);
	va_end_m(args);

	return(ui_button(ui, text_buffer));
}

	static inline ui_node * 
ui_box_region(game_ui *ui, u8 *text)
{
	ui_node *node = ui_create_node(ui,
			node_background | 
			node_border,
			text);
	return(node);
}

//simple scroll bar inside a given region
//draws and calculates a scroll bar based on the given scroll value.
//returns a new scroll delta.
#define ui_node_scroll_delta_y(ui, node)\
	ui_calculate_scroll_value(ui, node->size_y, node->content_size[ui_axis_y])
	static inline f32
ui_calculate_scroll_value(
		game_ui *ui,
		f32 frame_h,
		f32 total_node_content)
{
	i16 content_inside_frame_y = (i16)total_node_content;
	f32 content_difference = 1.0f;
	if(content_inside_frame_y > frame_h)
	{
		content_difference = frame_h / content_inside_frame_y;
	}

	f32 sv = -ui->input.mouse_wheel / content_difference * 10;
	return(sv);
}


//"panel" with a scrollable region
//#define ui_content_box_be(ui, label) ui_node *___cp___ = ui_content_box_begin(ui, label); \
//												   ui_DEFER_LOOP(1, ui_content_box_end(ui, ___cp___))


static inline f32
ui_scroll_area_vertical(
		game_ui *ui,
		ui_node *node)
{
	ui_node *scroll_region;
	ui_push_id_node(ui, node);
	ui_push_parent(ui, node);

	ui_set_interaction_layer(ui, ui_interaction_layer_mid)
	{
		scroll_region = ui_create_node(
				ui,
				node_clickeable |
				node_skip_layout_x |
				node_skip_layout_y |
				node_skip_content_size_x |
				node_skip_content_size_y,
				"__SCROLL_REGION__"
				);
	}
	ui_pop_id(ui);
	ui_pop_last_parent(ui);

	b32 scroll_by_wheel = ui_node_mouse_hover(ui, scroll_region);
	f32 sv = 0;
	if(scroll_by_wheel)
	{
		sv = ui_calculate_scroll_value(
				ui,
				node->size_y,
				node->content_size[ui_axis_y]);
		ui_node_change_target_scroll(node, sv, ui_axis_y);
	}

	return(sv);
}


#define ui_content_box_be_ex(ui, extra_flags, label) for(struct __s__tg {ui_node *n; int i;}\
		__ssc__ = {ui_content_box_begin_ex(ui, extra_flags,label), 0};\
		!__ssc__.i;\
		__ssc__.i += 1, ui_content_box_end(ui, __ssc__.n))

#define ui_content_box_be(ui, label) ui_content_box_be_ex(ui, 0, label)
#define ui_content_box_begin(ui, label) ui_content_box_begin_ex(ui, 0, label)

	static inline ui_node * 
ui_content_box_ex(
		game_ui *ui,
		ui_node_flags extra_flags,
		u8 *label)
{
//	ui_node *scroll_region;
	ui_node *background;

	ui_push_id_string(ui, label);
	background = ui_create_node(ui,
			node_clickeable | node_use_extra_flags | node_background | node_border | extra_flags | node_scroll_y,
			"__BACKGROUND__");
	b32 scroll_by_wheel = 0;
	ui_node *scroll_set_node = 0;
	background->padding_x = 6;
	background->padding_y = 6;

	ui_set_parent(ui, background)
	{

#if 0
		if(extra_flags & node_scroll_y)
		{
			ui_space_specified(ui, ui_SZ_SCROLL_WH, 1.0f);

			ui_node *scp = 0;
			ui_set_wh_ppct(ui, 1.0f, 0.0f)
				scp = ui_create_node(ui, node_skip_layout_x | node_skip_layout_y, 0);
			ui_set_w_specified(ui, ui_SZ_SCROLL_WH, 1.0f) ui_set_parent(ui, scp) ui_set_row(ui)
			{
				ui_space_ppct(ui, 1.0f, 0.0f);
				f32 scroll_delta = ui_scroll_vertical(
						ui,
						background->size[1] - 12.0f,
						background->content_size[1],
						background->scroll_y
						);
				if(scroll_delta)
				{
					ui_node_change_target_scroll(background, scroll_delta, ui_axis_y);
				}
			}
			ui_set_w_specified(ui, background->size_x, 1.0f)
				ui_set_h_specified(ui, background->size_y, 1.0f)
				scroll_region = ui_mid_region(ui);

			scroll_by_wheel = ui_node_mouse_hover(ui, scroll_region);
		}
		else
		{
		}
		ui_pop_row(ui);

		ui_space_specified(ui, 6.0f, 1.0f);
#endif
	}

#if 0
	if(scroll_by_wheel)
	{

		f32 sv = ui_calculate_scroll_value(
				ui,
				background->size_y,
				background->content_size[ui_axis_y]);
		ui_node_change_target_scroll(background, sv, ui_axis_y);
	}
#endif
	return(background);
}


	static inline ui_node *
ui_content_box_begin_ex(
		game_ui *ui,
		ui_node_flags extra_flags,
		u8 *label)
{
	ui_node *content_box = ui_content_box_ex(
			ui, extra_flags, label);
	ui_push_id(ui, content_box->id);
	ui_push_parent(ui, content_box);

	return(content_box);
}

	static inline void
ui_content_box_end(
		game_ui *ui,
		ui_node *child_panel_node)
{
	ui_pop_id(ui);
	ui_pop_parent(ui, child_panel_node);
}


//#define ui_content_box_be_ex(ui, extra_flags, label) for(struct __s__tg {ui_node *n; int i;}\
//		__ssc__ = {ui_content_box_begin_ex(ui, extra_flags,label), 0};\
//		                           !__ssc__.i;\
//								   __ssc__.i += 1, ui_content_box_end(ui, __ssc__.n))


	static inline void
ui_context_menu_open(game_ui *ui, i16 x, i16 y, ui_node *node)
{
	node->context_menu_active = 1;
	node->context_menu_got_opened = 1;
	ui_panel *context_menu_root = ui_push_or_get_panel_id(ui, 0, node->id);
	context_menu_root->p.x = x;
	context_menu_root->p.y = y;
}


	static inline ui_popup *
ui_look_for_opened_popup(game_ui *ui, ui_id id)
{
	ui_popup *opened_popup = ui->first_open_popup;
	if(opened_popup)
	{
		//look if the popup is already opened
		while(opened_popup && !ui_id_EQUALS(id, opened_popup->id))
		{
			opened_popup = opened_popup->next;
		}
		if(opened_popup)
		{
			opened_popup->last_touched_frame = ui->current_frame;
		}
	}
	ui->last_looked_popup_was_opened = opened_popup != 0;
	return(opened_popup);
}


static inline ui_popup * 
ui_popup_open_new(game_ui *ui, i16 x, i16 y, ui_id id)
{
	ui_popup *opened_popup = 0;
	ui_popup *previous = 0;
	//not opened
	//open new popup
	//linear search for a closed popup.
	u32 p = 0;
	while(!opened_popup && p < ui->popup_array_max)
	{
		if(!ui->popup_array[p].active)
		{
			opened_popup = ui->popup_array + p;
		}
		p++;
	}

	opened_popup->active = 1;
	opened_popup->last_touched_frame = 0;
	opened_popup->id = id;
	opened_popup->x = x;
	opened_popup->y = y;
	if(ui->first_open_popup)
	{
		ui->first_open_popup->prev = opened_popup;
	}
	opened_popup->next = ui->first_open_popup;
	ui->first_open_popup = opened_popup;
	ui->opened_popups_count++;

	return(opened_popup);
}

	static inline void
ui_popup_close_struct(game_ui *ui, ui_popup *opened_popup)
{
	if(opened_popup)
	{
		if(opened_popup->prev)
		{
			opened_popup->prev->next = opened_popup->next;
		}
		if(opened_popup->next)
		{
			opened_popup->next->prev = opened_popup->prev;
		}
		if(ui->first_open_popup == opened_popup)
		{
			ui->first_open_popup = opened_popup->next;
		}
		opened_popup->active = 0;
		opened_popup->next = 0;
		opened_popup->prev = 0;
		ui->opened_popups_count--;
	}
}

	static inline void
ui_popup_close(game_ui *ui, ui_id id)
{
	ui_popup *opened_popup = ui_look_for_opened_popup(ui, id);
	if(opened_popup)
	{
		ui_popup_close_struct(ui, opened_popup);
	}
}

static inline void
ui_popup_close_last(game_ui *ui)
{
	if(ui->first_open_popup)
	{
		ui_popup_close(ui, ui->first_open_popup->id);
	}
}

#define ui_popup_open_mouse(ui, id) \
	ui_popup_open(ui, (i16)ui->mouse_point.x, (i16)ui->mouse_point.y, id)
	static inline ui_popup *
ui_popup_open(game_ui *ui, i16 x, i16 y, ui_id id)
{
	ui_popup *opened_popup = ui_look_for_opened_popup(ui, id);
	if(!opened_popup)
	{
		opened_popup = ui_popup_open_new(ui, x, y, id);
	}
	return(opened_popup);
}

	static inline void 
ui_popup_open_or_close(game_ui *ui, i16 x, i16 y, ui_id id)
{
	ui_popup *opened_popup = ui_look_for_opened_popup(ui, id);
	if(opened_popup)
	{
		ui_popup_close_struct(ui, opened_popup);
	}
	else
	{
		ui_popup_open_new(ui, x, y, id);
	}
}

#define ui_popup(ui, id) for(u8 __I__ = (b8)ui_popup_begin(ui, id);\
		__I__ == 1 || (!__I__ ? ui_popup_end(ui) : 0);\
		__I__ += 1, ui_popup_end(ui))

	static b32 
ui_popup_begin(game_ui *ui, ui_id id)
{
	ui_panel *cm_panel = ui_push_root_for_rendering_id(
			ui,
			0, 0, 0, 0,
			ui_panel_flags_init_once | 
			ui_panel_flags_move | 
			ui_panel_flags_respect_layout | 
			ui_panel_flags_keep_on_front | 
			ui_panel_flags_init_closed,
			id);

	ui_popup *popup = ui_look_for_opened_popup(ui, id);
	if(popup)
	{
		cm_panel->root_node->added_x = popup->x;
		cm_panel->root_node->added_y = popup->y;
	}

	b32 is_opened = !cm_panel->closed;
	//interactions
	if(popup && cm_panel->closed)
	{
		ui_open_panel_ptr(ui, cm_panel);
	}
	else if(!popup && !cm_panel->closed)
	{
		ui_close_panel_ptr(ui, cm_panel);
	}

	return(is_opened);
}


	static inline b32 
ui_popup_end(game_ui *ui)
{
	ui_pop_root(ui);
	return(0);
}

#define ui_context_menu(ui, id) for(u8 __I__ = (b8)ui_context_menu_begin(ui, id);\
		__I__ == 1 || (!__I__ ? ui_context_menu_end(ui) : 0);\
		__I__ += 1, ui_context_menu_end(ui))

static b32
ui_context_menu_begin(game_ui *ui, ui_id popup_id)
{
	b32 opened = ui_popup_begin(ui, popup_id);
	if(opened)
	{

		ui_next_nodes_interaction_only_begin(ui);
		ui_node *region_node = 0;
		ui_node *interact_area = 0;
		interact_area = ui_interact_mid(ui, "__CM_INTERACTION_AREA__");

		ui_push_parent(ui, interact_area);
		{
			region_node = ui_create_node(ui, 0, 0);
		}
		ui_pop_last_parent(ui);
		//close popup if clicked outsude
		if(!ui_node_mouse_hover(ui, interact_area) && input_pressed(ui->input.mouse_left))
		{
			opened = 0;
			ui_popup_close(ui, popup_id);
		}
		else
		{
			ui_push_parent(ui, region_node);
		}
	}
	if(!opened)
	{

		ui_next_nodes_interaction_only_end(ui);
		ui_push_parent(ui, &ui->void_node);
	}
	return(opened);
}

static b32
ui_context_menu_end(game_ui *ui)
{
	ui_pop_last_parent(ui);
		ui_next_nodes_interaction_only_end(ui);
	ui_popup_end(ui);
	return(0);
}


	static inline b32 
ui_popup_defer_loop_check(
		game_ui *ui,
		ui_node *popup_node)
{
	if(popup_node->popup_opened)
	{
		return(1);
	}
	else
	{
		ui_pop_parent(ui, popup_node);
		ui_pop_root(ui);
		return(0);
	}
}






/*
   This is an useful function to try to recreate the behaviour of text
   input in most applications, it's also useful to take as reference and learn
   about the API usage. Maybe for future when adapting this to other engine...
   since this is not "perfect".
   */

	static inline u32
_ui_update_input_text_from_node(game_ui *ui,
		ui_node *text_box_node,
		bool32 confirm_on_enter,
		bool32 text_centered,
		u32 target_buffer_size,
		u32 *target_buffer_count,
		u8 *target_buffer)
{
	ui_push_parent(ui, text_box_node);
	ui_push_id_node(ui, text_box_node);
	ui_id input_text_id = text_box_node->id;

	u32 text_modified = 0;
	s_input_text *input_text = ui->input_text; 

	bool32 interacting = ui_node_interacting(ui, text_box_node);
	//flip the cursor locations depending on the selection
	u32 text_cursor_start = input_text->cursor_position_l; 
	u32 text_cursor_end = input_text->cursor_position_r;
	if(input_text->cursor_position_l >= input_text->cursor_position_r)
	{
		text_cursor_start = text_cursor_end;
		text_cursor_end   = input_text->cursor_position_l;
	}

	//reset the timer if the input cursor moved
	if(ui->input_text->cursor_moved ||
			ui->input_text->last_key_count != ui->input_text->key_count)
	{
		ui->input_text_timer = 0;
	}
	//if interacting, focus the input text on this id for the next frame
	b32 input_text_focused = ui_input_text_is_focused(ui, input_text_id);
	if(interacting && !input_text_focused)
	{
		ui_input_text_keep_interaction(ui, input_text_id);
	}

	u8 *preview_buffer = target_buffer;
	if(input_text_focused)
	{
		//keep the input text interaction
		ui_input_text_keep_interaction(ui, input_text_id);

		bool32 hot = ui_node_mouse_hover(ui, text_box_node);
		//input text region interaction node
		ui_interaction_info it_info = ui_interaction_from_node(ui, text_box_node);
		b32 it_mouse_l_down = it_info.flags & ui_interaction_mouse_left_down;
		b32 it_mouse_l_pressed = it_info.flags & ui_interaction_mouse_left_pressed;
		b32 it_mouse_l_double_click = it_info.flags & ui_interaction_mouse_left_double_click;
		b32 it_mouse_l_tripple_click = it_info.flags & ui_interaction_mouse_left_tripple_click;
		b8 mouse_l_pressed = (b8)input_pressed(ui->input.mouse_left);
		b32 got_clicked = it_mouse_l_pressed;
		b8 input_text_got_canceled = 0;
		//Tell to process input to it's buffer
		input_text->focused = 1;
		//Confirm changes
		//Use the buffer from s_input_text as backup until enter is pressed
		if(confirm_on_enter)
		{
			if(input_text->target_buffer != ui->input_text_buffer)
			{
				input_text_set_target(
						input_text,
						ui->input_text_buffer,
						target_buffer_count,
						sizeof(ui->input_text_buffer));
				string_copy(
						target_buffer,
						ui->input_text_buffer);
			}
			preview_buffer = ui->input_text_buffer;
		}
		else
		{
			if(input_text->target_buffer != target_buffer)
			{
				//Just modify the target buffer
				input_text_set_target(
						input_text,
						target_buffer,
						target_buffer_count,
						target_buffer_size);
			}
		}
		//If clicked anywere else 
		if(mouse_l_pressed && !hot)
		{
			//restore interactions with other widgets
			ui->keep_interaction = 0;
//			ui_cancel_interaction(ui, ui_interaction_layer_default);

			ui_cancel_input_text_interaction(ui);
			//reset to default buffer
			input_text_restore_buffer(ui->input_text);

			it_mouse_l_down = 0;
			it_mouse_l_pressed = 0;
			it_mouse_l_double_click = 0;
			it_mouse_l_tripple_click = 0;
			got_clicked = 0;
			input_text_got_canceled = 1;
		}
		//Target to text buffer
		bool32 got_focus = !input_text->got_focus || ui->interactions[ui_interaction_layer_default].node_transition;
		if(got_focus)
		{
			//u32 target_bufferCount = string_count(target_buffer);
			//input_text->key_count = string_copy_and_clear(target_buffer, input_text->buffer, ARRAYCOUNT(input_text->buffer));
			//re-position the cursor as below with a function
		}



		//just got focus or got clicked
		if(got_clicked || ui->input_text_interaction_transition)
		{
			f32 mouseDistanceFromInputX = ui->mouse_point.x - text_box_node->region.x0;
			if(it_mouse_l_double_click && !it_mouse_l_tripple_click)
			{
				/*When double clicking a white space, the left cursor will
				  go back until the first ' 'character, and the right cursor
				  will advance until the last ' '*/
				//Get both cursors at ' ' character
				input_text->cursor_position_l = string_get_previous_char_index_from(
						preview_buffer,
						ui->input_text->cursor_position_l,
						' ');
				input_text->cursor_position_r = string_get_next_char_index_from(preview_buffer,
						ui->input_text->cursor_position_r,
						' ');

				if(input_text->cursor_position_l &&
						preview_buffer[input_text->cursor_position_l + 1] == ' ')
				{
					while(preview_buffer[input_text->cursor_position_l] == ' ')
					{
						input_text->cursor_position_l--;
					}
				}
				if(input_text->cursor_position_r && 
						input_text->cursor_position_r < input_text->key_count &&
						preview_buffer[input_text->cursor_position_r - 1] == ' ')
				{
					while(preview_buffer[input_text->cursor_position_r] == ' ')
					{
						input_text->cursor_position_r++;
					}
				}
				//The normal behaviour of text input is to set the cursor position left
				//after the ' ' character, so if the cursor position left sits at said character.
				//I just advance it by one
				if(ui->input_text->cursor_position_l)
				{
					ui->input_text->cursor_position_l++;
				}
				text_cursor_start = ui->input_text->cursor_position_l;
				text_cursor_end = ui->input_text->cursor_position_r;			  
			}
			else if(it_mouse_l_tripple_click)
			{
				ui->input_text->cursor_position_l = 0;
				ui->input_text->cursor_position_r = ui->input_text->key_count;
			}
			else
			{
				//single click
				//Doesn't support multi-line text yet
				u32 finalCursorIndex = font_get_closest_index_at(&ui->fontp,
						preview_buffer,
						ui->font_scale,
						mouseDistanceFromInputX);
				text_cursor_start = finalCursorIndex;
				text_cursor_end = finalCursorIndex;
				ui->input_text->cursor_position_l = finalCursorIndex;
				ui->input_text->cursor_position_r = finalCursorIndex;
			}
		}
		ui->process_hot_nodes = 1;
		if(it_mouse_l_down)
		{
			ui->process_hot_nodes = 0;
		}

		i32 textWidthFromStart = (i32)ui_get_remaining_text_width_at(ui, 0, text_cursor_start, preview_buffer);

		i32 selectionWidth = (i32)ui_get_remaining_text_width_at(ui, text_cursor_start, text_cursor_end, preview_buffer);
		i32 cursorW = MAX(2, selectionWidth);
		//Input cursor
		//make the cursor "blink" if the timer is more than half the total time
		f32 inputCursorAlpha = 0xff;
		if(text_cursor_start == text_cursor_end)
		{
			inputCursorAlpha = 
				(ui->input_text_timer > (ui_INPUT_CURSOR_TIMER_TOTAL * 0.5f)) ? 0.0f : 0xff;
		}
		//render cursor
		f32 font_height      = ui->fontp.font_height * ui->font_scale;
		i32 x0 = textWidthFromStart + 0;
		i32 y0 = 0;
		i32 x1 = x0 + cursorW;
		i32 y1 = y0 + (i32)font_height;
		ui_node_push_rectangle(
				ui,
				text_box_node,
				x0,
				y0,
				x1,
				y1,
				V4(200, 200, 200, inputCursorAlpha));
		//render the buffer text, else the one to modify

		text_modified = 0;
		bool32 entered = input_text->entered;
		ui->input_text_entered = entered;
		//TODO: Apply confirm on enter
		if(confirm_on_enter)
		{
			if(entered)
			{
				string_copy(ui->input_text_buffer,
						target_buffer);
				text_modified = 1; 
			}
		}
		else
		{
			text_modified = input_text->last_key_count != input_text->key_count;
		}
		if(ui->input_text_interaction_transition || input_text_got_canceled)
		{
			text_modified = 0;
		}
	}

	ui_node_push_text(ui,
			text_box_node,
			0,
			0,
			text_centered,
			vec4_all(255),
			preview_buffer);

	ui_pop_id(ui);
	ui_pop_parent(ui, text_box_node);

	return(text_modified);
}

	static inline u32 
ui_input_text(game_ui *ui,
		bool32 confirm_on_enter,
		u8 *target_buffer,
		u32 target_buffer_size,
		u8 *text_label_id)
{

	ui_node *text_box_node = ui_create_node(ui,
			node_background |
			node_clickeable |
			node_border,
			text_label_id);

	u32 text_modified = _ui_update_input_text_from_node(ui,
			text_box_node,
			confirm_on_enter,
			0,
			target_buffer_size,
			0,
			target_buffer);

	return(text_modified);
}



	static inline u32 
ui_input_type(game_ui *ui,
		ui_node *text_box_node,
		bool32 confirm_on_enter,
		bool32 center_text,
		u8 *text_buffer_to_edit,
		u8 *text_label_id)
{
	u8 value_buffer[64] = {0};
	u8 *text_to_edit = text_buffer_to_edit;
	u32 text_to_edit_size = 64;

	bool32 interacting = ui_input_text_is_focused(ui, text_box_node->id);

	if(!interacting)
	{
	}
	else if(interacting)
	{
		if(ui->input_text_interaction_transition)//ui->interactions[ui_interaction_layer_default].node_transition)
		{
			string_copy_and_clear(
					text_buffer_to_edit,
					ui->input_text_buffer, 
					sizeof(ui->input_text_buffer));
		}
		text_to_edit = ui->input_text_buffer;
		text_to_edit_size = sizeof(ui->input_text_buffer);
	}

	u32 text_modified = _ui_update_input_text_from_node(ui,
			text_box_node,
			confirm_on_enter,
			center_text,
			text_to_edit_size,
			0,
			text_to_edit);

	return(text_modified);

}

#define ui_input_TYPE_PARAMS(type) (game_ui *ui,\
		bool32 confirm_on_enter,\
		type *target_value,\
		u8 *text_label_id)


	static inline ui_node * 
ui_input_u32 ui_input_TYPE_PARAMS(u32)
{
	u32 value = 0;
	u8 value_buffer[64] = {0};

	ui_node *text_box_node = ui_create_node(ui,
			node_background |
			node_clickeable |
			node_border,
			text_label_id);

	if(target_value)
	{
		value = *target_value;
		format_text(value_buffer, sizeof(value_buffer), "%u", value);
	}

	u32 text_modified = 
		ui_input_type(ui,
				text_box_node,
				confirm_on_enter,
				0,
				value_buffer,
				text_label_id);


	if(target_value && text_modified)
	{
		u32_from_string(ui->input_text_buffer, target_value);
	}

	return(text_box_node);
}

	static inline ui_node * 
ui_input_u16 ui_input_TYPE_PARAMS(u16)
{
	u32 value_32 = *target_value;
	ui_node *text_box_node = ui_input_u32(ui, confirm_on_enter, &value_32, text_label_id);
	*target_value = value_32;
	return(text_box_node);
}

	static inline ui_node * 
ui_input_f32 ui_input_TYPE_PARAMS(f32)
{
	f32 value = 0;
	u8 value_buffer[64] = {0};

	ui_node *text_box_node = ui_create_node(ui,
			node_background |
			node_clickeable |
			node_border,
			text_label_id);

	if(target_value)
	{
		value = *target_value;
		format_text(value_buffer, sizeof(value_buffer), "%f", value);
	}

	bool32 text_modified = 
		ui_input_type(ui,
				text_box_node,
				confirm_on_enter,
				0,
				value_buffer,
				text_label_id);


	if(target_value && text_modified)
	{
		f32_from_string(ui->input_text_buffer, target_value);
	}

	return(text_box_node);
}

	static inline ui_node * 
ui_input_i32 ui_input_TYPE_PARAMS(i32)
{
	i32 value = 0;
	u8 value_buffer[64] = {0};

	ui_node *text_box_node = ui_create_node(ui,
			node_background |
			node_clickeable |
			node_border,
			text_label_id);

	if(target_value)
	{
		value = *target_value;
		format_text(value_buffer, sizeof(value_buffer), "%d", value);
	}

	bool32 text_modified = 
		ui_input_type(ui,
				text_box_node,
				confirm_on_enter,
				0,
				value_buffer,
				text_label_id);


	if(target_value && text_modified)
	{
		i32_from_string(ui->input_text_buffer, target_value);
	}

	return(text_box_node);
}

	static inline u32
ui_button_down(game_ui *ui, u8 *text)
{
	return(0);
}

	inline i32 
ui_button_toggle(game_ui *ui, u32 *boolean, u8 *text)
{
	return(0);
}

	inline u32
ui_button_image_frames(game_ui *ui,
		f32 buttonW,
		f32 buttonH,
		render_texture *texture,
		u32 frame_x,
		u32 frame_y,
		u32 frame_w,
		u32 frame_h)
{
	return(0);
}

//this is a goddamn button...
	inline u32
ui_selectable_image_uv(game_ui *ui,
		u32 active,
		render_texture *texture,
		f32 element_w,
		f32 element_h,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
	return(0);
}

	inline u32
ui_selectable_image_uv_(game_ui *ui,
		render_texture *texture,
		u32 active,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label)
{
	ui_node *n = ui_create_node(
			ui,
			node_clickeable |
			node_background |
			node_border |
			node_hover_animation |
			node_active_animation,
			label);

	ui_node_push_image(
			ui,
			n,
			texture,
			0,
			0,
			uv0,
			uv1,
			uv2,
			uv3);
	return(0);
}

	inline u32
ui_selectable_image_uvf(game_ui *ui,
		render_texture *texture,
		u32 active,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label,
		...)
{

	u8 buffer[256] = {0};
	va_list args;
	va_start_m(args, label);
	format_text_list(buffer, sizeof(buffer), label, args);
	va_end_m(args);

	return(ui_selectable_image_uv_(
				ui,
				texture,
				active,
				uv0,
				uv1,
				uv2,
				uv3,
				buffer));
}

	inline ui_node * 
ui_image_uvs(game_ui *ui,
		render_texture *texture,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
	ui_node *n = ui_create_node(
			ui,
			node_background |
			node_border,
			0);

	ui_node_push_image(
			ui,
			n,
			texture,
			0,
			0,
			uv0,
			uv1,
			uv2,
			uv3);
	return(0);
}

	static inline ui_node *
ui_button_image_uvs_node(game_ui *ui,
		render_texture *texture,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label)
{
	ui_node *n;
	ui_node *n_background;

	n_background = ui_create_node(
			ui,
			0,
			0);

	ui_node_push_image(
			ui,
			n_background,
			texture,
			0,
			0,
			uv0,
			uv1,
			uv2,
			uv3);
	ui_set_color(ui, ui_color_background, vec4_all(0))
		ui_set_parent(ui, n_background)
		{
			n = ui_create_node(
					ui,
					node_clickeable |
					node_hover_animation |
					node_active_animation |
					node_background |
					node_border,
					label);
		}
	return(n);
}

	static inline ui_node *
ui_button_image_uvs_nodef(game_ui *ui,
		render_texture *texture,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label,
		...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, 256, label);
	return(ui_button_image_uvs_node(
				ui,
				texture,
				uv0,
				uv1,
				uv2,
				uv3,
				text_buffer));
}

	static inline u32
ui_button_image_uv(game_ui *ui,
		render_texture *texture,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label)
{
	ui_node *n;
	ui_node *n_background;

	n_background = ui_create_node(
			ui,
			0,
			0);

	ui_node_push_image(
			ui,
			n_background,
			texture,
			0,
			0,
			uv0,
			uv1,
			uv2,
			uv3);
	ui_set_color(ui, ui_color_background, vec4_all(0))
		ui_set_parent(ui, n_background)
		{
			n = ui_create_node(
					ui,
					node_clickeable |
					node_hover_animation |
					node_active_animation |
					node_background |
					node_border,
					label);
		}
	return(ui_node_mouse_l_up(ui, n));
}

	static inline u32
ui_button_image_uvf(game_ui *ui,
		render_texture *texture,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u8 *label,
		...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, 256, label);
	return(ui_button_image_uv(
				ui,
				texture,
				uv0,
				uv1,
				uv2,
				uv3,
				text_buffer));
}


	inline i32
ui_image_frames(game_ui *ui, u8 *label, render_texture *texture, f32 scaleX, f32 scaleY)
{
	return(0);
}

//Define the function by concadenating te type
#define ui_drag_PARAMS(type) (game_ui *ui, type inc_dec, type min, type max, type *value, u8 *label)
#define UI_DRAG_NAME_TYPE(name, type)\
	ui_drag_ ##name(game_ui *ui, type inc_dec, type min, type max, type *value, u8 *text)
#define UI_DRAG_TYPE(type) static inline \
	ui_drag_ ##type(game_ui *ui, type inc_dec, type min, type max, type *value, u8 *text)
#define UI_DRAG_TYPE_FORMAT(type) static inline \
	ui_drag_ ## type ##_format(game_ui *ui, type inc_dec, type min, type max, type *value, u8 *text, u8 *format)

#define UI_DRAGGABLEX_ADDDEC_(value, inc_dec) value += inc_dec; value = value <= min ? min : value >= max ? max : value;
#define UI_DRAGGABLEX_ADDDEC_u32(value, inc_dec) (inc_dec < 0 && inc_dec > value) ? 0 : (inc_dec > 0 &&  inc_dec > U32MAX - value) U32MAX : value + inc_dec;
#define UI_DRAGGABLEX_ADDDEC(type, value, inc_dec) UI_DRAGGABLEX_ADDDEC_(*(type *)value, inc_dec)

	inline u32 
ui_push_drag(game_ui *ui, void *value, u32 type, u8 *text, u8 *format)
{
	return(0);
}

UI_DRAG_TYPE_FORMAT(u32)
{
	return(0);
}
UI_DRAG_TYPE(u32)
{
	return(0);
}

UI_DRAG_TYPE_FORMAT(u16)
{
	return(0);
}
UI_DRAG_TYPE(u16)
{
	return(0);
}

UI_DRAG_TYPE_FORMAT(f32)
{
	return(0);
}

	static inline ui_node * 
_ui_drag_node_type(game_ui *ui,
		i32 *wheel,
		u8 *label)
{
	ui_node *drag_node = ui_create_node(ui,
			node_use_extra_flags | node_background | node_text | node_clickeable, label);
	//	ui_node_set_display_string(ui, display_string, f_value);

	ui_push_id(ui, drag_node->id);
	u8 f_value[64] = {0};

	ui_usri drag_usri = ui_usri_from_node(ui, drag_node);
	if(ui_usri_mouse_l_down(drag_usri))
	{
		i32 dt_sign = (i32)ui_mouse_delta(ui).x;
		*wheel = dt_sign;
	}
	ui_pop_id(ui);
	return(drag_node);
}

ui_drag_f32 ui_drag_PARAMS(f32)
{
	i32 wheel = 0;
	ui_node *drag_node = _ui_drag_node_type(ui, &wheel, label);

	u8 f_value[16] = {0};
	b32 changed = 0;
	if(value)
	{
		f32 value_before = *value;
		format_text(f_value, 64, "%f", *value);

		if(wheel)
		{
			*value += inc_dec * wheel;
			*value = *value < min ? min : *value > max ? max : *value;
		}

		changed = value_before != *value;
	}
	ui_node_set_display_string(ui, drag_node, f_value);
	return(changed);
}

ui_drag_i32 ui_drag_PARAMS(i32)
{
	i32 wheel = 0;
	ui_node *drag_node = _ui_drag_node_type(ui, &wheel, label);

	u8 f_value[16] = {0};
	if(value)
	{
		format_text(f_value, 64, "%d", *value);

		if(wheel)
		{
			*value += inc_dec * wheel;
			*value = *value < min ? min : *value > max ? max : *value;
		}

	}
	ui_node_set_display_string(ui, drag_node, f_value);
}

UI_DRAG_NAME_TYPE(radians, f32)
{
	return(0);
}


#define ui_keep_line_and_wrap(ui) _ui_keep_line_push(ui, 1)
#define ui_keep_line_push(ui) _ui_keep_line_push(ui, 0)

#define ui_set_disable_if(ui, condition) ui_DEFER_LOOP(ui_push_disable_if(ui, condition), ui_pop_disable(ui))
	inline void
ui_push_disable_if(game_ui *ui, bool32 condition)
{
	ui->pushed_disable_count += (condition != 0);
	ui->next_node_disabled = ui->pushed_disable_count != 0;
}
	static inline void
ui_pop_disable(game_ui *ui)
{
	if(ui->pushed_disable_count)
	{
		ui->pushed_disable_count--;
		ui->next_node_disabled = ui->pushed_disable_count != 0;
	}
}
#define ui_push_enable_if(ui, condition) ui->next_node_disabled = !condition
#define ui_pop_enable(ui, condition) ui->next_node_disabled = ui->pushed_disable_count != 0

	inline vec2
ui_get_remaining_frame_size(game_ui *ui)
{
	return(V2(0, 0));
}


#define UI_SELECTABLEIMAGE_TYPE(type) ui_image_selection_ ##type()(game_ui *ui, u32 pixelX, u32 pixelY, u32 pixelW, u32 pixelH, render_texture *texture, type *selectionX, type *selectionY, type *selectionW, type *selectionH)

	inline ui_element_persistent *
_ui_push_selectable_image(game_ui *ui,
		render_texture *texture,
		u32 tileSize_w,
		u32 tileSize_h,
		u32 showGrid,
		u8 *label)

{
}
	static u32
ui_image_selection_u32(game_ui *ui,
		render_texture *texture,
		u32 *selectionX,
		u32 *selectionY,
		u32 *selectionW,
		u32 *selectionH,
		u32 tileSize_w,
		u32 tileSize_h,
		u32 showGrid,
		u8 *label)
{
}

	static u32
ui_image_selection_u16(game_ui *ui,
		render_texture *texture,
		u16 *selectionX,
		u16 *selectionY,
		u16 *selectionW,
		u16 *selectionH,
		u32 tileSize_w,
		u32 tileSize_h,
		u32 showGrid,
		u8 *label)
{
}

	static u32
ui_image_selection_uv_min_max(game_ui *ui,
		render_texture *texture,
		vec2 *uv_min,
		vec2 *uv_max,
		u32 tileSize_w,
		u32 tileSize_h,
		u32 showGrid,
		u8 *label)
{
}


	static inline void 
ui_image_selection_region(game_ui *ui,
		render_texture *texture,
		u32 grid_w,
		u32 grid_h,
		b32 display_grid,
		u32 total_uvs_count,
		vec2 **uvs,
		u8 *label) 
{

	ui_node *selection_region_node;
	ui_node *top_bar_node;

	ui_node *cont = ui_create_node(
			ui,
			node_background |
			node_border |
			node_clip,
			0);

	bool32 flip_uvs_h = 0;
	bool32 new_selection = 0;

	f32 scroll_dt_x = 0;
	f32 scroll_dt_y = 0;
	ui_push_id_string(ui, label);
	ui_set_parent(ui, cont)
	{
		//top bar
		ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
			ui_set_w_ppct(ui, 1.0f, 1.0f)
			ui_set_color(ui, ui_color_background, V4(0x2c, 0x2c, 0x30, 0xff))
			{
				top_bar_node = ui_create_node(
						ui,
						node_background,
						0);
			}
		//contents like spacing, image... and where scroll occurs
		ui_node *n;
		ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 0.0f))
		{
			n = ui_create_node(
					ui,
					node_background |
					node_border |
					node_scroll_x |
					node_scroll_y |
					node_clip,
					label);

			scroll_dt_x = n->target_scroll[ui_axis_x];
			scroll_dt_y = n->target_scroll[ui_axis_y];
		}

		vec2 uv0 = {0.0f, 1.0f};
		vec2 uv1 = {0.0f, 0.0f};
		vec2 uv2 = {1.0f, 0.0f};
		vec2 uv3 = {1.0f, 1.0f};
		//n->zoom = 2.0f;
		bool32 region_hot = 0;


		if(n->zoom == 0)
		{
			n->zoom = 1;
		}

		ui_set_parent(ui, n)
		{
			//			ui_set_interaction_layer(ui, ui_interaction_layer_mid)
			ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 1.0f))
			{
				selection_region_node = ui_create_node(
						ui,
						node_clickeable |
						node_skip_layout_x |
						node_skip_layout_y,
						"S_IMAGE_REGION");
			}

			//interact and modify zoom or scroll before interacting with the rest.
			ui_usri region_interaction = ui_interaction_from_node(ui, selection_region_node);

			f32 old_zoom = n->zoom;
			if(ui_usri_mouse_hover(region_interaction))
			{
				region_hot = 1;
				n->selection_region_hot = 1;
				n->zoom += ui->input.mouse_wheel;
				n->zoom = n->zoom < 1 ? 1 : n->zoom > 12 ? 12 : n->zoom;
				n->zoom = (i16)n->zoom;

				if(ui->input.mouse_wheel)
				{
					f32 cursor_image_delta_x = ((ui->mouse_point.x) - (n->region.x0 - scroll_dt_x));
					f32 cursor_image_delta_y = ((ui->mouse_point.y) - (n->region.y0 - scroll_dt_y)); 
					f32 scaled_zoom = n->zoom / old_zoom;
					scroll_dt_x = (f32)(i32)(1 * ((cursor_image_delta_x * scaled_zoom) - (ui->mouse_point.x - n->region.x0)));
					scroll_dt_y = (f32)(i32)(1 * ((cursor_image_delta_y * scaled_zoom) - (ui->mouse_point.y - n->region.y0)));
				}
				if(ui_usri_mouse_m_down(region_interaction))
				{
					vec2 mouse_delta = ui_mouse_delta(ui);
					scroll_dt_x -= mouse_delta.x;
					scroll_dt_y -= mouse_delta.y;
					scroll_dt_x = (f32)(i32)scroll_dt_x;
					scroll_dt_y = (f32)(i32)scroll_dt_y;
					scroll_dt_x = scroll_dt_x < 0 ? 0 : scroll_dt_x;
					scroll_dt_y = scroll_dt_y < 0 ? 0 : scroll_dt_y;
				}
			}
			i32 zoom = n->zoom;
			i32 image_spacing = 8 * zoom;


			ui_space_specified(ui, image_spacing, 1.0f);
			//draws image/background/selections
			ui_node *image_node;

			i16 img_x0 = 0;
			i16 img_y0 = 0;
			i16 img_x1 = 0;
			i16 img_y1 = 0;

			ui_set_row(ui)
			{
				ui_space_specified(ui, image_spacing, 1.0f);
				ui_set_width(ui, ui_size_specified(texture->width * zoom, 1.0f))
					ui_set_height(ui, ui_size_specified(texture->height * zoom, 1.0f))
					{
						image_node = ui_create_node(ui, node_border, "S_IMAGE_DISPLAY");
					}

				//push background
				f32 bk_size = 16.0f;
				u32 bk_index = 0;
				u32 bk_index_start = 0;
				vec4 bk_colors[2] = {
					{050, 050, 050, 255},
					{100, 100, 100, 255},
				};


				f32 x = (f32)((i32)((scroll_dt_x - image_spacing) / (bk_size * zoom)) * (bk_size * zoom));
				f32 y = (f32)((i32)((scroll_dt_y - image_spacing) / (bk_size * zoom)) * (bk_size * zoom));;
				f32 x_start = x;
				f32 x_end = texture->width * (f32)zoom;
				f32 y_end = texture->height * (f32)zoom;
				bk_index = (i32)(y / (bk_size * zoom)) % 2 + 
					(i32)(x / (bk_size * zoom)) % 2;
				while(y < y_end)
				{
					bk_index_start = bk_index;
					while(x < x_end)
					{
						bk_index %= 2;
						ui_node_push_rectangle_wh(
								ui,
								image_node,
								(i16)x,
								(i16)y,
								(i16)(bk_size * zoom),
								(i16)(bk_size * zoom),
								bk_colors[bk_index]);

						x += bk_size * zoom;
						bk_index++;
					}
					bk_index %= 2;
					bk_index += (bk_index_start == bk_index);
					bk_index %= 2;
					x = x_start;
					y += bk_size * zoom;
				}

				//render with updated information
				//this is the updated region for the node on the next frame
				img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
				img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
				img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
				img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;

				//push image
				ui_node_push_image(
						ui,
						image_node,
						texture,
						0,
						0,
						uv0,
						uv1,
						uv2,
						uv3);
				ui_space_specified(ui, 8.0f * zoom, 1.0f);
				if(display_grid)
				{
					//push grid
					u32 grid_size = 10;

					grid_w = grid_w < 8 ? 8 : grid_w;
					grid_h = grid_h < 8 ? 8 : grid_h;
					//display first line
					vec4 grid_color = {0, 0, 255, 255};

					i32 grid_x = 0;
					while(grid_x < texture->width * zoom)
					{
						ui_node_push_rectangle(
								ui,
								image_node,
								grid_x,
								0,
								grid_x + 1,
								texture->height * zoom,
								grid_color);
						//next lines multiplies size by two to render cover both padding sizes
						grid_x += (grid_w) * zoom;
					}

#if 1
					i32 grid_y = 0;
					while(grid_y < texture->height * zoom)
					{
						ui_node_push_rectangle(
								ui,
								image_node,
								0,
								grid_y,
								texture->width * zoom, 
								grid_y + 1,	
								grid_color);
						grid_y += grid_h * zoom; 
					}
#endif
				}
			}
			ui_space_specified(ui, image_spacing, 1.0f);

			//uv selections
			bool32 uvs_interacted = 0;
			u32 uvs_index = 0;
			b8 mouse_l_down = (ui->mouse_l_down);
			n->uvs_group_down = n->uvs_group_down && mouse_l_down && region_hot;
			//enum{
			//	uv0_i = 0x1,
			//	uv1_i = 0x1,
			//	uv2_i = 0x1,
			//	uv3_i = 0x1,
			//	uv0_i = 0x1,
			//	uv0_i = 0x1,
			//	uv0_i = 0x1,
			//}uv_interacted = 0;
			for(u32 u = 0; u < total_uvs_count; u++)
			{
				vec2 *uv0 = uvs[uvs_index];
				vec2 *uv1 = uvs[uvs_index + 1];
				vec2 *uv2 = uvs[uvs_index + 2];
				vec2 *uv3 = uvs[uvs_index + 3];
				uvs_index += 4;

				vec2 uv0_scaled = {uv0->x * texture->width, uv0->y * texture->height};
				vec2 uv1_scaled = {uv1->x * texture->width, uv1->y * texture->height};
				vec2 uv2_scaled = {uv2->x * texture->width, uv2->y * texture->height};
				vec2 uv3_scaled = {uv3->x * texture->width, uv3->y * texture->height};

				vec2 uv0_selection = vec2_scale(uv0_scaled, zoom);
				vec2 uv1_selection = vec2_scale(uv1_scaled, zoom);
				vec2 uv2_selection = vec2_scale(uv2_scaled, zoom);
				vec2 uv3_selection = vec2_scale(uv3_scaled, zoom);

				f32 selection_rec_size = 8.0f;

				//offset these coordinates to draw the squares inside the rectangle
				f32 uv0_x = uv0_selection.x - 0;
				f32 uv0_y = uv0_selection.y - selection_rec_size;

				f32 uv1_x = uv1_selection.x - 0;
				f32 uv1_y = uv1_selection.y + 0;

				f32 uv2_x = uv2_selection.x - selection_rec_size;
				f32 uv2_y = uv2_selection.y + 0;

				f32 uv3_x = uv3_selection.x - selection_rec_size;
				f32 uv3_y = uv3_selection.y - selection_rec_size;

				f32 uv01_x = uv0_x + (uv1_x - uv0_x) * .5f;
				f32 uv01_y = uv0_y + (uv1_y - uv0_y) * .5f;

				f32 uv12_x = uv1_x + (uv2_x - uv1_x) * .5f;
				f32 uv12_y = uv1_y + (uv2_y - uv1_y) * .5f;

				f32 uv23_x = uv2_x + (uv3_x - uv2_x) * .5f;
				f32 uv23_y = uv2_y + (uv3_y - uv2_y) * .5f;

				f32 uv03_x = uv0_x + (uv3_x - uv0_x) * .5f;
				f32 uv03_y = uv0_y + (uv3_y - uv0_y) * .5f;

				//uv movement interaction
				//bottom left
				bool8 inside_uv0 = 0;
				bool8 inside_uv1 = 0;
				bool8 inside_uv2 = 0;
				bool8 inside_uv3 = 0;
				bool8 inside_uv01 = 0;
				bool8 inside_uv12 = 0;
				bool8 inside_uv23 = 0;
				bool8 inside_uv03 = 0;
				bool8 inside_uv_all = 0;
				if(region_hot && !n->uvs_group_down)
				{
					inside_uv0 = ui_mouse_inside_rec_xywh(ui,
							uv0_x + img_x0  ,
							uv0_y + img_y0  ,
							selection_rec_size,
							selection_rec_size
							);

					inside_uv1 = ui_mouse_inside_rec_xywh(ui,
							uv1_x + img_x0,
							uv1_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);


					inside_uv2 = ui_mouse_inside_rec_xywh(ui,
							uv2_x + img_x0,
							uv2_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);


					inside_uv3 = ui_mouse_inside_rec_xywh(ui,
							uv3_x + img_x0,
							uv3_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);

					inside_uv01 = ui_mouse_inside_rec_xywh(ui,
							uv01_x + img_x0,
							uv01_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);

					inside_uv12 = ui_mouse_inside_rec_xywh(ui,
							uv12_x + img_x0,
							uv12_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);


					inside_uv23 = ui_mouse_inside_rec_xywh(ui,
							uv23_x + img_x0,
							uv23_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);


					inside_uv03 = ui_mouse_inside_rec_xywh(ui,
							uv03_x + img_x0,
							uv03_y + img_y0,
							selection_rec_size,
							selection_rec_size
							);

					vec2 v0_r = {
						uv0_x + img_x0, uv0_y + img_y0};

					vec2 v1_r = {
						uv1_x + img_x0, uv1_y + img_y0};

					vec2 v2_r = {
						uv2_x + img_x0, uv2_y + img_y0};

					vec2 v3_r = {
						uv3_x + img_x0, uv3_y + img_y0};
					inside_uv_all |= ui_mouse_inside_baycentric(
							ui,
							v0_r,
							v1_r,
							v2_r);
					inside_uv_all |= ui_mouse_inside_baycentric(
							ui,
							v0_r,
							v2_r,
							v3_r);
					if(mouse_l_down && 
							(inside_uv0 +
							 inside_uv1 +
							 inside_uv2 +
							 inside_uv3 +
							 inside_uv01 +
							 inside_uv12 +
							 inside_uv23 +
							 inside_uv03 +
							 inside_uv_all
							))
					{
						uvs_interacted = 1;

						u8 uvsc_i[ ] ={
							inside_uv_all,
							inside_uv0,
							inside_uv1,
							inside_uv2,
							inside_uv3,
							inside_uv01,
							inside_uv12,
							inside_uv23,
							inside_uv03
						};
						u32 uvsc_i_count = ARRAYCOUNT(uvsc_i);

						n->uvs_group_down = 1;
						n->uvs_group_selection_index = u;

						for(u32 c = 0; c < 9; c++)
						{
							n->uvs_corner_index = uvsc_i[c] ? c : n->uvs_corner_index;
						}
					}
				}

				//u32 drag_interacting   = ui_element_interacting_flags(ui, ui_id_drag_uvs, ui_interaction_mouse_left_down);
				b8 drag_v0_interacting = inside_uv0 && mouse_l_down;

				//bool8 drag_v1_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv1, ui_interaction_mouse_left_down);
				//bool8 drag_v2_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv2, ui_interaction_mouse_left_down);
				//bool8 drag_v3_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv3, ui_interaction_mouse_left_down);

				//bool8 drag_uv01_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv01, ui_interaction_mouse_left_down);
				//bool8 drag_uv12_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv12, ui_interaction_mouse_left_down);
				//bool8 drag_uv23_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv23, ui_interaction_mouse_left_down);
				//bool8 drag_uv03_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv03, ui_interaction_mouse_left_down);

				//RENDER
				f32 selected_alpha = n->uvs_group_selection_index == u ? 
					255.0f : 180.0f;

				vec4 selection_line_color = {200, 0, 0, selected_alpha};
				vec4 uv_drag_color     = vec4_all(255);


				//;TODO add center squares to select by frame
				//render lines attached to the uv coordinates
#if 1

				ui_node_push_line(ui, image_node,
						uv0_selection,
						uv1_selection,
						2,
						selection_line_color);

				ui_node_push_line(ui, image_node,
						uv1_selection,
						uv2_selection,
						2,
						selection_line_color);

				ui_node_push_line(ui, image_node,
						vec2_add(uv2_selection, V2(0, -1)),
						vec2_add(uv3_selection, V2(0, -1)),
						2,
						selection_line_color);

				ui_node_push_line(ui, image_node,
						uv0_selection,
						uv3_selection,
						2,
						selection_line_color);
#endif

				//border selections for individual uvs
				ui_node_push_hollow_rectangle_wh(
						ui,
						image_node,
						uv0_x,
						uv0_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv1_x,
						uv1_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);
				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv2_x,
						uv2_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv3_x,
						uv3_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);
				//side selections

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv01_x,
						uv01_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv12_x,
						uv12_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv23_x,
						uv23_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);

				ui_node_push_hollow_rectangle_wh(ui, image_node,
						uv03_x,
						uv03_y,
						selection_rec_size,
						selection_rec_size,
						2,
						uv_drag_color);


			}
			//draw cursor inside image
			vec2 image_cursor = {
				ui->mouse_point.x - img_x0,
				ui->mouse_point.y - img_y0
			};
			image_cursor.x = (f32)((i32)(image_cursor.x / zoom) * zoom);
			image_cursor.y = (f32)((i32)(image_cursor.y / zoom) * zoom);
			ui_node_push_hollow_rectangle(
					ui,
					image_node,
					(i16)image_cursor.x,
					(i16)image_cursor.y,
					(i16)(1 * zoom),
					(i16)(1 * zoom),
					1,
					V4(255, 255, 0, 255));

			enum{
				selection_uv_all,
				selection_uv0,
				selection_uv1,
				selection_uv2,
				selection_uv3,
				selection_uv01,
				selection_uv12,
				selection_uv23,
				selection_uv03,
				selection_uv_new
			};
			//start new selection with the current selected group
			if(region_hot && mouse_l_down && !n->uvs_group_down)
			{
				n->uvs_group_down = 1;
				n->uvs_corner_index = selection_uv_new;
			}

			if(total_uvs_count)
			{
				n->uvs_group_selection_index = n->uvs_group_selection_index >= total_uvs_count ?
					total_uvs_count - 1 : n->uvs_group_selection_index;
				i32 selection_dx = 0;
				i32 selection_dy = 0;


				u32 uvs_interacted_index = n->uvs_group_selection_index * 4;
				vec2 *uv0 = uvs[uvs_interacted_index];
				vec2 *uv1 = uvs[uvs_interacted_index + 1];
				vec2 *uv2 = uvs[uvs_interacted_index + 2];
				vec2 *uv3 = uvs[uvs_interacted_index + 3];

				vec2 uv0_scaled = {uv0->x * texture->width, uv0->y * texture->height};
				vec2 uv1_scaled = {uv1->x * texture->width, uv1->y * texture->height};
				vec2 uv2_scaled = {uv2->x * texture->width, uv2->y * texture->height};
				vec2 uv3_scaled = {uv3->x * texture->width, uv3->y * texture->height};
				//interacting with any uvs
				if(n->uvs_group_down)
				{
					//drag selection around
					if(n->uvs_corner_index < selection_uv_new)
					{
						bool8 uv_all_i =n->uvs_corner_index == selection_uv_all;
						bool8 uv0_i = uv_all_i || n->uvs_corner_index == selection_uv0;
						bool8 uv1_i = uv_all_i || n->uvs_corner_index == selection_uv1;
						bool8 uv2_i = uv_all_i || n->uvs_corner_index == selection_uv2;
						bool8 uv3_i = uv_all_i || n->uvs_corner_index == selection_uv3;
						bool8 uv01_i =n->uvs_corner_index == selection_uv01;
						bool8 uv12_i =n->uvs_corner_index == selection_uv12;
						bool8 uv23_i =n->uvs_corner_index == selection_uv23;
						bool8 uv03_i =n->uvs_corner_index == selection_uv03;


						vec2 mouse_last = ui->mouse_point_last;
						vec2 mouse_point = ui->mouse_point;
						mouse_last.x /= zoom;
						mouse_last.y /= zoom;
						mouse_point.x /= zoom;
						mouse_point.y /= zoom;
						mouse_last = vec2_round_to_int(mouse_last);
						mouse_point = vec2_round_to_int(mouse_point);

						vec2 mouse_delta = vec2_sub(mouse_point, mouse_last);
						//mouse_delta.x /= zoom;
						//mouse_delta.y /= zoom;
						//						mouse_delta = vec2_round_to_int(mouse_delta);

						if(uv0_i || uv01_i || uv03_i)
						{
							uv0_scaled.x += mouse_delta.x;
							uv0_scaled.y += mouse_delta.y;
						}
						if(uv1_i || uv01_i || uv12_i)
						{
							uv1_scaled.x += mouse_delta.x;
							uv1_scaled.y += mouse_delta.y;
						}
						if(uv2_i || uv12_i || uv23_i)
						{
							uv2_scaled.x += mouse_delta.x;
							uv2_scaled.y += mouse_delta.y;
						}
						if(uv3_i || uv23_i || uv03_i)
						{
							uv3_scaled.x += mouse_delta.x;
							uv3_scaled.y += mouse_delta.y;
						}

						//						uv0_scaled = vec2_round_to_int(uv0_scaled);
						//						uv1_scaled = vec2_round_to_int(uv1_scaled);
						//						uv2_scaled = vec2_round_to_int(uv2_scaled);
						//						uv3_scaled = vec2_round_to_int(uv3_scaled);

						uv0->x = uv0_scaled.x /= texture->width;
						uv0->y = uv0_scaled.y /= texture->height;

						uv1->x = uv1_scaled.x /= texture->width;
						uv1->y = uv1_scaled.y /= texture->height;

						uv2->x = uv2_scaled.x /= texture->width;
						uv2->y = uv2_scaled.y /= texture->height;

						uv3->x = uv3_scaled.x /= texture->width;
						uv3->y = uv3_scaled.y /= texture->height;
					}
					else// new selection
					{
						vec2 mouse_image_hold = ui_mouse_hold_delta_from_node(ui, image_node);
						vec2 an_dt = {
							(f32)(i32)((image_cursor.x - mouse_image_hold.x) / zoom),
							(f32)(i32)((image_cursor.y - mouse_image_hold.y) / zoom)
						};


						mouse_image_hold.x /= zoom;
						mouse_image_hold.y /= zoom;
						mouse_image_hold = vec2_round_to_int(mouse_image_hold);

						i32 dx = (i32)an_dt.x;
						i32 dy = (i32)an_dt.y;
						i32 selected_x0 = (i32)mouse_image_hold.x;
						i32 selected_y0 = (i32)mouse_image_hold.y;
						i32 selected_x1 = (i32)mouse_image_hold.x;
						i32 selected_y1 = (i32)mouse_image_hold.y;

						selected_x1 += 1;
						selected_y1 += 1;
						//for displaying selection data
						selection_dx = ABS(dx) + 1;
						selection_dy = ABS(dy) + 1;
						if(dx >= 0)
						{
							selected_x1 += dx;
						}
						else
						{
							selected_x0 += dx;
						}

						if(dy >= 0)
						{
							selected_y1 += dy;
						}
						else
						{
							selected_y0 += dy;
						}

						uv0_scaled = vec2_round_to_int(uv0_scaled);
						uv1_scaled = vec2_round_to_int(uv1_scaled);
						uv2_scaled = vec2_round_to_int(uv2_scaled);
						uv3_scaled = vec2_round_to_int(uv3_scaled);

						uv0->x = (f32)selected_x0 / texture->width;
						uv0->y = (f32)selected_y1 / texture->height;

						uv1->x = (f32)selected_x0 / texture->width;
						uv1->y = (f32)selected_y0 / texture->height;

						uv2->x = (f32)selected_x1 / texture->width;
						uv2->y = (f32)selected_y0 / texture->height;

						uv3->x = (f32)selected_x1 / texture->width;
						uv3->y = (f32)selected_y1 / texture->height;

						//uv0_scaled.x = mouse_delta_node.x;
						//uv0_scaled.y = mouse_delta_node.y;

						//uv0->x = uv0_scaled.x /= texture->width;
						//uv0->y = uv0_scaled.y /= texture->height;

					}

				}

				ui_set_parent(ui, top_bar_node)
				{
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
								flip_uvs_h = 1;
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
											image_node->grid_display);
								}
							}
							if(ui_node_mouse_l_up(ui, cb_region))
							{
								image_node->grid_display = !image_node->grid_display;
							}
						}

						ui_space_ppct(ui, 1.0f, 0.0f);

						//data
						ui_set_h_text(ui, 4.0f, 1.0f)
							ui_set_w_em(ui, 40.0f, 1.0f)
							{
								i32 cursor_x = (((i32)ui->mouse_point.x - img_x0) / zoom);
								i32 cursor_y = (((i32)ui->mouse_point.y - img_y0) / zoom) ;
								ui_set_row(ui)
								{
									ui_textf(ui, 
											"Cursor {%d, %d} Selection {%u, %u}",
											cursor_x, cursor_y, selection_dx, selection_dy);

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
								}
							}
					}
				}

				if(flip_uvs_h)
				{

#if 1
					u32 uvs_interacted_index = n->uvs_group_selection_index * 4;
					vec2 *uv0 = uvs[uvs_interacted_index];
					vec2 *uv1 = uvs[uvs_interacted_index + 1];
					vec2 *uv2 = uvs[uvs_interacted_index + 2];
					vec2 *uv3 = uvs[uvs_interacted_index + 3];

					render_flip_and_fill_uvs_horizontally(uv0,
							uv1,
							uv2,
							uv3);
#else
					u32 uvs_interacted_index = n->uvs_group_selection_index * 4;

					vec2 *uv0 = uvs[uvs_interacted_index];
					vec2 *uv1 = uvs[uvs_interacted_index + 1];
					vec2 *uv2 = uvs[uvs_interacted_index + 2];
					vec2 *uv3 = uvs[uvs_interacted_index + 3];

					render_uvs uvs_f = 
						render_flip_uvs_horizontally(*uv0,
								*uv1,
								*uv2,
								*uv3);
#endif
				}
			}
		}
		n->target_scroll[ui_axis_x] = scroll_dt_x;
		n->target_scroll[ui_axis_y] = scroll_dt_y;

	}
	ui_pop_id(ui);


}

	static inline void
ui_image_uvs_selection_inside_region()
{
}


#define ui_image_selection_push_uv_selection(ui, uv0, uv1, uv2, uv3) _ui_image_selection_push_uv_selection(ui, uv0, uv1, uv2, uv3, 0)
#define ui_image_selection_push_uv_selection_label(ui, uv0, uv1, uv2, uv3, label) _ui_image_selection_push_uv_selection(ui, uv0, uv1, uv2, uv3, label)

	static u32
_ui_image_selection_push_uv_selection(game_ui *ui,
		vec2 *uv0,
		vec2 *uv1,
		vec2 *uv2,
		vec2 *uv3,
		u8 *label)
{

	return(0);
}

	inline void
ui_image_selection_push_uv_selection_labelf(game_ui *ui,
		vec2 *uv0,
		vec2 *uv1,
		vec2 *uv2,
		vec2 *uv3,
		u8 *label,
		...)
{
}

	static u32
ui_image_selection_push_frame_selection(game_ui *ui,
		u16 *frame_x,
		u16 *frame_y,
		u16 *frame_w,
		u16 *frame_h)
{
}



	static void
ui_timeline_begin(game_ui *ui, u8 *label)
{

}

	inline void
ui_timeline_set_lines_per_ms(game_ui *ui,
		u32 lines_per_ms)
{
}

#define ui_timeline_reproduce_button_u32(ui, r_ptr) ui_timeline_reproduce_button(ui, (u8 *)r_ptr)
	inline void
ui_timeline_reproduce_button(game_ui *ui,
		u8 *reproduce)
{
}

	inline void
ui_timeline_set_time(game_ui *ui,
		f32 *time_at,
		u32 *totalFrames)
{
}

#define ui_timeline_set_loop_cursor_TYPE(type)\
	ui_timeline_set_loop_cursor_ ##type(game_ui *ui,\
			u32 loop_active,\
			type *loop_start,\
			type *loop_end)

	inline u32 
_ui_timeline_set_loop_cursor(game_ui *ui,
		u32 loop_active)
{
	return(0);
}
	inline void
ui_timeline_set_loop_cursor_TYPE(f32)
{
}

	inline void
ui_timeline_set_loop_cursor_TYPE(u32)
{
}

	inline void
ui_timeline_set_loop_cursor_TYPE(u16)
{
}


	static void
ui_timeline_end(game_ui *ui)
{
}

	inline u32
ui_timeline_cursor_interacted(game_ui *ui)
{
	return(0);
}

	static u32
ui_timeline_addtrack(game_ui *ui, u8 *label)
{
	return(0);

}

#define UI_TIMELINE_CLIP_GROUP_TYPE(type) static u32 ui_timeline_clip_group_ ##type(game_ui *ui, u32 selected, type *timeStart, type *timeDuration, u32 trackIndex)
	static ui_timeline_clip *
_ui_timeline_clip(game_ui *ui, u32 selected)
{


}

UI_TIMELINE_CLIP_GROUP_TYPE(f32)
{
}

UI_TIMELINE_CLIP_GROUP_TYPE(u16)
{
}

	static u32
ui_timeline_add_frame_group(game_ui *ui,
		u32 selected,
		u32 target_frame)
{
	return(0);
}

	inline u32
ui_timeline_add_frame_key(game_ui *ui,
		u32 selected,
		u32 frame_duration,
		u32 frame_group_index,
		u32 track_index,
		u32 type,
		u8 *label)
{
	return(0);
}

	static u32
ui_timeline_add_clip_group_key(game_ui *ui,
		u32 selected,
		u32 clipIndex,
		u8 *label)
{
	return(0);
}

//
//ui push commands__
//

#define PANELBACKCOLOR V4(13, 17, 60, 0xe1)


	inline void
ui_open_or_close_next_panel(game_ui *ui)
{
}

	inline void
ui_start_next_panel_closed(game_ui *ui)
{
}

	inline void
ui_start_next_panel_minimized(game_ui *ui)
{
}

	inline void
ui_force_next_panel_focus(game_ui *ui)
{
}

//static inline ui_node
//ui_close_button(
//		game_ui *ui,
//		u8 *label)
//{
//}
//Initializes main panel

#define ui_panel_box(ui, flags, x, y, w, h, title)\
	ui_DEFER_LOOP(ui_panel_box_begin(ui, flags, x, y, w, h, title), ui_panel_end(ui))
	static inline void
ui_panel_box_begin(game_ui *ui,
		ui_panel_flags panel_flags,
		f32 panel_x,
		f32 panel_y,
		f32 panel_w,
		f32 panel_h,
		u8 *title)
{
	ui_panel *panel = ui_push_root_for_rendering(ui,
			panel_x,
			panel_y,
			panel_w,
			panel_h,
			panel_flags,
			title);

	ui_node *main_background_node;
	ui_node_flags mbf = 0;

	//create first background widget
	ui_set_width(ui, ui_size_specified(panel->root_node->size_x, 1))
	{

		ui_set_height(ui, ui_size_specified((f32)panel->root_node->size_y, 1))
		{
			main_background_node = ui_create_node(
					ui, node_clip | 
					node_background | 
					node_border | 
					node_clickeable,
					title);
			//
		}
	}

	ui_push_parent(ui, main_background_node);
}

	static inline void
ui_widget_region_begin(game_ui *ui,
		f32 panel_x,
		f32 panel_y,
		f32 panel_w,
		f32 panel_h,
		u8 *title)
{
	ui_panel *panel = ui_push_root_for_rendering(ui,
			panel_x,
			panel_y,
			panel_w,
			panel_h,
			ui_panel_flags_ignore_focus,
			title);
}

#define ui_widget_region_end(ui) ui_pop_root(ui);



//This probably should get a panel from the panel stack instead of allocating one on the command buffer since
//it will most likely still get the same id even after being  moved a bit...


static inline ui_node *
ui_title_bar(game_ui *ui)
{
	ui_node *title_node;
	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
		ui_set_height(ui, ui_size_specified( TITLEHEIGHT, 1.0f))
		{
			title_node = ui_create_node(
					ui, node_background | 
					node_clickeable | 
					node_border, "__TITLE__");
		}
	return(title_node);
}




//
//explorer specific
//
//
#define ui_explorer_selected_file_info(explorer) (explorer->current_directory_files[explorer->selected_file_index])

#define ui_explorer_FileDoubleClicked(ui) ((ui->explorer->file_got_selected) && !(ui->explorer->current_directory_files[ui->explorer->selected_file_index].is_directory) && (ui->interacted_flags & ui_interaction_mouse_left_double_click))
#define ui_explorer_SelectedFileIsDirectory(ui) (ui->explorer->current_directory_files[ui->explorer->selected_file_index].is_directory)
#define ui_explorer_SelectedFileData(ui) (ui->explorer->current_directory_files[ui->explorer->selected_file_index])
#define ui_explorer_selected_file(ui) (ui->explorer->selected_file_path_and_name)

	static inline void
ui_explorer_back(ui_explorer *explorer)
{
	i32 index = explorer->path_length - 1;
	//Remove the last ''
	explorer->directory_name[index] = '\0';
	while(index >= 0 && (explorer->directory_name[index] !=  '/'))
	{
		explorer->directory_name[index] = '\0';
		index--;
	}
	explorer->path_length = string_count(explorer->directory_name) - 1;
	explorer->update_path_files = 1;
	explorer->valid_file_focused = 0;
}


//
//
//


#define QUICKFORMAT(buffer, text) \
	va_list args;\
	va_start_m(args, preview);\
	format_text_list(buffer, sizeof(buffer), text, args);\
	va_end_m(args);


#define _SPINNER_INC_MAXEQ(value, inc, max, type) *(type *)value += inc; *(type *)value = MINEQ(*(type *)value ,max)
#define _SPINNER_DEC_MINEQ(value, dec, min, type) *(type *)value -= dec; *(type *)value = MAXEQ(*(type *)value ,min)
#define ui_spinner_PARAMS(type) (\
		game_ui *ui,\
		type inc_dec,\
		type min_value,\
		type max_value,\
		type *value,\
		bool32 confirm_on_enter,\
		uint8 *label)

#define ui_spinner_PARAMSf(type) (\
		game_ui *ui,\
		type inc_dec,\
		type min_value,\
		type max_value,\
		type *value,\
		bool32 confirm_on_enter,\
		uint8 *label,\
		...)

#define ui_spinner_TYPE(type) \
	ui_spinner_ ##type(\
			game_ui *ui,\
			type inc_dec,\
			type min_value,\
			type max_value,\
			type *value,\
			ui_input_text_flags textInputFlags,\
			uint8 *label)

	static inline void 
_ui_spinner(game_ui *ui,
		uint8 *label,
		void *value,
		ui_input_text_flags textInputFlags,
		ui_value_type valueType)
{
}

	static void
_ui_spinner_arrows(
		game_ui *ui,
		b8 *inc,
		b8 *dec,
		i32 *wheel)
{
	ui_node *left_arrow_node;
	ui_node *right_arrow_node;

	left_arrow_node = ui_create_nodef(
			ui,
			node_clickeable |
			node_active_animation |
			node_hover_animation |
			node_border |
			node_text |
			node_text_centered |
			node_background,
			"<#_spinner_la");
	//	ui_node_push_triangle(
	//			ui,
	//			left_arrow_node,
	//			ui_V2(12, 12),
	//			ui_V2(12, 3),
	//			ui_V2(0, 6),
	//			vec4_all(255));

	right_arrow_node = ui_create_node(
			ui,
			node_clickeable |
			node_active_animation |
			node_hover_animation |
			node_border |
			node_text |
			node_text_centered |
			node_background,
			">#_spinner_ra");
	//increase
	ui_usri r_usri = ui_usri_from_node(ui, right_arrow_node);
	ui_usri l_usri = ui_usri_from_node(ui, left_arrow_node);
	f32 hold_dt = 0.6f;
	if(ui_usri_mouse_l_pressed(r_usri) || (ui_usri_mouse_l_down(r_usri) && ui->mouse_hold_dt > hold_dt))
	{
		*inc = 1;
	}
	//decrease
	if(ui_usri_mouse_l_pressed(l_usri) || (ui_usri_mouse_l_down(l_usri) && ui->mouse_hold_dt > hold_dt))
	{
		*dec = 1;
	}
}

	static ui_node *
_ui_spinner_node(game_ui *ui,
		b8 *inc,
		b8 *dec,
		i32 *wheel,
		u8 *label)
{
	ui_node *spinner_scroll_region_node;
	ui_push_row(ui, 0, 0);
	ui_node *spinner_region_node = 0;
	spinner_region_node = ui_create_nodef(
			ui,
			node_clickeable |
			node_border |
			node_background,
			label);

	ui_push_id_string(ui, label);

	u32 updown_sz = 10;
	ui_set_parent(ui, spinner_region_node)
	{
		ui_set_parent(ui, spinner_region_node)
		{
			spinner_scroll_region_node = ui_mid_region(ui);
		}
	}
	ui_set_width(ui, ui_size_specified((f32)updown_sz, 1.0f))
	{
		_ui_spinner_arrows(ui,
				inc, dec, wheel);
	}
	ui_pop_row(ui);
	ui_pop_id(ui);
	if(ui_node_mouse_hover(ui, spinner_scroll_region_node))
	{
		*wheel = SIGN_OR_ZERO(ui->input.mouse_wheel);
	}

	return(spinner_region_node);
}

	static u32
ui_spinner_u32 ui_spinner_PARAMS(u32)
{
	b8 inc = 0;
	b8 dec = 0;
	i32 wheel = 0;
	ui_node *spinner_input_node;
	ui_node *spinner_region_node= _ui_spinner_node(ui, &inc, &dec, &wheel, label);
	ui_push_id_string(ui, label);
	ui_set_parent(ui, spinner_region_node) ui_set_wh_ppct(ui, 1.0f, 0.0f)
	{
		spinner_input_node = 
			ui_input_u32(ui,
					confirm_on_enter,
					value,
					"_sit");
	}
	ui_pop_id(ui);

	u32 value_before = *value;
	i32 v = inc_dec;
	v *= wheel ? wheel : (inc - dec);
	u32 va = ABS(v);
	if(v > 0 && u32_Add_OVERFLOWS(*value, va))
	{
		*value = U32MAX;
	}
	else if(v < 0 && va > *value)
	{
		*value = 0;
	}
	else
	{
		*value += v;
	}

	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);

	return(*value != value_before);
}


	static i32
ui_spinner_f32 ui_spinner_PARAMS(f32)
{
	b8 inc = 0;
	b8 dec = 0;
	i32 wheel = 0;
	ui_node *spinner_input_node;
	ui_node *spinner_region_node= _ui_spinner_node(ui, &inc, &dec, &wheel, label);
	ui_push_id_string(ui, label);
	f32 value_before = *value;
	ui_set_parent(ui, spinner_region_node) ui_set_wh_ppct(ui, 1.0f, 0.0f)
	{
		spinner_input_node = 
			ui_input_f32(ui,
					confirm_on_enter,
					value,
					"_sit");
	}
	ui_pop_id(ui);

	*value += inc_dec * (inc - dec);
	*value += inc_dec * wheel;

	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);

	return(*value != value_before);
}

	static u32
ui_spinner_u16 ui_spinner_PARAMS(u16)
{
	b8 inc = 0;
	b8 dec = 0;
	i32 wheel = 0;
	ui_node *spinner_input_node;
	ui_node *spinner_region_node= _ui_spinner_node(ui, &inc, &dec, &wheel, label);
	ui_push_id_string(ui, label);
	ui_set_parent(ui, spinner_region_node) ui_set_wh_ppct(ui, 1.0f, 0.0f)
	{
		spinner_input_node = 
			ui_input_u16(ui,
					confirm_on_enter,
					value,
					"_sit");
	}
	ui_pop_id(ui);

	u16 value_before = *value;
	i32 v = inc_dec;
	v *= wheel ? wheel : (inc - dec);
	u16 va = ABS(v);
	if(v > 0 && u16_Add_OVERFLOWS(*value, va))
	{
		*value = U16MAX;
	}
	else if(v < 0 && va > *value)
	{
		*value = 0;
	}
	else
	{
		*value += v;
	}

	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);

	return(*value != value_before);
}

#define spinner_inc_dec_type(type, changed)\
	b32 value_before = *value;\
	*value += inc_dec * (inc - dec);\
	*value += inc_dec * wheel;\
	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);\
	changed = *value != value_before;
	static i32
ui_spinner_i32 ui_spinner_PARAMS(i32)
{
	b8 inc = 0;
	b8 dec = 0;
	i32 wheel = 0;
	ui_node *spinner_input_node;
	ui_node *spinner_region_node= _ui_spinner_node(ui, &inc, &dec, &wheel, label);
	ui_push_id_string(ui, label);
	ui_set_parent(ui, spinner_region_node) ui_set_wh_ppct(ui, 1.0f, 0.0f)
	{
		spinner_input_node = 
			ui_input_i32(ui,
					confirm_on_enter,
					value,
					"_sit");
	}
	ui_pop_id(ui);

	i32 value_before = *value;
	*value += inc_dec * (inc - dec);
	*value += inc_dec * wheel;
	(*value) = (*value) < min_value ? min_value : (*value) > max_value ? max_value : (*value);

	return(*value != value_before);
}


static i16
ui_spinner_i16 ui_spinner_PARAMS(i16)
{
	i32 value32 = value ? *value : 0;
	i16 value_before = value ? *value : 0;
	b32 result = ui_spinner_i32(ui, inc_dec, min_value, max_value, &value32, confirm_on_enter, label);
	if(value)
	{
		if(value32 < I16MIN)
		{
			*value = I16MIN;
		}
		else if(value32 > I16MAX)
		{
			*value = I16MAX;
		}
		else
		{
			*value = (i16)value32;
		}
	}
	return(result);
}

static i8
ui_spinner_i8 ui_spinner_PARAMS(i8)
{
	i32 value32 = value ? *value : 0;
	i8 value_before = value ? *value : 0;
	b32 result = ui_spinner_i32(ui, inc_dec, min_value, max_value, &value32, confirm_on_enter, label);
	if(value)
	{
		if(value32 < -126)
		{
			*value = -126;
		}
		else if(value32 > 127)
		{
			*value = 127;
		}
		else
		{
			*value = (i8)value32;
		}
	}
	return(result);
}



//
//Post functionsReserved
//

//
// __Beginning and ending ui__
//

#define ui_increase_if_overflow(max, count, extra)\
	max = (count) >= (max) ? 1 + (count - max) + extra + max : max;

	inline game_ui *
ui_begin_frame(game_ui *ui,
		font_proportional *ui_font,
		platform_api *platform,
		game_renderer *gameRenderer,
		f32 dt)
{

	if(!ui) return(ui);
	//Reset, but not clear.
	memory_area_reset(&ui->area);

	//game_ui *ui = memory_area_push_struct(ui_area, game_ui);
	ui->frame_dt = dt;

	ui->mouse_point = V2(ui->input.mouse_x, ui->input.mouse_y);
	ui->mouse_point_last = V2(ui->input.mouse_x_last, ui->input.mouse_y_last);
	ui->last_looked_popup_was_opened = 0;

	ui->fontp = *ui_font; 
	ui->root_order = 0;
	//Note(Agu): Desired scale divided by height;
	f32 font_height = 12.0f; //20.0f
	ui->mouse_l_down	= (u8)input_down(ui->input.mouse_left);
	ui->mouse_l_pressed = (u8)input_pressed(ui->input.mouse_left);
	ui->mouse_l_up = (u8)input_up(ui->input.mouse_left);

	ui->mouse_wheel = ui->input.mouse_wheel;
	ui->input_text = &ui->input.input_text;
	//F*cking finally
	//   ui->input = gameInput;
	ui->current_layout_axis = ui_axis_y;
	ui->interaction_layer = 0;
	ui->next_node_extra_flags = 0;

	//Setup render
	ui->renderCommands = render_commands_begin_2d(gameRenderer);

	bool32 initialize_new_panels = 0;


	if(!ui->initialized)
	{
		ui->font_scale = 1;//font_height /  ui->fontp.font_height;

		ui->initialized = 1;
		ui->theme = ui_default_theme();
		//	   memory_area_preserve_size_and_clear(ui->area, sizeof(game_ui));
		//initialize a default max for the style stack
		for(u32 c = 0;
				c < ui_color_COUNT;
				c++)
		{
			ui->theme_colors_max[c] = 20;
		}
		//set the max for the node size stacks axis.
		for(u32 a = 0;
				a < ui_interaction_layer_COUNT;
				a++)
		{
			ui->node_size_stacks_max[a] = 20;
		}
	}

	//initialize default root node 
	ui->root_node.region.x0 = 0;
	ui->root_node.region.y0 = 0;
	ui->root_node.region.x1 = gameRenderer->os_window_width;
	ui->root_node.region.y1 = gameRenderer->os_window_height;
	ui->root_node.size_x = gameRenderer->os_window_width;  
	ui->root_node.size_y = gameRenderer->os_window_height; 
	ui->root_node.parent = 0;
	ui->root_node.first = 0;
	ui->root_node.next = 0;
	ui->root_node.prev = 0;
	ui->root_node.last = 0;
	ui->root_node.layout_axis = 1;
	memory_clear(&ui->void_node, sizeof(ui_node));
	ui->current_layout_axis = 1;


	//allocate explorer
	if(!ui->explorer)
	{
		ui->explorer = memory_area_clear_and_pushStruct(&ui->area, ui_explorer);

		ui->explorer->update_path_files = 1;
		ui->explorer->platform = platform;
		ui->explorer->search_pattern[0] = '*';
		ui->explorer->last_process_completed = 0;
		ui->explorer->closed = 1;
		//ui->explorer->panel  = 0;
	}
	else
	{
		ui->explorer = memory_area_push_struct(&ui->area, ui_explorer);
	}

	if(!ui->panel_stack_max)
	{
		//If not set, reserve space for 4 panels.
		ui->panel_stack_max = 40;
		ui->panel_last_avadible_slot = 0;
		initialize_new_panels = 1;
	}
	if(!ui->pushed_disable_max)
	{
		ui->pushed_disable_max = 6;
	}
	if(!ui->render_commands_max)
	{
		ui->render_commands_max = 50;
	}
	if(!ui->row_stack_max)
	{
		ui->row_stack_max = 20;
	}
	if(!ui->column_stack_max)
	{
		ui->column_stack_max = 20;
	}
	if(!ui->popup_array_max)
	{
		ui->popup_array_max = 40;
	}


	//Main panel overflow
	//increase maximum panel stack and allocate more later.
	if(ui->panel_stack_count >= ui->panel_stack_max)
	{
		Assert(0);
		//Add more space
		ui->panel_stack_max += ui->panel_stack_count - ui->panel_stack_max + 2;
		initialize_new_panels = 1;
	}
	else if(ui->panel_last_avadible_slot >= ui->panel_stack_max)
	{
		Assert(0);
		ui->panel_stack_max += 2;
		initialize_new_panels = 1;
	}


	ui->pushed_disable_count = 0;
	ui->next_node_disabled = 0;

	ui_increase_if_overflow(ui->pushed_disable_max, ui->pushed_disable_total_count, 2);
	ui_increase_if_overflow(ui->id_stack_max, ui->id_stack_count, 10);
	ui_increase_if_overflow(ui->render_commands_max, ui->render_commands_count, 10);

	//Push arrays

	//
	//arrays that should never lose their data
	//
	ui->persistent_nodes_max = 2000;
	ui->persistent_nodes = memory_area_push_array(
			&ui->area,
			ui_node,
			ui->persistent_nodes_max);
	//Allocate space needed
	ui->panel_stack = memory_area_push_array(&ui->area, ui_panel, ui->panel_stack_max);
	ui->panel_stack_order_ptr = memory_area_push_array(&ui->area, ui_panel *, ui->panel_stack_max);
	ui->popup_array = memory_area_push_array(&ui->area, ui_popup, ui->popup_array_max);

	ui->disable_stack = memory_area_push_size(&ui->area, ui->pushed_disable_max);

	//
	//restored arrays.
	//
	ui->row_stack = memory_area_push_size(
			&ui->area,
			sizeof(memory_size) * ui->row_stack_max);
	ui->column_stack = memory_area_push_size(
			&ui->area,
			sizeof(memory_size) * ui->column_stack_max);

	ui->id_stack_last = 0;
	ui->id_stack_count = 0;
	ui->id_stack_max = 100;
	ui->id_stack = memory_area_push_size(
			&ui->area, sizeof(ui_id_stack_slot) * 100 * 64);


	ui->parent_stack_max = 100;
	//reserve one for the root layout
	ui->parent_stack = memory_area_push_size(
			&ui->area,
			sizeof(memory_size) *  
			ui->parent_stack_max + 1);
	ui->parent_stack[ui->parent_stack_count] = &ui->root_node;
	Assert(ui->parent_stack_count == 0);

	//push theme colors stacks
	for(u32 c = 0;
			c < ui_color_COUNT;
			c++)
	{
		if(ui->theme_colors_counts[c] >= ui->theme_colors_max[c])
		{
			ui->theme_colors_max[c] += 1 + ui->theme_colors_counts[c] - ui->theme_colors_max[c];
		}
		//reset count
		ui->theme_colors_counts[c] = 0;
		//allocate theme stacks
		//reserve 1 for the default theme color.
		ui->theme_colors[c] = memory_area_push_array(
				&ui->area,
				ui_color_stack_slot,
				ui->theme_colors_max[c] + 1);
		//set default colors
		ui->theme_colors[c][0].color = ui->theme.colors[c];
		ui->theme_colors[c][0].previous = 0; 
		//set indices to default
		ui->color_indices.selected_colors_indices[c] = 0;
	}
	//push node sizes stack

	for(u32 a = 0;
			a < 2;
			a++)
	{
		//check for overflow, allocate more if needed
		if(ui->node_size_stacks_counts[a] >= ui->node_size_stacks_max[a])
		{
			ui->node_size_stacks_max[a] = 1 + ui->node_size_stacks_counts[a] - ui->node_size_stacks_max[a];
		}
		//reset count
		ui->node_size_stacks_counts[a] = 0;

		//allocate stacks
		ui->node_size_stacks[a] = memory_area_push_array(
				&ui->area,
				ui_node_size,
				ui->node_size_stacks_max[a]);
		//set default size
	}
	ui->node_size_stacks[0][0] = ui_default_node_w(ui);
	ui->node_size_stacks[1][0] = ui_default_node_h(ui);

	ui->render_commands_count = 0;
	ui->render_commands_buffer = memory_area_push_array(
			&ui->area,
			ui_render_command,
			ui->render_commands_max);

	ui->command_node_max = 100;
	ui->command_nodes = memory_area_push_array(
			&ui->area,
			ui_command_node,
			ui->command_node_max);











	//After allocation
	ui->reserved_space_used  = 0;
	ui->reserved_space_total = (u32)(ui->area.size - ui->area.used);
	ui->reserved_space      = ui->area.base + ui->area.used;
	//Main panel overflow
	if(initialize_new_panels)
	{
		//Reset new panels
		ui_panel zeroPanel = {0};
		for(u32 i = ui->panel_last_avadible_slot; i < ui->panel_stack_max;i++)
		{
			ui->panel_stack[i] = zeroPanel;
			ui->panel_stack[i].z_order = i;
		}
	}

	ui->panel_stack_count = 0;
	ui->panel_stack_generated = 0;

	if((ui->keep_input_text_interaction && !ui_id_EQUALS(ui->input_text_interacting_id, ui_id_ZERO)))
	{
		ui->input_text_timer += dt;
		if(ui->input_text_timer >= ui_INPUT_CURSOR_TIMER_TOTAL)
		{
			ui->input_text_timer = 0;
		}
	}
	else
	{
		ui->input_text_timer = 0;
	}


	//scale matrix to stay with the display buffer
	render_commands_SetClipAndViewport(
			ui->renderCommands,
			0,
			0,
			gameRenderer->back_buffer_width,
			gameRenderer->back_buffer_height);
	//render_commands_SetScaledToDisplay(ui->renderCommands);
	ui->projection = render_set_scaled_to_display(gameRenderer);
	ui->renderCommands->camera_type = render_camera_scale_to_display;
	//Scale mouse according to the choosen coordinates
	f32 scaleX = ui->projection.m[0][0];
	f32 scaleY = ui->projection.m[1][1];
	ui->mouse_point.x /= scaleX;
	ui->mouse_point.y /= scaleY;

	ui->mouse_point_last.x /= scaleX;
	ui->mouse_point_last.y /= scaleY;

	if(!ui->mouse_l_down)
	{
		ui->mouse_point_hold = ui->mouse_point;
		ui->mouse_hold_dt = 0;
	}
	else
	{
		ui->mouse_hold_dt += dt;
	}

	return(ui);

}

#define ZEROSTRUCT(a, type) {type b = {0}; a = b;}

	inline void
ui_RenderCheckIcon(game_ui *ui, f32 x, f32 y, f32 w, f32 h ,f32 lineThickness, vec4 color)
{

	f32 checkX0 = x + (w * 0.3f);
	f32 checkY0 = y;
	f32 checkX1 = checkX0 + (w * 0.6f);
	f32 checkY1 = checkY0 + h;

	vec2 line0P0 = {checkX1, checkY0};
	vec2 line0P1 = {checkX0, checkY1};

	render_line_2d_down(ui->renderCommands, line0P0, line0P1, lineThickness, vec4_all(255));

	vec2 line1P0 = line0P1;
	vec2 line1P1 = {line0P1.x - w * 0.3f, line0P1.y - h * 0.4f};
	render_line_2d_down(ui->renderCommands, line1P0, line1P1, lineThickness, vec4_all(255));

}


	static void
ui_render_grid(game_ui *ui,
		f32 frame_x, f32 frame_y, f32 frame_w, f32 frame_h,
		u32 grid_x, u32 grid_y, u32 grid_w, u32 grid_h)
{
	if(grid_w && grid_h)
	{
		vec4 gridColor = {0, 0, 0xFF, 0xFF};

		// SelectableImage grid
		//vertical lines
		f32 lineX = frame_x;
		i32 limitX = (i32)(frame_x + frame_w);
		for(u32 x = 1; lineX < limitX; x++)
		{
			lineX        = frame_x + (x * grid_w);
			f32 lineYEnd = frame_h;

			render_rectangle_2d_xywh(ui->renderCommands,
					lineX,
					frame_y,
					(1.0f),
					lineYEnd, gridColor);
		}

		//horizontal lines
		f32 lineY = 0; 
		i32 limitY = (i32)(frame_y + frame_h);
		for(u32 y = 1; lineY < limitY; y++)
		{
			lineY       = 0 + frame_y + (y * grid_h);
			f32 lineEnd = frame_w;

			render_rectangle_2d_xywh(ui->renderCommands,
					frame_x,
					lineY,
					lineEnd,
					1.0f, gridColor);
		}
	}
}

	inline void
ui_render_image_background(game_ui *ui,
		f32 background_x,
		f32 background_y,
		f32 background_w,
		f32 background_h,
		f32 zoom)
{

	vec4 backgroundColors[2] = {
		{050, 050, 050, 255},
		{100, 100, 100, 255},
	};
	u32 backgroundColorIndex = 0;
	u32 backgroundColorStart = 0;

	f32 backWH       = 20;
	f32 backWHScaled = backWH * zoom;

	//Multiply by two to advance by two blocks and keep the colors
	f32 wIndexStart       = (f32)background_x;
	f32 hIndexStart       = (f32)background_y;
	f32 wIndex            = wIndexStart;
	f32 hIndex            = hIndexStart;

	//i32 wEnd = (i32)(imageSizeX - clip_offset_x);
	//i32 hEnd = (i32)(imageSizeY - clip_offset_y);
	f32 background_x1 = (background_x + background_w);
	f32 background_y1 = (background_y + background_h);


	while(hIndex < background_y1)
	{
		backgroundColorStart = backgroundColorIndex;
		while(wIndex < background_x1)
		{
			backgroundColorIndex %= 2;

			f32 backX      = wIndex;
			f32 backY      = hIndex;
			f32 backWidth  = backWH * zoom * 1;
			f32 backHeight = backWH * zoom * 1;

			//if((wIndex + backWidth) > imageSizeXScaled)
			//{
			//     backWidth -= ((wIndex + backWidth) - imageSizeXScaled);
			//}
			//if((hIndex + backHeight) > imageSizeYScaled)
			//{
			//     backHeight -= ((hIndex + backHeight) - imageSizeYScaled);
			//}

			render_rectangle_2d_xywh(ui->renderCommands,
					backX,
					backY,
					backWidth,
					backHeight,
					backgroundColors[backgroundColorIndex++]);
			backWidth  -= backWH * zoom;
			backHeight -= backWH * zoom;
			//Multiply by two because two rectangles were drawn
			wIndex += (f32)(backWH * zoom);
		}
		backgroundColorIndex %= 2;
		backgroundColorIndex += (backgroundColorStart == backgroundColorIndex);
		if(backgroundColorIndex == 2)
		{
			backgroundColorIndex = 0;
		}
		//reset wIndex
		wIndex = wIndexStart;
		hIndex += (f32)(backWH * zoom);
	}
	//
	//
	//

}


typedef struct{
	ui_panel *nextPanel;
	ui_panel *current;
}ui_command_op;

	inline void
ui_draw_small_folder_icon(
		game_ui *ui,
		f32 x,
		f32 y,
		f32 height)
{
	f32 h = height;
	f32 w = h * 0.8f;
	//Icon
	vec4 green = {60, 225, 45, 255};
	render_rectangle_2d_xywh(
			ui->renderCommands,
			x,
			y,
			w,
			h,
			green);

	f32 side_h = h * 0.4f;
	f32 side_w = w * 0.1f;
	render_rectangle_2d_xywh(
			ui->renderCommands,
			x + w,
			y + (h - side_h),
			side_w,
			side_h,
			green);
}

	inline void
ui_draw_small_pack_icon()
{
}
	inline void
ui_draw_small_default_file_icon(
		game_ui *ui,
		f32 x,
		f32 y,
		f32 height)
{
	f32 h = height;
	f32 w = height * 0.7f;

	vec4 file_icon_color = {0xff, 0xff, 0xff, 0xff};

	render_rectangle_2d_xywh(
			ui->renderCommands,
			x,
			y,
			w,
			h, 
			file_icon_color);
}
	inline void
ui_draw_small_image_icon()
{
}
	inline void
ui_draw_small_model_icon()
{
}

	static inline ui_node *
ui_get_next_widget_depth_first_pre_order(ui_node *node)
{
	ui_node *next_node = node;
	//first childs
	if(next_node->first)
	{
		next_node = next_node->first;
	}
	//then siblings
	else if(next_node->next)
	{
		next_node = next_node->next;
	}
	else
	{
		//return to previous parent and looks for an unchecked sibling
		do
		{
			next_node = next_node->parent;

		}while(next_node && next_node->next == 0);

		if(next_node != 0)
		{
			next_node = next_node->next;
		}
	}
	return(next_node);
}

	static inline ui_node *
ui_get_next_widget_depth_first_post_order(ui_node *node)
{
	ui_node *next_node = node;
	//first childs
	if(next_node->first)
	{
		next_node = next_node->first;
	}
	//then siblings
	else if(next_node->next)
	{
		next_node = next_node->next;
	}
	else
	{
		//return to previous parent and looks for an unchecked sibling
		do
		{
			next_node = next_node->parent;

		}while(next_node && next_node->next == 0);

		if(next_node != 0)
		{
			next_node = next_node->next;
		}
	}
	return(next_node);
}

	static inline ui_node *
ui_get_last_node_for_post_order(ui_node *node)
{
	ui_node *result = node;
	while(result->last)
	{
		result = result->last;
	}
	return(result);
}

	static inline ui_node *
ui_get_next_widget_for_rendering(game_ui *ui, ui_node *start_node, ui_node *node)
{
	ui_node *next_node = node;
	//first childs
	if(next_node->first)
	{
		next_node = next_node->first;
	}
	//then siblings
	else if(next_node->next)
	{

		if(next_node->flags & node_clip)
		{
			render_commands_PopClip(ui->renderCommands);
		}
		next_node = next_node->next;
	}
	else
	{
		//return to previous parent and looks for an unchecked sibling
		do
		{
			if(next_node->flags & node_clip)
			{
				render_commands_PopClip(ui->renderCommands);
			}
			next_node = next_node->parent;

		}while(next_node && next_node->next == 0);

		if(next_node != 0)
		{

			if(next_node->flags & node_clip)
			{
				render_commands_PopClip(ui->renderCommands);
			}
			if(next_node != start_node)
			{
				next_node = next_node->next;
			}
			else
			{
				next_node = 0;
			}
		}
	}
	return(next_node);
}

//read commands start
	static void
ui_node_size_by_parent(ui_node *next_sibling, u32 a)
{
	if(next_sibling->size_types[a].type == size_percent_of_parent)
	{
		u32 parent_sum = 0;
		ui_node *p = next_sibling->parent;
		//look for a node in the hierarchy that doesn't depend on the size of its children.
		while(p && 
			 (p->size_types[a].type == size_sum_of_children ||
			 p->size_types[a].type == size_null))
		{
			p = p->parent;
		}
		parent_sum = p ? p->size[a] : 0;
		next_sibling->size[a]= 
			(u32)(parent_sum * next_sibling->size_types[a].amount);

	}
}


	static void
ui_calculate_hierarchy_layouts(
		game_ui *ui)
{
	//all of these are readed on a depth-first fashion which means childs first, then siblings
	ui_node *start_node = &ui->root_node;
	ui_node *next_node = start_node;
	ui_node *next_parent_node = 0;

	ui_node_flags skip_layout[2] = {
		(node_skip_layout_x),
		(node_skip_layout_y)
	};

	while(next_node = ui_get_next_widget_depth_first_pre_order(next_node))
	{
		//calculate stand-alone sizes for each index.
		for(u32 axis_index = 0;
				axis_index < 2;
				axis_index++)
		{
			ui_node_size node_size = next_node->size_types[axis_index];
			ui_node_size_type axis_size_type = node_size.type;

			if(axis_size_type == size_specified)
			{
				next_node->size[axis_index] = 
					(u32)(node_size.amount);

			}
			else if(axis_size_type == size_text && next_node->display_string)
			{
				f32 text_size = ui_get_text_size_until(ui, F32MAX, next_node->display_string).v[axis_index];

				next_node->size[axis_index] = (u32)text_size +
					(i16)node_size.amount;
			}
		}
	}
	next_node = start_node;

	//pre-order solve ascending sizes
	while(next_node = ui_get_next_widget_depth_first_pre_order(next_node))
	{
		//calculate stand-alone sizes.
		for(u32 axis_index = 0;
				axis_index < 2;
				axis_index++)
		{
			ui_node_size node_size = next_node->size_types[axis_index];
			ui_node_size_type axis_size_type = node_size.type;

			ui_node_size_by_parent(next_node, axis_index);
			if(node_size.type == size_sum_of_children)
			{
				next_node->size[axis_index] = 0;
			}
		}
	}
	next_node = ui_get_last_node_for_post_order(start_node);

	//calculate sum of children sizes
	//post-order descendant sizes
	/*This was a confusing as fuck process, but basically this makes sure to visit every children
	  of every node before any sibling adds its size to their corresponding parent. Cuando un nodo
	  va por el mismo eje que esta sumado por el tamaño de sus hijos, estos añaden su tamaño directamente,
	  sino se calcula cual es el valor máximo del otro eje del hijo y mas tarde se le añade al padre.*/
	while(next_node->parent)
	{
		//Make sure no children is left on every sibling
		while(next_node->prev)
		{
			next_node = next_node->prev;
			next_node = ui_get_last_node_for_post_order(next_node);
		}

		//now go through every sibling.
		ui_node *next_sibling = next_node;
		ui_node *parent = next_sibling->parent;

		b8 parent_soch[] = {
			next_sibling->parent->size_types[ui_axis_x].type == size_sum_of_children,
			next_sibling->parent->size_types[ui_axis_y].type == size_sum_of_children,
		};
		i16 _a = 0;
		u32 prev_axis = 0;
		u32 other_axis = 0;
		while(next_sibling)
		{
			for(u32 axis_index = 0;
					axis_index < 2;
					axis_index++)
			{
				if(next_sibling->flags & skip_layout[axis_index])
				{
					continue;
				}
				ui_node_size node_size_parent = next_sibling->parent->size_types[axis_index];
				ui_node_size_type axis_size_type = node_size_parent.type;

				i16 children_size_sum = 0;
				if(node_size_parent.type == size_sum_of_children)
				{
					//This child now advances on the opposite axis, so add the max.
					if(parent->layout_axis == axis_index)
					{
						//parent->size[!prev_axis] = _a;
						//_a = 0;
						parent->size[axis_index] += next_sibling->size[axis_index];
					}
					else
					{
						parent->size[axis_index] = MAX(parent->size[axis_index], next_sibling->size[axis_index]);
					}
				}
			}
			next_sibling = next_sibling->next;
		}
		if(parent_soch[ui_axis_x]) parent->size[ui_axis_x] += (parent->padding_x * 2);
		if(parent_soch[ui_axis_y]) parent->size[ui_axis_y] += (parent->padding_y * 2);

		//Sobro tamaño...
		if(_a)
		{
			parent->size[!prev_axis] = _a;
		}
		_a = 0;
		next_node = next_node->parent;
	}

	//solve violations!
	next_node = start_node;
	while(next_node = ui_get_next_widget_depth_first_pre_order(next_node))
	{
		bool16 allow_past_boundaries[2] = {
			(next_node->flags & node_scroll_x),
			(next_node->flags & node_scroll_y)
		};
		for(u32 a = 0;
				a < 2;
				a++)
		{
			ui_node *next_sibling = next_node->first;
			if(next_sibling && !allow_past_boundaries[a])
			{
				if(next_node->layout_axis == a)
				{
					u32 total_size = next_node->padding[a] * 2;
					while(next_sibling)
					{
						//in case the parent got its size corrected, make sure
						//the children that are sized by their parent get
						//re-sized correctly.
						ui_node_size_by_parent(next_sibling, a);
						//skips adding its total size
						if(next_sibling->flags & skip_layout[a])
						{
							next_sibling = next_sibling->next;
							continue;
						}
						total_size += next_sibling->size[a];

						next_sibling = next_sibling->next;
					}
					next_sibling = next_node->first;

					u32 violation_size = total_size - next_node->size[a];

					while(next_sibling && total_size > next_node->size[a])
					{
						//skips reducing its size.
						if(next_sibling->flags & skip_layout[a])
						{
							next_sibling = next_sibling->next;
							continue;
						}
						ui_node_size nz = next_sibling->size_types[a];
						i16 size_to_reduce = (i16)((1.0f - nz.size_strictness) * 
								violation_size);
						i16 reduced_size = next_sibling->size[a] - size_to_reduce;
						i16 min_size = (i16)((nz.size_strictness) * next_sibling->size[a]);
						i16 min_size_mo = (i16)((1.0f - nz.size_strictness) * next_sibling->size[a]);

						if(reduced_size < min_size)
						{
							reduced_size = min_size;
							size_to_reduce = next_sibling->size[a] - reduced_size;
						}

						total_size -= size_to_reduce;
						next_sibling->size[a] -= size_to_reduce;
						next_sibling = next_sibling->next;
					}
				}
				else
				{
					while(next_sibling)
					{
						//in case the parent got its size corrected, make sure
						//the children that are sized by their parent get
						//re-sized correctly.
						ui_node_size_by_parent(next_sibling, a);
						//skips adding its total size
						if(next_sibling->flags & skip_layout[a])
						{
							next_sibling = next_sibling->next;
							continue;
						}
						u32 node_size = next_sibling->size[a] + (next_node->padding[a] * 2);
						u32 violation_size = node_size - next_node->size[a];
						if(node_size > next_node->size[a])
						{
							//skips reducing its size.
							if(next_sibling->flags & skip_layout[a])
							{
								next_sibling = next_sibling->next;
								continue;
							}
							ui_node_size nz = next_sibling->size_types[a];
							i16 size_to_reduce = (i16)((1.0f - nz.size_strictness) * 
									violation_size);
							i16 reduced_size = next_sibling->size[a] - size_to_reduce;
							i16 min_size = (i16)((nz.size_strictness) * next_sibling->size[a]);
							i16 min_size_mo = (i16)((1.0f - nz.size_strictness) * next_sibling->size[a]);

							if(reduced_size < min_size)
							{
								reduced_size = min_size;
								size_to_reduce = next_sibling->size[a] - reduced_size;
							}

							next_sibling->size[a] -= size_to_reduce;
						}

						next_sibling = next_sibling->next;
					}
				}
			}
		}
	}

	//TODO: pre-order solve violations
	next_node = start_node;

	//based on sizes, calculate final positions.
	do
	{

		//scroll__
		//reset to re-calculate
		next_node->content_size[0] = 0;
		next_node->content_size[1] = 0;

		//calculate stand-alone sizes.
		i16 parent_relative_position[2] = {0, 0};
		ui_node *child_node = next_node->first;
		i16 padding_x = next_node->padding_x;
		i16 padding_y = next_node->padding_y;

		while(child_node)
		{
			bool16 skip_axis[2] = {
				(child_node->flags & node_skip_layout_x) == 0,
				(child_node->flags & node_skip_layout_y) == 0};
#if 1
			//this will stay relative to its parent node, but if
			//skipping axis, the relative position of its previous
			//siblings will not add to its final position.
			child_node->region.x0 = 
				next_node->region.x0 + 
				padding_x +
				child_node->added_x +
				(parent_relative_position[0])
				* skip_axis[0];

			child_node->region.y0 = 
				next_node->region.y0 + 
				child_node->added_y +
				padding_y +
				(parent_relative_position[1])
				* skip_axis[1];

			//calculate the layout position without scroll
			child_node->layout_position.x = child_node->region.x0 - next_node->region.x0; 
			child_node->layout_position.y = child_node->region.y0 - next_node->region.y0; 
			//add to the total size of the "contents" of the parent without counting
			//the scroll value
			if(!(child_node->flags & node_skip_content_size_x))
			{
				i16 end_x = child_node->layout_position.x + child_node->size_x + padding_x;
				next_node->content_size[0] = MAX(next_node->content_size[0], end_x);
			}
			if(!(child_node->flags & node_skip_content_size_y))
			{
				i16 end_y = child_node->layout_position.y + child_node->size_y + padding_y;
				next_node->content_size[1] = MAX(next_node->content_size[1], end_y);
			}

			child_node->region.x1 = child_node->region.x0 + child_node->size_x;
			child_node->region.y1 = child_node->region.y0 + child_node->size_y;

			parent_relative_position[next_node->layout_axis] +=
				child_node->size[next_node->layout_axis] * 
				skip_axis[next_node->layout_axis];
#else

			for(u32 a = 0;
					a < ui_axis_COUNT;
					a++)
			{
				i16 scroll_offset = offset_by_scroll[a] * skip_axis[a];
				//this will stay relative to its parent node, but if
				//skipping axis, the relative position of its previous
				//siblings will not add to its final position.
				child_node->region.r[a] = 
					next_node->region.r[a] + 
					child_node->added_position[a] +
					(parent_relative_position[a])
					* skip_axis[a];

				//calculate the layout position without scroll
				child_node->layout_position.v[a] = child_node->region.r[a] - next_node->region.r[a];
				//add to the total size of the "contents" of the parent without counting
				//the scroll value
				i16 end = child_node->layout_position.v[a] + child_node->size[a];
				next_node->content_size[a] = MAX(next_node->content_size[a], end);

				//finally, add the scroll value and calculate the final locations.
				child_node->region.r[a] += scroll_offset;
				child_node->layout_position.v[a] = scroll_offset; 

				child_node->region.r[a + 1] = child_node->region.r[a] + child_node->size[a];

			}
			parent_relative_position[child_node->layout_axis] +=
				child_node->size[child_node->layout_axis] * 
				skip_axis[child_node->layout_axis];
#endif

			//got to next sibling
			child_node = child_node->next;
		}


		ui_node_flags skip_scroll_bounds[ui_axis_COUNT] =
		{ 
			node_scroll_x_skip_bounds,
			node_scroll_y_skip_bounds
		};
		//calculate from the last frame
		for(u32 a = 0; a < ui_axis_COUNT; a++)
		{
			i32 contents_outside_bounds = next_node->content_size[a] - next_node->size[a];
			next_node->scroll_hits_bounds[a] = 0;
			if(!(next_node->flags & skip_scroll_bounds[a]))
			{
				next_node->target_scroll[a] = next_node->target_scroll[a] < 0 ? 0 : next_node->target_scroll[a];
			}
			f32 dts = next_node->dt_scroll[a];
			f32 delta_scroll = dts * dts + ui->frame_dt * 12.0f;
			next_node->dt_scroll[a] = MIN(delta_scroll, 1.0f);
			//only limit the scroll if content size > node size
			if(!(next_node->flags & skip_scroll_bounds[a]))
			{
				//the contents of this node suppasses its size
				if(contents_outside_bounds > 0)
				{
					//limit the target scroll by the content size
					next_node->target_scroll[a] = MIN(next_node->target_scroll[a], contents_outside_bounds);
					//scroll hitted the bounds!
					next_node->scroll_hits_bounds[a] = next_node->target_scroll[a] == contents_outside_bounds;
				}
				else
				{
					next_node->target_scroll[a] = 0;
					next_node->scroll_hits_bounds[a] = 1;
				}
			}
			else
			{
			}

			//set the display scroll
			next_node->scroll[a] = f32_lerp(next_node->scroll[a], next_node->dt_scroll[a], next_node->target_scroll[a]);
		}



		i16 offset_by_scroll[ ] = 
		{((next_node->flags & node_scroll_x) != 0) * (i16)-next_node->scroll_x,
			((next_node->flags & node_scroll_y) != 0) * (i16)-next_node->scroll_y};
		child_node = next_node->first;
		while(child_node)
		{
			bool16 skip_axis[2] = {
				(child_node->flags & node_skip_layout_x) == 0,
				(child_node->flags & node_skip_layout_y) == 0};

			i16 scroll_offset_x = offset_by_scroll[0] * skip_axis[0];
			i16 scroll_offset_y = offset_by_scroll[1] * skip_axis[1];
			//finally, add the scroll value and calculate the final locations.
			child_node->region.x0 += scroll_offset_x;
			child_node->region.y0 += scroll_offset_y;
			child_node->region.x1 += scroll_offset_x;
			child_node->region.y1 += scroll_offset_y;
			child_node->layout_position.x = scroll_offset_x; 
			child_node->layout_position.y = scroll_offset_y; 
			child_node = child_node->next;
		}

	}
	while(next_node = ui_get_next_widget_depth_first_pre_order(next_node));


}


	static inline ui_theme
ui_create_theme_from_indices(game_ui *ui, ui_theme_indices indices)
{
	ui_theme result;
	for(u32 c = 0;
			c < ui_color_COUNT;
			c++)
	{
		result.colors[c] = ui->theme_colors[c][indices.i[c]].color;
	}
	return(result);
}

	static void
ui_read_commands(game_ui *ui, ui_node *root_node)
{

	render_commands *commands = ui->renderCommands;   
	ui_node *current_node = root_node;


	while(current_node = ui_get_next_widget_for_rendering(ui, root_node, current_node))
	{

		i32 parent_relative_position = 0;
		ui_node *child_node = current_node;

		if(current_node->flags & node_clip)
		{
			i32 x0 = current_node->region.x0 + current_node->padding_x;
			i32 y0 = current_node->region.y0 + current_node->padding_y;
			i32 x1 = current_node->region.x1 - current_node->padding_x;
			i32 y1 = current_node->region.y1 - current_node->padding_y;
			x1 = x1 < x0 ? x0 : x1;
			y1 = y1 < y0 ? y0 : y1;
			ui_push_clip_inside_last(ui,
					x0,
					y0,
					x1 + 1,
					y1 + 1);
		}
		ui_node *node = (ui_node *)child_node;
		ui_theme node_theme = ui_create_theme_from_indices(ui, node->color_indices);
		//node->region.x0 += (u32)ui->current_panel->p.x;
		//node->region.y0 += (u32)ui->current_panel->p.y;
		//node->region.x1 += (u32)ui->current_panel->p.x;
		//node->region.y1 += (u32)ui->current_panel->p.y;
		vec4 background_color = {0};

		vec2 coords = {(f32)node->region.x,
			(f32)node->region.y};
		i32 size_x = node->size_x;
		i32 size_y = node->size_y;

		b8 has_background = node->flags & node_background;
		u8 has_active_animation = node->flags & node_active_animation;
		u8 has_hot_animation = node->flags & node_hover_animation;
		b8 hot = (ui_node_mouse_hover(ui, node) != 0);

		//disabled "colors"
		if(node->flags & node_disabled)
		{
			node_theme.background_color.w = 160;
			node_theme.text_color.w = 160;
		}
		//push default displays
		if(node->flags & node_clickeable)
		{
			ui_id id = node->id;
			if(!node->id.value0 || 
					node->flags & node_disabled ||
					node->flags & node_readonly)
			{
				id = ui_id_READONLY;
			}
			ui_update_node_interaction(
					ui,
					id,
					(f32)node->region.x0,
					(f32)node->region.y0,
					(f32)node->region.x1,
					(f32)node->region.y1,
					node->interaction_index);
		}
		if(has_background || has_active_animation || has_hot_animation)
		{
			background_color = node_theme.background_color;
			if(!has_background)
			{
				background_color = vec4_all(0);
			}

			//"hot" animation if hot
			if(hot)
			{
				if(node->flags & node_hover_animation)
				{
					background_color.r = f32_lerp(background_color.r, node->hot_time, node_theme.hot_color.r);
					background_color.g = f32_lerp(background_color.g, node->hot_time, node_theme.hot_color.g);
					background_color.b = f32_lerp(background_color.b, node->hot_time, node_theme.hot_color.b);
					background_color.w = f32_lerp(background_color.w, node->hot_time, node_theme.hot_color.w);
				}
			}
			//"interacting" animation if interacting
			if(node->flags & node_active_animation)
			{
				if(ui_node_mouse_l_down(ui, node))
				{
					background_color.r = f32_lerp(background_color.r, node->interacted_time, node_theme.interacting_color.r);
					background_color.g = f32_lerp(background_color.g, node->interacted_time, node_theme.interacting_color.g);
					background_color.b = f32_lerp(background_color.b, node->interacted_time, node_theme.interacting_color.b);
					background_color.w = f32_lerp(background_color.w, node->interacted_time, node_theme.interacting_color.w);
					f32 time_multiplier = 9.4f;
					node->interacted_time += ui->frame_dt * time_multiplier;
					node->interacted_time = node->interacted_time > 1.0f ? 1.0f : node->interacted_time;
				}
				else
				{
					node->interacted_time = 0;
				}
			}

			//   ui_node_push_rectangle(ui,
			//		   node,
			//		   node->region.x0,
			//		   node->region.y0,
			//		   node->region.x1,
			//		   node->region.y1,
			//		   background_color);
			render_rectangle_2d(ui->renderCommands,
					(f32)node->region.x0,
					(f32)node->region.y0,
					(f32)node->region.x1,
					(f32)node->region.y1,
					background_color);
		}
		if(node->flags & node_text)
		{
			//   ui_node_push_text(ui,
			//		   node,
			//		   node->region.x0,
			//		   node->region.y0,
			//		   flags & node_text_centered,
			//		   ui->theme.text_color,
			//		   node->data);

			vec2 text_offset = {0};
			if(node->flags & node_text_centered)
			{
				text_offset = font_get_text_pad_offset(
						&ui->fontp,
						node->size_x,
						node->size_y,
						ui->font_scale,
						font_text_pad_center,
						node->display_string
						);
			}
			ui_render_text(ui,
					(i32)(node->region.x0 + text_offset.x),
					(i32)(node->region.y0 + text_offset.y),
					I32MAX,//node->region.x1,
					I32MAX,//node->region.y1,
					node_theme.text_color,
					node->display_string);
		}
		if(node->flags & node_border)
		{
			//   ui_node_push_hollow_rectangle(
			//		   ui,
			//		   node,
			//		   node->region.x0,
			//		   node->region.y0,
			//		   node->region.x1,
			//		   node->region.y1,
			//		   1,
			//		   ui->theme.border_color);

			render_rectangle_borders_2D(
					ui->renderCommands,
					(f32)node->region.x0,
					(f32)node->region.y0,
					(f32)node->region.x1 - node->region.x0,
					(f32)node->region.y1 - node->region.y0,
					1, 
					node_theme.border_color);
		}

		//increase hot and interaction timers
		if(hot)
		{
			//increase hot time
			f32 time_multiplier = 6.4f;
			node->hot_time += ui->frame_dt * time_multiplier;
			node->hot_time = node->hot_time > 1.0f ? 1.0f : node->hot_time;
		}
		else
		{
			node->hot_time = 0;
		}

		//read render commands
		ui_render_command *r_command = child_node->first_render_command;
		while(r_command)
		{
			i32 rx0 = child_node->region.x0;
			i32 ry0 = child_node->region.y0;
			i32 rx1 = child_node->region.x1;
			i32 ry1 = child_node->region.y1;
			switch(r_command->type)
			{
				case type_rectangle:
					{
						render_rectangle_2d_xywh(commands,
								(f32)rx0 + r_command->x0,
								(f32)ry0 + r_command->y0,
								(f32)(r_command->x1 - r_command->x0),
								(f32)(r_command->y1 - r_command->y0),
								r_command->color);
					}break;
				case type_text:
					{
						ui_render_text(ui,
								(i32)(rx0 + r_command->p0.x),
								(i32)(ry0 + r_command->p0.y),
								(i32)r_command->p1.x,
								(i32)r_command->p1.y,
								node_theme.text_color,
								r_command->text);
					}break;
				case type_hollow_rectangle:
					{
						render_rectangle_borders_2D(
								ui->renderCommands,
								(f32)rx0 + r_command->x0,
								(f32)ry0 + r_command->y0,
								r_command->x1,
								r_command->y1,
								r_command->border_thickness, 
								r_command->color);
					}break;
				case type_triangle:
					{
						render_Triangle2D(
								ui->renderCommands,
								V2((f32)rx0 + r_command->p0.x,(f32)ry0 + r_command->p0.y),
								V2((f32)rx0 + r_command->p1.x,(f32)ry0 + r_command->p1.y),
								V2((f32)rx0 + r_command->p2.x,(f32)ry0 + r_command->p2.y),
								r_command->color);
					}break;
				case type_image:
					{
						vec2 v0 = {
							(f32)rx0,
							(f32)ry1,
						};
						vec2 v1 = {
							(f32)rx0,
							(f32)ry0,
						};
						vec2 v2 = {
							(f32)rx1,
							(f32)ry0,
						};
						vec2 v3 = {
							(f32)rx1,
							(f32)ry1,
						};
						render_push_quad_2d(
								ui->renderCommands,
								r_command->texture,
								v0,
								v1,
								v2,
								v3,
								r_command->uv0,
								r_command->uv1,
								r_command->uv2,
								r_command->uv3,
								vec4_all(255)
								);

						//   render_rectangle_2d_xywh(ui->renderCommands,
						//		  v0.x,
						//		  v0.y,
						//		  v2.x - v0.x,
						//		  v2.y - v0.y, 
						//		   V4(0, 0, 255, 255));
					}break;
				case type_line:
					{
						vec2 p0 = {
							rx0 + r_command->p0.x,
							ry0 + r_command->p0.y,
						};
						vec2 p1 = {
							rx0 + r_command->p1.x,
							ry0 + r_command->p1.y,
						};
						render_line_2d_down(
								ui->renderCommands,
								p0,
								p1,
								r_command->border_thickness,
								r_command->color);
					}break;
				default:
					{
						Assert(0);
					}break;
			}
			r_command = r_command->next;
		}
	}

	//ui commands end

}

	inline void 
ui_before_end(game_ui *ui)
{
	ui_command_op commands_operations = {0};

	for(u32 p = 0;
			p < ui->panel_stack_generated;
			p++)
	{
		ui_panel *current_panel = ui->panel_stack_order_ptr[p];
		//Pick the index from the end of the stack.
		//Pick one panel from the stack.
		current_panel->root_node->next = 0;
		//
		//Read panel commands.
		//
		//READ COMMANDS
		commands_operations.nextPanel = current_panel;

		//Process front panel focus for when the mouse overlaps the panel
		u32 processFocus = !commands_operations.nextPanel->closed && !(commands_operations.nextPanel->flags & ui_panel_flags_ignore_focus);
		if(processFocus)
		{
			vec2 panel_position = commands_operations.nextPanel->p;
			vec2 panel_size = commands_operations.nextPanel->sz;
			if(commands_operations.nextPanel->notVisible)
			{
				panel_size.y = TITLEHEIGHT;
			}
			i32 mouseOverThisPanel = ui_mouse_inside_rec_clipped_xywh(ui,
					panel_position.x,
					panel_position.y,
					panel_size.x,
					panel_size.y);
			if(mouseOverThisPanel)
			{
				ui->panel_hot = commands_operations.nextPanel;
			}
		}

		//This loop is necessary in case of child panels.
		while(commands_operations.nextPanel)
		{
			//completely ignore the panel if closed
			if(commands_operations.nextPanel->closed)
			{
				commands_operations.nextPanel = 0;
			}
			else
			{
				//Set current and reserve next
				commands_operations.current   = commands_operations.nextPanel;
				commands_operations.nextPanel = 0;
				//skip the command reading if not visible
				if(!commands_operations.current->notVisible)
				{
					ui_read_commands(ui, commands_operations.current->root_node); 

					//render_commands_PopClip(ui->renderCommands);

				}
			}
		}

		//End main panel layout
		ui->current_panel = 0;

	}

	if(ui->focused_panel)
	{
		if(!ui->mouse_l_down)
		{
			ui->focused_panel = 0;
		}
	}
	else
	{
		if(ui->mouse_l_down)
		{
			//re-set the z order of panels
			/*-I might need to change the focused panel to
			  another part.
			  I can probably get rid of the "focused-panel" variable
			  in game_ui

*/
			ui->focused_panel = ui->panel_hot;
			if(ui->focused_panel)
			{
				u32 last_z = ui->panel_last_avadible_slot - 1;
				u32 c_last_z = last_z;
				u32 prev_z = ui->focused_panel->z_order;
				ui_panel *lcp = 0;
				while(c_last_z != prev_z)
				{
					for(u32 p = 0;
							p < ui->panel_last_avadible_slot;
							p++)
					{
						ui_panel *current_panel = ui->panel_stack + p;
						if(current_panel != lcp && current_panel->z_order == c_last_z)
						{
							lcp = ui->panel_stack + p;
							c_last_z = --ui->panel_stack[p].z_order;
							break;
						}
					}
				}
				ui->focused_panel->z_order = last_z;
			}
		}
	}
	ui->panel_hot = 0;
}

	inline void
ui_sort_panel_order(game_ui *ui)
{
	//sort by z-order
	//panels whose z is lower than the rest will be
	//updated and rendered first.
	u32 count = ui->panel_stack_generated;
	u32 i = 0;
	for(u32 y = 0; y < count; y++)
	{
		u32 p0_z = ui->panel_stack_order_ptr[y]->z_order;
		for(u32 x = 0; x < count; x++)
		{
			u32 p1_z = ui->panel_stack_order_ptr[x]->z_order;
			if(p0_z < p1_z)
			{
				ui_panel *pc = ui->panel_stack_order_ptr[y];
				ui->panel_stack_order_ptr[y] = ui->panel_stack_order_ptr[x];
				ui->panel_stack_order_ptr[x] = pc;
			}
		}
	}
	//if a panel has an "ignore focus" flags, then send
	//to the start of the array.
	bool32 keep_sorting = 0;
	for(u32 y = 0; y < count; y++)
	{
		for(u32 x = 0; x < count;x ++)
		{
			bool32 send_back = ui->panel_stack_order_ptr[x]->flags &
				ui_panel_flags_ignore_focus;
			bool32 send_front = ui->panel_stack_order_ptr[x]->flags &
				ui_panel_flags_keep_on_front;
			if(send_back)
			{
				u32 i = x;
				while(i)
				{
					ui_panel *copy_ptr = ui->panel_stack_order_ptr[i];
					ui->panel_stack_order_ptr[i] = ui->panel_stack_order_ptr[i - 1];
					ui->panel_stack_order_ptr[i - 1] = copy_ptr;
					i--;
				}
			}
			else if(send_front)
			{
				u32 i = x;
#if 0
				while(i < count - 1)
				{
					ui_panel *copy_ptr = ui->panel_stack_order_ptr[i];
					ui_panel *panel1 = ui->panel_stack_order_ptr[i + 1];
					//check the order of calling for this panel and compare if 
					//the two should be sent to the front
					ui->panel_stack_order_ptr[i] = ui->panel_stack_order_ptr[i + 1];
					ui->panel_stack_order_ptr[i + 1] = copy_ptr;
					i++;
				}
#else
				u32 j = x + 1;
				while(i < count - 1)
				{
					ui_panel *copy_ptr = ui->panel_stack_order_ptr[i];
					ui_panel *panel1 = ui->panel_stack_order_ptr[j];
					while( j < count &&
						panel1->flags & ui_panel_flags_keep_on_front &&
						panel1->call_order > copy_ptr->call_order)
					{
						j++;
						panel1 = ui->panel_stack_order_ptr[j];
					}
					if(j >= count) break;
					//check the order of calling for this panel and compare if 
					//the two should be sent to the front
					ui->panel_stack_order_ptr[i] = ui->panel_stack_order_ptr[j];
					ui->panel_stack_order_ptr[j] = copy_ptr;
					i = j;
					j++;
				}
#endif
			}
		}
	}

}


	inline void
ui_end_frame(game_ui *ui, u32 window_focused)
{
	//ui->panel_stack_count = 0;
	ui_input *input = &ui->input;

	bool32 mouse_l_down = ui->mouse_l_down;
	bool32 mouse_l_up = input_up(ui->input.mouse_left);
	bool32 mouse_l_pressed = input_pressed(ui->input.mouse_left); 
	bool32 mouse_l_double_click = ui->input.doubleClickLeft;
	bool32 mouse_l_tripple_click = ui->input.tripleClickLeft;

	bool32 mouse_r_double_click = 0; 
	bool32 mouse_r_tripple_click = 0;

	bool32 mouse_r_down = input_down(input->mouse_right);
	bool32 mouse_r_up = input_up(ui->input.mouse_right);
	bool32 mouse_r_pressed = input_pressed(ui->input.mouse_right); 

	bool32 mouse_m_down = input->mouse_middle;//input_down(input->mouse_middle);
	bool32 any_mouse_down = mouse_l_down + mouse_r_down + mouse_m_down;
	//Prevents interaction with widgets while holding click outside anything ui related
	if(any_mouse_down && !ui_any_node_hot(ui) && !ui_any_node_interacting(ui))
	{
		ui->interactions[ui_interaction_layer_default].node_interacting = ui_id_ZERO; 
		ui->interactions[ui_interaction_layer_mid].node_interacting = ui_id_ZERO; 
		ui->interactions[ui_interaction_layer_top].node_interacting = ui_id_ZERO; 
	}
	//There is an element id
	bool32 interacting = !ui_any_node_interacting(ui); 
	bool32 interacted  = !ui_any_node_interacted(ui); 

	ui->interactions[ui_interaction_layer_default].node_last_interact = ui_id_ZERO;
	ui->interactions[ui_interaction_layer_mid].node_last_interact = ui_id_ZERO;
	ui->interactions[ui_interaction_layer_top].node_last_interact = ui_id_ZERO;
	//used on the next frame for inform for interaction
	ui_interaction_info interacting_info = {0};
	for(u32 i = 0;
			i < ui_interaction_layer_COUNT;
			i++)
	{
		ui_id_interactions *interaction_layer = ui->interactions + i;
		bool32 interacting = !ui_id_equals(interaction_layer->node_interacting, ui_id_ZERO); 
		bool32 is_hot = ui_id_equals(interaction_layer->node_hot, interaction_layer->node_interacting);

		interaction_layer->node_transition = 0;
		interaction_layer->interacting_flags = 0;
		ui_interaction_flags layer_interacting_flags = interaction_layer->interacting_flags;
		if(interacting)
		{
			//Not interacting with element anymore
			if((!any_mouse_down && !ui->keep_interaction) || !window_focused)
			{
				interaction_layer->node_transition++;
				interaction_layer->node_last_interact = interaction_layer->node_interacting; 
				interaction_layer->node_interacting = ui_id_ZERO;
				//Inherits interaction
				interaction_layer->interacted_flags = layer_interacting_flags;

				interaction_layer->interacted_flags |= ui_interaction_mouse_left_up * (is_hot && mouse_l_up);
				interaction_layer->interacted_flags |= ui_interaction_mouse_left_double_click * mouse_l_double_click;
				if(mouse_l_double_click)
				{
					int s = 0;
				}
			}
			else
			{

				interaction_layer->interacting_flags |= ui_interaction_mouse_left_down * mouse_l_down;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_pressed * mouse_l_pressed;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_up * (is_hot && mouse_l_up);
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_double_click * mouse_l_double_click;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_tripple_click * mouse_l_tripple_click;

				interaction_layer->interacting_flags |= ui_interaction_mouse_right_down * mouse_r_down;
				interaction_layer->interacting_flags |= ui_interaction_mouse_right_pressed * mouse_r_pressed;
				interaction_layer->interacting_flags |= ui_interaction_mouse_right_double_click * mouse_r_double_click;
				interaction_layer->interacting_flags |= ui_interaction_mouse_right_tripple_click * mouse_r_tripple_click;

				interaction_layer->interacting_flags |= ui_interaction_mouse_middle_down * mouse_m_down;
				interaction_layer->interacting_flags |= ui_interaction_mouse_hover * is_hot;
			}
		}
		if(ui->process_hot_nodes && window_focused)
		{
			//currently not interacting with any node 
			//and there is an element being hovered by the mouse.
			interaction_layer->node_hot = interaction_layer->node_last_hot;
			if(!interacting && any_mouse_down)
			{
				//process normal elements
				interaction_layer->node_interacting = interaction_layer->node_hot;
				interaction_layer->node_transition++;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_down * mouse_l_down;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_pressed * mouse_l_pressed;
				interaction_layer->interacting_flags |= ui_interaction_mouse_left_up * (is_hot && mouse_l_up);

				interaction_layer->interacting_flags |= ui_interaction_mouse_right_down * mouse_r_down;
				interaction_layer->interacting_flags |= ui_interaction_mouse_right_pressed * mouse_r_pressed;

				interaction_layer->interacting_flags |= ui_interaction_mouse_middle_down * mouse_m_down;
				//Process double click if this node got clicked on the last frame,
				//this should be separated depending on the mouse button maybe...
				if(ui_id_EQUALS(interaction_layer->node_interacting, interaction_layer->node_hot))
				{
					interaction_layer->interacting_flags |= ui_interaction_mouse_left_double_click * mouse_l_double_click;
					interaction_layer->interacting_flags |= ui_interaction_mouse_left_tripple_click * mouse_l_tripple_click;

					interaction_layer->interacting_flags |= ui_interaction_mouse_right_double_click * mouse_r_double_click;
					interaction_layer->interacting_flags |= ui_interaction_mouse_right_tripple_click * mouse_r_tripple_click;

				}

				interaction_layer->node_last_clicked = interaction_layer->node_hot;
			}
			interaction_layer->node_last_hot = ui_id_ZERO;
		}
	}

	//Keep interaction or readonly for a set of nodes
	ui->keep_interaction = (b8)ui->keep_interaction_countdown;
	ui->next_node_readonly = (b8)ui->next_node_readonly_countdown;
	ui->keep_input_text_interaction = (b8)ui->input_text_interaction_countdown;
	if(!ui->keep_input_text_interaction)
	{
		ui->input_text_interacting_id = ui_id_ZERO;
	}
	if(ui->keep_interaction_countdown)
	{
		ui->keep_interaction_countdown--;
	}
	if(ui->input_text_interaction_transition)
	{
		ui->input_text_interaction_transition--;
	}
	ui->next_node_readonly_countdown -= ui->next_node_readonly_countdown;
	ui->input_text_interaction_countdown -= ui->input_text_interaction_countdown;
	//Restore this before reading the commands.
	ui->process_hot_nodes = 1;

	ui_read_command_nodes(ui);
	ui_sort_panel_order(ui);
	ui_calculate_hierarchy_layouts(ui);
	ui_before_end(ui);
	ui->current_frame++;


	//reset root node
	ui->root_node.first = 0;
	ui->root_node.next = 0;

	if(!ui->input_text->focused)
	{
		ui->input_text_entered = 0;
	}

	render_commands_end(ui->renderCommands);

	input_text_update(&input->input_text);
}

#define ui_scroll_vertical_from_node(ui, node) \
	ui_scroll_vertical(ui, node->size[ui_axis_y], node->content_size[ui_axis_y], node->scroll[ui_axis_y])
static inline f32 
ui_scroll_vertical(
		game_ui *ui,
		f32 frame_h,
		f32 total_node_content,
		f32 scroll_value)
{
	ui_node *scroll_background;
	ui_node *scroll_bar;
	ui_node *scroll_region;
	f32 content_difference = 1.0f;
	ui_set_column(ui)
	{
		scroll_background = ui_create_node(
				ui,
				node_clickeable |
				node_skip_content_size_y |
				node_skip_content_size_x |
				node_skip_layout_y,
				"__SCROLL_BG__");
		ui_set_parent(ui, scroll_background) ui_set_wh_ppct(ui, 1.0f, 0.0f) ui_set_interaction_layer(ui, ui_interaction_layer_mid)
		{
			scroll_region = ui_create_node(
					ui,
					node_clickeable |
					node_skip_content_size_y |
					node_skip_content_size_x |
					node_skip_layout_x |
					node_skip_layout_y,
					"__SCROLL_REG__");
		}

		ui_set_parent(ui, scroll_background)
		{
			//scale wheel area with background

			i16 content_inside_frame_y = 
				(i16)total_node_content;
			if(content_inside_frame_y > frame_h)
			{
				content_difference = (f32)frame_h / content_inside_frame_y;
			}
			f32 scrollbar_size = content_difference * frame_h;

			ui_set_height(ui, ui_size_specified(scrollbar_size, 1.0f))
				ui_set_color(ui, ui_color_background, ui_COLOR_SCROLLBAR)
				{
					ui_set_row(ui)
						ui_set_w_ppct(ui, 1.0f, 0.0f)
						{
							ui_space_between(ui, ui_size_specified(2.0f, 1.0f))
							{
								scroll_bar = ui_create_node(
										ui,
										node_clickeable |
										node_background |
										node_active_animation |
										node_hover_animation |
										node_skip_content_size_y |
										node_skip_content_size_x |
										node_skip_layout_y,
										"__SB__");
							}
						}
					scroll_bar->added_y = (i16)(scroll_value * content_difference);
				}
		}
	}

	ui_interaction_info scroll_bar_interaction_info = ui_interaction_from_node(
			ui, scroll_bar);
//	ui_interaction_info scroll_bar_bg_interaction_info = ui_interaction_from_node(
//			ui, scroll_background);
	ui_usri scroll_region_usri = ui_interaction_from_node(ui, scroll_region);

	bool32 scroll_by_wheel = (
			scroll_region_usri.flags & ui_interaction_mouse_hover
			);

	f32 sv = scroll_value;
	if(ui_usri_mouse_l_down(scroll_bar_interaction_info))
	{
		vec2 delta_from_top = ui_mouse_delta_from_node(ui, scroll_background);
		sv = delta_from_top.y / content_difference;
	}
	else if(scroll_by_wheel && ui->input.mouse_wheel)
	{
		sv = scroll_value + (-ui->input.mouse_wheel / content_difference * 10);
	}

	return(sv);
}

static inline f32 
ui_scroll_vertical1(
		game_ui *ui,
		f32 frame_h,
		f32 total_node_content,
		f32 scroll_value,
		u8 *label)
{
	ui_push_id_string(ui, label);
	f32 sv = ui_scroll_vertical(
			ui,
			frame_h,
			total_node_content,
			scroll_value);
	ui_pop_id(ui);

	return(sv);
}

	static inline f32 
ui_scroll_horizontal(
		game_ui *ui,
		f32 frame_w,
		f32 total_node_content,
		f32 scroll_value)
{
	ui_node *scroll_background;
	ui_node *scroll_bar;
	f32 content_difference = 1.0f;
	ui_set_row(ui)
	{
		scroll_background = ui_create_node(
				ui,
				node_clickeable,
				"__SCROLL_BG__X__");
		ui_set_parent(ui, scroll_background)
		{
			//scale wheel area with background

			i16 content_inside_frame_x = 
				(i16)total_node_content;
			if(content_inside_frame_x > frame_w)
			{
				content_difference = (f32)frame_w / content_inside_frame_x;
			}
			f32 scrollbar_size = content_difference * frame_w;

			ui_set_width(ui, ui_size_specified(scrollbar_size, 1.0f))
				ui_set_color(ui, ui_color_background, ui_COLOR_SCROLLBAR)
				{
					ui_set_column(ui)
						ui_set_h_ppct(ui, 1.0f, 0.0f)
						{
							ui_space_between(ui, ui_size_specified(2.0f, 1.0f))
							{
								scroll_bar = ui_create_node(
										ui,
										node_clickeable |
										node_background |
										node_active_animation |
										node_hover_animation,
										"__SB__X__");
							}
						}
					scroll_bar->added_x = (i16)(scroll_value * content_difference);
				}
		}
	}

	ui_interaction_info scroll_bar_interaction_info = ui_interaction_from_node(
			ui, scroll_bar);
	ui_interaction_info scroll_bar_bg_interaction_info = ui_interaction_from_node(
			ui, scroll_background);

	bool32 scroll_by_wheel = (
			scroll_bar_interaction_info.flags & ui_interaction_mouse_hover ||
			scroll_bar_bg_interaction_info.flags & ui_interaction_mouse_hover
			);

	f32 sv = 0;
	if(ui_usri_mouse_l_down(scroll_bar_interaction_info))
	{
		vec2 mdt = ui_mouse_delta(ui);
		sv += mdt.x / content_difference;
	}
	else if(scroll_by_wheel)
	{
		sv = -ui->input.mouse_wheel / content_difference * 10;
	}

	return(sv);
}

static inline ui_node * 
ui_scroll_box(
		game_ui *ui,
		ui_usri *usri_out,
		u8 *label)
{
	ui_node *scroll_region;
	ui_node *background;
	ui_node *box;

	ui_push_id_string(ui, label);
	background = ui_create_node(ui,
			node_clickeable | node_use_extra_flags | node_background | node_border,
			"_SIGNAL__");
	b32 scroll_by_wheel = 0;
	ui_node *scroll_set_node = 0;

	ui_set_parent(ui, background)
	{
		ui_space_specified(ui, 6.0f, 1.0f);
		//ui_set_w_strictness(ui, 0.0f)
		//ui_set_h_strictness(ui, 0.0f)
		ui_push_row(ui, 0, 0);
		{
			//padding
			ui_space_specified(ui, 6.0f, 1.0f);

			//ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
			ui_set_wh(ui, ui_size_sum_of_children(0.0f))
			{
				box = ui_create_node(ui, node_clip, label);
			}

			//padding
			ui_space_specified(ui, 6.0f, 1.0f);
		}

		ui_space_specified(ui, ui_SZ_SCROLL_WH, 1.0f);

		ui_node *scp = 0;
		ui_set_wh_ppct(ui, 1.0f, 1.0f)
			scp = ui_create_node(ui, node_skip_layout_x | node_skip_layout_y, 0);
		ui_set_w_specified(ui, ui_SZ_SCROLL_WH, 1.0f) ui_set_parent(ui, scp) ui_set_row(ui)
		{
			ui_space_ppct(ui, 1.0f, 0.0f);
			f32 scroll_delta = ui_scroll_vertical(
					ui,
					background->size[1] - 12.0f,
					box->content_size[1],
					box->scroll_y
					);
			if(scroll_delta)
			{
				ui_node_change_target_scroll(box, scroll_delta, ui_axis_y);
			}
			ui_set_wh_ppct(ui, 1.0f, 1.0f)
			{
				scroll_region = ui_mid_region(ui);
			}

		}
		ui_pop_row(ui);

		ui_space_specified(ui, 6.0f, 1.0f);
	}
	ui_usri box_usri = ui_usri_from_node(ui, scroll_region);

	scroll_by_wheel = ui_usri_mouse_hover(box_usri);
	if(usri_out)
	{
		*usri_out = box_usri;
	}

	if(scroll_by_wheel)
	{

		f32 sv = ui_calculate_scroll_value(
				ui,
				box->size_y,
				box->content_size[ui_axis_y]);
		ui_node_change_target_scroll(box, sv, ui_axis_y);
	}
	ui_pop_id(ui);
	return(box);
}


inline u32 
_ui_window_begin(game_ui *ui,
		ui_panel_flags panel_flags,
		f32 panel_x,
		f32 panel_y,
		f32 panel_w,
		f32 panel_h,
		b32 use_scroll,
		bool32 *open_close_ptr,
		u8 *title)
{
	ui_node *title_node;
	//box where the widgets and scroll bar go
	ui_node *main_background_node;
	//where widgets go
	ui_node *panel_widgets_space_node;

	panel_flags |= ui_panel_flags_move | ui_panel_flags_init_once;
	ui_panel *panel = ui_push_root_for_rendering(ui,
			panel_x,
			panel_y,
			panel_w,
			panel_h,
			panel_flags,
			title);


	u32 is_opened_and_visible = !panel->closed;
	if(!is_opened_and_visible)
	{
		return(0);
	}


	title_node = ui_title_bar(ui);

	//title, the height is pre-defined by a macro and the of
	//its width is the same as the window
	ui_set_parent(ui, title_node)
	{
		ui_space(ui, ui_size_specified(6, 1.0f));
		ui_set_row(ui)
		{
			ui_space(ui, ui_size_specified(6, 1.0f));
			ui_set_wh(ui, ui_size_text(0, 1.0f)) ui_text(ui, title);

			ui_space(ui, ui_size_percent_of_parent(1.0f, 0.0f)); 
			ui_set_wh(ui, ui_size_specified(14.0f, 1.0f))
			{
				ui_node *close_button = ui_create_node(
						ui,
						node_clickeable |
						node_background |
						node_text |
						node_text_centered |
						node_border |
						node_hover_animation |
						node_active_animation,
						"X#__CLOSE__");
				ui_space(ui, ui_size_specified(6.0f, 1.0f));
				if(ui_node_mouse_l_up(ui, close_button))
				{
					ui_close_panel_ptr(ui, panel);
				}
			}
			ui_pop_width(ui);
			ui_pop_height(ui);
		}
	}


	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
		ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
		{
			main_background_node = ui_create_node(
					ui,  
					node_clip |
					node_background | 
					node_clickeable,
					title);
			//
		}


	ui_set_parent(ui, main_background_node)
	{

		ui_set_row(ui)
		{
			//scroll region before the widget region, in case of
			//boxes with scrollable regions.
			ui_node *sc;
			if(use_scroll)
			{
				ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 1.0f))
				{
					sc = ui_mid_region(ui);
				}
			}

			//where widgets go
			ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
				ui_set_width(ui, ui_size_percent_of_parent(1.0f, 0.0f))
				{
					ui_set_column(ui)
					{
						ui_node_flags node_flags = node_clip | (use_scroll ? node_scroll_y : 0);
						panel_widgets_space_node = ui_create_node(ui,
								node_flags,
								"__WIDGET_REGION__");
						panel_widgets_space_node->padding_x = 4;
						panel_widgets_space_node->padding_y = 4;
					}
				}
			//scroll bar
			if(use_scroll)
			{
				ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
					ui_set_width(ui, ui_size_specified(ui_SZ_SCROLL_WH, 1.0f))
					{

						f32 scroll_dt_y = ui_scroll_vertical(ui,
								panel_widgets_space_node->size_y,
								panel_widgets_space_node->content_size[ui_axis_y],
								panel_widgets_space_node->scroll_y);
						//scroll based on the bar
						if(scroll_dt_y != panel_widgets_space_node->scroll_y)
						{
							ui_node_set_target_scroll(panel_widgets_space_node, scroll_dt_y, ui_axis_y);
						}//scroll region
						else if(ui_node_mouse_hover(ui, sc))
						{
							f32 sv = ui_node_scroll_delta_y(
									ui, panel_widgets_space_node);
							if(sv)
							{
								ui_node_change_target_scroll(panel_widgets_space_node, sv, ui_axis_y);
							}
						}
					}
			}
		}
		//put a resize button on the bottom corner
		ui_node *size_button_separator_node;

		ui_push_prefered_width(ui, size_percent_of_parent, 1.0f, 1.0f);
		ui_push_prefered_height(ui, size_percent_of_parent, 1.0f, 1.0f);
		{
			//create a node floating on top of the title
			//and background without affecting their interactions
			ui_set_height(ui, ui_size_percent_of_parent(1.0f, 1.0f))
			{
				size_button_separator_node = ui_create_node(
						ui,
						node_skip_layout_x | 
						node_skip_layout_y, 
						0);
			}
			//put widgets on it
			ui_push_parent(ui, size_button_separator_node);
			{
				//size of resize button
				i16 rb_size = 20;
				//send the button at the bottom of the panel
				ui_space(ui, ui_size_percent_of_parent(1.0f , 0.0f));
				//push a row, sized by its children
				ui_set_row(ui)
				{
					//send the button to the bottom-right corner
					ui_space(ui, ui_size_percent_of_parent(1.0f, 0.0f)); 

					//ui_push_prefered_width(ui, size_specified, rb_size, 1.0f);
					ui_set_wh(ui, ui_size_specified(rb_size, 1.0f))
					{
						ui_push_color(ui, ui_color_background, ui_BUTTON_NORMAL_COLOR);
						{
							ui_node *size_button_node = ui_create_nodef(
									ui,
									node_background | 
									node_clickeable |
									node_active_animation |
									node_hover_animation, "%s _size_button", title);

							ui_interaction_info rb_interaction_data = ui_interaction_from_node(ui, size_button_node);
							if(rb_interaction_data.flags & ui_interaction_mouse_left_down)
							{
								f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x; 
								f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y; 
								//panel->sz.x = (f32)(i32)(panel->sz.x + mouse_delta_x);
								//panel->sz.y = (f32)(i32)(panel->sz.y + mouse_delta_y);
								ui_push_set_value_command(ui, &panel->sz.x, (f32)(i32)(panel->sz.x + mouse_delta_x));
								ui_push_set_value_command(ui, &panel->sz.y, (f32)(i32)(panel->sz.y + mouse_delta_y));
							}

						}
						ui_pop_color(ui, ui_color_background);
					}
				}
			}
			ui_pop_parent(ui, size_button_separator_node);
			//size_button_separator_node->added_x = (i32)panel->p.x;
			//size_button_separator_node->added_y = (i32)panel->p.y;
		}
		ui_pop_width(ui);
		ui_pop_height(ui);
	}


	if(ui_node_mouse_l_down(
				ui,
				main_background_node) || ui_node_mouse_l_down(ui, title_node))
	{
		f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x; 
		f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y; 
		panel->p.x = (f32)(i32)(panel->p.x + mouse_delta_x);
		panel->p.y = (f32)(i32)(panel->p.y + mouse_delta_y);
	}
	ui_push_parent(ui, panel_widgets_space_node);


	return(is_opened_and_visible);
}


/*A floating panel is mostly used for debug purposes.*/
	static inline u32
ui_window_begin(game_ui *ui,
		ui_panel_flags panel_flags,
		f32 panel_x,
		f32 panel_y,
		f32 panel_w,
		f32 panel_h,
		u8 *title)
{

	u32 is_opened_and_visible =
		_ui_window_begin(ui,
				panel_flags,
				panel_x,
				panel_y,
				panel_w,
				panel_h,
				1,
				0,
				title);
	return(is_opened_and_visible);
}

static inline u32
ui_panel_begin(game_ui *ui,
		ui_panel_flags panel_flags,
		f32 panel_x,
		f32 panel_y,
		f32 panel_w,
		f32 panel_h,
		u8 *title)
{

	u32 is_opened_and_visible =
		_ui_window_begin(ui,
				panel_flags,
				panel_x,
				panel_y,
				panel_w,
				panel_h,
				0,
				0,
				title);
	return(is_opened_and_visible);
}


inline void
ui_panel_end(game_ui *ui)
{
	ui_pop_root(ui);
}


static void
ui_run_explorer(game_ui *ui,
		bool32 *open_close_ptr)
{
	ui_explorer *explorer = ui->explorer;
	//u32 kindaHash = string_kinda_hash(id);
	//ui->explorer->process_id = kindaHash;
	//Special flags for explorer
	ui_panel_flags panel_flags = 
		(ui_panel_flags_title | 
		 ui_panel_flags_move | 
		 ui_panel_flags_close | 
		 ui_panel_flags_size_to_content |
		 ui_panel_flags_init_once |
		 ui_panel_flags_focus_when_opened);

	ui_panel *panel = ui_push_root_for_rendering(
			ui,
			200,
			200,
			400,
			400,
			panel_flags,
			"__ui_explorer__");

	if(explorer->closed)
	{
		ui_close_panel_ptr(ui, panel);
	}
	if(panel->closed && !explorer->closed)
	{
		ui_open_panel_ptr(ui, panel);
	}
	if(panel->closed)
	{
		ui_panel_end(ui);
		return;
	}

	bool32 ex_file_got_selected = 0;
	bool32 ex_clicked_file = 0;
	bool8 ex_clicked_ok = 0;
	bool8 ex_clicked_cancel = 0;
	//select new file when clicked

	ui_node *explorer_title;
	ui_node *explorer_bg;

	ui_node *explorer_contents_node;
	ui_node *explorer_files_folders;

	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
		ui_set_height(ui, ui_size_specified(TITLEHEIGHT, 1.0f))
		{
			explorer_title = ui_title_bar(ui);
			ui_set_parent(ui, explorer_title)
			{
				ui_space_specified(ui, 4.0f, 1.0f);
				ui_set_row(ui)
				{
					ui_space_specified(ui, 4.0f, 1.0f);
					ui_node *explorer_process = ui_create_node(ui,
							node_text,
							0);
					ui_node_set_display_string(ui,
							explorer_process,
							explorer->process_name);	
				}
			}
		}
	//background
	ui_set_color(ui, ui_color_background, V4(0x0f, 0x0f, 0x0f, 0xDC))
		ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 0.0f))
		{
			explorer_bg = ui_create_node(ui,
					node_background | 
					node_clickeable,
					"__EX_BG__");
		}
	ui_set_parent(ui, explorer_bg)
	{

		//space for the explorer panel widgets
		//files, folders...
		ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 0.0f))
		{
			ui_set_row(ui)
			{
				ui_space_specified(ui, 6.0f, 1.0f);
				explorer_contents_node = ui_create_node(ui,
						0,
						"__EX_WIDGETS__");
				ui_space_specified(ui, 6.0f, 1.0f);
			}
		}
		//files, folders...
		ui_set_parent(ui, explorer_contents_node)
		{

			ui_space_specified(ui, 4.0f, 1.0f);
			ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
			{
				//search for path and files
				ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
				{
#if 0
					explorer_path_search = ui_create_node(ui,
							node_clickeable |
							node_background |
							node_border,
							"__PATH_SEARCH__");
#endif
					bool32 path_changed = ui_input_text(ui,
							1,
							explorer->directory_name,
							sizeof(explorer->directory_name),
							"__PATH_SEARCH__");
					if(path_changed)
					{
						explorer->update_path_files = 1;
					}
				}
				ui_space_specified(ui, 4.0f, 1.0f);

				//show the files
				ui_set_row(ui)
				{
					ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
						ui_set_width(ui, ui_size_percent_of_parent(1.0f, 0.0f))
						{
							explorer_files_folders = ui_create_node(ui,
									node_scroll_y |
									node_clip |
									node_background |
									node_border,
									"__FILES_FOLDERS__");
						}
					//display and update files and folders
					ui_set_parent(ui, explorer_files_folders)
						ui_set_width(ui, ui_size_percent_of_parent(1.0f, 0.0f))
						ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
						{
							//if allowed, push the ... directory to go back
							ui_set_row(ui)
							{
								ui_space_specified(ui,4.0f, 1.0f);

								ui_set_column(ui)
									ui_space_between(ui, ui_size_specified(4.0f, 1.0f))
									{
										if(explorer->path_length > 1)
										{
											ui_node *ex_dir_go_back = ui_create_node(ui,
													node_text |
													node_clickeable |
													node_background |
													node_hover_animation |
													node_active_animation ,
													"..#__DIR_GO_BACK__"
													);
											//interacted
											if(ui_node_mouse_l_double_clicked(ui, ex_dir_go_back))
											{
												ui_explorer_back(explorer);
											}
										}


										ui_set_width(ui, ui_size_percent_of_parent(1.0f, 0.0f))
										{
											for(u32 i = 0;
													i < explorer->directory_file_count;
													i++)
											{
												ui_node *f;
												ui_explorer_file_attributes *current_file = explorer->current_directory_files + i;
												f = ui_selectable_boxf(ui,
														explorer->selected_file == current_file, 
														"__FILE__ %d",
														i);

												u32 icon_size = 12;
												//icon
												if(current_file->is_directory)
												{
													ui_node_push_rectangle_wh(
															ui,
															f,
															0,
															(i16)(f->size_y * 0.1f),
															icon_size,
															icon_size * 0.8f,
															V4(0, 255, 0, 255)
															);
												}
												else
												{
													ui_node_push_rectangle_wh(
															ui,
															f,
															0,
															0,
															icon_size * 0.8f,
															icon_size,
															vec4_all(255)
															);
												}
												//name
												ui_node_push_text(
														ui,
														f,
														icon_size,
														0,
														0,
														vec4_all(255),
														current_file->name);

												ui_usri f_usri = ui_interaction_from_node(ui, f);
												if(f_usri.flags & ui_interaction_mouse_left_up)
												{
													ex_clicked_file = 1;
													explorer->selected_file = current_file;
													explorer->valid_file_focused = 1;
												}
												if(f_usri.flags & ui_interaction_mouse_left_double_click)
												{
													ex_file_got_selected = 1;
													//advance to new directory
													if(current_file->is_directory)
													{
														//Advance
														u32 dirNameLength = string_count(current_file->name);
														//Remove null character
														u32 currentPathLength = string_count(explorer->directory_name) - 1;
														u32 newLength = dirNameLength + currentPathLength;
														Assert(newLength < 260);

														u32 i = currentPathLength;
														u32 c = 0;
														while(c < dirNameLength)
														{
															explorer->directory_name[i] = current_file->name[c];
															i++;
															c++;
														}
														explorer->directory_name[i - 1] = '/';
														explorer->path_length = newLength;

														explorer->update_path_files = 1;
														explorer->valid_file_focused = 0;
													}
												}
											}
										}
									}
							}


						}


					ui_set_width(ui, ui_size_specified(ui_SZ_SCROLL_WH, 1.0f))
						ui_set_height(ui, ui_size_percent_of_parent(1.0f, 0.0f))
						{
							//scroll bar
							f32 scrolled = explorer_files_folders->scroll_y;
							f32 new_scroll_value = ui_scroll_vertical(
									ui,
									explorer_files_folders->size_y,
									explorer_files_folders->content_size[ui_axis_y],
									explorer_files_folders->scroll_y);
							if(new_scroll_value)
							{
								ui_node_change_target_scroll(explorer_files_folders, new_scroll_value, ui_axis_y);
							}
						}
				}
				//file names

				ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
				{
					ui_space_specified(ui, 4.0f, 1.0f);
					ui_input_text(ui,
							0,
							explorer->process_file_name,
							sizeof(explorer->process_file_name),
							"__EX_PROCESS_FILE_NAME__");
					ui_space_specified(ui, 4.0f, 1.0f);
				}
			}
		}

		ui_set_w_em(ui, 8.0f, 1.0f)
			ui_set_h_text(ui, 4.0f, 1.0f)
			ui_set_row(ui)
			{
				ui_space_ppct(ui, 1.0f, 0.0f);
				ex_clicked_ok = ui_button(ui, "ok##__EX_OK__");
				ui_space_specified(ui, 4.0f, 0.0f);
				ex_clicked_cancel = ui_button(ui, "cancel##__EX_CANCEL__");
				ui_space_specified(ui, 4.0f, 0.0f);
			}
		ui_space_specified(ui, 4.0f, 0.0f);
	}
	ui_panel_end(ui);

	bool8 close_on_complete = explorer->flags & ui_explorer_close_on_complete;
	bool8 copy_selected_file_name = explorer->flags & ui_explorer_copy_selected_file_name;
	bool8 process_select_file = explorer->flags & ui_explorer_select_file;
	b8 process_type_name = explorer->flags & ui_explorer_type_file;


	//holding mouse left on the explorer's background
	bool32 move_panel = (ui_node_mouse_l_down(ui, explorer_contents_node)) ||
		(ui_node_mouse_l_down(ui, explorer_bg)) ||
		(ui_node_mouse_l_down(ui, explorer_title));
	if(move_panel)
	{
		vec2 mouse_dt = ui_mouse_delta(ui);
		panel->p.x += mouse_dt.x;
		panel->p.y += mouse_dt.y;
	}


	//
	//...
	//
	ui_explorer_file_attributes *selected_file_data = 0;
	if(explorer->valid_file_focused)
	{
		selected_file_data = explorer->selected_file; 
	}

	if(ex_clicked_file && copy_selected_file_name)
	{
		memory_clear(explorer->process_file_name, sizeof(explorer->process_file_name));
		string_copy(selected_file_data->name, explorer->process_file_name);
	}
	//file is selected, complete process
	if((ex_file_got_selected || ex_clicked_ok))
	{

		if(process_select_file)
		{
			u8 *directory_name = explorer->directory_name;
			u8 *selected_file_path_and_name = explorer->selected_file->path_and_name;
			u8 process_input_path_and_name[256] = {0};
			//pick the current selected file and concadenate it with the full path
			string_concadenate(
					explorer->directory_name,
					explorer->process_file_name,
					process_input_path_and_name,
					sizeof(process_input_path_and_name));
			u32 process_input_id = ui_explorer_generate_id(process_input_path_and_name);
			ui_explorer_file_attributes *existing_file = 0;
			for(u32 f = 0; f < explorer->directory_file_count && !existing_file; f++)
			{
				if(explorer->current_directory_files[f].id == process_input_id)
				{
					existing_file = explorer->current_directory_files + f;
				}
			}

			if(existing_file)
			{
				explorer->last_process_completed = 1;
				string_copy(
						existing_file->path_and_name,
						explorer->full_process_path_and_name);
			}
		}
		else if(!process_select_file)
		{
			explorer->last_process_completed = 1;
		}
	}

	if((explorer->last_process_completed & close_on_complete) || 
			ex_clicked_cancel)
	{
		explorer->closed = 1;
		explorer->selected_file = 0;
	}
	//Switched directory, completed process, etc...
	if(explorer->update_path_files)
	{
		explorer->selected_file = 0;
		explorer->valid_file_focused = 0;
		explorer->update_path_files = 0;

		memory_clear(explorer->process_file_name, sizeof(explorer->process_file_name));

		platform_api *platform = explorer->platform;

		platform_file_search_info fileData = {0};
		//combine directory name with pattern
		//format_text(textBuffer, sizeof(textBuffer), "%s*", explorer->directory_name);

		u32 totalFileSlots = ARRAYCOUNT(
				explorer->current_directory_files);
		//Search directories
		explorer->directory_file_count = 0;
		u8 textBuffer[312] = {0};
		format_text(textBuffer, sizeof(textBuffer), "%s*", explorer->directory_name);
		platform_file_search directorySearch = platform->f_find_first_file(textBuffer, &fileData);

		if(directorySearch.handle)
		{
			//Ignore '.' and '..' directories.
			(platform->f_find_next_file(directorySearch, &fileData));

			//Pick folder from array
			ui_explorer_file_attributes *current_file = explorer->current_directory_files + explorer->directory_file_count;

			while(platform->f_find_next_file(directorySearch, &fileData))
			{
				//Pick file
				if(fileData.is_directory)
				{
					ui_explorer_file_attributes *current_file = explorer->current_directory_files + explorer->directory_file_count;
					current_file->is_directory = fileData.is_directory;
					current_file->date = fileData.write_time;
					memory_clear(current_file->name, sizeof(current_file->name));
					memory_clear(current_file->path, sizeof(current_file->path));
					memory_clear(current_file->path_and_name, sizeof(current_file->path_and_name));
					string_copy(fileData.name, current_file->name);
					string_copy(explorer->directory_name, current_file->path);
					string_concadenate(current_file->path,
							current_file->name,
							current_file->path_and_name,
							sizeof(current_file->path_and_name));
					current_file->id = ui_explorer_generate_id(current_file->path_and_name);

					explorer->directory_file_count++;
					Assert(explorer->directory_file_count < totalFileSlots); 
				}
				//Advance
			}

			platform->f_find_close(directorySearch);
		}
		//Search by wildcard
		format_text(textBuffer, sizeof(textBuffer), "%s%s", explorer->directory_name, explorer->search_pattern);

		platform_file_search fileSearch = platform->f_find_first_file(textBuffer, &fileData);//, platform_file_type_Normal);
		if(fileSearch.handle)
		{
			do 
			{
				//Pick file
				if(!fileData.is_directory)
				{
					ui_explorer_file_attributes *current_file = explorer->current_directory_files + explorer->directory_file_count;
					memory_clear(current_file->name, sizeof(current_file->name));
					memory_clear(current_file->path, sizeof(current_file->path));
					memory_clear(current_file->path_and_name, sizeof(current_file->path_and_name));
					string_copy(fileData.name, current_file->name);
					string_copy(explorer->directory_name, current_file->path);
					current_file->is_directory = 0;
					current_file->size  = fileData.size;
					current_file->date  = fileData.write_time;
					string_concadenate(current_file->path,
							current_file->name,
							current_file->path_and_name,
							sizeof(current_file->path_and_name));
					current_file->id = ui_explorer_generate_id(current_file->path_and_name);
					Assert(explorer->directory_file_count < totalFileSlots);
					explorer->directory_file_count++;
				}
				//Set data
			}while(platform->f_find_next_file(fileSearch, &fileData));

			platform->f_find_close(fileSearch);
		}
	}
}

//explorer functions
	static void
ui_explorer_set_path(game_ui *ui, u8 *path)
{
	ui_explorer *explorer = ui->explorer;
	//Advance
	u32 path_length     = string_count(path);
	//Remove null character
	u32 current_path_length = string_count(explorer->directory_name);
	u32 newLength = current_path_length;
	Assert(newLength < 260);


	memory_clear(explorer->directory_name, current_path_length);
	string_copy(path, explorer->directory_name);

	explorer->directory_name[path_length - 1] = '/';
	explorer->path_length = path_length + 1;
	explorer->update_path_files = 1;
}


	static void 
ui_explorer_set_process(
		game_ui *ui,
		ui_explorer_flags flags,
		u8 *id)
{
	ui_explorer *explorer = ui->explorer;

	if(explorer)
	{

		u32 kindaHash = string_kinda_hash(id);
		//explorer->panel->title = id;

		if(explorer->process_id != kindaHash)
		{
			explorer->closed = 0;
			string_copy(id, explorer->process_name);
		}
		else
		{
			explorer->closed = !explorer->closed;
			memory_clear(explorer->search_pattern, 24);
			explorer->search_pattern[0] = '*';
		}

		explorer->started_process = 1;
		explorer->flags = flags;
		explorer->process_id = kindaHash;
		explorer->files_focused = 1;
		explorer->update_path_files = 1;
	}

}

	static void
ui_explorer_end_process(
		game_ui *ui)
{
	ui_explorer *explorer = ui->explorer;
	//explorer->panel->title = id;
	explorer->closed    = 1;
	explorer->process_id = 0;

}

	inline u32 
ui_explorer_CompletedSave(game_ui *ui)
{
	u32 completed = 0;
	if(ui->explorer->okay_pressed)
	{
		completed = path_and_name_is_valid(ui->explorer->process_file_name);
	}
	return(completed);
}

#define ui_explorer_last_selected_file_path_and_name(ui) (ui->explorer->selected_file_path_and_name)
	inline void
ui_explorer_get_file_path(game_ui *ui, u8 *dest, u32 destSize)
{
	ui_explorer *explorer = ui->explorer;
	u8 *directory_name = explorer->directory_name;
	u8 *process_file_name = explorer->process_file_name;
	string_concadenate(directory_name, process_file_name, dest, destSize);

}



//ui_explorer_set_process(ui, ui_explorer_process_type_file | ui_explorer_flags_SelectFile, "Save model");

	static u32
ui_explorer_check_process(game_ui *ui, u8 *id)
{
	u32 success = 0;
	ui_explorer *explorer = ui->explorer;

	if(1)
	{
		u32 kindaHash = string_kinda_hash(id);
		success = ui->explorer->last_process_completed && ui->explorer->process_id == kindaHash;
		if(success)
		{
			//Reset process result
			explorer->last_process_completed = 0;
		}
	}

	return(success);
}






static inline void
ui_selectable_u32(game_ui *ui, u32 set_value, u32 *value_to_set, u8 *label)
{
	bool32 active = 0;
	ui_node *selectable_node;
	if(value_to_set)
	{
		active = (*value_to_set) == set_value;
	}
	vec4 active_color = ui->theme.background_color;

	if(active)
	{
		active_color = ui->theme.hot_color;
	}

	ui_set_color(ui,
			ui_color_background,
			active_color)
	{
		selectable_node = ui_create_node(ui,
				node_background |
				node_clickeable |
				node_active_animation |
				node_hover_animation |
				node_text |
				node_text_centered |
				node_border,
				label);
	}
	ui_interaction_info ite = ui_interaction_from_node(ui, selectable_node);
	if(value_to_set && ite.flags & ui_interaction_mouse_left_up)
	{
		*value_to_set = set_value;
	}
}

static inline void
ui_selectable_text(game_ui *ui, u32 set_value, u32 *value_to_set, u8 *label)
{
}

static inline void
ui_console(
		game_ui *ui,
		s_game_console *log_console,
		u8 *label)
{
	ui_node *console_box_node;
	ui_node *console_output_node;
	ui_node *console_input_node;
	ui_node *console_output_scroll_region = 0;

	ui_push_id_string(ui, label);

//	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
	console_box_node = ui_create_node(ui, 0, 0);
	ui_set_parent(ui, console_box_node)
	{
		ui_set_row(ui)
		ui_set_h_text(ui, 4.0f, 1.0f)
		ui_set_w_text(ui, 1.0f, 1.0f)
		{
			ui_space_specified(ui, 4.0f, 1.0f);
			ui_text(ui, "buffer_used/total : {");
			f32 total_used_percent = !log_console->buffer_total_size ? 0 :
				(f32)log_console->buffer_used_size / (f32)log_console->buffer_total_size;
			if(total_used_percent > 0.85f)
			{
				ui_set_color(ui, ui_color_text, V4(255, 0, 0, 255))
				{
					ui_textf(ui, "%u", log_console->buffer_used_size);
				}
			}
			else if(total_used_percent > 0.5f)
			{
				ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
				{
					ui_textf(ui, "%u", log_console->buffer_used_size);
				}
			}
			else
			{
				ui_textf(ui, "%u", log_console->buffer_used_size);
			}
			ui_textf(ui, "/%u}", log_console->buffer_total_size);

		}
//		ui_set_height(ui, ui_size_specified(300, 1.0f))
		ui_set_h_ppct(ui, 1.0f, 0.0f)
		{
			//where text is shown
			console_output_node = ui_create_node(
					ui,
					node_clickeable |
					node_clip |
					node_background |
					node_scroll_y |
					node_border,
					label);
			ui_set_parent(ui, console_output_node)
			{
				console_output_scroll_region = ui_mid_region(ui);
			}


			u8 *buffer = log_console->buffer;
			vec2 text_dimensions = {0};
			if(buffer)
			{
				text_dimensions = font_get_text_size_wrapped_scaled(
						&ui->fontp,
						console_output_node->size_x,
						buffer,
						ui->font_scale);

				ui_set_parent(ui, console_output_node)
				{
					ui_space(ui, ui_size_specified(text_dimensions.y, 1.0f));
				}

				ui_node_push_text(
						ui,
						console_output_node,
						0,
						0 - (i16)console_output_node->scroll_y, //scroll y
						0,
						vec4_all(255),
						buffer
						);
			}
		}

		ui_set_height(ui, ui_size_text(1.0f, 1.0f))
		{
			console_input_node = ui_create_nodef(
					ui,
					node_clickeable |
					node_background |
					node_border,
					"%s_c_input",
					label);
		}
	}

	if(console_output_node->scroll_hits_bounds[ui_axis_y]
			&& log_console->buffer_updated)
	{
		console_output_node->target_scroll_y = 1000000;
		console_output_node->dt_scroll[1] = 1.0f;
	}
	//update scrolling
	ui_interaction_info output_console_interaction = ui_interaction_from_node(
			ui, console_output_scroll_region);
	if(output_console_interaction.flags & ui_interaction_mouse_hover)
	{
		f32 scroll_value = ui_calculate_scroll_value(ui, 
				console_output_node->size_y,
				console_output_node->content_size[1]);
		ui_node_change_target_scroll(
				console_output_node, scroll_value, ui_axis_y);
	}

	_ui_update_input_text_from_node(
			ui,
			console_input_node,
			0,
			0,
			log_console->input_buffer_max,
			&log_console->input_buffer_count,
			log_console->input_buffer);

	b32 entered = ui->input_text->entered;
	//set the scroll to the bottom if pushed something
	if(entered && ui->input_text->key_count)
	{

		//calculate the new input size
		vec2 console_input_size = font_get_text_size_wrapped_scaled(
						&ui->fontp,
						console_output_node->size_x,
						log_console->input_buffer,
						ui->font_scale);

		ui_set_parent(ui, console_output_node)
		{
			//put a space for the next input command
			ui_space(ui, ui_size_specified(console_input_size.y, 1.0f));
			console_output_node->target_scroll_y = 1000000;
			console_output_node->dt_scroll[1] = 1.0f;
		}
		ui->input_text->key_count = 0;
		input_text_reset_cursor(ui->input_text);

		console_push_input_command(log_console);
		//when using confirm on enter. The ui uses a backup buffer
		//to prevent changes made to the original
//		ui_clear_input_text_buffer(ui);
	}
	ui_pop_id(ui);

}

//
//Debugging and testing
//
static inline void
ui_radio_button_node(game_ui *ui, bool32 active, u8 *label)
{
	ui_node *rb_node = 0;
	ui_set_width_height(ui, ui_size_sum_of_children(1.0f))
	rb_node = ui_create_node(ui,
			node_clickeable,
			label);

	ui_set_parent(ui, rb_node)
	{
		ui_space(ui, ui_size_specified(4, 1.0f));
		ui_set_row(ui)
		{
			//put text inside
			ui_space(ui, ui_size_specified(4, 1.0f));

			u32 c_size = 12;
			ui_set_width_height(ui, ui_size_specified(c_size, 1.0f))
			{
				ui_space(ui, ui_size_specified(4, 1.0f));

				ui_node *cb_check = ui_create_node(ui,
						0, 0);
				ui_node_push_hollow_rectangle(ui,
						cb_check,
						0,
						0,
						c_size,
						c_size,
						1,
						V4(255, 255, 255, 255));
				if(active)
				{
					ui_node_push_rectangle(
							ui,
							cb_check,
							2,
							2,
							c_size - 2,
							c_size - 2,
							V4(255, 255, 255, 255));
				}
				ui_space(ui, ui_size_specified(4, 1.0f));
			}


			ui_node *cb_text = ui_create_node(ui,
					node_text,
					0);
			ui_node_set_display_string(ui,
					cb_text,
					rb_node->display_string);
		}
		ui_space(ui, ui_size_specified(4, 1.0f));
	}
}

static inline u32 
ui_checkbox(game_ui *ui, void *value, u8 *label)
{
	ui_node *checkbox_node = 0;
	ui_set_width_height(ui, ui_size_sum_of_children(1.0f))
	checkbox_node = ui_create_node(ui,
			node_clickeable |
			node_border |
			node_background |
			node_hover_animation |
			node_active_animation,
			label);

	u8 *cb_value_ptr = value;

	ui_set_parent(ui, checkbox_node)
	{
		ui_space(ui, ui_size_specified(4, 1.0f));
		ui_set_row(ui)
		{
			//put text inside
			ui_space(ui, ui_size_specified(4, 1.0f));

			ui_node *cb_text = ui_create_node(ui,
					node_text,
					0);
			ui_node_set_display_string(ui,
					cb_text,
					checkbox_node->display_string);

			u32 c_size = 12;
			ui_set_width_height(ui, ui_size_specified(c_size, 1.0f))
			{
				ui_space(ui, ui_size_specified(4, 1.0f));

				ui_node *cb_check = ui_create_node(ui,
						0, 0);
				ui_node_push_hollow_rectangle(ui,
						cb_check,
						0,
						0,
						c_size,
						c_size,
						1,
						V4(255, 255, 255, 255));

				if(cb_value_ptr && *cb_value_ptr)
				{
					ui_node_push_rectangle(
							ui,
							cb_check,
							2,
							2,
							c_size - 2,
							c_size - 2,
							V4(255, 255, 255, 255));
				}
				ui_space(ui, ui_size_specified(4, 1.0f));
			}
		}
		ui_space(ui, ui_size_specified(4, 1.0f));
	}

	bool32 clicked = 0;
	if(cb_value_ptr && ui_node_mouse_l_up(ui, checkbox_node))
	{
		*cb_value_ptr= !*cb_value_ptr;
		clicked = 1;
	}
	return(clicked);
}

static inline b32
ui_checkbox_flag(game_ui *ui, u32 flag_to_set, void *value, u8 *label)
{
	u32 flags = *(u32 *)value;
	b8 checked = flags & flag_to_set;
	b32 clicked = ui_checkbox(ui, &checked, label);

	if(!checked && (flags & flag_to_set))
	{
		flags &= ~flag_to_set;
	}
	else if(checked)
	{
		flags |= flag_to_set;
	}
	(*(u32 *)value) = flags;
	return(clicked);
}







//#================================#
//#================================#
//#================================#


#define ui_tab_group(ui, tab_index, label) for(struct __s__tg {ui_node *mtgn; int i;}\
		__tgs__ = {ui_begin_tab_group(ui, tab_index, label), 0};\
		                           !__tgs__.i;\
								   __tgs__.i += 1, (ui_end_tab_group(ui, __tgs__.mtgn)))
static inline ui_node *
ui_begin_tab_group(game_ui *ui, u32 *tab_index, u8 *label)
{
	ui_node *main_tab_group_node = 0;
	ui_set_width_height(ui, ui_size_sum_of_children(1.0f))
	main_tab_group_node = ui_create_node(ui,
			0,
			label);

	main_tab_group_node->tab_count = 0;
	*tab_index = main_tab_group_node->tab_index;

	ui_push_parent(ui, main_tab_group_node);
	ui_push_row(ui, 0, 0);

	ui_space(ui, ui_size_specified(4, 1.0f));

	return(main_tab_group_node);
}

static inline void
ui_end_tab_group(game_ui *ui, ui_node *mtgn)
{
    ui_pop_row(ui);
	ui_pop_parent(ui, mtgn);
}

#define ui_tab(ui, label) _ui_tab(ui, __tgs__.mtgn, label)
static inline u32
_ui_tab(game_ui *ui, ui_node *mtgn, u8 *label)
{
	bool32 active = (mtgn->tab_index == mtgn->tab_count);
	bool32 tab_node_clicked = 0;
	ui_extra_flags(ui, node_border | node_text_centered)
	{
		ui_set_color(ui, ui_color_background, ui_COLOR_F_BACKGROUND)
		{
			tab_node_clicked = ui_selectable(
					ui, active, label);
		}
	}
	ui_space(ui, ui_size_specified(4, 1.0f));
	if(tab_node_clicked)
	{
		mtgn->tab_index = mtgn->tab_count;
	}

	mtgn->tab_count++;

	return(tab_node_clicked);
}

static inline b32 
ui_game_orientation_selection(
		game_ui *ui, 
		u32 orientation_count,
		void *selected_orientation_ptr)
{
	u8 *selected_orientation = selected_orientation_ptr;
	u32 orientations[] = {0, 1, 2, 4, 8, 16};

	ui_node *so_background = ui_create_node(ui, 
			node_clickeable |
			node_border |
			node_background,
			"__SO_BK__");

	vec2 line_origin = {
		so_background->size_x * 0.5f,
		so_background->size_y * 0.5f};

	f32 max_angle = PI * 2;
	f32 angle_sum = max_angle / orientation_count;
	f32 angle_current = 0;

	vec2 border_l = {
		0,
	    line_origin.y};
	vec2 border_r = {
		so_background->size_x,
		border_l.y};

	vec2 border_u = {
line_origin.x,	
		0};
	vec2 border_d = {
		border_u.x,
		so_background->size_y};

	vec4 unselected_direction_color = {0, 160, 40, 200};
	vec4 selected_direction_color   = {160, 160, 0, 255};
	ui_id selected_direction_id = {0};

	u32 current_direction = 0;
	u32 focused_direction = 0;
	f32 current_mouse_line_distance = -100000.0f;

	vec2 absolute_origin =
	{so_background->region.x0 + line_origin.x,
		so_background->region.y0 + line_origin.y};
	vec2 distance_cursor_origin = vec2_normalize(vec2_sub(ui->mouse_point, absolute_origin ));
	b32 interacted = ui_node_mouse_l_down(ui, so_background);

	while(angle_current < max_angle)
	{
		vec2 line_end = {
			sin32(angle_current),
			cos32(angle_current)};

		//line_end.x *= selectable_w;
		//line_end.y *= selectable_h;


		//get the distance to the square faces
		vec2 distance_line_l = vec2_sub(border_l, line_origin);
		vec2 distance_line_r = vec2_sub(border_r, line_origin);
		vec2 distance_line_u = vec2_sub(border_u, line_origin);
		vec2 distance_line_d = vec2_sub(border_d, line_origin);

		f32 inner_rl = 0;
		f32 inner_ud = 0;

		if(line_end.y > 0)
		{
			inner_ud = distance_line_d.y / line_end.y;
		}
		else
		{
			inner_ud = distance_line_u.y / line_end.y;
		}

		if(line_end.x > 0)
		{
			inner_rl = distance_line_r.x / line_end.x;
		}
		else
		{
			inner_rl = distance_line_l.x / line_end.x;
		}

		if(!line_end.x || inner_ud < inner_rl)
		{
			line_end.x *= inner_ud;
			line_end.y *= inner_ud;
		}
		else
		{
			line_end.x *= inner_rl;
			line_end.y *= inner_rl;
		}
		//add the origin to correctly place the end point
		line_end.x += line_origin.x;
		line_end.y += line_origin.y;


		vec2 distance_end_origin = vec2_normalize(vec2_sub(line_end, line_origin));
		f32 distance_cursor_line_inner = vec2_inner(distance_end_origin, distance_cursor_origin);

		b32 current_direction_hot = 0;
		//compare inner products to set the id
		if(distance_cursor_line_inner > current_mouse_line_distance)
		{
			current_mouse_line_distance = distance_cursor_line_inner;
			focused_direction = current_direction;
		}


		u32 current_direction_selected = *selected_orientation == current_direction;

		vec4 dir_color = unselected_direction_color;
		if(current_direction_selected)
		{
			dir_color = selected_direction_color;
		}

		line_end = vec2_round_to_int(line_end);
		//render lines
		ui_node_push_line(ui,
				so_background,
				line_origin,
				line_end,
				2.0f,
				dir_color); 
	//	line_end.x = 0;

		angle_current     += angle_sum;
		current_direction += 1;
	}

	if(interacted)
	{
		*selected_orientation = focused_direction;
	}
	return(interacted);
}


typedef struct{
	render_texture *texture;
	u32 padding_x;
	u32 padding_y;
	b8 *selecting;
	b8 *hot;
	i32 zoom;
	i16 image_cursor_x;
	i16 image_cursor_y;
	ui_node *n;
}ui_image_selection_data;

static inline void
ui_image_selection_draw_frames(
		game_ui *ui,
		ui_image_selection_data selection_data,
		u32 frame_x,
		u32 frame_y,
		u32 frame_w,
		u32 frame_h,
		vec4 color)
{
	ui_node *n = selection_data.n;
	i32 zoom = selection_data.zoom;
	render_texture *texture = selection_data.texture;
//	f32 zoom = n->zoom;
//	b32 image_hot = image_hot;
	b32 uvs_interacted = 0;
	b32 interacting = 0;

	ui_push_wh_specified(ui, 0, 0);
	ui_node *image_node = ui_create_node(ui, 0, 0);
	ui_pop_wh(ui);

	i32 image_spacing = 0;//(i32)(8 * zoom);
	f32 scroll_dt_x = n->target_scroll[ui_axis_x];
	f32 scroll_dt_y = n->target_scroll[ui_axis_y];

	i16 img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
	i16 img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;


	f32 frame_x0_selection = (f32)frame_x * zoom;
	f32 frame_y0_selection = (f32)frame_y * zoom;
	f32 frame_w_selection = (f32)frame_w * zoom;
	f32 frame_h_selection = (f32)frame_h * zoom;
	f32 frame_x1_selection = frame_x0_selection + frame_w_selection;
	f32 frame_y1_selection = frame_y0_selection + frame_h_selection;

	vec2 uv0_selection = {frame_x0_selection, frame_y1_selection};
	vec2 uv1_selection = {frame_x0_selection, frame_y0_selection};
	vec2 uv2_selection = {frame_x1_selection, frame_y0_selection};
	vec2 uv3_selection = {frame_x1_selection, frame_y1_selection};

	f32 selection_rec_size = 8.0f;

	//offset these coordinates to draw the squares inside the rectangle
#if 0
	f32 uv0_x = frame_x0_selection - 0;
	f32 uv0_y = frame_y1_selection - selection_rec_size;

	f32 uv1_x = frame_x0_selection - 0;
	f32 uv1_y = frame_y0_selection + 0;

	f32 uv2_x = frame_x1_selection - selection_rec_size;
	f32 uv2_y = frame_y0_selection + 0;

	f32 uv3_x = frame_x1_selection - selection_rec_size;
	f32 uv3_y = frame_y1_selection - selection_rec_size;

#endif

	//RENDER

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv1_selection,
			2,
			color);

	ui_node_push_line(ui, image_node,
			uv1_selection,
			uv2_selection,
			2,
			color);

	ui_node_push_line(ui, image_node,
			uv2_selection,
			uv3_selection,
			2,
			color);

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv3_selection,
			2,
			color);

}

static inline void
ui_image_selection_draw_uvs(
		game_ui *ui,
		ui_image_selection_data selection_data,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		vec4 color)
{
	ui_node *n = selection_data.n;
	i32 zoom = selection_data.zoom;
	render_texture *texture = selection_data.texture;
//	f32 zoom = n->zoom;
//	b32 image_hot = image_hot;
	b32 uvs_interacted = 0;
	b32 interacting = 0;

	ui_push_wh_specified(ui, 0, 0);
	ui_node *image_node = ui_create_node(ui, 0, 0);
	ui_pop_wh(ui);

	i32 image_spacing = 0;//(i32)(8 * zoom);
	f32 scroll_dt_x = n->target_scroll[ui_axis_x];
	f32 scroll_dt_y = n->target_scroll[ui_axis_y];

	i16 img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
	i16 img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;

	vec2 uv0_scaled = {uv0.x * texture->width, uv0.y * texture->height};
	vec2 uv1_scaled = {uv1.x * texture->width, uv1.y * texture->height};
	vec2 uv2_scaled = {uv2.x * texture->width, uv2.y * texture->height};
	vec2 uv3_scaled = {uv3.x * texture->width, uv3.y * texture->height};

	vec2 uv0_selection = vec2_scale(uv0_scaled, zoom);
	vec2 uv1_selection = vec2_scale(uv1_scaled, zoom);
	vec2 uv2_selection = vec2_scale(uv2_scaled, zoom);
	vec2 uv3_selection = vec2_scale(uv3_scaled, zoom);

	f32 selection_rec_size = 8.0f;

	//offset these coordinates to draw the squares inside the rectangle
	f32 uv0_x = uv0_selection.x - 0;
	f32 uv0_y = uv0_selection.y - selection_rec_size;

	f32 uv1_x = uv1_selection.x - 0;
	f32 uv1_y = uv1_selection.y + 0;

	f32 uv2_x = uv2_selection.x - selection_rec_size;
	f32 uv2_y = uv2_selection.y + 0;

	f32 uv3_x = uv3_selection.x - selection_rec_size;
	f32 uv3_y = uv3_selection.y - selection_rec_size;

	f32 uv01_x = uv0_x + (uv1_x - uv0_x) * .5f;
	f32 uv01_y = uv0_y + (uv1_y - uv0_y) * .5f;

	f32 uv12_x = uv1_x + (uv2_x - uv1_x) * .5f;
	f32 uv12_y = uv1_y + (uv2_y - uv1_y) * .5f;

	f32 uv23_x = uv2_x + (uv3_x - uv2_x) * .5f;
	f32 uv23_y = uv2_y + (uv3_y - uv2_y) * .5f;

	f32 uv03_x = uv0_x + (uv3_x - uv0_x) * .5f;
	f32 uv03_y = uv0_y + (uv3_y - uv0_y) * .5f;



	//RENDER

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv1_selection,
			2,
			color);

	ui_node_push_line(ui, image_node,
			uv1_selection,
			uv2_selection,
			2,
			color);

	ui_node_push_line(ui, image_node,
			vec2_add(uv2_selection, V2(0, -1)),
			vec2_add(uv3_selection, V2(0, -1)),
			2,
			color);

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv3_selection,
			2,
			color);

}

static b32 
ui_image_selection_frames(
		game_ui *ui,
		ui_image_selection_data selection_data,
		b32 selected,
		u32 *frame_x,
		u32 *frame_y,
		u32 *frame_w,
		u32 *frame_h)
{
#if 0

	ui_node *n = selection_data.n;
	b8 *image_selecting = selection_data.selecting;
	b32 image_hot = *selection_data.hot;
	i32 zoom = selection_data.zoom;
	render_texture *texture= selection_data.texture;

//	f32 zoom = n->zoom;
//	b32 image_hot = image_hot;
	b8 mouse_l_down = (ui->mouse_l_down);
	b32 uvs_interacted = 0;
	b32 interacting = 0;

	ui_push_wh_specified(ui, 0, 0);
	ui_node *image_node = ui_create_node(ui, 0, 0);
	ui_pop_wh(ui);

	i32 image_spacing = 0;//(i32)(8 * zoom);
	f32 scroll_dt_x = n->target_scroll[ui_axis_x];
	f32 scroll_dt_y = n->target_scroll[ui_axis_y];

	i16 img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
	i16 img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;


	f32 frame_x_selection = frame_x * zoom;
	f32 frame_y_selection = frame_x * zoom;
	f32 frame_w_selection = frame_x * zoom;
	f32 frame_w_selection = frame_x * zoom;

	f32 selection_rec_size = 8.0f;

	//offset these coordinates to draw the squares inside the rectangle
	f32 uv0_x = uv0_selection.x - 0;
	f32 uv0_y = uv0_selection.y - selection_rec_size;

	f32 uv1_x = uv1_selection.x - 0;
	f32 uv1_y = uv1_selection.y + 0;

	f32 uv2_x = uv2_selection.x - selection_rec_size;
	f32 uv2_y = uv2_selection.y + 0;

	f32 uv3_x = uv3_selection.x - selection_rec_size;
	f32 uv3_y = uv3_selection.y - selection_rec_size;

	f32 uv01_x = frame_x;
	f32 uv01_y = frame_y + frame_h * .5f;

	f32 uv12_x = frame_x + frame_w * .5f;
	f32 uv12_y = frame_y + frame_h;

	f32 uv23_x = frame_x + frame_w;
	f32 uv23_y = frame_y + frame_h * .5f;

	f32 uv03_x = frame_x + frame_w * .5f;
	f32 uv03_y = frame_y + frame_h;

	//uv movement interaction
	//bottom left
	bool8 inside_uv01 = 0;
	bool8 inside_uv12 = 0;
	bool8 inside_uv23 = 0;
	bool8 inside_uv03 = 0;
	bool8 inside_uv_all = 0;
	if(image_hot && !*image_selecting)
	{

		inside_uv01 = ui_mouse_inside_rec_xywh(ui,
				uv01_x + img_x0,
				uv01_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);

		inside_uv12 = ui_mouse_inside_rec_xywh(ui,
				uv12_x + img_x0,
				uv12_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv23 = ui_mouse_inside_rec_xywh(ui,
				uv23_x + img_x0,
				uv23_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv03 = ui_mouse_inside_rec_xywh(ui,
				uv03_x + img_x0,
				uv03_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);

		inside_uv_all |= ui_mouse_inside_rec(
				ui,
				frame_x + img_x0,
				frame_y + img_y0,
				frame_x + frame_w + img_x0,
				frame_y + frame_h + img_y0
				);
		if(mouse_l_down && 
				(inside_uv01 +
				 inside_uv12 +
				 inside_uv23 +
				 inside_uv03 +
				 inside_uv_all
				))
		{
			uvs_interacted = 1;

			u8 uvsc_i[ ] ={
				inside_uv_all,
				inside_uv01,
				inside_uv12,
				inside_uv23,
				inside_uv03
			};
			u32 uvsc_i_count = ARRAYCOUNT(uvsc_i);

			*image_selecting = 1;
	//		n->uvs_group_selection_index = u;

			for(u32 c = 0; c < uvsc_i_count; c++)
			{
				n->uvs_corner_index = uvsc_i[c] ? c : n->uvs_corner_index;
			}
		}
	}

	//u32 drag_interacting   = ui_element_interacting_flags(ui, ui_id_drag_uvs, ui_interaction_mouse_left_down);
	b8 drag_v0_interacting = inside_uv0 && mouse_l_down;

	//bool8 drag_v1_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv1, ui_interaction_mouse_left_down);
	//bool8 drag_v2_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv2, ui_interaction_mouse_left_down);
	//bool8 drag_v3_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv3, ui_interaction_mouse_left_down);

	//bool8 drag_uv01_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv01, ui_interaction_mouse_left_down);
	//bool8 drag_uv12_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv12, ui_interaction_mouse_left_down);
	//bool8 drag_uv23_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv23, ui_interaction_mouse_left_down);
	//bool8 drag_uv03_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv03, ui_interaction_mouse_left_down);

	//RENDER
	f32 selected_alpha = selected ? 
		255.0f : 180.0f;

	vec4 selection_line_color = {200, 0, 0, selected_alpha};
	vec4 uv_drag_color     = vec4_all(255);


	//;TODO add center squares to select by frame
	//render lines attached to the uv coordinates
#if 1

	vec2 corner_tl = {
		frame_x_selection,
		frame_y_selection
	};
	vec2 corner_tr = {
		frame_x_selection + frame_w_selection,
		frame_y_selection
	};
	vec2 corner_bl = {
		frame_x_selection,
		frame_y_selection + frame_h_selection,
	};
	vec2 corner_br = {
		frame_x_selection + frame_w_selection,
		frame_y_selection + frame_h_selection,
	};
	ui_node_push_line(ui, image_node,
			corner_tl,
			corner_tr,
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			corner_tl,
			corner_bl,
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			vec2_add(corner_tr, V2(0, -1)),
			vec2_add(corner_br, V2(0, -1)),
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			corner_bl,
			corner_br,
			2,
			selection_line_color);
#endif

	//border selections for individual uvs

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv01_x,
			uv01_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv12_x,
			uv12_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv23_x,
			uv23_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv03_x,
			uv03_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);


	enum{
		selection_uv_all,
		selection_uv01,
		selection_uv12,
		selection_uv23,
		selection_uv03,
		selection_uv_new
	};
	//start new selection with the current selected group
	if(image_hot && mouse_l_down && !*image_selecting)
	{
		*image_selecting = 1;
		n->uvs_corner_index = selection_uv_new;
	}

	{
		i32 selection_dx = 0;
		i32 selection_dy = 0;

		vec2 uv0_scaled = {uv0->x * texture->width, uv0->y * texture->height};
		vec2 uv1_scaled = {uv1->x * texture->width, uv1->y * texture->height};
		vec2 uv2_scaled = {uv2->x * texture->width, uv2->y * texture->height};
		vec2 uv3_scaled = {uv3->x * texture->width, uv3->y * texture->height};
		//interacting with any uvs
		if(*image_selecting)
		{
			interacting = 1;
			//drag selection around
			if(n->uvs_corner_index < selection_uv_new)
			{
				bool8 uv_all_i =n->uvs_corner_index == selection_uv_all;
				bool8 uv0_i = uv_all_i || n->uvs_corner_index == selection_uv0;
				bool8 uv1_i = uv_all_i || n->uvs_corner_index == selection_uv1;
				bool8 uv2_i = uv_all_i || n->uvs_corner_index == selection_uv2;
				bool8 uv3_i = uv_all_i || n->uvs_corner_index == selection_uv3;
				bool8 uv01_i =n->uvs_corner_index == selection_uv01;
				bool8 uv12_i =n->uvs_corner_index == selection_uv12;
				bool8 uv23_i =n->uvs_corner_index == selection_uv23;
				bool8 uv03_i =n->uvs_corner_index == selection_uv03;


				vec2 mouse_last = ui->mouse_point_last;
				vec2 mouse_point = ui->mouse_point;
				mouse_last.x /= zoom;
				mouse_last.y /= zoom;
				mouse_point.x /= zoom;
				mouse_point.y /= zoom;
				mouse_last = vec2_round_to_int(mouse_last);
				mouse_point = vec2_round_to_int(mouse_point);

				vec2 mouse_delta = vec2_sub(mouse_point, mouse_last);
				//mouse_delta.x /= zoom;
				//mouse_delta.y /= zoom;
				//						mouse_delta = vec2_round_to_int(mouse_delta);

				if(uv0_i || uv01_i || uv03_i)
				{
					uv0_scaled.x += mouse_delta.x;
					uv0_scaled.y += mouse_delta.y;
				}
				if(uv1_i || uv01_i || uv12_i)
				{
					uv1_scaled.x += mouse_delta.x;
					uv1_scaled.y += mouse_delta.y;
				}
				if(uv2_i || uv12_i || uv23_i)
				{
					uv2_scaled.x += mouse_delta.x;
					uv2_scaled.y += mouse_delta.y;
				}
				if(uv3_i || uv23_i || uv03_i)
				{
					uv3_scaled.x += mouse_delta.x;
					uv3_scaled.y += mouse_delta.y;
				}

				//						uv0_scaled = vec2_round_to_int(uv0_scaled);
				//						uv1_scaled = vec2_round_to_int(uv1_scaled);
				//						uv2_scaled = vec2_round_to_int(uv2_scaled);
				//						uv3_scaled = vec2_round_to_int(uv3_scaled);

				uv0->x = uv0_scaled.x /= texture->width;
				uv0->y = uv0_scaled.y /= texture->height;

				uv1->x = uv1_scaled.x /= texture->width;
				uv1->y = uv1_scaled.y /= texture->height;

				uv2->x = uv2_scaled.x /= texture->width;
				uv2->y = uv2_scaled.y /= texture->height;

				uv3->x = uv3_scaled.x /= texture->width;
				uv3->y = uv3_scaled.y /= texture->height;
			}
			else// new selection
			{
				vec2 mouse_image_hold = {
					ui->mouse_point_hold.x - (n->region.x0 + image_spacing - scroll_dt_x) ,
					ui->mouse_point_hold.y - (n->region.y0 + image_spacing - scroll_dt_y) 
				};
				vec2 image_cursor = {
					ui->mouse_point.x - img_x0,
					ui->mouse_point.y - img_y0
				};
				vec2 an_dt = {
					(f32)(i32)((image_cursor.x - mouse_image_hold.x) / zoom),
					(f32)(i32)((image_cursor.y - mouse_image_hold.y) / zoom)
				};


				mouse_image_hold.x /= zoom;
				mouse_image_hold.y /= zoom;
				mouse_image_hold = vec2_round_to_int(mouse_image_hold);

				i32 dx = (i32)an_dt.x;
				i32 dy = (i32)an_dt.y;
				i32 selected_x0 = (i32)mouse_image_hold.x;
				i32 selected_y0 = (i32)mouse_image_hold.y;
				i32 selected_x1 = (i32)mouse_image_hold.x;
				i32 selected_y1 = (i32)mouse_image_hold.y;

				selected_x1 += 1;
				selected_y1 += 1;
				//for displaying selection data
				selection_dx = ABS(dx) + 1;
				selection_dy = ABS(dy) + 1;
				if(dx >= 0)
				{
					selected_x1 += dx;
				}
				else
				{
					selected_x0 += dx;
				}

				if(dy >= 0)
				{
					selected_y1 += dy;
				}
				else
				{
					selected_y0 += dy;
				}

				uv0_scaled = vec2_round_to_int(uv0_scaled);
				uv1_scaled = vec2_round_to_int(uv1_scaled);
				uv2_scaled = vec2_round_to_int(uv2_scaled);
				uv3_scaled = vec2_round_to_int(uv3_scaled);

				uv0->x = (f32)selected_x0 / texture->width;
				uv0->y = (f32)selected_y1 / texture->height;

				uv1->x = (f32)selected_x0 / texture->width;
				uv1->y = (f32)selected_y0 / texture->height;

				uv2->x = (f32)selected_x1 / texture->width;
				uv2->y = (f32)selected_y0 / texture->height;

				uv3->x = (f32)selected_x1 / texture->width;
				uv3->y = (f32)selected_y1 / texture->height;

				//uv0_scaled.x = mouse_delta_node.x;
				//uv0_scaled.y = mouse_delta_node.y;

				//uv0->x = uv0_scaled.x /= texture->width;
				//uv0->y = uv0_scaled.y /= texture->height;

			}

		}
	}
	return(interacting);
#endif
	return(0);
}

static b32 
ui_image_selection_uvs(
		game_ui *ui,
		ui_image_selection_data selection_data,
		b32 selected,
		vec2 *uv0,
		vec2 *uv1,
		vec2 *uv2,
		vec2 *uv3)
{

	ui_node *n = selection_data.n;
	b8 *image_selecting = selection_data.selecting;
	b32 image_hot = *selection_data.hot;
	i32 zoom = selection_data.zoom;
	render_texture *texture= selection_data.texture;

//	f32 zoom = n->zoom;
//	b32 image_hot = image_hot;
	b8 mouse_l_down = (ui->mouse_l_down);
	b32 uvs_interacted = 0;
	b32 interacting = 0;

	ui_push_wh_specified(ui, 0, 0);
	ui_node *image_node = ui_create_node(ui, 0, 0);
	ui_pop_wh(ui);

	i32 image_spacing = 0;//(i32)(8 * zoom);
	f32 scroll_dt_x = n->target_scroll[ui_axis_x];
	f32 scroll_dt_y = n->target_scroll[ui_axis_y];

	i16 img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
	i16 img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
	i16 img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;

	vec2 uv0_scaled = {uv0->x * texture->width, uv0->y * texture->height};
	vec2 uv1_scaled = {uv1->x * texture->width, uv1->y * texture->height};
	vec2 uv2_scaled = {uv2->x * texture->width, uv2->y * texture->height};
	vec2 uv3_scaled = {uv3->x * texture->width, uv3->y * texture->height};

	vec2 uv0_selection = vec2_scale(uv0_scaled, zoom);
	vec2 uv1_selection = vec2_scale(uv1_scaled, zoom);
	vec2 uv2_selection = vec2_scale(uv2_scaled, zoom);
	vec2 uv3_selection = vec2_scale(uv3_scaled, zoom);

	f32 selection_rec_size = 8.0f;

	//offset these coordinates to draw the squares inside the rectangle
	f32 uv0_x = uv0_selection.x - 0;
	f32 uv0_y = uv0_selection.y - selection_rec_size;

	f32 uv1_x = uv1_selection.x - 0;
	f32 uv1_y = uv1_selection.y + 0;

	f32 uv2_x = uv2_selection.x - selection_rec_size;
	f32 uv2_y = uv2_selection.y + 0;

	f32 uv3_x = uv3_selection.x - selection_rec_size;
	f32 uv3_y = uv3_selection.y - selection_rec_size;

	f32 uv01_x = uv0_x + (uv1_x - uv0_x) * .5f;
	f32 uv01_y = uv0_y + (uv1_y - uv0_y) * .5f;

	f32 uv12_x = uv1_x + (uv2_x - uv1_x) * .5f;
	f32 uv12_y = uv1_y + (uv2_y - uv1_y) * .5f;

	f32 uv23_x = uv2_x + (uv3_x - uv2_x) * .5f;
	f32 uv23_y = uv2_y + (uv3_y - uv2_y) * .5f;

	f32 uv03_x = uv0_x + (uv3_x - uv0_x) * .5f;
	f32 uv03_y = uv0_y + (uv3_y - uv0_y) * .5f;

	//uv movement interaction
	//bottom left
	bool8 inside_uv0 = 0;
	bool8 inside_uv1 = 0;
	bool8 inside_uv2 = 0;
	bool8 inside_uv3 = 0;
	bool8 inside_uv01 = 0;
	bool8 inside_uv12 = 0;
	bool8 inside_uv23 = 0;
	bool8 inside_uv03 = 0;
	bool8 inside_uv_all = 0;
	if(image_hot && !*image_selecting)
	{
		inside_uv0 = ui_mouse_inside_rec_xywh(ui,
				uv0_x + img_x0  ,
				uv0_y + img_y0  ,
				selection_rec_size,
				selection_rec_size
				);

		inside_uv1 = ui_mouse_inside_rec_xywh(ui,
				uv1_x + img_x0,
				uv1_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv2 = ui_mouse_inside_rec_xywh(ui,
				uv2_x + img_x0,
				uv2_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv3 = ui_mouse_inside_rec_xywh(ui,
				uv3_x + img_x0,
				uv3_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);

		inside_uv01 = ui_mouse_inside_rec_xywh(ui,
				uv01_x + img_x0,
				uv01_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);

		inside_uv12 = ui_mouse_inside_rec_xywh(ui,
				uv12_x + img_x0,
				uv12_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv23 = ui_mouse_inside_rec_xywh(ui,
				uv23_x + img_x0,
				uv23_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);


		inside_uv03 = ui_mouse_inside_rec_xywh(ui,
				uv03_x + img_x0,
				uv03_y + img_y0,
				selection_rec_size,
				selection_rec_size
				);

		vec2 v0_r = {
			uv0_x + img_x0, uv0_y + img_y0};

		vec2 v1_r = {
			uv1_x + img_x0, uv1_y + img_y0};

		vec2 v2_r = {
			uv2_x + img_x0, uv2_y + img_y0};

		vec2 v3_r = {
			uv3_x + img_x0, uv3_y + img_y0};
		inside_uv_all |= ui_mouse_inside_baycentric(
				ui,
				v0_r,
				v1_r,
				v2_r);
		inside_uv_all |= ui_mouse_inside_baycentric(
				ui,
				v0_r,
				v2_r,
				v3_r);
		if(mouse_l_down && 
				(inside_uv0 +
				 inside_uv1 +
				 inside_uv2 +
				 inside_uv3 +
				 inside_uv01 +
				 inside_uv12 +
				 inside_uv23 +
				 inside_uv03 +
				 inside_uv_all
				))
		{
			uvs_interacted = 1;

			u8 uvsc_i[ ] ={
				inside_uv_all,
				inside_uv0,
				inside_uv1,
				inside_uv2,
				inside_uv3,
				inside_uv01,
				inside_uv12,
				inside_uv23,
				inside_uv03
			};
			u32 uvsc_i_count = ARRAYCOUNT(uvsc_i);

			*image_selecting = 1;
	//		n->uvs_group_selection_index = u;

			for(u32 c = 0; c < 9; c++)
			{
				n->uvs_corner_index = uvsc_i[c] ? c : n->uvs_corner_index;
			}
		}
	}

	//u32 drag_interacting   = ui_element_interacting_flags(ui, ui_id_drag_uvs, ui_interaction_mouse_left_down);
	b8 drag_v0_interacting = inside_uv0 && mouse_l_down;

	//bool8 drag_v1_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv1, ui_interaction_mouse_left_down);
	//bool8 drag_v2_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv2, ui_interaction_mouse_left_down);
	//bool8 drag_v3_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv3, ui_interaction_mouse_left_down);

	//bool8 drag_uv01_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv01, ui_interaction_mouse_left_down);
	//bool8 drag_uv12_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv12, ui_interaction_mouse_left_down);
	//bool8 drag_uv23_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv23, ui_interaction_mouse_left_down);
	//bool8 drag_uv03_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv03, ui_interaction_mouse_left_down);

	//RENDER
	f32 selected_alpha = selected ? 
		255.0f : 180.0f;

	vec4 selection_line_color = {200, 0, 0, selected_alpha};
	vec4 uv_drag_color     = vec4_all(255);


	//;TODO add center squares to select by frame
	//render lines attached to the uv coordinates
#if 1

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv1_selection,
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			uv1_selection,
			uv2_selection,
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			vec2_add(uv2_selection, V2(0, -1)),
			vec2_add(uv3_selection, V2(0, -1)),
			2,
			selection_line_color);

	ui_node_push_line(ui, image_node,
			uv0_selection,
			uv3_selection,
			2,
			selection_line_color);
#endif

	//border selections for individual uvs
	ui_node_push_hollow_rectangle_wh(
			ui,
			image_node,
			uv0_x,
			uv0_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv1_x,
			uv1_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);
	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv2_x,
			uv2_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv3_x,
			uv3_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);
	//side selections

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv01_x,
			uv01_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv12_x,
			uv12_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv23_x,
			uv23_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);

	ui_node_push_hollow_rectangle_wh(ui, image_node,
			uv03_x,
			uv03_y,
			selection_rec_size,
			selection_rec_size,
			2,
			uv_drag_color);


	enum{
		selection_uv_all,
		selection_uv0,
		selection_uv1,
		selection_uv2,
		selection_uv3,
		selection_uv01,
		selection_uv12,
		selection_uv23,
		selection_uv03,
		selection_uv_new
	};
	//start new selection with the current selected group
	if(image_hot && mouse_l_down && !*image_selecting)
	{
		*image_selecting = 1;
		n->uvs_corner_index = selection_uv_new;
	}

	{
		i32 selection_dx = 0;
		i32 selection_dy = 0;

		vec2 uv0_scaled = {uv0->x * texture->width, uv0->y * texture->height};
		vec2 uv1_scaled = {uv1->x * texture->width, uv1->y * texture->height};
		vec2 uv2_scaled = {uv2->x * texture->width, uv2->y * texture->height};
		vec2 uv3_scaled = {uv3->x * texture->width, uv3->y * texture->height};
		//interacting with any uvs
		if(*image_selecting)
		{
			interacting = 1;
			//drag selection around
			if(n->uvs_corner_index < selection_uv_new)
			{
				bool8 uv_all_i =n->uvs_corner_index == selection_uv_all;
				bool8 uv0_i = uv_all_i || n->uvs_corner_index == selection_uv0;
				bool8 uv1_i = uv_all_i || n->uvs_corner_index == selection_uv1;
				bool8 uv2_i = uv_all_i || n->uvs_corner_index == selection_uv2;
				bool8 uv3_i = uv_all_i || n->uvs_corner_index == selection_uv3;
				bool8 uv01_i =n->uvs_corner_index == selection_uv01;
				bool8 uv12_i =n->uvs_corner_index == selection_uv12;
				bool8 uv23_i =n->uvs_corner_index == selection_uv23;
				bool8 uv03_i =n->uvs_corner_index == selection_uv03;


				vec2 mouse_last = ui->mouse_point_last;
				vec2 mouse_point = ui->mouse_point;
				mouse_last.x /= zoom;
				mouse_last.y /= zoom;
				mouse_point.x /= zoom;
				mouse_point.y /= zoom;
				mouse_last = vec2_round_to_int(mouse_last);
				mouse_point = vec2_round_to_int(mouse_point);

				vec2 mouse_delta = vec2_sub(mouse_point, mouse_last);
				//mouse_delta.x /= zoom;
				//mouse_delta.y /= zoom;
				//						mouse_delta = vec2_round_to_int(mouse_delta);

				if(uv0_i || uv01_i || uv03_i)
				{
					uv0_scaled.x += mouse_delta.x;
					uv0_scaled.y += mouse_delta.y;
				}
				if(uv1_i || uv01_i || uv12_i)
				{
					uv1_scaled.x += mouse_delta.x;
					uv1_scaled.y += mouse_delta.y;
				}
				if(uv2_i || uv12_i || uv23_i)
				{
					uv2_scaled.x += mouse_delta.x;
					uv2_scaled.y += mouse_delta.y;
				}
				if(uv3_i || uv23_i || uv03_i)
				{
					uv3_scaled.x += mouse_delta.x;
					uv3_scaled.y += mouse_delta.y;
				}

				//						uv0_scaled = vec2_round_to_int(uv0_scaled);
				//						uv1_scaled = vec2_round_to_int(uv1_scaled);
				//						uv2_scaled = vec2_round_to_int(uv2_scaled);
				//						uv3_scaled = vec2_round_to_int(uv3_scaled);

				uv0->x = uv0_scaled.x /= texture->width;
				uv0->y = uv0_scaled.y /= texture->height;

				uv1->x = uv1_scaled.x /= texture->width;
				uv1->y = uv1_scaled.y /= texture->height;

				uv2->x = uv2_scaled.x /= texture->width;
				uv2->y = uv2_scaled.y /= texture->height;

				uv3->x = uv3_scaled.x /= texture->width;
				uv3->y = uv3_scaled.y /= texture->height;
			}
			else// new selection
			{
				vec2 mouse_image_hold = {
					ui->mouse_point_hold.x - (n->region.x0 + image_spacing - scroll_dt_x) ,
					ui->mouse_point_hold.y - (n->region.y0 + image_spacing - scroll_dt_y) 
				};
				vec2 image_cursor = {
					ui->mouse_point.x - img_x0,
					ui->mouse_point.y - img_y0
				};
				vec2 an_dt = {
					(f32)(i32)((image_cursor.x - mouse_image_hold.x) / zoom),
					(f32)(i32)((image_cursor.y - mouse_image_hold.y) / zoom)
				};


				mouse_image_hold.x /= zoom;
				mouse_image_hold.y /= zoom;
				mouse_image_hold = vec2_round_to_int(mouse_image_hold);

				i32 dx = (i32)an_dt.x;
				i32 dy = (i32)an_dt.y;
				i32 selected_x0 = (i32)mouse_image_hold.x;
				i32 selected_y0 = (i32)mouse_image_hold.y;
				i32 selected_x1 = (i32)mouse_image_hold.x;
				i32 selected_y1 = (i32)mouse_image_hold.y;

				selected_x1 += 1;
				selected_y1 += 1;
				//for displaying selection data
				selection_dx = ABS(dx) + 1;
				selection_dy = ABS(dy) + 1;
				if(dx >= 0)
				{
					selected_x1 += dx;
				}
				else
				{
					selected_x0 += dx;
				}

				if(dy >= 0)
				{
					selected_y1 += dy;
				}
				else
				{
					selected_y0 += dy;
				}

				uv0_scaled = vec2_round_to_int(uv0_scaled);
				uv1_scaled = vec2_round_to_int(uv1_scaled);
				uv2_scaled = vec2_round_to_int(uv2_scaled);
				uv3_scaled = vec2_round_to_int(uv3_scaled);

				uv0->x = (f32)selected_x0 / texture->width;
				uv0->y = (f32)selected_y1 / texture->height;

				uv1->x = (f32)selected_x0 / texture->width;
				uv1->y = (f32)selected_y0 / texture->height;

				uv2->x = (f32)selected_x1 / texture->width;
				uv2->y = (f32)selected_y0 / texture->height;

				uv3->x = (f32)selected_x1 / texture->width;
				uv3->y = (f32)selected_y1 / texture->height;

				//uv0_scaled.x = mouse_delta_node.x;
				//uv0_scaled.y = mouse_delta_node.y;

				//uv0->x = uv0_scaled.x /= texture->width;
				//uv0->y = uv0_scaled.y /= texture->height;

			}

		}
	}
	return(interacting);
}

static void
ui_image_selection_grid(
		game_ui *ui,
		ui_image_selection_data selection_data,
		i32 grid_w,
		i32 grid_h)
{
	render_texture *texture = selection_data.texture;
	ui_node *image_node = ui_create_node(ui, node_floats_x | node_floats_y , 0);
	i32 zoom = selection_data.zoom;
	if(zoom <= 1)
		return;

	grid_w = grid_w < 8 ? 8 : grid_w;
	grid_h = grid_h < 8 ? 8 : grid_h;
	//display first line
	vec4 grid_color = {0, 0, 255, 255};

	i32 grid_x = 0;
	while(grid_x < texture->width * zoom)
	{
		ui_node_push_rectangle(
				ui,
				image_node,
				grid_x,
				0,
				grid_x + 1,
				texture->height * zoom,
				grid_color);
		//next lines multiplies size by two to render cover both padding sizes
		grid_x += (grid_w) * zoom;
	}

#if 1
	i32 grid_y = 0;
	while(grid_y < texture->height * zoom)
	{
		ui_node_push_rectangle(
				ui,
				image_node,
				0,
				grid_y,
				texture->width * zoom, 
				grid_y + 1,	
				grid_color);
		grid_y += grid_h * zoom; 
	}
#endif
}

static inline ui_image_selection_data
ui_image_selection_begin(
		game_ui *ui,
		render_texture *texture,
		b8 *image_selecting,
		b8 *image_selection_hot,
		f32 *image_selection_zoom,
		u8 *label)
{

	*image_selection_hot = 0;
	f32 zoom = *image_selection_zoom;
	ui_node *selection_region_node;
	b8 mouse_l_down = (ui->mouse_l_down);
	vec2 image_cursor = {0};

	ui_node *n = 0;
	n = ui_create_node(
			ui,
			node_background |
			node_border |
			node_scroll_x |
			node_scroll_y |
			node_clip,
			label);
	n->selection_region_hot = 0;
	n->selection_region_down = 0;
	ui_node *image_node = 0;

	bool32 flip_uvs_h = 0;
	bool32 new_selection = 0;

	f32 scroll_dt_x = 0;
	f32 scroll_dt_y = 0;
	ui_push_id_string(ui, label);
	ui_set_parent(ui, n)
	{
		//top bar
		//ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
		//	ui_set_w_ppct(ui, 1.0f, 1.0f)
		//	ui_set_color(ui, ui_color_background, V4(0x2c, 0x2c, 0x30, 0xff))
		//	{
		//		top_bar_node = ui_create_node(
		//				ui,
		//				node_background,
		//				0);
		//	}
		//contents like spacing, image... and where scroll occurs

		scroll_dt_x = n->target_scroll[ui_axis_x];
		scroll_dt_y = n->target_scroll[ui_axis_y];

		vec2 uv0 = {0.0f, 1.0f};
		vec2 uv1 = {0.0f, 0.0f};
		vec2 uv2 = {1.0f, 0.0f};
		vec2 uv3 = {1.0f, 1.0f};
		//n->zoom = 2.0f;
		bool32 image_hot = 0;


		if(zoom == 0)
		{
			zoom = 1;
		}

		ui_set_parent(ui, n)
		{
			//			ui_set_interaction_layer(ui, ui_interaction_layer_mid)
			ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 1.0f))
			{
				selection_region_node = ui_create_node(
						ui,
						node_clickeable |
						node_skip_layout_x |
						node_skip_layout_y,
						"S_IMAGE_REGION");
			}

			//interact and modify zoom or scroll before interacting with the rest.
			ui_usri region_interaction = ui_interaction_from_node(ui, selection_region_node);

			f32 old_zoom = zoom;
			if(ui_usri_mouse_hover(region_interaction))
			{
				image_hot = 1;
				(*image_selection_hot) = 1;
				n->selection_region_hot = 1;
				zoom += ui->input.mouse_wheel;
				zoom = zoom < 1 ? 1 : zoom > 12 ? 12 : zoom;
				zoom = (i16)zoom;

				if(ui->input.mouse_wheel)
				{
					f32 cursor_image_delta_x = ((ui->mouse_point.x) - (n->region.x0 - scroll_dt_x));
					f32 cursor_image_delta_y = ((ui->mouse_point.y) - (n->region.y0 - scroll_dt_y)); 
					f32 scaled_zoom = zoom / old_zoom;
					scroll_dt_x = (f32)(i32)(1 * ((cursor_image_delta_x * scaled_zoom) - (ui->mouse_point.x - n->region.x0)));
					scroll_dt_y = (f32)(i32)(1 * ((cursor_image_delta_y * scaled_zoom) - (ui->mouse_point.y - n->region.y0)));
					n->selection_region_down = 1;
				}
				if(ui_usri_mouse_m_down(region_interaction))
				{
					vec2 mouse_delta = ui_mouse_delta(ui);
					scroll_dt_x -= mouse_delta.x;
					scroll_dt_y -= mouse_delta.y;
					scroll_dt_x = (f32)(i32)scroll_dt_x;
					scroll_dt_y = (f32)(i32)scroll_dt_y;
					scroll_dt_x = scroll_dt_x < 0 ? 0 : scroll_dt_x;
					scroll_dt_y = scroll_dt_y < 0 ? 0 : scroll_dt_y;
					n->selection_region_down = 1;
				}
			}
			i32 image_spacing = 0;//8 * (i32)zoom;


			ui_space_specified(ui, image_spacing, 1.0f);
			//draws image/background/selections

			i16 img_x0 = 0;
			i16 img_y0 = 0;
			i16 img_x1 = 0;
			i16 img_y1 = 0;

			ui_set_row(ui)
			{
				ui_space_specified(ui, image_spacing, 1.0f);
				ui_set_width(ui, ui_size_specified(texture->width * zoom, 1.0f))
					ui_set_height(ui, ui_size_specified(texture->height * zoom, 1.0f))
					{
						image_node = ui_create_node(ui, node_border, "S_IMAGE_DISPLAY");
					}

				//push background
				f32 bk_size = 16.0f;
				u32 bk_index = 0;
				u32 bk_index_start = 0;
				vec4 bk_colors[2] = {
					{050, 050, 050, 255},
					{100, 100, 100, 255},
				};


				f32 x = (f32)((i32)((scroll_dt_x - image_spacing) / (bk_size * zoom)) * (bk_size * zoom));
				f32 y = (f32)((i32)((scroll_dt_y - image_spacing) / (bk_size * zoom)) * (bk_size * zoom));;
				x = x < 0 ? 0 : x;
				y = y < 0 ? 0 : y;
				f32 x_start = x;
				f32 x_end = texture->width * (f32)zoom;
				f32 y_end = texture->height * (f32)zoom;
				bk_index = (i32)(y / (bk_size * zoom)) % 2 + 
					(i32)(x / (bk_size * zoom)) % 2;
				while(y < y_end)
				{
					bk_index_start = bk_index;
					while(x < x_end)
					{
						bk_index %= 2;
						ui_node_push_rectangle_wh(
								ui,
								image_node,
								(i16)x,
								(i16)y,
								(i16)(bk_size * zoom),
								(i16)(bk_size * zoom),
								bk_colors[bk_index]);

						x += bk_size * zoom;
						bk_index++;
					}
					bk_index %= 2;
					bk_index += (bk_index_start == bk_index);
					bk_index %= 2;
					x = x_start;
					y += bk_size * zoom;
				}

				//render with updated information
				//this is the updated region for the node on the next frame
				img_x0 = (n->region.x0 + image_spacing) - (i16)scroll_dt_x;
				img_y0 = (n->region.y0 + image_spacing) - (i16)scroll_dt_y;
				img_x1 = (n->region.x1 + image_spacing) - (i16)scroll_dt_x;
				img_y1 = (n->region.y1 + image_spacing) - (i16)scroll_dt_y;

				//push image
				ui_node_push_image(
						ui,
						image_node,
						texture,
						0,
						0,
						uv0,
						uv1,
						uv2,
						uv3);
				ui_space_specified(ui, 8.0f * zoom, 1.0f);
			}
			ui_space_specified(ui, image_spacing, 1.0f);

			//uv selections
			bool32 uvs_interacted = 0;
			u32 uvs_index = 0;
			n->uvs_group_down = n->uvs_group_down && mouse_l_down && image_hot;
			//enum{
			//	uv0_i = 0x1,
			//	uv1_i = 0x1,
			//	uv2_i = 0x1,
			//	uv3_i = 0x1,
			//	uv0_i = 0x1,
			//	uv0_i = 0x1,
			//	uv0_i = 0x1,
			//}uv_interacted = 0;
			//draw cursor inside image
			image_cursor.x = ui->mouse_point.x - img_x0;
			image_cursor.y = ui->mouse_point.y - img_y0;
			image_cursor.x = (f32)((i32)(image_cursor.x / zoom) * zoom);
			image_cursor.y = (f32)((i32)(image_cursor.y / zoom) * zoom);
			ui_node_push_hollow_rectangle(
					ui,
					image_node,
					(i16)image_cursor.x,
					(i16)image_cursor.y,
					(i16)(1 * zoom),
					(i16)(1 * zoom),
					1,
					V4(255, 255, 0, 255));


		}
		n->target_scroll[ui_axis_x] = scroll_dt_x;
		n->target_scroll[ui_axis_y] = scroll_dt_y;

	}
	*image_selection_zoom = zoom;

//	n->uvs_group_down = n->uvs_group_down && mouse_l_down && image_hot;
	*image_selecting = *image_selecting && mouse_l_down && *image_selection_hot;

	ui_push_parent(ui, image_node);

	ui_image_selection_data result = {0};
	result.padding_x = 0;
	result.padding_y = 0;
	result.selecting = image_selecting;
	result.hot = image_selection_hot;
	result.zoom = (i32)(*image_selection_zoom);
	result.image_cursor_x = (i16)(image_cursor.x / zoom);
	result.image_cursor_y = (i16)(image_cursor.y / zoom);
	result.n = n;
	result.texture = texture;

	return(result);
}

static inline void
ui_image_selection_end(
		game_ui *ui)
{
	ui_pop_last_parent(ui);
	ui_pop_id(ui);
}

static ui_image_selection_data 
ui_image_selection_uvs_be(
		game_ui *ui,
		render_texture *texture,
		f32 *image_selection_zoom,
		b8 *image_selecting,
		b8 *image_selection_hot,
		vec2 *uv0,
		vec2 *uv1,
		vec2 *uv2,
		vec2 *uv3,
		u8 *label)
{
	ui_image_selection_data selection_data = ui_image_selection_begin(
			ui,
			texture,
			image_selecting,
			image_selection_hot,
			image_selection_zoom,
			label);
	ui_image_selection_uvs(ui, selection_data, 1, uv0, uv1, uv2, uv3);
	ui_image_selection_end(ui);

	return(selection_data);
}

static ui_node * 
ui_box_with_scroll(
		game_ui *ui, u8 *label)
{
	ui_push_id_string(ui, label);
	ui_node *frame_list_node_label = ui_create_node(ui, node_border | node_background, 0);
	ui_node *list_nodes = 0;
	//sprite sheet listui_content_box_ex(ui, node_scroll_y, "model_sprite_sheets_list");
	ui_set_parent(ui, frame_list_node_label) ui_set_row(ui) ui_set_w_ppct(ui, 1.0f, 0.0f)
	{
		list_nodes = ui_create_node(ui,
				node_scroll_y | node_clickeable | node_clip,
				"__BOX_CONTENT__"); 
		list_nodes->padding_x = 6;
		list_nodes->padding_y = 6;
		ui_set_w_specified(ui, 14.0f, 1.0f)
		{
			f32 scroll_value = ui_scroll_vertical1(
					ui,
					list_nodes->size_y,
					list_nodes->content_size[1],
					list_nodes->scroll_y,
					"__SCROLL_V_BOX__");
			if(scroll_value != list_nodes->scroll_y)
			{
				ui_node_set_scroll_y(list_nodes, scroll_value);
			}
		}
	}
	ui_scroll_area_vertical(ui, list_nodes);

	ui_pop_id(ui);
	return(list_nodes);
}

static inline ui_node *
ui_node_content(game_ui *ui, ui_node_flags extra_flags, u32 padding, u8 *label)
{
	ui_node *list_nodes = ui_create_node(ui,
			extra_flags | node_clickeable | node_clip,
			label); 
	list_nodes->padding_x = padding;
	list_nodes->padding_y = padding;

	return(list_nodes);
}

typedef struct{
	u32 *selected_line_index;
	u32 line_count;
	u32 lines_height;
}ui_line_list_data;
static void
ui_line_list_begin(game_ui *ui, u32 em_h, u8 *label)
{
	i32 padding_x = 2;
	i32 padding_y = 2;

	u32 scroll_width = 10;
	ui_node *lines_panel = ui_create_node(ui, node_background | node_border | node_clickeable | node_scroll_y, label);
	lines_panel->padding_x = padding_x;
	lines_panel->padding_y = padding_y;
	//paint background
	vec4 line_colors[] = {
		V4(0, 0, 0, 255),
		V4(40, 40, 40, 255)};
	vec4 selected_line_color = {80, 80, 80, 255};
	if(!em_h)
	{
		em_h = (u32)(ui_em(ui) * 2.0f);
	}
	if(em_h)
	{
		u32 c = 0;
		for(i32 size_y = 0;
				size_y < lines_panel->size_y - (padding_y * 2);
				size_y += em_h)
		{
			u32 color_index = c % 2;
			c++;
			f32 x = (f32)(padding_x);
			f32 y = (f32)(size_y + padding_y);
			f32 w = (f32)(lines_panel->size_x - (padding_x * 2) - scroll_width);
			f32 h = (f32)(em_h);
			ui_node_push_rectangle_wh(ui,
					lines_panel,
					x,
					y,
					w,
					h,
					line_colors[color_index]);
		}
	}
	ui_push_parent(ui, lines_panel);
	//scroll bar and scroll region
	ui_set_w_ppct(ui, 1.0f, 0.0f)
	{
		ui_node *scroll_node = ui_create_node(ui, node_skip_layout_x | node_skip_layout_y, 0);
		scroll_node->padding_x = padding_x;
		scroll_node->padding_y = padding_y;
		scroll_node->layout_axis = 0;
		ui_set_parent(ui, scroll_node)
		{
			ui_space_ppct(ui, 1.0f, 0.0f);
			ui_set_w_specified(ui, scroll_width, 1.0f)
			{
				f32 scroll_delta = ui_scroll_vertical(
						ui,
						lines_panel->size_y,
						lines_panel->content_size[ui_axis_y],
						lines_panel->scroll_y);
			}
		}
	}
	ui_push_w_ppct(ui, 1.0f, 0.0f);
	ui_push_h_soch(ui, 1.0f);
}

#define ui_line_list_line(ui, selected, out_usri, label) \
	ui_DEFER_LOOP(ui_line_list_line_begin(ui, selected, out_usri, label), ui_line_list_line_end(ui))
static void 
ui_line_list_line_begin(
		game_ui *ui, b32 selected, ui_usri *out_usri, u8 *label)
{
	vec4 color = {0};
	if(selected)
	{
		vec4 selected_line_color = {80, 80, 80, 255};
		color = selected_line_color;
	}
		ui_push_color(ui, ui_color_background, color);
	ui_node *line_node = ui_create_node(ui, node_background | node_clickeable, label);
		ui_pop_color(ui, ui_color_background);

	if(out_usri)
	{
		*out_usri = ui_usri_from_node(ui, line_node);
	}

	ui_push_parent(ui, line_node);
}

static void 
ui_line_list_line_beginf(
		game_ui *ui, b32 selected, ui_usri *out_usri, u8 *label, ...)
{
	u8 text_buffer[256] = {0};
	ui_ANYF(text_buffer, sizeof(text_buffer), label);
	ui_line_list_line_begin(
			ui, selected, out_usri, text_buffer);
}

static inline void
ui_line_list_line_end(
		game_ui *ui)
{
	ui_pop_last_parent(ui);
}

static void
ui_line_list_end(game_ui *ui)
{
	ui_pop_last_parent(ui);
}

static b32
ui_slider_f32(
		game_ui *ui,
		f32 min_value,
		f32 max_value,
		f32 *value,
		u8 *label)
{
	b32 modified = 0;
	f32 prev_value = 0;
	ui_push_id_string(ui, label);
	ui_set_h_specified(ui, 12.0f, 1.0f)
	{
		ui_node *back = ui_create_node(ui, node_background | node_border | node_clickeable, label);

		back->padding_y = 2;
		back->padding_x = 2;
		ui_set_parent(ui, back)
		{
			if(back->size_x && max_value && value) ui_set_row(ui)
			{
				prev_value = *value;
				ui_node *button_node = 0;
				f32 button_w = 12.0f;
				f32 width = max_value - min_value;
				f32 scale =  (((f32)back->size_x - button_w - back->padding_x * 2) / width);
				f32 button_x = (*value - min_value) * scale;
//				*value = 100.0f;

				ui_space_specified(ui, button_x, 1.0f);
				ui_push_h_ppct(ui, 1.0f, 0.0f);
				ui_push_w_specified(ui, button_w, 0.0f);
				ui_push_color(ui, ui_color_background, V4(255, 0, 0, 255));
				{
					button_node = ui_create_node(ui, node_background, "SLIDER_BUTTON");
				}
				ui_pop_color(ui, ui_color_background);
				ui_pop_width(ui);
				ui_pop_height(ui);

				if(ui_node_mouse_l_down(ui, back))
				{
					f32 mouse_x = ui_mouse_delta_from_node(ui, back).x;
					*value = min_value + (mouse_x / scale);

				}
				*value = *value > max_value ? max_value : *value;
				*value = *value < min_value ? min_value : *value;
			}
			modified = *value != prev_value;
		}

	}
	ui_pop_id(ui);
	return(modified);
}
static b32
ui_input_drag_f32(game_ui *ui,
		f32 inc_dec,
		f32 min,
		f32 max,
		f32 *value,
		b32 confirm_on_enter,
		u8 *label)
{
	b32 changed = 0;
	ui_push_id_string(ui, label); ui_set_row(ui)
	{
		if(value)
		{
			f32 value_before = *value;
			ui_input_f32(ui, confirm_on_enter, value, "__IDRAG_IT__");
			//drag button
			ui_set_w_specified(ui, 12.0f, 1.0f)
			{
				ui_node *drag_button = ui_create_node(ui, node_clickeable | node_border | node_text | node_text_centered, "::#__ITDRAG_VALUE__");
				ui_usri drag_usri = ui_usri_from_node(ui, drag_button);
				if(value && ui_usri_mouse_l_down(drag_usri))
				{
					i32 wheel = (i32)ui_mouse_delta(ui).x;
					ui_inc_dec_f32(value, wheel * inc_dec, min, max);
					//return true if the value changed
				}
			}
			changed = *value != value_before;
		}
	}
	ui_pop_id(ui);
	return(changed);
}

static b32
ui_input_drag_i32(game_ui *ui,
		i32 inc_dec,
		i32 min,
		i32 max,
		i32 *value,
		b32 confirm_on_enter,
		u8 *label)
{
	b32 changed = 0;
	ui_push_id_string(ui, label); ui_set_row(ui)
	{
		ui_input_i32(ui, confirm_on_enter, value, "__IDRAG_IT__");
		//drag button
		ui_set_w_specified(ui, 12.0f, 1.0f)
		{
			ui_node *drag_button = ui_create_node(ui, node_clickeable | node_border | node_text | node_text_centered, "::#__ITDRAG_VALUE__");
			ui_usri drag_usri = ui_usri_from_node(ui, drag_button);
			if(value && ui_usri_mouse_l_down(drag_usri))
			{
				i32 wheel = (i32)ui_mouse_delta(ui).x;
				i32 value_before = *value;
				ui_inc_dec_i32(value, wheel * inc_dec, min, max);
				//return true if the value changed
				changed = *value != value_before;
			}
		}
	}
	ui_pop_id(ui);
	return(changed);
}


static ui_node *
ui_popup_box(game_ui *ui, ui_id popup_id, u8 *label)
{
	ui_node *top_bar = 0;
	ui_set_h_em(ui, 2.0f, 1.0f)
		top_bar = ui_label(ui, 0);
	top_bar->padding_x = 2;
	top_bar->padding_y = 2;
	ui_set_parent(ui, top_bar) ui_set_wh_text(ui, 4.0f, 0.0f)
	{
		ui_push_id(ui, popup_id);
		if(ui_button(ui, "x#popup_box"))
		{
			ui_popup_close(ui, popup_id);
		}
	}


	ui_node *box = ui_node_box(ui, label);
	return(box);
}

static ui_node *
ui_popup_area_begin(game_ui *ui, ui_id popup_id)
{
	ui_push_id(ui, popup_id);
	ui_next_nodes_interaction_only_begin(ui);
	ui_node *area = ui_interact_area(ui, ui_interaction_layer_top);
	if(input_up(ui->input.mouse_left) && !ui_node_mouse_hover(ui, area))
	{
		ui_popup_close(ui, popup_id);
	}


	ui_pop_id(ui);
	ui_push_parent(ui, area);
	return(area);
}
static void
ui_popup_area_end(game_ui *ui)
{
	ui_next_nodes_interaction_only_end(ui);
	ui_pop_last_parent(ui);
}

	static inline ui_node *
ui_drop_down_b(game_ui *ui)
{
	ui_node *arrow_background = ui_create_node(
			ui,
			node_background,
			0);

	ui_node_push_triangle(
			ui,
			arrow_background,
			V2(1, 2),
			V2(3, 2),
			V2(2, 1),
			vec4_all(255));

	return(arrow_background);
}

	static inline ui_node *
ui_drop_down_node(game_ui *ui, u8 *preview, u8 *text)
{
	ui_push_id_string(ui, text);
	ui_node *box = ui_create_node(ui,
			node_clickeable |
			node_active_animation |
			node_hover_animation |
			node_background |
			node_border,
			"DD_BOX");
	ui_set_parent(ui, box)
	{
		ui_set_row(ui)
		{
			ui_space(ui, ui_size_em(ui, 0.2f, 1.0f));
			ui_set_wh(ui, ui_size_text(2.0f, 1.0f))
			{
				ui_text(ui, preview);
			}
			ui_space_ppct(ui, 1.0f, 0.0f);

			ui_set_color(ui, ui_color_background, V4(60, 60, 255, 255))
				ui_set_h_ppct(ui, 1.0f, 1.0f)
				ui_set_w_specified(ui, 40, 0.0f)
				{
					ui_node *arrow_background = ui_create_node(
							ui,
							node_background,
							0);

					ui_node_push_triangle(
							ui,
							arrow_background,
							V2(1, 2),
							V2(3, 2),
							V2(2, 1),
							vec4_all(255));
				}
		}
	}
	ui_pop_id(ui);

	return(box);

}

static inline ui_node *
ui_drop_down_button(
		game_ui *ui,
		u8 *preview,
		u8 *label)
{
	ui_node *drop_down_box = ui_create_nodef(
			ui,
			node_text | node_background | node_border,
			label);
	ui_node_set_display_string(ui, drop_down_box, preview);


	return(drop_down_box);
}


	static inline b32
ui_drop_down_begin(
		game_ui *ui,
		ui_id popup_id,
		u8 *label)
{
	ui_node *drop_down_box = ui_button_node(ui, label);
	b32 is_opened = 0;

	ui_set_wh_soch(ui, 0.0f)
		is_opened = ui_popup_begin(ui, popup_id);

	//by default, this get closed whenever the user clicks outside
	ui_node *region = ui_interact_area(ui, ui_interaction_layer_top);
	if(ui_node_mouse_l_up(ui, drop_down_box))
	{
		ui_popup_open(ui, 
				drop_down_box->region.x0,
				drop_down_box->region.y1,
				popup_id);
	}
	ui_push_parent(ui, region);

	if(is_opened)
	{
		ui_set_wh_soch(ui, 1.0f)
		ui_popup_area_begin(ui, popup_id);
	}

	return(is_opened);
}

#define ui_drop_down(ui, popup_string, preview_string) \
	ui_DEFER_CONDITION(ui_drop_down_begin(ui, popup_string, preview_string), ui_drop_down_end(ui))
#define ui_drop_down_quick(ui, popup_string, preview_string) \
	ui_DEFER_CONDITION(ui_drop_down_begin_quick(ui, popup_string, preview_string), ui_drop_down_end(ui))

//	for(u8 __I__ = (u8)ui_drop_down_begin_quick(ui, popup_string, preview_string);\
//		   __I__ == 1 || (!__I__ ? ui_drop_down_end(ui) : 0)\
//		   __I__ += 1, ui_drop_down_end(ui));
static inline b32
ui_drop_down_begin_quick(
		game_ui *ui,
		u8 *popup_string,
		u8 *preview_string)
{
	//ui_id popup_id = ui_node_id_from_string(ui, popup_string);
	ui_id popup_id = ui_id_from_string(popup_string);
	b32 is_opened = ui_drop_down_begin(ui, popup_id, preview_string);
	return(is_opened);
}

	static inline b32
ui_drop_down_beginf(
		game_ui *ui,
		ui_id popup_id,
		u8 *label,
		...)
{
	u8 tb[256] = {0};
	ui_ANYF(tb, sizeof(tb), label);
	return(ui_drop_down_begin(ui, popup_id, tb));
}

	static inline b32
ui_drop_down_end(
		game_ui *ui)
{
	if(ui->last_looked_popup_was_opened)
	{
		ui_popup_area_end(ui);
	}
	ui_popup_end(ui);
	return(0);
}

	static inline void
ui_drop_down_list_end()
{
}

static ui_node * 
ui_focus_box(game_ui *ui, b8 focused, u8 *label)
{
	//change border color if focused
	b32 has_focus = focused ? focused : 0;
	//this should actually pick the color from the theme
	f32 border_a = has_focus ? 1.0f : 0.4f;
	b32 clicked = 0;
	

	//interaction area
	ui_next_interaction(ui, ui_interaction_layer_top);
	ui_push_color_a(ui, ui_color_border, border_a);
	ui_node *box_area = ui_node_box(ui, label);//ui_interact_area(ui, ui_interaction_layer_top);
	ui_pop_color_a(ui, ui_color_border);
	ui_prev_interaction(ui, ui_interaction_layer_default);
	if(ui_node_mouse_l_pressed(ui, box_area))
	{
//		u8 *f8 = focused;
//		(*f8) = !(*f8);
		clicked = 1;
	}

	//push parent for other nodes
	return(box_area);
}
static void
ui_focus_box_e(game_ui *ui)
{
	ui_pop_last_parent(ui);
}


static u32 
ui_show_test_panel(game_ui *ui)
{
	if(ui_window_begin(ui,
			ui_panel_flags_init_closed,
			310,
			310,
			512,
			512,
			"Widget and options panel"))
	{
	
#if 1


		ui_set_specified_width(ui, size_text, 0, 1.0f)
		ui_set_specified_height(ui, size_text, 0, 1.0f)
		{
			ui_set_color(ui, ui_color_text, V4(0, 255, 0, 255))
			{
				ui_button(ui, "Button!");
				ui_set_color(ui, ui_color_text, V4(255, 0, 255, 255))
				{
					ui_button(ui, "Button!#2");
				}
			}

		}
		ui_set_w_em(ui, 6.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
		{
			static i32 static_i;
			ui_input_drag_i32(ui, 1, -100, 100, &static_i, 0, "drag_test_for_panel_test");
		}

		//row


		ui_push_prefered_height(
				ui,
				size_specified,
				40,
				1);
		ui_push_prefered_width(
				ui,
				size_percent_of_parent,
				1.0f,
				1.0f);

		ui_set_text_color(ui, V4(180, 180, 180, 180))
		{
			//ui_interaction_info text_interaction_info = ui_text(ui, "(?)");
			//if(text_interaction_info.flags & ui_interaction_mouse_hover)
			//{
			//	ui_set_text_color(ui, V4(255, 255, 255, 255))
			//	{
			//		ui_tool_tip_mouse(ui, "Tool tip!");
			//	}
			//}

		}

		ui_node *box_region;
		ui_set_h_ppct(ui, 0.4f, 1.0f)
		box_region = ui_content_box_begin(ui,
				"Box region!");
		{
			ui_set_height(ui, ui_size_text(1.0f, 1.0f))
			{

				ui_extra_flags(ui, node_border)
					ui_set_w_em(ui, 16.0f, 1.0f)
					for(u32 b = 0;
							b < 5;
							b++)
					{
						ui_buttonf(ui, "Child panel button!%u", b);
						ui_space_specified(ui, 4.0f, 1.0f);
					}

				ui_set_row(ui) ui_set_w_em(ui, 16.0f, 1.0f) ui_extra_flags(ui, node_border)
				{
					//ppct means parent percent
					ui_button(ui, "Left!");
					ui_button(ui, "Right!");
				}
			}
		}
		ui_content_box_end(ui, box_region);


		ui_space(
				ui, ui_size_specified(4, 1.0f));

		ui_set_width(ui, ui_size_percent_of_parent(0.4f, 1.0f))
		ui_set_height(ui, ui_size_text(1.0f, 1.0f))
		{
			static u8 editable_text_buffer[256] = {"Text to edit!"};
			static u32 static_value_u32 = 32;
			static f32 static_value_f32 = 32.0f;
			ui_input_text(ui,
					1,
					editable_text_buffer,
					256,
					"Input text label");
			ui_space_specified(ui, 4.0f, 1.0f);
			ui_input_u32(ui,
					1,
					&static_value_u32,
					"Input text label2");
				ui_space_specified(ui, 4.0f, 1.0f);
			ui_input_f32(ui,
					1,
					&static_value_f32,
					"Input text label3");
		}

		ui_pop_width(ui);
		ui_pop_height(ui);

#if 0
		//ui_push_parent(ui, box_region);
		//ui_push_prefered_height(
		//		ui,
		//		size_text,
		//		1,
		//		0);
		//ui_push_prefered_width(
		//		ui,
		//		size_text,
		//		1,
		//		0);
		//ui_button(ui, "Button 1!");
		//ui_button(ui, "Button 2!");
		//ui_pop_width(ui);
		//ui_pop_height(ui);
		//ui_pop_parent(ui);
#endif
#endif
		static u8 cb_value = 1;
		ui_set_width_height(ui, ui_size_text(1.0f, 1.0f))
		{
			ui_checkbox(ui, &cb_value, "Checkbox!");
			ui_radio_button_node(ui, 1, "Radio button");
		}
		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_textf(ui, "Frame: %d", ui->current_frame);
			ui_textf(ui, "Keep interaction count: %d", ui->keep_interaction_countdown);
			ui_textf(ui, "mouse_hold_dt: %f", ui->mouse_hold_dt);
		}

		ui_push_id_string(ui, "For button AA");
		{
			ui_button(ui, "AA");
		}
		ui_pop_id(ui);
		ui_button(ui, "AA");


		ui_node *n_button;
	//	ui_id context_menu_id0 = ui_id_from_string("A context_menu");
		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			n_button = ui_button_node(ui, "Hover me!");
			if(ui_node_mouse_l_up(ui, n_button))
			{
				//ui_context_menu_open_id(ui, context_menu_id);
			}
		}
		if(ui_node_mouse_hover(ui, n_button))
		{
			ui_tool_tip_mouse(ui, "Thanks!");
		}

		static u32 selected_text_index = 0;
		static u8 *text_list[ ] = {
			"Quiero",
			"Alta",
			"Milanga"};

		ui_id test_popup_id = ui_id_from_string("quiero alta milanga");

		ui_set_w_ppct(ui, 0.4f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f);
		if(ui_drop_down_begin(ui, test_popup_id, "DropDown!"))
		{
			ui_node *box = 0;
			ui_set_wh_soch(ui, 1.0f)
				box = ui_node_box(ui, "DropDownBox!");
			
			ui_set_parent(ui, box)
			{
				ui_set_wh_text(ui, 4.0f, 1.0f)
				for(u32 s = 0; s < 3; s++)
				{
					if(ui_selectablef(ui, 0, "%s##DdSelectable%u", text_list[s], s))
					{
						ui_popup_close(ui, test_popup_id);
					}
				}
			}
		}
		ui_drop_down_end(ui);

		ui_id context_menu_id = ui_id_from_string("A context menu but with a twist");
		ui_set_wh(ui, ui_size_sum_of_children(1.0f))
		ui_context_menu(
				ui,
				context_menu_id)
		{
			ui_set_h_text(ui, 4.0f, 1.0f)
			ui_set_w_em(ui, 16.0f, 1.0f)
			for(u32 l = 0; l < 3; l++)
			{
				if(ui_selectable(ui, selected_text_index == l, text_list[l]))
				{
					ui_popup_close(ui, context_menu_id);
					selected_text_index = l;

				}
			}

			ui_node *area = ui_interact_area(ui, ui_interaction_layer_top);
		}

		ui_node *dd_button;
		ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
		ui_set_w_ppct(ui, 1.0f, 1.0f)
		{
			dd_button = ui_drop_down_node(ui, text_list[selected_text_index], "Drop down!");
			if(ui_node_mouse_l_up(ui, dd_button))
			{
				//ui_context_menu_open_string(ui, "A context menu");
			}

			u8 *preview = "-";
			if(selected_text_index < 3)
			{
				preview = text_list[selected_text_index];
			}

			ui_drop_down_button(
					ui,
					preview,
					"A drop down button!");

			if(ui_node_mouse_l_up(ui, dd_button))
			{
				ui_popup_open(
						ui,
						dd_button->region.x0,
						dd_button->region.y1,
						context_menu_id);
			}
		}
		ui_space_specified(ui, 4.0f, 1.0f);
		ui_text(ui, "Colors!");
		u8 *colors[] = {
		"color_text",
		"color_background",
		"color_disabled",
		"color_hot",
		"color_interacting",
		"color_border",
		};
		ui_set_h_text(ui, 4.0f, 1.0f)
		ui_set_w_em(ui, 12.0f, 1.0f)
		{
				for(u32 c = 0;
						c < ui_color_COUNT;
						c++)
				{
					ui_set_row(ui)
					{
						ui_push_id_u32(ui, c);
						ui_spinner_f32(ui,
								1.0f,
								0.0f,
								255.0f,
								&ui->theme.colors[c].r,
								0,
								"r#color");
						ui_spinner_f32(ui,
								1.0f,
								0.0f,
								255.0f,
								&ui->theme.colors[c].g,
								0,
								"g#color");
						ui_spinner_f32(ui,
								1.0f,
								0.0f,
								255.0f,
								&ui->theme.colors[c].b,
								0,
								"b#color");
						ui_spinner_f32(ui,
								1.0f,
								0.0f,
								255.0f,
								&ui->theme.colors[c].a,
								0,
								"a#color");

						ui_set_wh(ui, ui_size_specified(20.0f, 1.0f))
						{
							ui_set_color(ui, ui_color_background, ui->theme.colors[c])
								ui_label(ui, "The color");
						}

						ui_text(ui, colors[c]);
						ui_pop_id(ui);

					}
				}
			ui_space_specified(ui, 4.0f, 1.0f);
		}
		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_text(ui, "Font scale");
			ui_input_f32(ui, 0, &ui->font_scale, "Font scale!");
		}

		static f32 f32_drag_value = 2.0f;
		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_drag_f32(ui, 1.0f, -200.0f, 200.0f, &f32_drag_value, "Drag f32");
		}

		ui_set_w_specified(ui, 126.0f, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
		{
			static f32 test_panel_f = 0;
			ui_slider_f32(ui, 50, 100.0f, &test_panel_f, "test_panel_slider!");
			ui_textf(ui, "Value %f", test_panel_f);
		}


	}
	ui_panel_end(ui);

	//ui_panel_begin(ui,
	//		0,
	//		310,
	//		310,
	//		512,
	//		512,
	//		"normal panel");
	//{
	//}
	//ui_panel_end(ui);
	return(0);
}
#define ui_open_or_close_test_panel(ui) ui_open_or_close_panel(ui, "Widget and options panel")
