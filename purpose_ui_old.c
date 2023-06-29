//TODO(Agu): Make the panel widgets be separate from only BeginPanel_All
//

#define ui_REMAINING_PANEL_WIDTH  (U16MAX - 0)
#define ui_REMAINING_PANEL_HEIGHT (U16MAX - 1)

#define ui_TEXT_OFFSET 4
#define ui_INPUT_CURSOR_TIMER_TOTAL (0.1f * 20.0f)

#define DEF_LAYOUT_OFFSET 6.0f

#define ui_id_READONLY UIIDFROMPTR((void *)2, 0)
#define ui_id_SCREEN UIIDFROMPTR((void *)1, 0)
#define ui_id_ZERO UIIDFROMPTR(0, 0)

#define ui_COMMANDID(ui) UIIDFROMPTR(ui, ui)

#define ui_LASTAVADIBLEID 2

#define UIIDFROMPTR(ptr, ptr2) _ui_id_from_ptr(ptr, ptr2)
#define ui_id_POINTER(ptr) UIIDFROMPTR(ptr, ptr)
#define ui_id_POINTERS(ptr, ptr2) UIIDFROMPTR(ptr, ptr2)
#define ui_id_NUMBER_POINTER(n, ptr) UIIDFROMPTR(((void *)(memory_size)(n)), ptr)
#define ui_id_POINTER_NUMBER(ptr, n) UIIDFROMPTR(ptr, ((void *)(memory_size)(n)))
#define ui_id_NUMBERS(n, n2) UIIDFROMPTR(((void *)(memory_size)(n)), (void *)(memory_size)(n2))
#define ui_id_NUMBER(n) UIIDFROMPTR(((void *)(memory_size)(n)), (void *)(memory_size)(n))

#define ui_id_EQUALS(id1, id2) ui_IdEquals(id1, id2) 


#define ui_element_interacting(ui, elId) ui_IdEquals(ui->element_interacting, elId)
#define ui_element_interacted(ui, elId) ui_IdEquals(ui->element_last_interact, elId)

#define ui_element_forced_interacting(ui, elId) ui_IdEquals(ui->element_forced_interacting, elId)

#define ui_element_interacting_flags(ui, elementId, flags) (ui_element_interacting(ui, elementId) && (ui->interacting_flags & (flags)))
#define ui_element_interacted_flags(ui, elementId, flags) (ui_element_interacted(ui, elementId) && (ui->interacted_flags & (flags)))

#define ui_FontHeight(ui) (ui->fontp.font_height * ui->font_scale)

#define ui_PushClip_XYWH(ui, x, y, w, h) ui_PushClip(ui, x, y, (x + w), (y + h))
#define ui_push_clip_inside_last_XYWH(ui, x, y, w, h) ui_push_clip_inside_last(ui, x, y, (x + w), (y + h))

#define ui_ScaleHeight(ui) (ui->renderCommands->projection.m[1][1]);
#define ui_ScaleWidth(ui) (ui->renderCommands->projection.m[0][0]);

#define ui_explorer_FileDoubleClicked(ui) ((ui->explorer->file_got_selected) && !(ui->explorer->current_directory_files[ui->explorer->selected_file_index].is_directory) && (ui->interacted_flags & ui_interaction_mouse_left_double_click))
#define ui_explorer_SelectedFileIsDirectory(ui) (ui->explorer->current_directory_files[ui->explorer->selected_file_index].is_directory)
#define ui_explorer_SelectedFileData(ui) (ui->explorer->current_directory_files[ui->explorer->selected_file_index])

#define ui_disabled(ui) (ui->pushed_disable_true_count > 0)

#define ui_SCROLL_WH 14

/*
   Notes for new ui:
panel:
  push float(&size_x)
  push float(&size_y)
  push int(&is_closed)
  push int(&is_minimized)
  title bar:
   set "background" color
   ui_box_region (background, clipped, borders, text (centered))
   visible, not visible buttons
  set another "background" color
  ui_box_region (background, clip, borders)
  set layout (offset from start 4)
   push other widgets
   pop layout
  push button at bottom right of the panel's box region



   ui_box_region (background, borders, hot and interacting color.)
   layout inside
   push image inside box region size (keep aspect ratio)

   selectable_image (in persistent storage):
   push_float(&zoom);
   push_float(&clip_offset_x);
   push_float(&clip_offset_y);
   ui_box_region (background, borders)
   push rectangles colored inside region of the image
   push interactuable with mouse middle
   push image
   push interactuable with mouse left
   push rectangle borders inside image sized by uvs
   push rectangles inside the borders for interacting with uvs
   set ids.



   ui_run_commands(*panel)
   {
      u8 *commands = panel->commands;

	  read commands
	  read visual commands
	  switch()
   }
   */


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
ui_PushClip(game_ui *ui, i32 x0, i32 y0 , i32 x1, i32 y1)
{
     //render_commands_PushClip_GameCoords(ui->renderCommands, x0, y0 , x1, y1); 
	matrix4x4 uiMatrix = ui->projection;

	vec3 x0y0 = {(f32)x0, (f32)y0, 1};
	vec3 x1y1 = {(f32)x1, (f32)y1, 1};
    x0y0 = matrix4x4_v3_mul_rows(uiMatrix, x0y0, 0);
    x1y1 = matrix4x4_v3_mul_rows(uiMatrix, x1y1, 0);

	x0 = (i32)x0y0.x;
	y0 = (i32)x0y0.y;
	x1 = (i32)x1y1.x;
	y1 = (i32)x1y1.y;

     render_commands_push_clip_inside_last(ui->renderCommands, x0, y0 , x1, y1); 
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

inline f32
ui_current_panel_w(game_ui *ui)
{
	ui_panel *current_panel = ui->current_panel;
	f32 w = 0;
	if(current_panel)
	{
		w = current_panel->sz.x - current_panel->cornerOffsetX;
	}

	  //f32 totalContentX = (w + currentLayout->cursorX - cornerX) + 2 * current_panel->cornerOffsetX;
	  //f32 totalContentY = (h + currentLayout->cursorY - cornerY) + 2 * current_panel->cornerOffsetY;
	return(w);

}

inline f32
ui_current_panel_h(game_ui *ui)
{
	ui_panel *current_panel = ui->current_panel;
	f32 h = 0;
	if(current_panel)
	{
		h = current_panel->sz.y - current_panel->cornerOffsetY * 2;
	}

	  //f32 totalContentX = (w + currentLayout->cursorX - cornerX) + 2 * current_panel->cornerOffsetX;
	  //f32 totalContentY = (h + currentLayout->cursorY - cornerY) + 2 * current_panel->cornerOffsetY;
	return(h);

}

#define ui_DISABLED_ALPHA 120
#if 0
inline void
ui_RenderText2D(game_ui *ui, f32 x0, f32 y0, f32 x1, f32 y1, f32 scale)
{
  /*
render_text_2d_All(render_commands *commands,
		      font_proportional *fontData,
			  f32 startX,
			  f32 startY,
			  f32 endX,
			  f32 endY,
			  f32 scale,
			  vec4 color,
			  uint8 *text)
			  */
	render_text_2d(ui->renderCommands,
			      &ui->fontp,
				  x + textOffset.x,
				  y + textOffset.y,
				  F32MAX,
				  F32MAX,
				  ui->font_scale,
				  text);
}
#endif
#define ui_MouseOverRecClipped_XYWH(ui, x, y, w, h) ui_MouseOverRecClipped(ui, x, y, (x + w), (y + h))
inline i32
ui_MouseOverRecClipped(game_ui *ui, real32 x0, real32 y0, real32 x1, real32 y1)
{
	render_commands *renderCommands = ui->renderCommands;
	vec2 mousePosition              = ui->mouse_point;
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

    mousePosition.x *= scaleX;
    mousePosition.y *= scaleY;
#endif
	return(render_mouse_over_rec_clipped(renderCommands, mousePosition, x0, y0, x1, y1));
	//}
	
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

inline vec2
ui_get_scale(game_ui *ui)
{
	matrix4x4 uiMatrix = ui->projection;
	f32 scale_x = uiMatrix.m[0][0];
	f32 scale_y = uiMatrix.m[1][1];

	vec2 result = {scale_x, scale_y};
	return(result);
}

inline f32
ui_get_remaining_layout_width(game_ui *ui)
{
	f32 result = ui->currentLayout->cornerEndX - ui->currentLayout->cursorX;
	return(result);
}

inline f32
ui_get_remaining_layout_height(game_ui *ui)
{
	f32 result = ui->currentLayout->cornerEndY - ui->currentLayout->cursorY;
	return(result);
}

inline vec2
ui_get_remaining_layout_size(game_ui *ui)
{
	vec2 result = {ui_get_remaining_layout_width(ui),
	               ui_get_remaining_layout_height(ui)};
	return(result);
}

inline f32
ui_get_total_layout_width(game_ui *ui)
{
	f32 result = ui->currentLayout->cornerEndX - ui->currentLayout->cornerX;
	return(result);
}

inline f32
ui_get_total_layout_height(game_ui *ui)
{
	f32 result = ui->currentLayout->cornerEndY - ui->currentLayout->cornerY;
	return(result);
}

inline vec2
ui_get_total_layout_size(game_ui *ui)
{
	vec2 result = {ui_get_total_layout_width(ui),
	               ui_get_total_layout_height(ui)};
	return(result);
}


inline vec2
ui_set_size_if_pushed(game_ui *ui, vec2 v)
{
	if(ui->pushed_element_size)
	{
		if(ui->pushed_element_w)
		{
			v.x = ui->pushed_element_w;
		}
		if(ui->pushed_element_h)
		{
			v.y = ui->pushed_element_h;
		}
	}
	return(v);
}

inline vec2 
ui_get_layout_cursor(game_ui *ui)
{
	vec2 result = {ui->currentLayout->cursorX, ui->currentLayout->cursorY};
	return(result);
}

inline i32
ui_IdEquals(ui_id ID1, ui_id ID2)
{
    return((ID1.value[0] == ID2.value[0]) &&
           (ID1.value[1] == ID2.value[1]));
}

static ui_element_persistent *
ui_push_or_get_persistent_element(game_ui *ui, ui_id id)
{
	ui_element_persistent *result = 0;
	for(u32 p = 0;
			p < ui->persistentElementsCount;
			p++)
	{
		ui_element_persistent *element = ui->persistentElements + p;
		if(ui_id_EQUALS(element->id, id))
		{
			result = element;
			break;
		}
	}
	if(!result)
	{
		//Push a new persistent element
		result        = ui->persistentElements + ui->persistentElementsCount;
		result->id    = id;
		result->index = ui->persistentElementsCount;
		ui->persistentElementsCount++;
		Assert(ui->persistentElementsCount < ui->persistentElementsMax);
	}

	result->previous = ui->last_persistent_element;

	ui->last_persistent_element = result;
	return(result);
}

static void
ui_pop_persistent_element(game_ui *ui)
{
	ui->last_persistent_element = ui->last_persistent_element->previous;
}

//
// __ui utilities
//

#define ui_get_text_size(ui, endX, text) font_get_text_size_wrapped_scaled(&ui->fontp, endX, text, ui->font_scale)

inline vec2
ui_get_text_padded_offset_vec2(game_ui *ui, vec2 size, u8 *text, font_text_pad padOptions)
{
	vec2 textOff = font_get_text_pad_offset_vec2(
			&ui->fontp,
			size,
			text,
			ui->font_scale,
			padOptions);

	textOff.x = (f32)(i32)textOff.x;
	textOff.y = (f32)(i32)textOff.y;
	return(textOff);
}

inline i32
ui_element_hot(game_ui *ui, ui_id uiId)
{
	return(ui_IdEquals(ui->element_hot, uiId));		  
}

inline i32
ui_element_last_hot(game_ui *ui, ui_id uiId)
{
	return(ui_IdEquals(ui->element_last_hot, uiId));		  
}

inline i32
ui_Interacting(game_ui *ui)
{
	return(!ui_element_interacting(ui, ui_id_ZERO) &&
		   !ui_element_interacting(ui, ui_id_SCREEN));
}
inline i32
ui_Interacted(game_ui *ui)
{
	return(!ui_element_interacted(ui, ui_id_ZERO) &&
		   !ui_element_interacted(ui, ui_id_SCREEN));
}

inline i32
ui_any_element_hot(game_ui *ui)
{
	return(!ui_element_hot(ui, ui_id_ZERO) &&
		   !ui_element_hot(ui, ui_id_SCREEN));
}

inline i32
ui_element_forced_hot(game_ui *ui, ui_id uiId)
{
	return(ui_IdEquals(ui->element_forced_hot, uiId));		  
}

inline i32
ui_element_forced_last_hot(game_ui *ui, ui_id uiId)
{
	return(ui_IdEquals(ui->element_forced_last_hot, uiId));		  
}

static void
ui_CancelCurrentInteraction(game_ui *ui)
{
    ui->element_last_interact = ui->element_interacting; 
    ui->element_interacting   = UIIDFROMPTR(0, 0); 
}

static void
ui_advance(game_ui *ui, f32 x, f32 y, f32 w, f32 h) 
{
	ui_layout *currentLayout = ui->currentLayout;

	currentLayout->cursor_x_last = currentLayout->cursorX;
	currentLayout->cursor_y_last = currentLayout->cursorY;

	//
	// set total content size
	//
	
   //TODO: Get content size also from the corner start to back
    ui_panel *current_panel = ui->current_panel;
    
    f32 cornerX = (current_panel->frameSpace.x + current_panel->p.x) - current_panel->scroll_h;
    f32 cornerY = (current_panel->frameSpace.y + current_panel->p.y) - current_panel->scroll_v;
    
    //distance from the start of the frame to the current cursor counting the corners.
    f32 totalContentX = (w + currentLayout->cursorX - cornerX) + 2 * current_panel->cornerOffsetX;
    f32 totalContentY = (h + currentLayout->cursorY - cornerY) + 2 * current_panel->cornerOffsetY;
    
    current_panel->totalContentSize.x = MAX(totalContentX, current_panel->totalContentSize.x);
    current_panel->totalContentSize.y = MAX(totalContentY, current_panel->totalContentSize.y); 
	

	//Assumes it starts at 0 for now
	f32 nextXDelta = (f32)(i32)((x + w) - currentLayout->cursorX);
	f32 nextYDelta = (f32)(i32)((y + h) - currentLayout->cursorY);

	currentLayout->last_x_delta = 0; 
	if(currentLayout->keep_delta_x)
	{
		//set only when the next delta is bigger than the current
	    if(nextXDelta > currentLayout->nextXDelta)
	    {
	    	currentLayout->nextXDelta = nextXDelta;
	    }
	}
	else
	{
		//just set it no matter the size
	    currentLayout->nextXDelta = nextXDelta;
	}

	if(nextYDelta > currentLayout->nextYDelta)
	{
		currentLayout->nextYDelta = nextYDelta;
	}

	f32 total_x_delta = currentLayout->nextXDelta + currentLayout->spacingX;
	f32 total_y_delta = currentLayout->nextYDelta + currentLayout->spacingY;

	//Never advance when keeping
	if(!currentLayout->keepCursor)
	{

		if(currentLayout->same_line)
		{
			currentLayout->same_line = 0;
			currentLayout->cursorX += currentLayout->nextXDelta;
		}
		else if(currentLayout->keepLine)
	    {
	        currentLayout->last_x_delta = currentLayout->nextXDelta + currentLayout->spacingX;
			//advance at x
	    	currentLayout->cursorX = currentLayout->cursorX + w + currentLayout->spacingX; 

	  	    if(currentLayout->wrapLine)
	  	    {
	  	    	//Check if the end is bigger than the corner
	  	    	f32 elementEndX = currentLayout->cursorX + w + currentLayout->spacingX;
	  	    	currentLayout->nextXDelta = 0;

				//wrap and advance at Y
	  	    	if(elementEndX > currentLayout->cornerEndX)
	  	    	{
	                  currentLayout->last_x_delta = 0; 
	                  currentLayout->last_y_delta = currentLayout->nextYDelta + currentLayout->spacingY;

                      currentLayout->cursorY = currentLayout->cursorY + currentLayout->nextYDelta + currentLayout->spacingY; 
	                  currentLayout->cursorX = currentLayout->cornerX;
	                  currentLayout->nextYDelta = 0;

	  	    	}
	  	    }

	    }
	    else
	    {

	       //set last deltas at the end
	       currentLayout->last_x_delta = 0; 
	       currentLayout->last_y_delta = currentLayout->nextYDelta + currentLayout->spacingY;

           currentLayout->cursorY      = currentLayout->cursorY + currentLayout->nextYDelta + currentLayout->spacingY; 
	       currentLayout->cursorX      = currentLayout->cornerX;
	       currentLayout->nextYDelta = 0;
	    }


	}

	if(total_x_delta > currentLayout->total_advance_x)
	{
		currentLayout->total_advance_x = totalContentX;
	}
	if(total_y_delta > currentLayout->total_advance_y)
	{
		currentLayout->total_advance_y = totalContentY;
	}

}

inline i32
ui_IdInteracting(game_ui *ui, ui_id id)
{
	return(ui_IdEquals(ui->element_interacting, id));		  
}

#define ui_GetTextWidthAt(ui, textStart, textEnd, text) font_get_remaining_width_at(&ui->fontp, textStart, textEnd, ui->font_scale, text)

#define ui_get_text_width_with_offset(ui, text) (ui_get_text_size_with_offset(ui, text).x)
inline vec2
ui_get_text_size_with_offset(game_ui *ui, u8 *text)
{
	vec2 font_size = font_get_text_size_scaled(&ui->fontp, ui->font_scale, text);
	font_size.x += ui_TEXT_OFFSET; 
	font_size.y += ui_TEXT_OFFSET; 

	return(font_size);
}
//
//ui utilities__ 
//

//
//__New functions
//

typedef struct{
	ui_command_type type;
    ui_id id;	
}ui_element_button;

#define ui_push_array_to_command_buffer(ui, type, count) (type *)ui_push_size_to_command_buffer(ui, (sizeof(type) * count))
//Only pushes size
inline void *
ui_push_size_to_command_buffer(game_ui *ui, u32 size)
{
	//Temp assert
   Assert((ui->commands_offset + size) < (ui->commands_base + ui->commandsTotalSize));
   void *data         = ui->commands_offset;
   ui->commands_offset += size;
   return(data);
}

inline void *
ui_push_size_to_reserved(game_ui *ui, u32 size)
{
	//Temp assert
   Assert(ui->reserved_space_used + size < ui->reserved_space_total);

   u8 *at = ui->reserved_space + ui->reserved_space_used;
   void *data         = at;
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

#define ui_push_command(ui, type, commandType) (type *)_ui_push_command(ui, sizeof(type), commandType)
inline void *
_ui_push_command(game_ui *ui, u32 size, ui_command_type commandType)
{
   void *data				= ui_push_size_to_command_buffer(ui, size);
   *(ui_command_type *)data = commandType;
   return(data);
}

inline ui_element *
ui_PushElement(game_ui *ui, ui_id elementId, ui_command_type type)
{
   ui_element *element = ui_push_command(ui, ui_element, type);

   //Got an element with different id
   i32 expectedId = ui_id_EQUALS(element->id, elementId);
   //Reset to zero
   if(!expectedId)
   {
   	  ui_element emptyElement = {0};
   	  *element = emptyElement;
   }
   element->type = type;
   element->id = elementId;

   return(element);
}

inline void
ui_InitializePanel(game_ui *ui, ui_panel *panel, ui_id panelId, f32 pX, f32 pY, f32 pW, f32 pH)
{
	i32 notInitialized = !ui_id_EQUALS(panel->id, panelId);
	if(notInitialized)
	{
		panel->p.x  = pX;
		panel->p.y  = pY;
		panel->sz.x = pW;
		panel->sz.y = pH;
	    panel->id   = panelId;
	}
	panel->sz.x = panel->sz.x < 10 ? 10 : panel->sz.x;
	panel->sz.y = panel->sz.y < 10 ? 10 : panel->sz.y;

}

typedef struct{
	ui_command_type type;
	u32 offset;
}ui_commands_offset;

inline void
ui_PopPanel(game_ui *ui)
{
   Assert(ui->current_panel);
   Assert(ui->panel_stack_count > 0);

   ui_panel *current_panel = ui->current_panel;

   ui->current_panel->commands_offset = ui->commands_offset;

   //only applies to front panels
	u32 isChild = ui->current_panel->flags & ui_panel_flags_child;
	if(!isChild && ui->current_panel->parentPanel)
   {
	   //ignore the next commands while reading from the parent panel.
	   ui_commands_offset *commands_offset = (ui_commands_offset *)(ui->current_panel->commands_base - sizeof(ui_commands_offset));
	   //specify the offset size
	   commands_offset->offset = (u32)(current_panel->commands_offset - current_panel->commands_base);
   }
   ui->current_panel = ui->current_panel->parentPanel; 
}

inline void
ui_PushPanel(game_ui *ui, ui_panel *panel, ui_panel_flags panel_flags, ui_id panelId)
{

	//Reset panel to zero.
	i32 expectedId = ui_id_EQUALS(panel->id, panelId);
	if(!expectedId)
	{
		ui_panel emptyPanel = {0};
		*panel = emptyPanel;
	}
	panel->parentPanel = ui->current_panel;

   //only applies to front panels
	u32 isChild = panel_flags & ui_panel_flags_child;
	if(!isChild && panel->parentPanel)
	{
		ui_push_command(ui, ui_commands_offset, ui_command_type_offsetcommands);
	}
	
	panel->flags	    = panel_flags;
	panel->commands_base = ui->commands_offset;

	
	ui->current_panel = panel;

}

inline ui_panel *
ui_GetOrPushPanelStack(game_ui *ui, ui_id panelId)
{
	ui_panel *result = 0;

	u32 i = 0;
    u32 foundPanelOnStack = 0;

	while(!foundPanelOnStack && i < ui->panel_last_avadible_slot)
	{
		ui_panel *panelFromStack = ui->panel_stack + i;
		foundPanelOnStack = ui_id_EQUALS(panelFromStack->id, panelId);
		if(foundPanelOnStack)
		{
			result = panelFromStack;
		}
		i++;
	}
	//Register new panel on stack!
	if(!result)
	{
		result = ui->panel_stack + ui->panel_last_avadible_slot++;
		if(ui->panel_last_avadible_slot == ui->panel_stack_count)
		{
			Assert(0);
			ui->panelOverflow++;
		}
	}

	Assert(result);
	return(result);
}
inline ui_panel *
ui_PushPanelFromStack(game_ui *ui, ui_panel_flags panel_flags, ui_id panelId)
{
	ui_panel *panel = 0;
	//Detect overflow
	if(ui->panelOverflow)
	{
		//ui->panel_stack_count += 2;
		ui->panelOverflow++;
	
		//Temporary push panel to buffer
		panel = ui_push_size_to_command_buffer(ui, sizeof(ui_panel));
	
	}
	else
	{
		panel = ui->panel_stack + ui->panel_stack_count; 

	    i32 expectedId = ui_id_EQUALS(panel->id, panelId);
	    if(!expectedId)
	    {
			u32 i = 0;
            u32 foundPanelOnStack = 0;
			while(!foundPanelOnStack && i < ui->panel_last_avadible_slot)
			{
				foundPanelOnStack = ui_id_EQUALS(ui->panel_stack[i].id, panelId);
#if 1
				if(foundPanelOnStack)
				{
					ui_panel copyPanel = ui->panel_stack[ui->panel_stack_count];
					ui->panel_stack[ui->panel_stack_count] = ui->panel_stack[i];
					ui->panel_stack[i] = copyPanel;
				}
#else
				if(foundPanelOnStack)
				{
					panel = ui->panel_stack + i;
				}
#endif
				i++;
			}
			//Register new panel on stack!
			if(!foundPanelOnStack)
			{
				u32 a = ui->panel_stack_count;
				u32 b = ui->panel_last_avadible_slot;
				ui_panel copyPanel = ui->panel_stack[a];

				ui->panel_stack[a] = ui->panel_stack[b];
				ui->panel_stack[b] = copyPanel;


				ui->panel_last_avadible_slot++;
				if(ui->panel_last_avadible_slot == ui->panel_stack_count)
				{
					ui->panelOverflow++;
				}
			}
	    }
	    ui->panel_stack_count++;
	}
	Assert(ui->panel_last_avadible_slot <= ui->panel_stack_count);
	Assert(ui->panel_stack_count <= (ui->panelOverflow + ui->panel_last_avadible_slot));
	ui_PushPanel(ui, panel, panel_flags, panelId);

   return(panel);
}

inline ui_panel *
ui_PushPanelToCommandBuffer(game_ui *ui, ui_panel_flags panel_flags, ui_id panelId)
{
	ui_panel *panel = ui_push_size_to_command_buffer(ui, sizeof(ui_panel));
	ui_PushPanel(ui, panel, panel_flags, panelId);

	return(panel);
}


//
// New functions__
//

//
// __RESERVED SPACE
//


inline ui_layout
ui_push_layout(game_ui *ui,
		       f32 x0,
			   f32 y0,
			   f32 x1,
			   f32 y1,
			   f32 elementOffX,
			   f32 elementOffY,
			   f32 cursorOffX,
			   f32 cursorOffY)
{
	ui_layout result = {0};

    result.keepLine = 0;
    result.spacingX = 4;
    result.spacingY = 4;
	result.cornerX  = x0 + elementOffX + cursorOffX;
	result.cornerY  = y0 + elementOffY + cursorOffY;
	result.cursorX  = result.cornerX;
	result.cursorY  = result.cornerY;
	//result.cornerOffsetX = elementOffX;
	//result.cornerOffsetY = elementOffY;


	result.cornerEndX = x1 - elementOffX; 
	result.cornerEndY = y1 - elementOffY;
	ui_layout *pushedLayout = 0;

	if(ui->layoutAmountPushed >= ui->layoutStackCount)
	{
		ui->layoutAmountBeyondStack = MAX((u32)(ui->layoutAmountPushed - ui->layoutStackCount + 1), ui->layoutAmountBeyondStack);
		//Push and reset the layout
		pushedLayout  = (ui_layout *)ui_push_size_to_command_buffer(ui, sizeof(ui_layout));
		*pushedLayout = result;


	}
	else
	{
	  ui->layoutStack[ui->layoutAmountPushed] = result;
	  pushedLayout = ui->layoutStack + ui->layoutAmountPushed;
	}

	pushedLayout->previousLayout = ui->currentLayout;
	ui->currentLayout = pushedLayout;
	ui->layoutAmountPushed++;

	Assert(ui->layoutAmountPushed <= (ui->layoutStackCount + ui->layoutAmountBeyondStack));

	return(result);
}

inline void
ui_pop_layout(game_ui *ui)
{
	Assert(ui->layoutAmountPushed);

	ui->layoutAmountPushed--;
	ui->currentLayout = ui->currentLayout->previousLayout;
}

//
// RESERVED SPACE__
//


inline void
ui_push_cursor_screen(game_ui *ui, f32 x, f32 y)
{
	ui_element *element = ui_PushElement(ui, ui_id_NUMBER_POINTER(ui->reservedIdValue++ ,ui), ui_command_type_pushcursorposition);
	element->layout.x = x;
	element->layout.y = y;
	element->layout.flags = ui_pushcursor_screen | ui_pushcursor_set;
}


inline void
ui_push_cursor_position(game_ui *ui, f32 x, f32 y)
{
	ui_element *element = ui_PushElement(ui, ui_id_NUMBER_POINTER(ui->reservedIdValue++ ,ui), ui_command_type_pushcursorposition);
	element->layout.x = x;
	element->layout.y = y;
	element->layout.flags = ui_pushcursor_set;
}

inline void
ui_AddAndPushCursor(game_ui *ui, f32 x, f32 y)
{
	ui_element *element = ui_PushElement(ui, ui_id_NUMBER_POINTER(ui->reservedIdValue++ ,ui), ui_command_type_pushcursorposition);
	element->layout.x = x;
	element->layout.y = y;
	element->layout.flags = ui_pushcursor_add;
}

#define ui_push_element_size(ui, w, h) _ui_push_element_size(ui, w, h, ui_size_Specified)
#define ui_push_element_width(ui, w) _ui_push_element_size(ui, w, 0, ui_size_Specified)
#define ui_push_element_height(ui, h) _ui_push_element_size(ui, 0, h, ui_size_Specified)
#define ui_push_element_sizeToText(ui) _ui_push_element_size(ui, 0, 0, ui_size_ToText)

inline void
_ui_push_element_size(game_ui *ui, f32 w, f32 h, ui_size_options sizeOption)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_setelementsize);
	element->elementsize.push   = 1;
	element->elementsize.width  = w;
	element->elementsize.height = h;
	element->elementsize.sizeOption = sizeOption;
}

inline void
ui_pop_element_size(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_setelementsize);
	element->elementsize.push   = 0;
	element->elementsize.width  = 0;
	element->elementsize.height = 0;
}


inline void
ui_pop_cursor(game_ui *ui)
{
	ui_push_command(ui, ui_command_type, ui_command_type_popcursor);
}

inline void
ui_update_element_forced_at(
		game_ui *ui,
		ui_id id,
		f32 x,
		f32 y,
		f32 w,
		f32 h)
{
	f32 x0 = x;
	f32 y0 = y;
	f32 x1 = x0 + w;
	f32 y1 = y0 + h;

	//probably will remove these
	u32 disabled = ui_disabled(ui);
	u32 hovered = ui_MouseOverRecClipped(
			ui, x0, y0, x1, y1);

	if(!disabled && hovered)
    {
		ui->element_forced_last_hot = id;
    }
}

inline void
ui_update_element_at(game_ui *ui, ui_element *element, f32 x, f32 y, f32 szX, f32 szY)
{
	f32 lX0 = x;
	f32 lY0 = y;
	f32 lX1 = lX0 + szX;
	f32 lY1 = lY0 + szY;

	//probably will remove these
	u32 disabled = ui_disabled(ui);
	u32 elementHovered = ui_MouseOverRecClipped(
			ui, lX0, lY0, lX1, lY1);

	if(!disabled && elementHovered)
    {
        ui->element_last_hot = element->id;
		ui->element_forced_last_hot = element->id;
    }
}

inline void
ui_create_update_element_at_baycentric(game_ui *ui,
		                               ui_id id,
		                               vec2 v0,
									   vec2 v1,
									   vec2 v2)
{
    f32 w1 = 0;
    f32 w2 = 0;
    
    vec2 p = ui->mouse_point;
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

    ui->reserved_vec.x = w1;
    ui->reserved_vec.y = w2;

	if(!disabled && elementHovered)
    {
        ui->element_last_hot = id;
    }
}

inline void
ui_create_update_element_at(game_ui *ui, ui_id id, f32 x, f32 y, f32 w, f32 h)
{
	ui_element tempElement = {0};
	tempElement.id = id;
	ui_update_element_at(ui, &tempElement, x, y, w, h);
}

inline void
ui_update_advance_element(game_ui *ui, ui_element *element, f32 szX, f32 szY)
{
	ui_layout *uiLayout = ui->currentLayout;

	f32 lX0 = uiLayout->cursorX;
	f32 lY0 = uiLayout->cursorY;
	f32 lX1 = lX0 + szX;
	f32 lY1 = lY0 + szY;
	ui_update_element_at(ui, element,uiLayout->cursorX, uiLayout->cursorY, szX, szY);
	ui_advance( ui, lX0, lY0, szX, szY);
}

inline void
ui_create_update_element_at_layout(game_ui *ui, ui_id id, f32 w, f32 h)
{
	ui_element tempElement = {0};
	tempElement.id = id;
	ui_update_advance_element(ui, &tempElement, w, h);
}


inline void
ui_UpdateTextElement(game_ui *ui, ui_element *element, f32 textEnd, u8 *text)
{
	ui_layout *currentLayout = ui->currentLayout;

	vec2 text_dimensions = ui_get_text_size(ui, textEnd, text);
	vec2 cursor_position_l = ui_get_layout_cursor(ui);

	ui_advance(ui, cursor_position_l.x, cursor_position_l.y, text_dimensions.x, text_dimensions.y); 
}


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
  ui_element *element  = ui_push_command(ui, ui_element, ui_command_type_selectable_directions);
  element->selectable_directions.directions = orientation_count;

  if(!orientation_count)
  {
	  orientation_count = 1;
  }
  u32 current_direction   = 0;

  ui_selectable_directions_info result = {0};
  u32 d = 0;
  while(d < orientation_count)
  {
      ui_id dir_id   = ui_id_NUMBER_POINTER(current_direction, element);
      u32 interacted = ui_element_interacting_flags(ui, dir_id, ui_interaction_mouse_left_down);

      if(interacted)
      {
		  result.interacted         = 1;
		  result.assigned_direction = current_direction;

		  element->selectable_directions.selected_direction = current_direction;
		  break;
      }
	  current_direction += 1;
	  d++;
  }	
  return(result);
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

#define ui_text(ui, text)               ui_text_All(ui, 0, vec4_all(255), text)
#define ui_text_colored(ui, color, text) ui_text_All(ui, 0, color, text)
#define ui_textWrapped(ui, text)        ui_text_All(ui, 1, vec4_all(255),text)

#define ui_textf(ui, text, ...)                ui_textf_All(ui, 0, vec4_all(255), text, __VA_ARGS__)
#define ui_textf_colored(ui, color, text, ...) ui_textf_All(ui, 0, color, text, __VA_ARGS__)

inline void
ui_text_All(game_ui *ui, u32 wrap, vec4 color, u8 *text)
{
  ui_element *element  = ui_push_command(ui, ui_element, ui_command_type_text);
  element->id		   = UIIDFROMPTR(text, text);
  element->text.length = string_count(text);
  element->label       = ui_push_string_to_reserved(ui, text, element->text.length);
  element->text.wrap   = wrap;
  element->text.color  = color;

}

inline void
ui_textf_All(game_ui *ui, u32 wrap, vec4 color, u8 *text, ...)
{
   u8 textBuffer[256] = {0};
   va_list args;
   va_start_m(args, text);
   FormatTextList(textBuffer, sizeof(textBuffer), text, args);
   va_end_m(args);

   ui_text_All(ui,wrap, color, textBuffer);
}

inline i32 
ui_button(game_ui *ui, u8 *text)
{

	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_button);
	element->id         = UIIDFROMPTR(text, text);
	element->label      = text;


	i32 interacting = ui_element_hot(ui, element->id) && ui_element_interacted_flags(ui, element->id, ui_interaction_mouse_left_down);
	return(interacting);
}

static inline u32
ui_button_down(game_ui *ui, u8 *text)
{

	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_button);
	element->id         = UIIDFROMPTR(text, text);
	element->label      = text;


	i32 interacting = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down);
	return(interacting);
}

inline i32 
ui_button_toggle(game_ui *ui, u32 *boolean, u8 *text)
{

	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_button);
	element->id         = UIIDFROMPTR(text, text);
	element->label      = text;


	i32 interacting = ui_element_hot(ui, element->id) && ui_element_interacted_flags(ui, element->id, ui_interaction_mouse_left_down);
	if(interacting)
	{
		(*boolean) = !(*boolean);
	}
	return(interacting);
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
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_imagebutton);
	element->id         = UIIDFROMPTR(element, texture);

	element->image_button.texture = texture;
	element->image_button.buttonW = buttonW;
	element->image_button.buttonH = buttonH;

    render_uvs uvs_transformed = render_frames_to_uv(
			ui->renderCommands->gameRenderer->texture_array_w,
			ui->renderCommands->gameRenderer->texture_array_h,
		                                                     frame_x,
					                                         frame_y,
					                                         frame_w,
					                                         frame_h);
	element->image_button.uv0 = uvs_transformed.uv0;
	element->image_button.uv1 = uvs_transformed.uv1;
	element->image_button.uv2 = uvs_transformed.uv2;
	element->image_button.uv3 = uvs_transformed.uv3;

	u32 interacted = ui_element_interacted(ui, element->id);
	return(interacted);
}

inline u32
ui_button_image_uv(game_ui *ui,
				render_texture *texture,
		        f32 buttonW,
				f32 buttonH,
				vec2 uv0,
				vec2 uv1,
				vec2 uv2,
				vec2 uv3)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_imagebutton);
	element->id         = UIIDFROMPTR(element, texture);

	element->image_button.texture = texture;
	element->image_button.buttonW = buttonW;
	element->image_button.buttonH = buttonH;
	element->image_button.uv0 = uv0;
	element->image_button.uv1 = uv1;
	element->image_button.uv2 = uv2;
	element->image_button.uv3 = uv3;

	u32 interacted = ui_element_interacted(ui, element->id);
	return(interacted);
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
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_imagebutton);
	element->id         = UIIDFROMPTR(element, texture);

	element->image_button.texture = texture;
	element->image_button.buttonW = element_w;
	element->image_button.buttonH = element_h;
	element->image_button.uv0 = uv0;
	element->image_button.uv1 = uv1;
	element->image_button.uv2 = uv2;
	element->image_button.uv3 = uv3;
	element->image_button.selectable_active = active;

	u32 interacted = ui_element_interacted(ui, element->id);
	return(interacted);
}



inline i32
ui_image_frames(game_ui *ui, u8 *label, render_texture *texture, f32 scaleX, f32 scaleY)
{
	//Ready to implemen *-*
	//Do selectable region?
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_image);
	element->image.source = texture;
	element->image.w = texture->width * scaleX;
	element->image.h = texture->height * scaleY;
	element->id = UIIDFROMPTR(label, texture);

	return( ui_element_hot(ui, element->id) && ui_element_interacted_flags(ui, element->id, ui_interaction_mouse_left_down));
}

#define ui_selectableBorders(ui, selected, text) ui_selectable_All(ui, selected, 1, text)
#define ui_selectable(ui, selected, text) ui_selectable_All(ui, selected, 0, text)
#define ui_selectableBordersf(ui, selected, text, ...) ui_selectablef_All(ui, selected, 1, text, __VA_ARGS__)
#define ui_selectablef(ui, selected, text, ...) ui_selectablef_All(ui, selected, 0, text, __VA_ARGS__)

inline i32 
ui_selectable_All(game_ui *ui, u32 selected, u32 displayBorders, u8 *text)
{

	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_selectable);
	//element->id         = UIIDFROMPTR(text, ui->reservedIdValue++);
	u32 textLength      = string_count(text);
	element->id         = ui_id_NUMBER_POINTER(ui->reservedIdValue++, text);
	element->label      = ui_push_string_to_reserved(ui, text, textLength);
	element->selection.selected = (u16)selected;
	element->selection.borders  = (u16)displayBorders;

	u32 interacted = (ui_element_interacted(ui, element->id));
	return(interacted);
}

inline i32
ui_selectablef_All(game_ui *ui, u32 selected, u32 displayBorders, u8 *text, ...)
{
	u8 textBuffer[256] = {0};

    va_list args;
    va_start_m(args, text);
    uint32 textSize = FormatTextList(textBuffer, sizeof(textBuffer), text, args);
    va_end_m(args);

	u32 interacted = ui_selectable_All(ui, selected, displayBorders, textBuffer);

	return(interacted);
	//return(ui_element_interacted(ui, element->id));
    //return(ui_selectable(ui, selected, selectionText));

}

inline u32
ui_selectable_toggle_u32(game_ui *ui, u32 *value, u8 *text)
{
	u32 interacted = ui_selectable_All(ui, *value, 0, text);

	if(interacted)
	{
		*value = !(*value);
	}
	return(interacted);
}

inline u32
ui_selectable_set_u32(
		game_ui *ui,
		u32 value_to_set,
		u32 *target_value,
		u8 *text)
{
	u32 interacted = ui_selectable_All(ui, (*target_value) == value_to_set, 0, text);

	if(interacted)
	{
		*target_value = value_to_set;
	}
	return(interacted);
}

inline u32
ui_selectable_set_u16(
		game_ui *ui,
		u16 value_to_set,
		u16 *target_value,
		u8 *text)
{
	u32 interacted = ui_selectable_All(ui, (*target_value) == value_to_set, 0, text);

	if(interacted)
	{
		*target_value = value_to_set;
	}
	return(interacted);
}

inline u32
ui_selectable_set_u32f(
		game_ui *ui,
		u32 value_to_set,
		u32 *target_value,
		u8 *text,
		...)
{

	u8 textBuffer[256] = {0};

    va_list args;
    va_start_m(args, text);
    uint32 textSize = FormatTextList(textBuffer, sizeof(textBuffer), text, args);
    va_end_m(args);
	u32 interacted = ui_selectable_set_u32(ui, value_to_set, target_value, textBuffer);

	return(interacted);
}

inline u32
ui_selectable_set_u16f(
		game_ui *ui,
		u16 value_to_set,
		u16 *target_value,
		u8 *text,
		...)
{

	u8 textBuffer[256] = {0};

    va_list args;
    va_start_m(args, text);
    uint32 textSize = FormatTextList(textBuffer, sizeof(textBuffer), text, args);
    va_end_m(args);
	u32 interacted = ui_selectable_set_u16(ui, value_to_set, target_value, textBuffer);

	return(interacted);
}


//Define the function by concadenating te type
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
	ui_id sliderId = UIIDFROMPTR(text, value);
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_drag);
	element->id         = sliderId; 
	element->label      = text;
	element->slider.value     = value;
	element->slider.valueType = type;
	element->slider.format = ui_count_and_push_string_to_reserved(ui, format); 

	u32 interacting = ui_element_interacting(ui, sliderId);
	return(interacting);
}

UI_DRAG_TYPE_FORMAT(u32)
{
	u32 interacting = ui_push_drag(ui, value, ui_value_u32, text, format);
	if(interacting)
	{
	    f32 mouse_delta_x    = ui->mouse_point.x - ui->mouse_point_last.x; 
		UI_DRAGGABLEX_ADDDEC(u32, value, (u32)(mouse_delta_x * inc_dec));
	}
	return(interacting);
}
UI_DRAG_TYPE(u32)
{
	u8 text_buffer[32] = {0};
	FormatText(text_buffer, sizeof(text_buffer), "%u", *value);
	ui_drag_u32_format(ui, inc_dec, min, max, value, text, text_buffer);
}

UI_DRAG_TYPE_FORMAT(u16)
{
	u16 interacting = ui_push_drag(ui, value, ui_value_u16, text, format);
	if(interacting)
	{
	    f32 mouse_delta_x    = ui->mouse_point.x - ui->mouse_point_last.x; 
		UI_DRAGGABLEX_ADDDEC(u16, value, (u16)(mouse_delta_x * inc_dec));
	}
	return(interacting);
}
UI_DRAG_TYPE(u16)
{
	u8 text_buffer[32] = {0};
	FormatText(text_buffer, sizeof(text_buffer), "%u",*value);
	ui_drag_u16_format(ui, inc_dec, min, max, value, text, text_buffer);
}

UI_DRAG_TYPE_FORMAT(f32)
{
	u32 interacting = ui_push_drag(ui, value, ui_value_f32, text, format);
	if(interacting)
	{
	    i32 mouse_delta_x = (i32)(ui->mouse_point.x - ui->mouse_point_last.x);
		UI_DRAGGABLEX_ADDDEC(f32, value, mouse_delta_x * inc_dec);
	}
	return(interacting);
}
UI_DRAG_TYPE(f32)
{
	u8 text_buffer[32] = {0};
	FormatText(text_buffer, sizeof(text_buffer), "%f", *value);
	ui_drag_f32_format(ui, inc_dec, min, max, value, text, text_buffer);
}

UI_DRAG_NAME_TYPE(radians, f32)
{
	u8 text_buffer[32] = {0};
	f32 radians_as_degree = radians_to_degrees_f32(*value);
	f32 min_as_radians = degrees_to_radians_f32(min);
	f32 max_as_radians = degrees_to_radians_f32(max);
	f32 inc_dec_as_radians = degrees_to_radians_f32(inc_dec);
	FormatText(text_buffer, sizeof(text_buffer), "%f", radians_as_degree);
	ui_drag_f32_format(ui, inc_dec_as_radians, min_as_radians, max_as_radians, value, text, text_buffer);
}

inline i32
ui_check_box(game_ui *ui, void *value, u8 *text)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_checkbox);
	element->id      = UIIDFROMPTR(text, value);
	element->label   = text;
	element->value	 = value;


	u8 checked = *(u8 *)value; 
	u32 interacted = ui_element_interacted_flags(ui, element->id, ui_interaction_mouse_left_down);
	if(interacted)
	{
       *(u8 *)value = !*(u8 *)value;	
	}
	return(interacted);

}

inline void 
ui_QuickToolTip(game_ui *ui, f32 regionW, f32 regionH, u8 *label)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_tooltip);
	element->id         = UIIDFROMPTR(label, label);
	element->label      = label;
	element->tooltip.regionW = regionW;
	element->tooltip.regionH = regionH;

	ui_id panelId       = UIIDFROMPTR(label, "tt_panel");

	element->tooltip.panel = ui_PushPanelToCommandBuffer(ui, ui_panel_flags_child | ui_panel_flags_tooltip, panelId);
	ui_panel *toolTipPanel = element->tooltip.panel;
	ui_InitializePanel(ui, toolTipPanel, panelId, 0, 0, 200, 200);

	ui_text(ui, label);

	ui_PopPanel(ui);
	// draw text and pop panel
}

inline void 
_ui_keep_line_push(game_ui *ui, u32 wrap)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_keeplinepush);
   element->keepline.wrap = wrap;
}

#define ui_keep_line_and_wrap(ui) _ui_keep_line_push(ui, 1)
#define ui_keep_line_push(ui) _ui_keep_line_push(ui, 0)

//inline void 
//ui_keep_line_and_wrap(game_ui *ui)
//{
//   ui_push_command(ui, ui_command_type, ui_command_type_keeplinepush);
//}

inline void 
ui_keep_line_pop(game_ui *ui)
{
   ui_push_command(ui, ui_command_type, ui_command_type_keeplinepop);
}

inline void 
ui_KeepCursorPush(game_ui *ui)
{
   ui_push_command(ui, ui_command_type, ui_command_type_keepcursorpush);
}

inline void 
ui_KeepCursorPop(game_ui *ui)
{
   ui_push_command(ui, ui_command_type, ui_command_type_keepcursorpop);
}

inline void
ui_push_disable_if(game_ui *ui, u32 condition)
{
	ui_id pushDisableId = ui_id_READONLY;
	ui_element *element = ui_PushElement(ui, pushDisableId, ui_command_type_setdisable);
	element->disable.condition = condition;
	element->disable.pop = 0;
}

inline void
ui_pop_disable(game_ui *ui)
{
	ui_id pushDisableId = ui_id_READONLY;
	ui_element *element = ui_PushElement(ui, pushDisableId, ui_command_type_setdisable);
	element->disable.condition = 0;
	element->disable.pop = 1;
}


inline void
ui_space(game_ui *ui, f32 distance)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_separator);
	element->separator.distance = distance;

}

inline void
ui_next_line_push(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_next_line_set);
	element->next_line.push = 1;
}


inline void
ui_next_line_pop(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_next_line_set);
	element->next_line.push = 0;
}

inline void
ui_same_line(game_ui *ui)
{
	ui_push_command(ui, ui_command_type, ui_command_type_same_line);
}

inline vec2
ui_get_frame_size(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_get_frame_size);
	element->get_frame_size.remaining = 1;
	vec2 result = element->get_frame_size.size;

	return(result);
}

inline vec2
ui_get_remaining_frame_size(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_get_frame_size);
	element->get_frame_size.remaining = 1;
	vec2 result = element->get_frame_size.size;

	return(result);
}



static u32 
ui_selectableGrid(game_ui *ui, u8 *label, f32 gridSizeX, f32 gridSizeY, f32 tileGridSize, u32 tileDisplacementX, u32 tileDisplacementY, f32 gridScale, vec2 *selectionPosition)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_selectable_grid);
	element->id						= UIIDFROMPTR(label, label);
	element->grid.tileGridSize      = tileGridSize;
	element->grid.sizeX             = gridSizeX;
	element->grid.sizeY             = gridSizeY;
	element->grid.tileDisplacementX = tileDisplacementX;
	element->grid.tileDisplacementY = tileDisplacementY;
	element->grid.selectedP         = selectionPosition;
	element->grid.scale = gridScale;

	return(ui_element_interacted(ui, element->id));
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

	u32 kindaHash = string_kinda_hash(label);
	ui_id selectableImageId = ui_id_NUMBER(kindaHash);

	ui_element_persistent *persistentElement = ui_push_or_get_persistent_element(ui, selectableImageId);
	ui_element *element = ui_PushElement(ui, selectableImageId, ui_command_type_selectable_image);
	
	element->persistent_element_array_index = persistentElement->index;

	persistentElement->selectable_image.texture = texture;

	persistentElement->selectable_image.tileSize_w = tileSize_w;
	persistentElement->selectable_image.tileSize_h = tileSize_h;

	if(f32_NEARZERO(persistentElement->selectable_image.zoom))
	{
		persistentElement->selectable_image.zoom = 1.0f;
	}
	return(persistentElement);

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
	ui_element_persistent *element = _ui_push_selectable_image(ui,
			                                                   texture,
															   tileSize_w,
															   tileSize_h,
															   showGrid,
															   label);

	u32 element_interacting = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down | ui_interaction_mouse_right_down);
	if(element_interacting)
	{
		*selectionX = element->selectable_image.selectedPixelX;
		*selectionY = element->selectable_image.selectedPixelY;
		*selectionW = element->selectable_image.selectedPixelW;
		*selectionH = element->selectable_image.selectedPixelH;
	}

	element->selectable_image.selectedPixelX = *selectionX;
	element->selectable_image.selectedPixelY = *selectionY;
	element->selectable_image.selectedPixelW = *selectionW;
	element->selectable_image.selectedPixelH = *selectionH;
	return(element_interacting);
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
	ui_element_persistent *element = _ui_push_selectable_image(ui,
			                                                   texture,
															   tileSize_w,
															   tileSize_h,
															   showGrid,
															   label);

	u32 element_interacting = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down | ui_interaction_mouse_right_down);
	if(element_interacting)
	{
		*selectionX = element->selectable_image.selectedPixelX;
		*selectionY = element->selectable_image.selectedPixelY;
		*selectionW = element->selectable_image.selectedPixelW;
		*selectionH = element->selectable_image.selectedPixelH;
	}

	element->selectable_image.selectedPixelX = *selectionX;
	element->selectable_image.selectedPixelY = *selectionY;
	element->selectable_image.selectedPixelW = *selectionW;
	element->selectable_image.selectedPixelH = *selectionH;
	return(element_interacting);
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
	ui_element_persistent *element = _ui_push_selectable_image(ui,
			                                                   texture,
															   tileSize_w,
															   tileSize_h,
															   showGrid,
															   label);

	u32 element_interacting = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down | ui_interaction_mouse_right_down);
	if(element_interacting)
	{
		render_uv_minmax uv_min_max =  render_frames_to_uv_min_max(
				ui->renderCommands->gameRenderer->texture_array_w,
				ui->renderCommands->gameRenderer->texture_array_h,
		                                                   element->selectable_image.selectedPixelX,
					                                       element->selectable_image.selectedPixelY,
					                                       element->selectable_image.selectedPixelW,
					                                       element->selectable_image.selectedPixelH);

		*uv_min = uv_min_max.min;
		*uv_max = uv_min_max.max;
	}
    render_uv_frames uv_frames = render_uv_min_max_to_frames(
			ui->renderCommands->gameRenderer->texture_array_w,
			ui->renderCommands->gameRenderer->texture_array_h,
			                                         *uv_min,
													 *uv_max);

	element->selectable_image.selectedPixelX = uv_frames.frame_x;
	element->selectable_image.selectedPixelY = uv_frames.frame_y;
	element->selectable_image.selectedPixelW = uv_frames.frame_w;
	element->selectable_image.selectedPixelH = uv_frames.frame_h;
	return(element_interacting);
}

static u32
ui_image_selection_uv(game_ui *ui,
					   render_texture *texture,
					   vec2 *uv0,
					   vec2 *uv1,
					   vec2 *uv2,
					   vec2 *uv3,
				       u32 tileSize_w,
				       u32 tileSize_h,
				       u32 showGrid,
					   u8 *label)
{
	ui_element_persistent *element = _ui_push_selectable_image(ui,
			                                                   texture,
															   tileSize_w,
															   tileSize_h,
															   showGrid,
															   label);

	u32 element_interacting = element->selectable_image.uvsUpdated; 
	if(element_interacting)
	{

		vec2 element_uv0 = element->selectable_image.uv0_scaled;
		vec2 element_uv1 = element->selectable_image.uv1_scaled;
		vec2 element_uv2 = element->selectable_image.uv2_scaled;
		vec2 element_uv3 = element->selectable_image.uv3_scaled;
		f32 one_over_w = 1.0f / texture->width;
		f32 one_over_h = 1.0f / texture->height;

		uv0->x = element_uv0.x * one_over_w;
		uv0->y = element_uv0.y * one_over_h;

		uv1->x = element_uv1.x * one_over_w;
		uv1->y = element_uv1.y * one_over_h;

		uv2->x = element_uv2.x * one_over_w;
		uv2->y = element_uv2.y * one_over_h;

		uv3->x = element_uv3.x * one_over_w;
		uv3->y = element_uv3.y * one_over_h;
	}
	vec2 uv0_scaled = {0};
	vec2 uv1_scaled = {0};
	vec2 uv2_scaled = {0};
	vec2 uv3_scaled = {0};

	uv0_scaled.x = (f32)(i32)(uv0->x * texture->width);
	uv0_scaled.y = (f32)(i32)(uv0->y * texture->height);

	uv1_scaled.x = (f32)(i32)(uv1->x * texture->width);
	uv1_scaled.y = (f32)(i32)(uv1->y * texture->height);

	uv2_scaled.x = (f32)(i32)(uv2->x * texture->width);
	uv2_scaled.y = (f32)(i32)(uv2->y * texture->height);

	uv3_scaled.x = (f32)(i32)(uv3->x * texture->width);
	uv3_scaled.y = (f32)(i32)(uv3->y * texture->height);

	element->selectable_image.uv0_scaled = uv0_scaled;
	element->selectable_image.uv1_scaled = uv1_scaled;
	element->selectable_image.uv2_scaled = uv2_scaled;
	element->selectable_image.uv3_scaled = uv3_scaled;
	return(element_interacting);
}

static u32
ui_image_selection_begin(game_ui *ui,
					      render_texture *texture,
				          u32 tileSize_w,
				          u32 tileSize_h,
				          u32 showGrid,
					      u8 *label)
{
	ui_element_persistent *element = _ui_push_selectable_image(ui,
			                                                   texture,
															   tileSize_w,
															   tileSize_h,
															   showGrid,
															   label);

	ui_image_selection *selectable_image = &element->selectable_image;
	if(!selectable_image->selectable_group_max)
	{
		selectable_image->selectable_group_max = 3;
	}
	if(selectable_image->selectable_group_count > selectable_image->selectable_group_max)
	{
		selectable_image->selectable_group_max += selectable_image->selectable_group_count + 3;
	}

	selectable_image->selectable_group_count = 0;
	selectable_image->selectable_group       = ui_push_size_to_command_buffer(ui, selectable_image->selectable_group_max * sizeof(struct ui_image_selection_group));

	u32 element_interacting = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down | ui_interaction_mouse_right_down);
	ui->last_persistent_element = element;

	return(element_interacting);
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
	ui_image_selection *selectable_image = &ui->last_persistent_element->selectable_image;
	u32 selectable_group_overflow         = selectable_image->selectable_group_count >= selectable_image->selectable_group_max;
	u32 interacted = 0;

	render_texture *texture = selectable_image->texture;
	if(!selectable_group_overflow && texture)
	{
		struct ui_image_selection_group *selectable_group = selectable_image->selectable_group + selectable_image->selectable_group_count;
		u32 updated = selectable_image->uvsUpdated &&
			          selectable_image->selectable_group_selected == selectable_image->selectable_group_count;
		selectable_group->selection_type_uv = 1;
		selectable_group->label = label;
		//pick texture from the main struct

	    if(updated)
	    {
			interacted = 1;

	    	vec2 element_uv0 = selectable_group->uv0_scaled;
	    	vec2 element_uv1 = selectable_group->uv1_scaled;
	    	vec2 element_uv2 = selectable_group->uv2_scaled;
	    	vec2 element_uv3 = selectable_group->uv3_scaled;

	    	f32 one_over_w = 1.0f / texture->width;
	    	f32 one_over_h = 1.0f / texture->height;

	    	uv0->x = element_uv0.x * one_over_w;
	    	uv0->y = element_uv0.y * one_over_h;

	    	uv1->x = element_uv1.x * one_over_w;
	    	uv1->y = element_uv1.y * one_over_h;

	    	uv2->x = element_uv2.x * one_over_w;
	    	uv2->y = element_uv2.y * one_over_h;

	    	uv3->x = element_uv3.x * one_over_w;
	    	uv3->y = element_uv3.y * one_over_h;
	    }
	    vec2 uv0_scaled = {0};
	    vec2 uv1_scaled = {0};
	    vec2 uv2_scaled = {0};
	    vec2 uv3_scaled = {0};

	    uv0_scaled.x = (f32)(i32)(uv0->x * texture->width);
	    uv0_scaled.y = (f32)(i32)(uv0->y * texture->height);

	    uv1_scaled.x = (f32)(i32)(uv1->x * texture->width);
	    uv1_scaled.y = (f32)(i32)(uv1->y * texture->height);

	    uv2_scaled.x = (f32)(i32)(uv2->x * texture->width);
	    uv2_scaled.y = (f32)(i32)(uv2->y * texture->height);

	    uv3_scaled.x = (f32)(i32)(uv3->x * texture->width);
	    uv3_scaled.y = (f32)(i32)(uv3->y * texture->height);

	    selectable_group->uv0_scaled = uv0_scaled;
	    selectable_group->uv1_scaled = uv1_scaled;
	    selectable_group->uv2_scaled = uv2_scaled;
	    selectable_group->uv3_scaled = uv3_scaled;


	}
	selectable_image->selectable_group_count++;

	return(interacted);
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
   u8 text_buffer[256] = {0};
   va_list args;
   va_start_m(args, label);
   u32 text_length = FormatTextList(text_buffer, sizeof(text_buffer), label, args);
   va_end_m(args);

   u8 *label_pushed = ui_push_string_to_reserved(ui, text_buffer, text_length);
   ui_image_selection_push_uv_selection_label(
		   ui,
		   uv0,
		   uv1,
		   uv2,
		   uv3,
		   label_pushed);
}

static u32
ui_image_selection_push_frame_selection(game_ui *ui,
					                     u16 *frame_x,
					                     u16 *frame_y,
					                     u16 *frame_w,
					                     u16 *frame_h)
{
	ui_image_selection *selectable_image = &ui->last_persistent_element->selectable_image;
	u32 selectable_group_overflow         = selectable_image->selectable_group_count >= selectable_image->selectable_group_max;

	if(!selectable_group_overflow)
	{
		struct ui_image_selection_group *selectable_group = selectable_image->selectable_group + selectable_image->selectable_group_count;

		u32 updated = selectable_image->uvsUpdated &&
			          selectable_image->selectable_group_selected == selectable_image->selectable_group_count;

		selectable_group->selection_type_uv = 0;
		//pick texture from the main struct
		render_texture *texture = selectable_image->texture;

	    if(updated)
	    {
	    	*frame_x = selectable_group->frame_x;
	    	*frame_y = selectable_group->frame_y;
	    	*frame_w = selectable_group->frame_w;
	    	*frame_h = selectable_group->frame_h;
	    }

	    selectable_group->frame_x = *frame_x;
	    selectable_group->frame_y = *frame_y;
	    selectable_group->frame_w = *frame_w;
	    selectable_group->frame_h = *frame_h;

	}
	selectable_image->selectable_group_count++;
	u32 element_interacting = 0;

	return(element_interacting);
}

inline void
ui_image_selection_end(game_ui *ui)
{
	ui_pop_persistent_element(ui);
}


static void
ui_timeline_begin(game_ui *ui, u8 *label)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_timeline);
	u32 timeline_id_hash = string_kinda_hash(label);
    ui_id expectedId    = ui_id_NUMBER(timeline_id_hash);

	ui_element_persistent *timelineElement = ui_push_or_get_persistent_element(ui, expectedId);
	ui->last_persistent_element = timelineElement;
	ui_timeline *timeline       = &timelineElement->timeline; 
	timeline->id = expectedId;

	element->persistent_element_array_index = timelineElement->index;

	//Set initial data
	//convert to ms for now

	if(!timeline->frame_group_max)
	{
		timeline->frame_group_max = 20;
	}
	if(!timeline->frame_group_key_max)
	{
		timeline->frame_group_key_max = 20;
	}
	if(!timeline->trackMax)
	{
		timeline->trackMax = 20;
	}
	if(!timeline->clip_max)
	{
		timeline->clip_max = 20;
	}
	//check for overflow
	if(timeline->frame_group_key_count >= timeline->frame_group_key_max)
	{
		timeline->frame_group_key_max = 1 + timeline->frame_group_key_count;
	}
	if(timeline->frame_group_count >= timeline->frame_group_max)
	{
		timeline->frame_group_max = 1 + timeline->frame_group_count;
	}
	//Clips
	if(timeline->clip_count >= timeline->clip_max)
	{
		timeline->clip_max = timeline->clip_count + 1;
	}
	//Tracks
	if(timeline->trackCount >= timeline->trackMax)
	{
		//Some extra space
		timeline->trackMax += timeline->trackCount + 1;
	}

	timeline->trackCount            = 0;
	timeline->clip_group_key_count  = 0;
	timeline->clip_count             = 0;
	timeline->frame_group_count     = 0;
	timeline->frame_group_key_count = 0;
	timeline->loop_visible = 0;
	timeline->loop = 0;
	timeline->lines_per_ms = 1;

	timeline->clips            = ui_push_array_to_command_buffer(ui, ui_timeline_clip, timeline->clip_max);
	timeline->tracks           = ui_push_array_to_command_buffer(ui, ui_timeline_track, timeline->trackMax);
	timeline->frame_groups     = ui_push_array_to_command_buffer(ui, ui_timeline_frame_group, timeline->frame_group_max);
	timeline->frame_group_keys = ui_push_array_to_command_buffer(ui, ui_timeline_frame_group_key, timeline->frame_group_key_max);

	u8 *data_start = (u8 *)timeline->clips;
	timeline->data = (u8 *)(timeline->frame_group_keys + timeline->frame_group_key_max);

	//get the total size used for the arrays
	timeline->dataOffset = (u32)(timeline->data - data_start);

}

inline void
ui_timeline_set_lines_per_ms(game_ui *ui,
		                     u32 lines_per_ms)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	if(lines_per_ms)
	{
		timeline->lines_per_ms = lines_per_ms;
	}
}

#define ui_timeline_reproduce_button_u32(ui, r_ptr) ui_timeline_reproduce_button(ui, (u8 *)r_ptr)
inline void
ui_timeline_reproduce_button(game_ui *ui,
		                     u8 *reproduce)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	if(timeline->reproduce_interacted)
	{
		*reproduce = !*reproduce;
	}
	timeline->reproducing = *(u8 *)reproduce;
}

inline void
ui_timeline_set_time(game_ui *ui,
		             f32 *time_at,
					 u32 *totalFrames)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;

	//Process interactions
	if(timeline->resizer_interacting)
	{
		*totalFrames = (u16)((timeline->time_total + 0.05f) / 0.1f);
	}
	if(timeline->timelineInteracting)
	{
	    *time_at = timeline->time_at;
	}
	timeline->time_at    = *time_at;
	timeline->time_total = (f32)(*totalFrames) * 0.1f;
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
	ui_timeline *timeline = &ui->last_persistent_element->timeline;

	timeline->loop = loop_active;
	timeline->loop_visible = 1;

	return(timeline->loop_interacting);
}
inline void
ui_timeline_set_loop_cursor_TYPE(f32)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	u32 interacting = _ui_timeline_set_loop_cursor(ui,
			                                       loop_active);
	if(timeline->loop_interacting)
	{
		*loop_start = timeline->loop_start;
		*loop_end   = timeline->loop_end;
	}
	timeline->loop_start = *loop_start;
	timeline->loop_end   = *loop_end;

}

inline void
ui_timeline_set_loop_cursor_TYPE(u32)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	u32 interacting = _ui_timeline_set_loop_cursor(ui,
			                                       loop_active);
	if(timeline->loop_interacting)
	{
		*loop_start = (u32)timeline->loop_start;
		*loop_end   = (u32)timeline->loop_end;
	}
	timeline->loop_start = (f32)*loop_start;
	timeline->loop_end   = (f32)*loop_end;

}

inline void
ui_timeline_set_loop_cursor_TYPE(u16)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	u32 interacting = _ui_timeline_set_loop_cursor(ui,
			                                       loop_active);
	if(timeline->loop_interacting)
	{
		*loop_start = (u16)timeline->loop_start;
		*loop_end   = (u16)timeline->loop_end;
	}
	timeline->loop_start = (f32)*loop_start;
	timeline->loop_end   = (f32)*loop_end;

}


static void
ui_timeline_end(game_ui *ui)
{
	//Get the pushed tracks
	ui_pop_persistent_element(ui);
}

inline u32
ui_timeline_cursor_interacted(game_ui *ui)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;

	return(timeline->timelineInteracting);
}

static u32
ui_timeline_addtrack(game_ui *ui, u8 *label)
{
	u32 success = 0;
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	Assert(timeline);
	//Don't process this track for this frame
	if(timeline->trackCount >= timeline->trackMax)
	{
	}
	else
	{
	   ui_timeline_track *timelineTrack = timeline->tracks + timeline->trackCount;
	   //The text should be allocated...
	   u32 textLength = string_count(label);
	   timelineTrack->label = ui_push_string_to_reserved(ui, label, textLength);
	   //Track got added!
	   success = 1;

	}
	timeline->trackCount++;

	return(success);

}

#define UI_TIMELINE_CLIP_GROUP_TYPE(type) static u32 ui_timeline_clip_group_ ##type(game_ui *ui, u32 selected, type *timeStart, type *timeDuration, u32 trackIndex)
static ui_timeline_clip *
_ui_timeline_clip(game_ui *ui, u32 selected)
{

	u32 interacted = 0;
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	Assert(timeline);
    ui_timeline_clip *timelineClip = 0;
	//Don't process this clip for this frame
	if(timeline->clip_count < timeline->clip_max)
	{
	   ui_id clip_id = ui_id_NUMBER_POINTER(ui->reservedIdValue++, timelineClip);

	   timelineClip = timeline->clips + timeline->clip_count;
	   timelineClip->selected = selected;
	   timelineClip->id       = clip_id;

	   if(timelineClip->selected)
	   {
		   timeline->clip_selected_index = timeline->clip_count;
	   }
	}
	timeline->clip_count++;
	return(timelineClip);

}

UI_TIMELINE_CLIP_GROUP_TYPE(f32)
{
    ui_timeline_clip *timelineClip = _ui_timeline_clip(ui, selected);
	u32 interacting = 0; 
	if(timelineClip)
	{
		interacting = ui_element_interacting_flags(ui, timelineClip->id, ui_interaction_mouse_left_down);
	   if(interacting)
	   {
	      *timeStart    = timelineClip->timeStart;
	      *timeDuration = timelineClip->timeDuration;
	   }
	   timelineClip->timeStart    = *timeStart; 
	   timelineClip->timeDuration = *timeDuration; 
	}
	return(interacting);
}

UI_TIMELINE_CLIP_GROUP_TYPE(u16)
{
    ui_timeline_clip *timelineClip = _ui_timeline_clip(ui, selected);
	u32 interacting = 0;
	if(timelineClip)
	{
		interacting = ui_element_interacting_flags(ui, timelineClip->id, ui_interaction_mouse_left_down);
	   if(interacting)
	   {
	      *timeStart    = (u16)(timelineClip->timeStart * 10);
	      *timeDuration = (u16)(timelineClip->timeDuration * 10);
	   }
	   timelineClip->timeStart    = (f32)*timeStart * .1f; 
	   timelineClip->timeDuration = (f32)*timeDuration * 0.1f; 
	}
	return(interacting);
}

static u32
ui_timeline_add_frame_group(game_ui *ui,
		                    u32 selected,
							u32 target_frame)
{
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	Assert(timeline);

	u32 interacting = 0;
	if(timeline->frame_group_count < timeline->frame_group_max)
	{
		ui_timeline_frame_group *new_frame_group = timeline->frame_groups + timeline->frame_group_count;
		ui_id frame_group_id = ui_id_NUMBER_POINTER(timeline->frame_group_count, new_frame_group);

		new_frame_group->id                      = frame_group_id;
		new_frame_group->keyframe_count          = 0;
		new_frame_group->keyframe_rendered_count = 0;
		new_frame_group->frame_start    = target_frame;
		new_frame_group->selected       = selected;

		interacting = ui_element_interacting(ui, frame_group_id);
	}
	timeline->frame_group_count++;

	return(interacting);
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
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	Assert(timeline);
	//
	u32 frame_group_count = timeline->frame_group_count < timeline->frame_group_max ?
		                    timeline->frame_group_count : timeline->frame_group_max;

	u32 needs_another_track = 0;
	u32 index_is_avadible = frame_group_index < frame_group_count;
	u32 track_is_avadible = 1;
	if(track_index >= timeline->trackCount)
	{
		track_is_avadible = ui_timeline_addtrack(ui, " ");
	}

    ui_timeline_frame_group_key *new_frame_group_key = 0;

	u32 interacting = 0;
	if(index_is_avadible && track_is_avadible)
	{
		if(timeline->frame_group_key_count < timeline->frame_group_key_max)
		{

			new_frame_group_key      = timeline->frame_group_keys + timeline->frame_group_key_count;
			ui_id frame_group_key_id = ui_id_NUMBER_POINTER(timeline->frame_group_key_count, new_frame_group_key);

		    new_frame_group_key->id                = frame_group_key_id;
			new_frame_group_key->frame_group_index = frame_group_index;
		    new_frame_group_key->selected          = selected;
			new_frame_group_key->frame_duration    = frame_duration;
			new_frame_group_key->clipIndex = track_index;
			new_frame_group_key->type = type;

	        interacting = ui_element_interacting_flags(ui, frame_group_key_id, ui_interaction_mouse_left_down);
            
	        u32 label_length = string_count(label);
		    new_frame_group_key->label = ui_push_string_to_reserved(ui, label, label_length);

		}
	}
	timeline->frame_group_key_count++;

	return(interacting);
}

static u32
ui_timeline_add_clip_group_key(game_ui *ui,
		                   u32 selected,
						   u32 clipIndex,
						   u8 *label)
{
#if 0
	ui_timeline *timeline = &ui->last_persistent_element->timeline;
	Assert(timeline);
	//
	u32 index_is_avadible = clipIndex < timeline->clip_count;
    ui_timeline_clip_group_key *trackKey = 0;

	u32 interacting = 0;
	if(index_is_avadible)
	{
		trackKey = ui_push_size_to_command_buffer(ui, sizeof(ui_timeline_clip_group_key));
		//Reset to avoid false interaction.
		trackKey->id = ui_id_NUMBER_POINTER(timeline->clip_group_key_count, trackKey);
		//Set where to render this key
		trackKey->clipIndex = clipIndex;
		trackKey->selected = selected;

	    u32 textLength = string_count(label);
		trackKey->label = ui_push_string_to_reserved(ui, label, textLength);

		timeline->clip_group_key_count++;
	    interacting = ui_element_interacting_flags(ui, trackKey->id, ui_interaction_mouse_left_down);
	}
	return(interacting);
#endif
	return(0);
}


static void
ui_input_text(game_ui *ui,
			  u32 textLimit,
			  uint8 *textToModify)
{
    ui_id inputTextId = UIIDFROMPTR(ui->input_text, textToModify);
	ui_element *element = ui_PushElement(ui, inputTextId, ui_command_type_inputtext);

	element->input.dest      = textToModify;
	element->input.textLimit = textLimit;
}

//
//ui push commands__
//

#define PANELBACKCOLOR V4(13, 17, 60, 0xe1)


inline void
ui_open_or_close_next_panel(game_ui *ui)
{
	ui->switchPanelClose = 1;
}

inline void
ui_start_next_panel_closed(game_ui *ui)
{
	ui->initializePanelClosed = 1;
}

inline void
ui_start_next_panel_minimized(game_ui *ui)
{
	ui->initializePanelMinimized = 1;
}

inline void
ui_force_next_panel_focus(game_ui *ui)
{
	ui->forcePanelFocus = 1;
}

inline void
ui_set_next_panel_position(game_ui *ui, f32 x, f32 y)
{
	ui->next_panel_x = x;
	ui->next_panel_y = y;
	ui->setPanelPosition = 1;
}

inline void
ui_set_next_panel_size(game_ui *ui, f32 w, f32 h)
{
	ui->next_panel_w = w;
	ui->next_panel_h = h;
	ui->setPanelSize = 1;
}

//Initializes main panel
static void
ui_tab_group_begin(game_ui *ui, u32 flags, u8 *label)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_tab_group);

   u32 tab_group_label_hash = string_kinda_hash(label);
   ui_id tab_group_id       = ui_id_NUMBER(tab_group_label_hash);

   ui_element_persistent *tab_group_element = ui_push_or_get_persistent_element(ui, tab_group_id);
   element->persistent_element_array_index        = tab_group_element->index;

   ui_tab_group *group    = &tab_group_element->tab_group;
   if(!group->tabs_max)
   {
	   group->tabs_max = 3;
   }
   if(group->tabs_count >= group->tabs_max)
   {
	   group->tabs_max = group->tabs_count + 1;
   }
   group->tabs_count = 0;
   group->tabs      = ui_push_array_to_command_buffer(ui, ui_tab_group_tab, group->tabs_max);

   group->commands_base   = ui->commands_base;
   group->commands_offset = ui->commands_base;
}

static void
ui_tab_group_end(game_ui *ui)
{
   //ui_element *element = ui_push_command(ui, ui_element, ui_command_type_tab_group);
    ui_tab_group *group    = &ui->last_persistent_element->tab_group;
	group->commands_offset = ui->commands_offset;

	//set the final tab commands offset
    if(group->tabs_count)
    {
        ui_tab_group_tab *previous_tab = group->tabs + (group->tabs_count - 1);
        previous_tab->commands_offset = ui->commands_offset;

		//set valid active tab if any was pushed
		if(group->active_tab_index >= group->tabs_count)
		{
			group->active_tab_index = group->tabs_count - 1;
		}
    }
	ui_pop_persistent_element(ui);
}

static u32 
ui_tab_push(game_ui *ui, u8 *title)
{
   ui_tab_group *group = &ui->last_persistent_element->tab_group;
   u32 active_tab_index = 0;
   //check for tab overflow
   if(group->tabs_count < group->tabs_max)
   {
       //pick the previous tab and set its commands offset
       if(group->tabs_count)
       {
           ui_tab_group_tab *previous_tab = group->tabs + (group->tabs_count - 1);
           previous_tab->commands_offset = ui->commands_offset;
       }
       //set commands for this tab
       ui_tab_group_tab *current_tab = group->tabs + group->tabs_count;
       current_tab->commands_base     = ui->commands_offset;
       current_tab->commands_offset   = ui->commands_offset;
       current_tab->title             = title;

	   active_tab_index = group->active_tab_index == group->tabs_count;
   }

   group->tabs_count++;
   return(active_tab_index);

}

static u32 
ui_selectable_tab(game_ui *ui, u32 active, u8 *title)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_tab);
   ui_id tab_id = UIIDFROMPTR(title, title);
   element->id	= tab_id;


   u32 interacted = ui_element_interacted_flags(ui, tab_id, ui_interaction_mouse_left_down);
   return(interacted);

}

static void
ui_tab_end(game_ui *ui)
{
	ui_PopPanel(ui);
}

inline u32 
_ui_panel_begin(game_ui *ui,
		       ui_panel_flags panel_flags,
			   f32 panel_x,
			   f32 panel_y,
			   f32 panel_w,
			   f32 panel_h,
			   bool32 *open_close_ptr,
			   u8 *title)
{
	//ui_id panelId   = UIIDFROMPTR(title, title);
	u32 titleLength = string_count(title);
#if 0
	u32 kindaHash = 0;
	for(u32 t = 0; t < titleLength; t++)
	{
		u32 c = (u32)title[t];
		kindaHash += noise1D_U32(c, titleLength);
	}
#endif
	//ui_id panelId   = ui_id_NUMBER_POINTER((u8)(title + titleLength), title);
	ui_id panelId   = ui_id_NUMBER(string_kinda_hash(title));
	ui_panel *panel = ui_PushPanelFromStack(ui, panel_flags, panelId);

	i32 notInitialized = !ui_id_EQUALS(panel->id, panelId);
	//panel->id			= panelId;
	panel->title		= title;
    ui_InitializePanel(ui, panel, panelId, panel_x, panel_y, panel_w, panel_h);
				      
	u32 is_opened_and_visible = !panel->closed && !panel->notVisible;
	if(open_close_ptr)
	{
		panel->close_button = 1;
		if(panel->closed)
		{
			*open_close_ptr = 0; 
			panel->closed = 0;
		}
	}
	else
	{
		panel->closed = 0;
		panel->close_button = 0;
	}

	if(!(panel_flags & ui_panel_flags_move))
	{
		panel->p.x = panel_x;
		panel->p.y = panel_y;
	}
	if(!(panel_flags & ui_panel_flags_resize))
	{
		panel->sz.x = panel_w;
		panel->sz.y = panel_h;
	}

	if(ui->initializePanelClosed)
	{
		ui->initializePanelClosed = 0;

	   if(notInitialized)
	   {
		  panel->closed = 1;
	   }
	}
    if(ui->initializePanelMinimized)
	{
		ui->initializePanelMinimized = 0;

	   if(notInitialized)
	   {
		  panel->notVisible = 1;
	   }
	}
	if(ui->switchPanelClose)
	{
		ui->switchPanelClose = 0;
		panel->closed = !panel->closed;
	}
	if(ui->forcePanelFocus)
	{
		ui->forcePanelFocus = 0;
		ui->focused_panel = panel;
	}

	if(ui->setPanelPosition)
	{
		ui->setPanelPosition = 0;
		panel->p.x = ui->next_panel_x;
		panel->p.y = ui->next_panel_y;
	}
	if(ui->setPanelSize)
	{
		ui->setPanelSize = 0;
		panel->sz.x = ui->next_panel_w;
		panel->sz.y = ui->next_panel_h;
	}

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
		_ui_panel_begin(ui,
				panel_flags,
				panel_x,
				panel_y,
				panel_w,
				panel_h,
				0,
				title);
	return(is_opened_and_visible);
}

static inline u32
ui_panel_begin_close_button(game_ui *ui,
		       ui_panel_flags panel_flags,
			   f32 panel_x,
			   f32 panel_y,
			   f32 panel_w,
			   f32 panel_h,
			   bool32 *open_close_ptr,
			   u8 *title)
{
	u32 is_opened_and_visible =
		_ui_panel_begin(ui,
				panel_flags,
				panel_x,
				panel_y,
				panel_w,
				panel_h,
				open_close_ptr,
				title);
	return(is_opened_and_visible);
}

inline void
ui_layout_group_begin(game_ui *ui)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_group_set);
   element->layout_group.begin = 1;
}

inline void
ui_layout_group_end(game_ui *ui)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_group_set);
   element->layout_group.begin = 0;
}

//This probably should get a panel from the panel stack instead of allocating one on the command buffer since
//it will most likely still get the same id even after being  moved a bit...
#define ui_begin_child_panel(ui, w, h, text) ui_begin_child_panel_flags(ui, ui_panel_flags_scroll_v, w, h, text)
inline void
ui_begin_child_panel_flags(game_ui *ui,
		                   ui_panel_flags flags,
						   f32 w,
						   f32 h,
						   u8 *text)
{
   ui_element *element = ui_push_command(ui, ui_element, ui_command_type_childpanel);
   ui_id childPanelId  = UIIDFROMPTR(text, text);
   u32 reset = !ui_id_EQUALS(childPanelId, element->id);
   element->id		   = UIIDFROMPTR(text, text);

   //Push buffer instead ?
	ui_id panelId   = UIIDFROMPTR(text, element);
   element->child_panel.panel = ui_PushPanelToCommandBuffer(ui, ui_panel_flags_borders | ui_panel_flags_child | flags, panelId); 
   element->child_panel.panel->notVisible = 0;
   element->child_panel.panel->title = text;
   ui_InitializePanel(ui, element->child_panel.panel, panelId, 0, 0, w, h); 

   element->child_panel.panel->sz.x = w;
   element->child_panel.panel->sz.y = h;
}
#if 0
static void
ui_BeginTopPanel(game_ui *ui, panel_flags, f32 pX, f32 pY, f32 pW, f32 pH)
{

	ui_id panelId   = UIIDFROMPTR(label, label);
	ui_panel *panel = ui_PushPanelFromStack(ui, panel_flags, panelId);

	//panel->id			= panelId;
	panel->title		= label;
    ui_InitializePanel(ui, panel, panelId, pX, pY, pW, pH);
}
#endif

#define ui_EndTopPanel(ui) ui_panel_end(ui)
#define ui_end_child_panel(ui) ui_panel_end(ui)
inline void
ui_panel_end(game_ui *ui)
{
	ui_PopPanel(ui);
}



//
//explorer specific
//

static void
ui_run_explorer(game_ui *ui,
		bool32 *open_close_ptr)
{

	ui_explorer *explorer = ui->explorer;
	//u32 kindaHash = string_kinda_hash(id);
	//ui->explorer->process_id = kindaHash;
    explorer->file_got_selected = 0;

	//Special flags for explorer
	ui_panel_flags panel_flags = 
		(ui_panel_flags_title | 
		 ui_panel_flags_move | 
		 ui_panel_flags_close | 
		 ui_panel_flags_size_to_content |
		 ui_panel_flags_focus_when_opened);

	ui_id panelId   = ui_id_NUMBER_POINTER("_Explorer", ui->explorer);
	ui_panel *panel = ui_PushPanelFromStack(ui, panel_flags, panelId);

	u32 expectedPanel = ui_id_EQUALS(panel->id, panelId);
	//Panel was not previously on the stack
	if(!expectedPanel)
	{
	   explorer->panel = panel;
	   panel->closed = 1;
	   panel->title  = "No process";
	   explorer->closed = 1;
	}
	if(panel->closed && !explorer->started_process)
	{
		explorer->closed = 1;
	}
	if(open_close_ptr)
	{
		panel->close_button = 1;
		panel->closed = *open_close_ptr == 0;
	}
	else
	{
		panel->closed = 0;
	}
	explorer->started_process = 0;

	panel->was_closed = (panel->closed && !explorer->closed);
	panel->closed = explorer->closed;
	panel->title = explorer->process_name;

    ui_InitializePanel(ui, panel, panelId, 60, 60, 60, 60);

	//push command to it's panel
	if(!panel->closed)
	{
	    ui_push_command(ui, ui_command_type, ui_command_type_explorer);
	}
	ui_panel_end(ui);
}

//inline void
//ui_resource_tree_list_begin(
//		game_ui *ui,
//		ppse_resources_file_and_folders *resources,
//		u8 *label)
//{
//	u32 label_hash = string_kinda_hash(label);
//	ui_id persistent_element_id = ui_id_NUMBER(label_hash);
//
//	//get persistent element
//	ui_element_persistent *element_resource_explorer = ui_push_or_get_persistent_element(ui, persistent_element_id);
//	element_resource_explorer->resource_tree_explorer.resources = resources;
//
//	//push command
//	ui_element_persistent_index *element    = ui_push_command(ui, ui_element_persistent_index, ui_command_type_resource_explorer);
//	element->persistent_element_array_index = element_resource_explorer->index;
//
//	
//}
//
//inline void
//ui_resource_tree_list_end(game_ui *ui)
//{
//	ui_pop_persistent_element(ui);
//}

inline void
ui_explorer_clear_current_file_name(game_ui *ui)
{
	string_clear(ui->explorer->selected_file_path_and_name);
	string_clear(ui->explorer->process_file_name);
}

static void 
ui_explorer_set_process(
		game_ui *ui,
		ui_explorer_process_type process_type,
		u8 *id)
{
	ui_explorer *explorer = ui->explorer;

	u32 kindaHash = string_kinda_hash(id);
	//explorer->panel->title = id;

	if(explorer->process_id != kindaHash)
	{
	    explorer->closed = 0;
	    string_copy(id, explorer->process_name);
	    //ui->focused_panel = explorer->panel;
	}
	else
	{
		explorer->closed = !explorer->closed;
		memory_clear(explorer->search_pattern, 24);
		explorer->search_pattern[0] = '*';
	}

	explorer->started_process = 1;
	explorer->process_type     = process_type;
	explorer->process_id       = kindaHash;
	explorer->files_focused   = 1;

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

static void 
ui_explorer_set_flags_and_process(game_ui *ui,
                        	  ui_explorer_process_type process_type,
							  ui_explorer_flags explorerFlags,
							  u8 *id)
{
	ui->explorer->flags = explorerFlags;
	ui_explorer_set_process(ui, process_type, id);
}

inline u32 
ui_explorer_CompletedSave(game_ui *ui)
{
	u32 completed = 0;
	if(ui->explorer->okay_pressed)
	{
       completed = path_IsValidFileName(ui->explorer->process_file_name);
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
	    success   = ui->explorer->last_process_completed && ui->explorer->process_id == kindaHash;
		if(success)
		{
			//Reset process result
			explorer->last_process_completed = 0;
		}
	}

	return(success);
}

//
//
//


static u32 
ui_begin_drop_down(game_ui *ui, u8 *text, u8 *preview)
{

	u32 active = 1;

	ui_layout *layout   = ui->currentLayout;
	ui_element *element = ui_push_command(ui, ui_element , ui_command_type_drop_down);

	ui_id dropDownId    = ui_id_NUMBER_POINTER(noise1D_U32(ui->reservedIdValue++, (u32)(u64)text), text);

	u32 textLength = string_count(preview);
	element->label = ui_push_string_to_reserved(ui, preview, textLength);
	element->id	   = dropDownId;
	if(ui_element_interacted_flags(ui, element->id, ui_interaction_mouse_left_down))
	{
		ui->last_interacted_drop_down = element->id;
		//ui->activeDropDown;
	}
		//element->drop_down.active = 0;

	//Use a panel to push commands onto it.
	ui_id panelId = UIIDFROMPTR(text, text);
    element->drop_down.panel = ui_PushPanelToCommandBuffer(ui, ui_panel_flags_child | ui_panel_flags_scroll_v, panelId);
    element->drop_down.panel->notVisible = 0;
    ui_InitializePanel(ui, element->drop_down.panel, 
					   panelId, 0, 0, element->drop_down.size.x, element->drop_down.size.y); 
	return(active);

}

#define QUICKFORMAT(buffer, text) \
	va_list args;\
	va_start_m(args, preview);\
	FormatTextList(buffer, sizeof(buffer), text, args);\
	va_end_m(args);

inline u32 
ui_begin_drop_downf(game_ui *ui, u8 *text, u8 *preview, ...)
{

   u8 textBuffer[256] = {0};
   QUICKFORMAT(textBuffer, preview);
   //va_list args;
   //va_start_m(args, preview);
   //FormatTextList(textBuffer, sizeof(textBuffer), preview, args);
   //va_end_m(args);

   u32 active = ui_begin_drop_down(ui, text, textBuffer);
   return(active);
}

static void
ui_end_drop_down(game_ui *ui)
{
	ui_PopPanel(ui);
}

static u32
ui_begin_drop_panel(game_ui *ui,
		            u32 flags,
					u8 *label)
{
	u32 opened = 1;

	return(opened);
}

inline void
ui_end_drop_panel(game_ui *ui)
{
}

#define _SPINNER_INC_MAXEQ(value, inc, max, type) *(type *)value += inc; *(type *)value = MINEQ(*(type *)value ,max)
#define _SPINNER_DEC_MINEQ(value, dec, min, type) *(type *)value -= dec; *(type *)value = MAXEQ(*(type *)value ,min)

#define ui_spinner_TYPE(type) \
ui_spinner_ ##type(\
		game_ui *ui,\
		type IncDec,\
		type minValue,\
		type maxValue,\
		type *value,\
		ui_input_text_flags textInputFlags,\
		uint8 *label)

static ui_element * 
_ui_spinner(game_ui *ui, uint8 *label, void *value, ui_input_text_flags textInputFlags, ui_value_type valueType)
{
	//ui_element *element       = ui_push_command(ui, ui_element, ui_command_type_updown);
	ui_id updownHId    = ui_id_POINTERS(label, value);
	ui_id incrementId  = ui_id_POINTERS(label, (u8 *)value + 1);
	ui_id decrementId  = ui_id_POINTERS(label, (u8 *)value + 2);
	ui_id input_id      = ui_id_POINTERS(label, (u8 *)value + 3);

	ui_element *element            = ui_PushElement(ui, updownHId, ui_command_type_updown);
	element->id                    = updownHId; 
	element->updown.incrementId    = incrementId; 
	element->updown.decrementId    = decrementId; 
	element->updown.input_id        = input_id; 
	element->updown.valueType      = valueType;
	element->updown.textInputFlags = textInputFlags;
	return(element);
}
static i32
ui_spinner_TYPE(i32)
{
	ui_element *element     = _ui_spinner(ui, label, value, textInputFlags, ui_value_i32);


	u16 increase     = ui_element_interacted_flags(ui, element->updown.incrementId, ui_interaction_mouse_left_down);
	u16 decrease     = ui_element_interacted_flags(ui, element->updown.decrementId, ui_interaction_mouse_left_down);
	u32 inputUpdated = ui_element_interacting(ui, element->updown.input_id);
	u32 inputHovered = ui_element_hot(ui, element->updown.input_id);

	increase |= inputHovered && ui->input->mouse_wheel > 0;
	decrease |= inputHovered && ui->input->mouse_wheel < 0;

	if(increase && !i32_Add_OVERFLOWS(*value, IncDec))
	{
	   _SPINNER_INC_MAXEQ(value, IncDec, maxValue, i32);
	}
    else if(decrease && !i32_Sub_UNDERFLOWS(*value, IncDec))
	{
	   _SPINNER_DEC_MINEQ(value, IncDec, minValue, i32);
	}
	else if(inputUpdated && IncDec)
	{
		u32 confirm_on_enter = textInputFlags & ui_text_input_confirm_on_enter;

		i32 finalValue = *value;
		if(confirm_on_enter)
		{
           if(ui->input_text_entered)
		   {
		      i32_from_string(ui->inputTextBuffer, &finalValue);
		   }
		}
		else
		{
			if(ui->inputTextBuffer[0] != '-')
			{
				int s = 0;
			}


		   i32_from_string(ui->inputTextBuffer, &finalValue);
		}


		finalValue = IncDec * (finalValue / IncDec);
        *value = finalValue <= minValue ? minValue : finalValue >= maxValue ? maxValue : finalValue;
	}

	element->updown.value_i32 = *value;

	return(increase || decrease || inputUpdated);
}

static i32
ui_spinner_TYPE(u32)
{
	ui_element *element     = _ui_spinner(ui, label, value, textInputFlags, ui_value_u32);


	u16 increase     = ui_element_interacted_flags(ui, element->updown.incrementId, ui_interaction_mouse_left_down);
	u16 decrease     = ui_element_interacted_flags(ui, element->updown.decrementId, ui_interaction_mouse_left_down);
	u32 inputUpdated = ui_element_interacting(ui, element->updown.input_id);
	u32 inputHovered = ui_element_hot(ui, element->updown.input_id);

	increase |= inputHovered && ui->input->mouse_wheel > 0;
	decrease |= inputHovered && ui->input->mouse_wheel < 0;

	if(increase && !u32_Add_OVERFLOWS(*value, IncDec))
	{
	   _SPINNER_INC_MAXEQ(value, IncDec, maxValue, u32);
	}
    else if(decrease && !u32_Sub_UNDERFLOWS(*value, IncDec))
	{
	   _SPINNER_DEC_MINEQ(value, IncDec, minValue, u32);
	}
	else if(inputUpdated && IncDec)
	{
		u32 confirm_on_enter = textInputFlags & ui_text_input_confirm_on_enter;

		u32 finalValue = *value;
		if(confirm_on_enter)
		{
           if(ui->input_text_entered)
		   {
		      u32_from_string(ui->inputTextBuffer, &finalValue);
		   }
		}
		else
		{
		   u32_from_string(ui->inputTextBuffer, &finalValue);
		}


		finalValue = IncDec * (finalValue / IncDec);
        *value = finalValue <= minValue ? minValue : finalValue >= maxValue ? maxValue : finalValue;
	}

	element->updown.value_u32 = *value;

	return(increase || decrease || inputUpdated);
}

static i32
ui_spinner_TYPE(u16)
{
	ui_element *element = _ui_spinner(ui, label, value, textInputFlags, ui_value_u16);

	u16 increase     = ui_element_interacted_flags(ui, element->updown.incrementId, ui_interaction_mouse_left_down);
	u16 decrease     = ui_element_interacted_flags(ui, element->updown.decrementId, ui_interaction_mouse_left_down);
	u32 inputUpdated = ui_element_interacting(ui, element->updown.input_id);
	u32 inputHovered = ui_element_hot(ui, element->updown.input_id);

	increase |= inputHovered && ui->input->mouse_wheel > 0;
	decrease |= inputHovered && ui->input->mouse_wheel < 0;

	if(increase && !u16_Add_OVERFLOWS(*value, IncDec))
	{
	   _SPINNER_INC_MAXEQ(value, IncDec, maxValue, u16);
	}
    else if(decrease && !u16_Sub_UNDERFLOWS(*value, IncDec))
	{
	   _SPINNER_DEC_MINEQ(value, IncDec, minValue, u16);
	}
	else if(inputUpdated && IncDec)
	{
		u32 confirm_on_enter = textInputFlags & ui_text_input_confirm_on_enter;

		u32 value32    = *value;
		if(confirm_on_enter)
		{
           if(ui->input_text_entered)
		   {
		      u32_from_string(ui->inputTextBuffer, &value32);
		   }
		}
		else
		{
		   u32_from_string(ui->inputTextBuffer, &value32);
		}
		u16 finalValue = (u16)value32;


		finalValue = IncDec * (finalValue / IncDec);
        *value = finalValue <= minValue ? minValue : finalValue >= maxValue ? maxValue : finalValue;
	}
	element->updown.value_u16 = *value;

	return(increase || decrease || inputUpdated);
}

static i32
ui_spinner_TYPE(f32)
{
	ui_element *element = _ui_spinner(ui, label, value, textInputFlags, ui_value_f32);

	u16 increase     = ui_element_interacted_flags(ui, element->updown.incrementId, ui_interaction_mouse_left_down);
	u16 decrease     = ui_element_interacted_flags(ui, element->updown.decrementId, ui_interaction_mouse_left_down);
	u32 inputUpdated = ui_element_interacting(ui, element->updown.input_id);
	u32 inputHovered = ui_element_hot(ui, element->updown.input_id);

	increase |= inputHovered && ui->input->mouse_wheel > 0;
	decrease |= inputHovered && ui->input->mouse_wheel < 0;

	if(increase && !f32_Add_OVERFLOWS(*value, IncDec))
	{
	   _SPINNER_INC_MAXEQ(value, IncDec, maxValue, f32);
	}
    else if(decrease && !f32_Sub_UNDERFLOWS(*value, IncDec))
	{
	   _SPINNER_DEC_MINEQ(value, IncDec, minValue, f32);
	}
	else if(inputUpdated && IncDec)
	{
		u32 confirm_on_enter = textInputFlags & ui_text_input_confirm_on_enter;

		f32 finalValue = *value;
		if(confirm_on_enter)
		{
           if(ui->input_text_entered)
		   {
		      f32_from_string(ui->inputTextBuffer, &finalValue);
		   }
		}
		else
		{
		   f32_from_string(ui->inputTextBuffer, &finalValue);
		}


		finalValue = IncDec * (finalValue / IncDec);
        *value = finalValue <= minValue ? minValue : finalValue >= maxValue ? maxValue : finalValue;
	}
	element->updown.value_f32 = *value;

	return(increase || decrease || inputUpdated);
}

static void
ui_LogConsole(game_ui *ui, s_game_console *log_console, u8 *label)
{
	u32 console_hash_id = string_kinda_hash(label);
	ui_element_persistent *consoleElement = ui_push_or_get_persistent_element(ui, ui_id_NUMBER(console_hash_id));

	ui_element *element          = ui_push_command(ui, ui_element, ui_command_type_console_log);
	element->persistent_element_array_index = consoleElement->index;

	consoleElement->console.buffer      = log_console->buffer; 
	consoleElement->console.gameConsole = log_console; 
}

inline void
ui_set_text_color(game_ui *ui, f32 r, f32 g, f32 b, f32 a)
{
	ui_element *element = ui_push_command(ui,
			                              ui_element,
										  ui_command_type_set_text_color);
	element->text_color.color = V4(r, g, b, a);
}

inline void
ui_reset_text_color(game_ui *ui)
{
	ui_element *element = ui_push_command(ui, ui_element, ui_command_type_set_text_color);
	element->text_color.color = ui_theme_default_text;
}


//
//ui push commands__
//ui elements___


//
//Post functionsReserved
//

static void 
ui_render_interactive(game_ui *ui,
			    		    ui_id uiId,
							ui_interaction_flags interactionFlags,
						    f32 x,
							f32 y,
							f32 w,
							f32 h,
			    		    vec4 elementNormalColor,
			    		    vec4 element_hotColor,
			    		    vec4 element_interactingColor)
{
	i32 element_interacting = ui_element_interacting_flags(ui, uiId, interactionFlags);
	i32 elementHovered	   = ui_element_hot(ui, uiId); 
	u32 elementDisabled    = ui_disabled(ui);

    //if interacting
    vec4 color = elementNormalColor; 
	//Clicked
    if(element_interacting)
    {
        color = element_interactingColor;
    }
    else if(elementHovered)
    {
        color = element_hotColor;
    }

	f32 x0 = x;
	f32 y0 = y; 
	f32 x1 = w;
	f32 y1 = h;

	//Frame
	//render_rectangle_borders_2D(ui->renderCommands, x0, y0, x1, y1, 1, ui->theme.frameBorderColor);
	//background
	if(elementDisabled)
	{
		color.w = 120;
	}
    render_draw_rectangle_2D(ui->renderCommands, x0, y0, x1, y1, color);
	//ui_advance(ui,x0, y0, x1, y1); 
}

static void
ui_render_interactive_text(game_ui *ui,
		                   ui_id id,
						   ui_interaction_flags interactionFlags,
						   f32 x,
						   f32 y,
						   f32 w,
						   f32 h,
						   vec4 elementNormalColor,
						   vec4 element_hotColor,
						   vec4 element_interactingColor,
						   font_text_pad padOption,
						   u8 *text)
{
	//Background
	ui_render_interactive(ui, id, ui_interaction_mouse_left_down, x, y, w, h, elementNormalColor, element_hotColor, element_interactingColor);
	//Text
	vec2 textOffset = ui_get_text_padded_offset_vec2(ui, V2(w, h), text, padOption);
	vec4 textColor = vec4_all(255);
	if(ui_disabled(ui))
	{
		textColor.w = ui_DISABLED_ALPHA;
	}
	render_text_2d_All(ui->renderCommands,
			      &ui->fontp,
				  x + textOffset.x,
				  y + textOffset.y,
				  F32MAX,
				  F32MAX,
				  ui->font_scale,
				  textColor,
				  text);
}

static void
ui_render_selectable(game_ui *ui,
		                   ui_id id,
						   u32 is_active,
						   ui_interaction_flags interactionFlags,
						   f32 x,
						   f32 y,
						   f32 w,
						   f32 h,
						   vec4 element_active_color,
						   vec4 element_inactive_color,
						   vec4 element_hot_color,
						   vec4 element_interacting_color)
{

	vec4 active_or_inactive_color = element_inactive_color;
	if(is_active)
	{
		active_or_inactive_color = element_active_color;
	}
    ui_render_interactive(ui,
		                  id,
						  interactionFlags,
						  x,
						  y,
						  w,
						  h,
						  active_or_inactive_color,
						  element_hot_color,
						  element_interacting_color);
}

static void
ui_render_selectable_text(game_ui *ui,
		                   ui_id id,
						   u32 is_active,
						   ui_interaction_flags interactionFlags,
						   f32 x,
						   f32 y,
						   f32 w,
						   f32 h,
						   vec4 element_active_color,
						   vec4 element_inactive_color,
						   vec4 element_hot_color,
						   vec4 element_interacting_color,
						   font_text_pad padOption,
						   u8 *text)
{

	vec4 active_or_inactive_color = element_inactive_color;
	if(is_active)
	{
		active_or_inactive_color = element_active_color;
	}
    ui_render_interactive_text(ui,
		                       id,
						       interactionFlags,
						       x,
						       y,
						       w,
						       h,
							   active_or_inactive_color,
						       element_hot_color,
						       element_interacting_color,
						       padOption,
						       text);
}

static void
ui_update_render_button_label_at(
		game_ui *ui,
		ui_id id, f32 x, f32 y, f32 w, f32 h, font_text_pad padOptions, u8 *text)
{
	vec4 button_normal_color		= ui->theme.button_normal_color;
	vec4 button_hot_color			= ui->theme.button_hot_color;
	vec4 button_interacting_color = ui->theme.button_interacting_color;

	ui_create_update_element_at(ui, id, x, y, w, h);
	ui_render_interactive_text(ui, id, ui_interaction_mouse_left_down, x, y, w, h, button_normal_color, button_hot_color, button_interacting_color, padOptions, text);
}

static void
ui_UpdateAndRenderButtonAt(game_ui *ui, ui_id id, f32 x, f32 y, f32 w, f32 h)
{
	vec4 button_normal_color		= ui->theme.button_normal_color;
	vec4 button_hot_color			= ui->theme.button_hot_color;
	vec4 button_interacting_color = ui->theme.button_interacting_color;

	ui_create_update_element_at(ui, id, x, y, w, h);
	ui_render_interactive(ui, id, ui_interaction_mouse_left_down, x, y, w, h, button_normal_color, button_hot_color, button_interacting_color);
}

static void
ui_UpdateAndRenderButtonTextAtLayout(game_ui *ui, ui_id id, f32 w, f32 h, font_text_pad padOptions, u8 *text)
{
	vec2 layoutCursor = ui_get_layout_cursor(ui);
    ui_update_render_button_label_at(ui, id, layoutCursor.x, layoutCursor.y, w, h, padOptions, text);
	//Advance on layout
	ui_advance(ui, layoutCursor.x, layoutCursor.y, w, h);
}

static void
ui_update_render_button_text_sized_at_layout(game_ui *ui,
		                                     ui_id id,
											 font_text_pad padOptions,
											 u8 *text)
{
	f32 defaultTextOffset = 4;
	vec2 textSz  = font_get_text_size_wrapped_scaled(&ui->fontp, F32MAX, text, ui->font_scale);
    ui_UpdateAndRenderButtonTextAtLayout(ui,
			                             id,
										 textSz.x + defaultTextOffset,
										 textSz.y + defaultTextOffset, padOptions, text);
}


static void
ui_UpdateAndRenderButtonAtLayout(game_ui *ui, ui_id id, f32 w, f32 h)
{
	vec2 cursorLayout = ui_get_layout_cursor(ui);
    ui_UpdateAndRenderButtonAt(ui, id, cursorLayout.x, cursorLayout.y, w, h);
	ui_advance(ui, cursorLayout.x, cursorLayout.y, w, h);
}

inline void
ui_UpdateKeepCursorPush(game_ui *ui)
{
	ui->currentLayout->keepCursor = 1;
}

inline void
ui_UpdateKeepCursorPop(game_ui *ui)
{
	ui_layout *currentLayout = ui->currentLayout;
	currentLayout->cursorX    = currentLayout->cornerX;
	currentLayout->cursorY	  = currentLayout->cursorY + currentLayout->nextYDelta + currentLayout->spacingY;
	currentLayout->nextYDelta = 0;
	ui->currentLayout->keepCursor = 0;
}

inline void
ui_UpdateKeepLinePush(game_ui *ui)
{
	ui_layout *currentLayout = ui->currentLayout;
	ui->currentLayout->keepLine  = 1;
}

inline void
ui_UpdateKeepLinePop(game_ui *ui)
{
	ui_layout *currentLayout = ui->currentLayout;
	currentLayout->cursorX    = currentLayout->cornerX;
	currentLayout->cursorY	  = currentLayout->cursorY + currentLayout->nextYDelta + currentLayout->spacingY;
	currentLayout->nextYDelta = 0;
	currentLayout->keepLine   = 0;
}

inline void
ui_UpdatePushCursorToScreen(game_ui *ui, f32 cornerX, f32 cornerY, f32 cornerEndX, f32 cornerEndY)
{
	f32 cursorOffsetY = 0;

	ui_push_layout(ui, cornerX,
					  cornerY,
					  cornerEndX,
					  cornerEndY,
					  0, 0, 0, 0);

}

static ui_id
ui_update_render_scroll_vertical(game_ui *ui,
								 u8 *label,
								 f32 frameX,
								 f32 frameY,
								 f32 frameW,
								 f32 frameH,
								 f32 contentInsideFrameY,
								 f32 *scroll_value)
{

	   	//Based on where the cursor ended on the last frame, get the total size of the contents.

	   	vec2 frameSize = {frameW, frameH};
	   	vec2 frameP    = {frameX, frameY}; 
	    f32 scrollW    = ui_SCROLL_WH;
	    f32 scrollH    = frameSize.y; 
	    f32 scrollX    = frameP.x + frameSize.x - scrollW;
	    f32 scrollY    = frameP.y;

	   	ui_id scrollId	      = UIIDFROMPTR(label, scroll_value);
		//used to scroll with wheel
		ui_update_element_forced_at(
				ui,
				scrollId,
				frameX,
				frameY,
			    frameW,
				frameH);
		//used for holding click
		ui_create_update_element_at(
				ui,
				scrollId,
				scrollX,
				scrollY,
			    scrollW,
				scrollH);
	   	i32 scrollInteracting = ui_element_interacting(ui, scrollId); 
	   	i32 scrollHover       = ui_element_forced_hot(ui, scrollId);

	    //position and sizes

        f32 contentOutsideBoundsY = contentInsideFrameY - frameH; 
	   	if(contentOutsideBoundsY > 0)
	   	{
			f32 scrollVCopy = *scroll_value;

	   		f32 contentDifference = frameH / contentInsideFrameY;
	   		scrollY += *scroll_value * contentDifference;
	   		scrollH =  frameH * contentDifference;
	   		//for clicking and scrolling
	   		if(scrollHover)
	   		{
	   		   //Add the mouse wheel value to the scroll
	   		   i16 wheelDir  = ui->mouse_wheel;
	   		   scrollVCopy -= wheelDir * 40;
			   if(wheelDir != 0)
			   {
				   int s = 0;
			   }
	   		}
            if(scrollInteracting)
	        {
	        	if(ui->mouse_l_down)
	        	{
				  f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y; 
	   		 	  f32 scrollSpeed = mouse_delta_y / contentDifference;
	              scrollVCopy     += scrollSpeed;
	        	}

	        }


	        scrollVCopy = scrollVCopy < 0 ? 0 : 
	        			  scrollVCopy > contentOutsideBoundsY ? contentOutsideBoundsY :
	        			  scrollVCopy; 
			*scroll_value = scrollVCopy;
		}
		else
		{
			*scroll_value = 0;
		}



    	//;Mark theme_scrollbar
    	vec4 scrollbarColor			   = ui->theme.scrollbarColor;
    	vec4 scrollbarHotColor	       = ui->theme.scrollbarHotColor;
    	vec4 scrollbarInteractingColor = ui->theme.scrollbarInteractingColor;
    	vec4 scrollbarBackColor		   = ui->theme.scrollbarBackColor;

    	render_draw_rectangle_2D(ui->renderCommands, scrollX, frameY, scrollW, frameH, scrollbarBackColor); 
		ui_render_interactive(
				ui,
				scrollId,	
				ui_interaction_mouse_left_down,
				scrollX, scrollY,
				scrollW, scrollH,
				scrollbarColor,
				scrollbarInteractingColor,
				scrollbarHotColor);

		*scroll_value = f32_ceil(*scroll_value);
		return(scrollId);
}

static ui_id
ui_update_render_scroll_horizontal(game_ui *ui, u8 *label,
								   f32 frameX, f32 frameY, f32 frameW, f32 frameH, f32 contentInsideFrameX, f32 *scroll_value)
{
	   	ui_id scrollId	      = UIIDFROMPTR(label, scroll_value);
	   	i32 scrollInteracting = ui_element_interacting(ui, scrollId); 
	   	i32 scrollHover       = ui_element_hot(ui, scrollId);
        ui_element element    = {0};
		element.id			  = scrollId;
	   	//Based on where the cursor ended on the last frame, get the total size of the contents.
	    //position and sizes
		f32 totalScrollSize = 14;
	    f32 scrollW = frameW;
	    f32 scrollH = totalScrollSize; 
	    f32 scrollX = frameX;
	    f32 scrollY = frameY + frameH - scrollH;
        //with the entire scroll bar.
		ui_update_element_at(ui, &element, scrollX, scrollY, scrollW, scrollH);

        f32 contentOutsideBoundsX = contentInsideFrameX - frameW; 
	   	if(contentOutsideBoundsX > 0)
	   	{
			f32 scrollVCopy       = *scroll_value;
	   		f32 contentDifference = frameW / contentInsideFrameX;

	   		scrollX += *scroll_value * contentDifference;
	   		scrollW *= contentDifference;
	   		//for clicking and scrolling
	   		if(scrollHover)
	   		{
	   		   //Add the mouse wheel value to the scroll
	   		   scrollVCopy -= ui->mouse_wheel * 40;
	   		}
            if(scrollInteracting)
	        {
	        	if(ui->mouse_l_down)
	        	{
				  f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x; 
	   		 	  f32 scrollSpeed = mouse_delta_x / contentDifference;
	              scrollVCopy     += scrollSpeed;
	        	}

	        }


	        *scroll_value = scrollVCopy < 0 ? 0 : 
	        			   scrollVCopy > contentOutsideBoundsX ? contentOutsideBoundsX : scrollVCopy; 
		}
    	//;Mark theme_scrollbar
    	vec4 scrollbarColor			   = ui->theme.scrollbarColor;
    	vec4 scrollbarHotColor	       = ui->theme.scrollbarHotColor;
    	vec4 scrollbarInteractingColor = ui->theme.scrollbarInteractingColor;
    	vec4 scrollbarBackColor		   = ui->theme.scrollbarBackColor;
        //background
    	render_draw_rectangle_2D(ui->renderCommands, frameX, scrollY, frameW, scrollH, scrollbarBackColor); 
		ui_render_interactive(ui,
									element.id,
				                    ui_interaction_mouse_left_down,
									scrollX, scrollY,
									scrollW, scrollH,
									scrollbarColor,
									scrollbarInteractingColor,
									scrollbarHotColor);
		*scroll_value = f32_ceil(*scroll_value);
		return(scrollId);
}

static u32 
_ui_update_render_input_text(game_ui *ui,
		                     ui_id input_id,
							 f32 x,
							 f32 y,
							 f32 w,
							 f32 h,
							 u32 confirm_on_enter, 
							 u32 target_buffer_size,
							 u8 *target_buffer)
{
//VATih
    ui_push_clip_inside_last(ui, (i32)x, (i32)y, (i32)(x + w), (i32)(y + h));
	u32 entered = 0;

	s_input_text *input_text  = ui->input_text; 
	vec2 layoutCursor      = {x, y};

    f32 font_height      = ui->fontp.font_height * ui->font_scale;
	f32 inputTextOffset  = 2;
	//for interaction with the input text
	ui_create_update_element_at(ui, input_id, x, y, w, h);

	//state
    u32 element_interacting = ui_element_interacting(ui, input_id); 
	u32 element_hot         = ui_element_hot(ui, input_id);

    u32 textCursorStart = input_text->cursor_position_l; 
    u32 textCursorEnd   = input_text->cursor_position_r;
    if(input_text->cursor_position_l >= input_text->cursor_position_r)
    {
        textCursorStart = textCursorEnd;
        textCursorEnd   = input_text->cursor_position_l;
    }
	//timer

	//reset the timer if the input cursor moved
	if(ui->input_text->cursor_moved ||
	   ui->input_text->last_key_count != ui->input_text->key_count)
	{
	    ui->input_text_timer = 0;;
	}


	//background
	vec4 inputTextBackColor = ui->theme.frame_background_color; 
    render_draw_rectangle_2D(ui->renderCommands, x, y, w, h, inputTextBackColor);
	//borders
    render_rectangle_borders_2D(ui->renderCommands, x, y, w, h, 1, ui->theme.frameBorderColor);

	u8 *previewBuffer  = target_buffer;
    //Got focus
    if(element_interacting)
    {
       //mouse input
       u32 mouseDown    = ui->mouse_l_down;
	   u32 mousePressed = input_pressed(ui->input->mouse_left);
	   u32 mouseDoubleClickLeft = ui->input->doubleClickLeft;
	   u32 mouseTrippleClickLeft = ui->input->tripleClickLeft;
	   u32 gotClicked = element_hot && mousePressed;

	   //Tell to process input to it's buffer
       input_text->focused = 1;
	   ui->input_text_focused = 1;
	   //Don't process any other interaction
	   ui->keepInteraction = 1;
	   //If clicked anywere else 
	   if(mousePressed && !ui_element_last_hot(ui, input_id))
	   {
		   ui->keepInteraction = 0;
		   ui_CancelCurrentInteraction(ui);
		   input_text_RestoreBuffer(ui->input_text);
	   }
	   //Confirm changes
	   //Use the buffer from s_input_text as backup until enter is pressed
       if(confirm_on_enter)
       {
		   input_text_RestoreBuffer(input_text);
		   previewBuffer = input_text->buffer;
       }
	   else if(input_text->targetBuffer != target_buffer)
	   {
		   //Just modify the target buffer
           u32 target_bufferCount = string_count(target_buffer);
		   input_text_set_target(
				   input_text,
				   target_buffer, target_bufferCount, target_buffer_size);
	   }

	   //Target to text buffer
	   u32 got_focus = !input_text->got_focus || ui->element_transition;
       if(got_focus)
       {
		   u32 target_bufferCount = string_count(target_buffer);
           input_text->key_count   = string_copy_and_clear(target_buffer, input_text->buffer, ARRAYCOUNT(input_text->buffer));
		   //re-position the cursor as below with a function
       }
       //just got focus or got clicked
	   if(got_focus || gotClicked)
	   {
		   u32 gotSelection = ui->input_text->cursor_position_l != ui->input_text->cursor_position_r;
		   if(got_focus || (!gotSelection && mouseDoubleClickLeft))
		   {
			  //Get both cursors at ' ' character
		      ui->input_text->cursor_position_l = string_get_previous_char_index_from(previewBuffer, ui->input_text->cursor_position_l, ' ');
		      ui->input_text->cursor_position_r = string_get_next_char_index_from(previewBuffer, ui->input_text->cursor_position_r, ' ');

			  //Advance not select the ' ' char
			  if(ui->input_text->cursor_position_l)
			  {
                 ui->input_text->cursor_position_l++;
			  }
			  textCursorStart =ui->input_text->cursor_position_l;
			  textCursorEnd =ui->input_text->cursor_position_r;			  
		   }
		   else if(!got_focus && mouseTrippleClickLeft)
		   {
		      ui->input_text->cursor_position_l    = 0;
		      ui->input_text->cursor_position_r = ui->input_text->key_count;
		   }
		   else if(gotClicked )
           {
              f32 mouseDistanceFromInputX = ui->mouse_point.x - x;
		      //Doesn't support multi-line text yet
		      u32 finalCursorIndex = font_get_closest_index_at(&ui->fontp, previewBuffer, ui->font_scale, mouseDistanceFromInputX);
		      textCursorStart           = finalCursorIndex;
		      textCursorEnd             = finalCursorIndex;
		      ui->input_text->cursor_position_l    = finalCursorIndex;
		      ui->input_text->cursor_position_r = finalCursorIndex;
		   }
		   if(!got_focus)
		   {
			   int s = 0;
		   }
	   }
	   ui->process_hot_nodes = 1;
	   if(mouseDown)
	   {
		   ui->process_hot_nodes = 0;
	   }

      f32 textWidthFromStart = ui_GetTextWidthAt(ui, 0, textCursorStart, previewBuffer);

      real32 selectionWidth = ui_GetTextWidthAt(ui, textCursorStart, textCursorEnd, previewBuffer);
      real32 cursorW        = MAX(2.0f, selectionWidth);
	  //Input cursor
	  //make the cursor "blink" if the timer is more than half the total time
	  f32 inputCursorAlpha = 0xff;
	  if(textCursorStart == textCursorEnd)
	  {
          inputCursorAlpha = (ui->input_text_timer > (ui_INPUT_CURSOR_TIMER_TOTAL * 0.5f)) ? 0.0f : 0xff;
	  }
	  //render cursor
      render_draw_rectangle_2D(ui->renderCommands,
			             layoutCursor.x + textWidthFromStart + inputTextOffset,
						 layoutCursor.y + inputTextOffset,
						 cursorW,
						 font_height,
						 V4(200, 200, 200, inputCursorAlpha));
      //render the buffer text, else the one to modify

	  entered = input_text->entered;
	  ui->input_text_entered = entered;
	  if(confirm_on_enter && entered)
	  {
		  string_copy(input_text->buffer, target_buffer);
	  }
    }
	render_text_2d_no_wrap(ui->renderCommands, &ui->fontp, layoutCursor.x + inputTextOffset, layoutCursor.y + inputTextOffset, ui->font_scale, ui->theme.textColor, previewBuffer);

	render_commands_PopClip(ui->renderCommands);

	return(entered);
}

static u32 
_ui_update_render_input_text_element(game_ui *ui, ui_id input_id, u32 confirm_on_enter, u32 target_buffer_size, u8 *target_buffer)
{

	f32 remaining_panel_w = ui->currentLayout->cornerEndX - ui->currentLayout->cursorX;
	f32 inputTextHeight     = 32;
	vec2 layoutCursor       = ui_get_layout_cursor(ui);
    u32 entered             = _ui_update_render_input_text(ui, input_id, layoutCursor.x, layoutCursor.y, remaining_panel_w, inputTextHeight, confirm_on_enter, target_buffer_size, target_buffer);
	ui_advance(ui, layoutCursor.x, layoutCursor.y, remaining_panel_w, inputTextHeight);
//(game_ui *ui, ui_id input_id, f32 x, f32 y, f32 w, f32 h, u32 confirm_on_enter, u32 target_buffer_size, u8 *target_buffer)
	return(entered);
}

inline u32 
ui_update_render_input_text_element(game_ui *ui, ui_element *element)
{

   u32 confirm_on_enter = 1;
   u32 confirmChanges = _ui_update_render_input_text_element(ui, element->id, confirm_on_enter, element->input.textLimit, element->input.dest);
   if(confirm_on_enter && confirmChanges)
   {
      u8 *textBuffer     = ui->input_text->buffer;
	  u8 *target_buffer   = element->input.dest;

	  u32 textBufferCount = string_count(textBuffer);
      string_copy_and_clear(textBuffer, target_buffer, textBufferCount);
   }
   return(confirmChanges);
}

#define ui_update_render_input_TYPE(type) \
inline void ui_update_render_input_ ##type(game_ui *ui, ui_id id, type value, f32 x, f32 y, f32 w, f32 h, ui_input_text_flags textInputFlags)

static void
ui_update_render_input_(game_ui *ui, ui_id id, f32 x, f32 y, f32 w, f32 h, ui_input_text_flags textInputFlags, u8 *valueBuffer)
{
	u32 confirm_on_enter = textInputFlags & ui_text_input_confirm_on_enter;
	u32 interacting    = ui_element_interacting(ui, id);

	u8 *buffer     = valueBuffer;
	u32 bufferSize = sizeof(valueBuffer);
	//switch buffers
	if(interacting)
	{
	   //if(textInputFlags & ui_text_input_confirm_on_enter)
		u32 got_focus = ui->element_transition;
		if(got_focus)
		{
			string_copy_and_clear(valueBuffer,
					ui->inputTextBuffer, 
					sizeof(ui->inputTextBuffer));
		}
		buffer     = ui->inputTextBuffer;
		bufferSize = sizeof(ui->inputTextBuffer);

	}

    _ui_update_render_input_text(ui, id, x, y, w, h, confirm_on_enter, bufferSize, buffer);
}

ui_update_render_input_TYPE(i32)
{
    u8 valueBuffer[64];
    FormatText(valueBuffer, sizeof(valueBuffer), "%d", value);
    ui_update_render_input_(ui, id, x, y, w, h, textInputFlags, valueBuffer);
}

ui_update_render_input_TYPE(u16)
{
    u8 valueBuffer[64];
    FormatText(valueBuffer, sizeof(valueBuffer), "%u", value);
    ui_update_render_input_(ui, id, x, y, w, h, textInputFlags, valueBuffer);
}

ui_update_render_input_TYPE(u32)
{
    u8 valueBuffer[64];
    FormatText(valueBuffer, sizeof(valueBuffer), "%u", value);
    ui_update_render_input_(ui, id, x, y, w, h, textInputFlags, valueBuffer);
}

ui_update_render_input_TYPE(f32)
{
    u8 valueBuffer[64];
    FormatText(valueBuffer, sizeof(valueBuffer), "%f", value);
    ui_update_render_input_(ui, id, x, y, w, h, textInputFlags, valueBuffer);
}

static void 
ui_Post_Label_All(game_ui *ui, f32 x, f32 y, f32 szX, f32 szY, vec2 textOff , vec4 color, u8 *text)
{
    render_draw_rectangle_2D(ui->renderCommands, x, y, szX, szY, color);

#if 1
    render_text_2d(ui->renderCommands,
				  &ui->fontp,
			      x + textOff.x,
			      y + textOff.y,
				  F32MAX,
				  F32MAX,
				  ui->font_scale,
				  ui->theme.textColor,
			      text);
#endif
}


static void
ui_Post_Label_Sz(game_ui *ui, f32 x, f32 y, f32 szX, f32 szY, vec4 color, u8 *text, font_text_pad padOptions)
{
	vec2 textOff = ui_get_text_padded_offset_vec2( ui, V2(szX, szY), text, padOptions);

	ui_Post_Label_All(ui, x, y, szX, szY, textOff, color, text);

}

static void
ui_Post_Label(game_ui *ui, f32 x, f32 y, vec4 color, u8 *text)
{

	vec2 textDim = ui_get_text_size(ui, F32MAX, text);

	vec2 lSize = { textDim.x + 8,
				   textDim.y + 12 };

	vec2 textOff = { (lSize.x - textDim.x) * 0.5f,
					 (lSize.y - textDim.y) * 0.5f};

	ui_Post_Label_All(ui, x, y, lSize.x, lSize.y, textOff, color, text);
}

static void 
ui_Post_Labelf_Sz(game_ui *ui, f32 x, f32 y, f32 szX, f32 szY, vec4 color, u8 *text, ...)
{
   u8 buffer[256];
   va_list args;
   va_start_m(args, text);
   FormatTextList(buffer, sizeof(buffer), text, args);
   va_end_m(args);

   ui_Post_Label_Sz(ui, x, y, szX, szY, color, buffer, font_text_pad_center);
}

static void 
ui_Post_Labelf(ui_layout *currentLayout, vec4 color, u8 *text, ...)
{
   u8 buffer[256];
   va_list args;
   va_start_m(args, text);
   FormatTextList(buffer, sizeof(buffer), text, args);
   va_end_m(args);

   //vec2 result = ui_Post_Label(currentLayout, color, buffer);
}

//
//Content boxes, panels, etcs...
//

#define PANELBACKCOLOR V4(13, 17, 60, 0xe1)

//
// Post ui functions__
//



//
// __Beginning and ending ui__
//

inline game_ui *
ui_begin_frame(memory_area *ui_area,
		       font_proportional *ui_font,
			   platform_api *platform,
			   game_renderer *gameRenderer,
			   game_input *gameInput,
			   f32 dt)
{
   //Reset, but not clear.
   memory_area_reset(ui_area);

	game_ui *ui = memory_area_push_struct(ui_area, game_ui);
	ui->area = ui_area;

   ui->mouse_point     = V2(gameInput->mouse_x, gameInput->mouse_y);
   ui->mouse_point_last = V2(gameInput->mouse_x_last, gameInput->mouse_y_last);

   ui->fontp         = *ui_font; 
   //Note(Agu): Desired scale divided by height;
   f32 font_height = 12.0f; //20.0f
   ui->font_scale	  = 1;//font_height /  ui->fontp.font_height;
   ui->mouseOverPanel = 0;
   ui->mouse_l_down	 = (u8)input_down(gameInput->mouse_left);
   ui->mouse_l_pressed = (u8)input_pressed(gameInput->mouse_left);
   ui->mouse_l_up = (u8)input_up(gameInput->mouse_left);

   ui->mouse_wheel    = gameInput->mouse_wheel;
   ui->input_text     = &gameInput->input_text;
   //F*cking finally
   ui->input         = gameInput;

   //Setup render
   ui->renderCommands = render_commands_Begin(gameRenderer);
   ui->renderCommands->render_flags = 0 | render_flags_Blending;

   ui->reservedIdValue = ui_LASTAVADIBLEID;


   if(!ui->initialized)
   {
	   for(u32 i = 0; i < ui->panel_stack_count;i++)
	   {
		   ui->panel_stack_order[i]= i;
	   }

	   ui->initialized = 1;
	   ui->theme       = ui_DefaultTheme();
       ui->persistentElementsCount = 0;
	   memory_area_preserve_size_and_clear(ui->area, sizeof(game_ui));
   }

   if(!ui->explorer)
   {
	   ui->explorer = memory_area_clear_and_pushStruct(ui->area, ui_explorer);

	   ui->explorer->update_path_files  = 1;
	   ui->explorer->platform         = platform;
	   ui->explorer->search_pattern[0] = '*';
	   ui->explorer->last_process_completed = 0;
	   ui->explorer->closed = 1;
	   //ui->explorer->panel  = 0;
   }
   else
   {
	   ui->explorer = memory_area_push_struct(ui->area, ui_explorer);
   }
   
   if(!ui->panel_stack_count)
   {
	   //If not set, reserve space for 4 panels.
	   ui->panel_stack_count = 4;
	   ui->panel_last_avadible_slot = 0;
   }
   if(!ui->layoutStackCount)
   {
	   ui->layoutStackCount = 1;
   }
   if(!ui->pushed_disable_max)
   {
	   ui->pushed_disable_max = 6;
   }
   ui->layoutStackCount       += ui->layoutAmountBeyondStack;
   ui->layoutAmountBeyondStack = 0;

   //disable stack overflow
   if(ui->pushed_disable_total_count >= ui->pushed_disable_max)
   {
	   ui->pushed_disable_max += 1 + (ui->pushed_disable_total_count - ui->pushed_disable_max);
   }
   ui->pushed_disable_count = 0;
   ui->pushed_disable_total_count = 0;
   ui->pushed_disable_true_count = 0;

   //Main panel overflow
   if(ui->panelOverflow)
   {
	   //Add more space
	   ui->panel_stack_count += ui->panelOverflow + 2;
   }

   ui->persistentElementsMax   = 20;
   ui->persistentElements      = memory_area_push_array(ui->area, ui_element_persistent, ui->persistentElementsMax);
   //Allocate space needed
   ui->panel_stack      = memory_area_push_array(ui->area, ui_panel, ui->panel_stack_count);
   ui->panel_stack_order = memory_area_push_array(ui->area, u16, ui->panel_stack_count);
   ui->layoutStack     = memory_area_push_array(ui->area, ui_layout, ui->layoutStackCount);
   ui->disable_stack   = memory_area_push_size(ui->area, ui->pushed_disable_max);


   ui->commandsTotalSize = sizeof(ui_element) * 6000; 
   ui->commands_base	     = memory_area_push_size(ui->area, ui->commandsTotalSize); 
   ui->commands_offset    = ui->commands_base;

   ui->reserved_space_used  = 0;
   ui->reserved_space_total = (u32)(ui->area->size - ui->area->used);
   ui->reserved_space      = ui->area->base + ui->area->used;
   //Main panel overflow
   if(ui->panelOverflow)
   {
	   //Reset new panels
	   ui_panel zeroPanel = {0};
	   for(u32 i = ui->panel_last_avadible_slot; i < ui->panel_stack_count;i++)
	   {
		   ui->panel_stack_order[i] = i;
		   ui->panel_stack[i]      = zeroPanel;
	   }

	   for(u32 i = 0; i < ui->panel_stack_count;i++)
	   {
		   ui->panel_stack_order[i]= i;
	   }
	   ui->panelOverflow = 0;
   }

   ui->panel_stack_count   = 0;

   if(ui->input_text_focused)
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



   //ui_id panelId                     = ui_id_NUMBER_POINTER("_Explorer", ui->explorer);
   //ui_panel_flags explorerPanelFlags = ui_panel_flags_title | ui_panel_flags_move | ui_panel_flags_close | ui_panel_flags_size_to_content;
   //ui_id explorerPanelId             = ui_id_NUMBER_POINTER("_Explorer", ui->explorer);
   //ui_panel *explorerPanel           = ui_GetOrPushPanelStack(ui, explorerPanelId);
   ////Panel was not previously on the stack
   //u32 expectedPanel                 = ui_id_EQUALS(explorerPanel->id, panelId);
   //if(!expectedPanel)
   //{
   //    explorer->panel = explorerPanel;
   //    explorerPanel->closed = 1;
   //    explorerPanel->title  = "No process";
   //    explorerPanel->id     = explorerPanelId;
   //}
   //explorerPanel->flags = explorerPanelFlags;
#if 0
   ui->commands_base	     = ui->area.base + ui->area.used;
   ui->commands_offset    = ui->commands_base;
   ui->commandsTotalSize = (u32)(ui->area.size - ui->area.used);
#endif

   render_commands_SetClipAndViewport(ui->renderCommands, 0, 0, gameRenderer->back_buffer_width,  gameRenderer->back_buffer_height);
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

   return(ui);

}

#define ZEROSTRUCT(a, type) {type b = {0}; a = b;}

static void
ui_panel_update_and_render_begin(game_ui *ui, ui_panel *panel)
{
	ui_id panelId      = panel->id;
	vec2 panel_position = panel->p;
	vec2 panel_size     = panel->sz;
	vec4 panel_frame_space = {0}; 

	f32 titleHeight = 0;

	f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x; 
	f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y; 

	//for moving
	u32 element_interactingMouseLeft = ui_element_interacting_flags(ui, panel->id, ui_interaction_mouse_left_down);
	//Interaction for certain panels
	u32 ignoreInteraction = panel->flags & ui_panel_flags_tooltip;
	if(!ignoreInteraction)
	{
		f32 panelW = panel->sz.x;
		f32 panelH = panel->sz.y;
		if(panel->notVisible)
		{
			panelH = TITLEHEIGHT;
		}
		ui_create_update_element_at(ui, panelId, panel->p.x, panel->p.y, panelW, panelH); 
		//process panel interaction before widgets
		if(ui_element_hot(ui, panelId))
		{
			ui->mouseOverPanel = 1;
		}
	}

	if((panel->flags & ui_panel_flags_move) && element_interactingMouseLeft)
	{
		panel_position.x = (f32)(i32)(panel->p.x + mouse_delta_x);
		panel_position.y = (f32)(i32)(panel->p.y + mouse_delta_y);
		panel->p  = panel_position;
	}

	//
	// title bar
	
	if(panel->flags & 
		(ui_panel_flags_title | ui_panel_flags_minimize) || panel->close_button)
	{
		panel_frame_space.y += TITLEHEIGHT;
		panel_frame_space.w -= TITLEHEIGHT;

		titleHeight     = TITLEHEIGHT;
		vec4 titleColor = ui->theme.titleColor;

		u32 panelInteracting            = ui->focused_panel == panel;
		if(panelInteracting)
		{
			titleColor.w = 250;
		}
		//render title
		render_draw_rectangle_2D(ui->renderCommands, 
				panel->p.x,
				panel->p.y,
				panel->sz.x,
				titleHeight,
				titleColor);

		if(panel->flags & ui_panel_flags_title)
		{

			u8 *titleText      = panel->title;
			f32 titleFontScale = ui->font_scale + 0.00f;
			vec2 textOffset	= font_get_text_pad_offset(&ui->fontp, panel->sz.x, titleHeight, titleText, titleFontScale, font_text_pad_left);
			textOffset.x += 4;

			render_text_2d(ui->renderCommands, &ui->fontp, panel->p.x + textOffset.x, panel->p.y + textOffset.y, F32MAX, F32MAX, titleFontScale, ui->theme.textColor, titleText); 
		}
		f32 buttonsWH = (f32)(u32)(titleHeight * 0.6f);
		f32 buttonCorner = panel->p.x + panel->sz.x;
		if(panel->flags & ui_panel_flags_minimize)
		{
			//
			// Show/Hide button
			//
			f32 show_hide_button_wh = buttonsWH;
			//4 for offset

			//Mid point
			vec2 triangle_position = {
				buttonCorner - show_hide_button_wh - 2, (panel->p.y + titleHeight * 0.5f)}; 
			buttonCorner -= show_hide_button_wh * 2;


			vec2 show_hide_button_position = {
				triangle_position.x - show_hide_button_wh * 0.5f,
				triangle_position.y - show_hide_button_wh * 0.5f};

			triangle_position = vec2_round_to_int(triangle_position);
			show_hide_button_position = vec2_round_to_int(show_hide_button_position);
			show_hide_button_wh = (f32)(i32)show_hide_button_wh;

			ui_id showHideButtonId     = UIIDFROMPTR(panel, &panel->notVisible);
			ui_element showHideElement = {0};
			showHideElement.id         = showHideButtonId;
			ui_render_interactive(ui, showHideButtonId, ui_interaction_mouse_left_down, show_hide_button_position.x, show_hide_button_position.y, show_hide_button_wh, show_hide_button_wh, V4(0, 0, 0, 0), vec4_add(ui->theme.titleColor, V4(20, 20, 20, 0)), vec4_add(ui->theme.titleColor, V4(20, 20, 20, 0)));

			f32 triangleAngle = PI;
			if(panel->notVisible)
			{
				triangleAngle = -0.5f * PI;
			}
			render_Triangle2DAngle(
					ui->renderCommands,
					triangle_position,
					show_hide_button_wh,
					triangleAngle,
					V4(0xff, 0xff, 0xff, 0xff)); 


			ui_update_element_at(ui, &showHideElement, triangle_position.x - show_hide_button_wh * 0.5f, triangle_position.y - show_hide_button_wh * 0.5f, show_hide_button_wh, show_hide_button_wh);

			u32 showHideButtonInteracted = ui_element_interacted(ui, showHideButtonId);
			if(showHideButtonInteracted)
			{
				panel->notVisible = !panel->notVisible;
			}
		}
		if(panel->close_button)
		{
			//Implement!
			f32 lineThickness = 3.0f;
			vec2 p0 = {
				buttonCorner - buttonsWH - 2,
				(panel->p.y + (titleHeight - buttonsWH) * 0.5f)};

			vec2 p1 = {p0.x + buttonsWH, p0.y + buttonsWH - lineThickness};

			vec2 p2 = {p0.x + buttonsWH, p0.y};
			vec2 p3 = {p1.x - buttonsWH, p1.y};

			render_line_2d_down(ui->renderCommands, p0, p1, lineThickness, vec4_all(255));
			render_line_2d_down(ui->renderCommands, p2, p3, lineThickness, vec4_all(255));

			ui_id close_button_id = UIIDFROMPTR(panel, &panel->closed);
			ui_create_update_element_at(ui, close_button_id, p0.x, p0.y, (p1.x - p0.x), (p1.y - p0.y) + lineThickness);
			if(ui_element_interacted(ui, close_button_id))
			{
				panel->closed = 1;
			}

		}

	}//

	if(!panel->notVisible)
	{
		panel_frame_space.z += panel->sz.x;
		panel_frame_space.w += panel->sz.y;
		//Background
		u32 is_invisible = panel->flags & ui_panel_flags_invisible;
		if(!is_invisible)
		{
			vec4 frontPanelColor =  ui->theme.frontPanelColor; 
			render_draw_rectangle_2D(ui->renderCommands, panel->p.x,
					panel->p.y + panel_frame_space.y,
					panel_frame_space.z,
					panel_frame_space.w,
					frontPanelColor);
		}

		// scroll bar
		panel->scrollInteracting = 0;
		if(panel->flags & ui_panel_flags_scroll_v)
		{
			//vertical
			vec2 frameSize     = {panel_frame_space.z, panel_frame_space.w};
			vec2 frameP        = {panel->p.x, panel->p.y + titleHeight};
			//ui_id frameId  = panel->id;

			ui_id scrollId = ui_update_render_scroll_vertical(ui, "SCOLL!!!",
					frameP.x, frameP.y,
					frameSize.x, frameSize.y,
					panel->totalContentSize.y,
					&panel->scroll_v);

			panel->scrollInteracting = ui_element_interacting(ui, scrollId);
			panel->scrollInteracted  = ui_element_interacted(ui, scrollId);
		}
		else
		{
			panel->scroll_v = 0;
		}

		if(panel->flags & ui_panel_flags_scroll_h)
		{
			//horizontal

			vec2 frameSize     = {panel->sz.x, panel->sz.y - titleHeight};
			vec2 frameP        = {panel->p.x, panel->p.y + titleHeight};
			//ui_id frameId  = panel->id;

			ui_id scrollId = ui_update_render_scroll_horizontal(ui, "SCOLL!!!",
					frameP.x, frameP.y,
					frameSize.x, frameSize.y,
					panel->totalContentSize.x,
					&panel->scroll_h);
			panel->scrollInteracting = ui_element_interacting(ui, scrollId);
			panel->scrollInteracted  = ui_element_interacted(ui, scrollId);
		}
		else
		{
			panel->scroll_h = 0;
		}
		//
		// panel resizing and movement
		//
		f32 szButtonWH = 0;
		if(panel->flags & ui_panel_flags_resize)
		{
			szButtonWH      = 16;
			ui_id resizeId   = UIIDFROMPTR(&panel->id, &panel->sz);

			f32 szButtonX   = panel->p.x + panel->sz.x - szButtonWH;
			f32 szButtonY   = panel->p.y + panel->sz.y - szButtonWH;

			ui_element resizeElement = {0};
			resizeElement.id = resizeId;

			ui_update_element_at(ui, &resizeElement, szButtonX, szButtonY, szButtonWH, szButtonWH);
			//now render it
			vec2 buttonP = {szButtonX, szButtonY};
			vec2 buttonSz = {szButtonWH, szButtonWH};
			ui_render_interactive(ui, resizeId, ui_interaction_mouse_left_down, buttonP.x, buttonP.y, buttonSz.x, buttonSz.y, ui->theme.button_normal_color, ui->theme.button_interacting_color, ui->theme.button_hot_color);

			//Process interaction
			i32 resizeInteracting = ui_element_interacting(ui, resizeId); 
			if(resizeInteracting)
			{
				f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x; 
				f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y; 
				panel_size.x += mouse_delta_x;
				panel_size.y += mouse_delta_y;
				panel_size.x = MAX(szButtonWH * 2.0f, panel_size.x);
				panel_size.y = MAX(szButtonWH * 2.0f, panel_size.y);
			}
		} 
		if(panel->flags & ui_panel_flags_size_to_content)
		{
			vec4 panel_frame_space = panel->frameSpace;
			vec2 contentSize = panel->totalContentSize;
			vec2 finalSize = {
				(panel->cornerOffsetX * 2) + panel_frame_space.x + contentSize.x,
				(panel->cornerOffsetY * 2) + panel_frame_space.y + contentSize.y,
			};
			//render_draw_rectangle_2D(ui->renderCommands, panel->p.x, panel->p.y, finalSize.x, finalSize.y, vec4_all(255));
			//panel->sz = finalSize;
			panel_size = finalSize;

		}
	}





	if(!panel->notVisible)
	{
		//Subtracts the space occuppied by the scroll bar or resize button
		if(panel->flags & (ui_panel_flags_scroll_v | ui_panel_flags_resize))
		{
			panel_frame_space.z -= 16;
		}

		if(panel->flags & ui_panel_flags_scroll_h)
		{

			panel_frame_space.w -= 16;
		}


		//Border
		render_rectangle_borders_2D(
				ui->renderCommands,
				panel->p.x,
				panel->p.y + panel_frame_space.y,
				panel->sz.x,
				panel->sz.y - panel_frame_space.y,
				1, ui->theme.frameBorderColor);
		//Layout and clip setup

		f32 x0 = panel->p.x;
		f32 y0 = panel->p.y + panel_frame_space.y;
		f32 x1 = x0 + panel_frame_space.z; 
		f32 y1 = y0 + panel_frame_space.w;

		f32 layoutOffsetX = 6.0f;
		f32 layoutOffsetY = 6.0f;

		f32 cursorOffsetX = -panel->scroll_h;
		f32 cursorOffsetY = -panel->scroll_v;
		//Start main panel layout
		ui_push_layout(ui, x0, y0, x1, y1, layoutOffsetX, layoutOffsetY, cursorOffsetX, cursorOffsetY);

		i32 clipX0 = (i32)(x0 + layoutOffsetX);
		i32 clipY0 = (i32)(y0 + layoutOffsetY);
		i32 clipX1 = (i32)(x1 - layoutOffsetX);
		i32 clipY1 = (i32)(y1 - layoutOffsetY);

		if(panel->flags & ui_panel_flags_child)
		{
			ui_push_clip_inside_last(ui, clipX0, clipY0, clipX1, clipY1);
		}
		else
		{
			ui_PushClip(ui, clipX0, clipY0, clipX1, clipY1); 
		}

		//Avadible space to draw
		panel->frameSpace.x = panel_frame_space.x + layoutOffsetX;
		panel->frameSpace.y = panel_frame_space.y + layoutOffsetY;
		panel->frameSpace.z = panel_frame_space.z - layoutOffsetX;
		panel->frameSpace.w = panel_frame_space.w - layoutOffsetY;
		panel->cornerOffsetX = layoutOffsetX;
		panel->cornerOffsetY = layoutOffsetY;
	}

	panel->totalContentSize.x = 0;
	panel->totalContentSize.y = 0;
	panel->sz = panel_size;
}

static void
ui_panel_update_render_end(game_ui *ui, ui_panel *panel)
{
	if(!(panel->notVisible))
	{
		ui_pop_layout(ui);
		render_commands_PopClip(ui->renderCommands);
	}

	if((panel->flags & ui_panel_flags_focus_when_opened) && 
			panel->was_closed)
	{
		panel->was_closed = 0;
		ui->focused_panel= panel;
		ui->forced_panel_focus = 1;
	}


}

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

static vec2
ui_RenderCheckBox(game_ui *ui, ui_id checkBoxId, f32 x, f32 y, u32 checked, u8 *text)
{
	vec2 checkBoxSize = font_get_text_size_wrapped_scaled(&ui->fontp, F32MAX, text, ui->font_scale);
	checkBoxSize.x += 24;
	checkBoxSize.y += 4;
	ui_update_render_button_label_at(ui,
								   checkBoxId,
			                       x,
								   y,
								   checkBoxSize.x,
								   checkBoxSize.y,
								   font_text_pad_right,
								   text);

    vec4 checkBackColor = 
    { 20, 20, 20, 0xcf};

    f32 cbOff = 4;
    f32 cbSz = 32;
    f32 cbX = x + cbOff;
    f32 cbY = y + cbOff;

	f32 cbH = cbSz - (cbSz - checkBoxSize.y) - (cbOff * 2);
	f32 cbW = cbH;

	 //Draw the checkbox
     render_draw_rectangle_2D(ui->renderCommands, cbX, cbY, cbW, cbH, checkBackColor);

     u32 checkActive = checked;
     if(checkActive)
     {
		f32 check_x = cbX + cbW * 0.16f;
		f32 check_y = cbY + cbH * 0.14f;
		f32 check_w = cbW - ((check_x + cbW) - (cbX + cbW));
    	f32 check_h = cbH - ((check_y + cbH) - (cbY + cbH)) * 2;

        ui_RenderCheckIcon(ui, check_x, check_y, check_w, check_h , 3.0f, vec4_all(255));
     }
	 return(checkBoxSize);
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

	    	render_draw_rectangle_2D(ui->renderCommands,
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

	    	render_draw_rectangle_2D(ui->renderCommands,
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

	      render_draw_rectangle_2D(ui->renderCommands,
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
	render_draw_rectangle_2D(
	   	 ui->renderCommands,
	   	 x,
	   	 y,
	   	 w,
	   	 h,
	   	 green);

	f32 side_h = h * 0.4f;
	f32 side_w = w * 0.1f;
	render_draw_rectangle_2D(
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

	render_draw_rectangle_2D(
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
//read commands start
static void
ui_read_commands(game_ui *ui, ui_command_op *uiCommandsOp)
{

       render_commands *commands = ui->renderCommands;   
	   u8 *commands_base  = uiCommandsOp->current->commands_base;
	   u8 *commands_end   = uiCommandsOp->current->commands_offset;
	   ui->current_panel  = uiCommandsOp->current;

	   u8 *panel_commands_at = commands_base;

	   Assert(commands_end);
	   Assert(commands_base);
	   for(u32 j = 0;
			   panel_commands_at != commands_end; 
			   j++)
       {
	   	 //assumming the commands will be readed immediately
		 ui_command_type panelCommandType = *(ui_command_type *)panel_commands_at;
		 //Draw panel

		 switch(panelCommandType)
		 {
			 case ui_command_type_set_text_color:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	   += sizeof(ui_element); 

					ui->theme.textColor = element->text_color.color;
				 }break;
			 case ui_command_type_button:
				 {
					 //only advance for now
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	   += sizeof(ui_element); 


					u8 *buttonText = element->label;
				    ui_update_render_button_text_sized_at_layout(ui, element->id, font_text_pad_center, buttonText);
				 }break;
			 case ui_command_type_imagebutton:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	   += sizeof(ui_element); 

					render_texture *texture = element->image_button.texture;

	                vec4 selectionDisabledColor    = {0, 0, 0, 160};
	                vec4 selectionNormalColor      = {0, 0, 0, 0};
	                vec4 selectionHotColor         = ui->theme.button_hot_color; 
	                vec4 selectionInteractingColor = ui->theme.button_interacting_color;

					vec2 layoutCursor = ui_get_layout_cursor(ui);
					f32 sizeX = element->image_button.buttonW;
					f32 sizeY = element->image_button.buttonH;
				    ui_update_advance_element(ui, element, sizeX, sizeY);

					//doesn't get used if it's only a button
					u32 selectable_active = element->image_button.selectable_active;

					ui_render_selectable(ui,
							             element->id, 
										 selectable_active,
										 ui_interaction_mouse_left_down,
										 layoutCursor.x,
										 layoutCursor.y,
										 sizeX,
										 sizeY,
										 selectionInteractingColor,
										 selectionNormalColor,
										 selectionHotColor,
										 selectionInteractingColor);

					f32 textureOffset = 6;
					vec2 imageFramePosition = {
						layoutCursor.x + textureOffset,
						layoutCursor.y + textureOffset
					};
					vec2 imageFrameSize = {
						sizeX - (textureOffset * 2),
						sizeY - (textureOffset * 2)
					};
					//Background
					vec4 imageBackgroundColor = ui->theme.frame_background_color;
					render_draw_rectangle_2D(ui->renderCommands, imageFramePosition.x, imageFramePosition.y, imageFrameSize.x, imageFrameSize.y, imageBackgroundColor);
					//Borders
					vec4 frameBorderColor = ui->theme.frameBorderColor; 
					render_rectangle_borders_2D(ui->renderCommands, imageFramePosition.x, imageFramePosition.y, imageFrameSize.x, imageFrameSize.y, 1, frameBorderColor);

					//Image
					vec2 uv0 = element->image_button.uv0; 
					vec2 uv1 = element->image_button.uv1;
					vec2 uv2 = element->image_button.uv2;
					vec2 uv3 = element->image_button.uv3;
					//"convert" to frames to get the corners of each uv in order to get the correct fit ratio
                    render_uv_frames uv_as_frames = render_uv_to_frames(
							ui->renderCommands->gameRenderer->texture_array_w,
							ui->renderCommands->gameRenderer->texture_array_h,
							                                            uv0,
																		uv1,
																		uv2,
																		uv3);
					
					u32 pixelX = uv_as_frames.frame_x;
					u32 pixelY = uv_as_frames.frame_y;
					u32 pixelW = uv_as_frames.frame_w;
					u32 pixelH = uv_as_frames.frame_h;

					vec2 endImageFrameSize = imageFrameSize;
					//Scale to image to preserve aspect ratio
					f32 imageRatio  = (f32)pixelW / pixelH;
					f32 imageRatioY = (f32)pixelH / pixelW;
					endImageFrameSize.x *= imageRatio;

					//Adjust size to the button
					f32 imageFrameRatioX = imageFrameSize.x / endImageFrameSize.x;
					f32 imageFrameRatioY = imageFrameSize.y / endImageFrameSize.y;
					u32 ratioXOverOne = imageRatio > 1.0f;
					u32 ratioYOverOne = imageRatioY > 1.0f;
					if(ratioXOverOne || ratioYOverOne)
					{
					    if(imageRatio > 1.0f)
					    {
					    	endImageFrameSize.x *= imageFrameRatioX;
					    	endImageFrameSize.y *= imageFrameRatioX;
					    }
					    if(imageRatioY > 1.0f)
					    {
					    	endImageFrameSize.x *= imageFrameRatioY;
					    	endImageFrameSize.y *= imageFrameRatioY;

					    }
					    f32 bordersX = 0.5f * (endImageFrameSize.x - imageFrameSize.x);
					    f32 bordersY = 0.5f * (endImageFrameSize.y - imageFrameSize.y);
					    imageFramePosition.x -= bordersX;
					    imageFramePosition.y -= bordersY;
					}

					vec2 v0 = {imageFramePosition.x, imageFramePosition.y + endImageFrameSize.y};
					vec2 v1 = {v0.x, imageFramePosition.y};
					vec2 v2 = {imageFramePosition.x + endImageFrameSize.x, v1.y};
					vec2 v3 = {v2.x, v0.y};

					if(texture)
					{
					    //render final image
					    render_push_quad_2d(ui->renderCommands,
					    		            texture,
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

				 }break;
			 case ui_command_type_tooltip:
				 {
					 //only advance for now
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	   += sizeof(ui_element); 
					panel_commands_at	   += sizeof(ui_panel); 

					//ui_update_advance_element(ui, element, element->tooltip.regionW, element->tooltip.regionH); 

					vec2 cursorLayout = ui_get_layout_cursor(ui);
	                ui_update_element_at(ui, element, cursorLayout.x, cursorLayout.y, element->tooltip.regionW, element->tooltip.regionH); 
					u32 toolTipHot = ui_element_hot(ui, element->id);

					ui_panel *toolTipPanel = element->tooltip.panel; 
					toolTipPanel->p        = ui->mouse_point;
					toolTipPanel->p.y      -= toolTipPanel->totalContentSize.y;
					toolTipPanel->sz       = toolTipPanel->totalContentSize;

					if(toolTipHot)
					{
						uiCommandsOp->nextPanel = toolTipPanel;
					}
					//Ignore the tooltip commands and advance
					panel_commands_at = toolTipPanel->commands_offset;

				 }break;
			 case ui_command_type_selectable:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at += sizeof(ui_element);


	                vec4 selectionDisabledColor    = {0, 0, 0, 160};
	                vec4 selectionNormalColor      = {0, 0, 0, 0};
	                vec4 selectionHotColor         = ui->theme.button_hot_color; 
	                vec4 selectionInteractingColor = ui->theme.button_interacting_color;

					if(element->selection.selected)
					{
					   selectionNormalColor = selectionInteractingColor;
	                   selectionHotColor    = selectionInteractingColor; 
					}


					vec2 elementP  = {ui->currentLayout->cursorX, ui->currentLayout->cursorY};
					f32 sizeX      = ui->currentLayout->cornerEndX - ui->currentLayout->cornerX;
					f32 sizeY      = ui_FontHeight(ui); 
					if(ui->pushed_element_size)
					{
						if(ui->pushed_element_size_option == ui_size_Specified)
						{
					        if(ui->pushed_element_w)
					        {
					        	sizeX = ui->pushed_element_w;
					        }
					        if(ui->pushed_element_h)
					        {
					        	sizeY = ui->pushed_element_h;
					        }
						}
						else if(ui->pushed_element_size_option == ui_size_ToText)
						{
							f32 textWidth = font_GetTextWidth(&ui->fontp, ui->font_scale, element->label);
							sizeX = textWidth + 4;
						}
					}

					f32 borderSize = 0;
					if(element->selection.borders)
					{
						borderSize = 2;
					    //Borders
					    render_rectangle_borders_2D(ui->renderCommands, elementP.x, elementP.y, sizeX + borderSize, sizeY + borderSize, borderSize, ui->theme.frameBorderColor);
					}
					ui_update_advance_element(ui, element, sizeX + borderSize, sizeY + borderSize);
					//Display element
					ui_render_interactive(ui,
							                        element->id,
													ui_interaction_mouse_left_down,
													elementP.x,
													elementP.y,
												    sizeX + borderSize,
													sizeY + borderSize,
													selectionNormalColor,
													selectionInteractingColor,
													selectionHotColor);

					//display text and add border thickness to offset
	                vec2 textOffset = ui_get_text_padded_offset_vec2(ui, V2(sizeX, sizeY), element->label, font_text_pad_left);
	                render_text_2d(ui->renderCommands,
	                		      &ui->fontp,
	                			  elementP.x + textOffset.x + borderSize,
	                			  elementP.y + textOffset.y + borderSize,
	                			  F32MAX,
	                			  F32MAX,
	                			  ui->font_scale,
								  ui->theme.textColor,
	                			  element->label);

				 }break;
			 case ui_command_type_checkbox:
				 {
					 //only advance for now
				    //ui_UpdateAndRenderButtonTextSized(ui, element, buttonText, font_text_pad_center);
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	   += sizeof(ui_element); 
					vec2 layoutCursor		= ui_get_layout_cursor(ui);


					vec2 checkBoxSize = font_get_text_size_wrapped_scaled(&ui->fontp, F32MAX, element->label, ui->font_scale);
					checkBoxSize.x += 24;
					checkBoxSize.y += 4;
				    ui_UpdateAndRenderButtonTextAtLayout(ui,
														 element->id,
														 checkBoxSize.x,
														 checkBoxSize.y,
														 font_text_pad_right,
														 element->label);

                    vec4 checkBackColor = 
                    { 20, 20, 20, 0xcf};
					u8 *value = (u8 *)element->value;

                    f32 cbOff = 4;
                 	f32 cbSz = 32;
                    f32 cbX = layoutCursor.x + cbOff;
                    f32 cbY = layoutCursor.y + cbOff;

					f32 cbH = cbSz - (cbSz - checkBoxSize.y) - (cbOff * 2);
					f32 cbW = cbH;

					 //Draw the checkbox
                     render_draw_rectangle_2D(ui->renderCommands, cbX, cbY, cbW, cbH, checkBackColor);

                     u32 checkActive = *value;
                     if(checkActive)
                     {
						f32 check_x = cbX + cbW * 0.16f;
						f32 check_y = cbY + cbH * 0.14f;
						f32 check_w = cbW - ((check_x + cbW) - (cbX + cbW));
                 		f32 check_h = cbH - ((check_y + cbH) - (cbY + cbH)) * 2;

                        ui_RenderCheckIcon(ui, check_x, check_y, check_w, check_h , 3.0f, vec4_all(255));
                     }
                 
                 
				 }break;
			 case ui_command_type_drag:
				 {
					 //NOT TESTED
					 //only advance for now
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at	+= sizeof(ui_element); 

					u8 textBuffer[32] = {0};
					u32 vType = element->slider.valueType;

					f32 elementWidth  = ui->pushed_element_w;
					f32 elementHeight = ui->pushed_element_h;
					if(!elementWidth)
					{
						elementWidth = font_GetTextWidth(&ui->fontp, ui->font_scale, element->slider.format);
					}
					if(!elementHeight)
					{
                        elementHeight = ui_FontHeight(ui);
					}
					ui_UpdateAndRenderButtonTextAtLayout(
							ui,
							element->id,
							elementWidth,
							elementHeight,
							font_text_pad_center,
							element->slider.format);

#if 0
				    ui_render_interactiveWithText(ui,
												        textBuffer,
												        font_text_pad_center,
												        V2(element->x, element->y),
												        V2(element->szX, element->szY),
												        element->id,
												        button_normal_color,
												        button_interacting_color,
												        button_hot_color);
#endif

				 }break;
			 case ui_command_type_updown:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 
	                //Not implemented.
	                //ui_UpDownu32(&tilesetPanel->layout, "TileSize_Updown", &value, 1, 0, 20);
	                vec4 inputTextBackColor = {12, 0, 42, 0xff};
	                //ui_id updownHId = element->id; 
	                //ui_id incId		   = ui_id_POINTERS(">", element);
	                //ui_id decId		   = ui_id_POINTERS("<", element);
	                ui_id incId		   = element->updown.incrementId; 
	                ui_id decId		   = element->updown.decrementId; 
					ui_id valueInputId = element->updown.input_id; 

					ui_input_text_flags textInputFlags = element->updown.textInputFlags;


					u32 lineWasPushed = ui->currentLayout->keepLine;
					ui_UpdateKeepLinePush(ui);

					f32 offsetFromButtons = 2;

					vec2 layoutCursor = ui_get_layout_cursor(ui);
	                vec2 buttonSz     = {20, 12};
					vec2 inputAt      = {layoutCursor.x, layoutCursor.y};

					f32 element_w = ui_get_remaining_layout_width(ui);
					f32 element_h = buttonSz.y * 2;
					if(ui->pushed_element_size)
					{
	                     element_w = ui->pushed_element_w ? ui->pushed_element_w : element_w;
	                     element_h = ui->pushed_element_h ? ui->pushed_element_h : element_h;
					}

					vec2 inputSz = {0};
					inputSz.x    = element_w - buttonSz.x;
					inputSz.y    = element_h;

					//Increase/Decrease buttons.
					f32 buttons_x = inputAt.x + inputSz.x + offsetFromButtons;
					f32 buttons_y = inputAt.y;
					ui_update_render_button_label_at(ui, incId, buttons_x, buttons_y, buttonSz.x, buttonSz.y, font_text_pad_center, ">");
					ui_update_render_button_label_at(ui, decId, buttons_x, buttons_y + buttonSz.y, buttonSz.x, buttonSz.y, font_text_pad_center, "<");


					if(element->updown.valueType == ui_value_i32)
					{
						u32 val = element->updown.value_i32;
                        ui_update_render_input_i32(ui,
								                    valueInputId,
													val,
													inputAt.x,
													inputAt.y,
													inputSz.x,
													inputSz.y,
													textInputFlags);
					}
					if(element->updown.valueType == ui_value_u32)
					{
						u32 val = element->updown.value_u32;
                        ui_update_render_input_u32(ui,
								                    valueInputId,
													val,
													inputAt.x,
													inputAt.y,
													inputSz.x,
													inputSz.y,
													textInputFlags);
					}
					else if(element->updown.valueType == ui_value_u16)
					{
						u32 val = element->updown.value_u16;
                        ui_update_render_input_u16(ui,
								                    valueInputId,
													val,
													inputAt.x,
													inputAt.y,
													inputSz.x,
													inputSz.y,
													textInputFlags);

					}
                    else if(element->updown.valueType == ui_value_f32)
					{
						f32 val = element->updown.value_f32;
                        ui_update_render_input_f32(ui, valueInputId, val, inputAt.x, inputAt.y, inputSz.x, inputSz.y, textInputFlags);

					}
					ui_advance(ui, inputAt.x, inputAt.y, inputSz.x + buttonSz.x, inputSz.y);
					//Increase

					if(!lineWasPushed)
					{
					  ui_UpdateKeepLinePop(ui);
					}

				 }break;
			 case ui_command_type_image:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 
					vec2 cursor_position_l        = ui_get_layout_cursor(ui);
					ui_update_advance_element(ui, element, element->image.w, element->image.h);

					//Hardcodede frames for the moment
					vec2 imageSize = {element->image.w, element->image.h};

					render_draw_rectangle_2D_vec2(ui->renderCommands, cursor_position_l, imageSize, V4(0, 0, 0, 190));

					render_draw_sprite_2D(ui->renderCommands, element->image.source,cursor_position_l, imageSize, vec4_all(255) , 0, 0, 512, 512);
					 
				 }break;
			 case ui_command_type_text:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					vec2 cursor_position_l = ui_get_layout_cursor(ui);
					f32 textEnd  = F32MAX; 
					if(element->text.wrap)
					{
						textEnd = ui->currentLayout->cornerEndX;
					}
				    ui_UpdateTextElement(ui, element, textEnd, element->label);
					render_text_2d_All(ui->renderCommands,
							          &ui->fontp,
									  cursor_position_l.x,
									  cursor_position_l.y,
									  textEnd,
									  F32MAX,
									  ui->font_scale,
									  ui->theme.textColor,
									  element->label);
				 }break;

				 //Custom widgets
			 case ui_command_type_selectable_image:
				 {
					ui_element *_element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					ui_element_persistent *element        = ui->persistentElements + _element->persistent_element_array_index;
					ui_image_selection *selectable_image = &element->selectable_image;

					panel_commands_at += sizeof(struct ui_image_selection_group) * selectable_image->selectable_group_max;


                    selectable_image->uvsUpdated = 0;

					vec2 layoutCursor  = ui_get_layout_cursor(ui);
					vec4 frameBorderColor = ui->theme.frameBorderColor; 

					//Cursor to add other widgets
					vec2 elementCursor = layoutCursor;
					//Widget size
					vec2 selectableImageSize = ui_get_remaining_layout_size(ui);

					f32 mouse_delta_x = ui->mouse_point.x - ui->mouse_point_last.x;
					f32 mouse_delta_y = ui->mouse_point.y - ui->mouse_point_last.y;


					f32 zoomCopy = element->selectable_image.zoom;
					//coordinates on image

					// ;delete?
					element->selectable_image.displayGrid = 0;

					//Image 
					ui_update_element_at(ui, _element, elementCursor.x, elementCursor.y, selectableImageSize.x, selectableImageSize.y);
					u32 element_interacting = ui_element_interacting(ui, element->id);
					u32 element_hot         = ui_element_hot(ui, element->id);

					//Image hot
					if(element_hot)
					{
						f32 oldZoom = zoomCopy;
	   		            i16 wheelDir = ui->mouse_wheel;
	   		            zoomCopy    += wheelDir * 1.0f;
						zoomCopy     = zoomCopy < 1.0f ? 1.0f : zoomCopy >= 12.0f ? 12.0f : zoomCopy;
						element->selectable_image.zoom = zoomCopy;
						//zoom towards mouse
						if(wheelDir)
						{
#if 1
					        f32 cursorImageDeltaX = ((ui->mouse_point.x) - (elementCursor.x + element->selectable_image.clip_offset_x));
					        f32 cursorImageDeltaY = ((ui->mouse_point.y) - (elementCursor.y + element->selectable_image.clip_offset_y)); 
							f32 scaledDiv = zoomCopy / oldZoom;
							
							element->selectable_image.clip_offset_x = (i32)(-1 * ((cursorImageDeltaX * scaledDiv) - (ui->mouse_point.x - elementCursor.x)));
							element->selectable_image.clip_offset_y = (i32)(-1 * ((cursorImageDeltaY * scaledDiv) - (ui->mouse_point.y - elementCursor.y)));
#endif

						}

						if(ui->input->mouse_middle)
						{

							element->selectable_image.clip_offset_x += (i32)mouse_delta_x;
							element->selectable_image.clip_offset_y += (i32)mouse_delta_y;

						}
					}

					f32 texture_array_wh_half = 512 / 2;

					f32 cox = (f32)element->selectable_image.clip_offset_x / zoomCopy;
					f32 coy = (f32)element->selectable_image.clip_offset_y / zoomCopy;
					if(cox > texture_array_wh_half)
					{
						element->selectable_image.clip_offset_x = (i32)(texture_array_wh_half * zoomCopy);
					}
					else if(cox < (-texture_array_wh_half))
					{
						element->selectable_image.clip_offset_x = (i32)((-texture_array_wh_half) * zoomCopy);
					}
					if(coy > texture_array_wh_half)
					{
						element->selectable_image.clip_offset_y = (i32)(texture_array_wh_half * zoomCopy);
					}
					else if(coy < (-texture_array_wh_half))
					{
						element->selectable_image.clip_offset_y = (i32)((-texture_array_wh_half) * zoomCopy);
					}

					i32 clip_offset_x = element->selectable_image.clip_offset_x;
					i32 clip_offset_y = element->selectable_image.clip_offset_y;


					f32 imageSizeX       = (f32)element->selectable_image.texture->width;
					f32 imageSizeY       = (f32)element->selectable_image.texture->height;
					f32 imageSizeXScaled = imageSizeX * zoomCopy;
					f32 imageSizeYScaled = imageSizeY * zoomCopy;

					//Frame background
					render_draw_rectangle_2D(
							ui->renderCommands,
							elementCursor.x,
							elementCursor.y,
							selectableImageSize.x,
							selectableImageSize.y,
							V4(10, 10, 10, 255));
					//Frame Borders
					render_rectangle_borders_2D(
							ui->renderCommands,
							elementCursor.x,
							elementCursor.y,
							selectableImageSize.x,
							selectableImageSize.y,
							1,
							frameBorderColor);
				
					//Clip
					ui_push_clip_inside_last_XYWH(ui, (i32)elementCursor.x, (i32)elementCursor.y, (i32)selectableImageSize.x, (i32)selectableImageSize.y);
					//
					//Background
					//
					vec4 backgroundColors[2] = {
					             {050, 050, 050, 255},
					             {100, 100, 100, 255},
					};
					u32 backgroundColorIndex = 0;
					u32 backgroundColorStart = 0;

					//size of the background "squares"
					f32 backWH       = 16;
					f32 backWHScaled = backWH * zoomCopy;

					//Multiply by two to advance by two blocks and keep the colors
					f32 background_x_clamped = clip_offset_x > 0 ? 0 : (f32)clip_offset_x;
					f32 background_y_clamped = clip_offset_y > 0 ? 0 : (f32)clip_offset_y;
					i32 backClipDistanceX = (i32)(background_x_clamped / (backWHScaled * 2));
					i32 backClipDistanceY = (i32)(background_y_clamped / (backWHScaled * 2));
					i32 wIndexStart       = -1 * (i32)(backClipDistanceX * (backWHScaled * 2));
					i32 hIndexStart       = -1 * (i32)(backClipDistanceY * (backWHScaled * 2));
					i32 wIndex            = wIndexStart;
					i32 hIndex            = hIndexStart;

					i32 wEnd = (i32)(imageSizeX - clip_offset_x);
					i32 hEnd = (i32)(imageSizeY - clip_offset_y);

					while(hIndex < (selectableImageSize.y - clip_offset_y) && hIndex < imageSizeYScaled)
					{
						backgroundColorStart = backgroundColorIndex;
					   while(wIndex < (selectableImageSize.x - clip_offset_x) && wIndex < imageSizeXScaled)
					   {
						   backgroundColorIndex %= 2;

						   f32 backX = elementCursor.x + wIndex + clip_offset_x;
						   f32 backY = elementCursor.y + hIndex + clip_offset_y;
						   f32 backWidth  = backWH * zoomCopy * 1;
						   f32 backHeight = backWH * zoomCopy * 1;

						   if((wIndex + backWidth) > imageSizeXScaled)
						   {
							    backWidth -= ((wIndex + backWidth) - imageSizeXScaled);
						   }
						   if((hIndex + backHeight) > imageSizeYScaled)
						   {
							    backHeight -= ((hIndex + backHeight) - imageSizeYScaled);
						   }

					      render_draw_rectangle_2D(ui->renderCommands,
								             backX,
											 backY,
											 backWidth,
											 backHeight,
											 backgroundColors[backgroundColorIndex++]);
						  backWidth  -= backWH * zoomCopy;
						  backHeight -= backWH * zoomCopy;
						  //Multiply by two because two rectangles were drawn
						  wIndex += (i32)(backWH * zoomCopy);
					   }
					   backgroundColorIndex %= 2;
					   backgroundColorIndex += (backgroundColorStart == backgroundColorIndex);
					   if(backgroundColorIndex == 2)
					   {
						   backgroundColorIndex = 0;
					   }
					   //reset wIndex
					   wIndex = wIndexStart;
					   hIndex += (i32)(backWH * zoomCopy);
					}
					//
					//
					//

					//Image
					if(element->selectable_image.texture)
					{
						u32 image_w = element->selectable_image.texture->width;
						u32 image_h = element->selectable_image.texture->height;
				     	render_draw_sprite_2D(ui->renderCommands,
				     			            element->selectable_image.texture,
				     						V2(elementCursor.x + clip_offset_x, elementCursor.y + clip_offset_y),
				     						V2(imageSizeX * zoomCopy, imageSizeY * zoomCopy),
				     						vec4_all(255),
				     						0, 0, image_w, image_h);
					}

					//Current selected imageFrames
					i32 imageFrameX = (i32)(element->selectable_image.selectedPixelX * zoomCopy + clip_offset_x);
					i32 imageFrameY = (i32)(element->selectable_image.selectedPixelY * zoomCopy + clip_offset_y);
					i32 imageFrameW = (i32)(element->selectable_image.selectedPixelW * zoomCopy);
					i32 imageFrameH = (i32)(element->selectable_image.selectedPixelH * zoomCopy);

#if 0
					render_rectangle_borders_2D_u32(ui->renderCommands,
							                        elementCursor.x + imageFrameX,
													elementCursor.y + imageFrameY,
													imageFrameW,
													imageFrameH,
													3,
													V4(255, 0, 0, 255));
#endif
					u32 selectable_group_count = element->selectable_image.selectable_group_count;
					//Mouse inside the image
					f32 cursor_on_image_x = ((ui->mouse_point.x) - (elementCursor.x + clip_offset_x)) / zoomCopy;
					f32 cursor_on_image_y = ((ui->mouse_point.y) - (elementCursor.y + clip_offset_y)) / zoomCopy;

					u32 group_id_count = 10;
					for(u32 s = 0; s < selectable_group_count; s++)
					{
						struct ui_image_selection_group *selectable_group = selectable_image->selectable_group + s;
						f32 selected_alpha = selectable_image->selectable_group_selected == s ? 
							                 255.0f : 180.0f;

					    //drag and edit uvs after selection
						//all uvs
					    ui_id ui_id_drag_uvs = ui_id_NUMBER_POINTER(group_id_count++, element);
						//individual uvs
					    ui_id ui_id_drag_uv0 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv1 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv2 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv3 = ui_id_NUMBER_POINTER(group_id_count++, element);

						//edges
					    ui_id ui_id_drag_uv01 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv12 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv23 = ui_id_NUMBER_POINTER(group_id_count++, element);
					    ui_id ui_id_drag_uv03 = ui_id_NUMBER_POINTER(group_id_count++, element);

						ui_id ui_id_flip_uvs = ui_id_NUMBER_POINTER(group_id_count++, element);

					    vec4 selection_line_color = {200, 0, 0, selected_alpha};
						if(selectable_group->selection_type_uv)
						{
							//coordinates to show on the screen
					        vec2 uv0_selection = vec2_scale(selectable_group->uv0_scaled, zoomCopy);
					        vec2 uv1_selection = vec2_scale(selectable_group->uv1_scaled, zoomCopy);
					        vec2 uv2_selection = vec2_scale(selectable_group->uv2_scaled, zoomCopy);
					        vec2 uv3_selection = vec2_scale(selectable_group->uv3_scaled, zoomCopy);

					        uv0_selection.x += elementCursor.x + clip_offset_x;
					        uv0_selection.y += elementCursor.y + clip_offset_y;

					        uv1_selection.x += elementCursor.x + clip_offset_x;
					        uv1_selection.y += elementCursor.y + clip_offset_y;

					        uv2_selection.x += elementCursor.x + clip_offset_x;
					        uv2_selection.y += elementCursor.y + clip_offset_y;

					        uv3_selection.x += elementCursor.x + clip_offset_x;
					        uv3_selection.y += elementCursor.y + clip_offset_y;

							uv0_selection = vec2_round_to_int(uv0_selection);
							uv1_selection = vec2_round_to_int(uv1_selection);
							uv2_selection = vec2_round_to_int(uv2_selection);
							uv3_selection = vec2_round_to_int(uv3_selection);
					        //update
					        f32 selection_rec_size = 8.0f;
						    f32 selection_rec_size_half = selection_rec_size * 0.5f;

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

							//uv0_x = (f32)(i32)uv0_x;
							//uv0_y = (f32)(i32)uv0_y;
							//uv1_x = (f32)(i32)uv1_x;
							//uv1_y = (f32)(i32)uv1_y;
							//uv2_x = (f32)(i32)uv2_x;
							//uv2_y = (f32)(i32)uv2_y;
							//uv3_x = (f32)(i32)uv3_x;
							//uv3_y = (f32)(i32)uv3_y;

							//move interaction
							ui_create_update_element_at_baycentric(ui,
									                               ui_id_drag_uvs,
																   uv0_selection,
																   uv1_selection,
																   uv2_selection);

							ui_create_update_element_at_baycentric(ui,
									                               ui_id_drag_uvs,
																   uv0_selection,
																   uv2_selection,
																   uv3_selection);

						    //uv movement interaction
					        //bottom left
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv0,
					        		                    uv0_x,
					        							uv0_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);

					        //top left
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv1,
					        		                    uv1_x,
					        							uv1_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);

					        //top right
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv2,
					        		                    uv2_x,
					        							uv2_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);

					        //bottom right
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv3,
					        		                    uv3_x,
					        							uv3_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);

							//left
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv01,
					        		                    uv01_x,
					        							uv01_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);
							//top
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv12,
					        		                    uv12_x,
					        							uv12_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);
							//right
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv23,
					        		                    uv23_x,
					        							uv23_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);
							//bottom
					        ui_create_update_element_at(ui,
					        		                    ui_id_drag_uv03,
					        		                    uv03_x,
					        							uv03_y,
					        							selection_rec_size,
					        							selection_rec_size
					        							);

							u32 drag_interacting   = ui_element_interacting_flags(ui, ui_id_drag_uvs, ui_interaction_mouse_left_down);
					        bool8 drag_v0_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv0, ui_interaction_mouse_left_down);
					        bool8 drag_v1_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv1, ui_interaction_mouse_left_down);
					        bool8 drag_v2_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv2, ui_interaction_mouse_left_down);
					        bool8 drag_v3_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv3, ui_interaction_mouse_left_down);

					        bool8 drag_uv01_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv01, ui_interaction_mouse_left_down);
					        bool8 drag_uv12_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv12, ui_interaction_mouse_left_down);
					        bool8 drag_uv23_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv23, ui_interaction_mouse_left_down);
					        bool8 drag_uv03_interacting = ui_element_interacting_flags(ui, ui_id_drag_uv03, ui_interaction_mouse_left_down);

							u32 uv_flip_horizontally_interacted = ui_element_interacted_flags(
									ui, 
									ui_id_flip_uvs,
									ui_interaction_mouse_left_down);

							i32 uv_delta_x = 0;
							i32 uv_delta_y = 0;
							//if interacting with any
						    if(drag_interacting + 
							   drag_v0_interacting + 
							   drag_v1_interacting + 
							   drag_v2_interacting + 
							   drag_v3_interacting + 
							   drag_uv01_interacting +
							   drag_uv12_interacting +
							   drag_uv23_interacting +
							   drag_uv03_interacting +
							   uv_flip_horizontally_interacted)
						    {
						    	element->selectable_image.selectable_group_selected = s;
					        	element->selectable_image.uvsUpdated = 1;

								//used to calculate delta with uvs
					            i32 cursor_on_image_last_x = (i32)(((ui->mouse_point_last.x) - (elementCursor.x + clip_offset_x)) / zoomCopy);
					            i32 cursor_on_image_last_y = (i32)(((ui->mouse_point_last.y) - (elementCursor.y + clip_offset_y)) / zoomCopy);

								uv_delta_x = ((i32)cursor_on_image_x - cursor_on_image_last_x);
								uv_delta_y = ((i32)cursor_on_image_y - cursor_on_image_last_y);
						    }
							if(drag_interacting)
							{

								selectable_group->uv0_scaled.x += uv_delta_x;
								selectable_group->uv0_scaled.y += uv_delta_y; 

								selectable_group->uv1_scaled.x += uv_delta_x;  
								selectable_group->uv1_scaled.y += uv_delta_y;  

								selectable_group->uv2_scaled.x += uv_delta_x;  
								selectable_group->uv2_scaled.y += uv_delta_y;  

								selectable_group->uv3_scaled.x += uv_delta_x;  
								selectable_group->uv3_scaled.y += uv_delta_y;  
							}
							if(drag_v0_interacting || drag_uv01_interacting || drag_uv03_interacting)
					        {
								selectable_group->uv0_scaled.x += uv_delta_x;
								selectable_group->uv0_scaled.y += uv_delta_y; 

					        }
					        if(drag_v1_interacting || drag_uv01_interacting || drag_uv12_interacting)
					        {
					        	selectable_group->uv1_scaled.x += uv_delta_x;
					        	selectable_group->uv1_scaled.y += uv_delta_y; 

					        }
					        if(drag_v2_interacting || drag_uv12_interacting || drag_uv23_interacting)
					        {
					        	selectable_group->uv2_scaled.x += uv_delta_x;
					        	selectable_group->uv2_scaled.y += uv_delta_y; 

					        }
					        if(drag_v3_interacting|| drag_uv23_interacting || drag_uv03_interacting)
					        {
					        	selectable_group->uv3_scaled.x += uv_delta_x;
					        	selectable_group->uv3_scaled.y += uv_delta_y; 
					        }

							if(uv_flip_horizontally_interacted)
							{
								render_uvs flipped_uvs = render_flip_uvs_horizontally(selectable_group->uv0_scaled,
										                                              selectable_group->uv1_scaled,
										                                              selectable_group->uv2_scaled,
										                                              selectable_group->uv3_scaled
																					  );
                                selectable_group->uv0_scaled = flipped_uvs.uv0;
							    selectable_group->uv1_scaled = flipped_uvs.uv1;
							    selectable_group->uv2_scaled = flipped_uvs.uv2;
							    selectable_group->uv3_scaled = flipped_uvs.uv3;
																					  
							}
					        //draw uv squares
						    
					        vec4 uv_drag_color     = vec4_all(255);


							//;TODO add center squares to select by frame
							//render lines attached to the uv coordinates
                            render_line_2d_down(ui->renderCommands,
                            			   uv0_selection,
                            		       uv1_selection,
                            			   3,
                            			   selection_line_color);

                            render_line_2d_down(ui->renderCommands,
                            		       uv1_selection,
                            			   uv2_selection,
                            			   3,
                            			   selection_line_color);

                            render_line_2d_up(ui->renderCommands,
                            		       uv2_selection,
                            			   uv3_selection,
                            			   3,
                            			   selection_line_color);

                            render_line_2d_down(ui->renderCommands,
                            		       uv0_selection,
                            			   uv3_selection,
                            			   3,
                            			   selection_line_color);

							//border selections for individual uvs
					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv0_x,
					        							    uv0_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv1_x,
					        							    uv1_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);
					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv2_x,
					        							    uv2_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv3_x,
					        							    uv3_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);
							//side selections

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv01_x,
					        							    uv01_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv12_x,
					        							    uv12_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv23_x,
					        							    uv23_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        uv03_x,
					        							    uv03_y,
					        								selection_rec_size,
					        								selection_rec_size,
					        								2,
					        							    uv_drag_color);

							
							//flip uvs button
							vec2 uv_selection_max = vec2_max(uv0_selection, uv1_selection);
							     uv_selection_max = vec2_max(uv_selection_max, vec2_max(uv2_selection, uv3_selection));
							vec2 uv_selection_min = vec2_min(uv0_selection, uv1_selection);
							     uv_selection_min = vec2_min(uv_selection_min, vec2_min(uv2_selection, uv3_selection));

							vec2 flip_arrow_p;
							flip_arrow_p.x = uv_selection_min.x + (uv_selection_max.x - uv_selection_min.x) * 0.5f;
							flip_arrow_p.y = uv_selection_min.y - 8;

							f32 flip_arrow_w = 16.0f;
							f32 flip_arrow_h = 8;

					        render_draw_rectangle_2D(ui->renderCommands,
							                         flip_arrow_p.x,
									                 flip_arrow_p.y,
									                 16,
									                 flip_arrow_h,
									                 V4(255, 255, 255, 255));

							ui_create_update_element_at(ui, 
									                        ui_id_flip_uvs,
															flip_arrow_p.x,
															flip_arrow_p.y,
															flip_arrow_w,
															flip_arrow_h);
							if(selectable_group->label)
							{
                                f32 text_height = 18.0f; //20.0f
                                f32 text_scale = text_height /  ui->fontp.font_height;

								//vec2 font_size = font_get_text_size_scaled(
								//		&ui->fontp,
								//		text_scale,
								//		selectable_group->label);

								//position text to center
								vec2 label_p = {
                                uv_selection_min.x + (uv_selection_max.x - uv_selection_min.x) * 0.5f,
                                uv_selection_min.y + (uv_selection_max.y - uv_selection_min.y) * 0.5f
								};

								//render text
								render_text_2d_no_wrap(ui->renderCommands,
										            &ui->fontp,
													label_p.x,
													label_p.y,
													text_scale,
													vec4_all(255),
													selectable_group->label);
							}


								 


						}
						else
						{
							u32 interacting = ui_element_interacting_flags(ui,
									                                       ui_id_drag_uvs,
									                                       ui_interaction_mouse_left_down);
							if(interacting)
							{
							    element->selectable_image.selectable_group_selected = s;
					    	    element->selectable_image.uvsUpdated = 1;

					            i32 cursor_on_image_last_x = (i32)(((ui->mouse_point_last.x) - (elementCursor.x + clip_offset_x)) / zoomCopy);
					            i32 cursor_on_image_last_y = (i32)(((ui->mouse_point_last.y) - (elementCursor.y + clip_offset_y)) / zoomCopy);

								i32 distance_rec_mouse_last_x = cursor_on_image_last_x - selectable_group->frame_x;
								i32 distance_rec_mouse_last_y = cursor_on_image_last_y - selectable_group->frame_y;

								selectable_group->frame_x = (i32)cursor_on_image_x - distance_rec_mouse_last_x;
								selectable_group->frame_y = (i32)cursor_on_image_y - distance_rec_mouse_last_y;
							}


							f32 selection_rec_x = selectable_group->frame_x * zoomCopy;
							f32 selection_rec_y = selectable_group->frame_y * zoomCopy;
							f32 selection_rec_w = selectable_group->frame_w * zoomCopy;
							f32 selection_rec_h = selectable_group->frame_h * zoomCopy;

                            selection_rec_x += elementCursor.x + clip_offset_x;
                            selection_rec_y += elementCursor.y + clip_offset_y;
                            //selection_rec_w += elementCursor.x + clip_offset_x;
                            //selection_rec_h += elementCursor.y + clip_offset_y;
							ui_create_update_element_at(ui,
									                    ui_id_drag_uvs,
														selection_rec_x,
														selection_rec_y,
														selection_rec_w,
														selection_rec_h);


					        render_rectangle_borders_2D_u32(ui->renderCommands,
					        		                        selection_rec_x,
					        							    selection_rec_y,
					        								selection_rec_w,
					        								selection_rec_h,
					        								2,
					        							    selection_line_color);
						}

					} //end of loop

					//Mouse selection

				    f32 distanceMouseLayoutX       = (ui->mouse_point.x - elementCursor.x);
				    f32 distanceMouseLayoutY       = (ui->mouse_point.y - elementCursor.y);
				    i32 distanceMouseLayoutScaledX = (i32)((distanceMouseLayoutX) / zoomCopy);
				    i32 distanceMouseLayoutScaledY = (i32)((distanceMouseLayoutY) / zoomCopy);
#if 0
					render_draw_rectangle_2D(ui->renderCommands,
							           elementCursor.x + distanceMouseLayoutX,
									   elementCursor.y + distanceMouseLayoutY,
									   2 ,
									   2 ,
									   V4(255, 255, 0, 255));
#endif


					//render_draw_rectangle_2D(ui->renderCommands,
					//		           cursor_position_lixelOnImageX + elementCursor.x,
					//		           cursor_position_lixelOnImageX + elementCursor.y,
					//				   2 ,
					//				   2 ,
					//				   V4(255, 255, 0, 255));
					//Mouse in image and screen
					i32 mousePixelX = (i32)(((i32)(cursor_on_image_x) * zoomCopy) + clip_offset_x + elementCursor.x);
					i32 mousePixelY = (i32)(((i32)(cursor_on_image_y) * zoomCopy) + clip_offset_y + elementCursor.y);

					i32 mousePixelW = (i32)zoomCopy;
					i32 mousePixelH = (i32)zoomCopy;
					u32 interactingMouseLeft  = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_left_down);
					u32 interactingMouseRight = ui_element_interacting_flags(ui, element->id, ui_interaction_mouse_right_down);

					u32 cursor_is_inside_image = (cursor_on_image_x < imageSizeX) && (cursor_on_image_y < imageSizeY);
					u32 selected_valid_group   = selectable_image->selectable_group_selected < selectable_image->selectable_group_count;

					if(interactingMouseLeft && cursor_is_inside_image && selected_valid_group) 
					{
						struct ui_image_selection_group *selectable_group = selectable_image->selectable_group + selectable_image->selectable_group_selected;

						i32 tileSize_w = element->selectable_image.tileSize_w;
						i32 tileSize_h = element->selectable_image.tileSize_h;

						i32 mouseHoldX = (element->selectable_image.mouseHoldX / tileSize_w) * tileSize_w;
						i32 mouseHoldY = (element->selectable_image.mouseHoldY / tileSize_h) * tileSize_h;


						//Selection X
					    i32 cursor_tiled_x = (i32)(cursor_on_image_x / tileSize_w) * tileSize_w;
					    i32 cursor_tiled_y = (i32)(cursor_on_image_y / tileSize_h) * tileSize_h;

						i32 distanceFromMouseHoldX = (i32)((cursor_tiled_x - mouseHoldX));
						i32 distanceFromMouseHoldY = (i32)((cursor_tiled_y - mouseHoldY));
						i32 cursor_tiled_w = distanceFromMouseHoldX;
						i32 cursor_tiled_h = distanceFromMouseHoldY;

						i32 selected_x = 0;
						i32 selected_y = 0;
						i32 selected_w = 0;
						i32 selected_h = 0;
						if(distanceFromMouseHoldX >= 0)
						{
							selected_x = cursor_tiled_x - distanceFromMouseHoldX;
							selected_w = distanceFromMouseHoldX;
						}
						else
						{
							selected_x = cursor_tiled_x;
							selected_w = -(distanceFromMouseHoldX);
						}
						//Selection Y

						if(distanceFromMouseHoldY >= 0)
						{

							selected_y = cursor_tiled_y - distanceFromMouseHoldY; 
							selected_h = distanceFromMouseHoldY;
						}
						else
						{
							selected_y = cursor_tiled_y;
							selected_h = -distanceFromMouseHoldY;
						}

						selected_w = selected_w >= 0 ? selected_w + tileSize_w : selected_w;
						selected_h = selected_h >= 0 ? selected_h + tileSize_h : selected_h;

					    //selected_x = (i32)(selected_x / tileSize_w) * tileSize_w;
					    //selected_y = (i32)(selected_y / tileSize_h) * tileSize_h;
					    //selected_w = (i32)(selected_w / tileSize_w) * tileSize_w;
					    //selected_h = (i32)(selected_h / tileSize_h) * tileSize_h;

						//selected_w = selected_w < tileSize_w ? tileSize_w : selected_w;
						//selected_h = selected_h < tileSize_h ? tileSize_h : selected_h;



						if(selectable_group->selection_type_uv)
						{

						    f32 one_over_w = element->selectable_image.texture->width;
						    f32 one_over_h = element->selectable_image.texture->height;

						    vec2 uv1_scaled = {(f32)selected_x, (f32)selected_y};
						    vec2 uv3_scaled = {uv1_scaled.x + selected_w, 
						                       uv1_scaled.y + selected_h};
						    vec2 uv0_scaled = {uv1_scaled.x, uv3_scaled.y};
						    vec2 uv2_scaled = {uv3_scaled.x, uv1_scaled.y};

						    selectable_group->uv0_scaled = uv0_scaled;
						    selectable_group->uv1_scaled = uv1_scaled;
						    selectable_group->uv2_scaled = uv2_scaled;
						    selectable_group->uv3_scaled = uv3_scaled;
						}
						else
						{
					        selectable_group->frame_x = selected_x;
					        selectable_group->frame_y = selected_y;
					        selectable_group->frame_w = selected_w;
					        selectable_group->frame_h = selected_h;
						}



					    mousePixelX = (i32)(selected_x * zoomCopy + clip_offset_x + elementCursor.x);
					    mousePixelY = (i32)(selected_y * zoomCopy + clip_offset_y + elementCursor.y);
						mousePixelW = (i32)(selected_w * zoomCopy);
						mousePixelH = (i32)(selected_h * zoomCopy);

						element->selectable_image.uvsUpdated = 1;
						

					} //if interacting
					else
					{
						//Keep track on where the mouse will be located
						element->selectable_image.mouseHoldX = (i32)cursor_on_image_x;
						element->selectable_image.mouseHoldY = (i32)cursor_on_image_y;
					}
					//mouse selection
					render_rectangle_borders_2D_u32(ui->renderCommands,
													mousePixelX,
													mousePixelY,
													mousePixelW,
													mousePixelH,
													3,
													V4(255, 255, 0, 255));

					element->selectable_image.gridW = 16;
					element->selectable_image.gridH = 16;
					if(element->selectable_image.displayGrid)
					{
					    i16 gridX = element->selectable_image.gridX;
					    i16 gridY = element->selectable_image.gridY;
					    i16 gridW = element->selectable_image.gridW;
					    i16 gridH = element->selectable_image.gridH;
						if(gridW && gridH)
						{
						    vec4 gridColor = {0, 0, 0xFF, 0xFF};

					        // SelectableImage grid
							//vertical lines
					        f32 lineX = 0;
					        i32 limitX = (i32)(elementCursor.x + selectableImageSize.x);
					        for(u32 x = 1; lineX < limitX; x++)
					        {
					        	lineX        = 0 + elementCursor.x + clip_offset_x + (x * gridW * zoomCopy);
					        	f32 lineYEnd = elementCursor.y + selectableImageSize.y;

					        	render_draw_rectangle_2D(ui->renderCommands,
						    			           lineX,
						    					   elementCursor.y + 1,
						    					   (1.0f),
						    					   selectableImageSize.y, gridColor);
					        }

							//horizontal lines
					        f32 lineY = 0; 
					        i32 limitY = (i32)(elementCursor.y + selectableImageSize.y);
					        for(u32 y = 1; lineY < limitY; y++)
					        {
					        	lineY       = 0 + elementCursor.y + clip_offset_y + (y * gridH * zoomCopy);
					        	f32 lineEnd = selectableImageSize.x;

					        	render_draw_rectangle_2D(ui->renderCommands,
					        			           elementCursor.x,
					        					   lineY,
					        					   lineEnd,
					        					   1.0f, gridColor);
					        }
						}
					}
					//mouse coodinates text
					u8 textBuffer[96] = {0};
					i32 selectedPixelX = (i32)(distanceMouseLayoutScaledX - clip_offset_x / zoomCopy);
					i32 selectedPixelY = (i32)(distanceMouseLayoutScaledY - clip_offset_y / zoomCopy);
					FormatText(textBuffer, sizeof(textBuffer), "%d ,%d {mousePixels: x:%d, y:%f} {clipOffset: x:%f y:%f}", selectedPixelX, selectedPixelY, (i32)cursor_on_image_x, cursor_on_image_y, clip_offset_x, clip_offset_y);
					render_text_2d_no_wrap(ui->renderCommands, &ui->fontp, (elementCursor.x + imageSizeX) - 256, (elementCursor.y + imageSizeY) - (ui->fontp.font_height * ui->font_scale) - 4, ui->font_scale, ui->theme.textColor, textBuffer);

					u32 mouseLeftDown = ui->mouse_l_down;

					u32 mouseWasHolding = element->selectable_image.mouseWasHolding;
                    element->selectable_image.mouseWasHolding = element_interacting;

					render_commands_PopClip(ui->renderCommands);

					elementCursor.y += selectableImageSize.y;


					f32 advancedY = elementCursor.y - layoutCursor.y;
					ui_advance(ui, layoutCursor.x, layoutCursor.y, selectableImageSize.x, advancedY);
					

				 }break;
			 case ui_command_type_explorer:
				 {
					 panel_commands_at += sizeof(ui_command_type);

					 ui_explorer *explorer = ui->explorer;

					 //Switched directory, completed process, etc...
					 if(explorer->update_path_files)
					 {
						 explorer->update_path_files = 0;
			             platform_api *platform    = explorer->platform;

			             platform_file_search_info fileData = {0};
						 //combine directory name with pattern
						 //FormatText(textBuffer, sizeof(textBuffer), "%s*", explorer->directory_name);
						 
						 u32 totalFileSlots = ARRAYCOUNT(explorer->current_directory_files);
						 //Search directories
						 explorer->directory_file_count = 0;
						 u8 textBuffer[312] = {0};
						 FormatText(textBuffer, sizeof(textBuffer), "%s*", explorer->directory_name);
						 platform_file_search directorySearch = platform->f_find_first_file(textBuffer, &fileData);

						 if(directorySearch.handle)
						 {
							 //Ignore '.' and '..' directories.
							 (platform->f_find_next_file(directorySearch, &fileData));

						    //Pick folder from array
                             platform_file_info_details *currentFile = explorer->current_directory_files + explorer->directory_file_count;

			                 while(platform->f_find_next_file(directorySearch, &fileData))
			                 {
								 //Pick file
								 if(fileData.is_directory)
								 {
                                     platform_file_info_details *currentFile = explorer->current_directory_files + explorer->directory_file_count;
								     currentFile->is_directory = fileData.is_directory;
								     currentFile->info.date   = fileData.write_time;
								     string_copy(fileData.name, currentFile->name);
								     string_copy(explorer->directory_name, currentFile->path);

								     explorer->directory_file_count++;
								     Assert(explorer->directory_file_count < totalFileSlots); 
								 }
								 //Advance
			                 }

			                 platform->f_find_close(directorySearch);
						 }
						 //Search by wildcard
						 FormatText(textBuffer, sizeof(textBuffer), "%s%s", explorer->directory_name, explorer->search_pattern);

			             platform_file_search fileSearch = platform->f_find_first_file(textBuffer, &fileData);//, platform_file_type_Normal);
			             if(fileSearch.handle)
			             {
			                 do 
			                 {
								 //Pick file
								 if(!fileData.is_directory)
								 {
                                    platform_file_info_details *currentFile = explorer->current_directory_files + explorer->directory_file_count;
								    string_copy(fileData.name, currentFile->name);
								    string_copy(explorer->directory_name, currentFile->path);
								    currentFile->is_directory = 0;
								    currentFile->info.size  = fileData.size;
								    currentFile->info.date  = fileData.write_time;
								    Assert(explorer->directory_file_count < totalFileSlots);
								    explorer->directory_file_count++;
								 }
								 //Set data
			                 }while(platform->f_find_next_file(fileSearch, &fileData));

			                 platform->f_find_close(fileSearch);
			             }
					 }


					 vec2 layoutCursor = ui_get_layout_cursor(ui);
					 vec2 explorerCursor = layoutCursor;
					 f32 totalExplorerW = 400;
					 f32 totalExplorerH = 400;

					 //Advance by total size
					 //ui_advance(ui, layoutCursor.x, layoutCursor.y, totalExplorerW, totalExplorerH);

					 ui_id explorerInputId = ui_id_NUMBER_POINTER(explorer->directory_name, explorer);
					 ui_id searchInputId = ui_id_NUMBER_POINTER(explorer->search_pattern, explorer);
					 //current directory input
					 f32 textInputH = 30;

					 //Input directory
					 render_text_2d_no_wrap(ui->renderCommands, &ui->fontp, explorerCursor.x, explorerCursor.y, ui->font_scale, ui->theme.textColor, "Directory");
					 explorerCursor.y += ui_FontHeight(ui) + 4;
					 //Subtract 2 to the total length to save space for the null character
                     u32 enteredDirectory = _ui_update_render_input_text(ui, explorerInputId, explorerCursor.x, explorerCursor.y, totalExplorerW, textInputH, 1, sizeof(explorer->directory_name) - 2, explorer->directory_name);
					 explorerCursor.y += textInputH + 4;

					 //Input search pattern
					 render_text_2d_no_wrap(ui->renderCommands, &ui->fontp, explorerCursor.x, explorerCursor.y, ui->font_scale, ui->theme.textColor, "Search pattern");
					 explorerCursor.y += ui_FontHeight(ui) + 4;
                     u32 enteredPattern = _ui_update_render_input_text(ui, searchInputId, explorerCursor.x, explorerCursor.y, totalExplorerW, textInputH, 0, sizeof(explorer->search_pattern), explorer->search_pattern);
					 explorerCursor.y += textInputH + 4;

					 if(enteredDirectory)
					 {
						 explorer->update_path_files = 1;
						 u32 directoryLength = string_copy(ui->input_text->buffer, explorer->directory_name);
						 if(directoryLength)
						 {
						     u8 c = explorer->directory_name[directoryLength - 1];
						     //Make sure it always ends as a valid path to combine with the search pattern
						     if((c != '/') || (c != '\\'))
						     {
						         explorer->directory_name[directoryLength] = '/';
						         explorer->directory_name[directoryLength + 1] = '\0';
						     }
						 }
					 }
					 if(enteredPattern)
					 {
						 explorer->update_path_files = 1;
						 //u32 textLength = string_copy(ui->input_text->buffer, explorer->search_pattern);
						 u32 c = explorer->search_pattern[0];
						 if(c == '\0')
						 {
							 explorer->search_pattern[0] = '*';
							 explorer->search_pattern[1] = '\0';
						 }
						 else
						 {
							 string_removeandtruncate(explorer->search_pattern, '/');
							 string_removeandtruncate(explorer->search_pattern, '\\');
							 //input_text_CheckCursor(ui->input_text);
						 }
					 }
					 //Background
					 render_draw_rectangle_2D(ui->renderCommands, explorerCursor.x,
							                                explorerCursor.y,
															totalExplorerW,
															totalExplorerH, ui->theme.frame_background_color);
					 //Borders
					 render_rectangle_borders_2D(ui->renderCommands,
							                   explorerCursor.x,
											   explorerCursor.y,
											   totalExplorerW,
											   totalExplorerH, 1, ui->theme.frameBorderColor);
					 //data and colors
					 f32 nameOffset = 4;
	                 vec4 selectionDisabledColor    = {0, 0, 0, 160};
	                 vec4 selectionNormalColor      = {0, 0, 0, 0};
	                 vec4 selectionHotColor         = ui->theme.button_hot_color; 
	                 vec4 selectionInteractingColor = ui->theme.button_interacting_color;


					 //Display file names
					 ui_PushClip_XYWH(ui, (i32)explorerCursor.x, (i32)explorerCursor.y, (i32)totalExplorerW, (i32)totalExplorerH);

					 //render the ".." directory
					 if(explorer->path_length > 1)
					 {
						 ui_id previousPathId = ui_id_NUMBER_POINTER(0, explorer);
						 //u32 elementInteracted = ui_element_interacting_flags(ui, previousPathId, ui_interaction_mouse_left_down);
						 //u32 doubleClicked = ui_element_hot(ui, previousPathId) && ui->input->doubleClickedLeft;
						 u32 doubleClicked = ui_element_interacted_flags(ui, previousPathId, ui_interaction_mouse_left_double_click);
						 if(doubleClicked)
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
						 }

						 f32 pX = explorerCursor.x + nameOffset;
						 f32 pY = explorerCursor.y + nameOffset;
						 f32 pW = totalExplorerW;
						 f32 pH = ui_FontHeight(ui);
						 ui_create_update_element_at(ui, previousPathId, pX, pY, pW, pH);
						 //Render selectable
					     ui_render_interactive_text(
								 ui,
								 previousPathId,
								 ui_interaction_mouse_left_down,
								 pX,
								 pY,
								 pW,
								 pH,
								 selectionNormalColor,
								 selectionInteractingColor,
								 selectionHotColor,
								 font_text_pad_left,
								 "..");

						 explorerCursor.y += ui_FontHeight(ui);

					 }
					 //
					 //update and render all files and directories.
					 //
					 f32 fileHeight = ui_FontHeight(ui);
					 u32 file_got_clicked = 0;
					 for(u32 f = 0;
							 f < explorer->directory_file_count;
							 f++)
					 {
						 ui_id fileId = ui_id_NUMBER_POINTER(f + 1, explorer);
						 f32 fileX = explorerCursor.x + nameOffset;
						 f32 fileY = explorerCursor.y + nameOffset + (ui_FontHeight(ui) * f);
						 f32 fileW = totalExplorerW;
						 f32 fileH = ui_FontHeight(ui);

						 u32 elementInteracted = ui_element_interacted_flags(ui, fileId, ui_interaction_mouse_left_down | ui_interaction_mouse_left_double_click);
						 u32 doubleClicked     = elementInteracted && ui->interacted_flags & ui_interaction_mouse_left_double_click;
						 vec4 fileNormalColor = selectionNormalColor;


                         platform_file_info_details *currentFile = explorer->current_directory_files + f;
								 
						 if(elementInteracted)
						 {
							 file_got_clicked = 1;
							 if(!currentFile->is_directory)
							 {
								 explorer->selected_file_index = f;
								 explorer->file_got_selected   = 1;
								 //detect if it is a .pack format!

								 if(doubleClicked)
								 {
									 if(path_IsExtension(currentFile->name, "pack"))
									 {
										 explorer->current_selected_file_is_pack = 1;
										 //ui_explorer_set_path(ui, currentFile->path_and_name);
									 }
								 }


							 }
						 }

						 if(currentFile->is_directory && doubleClicked)
						 {
						     //Advance
						     u32 dirNameLength     = string_count(currentFile->name);
							 //Remove null character
						     u32 currentPathLength = string_count(explorer->directory_name) - 1;
						     u32 newLength = dirNameLength + currentPathLength;
						     Assert(newLength < 260);

						     u32 i = currentPathLength;
						     u32 c = 0;
						     while(c < dirNameLength)
						     {
						    	 explorer->directory_name[i] = currentFile->name[c];
						    	 i++;
						    	 c++;
						     }
						     explorer->directory_name[i - 1] = '/';
						     explorer->path_length = newLength;

						     explorer->update_path_files = 1;
						 }

						 ui_create_update_element_at(ui, fileId, fileX, fileY, fileW, fileH);
						 //Render selectable
						 if(explorer->selected_file_index == f)
						 {
							 fileNormalColor = selectionHotColor;
							 //if got clicked, ignore this part for this frame
							 if(!file_got_clicked)
							 {
								 fileNormalColor.w = explorer->files_focused ? 
									 fileNormalColor.w : fileNormalColor.w * 0.5f;
							 }
						 }
					     ui_render_interactive_text(ui,
								 fileId, ui_interaction_mouse_left_down,
								                         fileX + 20, fileY, fileW, fileH,
					     								 fileNormalColor,
					     								 selectionInteractingColor,
					     								 selectionHotColor,
					     								 font_text_pad_left,
					     								 currentFile->name);

						 if(currentFile->is_directory)
						 {
                            ui_draw_small_folder_icon(
									 ui,
									 fileX,
									 fileY,
									 12);
						 }
						 else
						 {
							 //Icon
							 ui_draw_small_default_file_icon(
									 ui,
									 fileX + 2,
									 fileY,
									 14);
							
						 }
					 }
					 render_commands_PopClip(ui->renderCommands);
					 //set the explorer focus if any file got clicked
					 if(ui->element_transition && (ui->interacted_flags &ui_interaction_mouse_left_down)) 
					 {
						 explorer->files_focused = file_got_clicked;
					 }
					 //Set selected file full path and name

					 //
					 //update explorer input
					 //
					 u32 selected_with_enter = explorer->files_focused && ui->input->enter;
					 if(explorer->files_focused)
					 {
						 if(explorer->directory_file_count)
						 {
							 game_input *io = ui->input;
							 i32 fi = explorer->selected_file_index;
							 if(input_pressed(io->input.up))
							 {
								 fi--;
							 }
							 else if(input_pressed(io->input.down))
							 {
								 fi++;
							 }
							 fi = fi >= explorer->directory_file_count ?
								 fi - 1 : fi < 0 ? 0 : fi;
							 if(explorer->selected_file_index != fi)
							 {
								 explorer->file_got_selected = 1;
							 }
							 explorer->selected_file_index = fi;
						 }
					 }
					 if(explorer->file_got_selected)
					 {
						 //Combine the file name with the relative path
						 platform_file_info_details *selectedFile = explorer->current_directory_files + explorer->selected_file_index;

						 u8 *directory_name        = explorer->directory_name;
						 u8 *selected_file_path_and_name = explorer->current_directory_files[explorer->selected_file_index].name;
						 memory_clear(explorer->selected_file_path_and_name, sizeof(explorer->selected_file_path_and_name));
						 string_concadenate(directory_name, selected_file_path_and_name, explorer->selected_file_path_and_name, sizeof(explorer->selected_file_path_and_name));

						 if(explorer->flags & ui_explorer_flags_copy_selected_file_name)
						 {
							 memory_clear(explorer->process_file_name, sizeof(explorer->process_file_name));
							 string_copy(selectedFile->name, explorer->process_file_name);
						 }

					 }

					 explorerCursor.y += totalExplorerH + 4;

					 ui_id fileNameId = UIIDFROMPTR(explorer->process_file_name, explorer);
					 //Input search pattern
					 render_text_2d_no_wrap(
							 ui->renderCommands,
							 &ui->fontp,
							 explorerCursor.x, 
							 explorerCursor.y,
							 ui->font_scale,
							 ui->theme.textColor, "File name");

					 explorerCursor.y += ui_FontHeight(ui) + 4;
                     u32 enteredName = _ui_update_render_input_text(
							 ui,
							 fileNameId,
							 explorerCursor.x,
							 explorerCursor.y,
							 totalExplorerW,
							 textInputH,
							 0,
							 sizeof(explorer->process_file_name),
							 explorer->process_file_name);
					 explorerCursor.y += textInputH + 4;

					 //Buttons
					 ui_id buttonOkId     = UIIDFROMPTR(&explorer->okay_pressed ,explorer);
					 ui_id buttonCancelId = UIIDFROMPTR(&explorer->cancel_pressed ,explorer);
					 vec4 explorerButtonNormal      = {150, 90, 0, 255};
					 vec4 explorerButtonHot         = {180, 120, 0, 255};
					 vec4 explorerButtonInteracting = {120, 70, 0, 255};

					 f32 explorerButtonsW = 80;
					 f32 explorerButtonsH = 20;

					 f32 btnOkX     = explorerCursor.x;
					 f32 btnCancelX = btnOkX + explorerButtonsW + 4; 
//font_get_text_pad_offset(font_proportional *fontData, f32 sizeX, f32 sizeY, u8 *text, f32 scale, font_text_pad padOptions)
					 ui_create_update_element_at(ui, buttonOkId,
							                     explorerCursor.x,
												 explorerCursor.y,
												 explorerButtonsW,
												 explorerButtonsH);

					 ui_create_update_element_at(ui, buttonCancelId,
							                     btnCancelX, 
												 explorerCursor.y,
												 explorerButtonsW,
												 explorerButtonsH);
					 //All of this shit for a button...
                     ui_render_interactive_text(ui, buttonOkId, ui_interaction_mouse_left_down,
				        		                   explorerCursor.x,
				        				           explorerCursor.y,
				        				           explorerButtonsW,
				        				           explorerButtonsH,
				        						   explorerButtonNormal, 
				        						   explorerButtonHot,    
					 					           explorerButtonInteracting,
												   font_text_pad_center,
												   "Ok");

                     ui_render_interactive_text(ui, buttonCancelId, ui_interaction_mouse_left_down,
							                       btnCancelX,
				        				           explorerCursor.y,
				        				           explorerButtonsW,
				        				           explorerButtonsH,
				        						   explorerButtonNormal, 
				        						   explorerButtonHot,    
					 					           explorerButtonInteracting,
												   font_text_pad_center,
												   "Cancel");

					 explorer->okay_pressed = 0;
					 u32 cancel_pressed = ui_element_interacted_flags(ui, buttonCancelId, ui_interaction_mouse_left_down);
					 u32 okay_pressed = ui_element_interacted_flags(ui, buttonOkId, ui_interaction_mouse_left_down);
					 //Just close the current panel
					 if(cancel_pressed)
					 {
						 uiCommandsOp->current->closed = 1;
						 explorer->closed = 1;
					 }
					 if(okay_pressed)
					 {
						 explorer->okay_pressed = 1;
					 }
                    // ui_render_interactive(ui, buttonOkId, ui_interaction_mouse_left_down,
				    //     		                   explorerCursor.x,
				    //     				           explorerCursor.y,
				    //     				           explorerButtonsW,
				    //     				           explorerButtonsH,
				    //     						   explorerButtonNormal, 
				    //     						   explorerButtonHot,    
					//						       explorerButtonInteracting);
					// vec2 buttonOkTextOffset = font_get_text_pad_offset(&ui->fontp, explorerButtonsW, explorerButtonsH, "Ok", ui->font_scale, font_text_pad_center);
					// render_text_2d_no_wrap(ui->renderCommands, &ui->fontp,
					//		             explorerCursor.x + buttonOkTextOffset.x,
					//					 explorerCursor.y + buttonOkTextOffset.y,
					//					 ui->font_scale, "Ok");

					 explorerCursor.y += explorerButtonsH;

					 //
					 //Check current process
					 //

					 u32 completedProcess = 0;
			         if(explorer->process_type & ui_explorer_process_type_file)
			         {
                         completedProcess = (selected_with_enter || okay_pressed) && path_IsValidFileName(ui->explorer->process_file_name);
			         }
					 if(!completedProcess && explorer->process_type & ui_explorer_process_select_file)
			         {
						 completedProcess  = selected_with_enter || (explorer->file_got_selected && (ui->interacted_flags & ui_interaction_mouse_left_double_click));

						 if((ui->interacted_flags & ui_interaction_mouse_left_double_click))
						 {
							 int s = 0;
						 }
			         }
					 if(completedProcess)
					 {
					     explorer->last_process_completed = completedProcess;
						 if(explorer->flags & ui_explorer_flags_close_on_complete)
						 {
							 explorer->closed = 1;
						     uiCommandsOp->current->closed = 1;
							 
						 }
	                     explorer->update_path_files  = 1;
					 }
					 //
					 //
					 //

					 //Advance by total size
					 f32 totalAdvancedHeight = explorerCursor.y - layoutCursor.y;
					 ui_advance(ui, layoutCursor.x, layoutCursor.y, totalExplorerW, totalAdvancedHeight);

				 }break;
			 case ui_command_type_tileset:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					ui_element_persistent *elementPersistent = ui->persistentElements + element->persistent_element_array_index;
					u32 tilesetTileCount = elementPersistent->tileset.tileCount;
					u32 zoom = 2;

					u16 display_w = elementPersistent->tileset.w * zoom;
					u16 display_h = elementPersistent->tileset.w * zoom;
					u32 tileSize  = elementPersistent->tileset.tileSize;
					u32 tileSizeZoomed = (u32)(zoom * tileSize);

					u16 w         = elementPersistent->tileset.w / tileSize;
					u16 h         = elementPersistent->tileset.h / tileSize;
					u32 tile_x    = 0;
					u32 tile_y    = 0;
					u32 selected_tileset_tile_index = elementPersistent->tileset.selectedTile;
					render_texture tilesetTexture = elementPersistent->tileset.texture;
					ui_tileset_tile *tileArray    = elementPersistent->tileset.tiles;

					panel_commands_at     += tilesetTileCount * sizeof(ui_tileset_tile);

					vec2 layoutCursor = ui_get_layout_cursor(ui);

					ui_PushClip_XYWH(ui, (i32)layoutCursor.x,
							             (i32)layoutCursor.y,
										 display_w ,
										 display_h );
					//background
					//render_draw_rectangle_2D(ui->renderCommands, layoutCursor.x, layoutCursor.y, display_w, display_h, ui->theme.frame_background_color);

                    ui_render_image_background(ui,
							                   layoutCursor.x,
											   layoutCursor.y,
											   display_w ,
											   display_h ,
											   2);

					//Display tiles
					for(u32 t = 0; t < tilesetTileCount; t++)
					{
					    tile_x = (tileSizeZoomed * (t % w));
						tile_y = (tileSizeZoomed * (t / w));

						u32 tileIndex         = t; 
						ui_tileset_tile *tile = tileArray + tileIndex;
						ui_id tilesetTileId   = ui_id_NUMBER_POINTER(tileIndex, elementPersistent);

						vec2 tilePosition = {
							(f32)tile_x + layoutCursor.x,
							(f32)tile_y + layoutCursor.y
						};
						vec2 tileDisplaySize = {
							(f32)tileSizeZoomed,
							(f32)tileSizeZoomed
						};

						//tile interaction
						ui_create_update_element_at(ui, tilesetTileId,
								                    tilePosition.x, tilePosition.y,
													tileDisplaySize.x, tileDisplaySize.y);

						//Draw tile
						render_draw_sprite_2D(ui->renderCommands,
								            &tilesetTexture,
										    tilePosition,	
										    tileDisplaySize,
											vec4_all(255),
							                tile->frame_x,
							                tile->frame_y,
							                tile->frame_w,
							                tile->frame_h
											);


					}

					//grid
					ui_render_grid(ui,
							       layoutCursor.x,
								   layoutCursor.y,
								   (display_w),
								   (display_h),
							           0, 0, tileSizeZoomed , tileSizeZoomed );
					//draw selection
					if(selected_tileset_tile_index < tilesetTileCount)
					{
    					ui_tileset_tile *selectedTile = tileArray + selected_tileset_tile_index;
    					tile_x = (u32)layoutCursor.x + (tileSizeZoomed * (selected_tileset_tile_index % w));
    					tile_y = (u32)layoutCursor.y + (tileSizeZoomed * (selected_tileset_tile_index / w));
    
    					render_rectangle_borders_2D(ui->renderCommands,
    							                    (f32)tile_x,
    												(f32)tile_y,
    												(f32)tileSizeZoomed,
    												(f32)tileSizeZoomed,
													1, V4(0xff, 0, 0, 0xff));
    
					}


					ui_advance(ui, layoutCursor.x, layoutCursor.y, display_w, display_h);
					render_commands_PopClip(ui->renderCommands);

				 }break;
			 case ui_command_type_timeline:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					ui_element_persistent *timelineElement = ui->persistentElements + element->persistent_element_array_index; 
					ui_timeline *timeline = &timelineElement->timeline;

					panel_commands_at += timeline->dataOffset;

					u32 id_count = 0;
					

					//panel_commands_at     += sizeof(ui_timeline_track) * timeline->trackMax;
					//panel_commands_at     += sizeof(ui_timeline_clip_group_key) * timeline->clip_group_key_count;
					//panel_commands_at     += sizeof(ui_timeline_clip) * timeline->clip_max;
					//panel_commands_at     += sizeof(ui_timeline_frame_group) * timeline->frame_group_max;
					//reproduce button

					vec2 timeline_size = ui_get_remaining_layout_size(ui);
					vec2 layoutCursor = ui_get_layout_cursor(ui);
					//offset for track names
					f32 timeline_offset_x = 80;

					timeline_size.x -= timeline_offset_x;

					vec2 timeline_position = {
						layoutCursor.x + timeline_offset_x,
						layoutCursor.y
					};

					ui_id reproduce_button_id = ui_id_NUMBER_POINTER(id_count++, timeline);
					u8 *reproduce_button_text = ">";
					if(timeline->reproducing)
					{
					   reproduce_button_text = "||";
					}
					timeline->reproduce_interacted = ui_element_interacted_flags(
							ui, reproduce_button_id, ui_interaction_mouse_left_down);
					//reproduce button
					ui_update_render_button_label_at(
							ui,
							reproduce_button_id,
							0,
							layoutCursor.y,
							20,
							20,
							font_text_pad_center,
							reproduce_button_text);

					//Advance by total space
					ui_advance(ui,
							   layoutCursor.x,
							   layoutCursor.y,
							   timeline_size.x,
							   timeline_size.y);
					//Background
					render_draw_rectangle_2D(ui->renderCommands, timeline_position.x, timeline_position.y, timeline_size.x, timeline_size.y, ui->theme.frame_background_color);
					//Borders
					render_rectangle_borders_2D(ui->renderCommands, timeline_position.x, timeline_position.y, timeline_size.x, timeline_size.y, 1, ui->theme.frameBorderColor);

					//Space for the top
					f32 timeNumbersHeight = 26 + ui_FontHeight(ui) + 4;

                    //General data
					f32 timeline_precision    = 0.1f / timeline->lines_per_ms;
					f32 timeline_scale        = 1.0f;
					f32 timeline_step         = 0.1f * 10000.0f * timeline_scale;
					f32 timeline_track_height = ui->fontp.font_height + 8.0f;

					f32 offset_from_timeline_y = timeNumbersHeight + 2;
					//theme
					//vec4 celesteNormal      = V4(0, 135, 175, 200);
					//vec4 celesteHot         = V4(0, 165, 205, 200);
					//vec4 celesteInteracting = V4(0, 90, 105 , 200);

					//
					// render and update tracks
					// tracks


					vec2 track_position = {
						timeline_position.x - timeline->scroll_h,
						timeline_position.y + timeNumbersHeight + 2 - timeline->scrollVertical
					};
					u32 track_count = timeline->trackCount > timeline->trackMax ? 
						              timeline->trackMax : timeline->trackCount;
					for(u32 t = 0; t < track_count; t++)
					{

					   ui_timeline_track *timelineTrack = timeline->tracks + t;
					   f32 trackX = track_position.x;
					   f32 trackY = track_position.y + (timeline_track_height * t);
					   f32 trackLabelX = layoutCursor.x;

					   	//track Label
			//		   	render_text_2d_no_wrap(ui->renderCommands,
			//					            &ui->fontp,
			//								trackLabelX,
			//								trackY,
			//								ui->font_scale,
			//								ui->theme.textColor,
			//								timelineTrack->label);

					   //track background
					   render_draw_rectangle_2D(ui->renderCommands,
					   		           timeline_position.x,
					   				   trackY,
					   				   timeline_size.x,
					   				   timeline_track_height,
					   				   V4(0, 0, 0, 0xff));
					   //track borders
					   render_rectangle_borders_2D(ui->renderCommands,
					   		                     timeline_position.x,
					   						     trackY,
					   						     timeline_size.x,
					   						     timeline_track_height,
					   						     1,
					   						     ui->theme.frameBorderColor);
					}
					//
					//
					//


					//
					//Update and render top
					//
					ui_create_update_element_at(ui, timeline->id, timeline_position.x, timeline_position.y, timeline_size.x, timeNumbersHeight);
					//states
					u32 timelineHot         = ui_element_hot(ui, timeline->id);
					u32 timelineInteracting = ui_element_interacting(ui, timeline->id);

					if(timeNumbersHeight > timeline_size.y)
					{
						timeNumbersHeight = timeline_size.y;
					}
					//
					//
					//
					i32 clip_x0 = (i32)(timeline_position.x);
					i32 clip_y0 = (i32)(timeline_position.y + offset_from_timeline_y);
					i32 clip_x1 = (i32)(clip_x0 + timeline_size.x);
					i32 clip_y1 = (i32)(clip_y0 + timeline_size.y - offset_from_timeline_y);
					ui_push_clip_inside_last(ui,
							              clip_x0,
										  clip_y0,
										  clip_x1,
										  clip_y1);

					//render and update frame groups
					vec2 frame_group_at =
					{
						track_position.x,
						timeline_position.y
					};
					u32 frame_group_count = timeline->frame_group_count > timeline->frame_group_max ?
						                    timeline->frame_group_max : timeline->frame_group_count;

					for(u32 g = 0;
							g < frame_group_count;
							g++)
					{
						ui_timeline_frame_group *current_frame_group = timeline->frame_groups + g;

						f32 frame_group_x = frame_group_at.x + (current_frame_group->frame_start * 0.1f * timeline_step);
				        f32 frame_group_y = frame_group_at.y;
						f32 frame_group_w = 3.0f;
						f32 frame_group_h = timeline_size.y;

						f32 frame_group_selectable_wh = 10.0f;
						f32 frame_group_selectable_y = track_position.y;

					    f32 alpha = (current_frame_group->selected) ? 255.0f : 160.0f;
						//draw line
					    render_draw_rectangle_2D(ui->renderCommands,
							                     frame_group_x,
												 frame_group_y,
												 frame_group_w,
												 frame_group_h,
												 V4(255, 255, 255, alpha));

						//update selectable
						ui_create_update_element_at(ui,
								                    current_frame_group->id,
                                                    frame_group_x,
                                                    frame_group_selectable_y, 								                    
                                                    frame_group_selectable_wh,
                                                    frame_group_selectable_wh);
						//draw selectable

					    render_draw_rectangle_2D(ui->renderCommands,
							                     frame_group_x,
												 frame_group_selectable_y,
												 frame_group_selectable_wh,
												 frame_group_selectable_wh,
												 V4(255, 255, 255, alpha));


						//draw line from bottom to top
					}

					//render and update frame group keys
					u32 frame_group_key_count = timeline->frame_group_key_count < timeline->frame_group_key_max ?
						                        timeline->frame_group_key_count : timeline->frame_group_key_max;

					for(u32 k = 0; 
							k < frame_group_key_count;
							k++)
					{
						ui_timeline_frame_group_key *current_frame_group_key = timeline->frame_group_keys + k;
						//get the group in which this one belongs
						ui_timeline_frame_group *target_frame_group = timeline->frame_groups + current_frame_group_key->frame_group_index;
						vec2 label_size =  ui_get_text_size(ui, F32MAX, current_frame_group_key->label);

						//f32 track_at_y   = track_position.y + (timeline_track_height * target_frame_group->keyframe_rendered_count);
						f32 track_at_y   = track_position.y + (timeline_track_height * current_frame_group_key->clipIndex);
						u32 key_duration = current_frame_group_key->frame_duration;

						u32 selected = current_frame_group_key->selected;
						f32 key_x = track_position.x + (target_frame_group->frame_start * timeline_step * 0.1f);
						f32 key_w = label_size.x + 4.0f;
						f32 key_h = label_size.y;
						f32 key_y = track_at_y + timeline_track_height - key_h; 

						//increase key count to correctly position the next keys
						target_frame_group->keyframe_rendered_count++;

						//temporary colors
					    vec4 key_normal_color = {0x70, 0x15, 0x15, 180};
					    vec4 key_hot_color    = {0xd9, 0x1f, 0x1f, 180};
						if(current_frame_group_key->type == 1)
						{
							key_normal_color = V4(0x00, 0x15, 0x9a, 190);
							key_hot_color    = V4(0x00, 0x1f, 0xd9, 190);
						}
						else
						{
						}

						ui_create_update_element_at(ui,
								                    current_frame_group_key->id,
													key_x,
													key_y,
													key_w,
													key_h);

						ui_render_selectable_text(ui,
								                  current_frame_group_key->id,
												  current_frame_group_key->selected,
												  ui_interaction_mouse_left_down,
												  key_x,
												  key_y,
												  key_w,
												  key_h,
												  key_hot_color,
												  key_normal_color,
												  key_hot_color,
												  key_hot_color,
												  font_text_pad_left,
												  current_frame_group_key->label
												  );
						//draw duration if selected
					    f32 key_duration_alpha = 200;
					    vec4 key_duration_color = {255, 255, 0, key_duration_alpha};
					    vec4 key_duration_rec_color = {255, 255, 255, key_duration_alpha};
					    f32 key_duration_l_x = key_x;
					    f32 key_duration_l_y = track_at_y;
					    f32 key_duration_w   = 2.0f;
					    f32 key_duration_h   = 6.0f;
						f32 key_duration_rec_wh = 8.0f;
						if(target_frame_group->selected)
						{
							if(!selected)
							{
								key_duration_color.w     = 120.0f;
								key_duration_rec_color.w = 120.0f;
								key_duration_h = 2.0f;
								key_duration_rec_wh = 6.0f;
							}

							//end
							render_draw_rectangle_2D(ui->renderCommands,
									key_duration_l_x + (key_duration * timeline_step * 0.1f),
									key_duration_l_y,
									key_duration_w,
									key_duration_h,
									key_duration_color);
							//line between start and end

							render_draw_rectangle_2D(ui->renderCommands,
									key_duration_l_x,
									key_duration_l_y,
									(key_duration * timeline_step * 0.1f),
									key_duration_h,
									key_duration_color);

							render_draw_rectangle_2D(ui->renderCommands,
									key_duration_l_x + (key_duration * timeline_step * 0.1f),
									key_duration_l_y,
									key_duration_rec_wh,
									key_duration_rec_wh,
									key_duration_color);
						}

						u32 interacting = ui_element_interacting_flags(ui, current_frame_group_key->id, ui_interaction_mouse_left_down);
						if(interacting)
						{
						}
					}
					
					//
					//render and update timeline clips
					//
					if(timeline->clip_selected_index >= timeline->clip_count)
					{
						timeline->clip_selected_index = timeline->clip_count - 1;
					}
					f32 clipRecWH = 10.0f;
					vec2 clipAt = {
						track_position.x,
					    track_position.y	
					};

					u32 clip_count = timeline->clip_count > timeline->clip_max ? 
						              timeline->clip_max : timeline->clip_count;
                    for(u32 c = 0; c < clip_count; c++)
					{
					   ui_timeline_clip *timelineClip = timeline->clips + c;

					   f32 clipX      = clipAt.x + (timelineClip->timeStart * timeline_step);
					   f32 clipY      = clipAt.y + (timeline_track_height * c);

					   f32 clip_size = timelineClip->timeDuration * timeline_step;
					   f32 clip_x2   = clipX + clip_size;

					   f32 alpha = (timelineClip->selected) ? 255.0f : 160.0f;
					   render_draw_rectangle_2D(ui->renderCommands,
							                    clipX,
												clipY,
												clipRecWH,
												clipRecWH,
												V4(255, 255, 255, alpha));

					   //second rectangle
					   render_draw_rectangle_2D(ui->renderCommands,
							                    clip_x2,
												clipY,
												clipRecWH,
												clipRecWH,
												V4(255, 255, 255, alpha));
					   //line to second rectangle
					   render_draw_rectangle_2D(ui->renderCommands,
							                    clipX,
												clipY,
												clip_size,
											    4,	
												V4(255, 255, 255, alpha));

					   ui_id clipId = timelineClip->id; 
					   ui_create_update_element_at(ui, clipId, clipX, clipY, clipRecWH, clipRecWH);

					   u32 interacted  = ui_element_interacted_flags(ui, clipId, ui_interaction_mouse_left_down);
					   u32 interacting = ui_element_interacting_flags(ui, clipId, ui_interaction_mouse_left_down);

					   if(interacting)
					   {
						   timeline->clip_selected_index = c;

						   f32 mouseAtX    = (ui->mouse_point.x - timeline_position.x) / timeline_step;
                           if(mouseAtX < 0)
						   {
							   mouseAtX = 0;
						   }

						   timelineClip->timeStart = (f32)(i32)((mouseAtX) / 0.1f) * 0.1f;

						   if(timelineClip->timeStart < 0)
						   {
						   	  timelineClip->timeStart = 0;
						   }
					   }
					}
					//
					//
					//

					//
					//render and update key frame tracks
					//

					//make sure the selected track is less than the total pushed
					if(timeline->clip_group_key_selected >= timeline->clip_group_key_count)
					{
						timeline->clip_group_key_selected = timeline->clip_group_key_count - 1;
					}
					timeline->clip_group_key_got_selected = 0;
					//get array from the end of the data array
				    ui_timeline_clip_group_key *trackKeyArray = (ui_timeline_clip_group_key *)(timeline->data);
					//offset is increased by name length of the assigned track
		            f32 *clip_group_key_rendered_offset = ui_push_size_to_reserved_and_clear(ui, sizeof(f32) * timeline->clip_group_key_count);
					f32 track_key_spacing               = 4.0f;

					vec4 track_key_normal_color = {0x70, 0x15, 0x15, 180};
					vec4 track_key_hot_color    = {0xd9, 0x1f, 0x1f, 180};
					for(u32 t = 0; t < timeline->clip_group_key_count; t++)
					{
						ui_timeline_clip_group_key *trackKey = trackKeyArray + t;
						u32 selected   = (trackKey->selected);
						u32 clip_index = trackKey->clipIndex;

						ui_timeline_clip *clip_group = timeline->clips + clip_index;
						f32 clip_time_start          = clip_group->timeStart;
						
						//Get the position of the parent track
						vec2 text_size       = font_get_text_size_scaled(&ui->fontp, ui->font_scale, trackKey->label);
					    f32 track_key_w        = text_size.x + 4;
			            //advance by the offset so far
						f32 clip_key_spacing = clip_group_key_rendered_offset[clip_index];
						f32 track_key_x        = track_position.x + (clip_time_start * timeline_step) + clip_key_spacing + track_key_spacing;
						f32 track_key_y        = track_position.y + (timeline_track_height * trackKey->clipIndex) + ((timeline_track_height - 10) * 0.5f);

						//add the text size to the offset array
						clip_group_key_rendered_offset[clip_index] += track_key_w + track_key_spacing;

						ui_create_update_element_at(ui,
								                    trackKey->id,
													track_key_x,
													track_key_y,
													track_key_w,
													text_size.y);

						ui_render_selectable_text(ui,
								                   trackKey->id,
												   selected,
												   ui_interaction_mouse_left_down,
												   track_key_x,
												   track_key_y,
												   track_key_w,
												   text_size.y,
												   track_key_hot_color,
												   track_key_normal_color,
												   track_key_hot_color,
												   track_key_hot_color,
												   font_text_pad_left,
												   trackKey->label
												   );
					}

					render_commands_PopClip(ui->renderCommands);

					//render the top part of the timeline
					clip_x0 = (i32)(timeline_position.x);
					clip_y0 = (i32)(timeline_position.y);
					clip_x1 = (i32)(clip_x0 + timeline_size.x);
					clip_y1 = (i32)(clip_y0 + timeline_size.y);
					ui_push_clip_inside_last(ui, clip_x0, clip_y0, clip_x1, clip_y1);

					//current selected clip occupied time
					if(timeline->clip_count)
					{
						ui_timeline_clip *selected_clip = timeline->clips + timeline->clip_selected_index;

						f32 clip_space_x = track_position.x + (selected_clip->timeStart * timeline_step);
						f32 clip_space_w = selected_clip->timeDuration * timeline_step;
						f32 clip_space_y = timeline_position.y;
						f32 clip_space_h = timeline_size.y;

						vec4 clip_space_color = {255, 0, 0, 100};

						render_draw_rectangle_2D(ui->renderCommands,
								                 clip_space_x,
								                 clip_space_y,
								                 clip_space_w,
								                 clip_space_h,
												 clip_space_color);
					}



					f32 ms     = 0.0f;
					f32 second = 0.0f;
					f32 time_numbers_x = track_position.x;
					f32 time_numbers_y = timeline_position.y;

					//lines background 
					render_draw_rectangle_2D(ui->renderCommands,
							                 timeline_position.x,
											 time_numbers_y,
											 timeline_size.x,
											 timeNumbersHeight,
											 V4(0, 0, 0, 0xFF));
					//Horizontal line
					vec4 lineColor = {0xff, 0xff, 0xff, 0xB0};
					render_draw_rectangle_2D(ui->renderCommands,
							                 timeline_position.x,
											 time_numbers_y,
											 timeline_size.x,
											 2,
											 lineColor); 

					//render time "lines"
					// ;optimize
					f32 next_whole_number_count = 0;
					while(time_numbers_x < (timeline_size.x + timeline_position.x))
					{
						f32 lineWidth  = 2;
						f32 lineHeight = 10;
						u8 *format = "%.2f";
						//if this is a whole number
						if(f32_NEARZERO(next_whole_number_count))
						{
							lineWidth += 1;
							lineHeight += 16;
					        next_whole_number_count = 0.1f;
							format = "%.1f";
						}
						//draw line pointing below
					    render_draw_rectangle_2D(ui->renderCommands,
								                 time_numbers_x, 
										         time_numbers_y, 
										         lineWidth,
										         lineHeight,
										         lineColor);
						//get text to render
						u8 textBuffer[16] = {0};
						FormatText(textBuffer, sizeof(textBuffer), format, ms);
						//number below lines
						render_text_2d_no_wrap(ui->renderCommands,
								            &ui->fontp,
											time_numbers_x,
											time_numbers_y + lineHeight,
											ui->font_scale,
											ui->theme.textColor,
											textBuffer);

						second++;
						ms = second * timeline_precision;
						next_whole_number_count -= timeline_precision;

						time_numbers_x += timeline_step * 0.1f;
					}

					//Display time_at and Total cursors
					f32 time_at          = timeline->time_at; 
					f32 time_total       = timeline->time_total;
					f32 total_time_step = time_total * timeline_step;

					vec4 timeline_animation_time_color              = {0, 170, 200, 180};
					vec4 timeline_animation_time_resizers_color     = {0, 200, 230, 200};
					vec4 timeline_animation_time_resizers_hot_color = {0, 230, 255, 200};
					f32 timeline_animation_time_y                   = timeline_position.y + 20;

					f32 timeline_animation_time_resizers_w = 10;
					f32 timeline_animation_time_resizers_h = 8;
					f32 timeline_animation_time_r_x = timeline_position.x + total_time_step -  timeline_animation_time_resizers_w - timeline->scroll_h;

					ui_id timeline_animation_time_r_id = ui_id_NUMBER_POINTER(id_count++, timeline);

					u16 resizer_r_interacting = ui_element_interacting_flags(
							ui,
							timeline_animation_time_r_id,
							ui_interaction_mouse_left_down);

					//At
					f32 timeline_triangle_size = 20.0f;
					vec2 timeline_triangle_position =  {
						timeline_position.x + (time_at * timeline_step) - timeline->scroll_h,
						time_numbers_y + timeline_triangle_size * .5f
					};
					vec4 triangleAtColor = {255, 255, 0, 230};
                    render_Triangle2DAngle(ui->renderCommands,
							               timeline_triangle_position,
										   timeline_triangle_size,
										   PI,
										   triangleAtColor);
					//Draw line from at to bottom
					render_draw_rectangle_2D(ui->renderCommands,
							                 timeline_triangle_position.x,
											 timeline_triangle_position.y + timeline_triangle_size * 0.5f,
											 2,
											 timeline_size.y, V4(255, 255, 0, 0xD6));
					//total time rectangle
					render_draw_rectangle_2D(ui->renderCommands,
							                timeline_position.x - timeline->scroll_h,
											timeline_animation_time_y,
											total_time_step,
											timeline_animation_time_resizers_h,
											V4(0, 170, 200, 180));
					//update
					//resizer right
					ui_create_update_element_at(ui,
							                    timeline_animation_time_r_id,
                                                timeline_animation_time_r_x,
                                                timeline_animation_time_y,
                                                timeline_animation_time_resizers_w,
                                                timeline_animation_time_resizers_h
												);
					//right resizer
                    ui_render_interactive(ui,
			    		                  timeline_animation_time_r_id,
							              ui_interaction_mouse_left_down,
                                          timeline_animation_time_r_x,
                                          timeline_animation_time_y,
                                          timeline_animation_time_resizers_w,
                                          timeline_animation_time_resizers_h,
			    		                  timeline_animation_time_resizers_color,
			    		                  timeline_animation_time_resizers_hot_color,
			    		                  timeline_animation_time_resizers_hot_color);

					render_commands_PopClip(ui->renderCommands);


					//ui_id loop_id_s = ui_id_POINTERS(timeline, "timeline loop start");
					//ui_id loop_id_e = ui_id_POINTERS(timeline, "timeline loop end");
					//render and update loop cursors
					if(timeline->loop_visible)
					{
						vec4 loop_s_color = {0, 255, 0, 255};
						vec4 loop_e_color = {255, 0, 0, 255};

						f32 loop_wh = 9.0f;
						f32 loop_s_x = track_position.x + (timeline->loop_start * timeline_step * 0.1f) - loop_wh;
						f32 loop_e_x = track_position.x + timeline->loop_end   * timeline_step * 0.1f;
						f32 loop_y = timeline_position.y;
						f32 loop_line_w = 2.0f;
						f32 loop_line_h = timeline_size.y;

					    render_draw_rectangle_2D(ui->renderCommands,
								                 loop_s_x,
												 loop_y,
												 loop_wh,
												 loop_wh,
												 loop_s_color);
					    render_draw_rectangle_2D(ui->renderCommands,
								                 loop_e_x,
												 loop_y,
												 loop_wh,
												 loop_wh,
												 loop_e_color);

						//draw lines
					    render_draw_rectangle_2D(ui->renderCommands,
								                 loop_s_x + loop_wh,
												 loop_y,
												 loop_line_w,
												 loop_line_h,
												 loop_s_color);

					    render_draw_rectangle_2D(ui->renderCommands,
								                 loop_e_x,
												 loop_y,
												 loop_line_w,
												 loop_line_h,
												 loop_e_color);

					}

					//CursorLine
					f32 mouseAtTimeLineX = ui->mouse_point.x - (timeline_position.x - timeline->scroll_h);
					f32 cursorLineX      = mouseAtTimeLineX + timeline_position.x;

					timeline->timelineInteracting = 0;
					timeline->resizer_interacting = 0;
					if(timelineHot)
					{
					    render_draw_rectangle_2D(ui->renderCommands,
								                 cursorLineX,
												 timeline_position.y,
												 2,
												 timeline_size.y,
												 V4(255, 255, 255, 0xD6));
					}
					if(timelineInteracting)
					{
						timeline->time_at = mouseAtTimeLineX / timeline_step;
						if(timeline->time_at < 0)
						{
							timeline->time_at = 0;
						}
						timeline->timelineInteracting = 1;
					}

					if(resizer_r_interacting)
					{
						timeline->time_total = mouseAtTimeLineX / timeline_step;
						if(timeline->time_total < 0)
						{
							timeline->time_total = 0;
						}
						timeline->resizer_interacting = 1;
					}


					//scroll bar
					f32 timelineFrameY   = timeline_position.y + timeNumbersHeight;
					f32 timelineFrameH   = timeline_size.y - timeNumbersHeight;
					f32 timelineContentY = timeline->trackCount * timeline_track_height;
					f32 timelineContentX = timeline->time_total * timeline_step * 2;
					f32 scroll_wh        = ui_SCROLL_WH;
                    ui_update_render_scroll_vertical(ui,
							"timelineScroll",
							timeline_position.x,
							timelineFrameY,
							timeline_size.x,
							timelineFrameH - scroll_wh,
							timelineContentY,
							&timeline->scrollVertical);
					f32 cheese_horizontal_scollbar = 0;
                    ui_update_render_scroll_horizontal(ui,
							                         "timelineScroll",
													 timeline_position.x,
													 timelineFrameY,
													 timeline_size.x - scroll_wh,
													 timelineFrameH,
													 timelineContentX,
													 &timeline->scroll_h);
#if 0
					//key frame selection
					f32 keyFrameSelectionW = keyFrameSelectionH * 0.8f;
					f32 keyFrameStart = 0.1f;
					f32 keyFrameEnd = 0.5f;
					f32 keyFrameSelectionX =  keyFramePosition.x + keyFrameStart * timeline_step;
					f32 keyFrameSelectionY =  keyFramePosition.y + (keyFrameHeight - keyFrameSelectionH) * 0.5f;
					//interactive borders
					ui_id keyFrameSizeLeftId  = UIIDFROMPTR(timeline, &keyFrameSelectionX);
					ui_id keyFrameSizeRightId = UIIDFROMPTR(timeline, &keyFrameSelectionY);
					//Time
					ui_id keyFrameMoveId = UIIDFROMPTR(timeline, &keyFrameStart);
					vec4 keyFrameNormalColor      = V4(0, 135, 175, 255);
					vec4 keyFrameHotColor         = V4(0, 165, 205, 255);
					vec4 keyFrameInteractingColor = V4(0, 90, 105, 255);
					render_draw_rectangle_2D(ui->renderCommands,
							           keyFrameSelectionX,
									   keyFrameSelectionY,
									   (keyFrameEnd * timeline_step),
									   keyFrameSelectionWH,
									   );
					f32 keyFrameMoveWidth = (keyFrameEnd * timeline_step);
					//Middle interactive
				   ui_create_update_element_at(ui, keyFrameMoveId, keyFrameSelectionX, keyFrameSelectionY, keyFrameMoveWidth, keyFrameSelectionH);
                   ui_render_interactive(ui,
			    		                       keyFrameMoveId,
											   ui_interaction_mouse_left_down,
							                   keyFrameSelectionX,
									           keyFrameSelectionY,
									           keyFrameMoveWidth,
									           keyFrameSelectionH,
											   keyFrameNormalColor, 
											   keyFrameHotColor,    
											   keyFrameInteractingColor);

				   //resize buttons

					vec4 resizeFrameNormalColor      = {0x0, 0xff, 120, 0xff};
					vec4 resizeFrameHotColor         = {0, 165, 205, 255};
					vec4 resizeFrameInteractingColor = {0, 90, 105, 255};
				   ui_create_update_element_at(ui, keyFrameSizeLeftId, keyFrameSelectionX, keyFrameSelectionY, keyFrameMoveWidth, keyFrameSelectionH);
                   ui_render_interactive(ui,
			    		                       keyFrameMoveId,
											   ui_interaction_mouse_left_down,
							                   keyFrameSelectionX,
									           keyFrameSelectionY,
									           keyFrameSelectionW,
									           keyFrameSelectionH,
											   keyFrameNormalColor, 
											   keyFrameHotColor,    
											   keyFrameInteractingColor);
					render_draw_rectangle_2D(ui->renderCommands,
							           keyFrameSelectionX,
									   keyFrameSelectionY  ,
									   keyFrameSelectionW,
									   keyFrameSelectionH,
									   V4(0x0, 0xff, 120, 0xff));

#endif

				 }break;
			 case ui_command_type_selectable_grid:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					vec2 cursor_position_l = ui_get_layout_cursor(ui);
					f32 scale = element->grid.scale;
					f32 tileGridSize      = element->grid.tileGridSize;
					f32 gridSizeX         = element->grid.sizeX;
					f32 gridSizeY         = element->grid.sizeY;
					u32 tileDisplacementX = element->grid.tileDisplacementX;
					u32 tileDisplacementY = element->grid.tileDisplacementY;

					u32 stepsX            = (u32)(gridSizeX / tileGridSize); 
					u32 stepsY            = (u32)(gridSizeY / tileGridSize); 
	                 //Draw image grid
	                 vec4 gridColor = vec4_all(255);
	                 gridColor.w = 255;

					 f32 scaledTileSize = tileGridSize * scale;
					 //Advance horizontally, draw vertially.
	                 for(u32 lineIndex = 1; lineIndex < stepsX; lineIndex++)
	                 {
	                     f32 lineX  = cursor_position_l.x + lineIndex * scaledTileSize;
	                     lineX += ((lineIndex - 1) * tileDisplacementX) * scale;
	                     f32 lineY0 = cursor_position_l.y;
	                     f32 lineY1 = gridSizeY * scale;

	                     f32 lineSizeX = tileDisplacementX ? (f32)tileDisplacementX * scale : 1;
	                     render_draw_rectangle_2D(ui->renderCommands, lineX, lineY0, lineSizeX, lineY1, gridColor);
	                 }
	                 for(u32 lineIndex = 1; lineIndex < stepsY; lineIndex++)
	                 {
	                     f32 lineX0 = cursor_position_l.x;
	                     f32 lineX1 = gridSizeX * scale;

	                     f32 lineY0 = cursor_position_l.y + lineIndex * scaledTileSize; 
						 lineY0 += (lineIndex - 1) * tileDisplacementY * scale;

	                     f32 lineSizeY = tileDisplacementY ? (f32)tileDisplacementY * scale : 1;
	                     render_draw_rectangle_2D(ui->renderCommands, lineX0, lineY0, lineX1, lineSizeY, gridColor);
	                 }

					 u32 interacted = ui_element_interacted(ui, element->id);
					 vec2 *gridSelectedP = element->grid.selectedP; 

					 if(gridSelectedP)
					 {

	   	                f32 cursor_on_image_x = ui->mouse_point.x - cursor_position_l.x; 
	   	                f32 cursor_on_image_y = ui->mouse_point.y - cursor_position_l.y;
						u32 cursorInside = cursor_on_image_x >= 0 && cursor_on_image_x <= (gridSizeX * scale) &&
										   cursor_on_image_y >= 0 && cursor_on_image_y <= (gridSizeY * scale);
					   if(interacted && cursorInside)
					   {
	   	                  Assert(cursor_on_image_x >= 0);
	   	                  Assert(cursor_on_image_y >= 0);

			              i32 cursorOnTileX   = (i32)(cursor_on_image_x / ((tileGridSize + tileDisplacementX) * scale));
			              i32 cursorOnTileY   = (i32)(cursor_on_image_y / ((tileGridSize + tileDisplacementY) * scale));
			              f32 cursorFinalLocX = (f32)(cursorOnTileX * tileGridSize) + cursorOnTileX * tileDisplacementX;
			              f32 cursorFinalLocY = (f32)(cursorOnTileY * tileGridSize) + cursorOnTileY * tileDisplacementY;
						  gridSelectedP->x = cursorFinalLocX;
						  gridSelectedP->y = cursorFinalLocY;
					   }

	                   //Draw selection
	                   f32 sX = cursor_position_l.x + (gridSelectedP->x * scale);
	                   f32 sY = cursor_position_l.y + (gridSelectedP->y * scale);
	                   render_rectangle_borders_2D(ui->renderCommands, sX, sY, scaledTileSize, scaledTileSize, 2, V4(255, 0, 0, 255));

					 }
					 
					 ui_update_advance_element(ui, element, gridSizeX * scale, gridSizeY * scale);
				 }break;
			 case ui_command_type_childpanel:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 
					panel_commands_at     += sizeof(ui_panel);

				    vec2 cursor_position_l = ui_get_layout_cursor(ui);

					if(element->child_panel.panel->sz.x == 0)
					{
						element->child_panel.panel->sz.x = ui_get_remaining_layout_width(ui);
					}
					if(element->child_panel.panel->sz.y == 0)
					{
						element->child_panel.panel->sz.y = ui_get_remaining_layout_height(ui);
					}


				    ui_update_advance_element(
							ui,
							element,
							element->child_panel.panel->sz.x,
							element->child_panel.panel->sz.y);


					//In place read
					{
					   ui_panel *childPanel = element->child_panel.panel;
					   childPanel->p.x      = cursor_position_l.x;
					   childPanel->p.y      = cursor_position_l.y;

					   ui_panel_update_and_render_begin(ui, childPanel);

					   ui_panel *panelBeforeThisOne = uiCommandsOp->current; 

						//ui_push_clip_inside_last(ui->renderCommands, clipX0, clipY0, clipX1, clipY1);
						//TODO(Agu): skip this part if not inside the clip
					    uiCommandsOp->current = childPanel;
						ui_read_commands(ui, uiCommandsOp);

						//Recover after reading
					    ui->current_panel      = panelBeforeThisOne;
						uiCommandsOp->current = panelBeforeThisOne;

						//offset to the end of the child command buffer
					    panel_commands_at = childPanel->commands_offset; 
					   ui_panel_update_render_end(ui, childPanel);

					    //ui_pop_layout(ui);
						//render_commands_PopClip(ui->renderCommands);
					}

				 }break;
			 case ui_command_type_drop_down:
				 {
#if 1
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at += sizeof(ui_element);
					panel_commands_at += sizeof(ui_panel);

					//i32 keepActive = element->drop_down.active;
					u32 keepActive = ui->activeDropDown && ui_id_EQUALS(element->id, ui->last_interacted_drop_down); 

					vec2 elementP  = {ui->currentLayout->cursorX, ui->currentLayout->cursorY};
					vec2 elementSz = {ui_get_remaining_layout_width(ui), ui_FontHeight(ui)};
					elementSz = ui_set_size_if_pushed(ui, elementSz);

					ui_update_advance_element(ui, element, elementSz.x, elementSz.y);

	                vec4 comboBoxDisabledColor    = {0, 0, 0, 160};
	                vec4 comboBoxNormalColor      = {0, 0, 0, 255};
	                vec4 comboBoxHotColor         = {20, 20, 20, 255};
	                vec4 comboBoxInteractingColor = {40, 40, 40, 255};


					u32 elementInteracted = ui_element_interacted(ui, element->id);
					u32 transition        = element->drop_down.transition;
					element->drop_down.transition = 0;
					if(keepActive)
					{
						//Force change color to be an "interacting color"
						comboBoxNormalColor = comboBoxInteractingColor;
						comboBoxHotColor    = comboBoxInteractingColor;

						uiCommandsOp->nextPanel = element->drop_down.panel; 
						element->drop_down.panel->p  = V2(elementP.x, elementP.y + elementSz.y);
						element->drop_down.panel->sz = V2(elementSz.x, 200);
						element->drop_down.panel->closed = 0;
						//Detect clicks inside the list
					//	if(ui->interactedElement->parentPanel != )
						if(ui->mouse_l_up)
						{
							if(!element->drop_down.panel->scrollInteracting && !element->drop_down.panel->scrollInteracted)
							{
							  //element->drop_down.active = 0;
							  ui->activeDropDown = 0;
					     	  element->drop_down.transition++;
							}
						}
					}
					else
					{
						if(elementInteracted && !transition)
						{
							ui->last_interacted_drop_down = element->id;
							ui->activeDropDown = 1;
							element->drop_down.transition++;
						}
					}

					ui_render_interactive_text(ui,
							                          element->id,
													  ui_interaction_mouse_left_down,
													  elementP.x,
													  elementP.y,
													  elementSz.x,
													  elementSz.y,
													  comboBoxNormalColor,
													  comboBoxInteractingColor,
													  comboBoxHotColor,
													  font_text_pad_left,
													  element->label);
					vec4 shapeColors = {0xff, 0xff, 0xff, 255};
					//Border
					render_rectangle_borders_2D(ui->renderCommands, elementP.x, elementP.y, elementSz.x, elementSz.y, 1, ui->theme.frameBorderColor);
					//Triangle
					f32 triangleSz = elementSz.y * 0.6f;
					vec2 triangleBLeft  = {elementP.x + elementSz.x - 26     , elementP.y + (elementSz.y - triangleSz) * .5f};
					vec2 triangleBRight = {triangleBLeft.x + triangleSz      , triangleBLeft.y};
					vec2 triangleTop    = {triangleBLeft.x + triangleSz * .5f,triangleBLeft.y + triangleSz};
					render_Triangle2D(ui->renderCommands,
									  triangleBLeft,
									  triangleBRight,
									  triangleTop,
									  shapeColors);



					//Jump to the end of these commands to keep reading after them.
					panel_commands_at = element->drop_down.panel->commands_offset; 

#endif
				 }break;
			 case ui_command_type_pushcursorposition:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at		+= sizeof(ui_element); 

					ui_pushcursor_flags cursorFlags = element->layout.flags;

					f32 layoutX = 0;
					f32 layoutY = 0;

					f32 cursorOffsetY = 0;

					if(cursorFlags & ui_pushcursor_set)
					{
					   layoutX = element->layout.x;
					   layoutY = element->layout.y;
					}

					if(cursorFlags & ui_pushcursor_add)
					{
					   layoutX = ui->currentLayout->cursorX + element->layout.x;
					   layoutY = ui->currentLayout->cursorY + element->layout.y;
						
					}
					else if(!(cursorFlags & ui_pushcursor_screen))
					{
					    //use layout coordinates
						layoutX += ui->currentLayout->cornerX;
						layoutY += ui->currentLayout->cornerY;
					}

                    ui_layout *current = ui->currentLayout;
	                ui_push_layout(ui, layoutX,
									  layoutY,
				  					  current->cornerEndX,
				  					  current->cornerEndY,
				  					  0, 0, 0, 0);

					ui->currentLayout->keepLine   = current->keepLine;
					ui->currentLayout->keepCursor = current->keepCursor;
					ui->currentLayout->wrapLine   = current->wrapLine;

					ui->currentLayout->nextYDelta = current->nextYDelta;
					ui->currentLayout->nextXDelta = current->nextXDelta;

				 }break;
			 case ui_command_type_popcursor:
				 {
					 panel_commands_at += sizeof(ui_command_type);
					 ui_pop_layout(ui);
				 }break;
			 case ui_command_type_keeplinepush:
				 {
					 ui_element *element = (ui_element *)panel_commands_at;
					 panel_commands_at += sizeof(ui_element);

					 ui->currentLayout->keepLine = 1;
					 ui->currentLayout->wrapLine = element->keepline.wrap;
				 }break;
			 case ui_command_type_keeplinepop:
				 {
					 panel_commands_at += sizeof(ui_command_type);
	                 ui_layout *currentLayout  = ui->currentLayout;

	                 currentLayout->cursorX    = currentLayout->cornerX;
	                 currentLayout->cursorY	  = currentLayout->cursorY + currentLayout->nextYDelta + currentLayout->spacingY;
	                 currentLayout->nextYDelta = 0;
	                 currentLayout->keepLine   = 0;
					 
				 }break;
			 case ui_command_type_next_line_set:
				 {
					 ui_element *element = (ui_element *)panel_commands_at;
					 panel_commands_at += sizeof(ui_element);

					 u32 push = element->next_line.push;
					 if(push)
					 {
						 f32 layout_x = ui->currentLayout->cursorX + ui->currentLayout->nextXDelta;
						 ui->currentLayout->nextXDelta = 0;
						 f32 layout_y = ui->currentLayout->cornerY;

                          ui_layout *current = ui->currentLayout;
	                      ui_push_layout(ui, layout_x,
					      				    layout_y,
				  	      				    ui->currentLayout->cornerEndX,
				  	      				    ui->currentLayout->cornerEndY,
				  	      				    0, 0, 0, 0);

					 }
					 else
					 {
						 ui_pop_layout(ui);
					 }
				 }break;
			 case ui_command_type_group_set:
				 {
					 ui_element *element = (ui_element *)panel_commands_at;
					 panel_commands_at += sizeof(ui_element);
					 if(element->layout_group.begin)
					 {
						 f32 layout_x = ui->currentLayout->cursorX;
						 f32 layout_y = ui->currentLayout->cursorY;
						 ui_push_layout(
								 ui,
								 layout_x,
								 layout_y,
								 ui->currentLayout->cornerEndX,
								 ui->currentLayout->cornerEndY,
								 0,
								 0,
								 0,
								 0);
					 }
					 else
					 {
						 f32 next_advance_x = ui->currentLayout->total_advance_x;
						 f32 next_advance_y = ui->currentLayout->total_advance_y;
						 ui_pop_layout(ui);

						 ui_advance(
								 ui,
								 ui->currentLayout->cursorX,
								 ui->currentLayout->cursorY,
								 next_advance_x,
								 next_advance_y);
					 }

				 }break;
			 case ui_command_type_same_line:
				 {
					 panel_commands_at += sizeof(ui_command_type);
					 ui_layout *current_layout = ui->currentLayout;

					 current_layout->cursorX = current_layout->cursor_x_last;
					 current_layout->cursorY = current_layout->cursor_y_last;

					 current_layout->cursorX += current_layout->nextXDelta + current_layout->spacingX;
					 //ui->currentLayout->same_line = 1;
					 
				 }break;
			 case ui_command_type_get_frame_size:
				 {
					 ui_element *element = (ui_element *)panel_commands_at;
					 panel_commands_at += sizeof(ui_element);

					 if(element->get_frame_size.remaining)
					 {
						 element->get_frame_size.size = ui_get_remaining_layout_size(ui);
					 }
					 else
					 {
					     element->get_frame_size.size = ui_get_total_layout_size(ui);
					 }

				 }break;
			 case ui_command_type_keepcursorpush:
				 {
					 panel_commands_at += sizeof(ui_command_type);
					 ui_UpdateKeepCursorPush(ui);
				 }break;
			 case ui_command_type_keepcursorpop:
				 {
					 panel_commands_at += sizeof(ui_command_type);
					 ui_UpdateKeepCursorPop(ui);
					 
				 }break;
			 case ui_command_type_separator:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at += sizeof(ui_element);

	                f32 separatorH = 2;
	                f32 separatorW = ui_get_remaining_layout_width(ui);
					ui_layout *layout = ui->currentLayout;
					f32 distance = element->separator.distance;

	                render_draw_rectangle_2D(ui->renderCommands,
	                				   layout->cornerX,
	                				   layout->cursorY + distance * 0.5f,
	                				   separatorW,
	                				   separatorH,
	                				   vec4_all(255));
					//Force advance without counting this separator as content.
	                layout->cursorY += distance;

				 }break;
			 case ui_command_type_inputtext:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at += sizeof(ui_element);

                    //vec2 cursor_position_l = ui_get_layout_cursor(ui);
	                f32 remaining_panel_w = ui->currentLayout->cornerEndX - ui->currentLayout->cursorX;
	                f32 inputTextHeight = 32;
					u32 valueUpdated = 0;


					switch(element->input.type)
					{
						default:
							{
                               ui_update_render_input_text_element(ui, element);
							}
				    }
					element->input.valueUpdated = valueUpdated;

				 }break;
			 case ui_command_type_console_log:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at     += sizeof(ui_element);
					ui_element_persistent *consoleElement = ui->persistentElements + element->persistent_element_array_index;


	                f32 remaining_panel_w = ui->currentLayout->cornerEndX - ui->currentLayout->cursorX;
					vec2 console_position = {ui->currentLayout->cursorX, ui->currentLayout->cursorY};
					vec2 console_size = {remaining_panel_w, 300};

					//Create a panel for storing content
					ui_panel console_panel = {0};
					console_panel.p          = console_position;
					console_panel.sz         = console_size;
					ui_panel *previous_panel = ui->current_panel;
					ui->current_panel        = &console_panel;

				    ui_create_update_element_at_layout(ui,
							                           consoleElement->id,
													   console_size.x,
													   console_size.y); 
					//Background
	                render_draw_rectangle_2D(ui->renderCommands,
	                				   console_position.x,
									   console_position.y,
									   console_size.x,
									   console_size.y,
									   V4(0x00, 0x00, 0x00, 0xff)); 

					//Clip
					ui_push_clip_inside_last(ui, (i32)console_position.x, (i32)console_position.y, (i32)(console_position.x + console_size.x), (i32)(console_position.y + console_size.y));
					//Render text from bottom to top
			        u8 *buffer = consoleElement->console.buffer;
			        s_game_console *log_console = consoleElement->console.gameConsole;

					//Text from top to bottom 
					f32 cursorY            = console_position.y + 4 - (consoleElement->console.scroll);
					f32 consoleContentSize = 0;

	                vec2 text_dimensions = {0};
					if(buffer)
					{
						text_dimensions = font_get_text_size_wrapped_scaled(
								&ui->fontp,
								4 + console_size.x,
								buffer,
								ui->font_scale);

						consoleContentSize = text_dimensions.y;

						render_text_2d(
								ui->renderCommands,
								&ui->fontp,
								console_position.x,
								cursorY, console_position.x + console_size.x,
								0,
								ui->font_scale,
								ui->theme.textColor,
								buffer);
					}
					else
					{
						render_text_2d(
								ui->renderCommands,
								&ui->fontp,
								console_position.x,
								cursorY, console_position.x + console_size.x,
								0,
								ui->font_scale,
								ui->theme.textColor,
								"No buffer got allocated");
					}
                    ui_update_render_scroll_vertical(
							ui,
							"consoleScroll",
							console_position.x,
							console_position.y,
							console_size.x,
							console_size.y,
							consoleContentSize,
							&consoleElement->console.scroll);

					render_commands_PopClip(ui->renderCommands);

					//Input text for the command buffer
					ui_id consoleInputId = UIIDFROMPTR(log_console, log_console);
					u32 entered = _ui_update_render_input_text_element(ui, 
							consoleInputId,
							0,
							log_console->command_history_char_limit,
							log_console->input_buffer);

					//If there is something typed and entered 
					if(entered && ui->input_text->key_count)
					{
						log_console->command_requested = 1;
						//reset input text cursor position.
						input_text_set_target(
								ui->input_text,
								log_console->input_buffer,
								string_count(log_console->input_buffer),
								log_console->command_history_char_limit);
						ui->input_text->cursor_position_l = 0;
						ui->input_text->cursor_position_r = 0;
					}
					//set the scroll position to read the lastest console output
					if(log_console->buffer_updated)
					{
						if(consoleContentSize > console_size.y)
						{
							//Scroll
                           f32 contentOutsideBoundsY = consoleContentSize - console_size.y; 
                           consoleElement->console.scroll = contentOutsideBoundsY;
						}
					}
					//Restore panel to the current
					ui->current_panel = previous_panel;

				 }break;
			 case ui_command_type_setelementsize:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at     += sizeof(ui_element);

					ui->pushed_element_size       = element->elementsize.push;
					ui->pushed_element_size_option = element->elementsize.sizeOption;
					ui->pushed_element_w      = element->elementsize.width;
					ui->pushed_element_h     = element->elementsize.height;

				 }break;
			 case ui_command_type_setdisable:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at     += sizeof(ui_element);

					if(ui->pushed_disable_total_count < ui->pushed_disable_max)
					{
						if(ui->pushed_disable_count && element->disable.pop)
						{
                            ui->pushed_disable_true_count -= ui->disable_stack[--ui->pushed_disable_count] > 0;
							ui->disable_stack[ui->pushed_disable_count] = 0;
						}
						else
						{
							ui->disable_stack[ui->pushed_disable_count++] = (element->disable.condition != 0);
							ui->pushed_disable_total_count++;
                            ui->pushed_disable_true_count += (element->disable.condition != 0);
						}
					}

				 }break;
			 case ui_command_type_offsetcommands:
				 {

					ui_commands_offset *element = (ui_commands_offset *)panel_commands_at;
					panel_commands_at     += sizeof(ui_commands_offset);
					panel_commands_at     += element->offset;

				 }break;
			 case ui_command_type_tab_group:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					ui_element_persistent *persistent_element = ui->persistentElements + element->persistent_element_array_index;

					ui_tab_group *group = &persistent_element->tab_group;

					panel_commands_at = group->commands_offset;

					u32 active_tab_index = group->active_tab_index;
					f32 tabs_h     = 20;
					vec2 layout_cursor  = ui_get_layout_cursor(ui);
					vec2 remaining_size = ui_get_remaining_layout_size(ui);


					//keep implementing this
					//draw group line
					f32 tab_line_size = 1;
					render_draw_rectangle_2D(commands,
							                 layout_cursor.x,
											 layout_cursor.y + tabs_h,
											 remaining_size.x,
											 tab_line_size,
											 ui->theme.button_normal_color);

					vec4 tab_active_color      = ui->theme.button_normal_color;
					vec4 tab_inactive_color    = ui->theme.button_normal_color;
					vec4 tab_hot_color         = ui->theme.button_hot_color;
					vec4 tab_interacting_color = ui->theme.button_interacting_color;

					tab_inactive_color.w = tab_active_color.w - 60;

					//get names and draw tabs
					f32 tab_spacing = 0;
					f32 tab_x       = layout_cursor.x;
					u32 t = 0;
					while(t < group->tabs_count)
					{
						ui_tab_group_tab *tab = group->tabs + t;
						ui_id tab_id          = ui_id_NUMBER_POINTER(t, persistent_element);
						f32 title_width       = ui_get_text_width_with_offset(ui, tab->title);
						u32 tab_active        = active_tab_index == t;

						//render tab
                        ui_render_selectable_text(ui,
								                   tab_id,
												   tab_active,
												   ui_interaction_any,
												   tab_x,
												   layout_cursor.y,
												   title_width,
												   tabs_h,
												   tab_active_color,
												   tab_inactive_color,
												   tab_hot_color,
												   tab_interacting_color,
												   font_text_pad_center,
												   tab->title);
						//update for interaction
						ui_create_update_element_at(ui,
								                    tab_id,
													tab_x,
													layout_cursor.y,
													title_width,
													tabs_h);

						u32 tab_interacting = ui_element_interacting(ui, tab_id);
						//focus on this tab if interacting
						if(tab_interacting)
						{
							group->active_tab_index = t;
						}

						tab_spacing = 4;
						tab_x += (title_width + tab_spacing);
						t++;
					}

					f32 advance_w = tab_x;
					f32 advance_h = tabs_h;
					//update and render the active tab contents
					if(group->tabs_count)
					{
					    ui_tab_group_tab *active_tab = group->tabs + active_tab_index;

						//create a default panel
					    ui_panel tab_panel = {0};
					    tab_panel.p.x            = layout_cursor.x;
					    tab_panel.p.y            = layout_cursor.y + tabs_h + tab_line_size;
					    tab_panel.sz             = remaining_size;
					    tab_panel.flags          = ui_panel_flags_invisible;
					    tab_panel.commands_base   = active_tab->commands_base;
					    tab_panel.commands_offset = active_tab->commands_offset;
						tab_panel.id             = ui_id_POINTER_NUMBER(active_tab, active_tab_index);


					    ui_panel_update_and_render_begin(ui, &tab_panel);

					    //to restore the current panel
					    ui_panel *panelBeforeThisOne = uiCommandsOp->current; 

					    //set the created panel
					    uiCommandsOp->current = &tab_panel;
					    ui_read_commands(ui, uiCommandsOp);

					    //Recover after reading
					    ui->current_panel      = panelBeforeThisOne;
					    uiCommandsOp->current = panelBeforeThisOne;

					    //offset to the end of the groups command buffer
					    ui_panel_update_render_end(ui, &tab_panel);

						advance_w = MAX(advance_w, tab_panel.totalContentSize.x);
						advance_h += tab_panel.totalContentSize.y;
					}
					//advance layout cursor
					ui_advance(ui,
							   layout_cursor.x,
							   layout_cursor.y,
							   advance_w,
							   advance_h);


				 }break;
			 case ui_command_type_selectable_directions:
				 {
					ui_element *element = (ui_element *)panel_commands_at;
					panel_commands_at     += sizeof(ui_element);
					vec2 layout_cursor   = ui_get_layout_cursor(ui);

                    
					u32 directions = element->selectable_directions.directions;
					f32 selectable_w = 80;
					f32 selectable_h = 80;
					vec4 selectable_borders_color = {255, 255, 255, 200};
					vec4 selectable_background_color = ui->theme.frame_background_color;

					//background
					render_draw_rectangle_2D(commands,
							                 layout_cursor.x,
											 layout_cursor.y,
											 selectable_w,
											 selectable_h,
											 selectable_background_color);
					//borders
					render_rectangle_borders_2D(commands,
							                    layout_cursor.x,
												layout_cursor.y,
												selectable_w,
												selectable_h,
												1,
												selectable_borders_color);
					if(directions)
					{

						vec2 line_origin = { layout_cursor.x + selectable_w * 0.5f,
							layout_cursor.y + selectable_h * 0.5f};
						f32 max_angle     = PI * 2;
						//this is added to angle_current
						f32 angle_sum     = max_angle / directions;
						f32 angle_current = 0;

						//to get the correct distance to the square borders
						vec2 border_l = {layout_cursor.x,
							layout_cursor.y + selectable_h * 0.5f};
						vec2 border_r = {layout_cursor.x + selectable_w,
							border_l.y};

						vec2 border_u = {layout_cursor.x + selectable_w * 0.5f,
							layout_cursor.y};
						vec2 border_d = {border_u.x,
							layout_cursor.y + selectable_h};

						vec4 unselected_direction_color = {0, 160, 40, 200};
						vec4 selected_direction_color   = {160, 160, 0, 255};
						ui_id selected_direction_id = {0};

						u32 current_direction   = 0;
						f32 current_mouse_line_distance = -100000.0f;
						vec2 distance_cursor_origin = vec2_Normalize(vec2_sub(ui->mouse_point, line_origin));

						while(angle_current < max_angle)
						{
							vec2 line_end = { sin32(angle_current),
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

							//generate id for this direction
							ui_id current_direction_id = ui_id_NUMBER_POINTER(current_direction, element);

							vec2 distance_end_origin       = vec2_Normalize(vec2_sub(line_end, line_origin));
							f32 distance_cursor_line_inner = vec2_inner(distance_end_origin, distance_cursor_origin);

							//compare inner products to set the id
							if(distance_cursor_line_inner > current_mouse_line_distance)
							{
								current_mouse_line_distance = distance_cursor_line_inner;
								selected_direction_id       = current_direction_id;
							}

							//determine the color of the current line
							u32 element_interacting = ui_element_interacting_flags(ui,
									current_direction_id,
									ui_interaction_mouse_left_down);

							u32 current_direction_selected = element->selectable_directions.selected_direction == current_direction;

							vec4 dir_color = unselected_direction_color;
							if(current_direction_selected || element_interacting)
							{
								dir_color = selected_direction_color;
							}

							line_end = vec2_round_to_int(line_end);
							//render lines
							render_line_2d_center(commands,
									line_origin,
									line_end,
									1.4f,
									dir_color); 

							angle_current     += angle_sum;
							current_direction += 1;
						}
						//interaction id and advance
						ui_create_update_element_at_layout(ui,
								selected_direction_id,
								selectable_w,
								selectable_h);
					}

				 }break;
			 default:
				 {
					 Assert(0);
				 }break;
		 }
		 Assert(panel_commands_at <= commands_end);
	   }
	   Assert(panel_commands_at == commands_end);

	//ui commands end

}
inline void 
ui_BeforeEnd(game_ui *ui)
{
	u32 commands_offset         = 0;
    ui_panel *current_panel     = 0;
	ui_command_op uiCommandsOp = {0};

	//Commands pushed is actually the amount of panels used on the ui side
	for(u32 commandI = 0;
			commandI < ui->panel_stack_count;
			commandI++)
    {
		//Pick the index from the end of the stack.
		//Pick one panel from the stack.
		u32 panelIndex        = ui->panel_stack_order[(ui->panel_stack_count - 1) - commandI];
		current_panel          = ui->panel_stack + panelIndex;
		//
		//Read panel commands.
		//
	   //READ COMMANDS
	   uiCommandsOp.nextPanel = current_panel;
		   
	   //Process front panel focus.
	   u32 processFocus = !uiCommandsOp.nextPanel->closed && !(uiCommandsOp.nextPanel->flags & ui_panel_flags_ignore_focus);
       if(processFocus)
   	   {
		  vec2 panel_position = uiCommandsOp.nextPanel->p;
		  vec2 panel_size     = uiCommandsOp.nextPanel->sz;
		  if(uiCommandsOp.nextPanel->notVisible)
		  {
			  panel_size.y = TITLEHEIGHT;
		  }
          i32 mouseOverThisPanel = ui_MouseOverRecClipped_XYWH(ui, panel_position.x, panel_position.y, panel_size.x, panel_size.y);
		  if(mouseOverThisPanel)
		  {
		     ui->panel_hot = uiCommandsOp.nextPanel;
		  }
	   }

	   //This loop is necessary in case of child panels.
	   while(uiCommandsOp.nextPanel)
	   {
		   //completely ignore the panel if closed
		   if(uiCommandsOp.nextPanel->closed)
		   {
			   uiCommandsOp.nextPanel = 0;
		   }
		   else
		   {
		       ui_panel_update_and_render_begin(ui, uiCommandsOp.nextPanel);
			   //Set current and reserve next
		       uiCommandsOp.current   = uiCommandsOp.nextPanel;
	           uiCommandsOp.nextPanel = 0;
			   //skip the command reading if not visible
		       if(!uiCommandsOp.current->notVisible)
		       {
	             ui_read_commands(ui, &uiCommandsOp); 

                 //ui_pop_layout(ui);
                 //render_commands_PopClip(ui->renderCommands);

		       }
		       ui_panel_update_render_end(ui, uiCommandsOp.current);
		  }
	   }

	   //End main panel layout
	   ui->current_panel = 0;

	   current_panel = 0;

	}

	if(ui->focused_panel)
	{
		if(!ui->mouse_l_down && !ui->forced_panel_focus)
		{
	       ui->focused_panel = 0;
		}
		ui->forced_panel_focus = 0;
	}
	else
	{
		if(ui->mouse_l_down)
		{
	       ui->focused_panel = ui->panel_hot;
		}
	}
	ui->panel_hot = 0;
}

inline void
ui_sort_panel_order(game_ui *ui)
{

   u32 s = 0;
   while(s < ui->panel_stack_count)
   {
	   //make sure no numbers pass the push amount
	   if(ui->panel_stack_order[s] >= ui->panel_stack_count)
	   {
		   //Only move to preserve order
		   while(ui->panel_stack_order[s] >= ui->panel_stack_count)
		   {
		      i32 i = s;
		      while(i < (ui->panel_last_avadible_slot - 1))
		      {
               	  u16 swappedI		     = ui->panel_stack_order[i];
               	  ui->panel_stack_order[i] = ui->panel_stack_order[i + 1];
               	  ui->panel_stack_order[i + 1] = swappedI;
		          i++;
		      }
		   }

	   }
	   s++;
   }
	//Find if there is a panel being interacted
	//;Cleanup obviously
   ui_panel *current_panel = 0; 
   u32 i				  = ui->panel_stack_count;
   if(ui->focused_panel)
   {
       for(u32 pI = 0; 
        	      pI < ui->panel_stack_count;
        		  pI++)
       {
           //Only check if interacting for now
           current_panel = ui->panel_stack + ui->panel_stack_order[pI];
           if(ui->focused_panel == current_panel)
           {
        	   i = pI; 
        	   break;
           }
       }
       
	   //found focused panel. Push it to the first index
       if(i != ui->panel_stack_count)
       {
           Assert(i < ui->panel_stack_count);
        	while(i > 0) 
        	{
        		u16 swappedI		   = ui->panel_stack_order[i];
        		ui->panel_stack_order[i] = ui->panel_stack_order[i - 1];
        		ui->panel_stack_order[i - 1] = swappedI;
        		i--;
        	}

       }
   }
}

inline void
ui_reset_interactions(game_ui *ui)
{
	ui->keepInteraction = 0;
	ui->element_interacting = ui_id_ZERO;
	ui->element_forced_interacting = ui_id_ZERO;
	ui->element_forced_last_hot = ui_id_ZERO;
	ui->element_forced_hot = ui_id_ZERO;

    ui->element_hot = ui_id_ZERO;
    ui->element_interacting = ui_id_ZERO;
    ui->element_last_interact = ui_id_ZERO;
    ui->element_last_hot = ui_id_ZERO;

	ui->last_interacted_drop_down = ui_id_ZERO;

	ui->lastElementPushed = ui_id_ZERO;


	ui->interacting_flags = 0;
	ui->interacted_flags = 0;
	ui->forced_interacting_flags = 0;
}

inline void
ui_end_frame(game_ui *ui, game_input *input, u32 windowFocused)
{
	u32 lastTransition = ui->element_transition;
   ui->element_transition = 0;
   ui_interaction_flags lastInteractingFlags = ui->interacting_flags;

   ui->interacting_flags = 0;
   ui->interacted_flags = 0;
   ui->forced_interacting_flags = 0;
   //ui->panel_stack_count = 0;

   i32 mouseLeftDown  = input_down(input->mouse_left);
   i32 mouseLeftUp    = !mouseLeftDown;
   u32 mouseRightDown = input_down(input->mouse_right);
   //include scroll *-*
   u32 anyMouseDown = mouseLeftDown || input_down(input->mouse_right);
   ui_element ZEROELEMENT = {0};
   //Prevents interaction with widgets while holding click outside anything ui related
   if(anyMouseDown && ui_element_hot(ui, ui_id_ZERO) && ui_element_interacting(ui, ui_id_ZERO))
   {
      ui->element_interacting = ui_id_SCREEN; 
   }
   //There is an element id
   i32 interacting = !ui_element_interacting(ui, ui_id_ZERO); 
   i32 interacted  = !ui_element_interacted(ui, ui_id_ZERO); 
   u32 interacting_with_forced = !ui_id_EQUALS(ui->element_forced_interacting, ui_id_ZERO);

   ui->element_last_interact = ui_id_ZERO; 
   if(interacting)
   {
	   //Not interacting with element anymore
	   if((!anyMouseDown && !ui->keepInteraction) || !windowFocused)
       {
		   ui->element_transition++;
           ui->element_last_interact = ui->element_interacting; 
           ui->element_interacting = ui_id_ZERO;
		   //Inherits interaction
		   ui->interacted_flags = lastInteractingFlags;
		   u32 hotInteracting = ui_id_EQUALS(ui->element_hot, ui->element_last_interact);
		   if(input->doubleClickedLeft)
		   {
			   ui->interacted_flags |= ui_interaction_mouse_left_double_click;
		   }
       }
	   else
	   {
	       if(mouseLeftDown)
	       {
	           ui->interacting_flags |= ui_interaction_mouse_left_down;
	       }
	       if(mouseRightDown)
	       {
	           ui->interacting_flags |= ui_interaction_mouse_right_down;
	       }
	   }
   }
   if(interacting_with_forced)
   {
	   if(!anyMouseDown || !windowFocused)
	   {
		   ui->element_forced_interacting = ui_id_ZERO;
	   }
	   else
	   {
	       if(mouseLeftDown)
	       {
	           ui->forced_interacting_flags |= ui_interaction_mouse_left_down;
	       }
	       if(mouseRightDown)
	       {
	           ui->forced_interacting_flags |= ui_interaction_mouse_right_down;
	       }
	   }
   }
   if(ui->process_hot_nodes && windowFocused)
   {
      ui->element_hot = ui->element_last_hot;
      if(!interacting && anyMouseDown)
      {
		  //process normal elements
          ui->element_interacting = ui->element_hot;
          ui->element_transition++;

	      if(mouseLeftDown)
	      {
	          ui->interacting_flags |= ui_interaction_mouse_left_down;
	      }
	      if(mouseRightDown)
	      {
	          ui->interacting_flags |= ui_interaction_mouse_right_down;
	      }
		  if(input->doubleClickLeft)
		  {
			  ui->interacting_flags |= ui_interaction_mouse_left_double_click;
		  }
	  }
	  ui->element_forced_hot = ui->element_forced_last_hot;
	  if(!interacting_with_forced && anyMouseDown)
	  {
		  ui->element_forced_interacting = ui->element_forced_hot;
	      if(mouseLeftDown)
	      {
	          ui->forced_interacting_flags |= ui_interaction_mouse_left_down;
	      }
	      if(mouseRightDown)
	      {
	          ui->forced_interacting_flags |= ui_interaction_mouse_right_down;
	      }
		  if(input->doubleClickLeft)
		  {
			  ui->forced_interacting_flags |= ui_interaction_mouse_left_double_click;
		  }
	  }
	  ui->element_last_hot = ui_id_ZERO;
	  ui->element_forced_last_hot = ui_id_ZERO;


   }

   //Restore this before reading the commands.
   ui->process_hot_nodes = 1;

   ui_sort_panel_order(ui);
   ui_BeforeEnd(ui);

   ui->currentLayout = 0;
   if(!ui->input_text->focused)
   {
	   ui->input_text_entered = 0;
   }

   render_commands_End(ui->renderCommands);
}





inline void
ui_test_ShowColorPickerFor(game_ui *ui, u8 *text, vec4 *color)
{
	ui_text(ui, text); 

	ui_keep_line_push(ui);
	ui_drag_f32(ui, 1.0f, 0, 255, &color->x, "color_r");
	ui_drag_f32(ui, 1.0f, 0, 255, &color->y, "color_g");
	ui_drag_f32(ui, 1.0f, 0, 255, &color->z, "color_b");
	ui_drag_f32(ui, 1.0f, 0, 255, &color->w, "color_a");
	ui_keep_line_pop(ui);
}

static u32 
ui_show_test_panel(game_ui *ui, s_input_text *input_text)
{
	ui_panel_begin(ui, ui_panel_flags_front_panel, 310, 310, 512, 512, "Widget and options panel");

	if(ui_button(ui, "Restore default"))
	{
		ui->theme = ui_DefaultTheme();
	}
	ui_text(ui, "Front panel color");

    ui_push_element_size(ui, 100, 0);
	ui_test_ShowColorPickerFor(ui, "Front panel color"       , &ui->theme.frontPanelColor);
	ui_test_ShowColorPickerFor(ui, "Frame panel color"       , &ui->theme.frame_background_color);
	ui_test_ShowColorPickerFor(ui, "Scrollbar color"         , &ui->theme.scrollbarColor);
	ui_test_ShowColorPickerFor(ui, "Button color"            , &ui->theme.button_normal_color);
	ui_test_ShowColorPickerFor(ui, "Button hot color"        , &ui->theme.button_hot_color);
	ui_test_ShowColorPickerFor(ui, "Button interacting color", &ui->theme.button_interacting_color);
	ui_test_ShowColorPickerFor(ui, "Title color"             , &ui->theme.titleColor);
	ui_pop_element_size(ui);

	ui_text(ui, "Title color");

	ui_keep_line_push(ui);
	ui_drag_f32(ui, 0, 255, 1.0f, &ui->theme.titleColor.x, "Title_r");
	ui_drag_f32(ui, 0, 255, 1.0f, &ui->theme.titleColor.y, "Title_g");
	ui_drag_f32(ui, 0, 255, 1.0f, &ui->theme.titleColor.z, "Title_b");
	ui_drag_f32(ui, 0, 255, 1.0f, &ui->theme.titleColor.w, "Title_a");
	ui_keep_line_pop(ui);

	ui_space(ui, 4);
	ui_button(ui, "Button Hi!");
	ui_button(ui, "Button Hi again!");

	ui_begin_child_panel(ui, 200, 400, "_Child panel 0");
	{
	  ui_text(ui, "Text child!");
	  ui_button(ui, "Button child!");
	}
	ui_end_child_panel(ui);
	ui_QuickToolTip(ui, 40, 40, "ToolTip!");
	ui_text(ui, "(?)");
#if 1
    ui_begin_drop_down(ui, "1dd", "Drop down preview");
	{
	  ui_selectable(ui, 0, "selection inside drop down! 0");
	  ui_selectable(ui, 0, "selection inside drop down! 1");
	  ui_selectable(ui, 0, "selection inside drop down! 2");
	  ui_selectable(ui, 0, "selection inside drop down! 3");
	  ui_selectable(ui, 0, "selection inside drop down! 4");
	  ui_selectable(ui, 0, "selection inside drop down! 5");
	  ui_selectable(ui, 0, "selection inside drop down! 6");
	  ui_selectable(ui, 0, "selection inside drop down! 7");
	  ui_selectable(ui, 0, "selection inside drop down! 8");
	  ui_selectable(ui, 0, "selection inside drop down! 9");
	  ui_selectable(ui, 0, "selection inside drop down! 10");
	  ui_selectable(ui, 0, "selection inside drop down! 11");
	  ui_selectable(ui, 0, "selection inside drop down! 12");
	  ui_selectable(ui, 0, "selection inside drop down! 13");
	  ui_selectable(ui, 0, "selection inside drop down! 14");
	}
	ui_end_drop_down(ui);
#endif
	static u8 staticString[64] = {"This is a static string text!"};
	static u32 staticU32 = 32;
	static u16 staticU16 = 16;
	static f32 staticF32 = 32.00f;
	static u32 staticCheck = 0;
#if 0
	ui_text(ui, "Input_Text");
	ui_input_text(ui, input_text, "IO_text", sizeof(staticString), staticString);
	ui_text(ui, "Input_u32");
	ui_Input_u32(ui, input_text, "IO_u32", &staticU32);
	ui_text(ui, "Input-f32");
	ui_Input_f32(ui, input_text, "IO_f32", &staticF32);
#endif
	ui_spinner_u32(ui, 1, 0, U32MAX, &staticU32, ui_text_input_confirm_on_enter, "UpDown_u32");
	ui_spinner_u16(ui, 1, 0, U16MAX, &staticU16, 0, "UpDown_u16");
	ui_spinner_f32(ui, 1, 0, F32MAX, &staticF32, 0, "UpDown_f32");
    ui_check_box(ui, &staticCheck, "Check box");

	if(ui_button(ui, "Test explorer"))
	{
		ui_explorer_set_flags_and_process(ui, 0, ui_explorer_flags_copy_selected_file_name, "Free explorer test");
	}

	ui_selectable(ui, 0, "Right click me");

	ui_tab_group_begin(ui, 0, "Tab group test");
	{
		if(ui_tab_push(ui, "Tab 1"))
		{
			ui_text(ui, "This is tab 1");
		}
		if(ui_tab_push(ui, "Tab 2"))
		{
			ui_text(ui, "Now this is tab 2!");
		}
	}
	ui_tab_group_end(ui);
	//if(ui_last_element_right_clicked)
	//{
	//}
	


	ui_panel_end(ui);
}
