#define render_MAX_SWAP_BUFFERS 2
#define SPRITE_ORIENTATION_MAX (16)

#define RENDER_SWAP_BUFFERS(name) \
	name(struct s_platform_renderer *r, struct game_renderer *game_renderer)
#define RENDER_DRAW_START(name) \
	name(struct s_platform_renderer *r, struct game_renderer *game_renderer)
#define RENDER_DRAW_END(name) \
	name(struct s_platform_renderer *r, struct game_renderer *game_renderer)
#define RENDER_PUSH_TEXTURE(name) \
	struct render_texture name(struct s_platform_renderer *r, u8 *pixels, u32 w, u32 h, u32 bpp, u32 index)
#define RENDER_ALLOCATE_FRAME_BUFFER(name) \
    u32 name(struct s_platform_renderer *r, u32 w, u32 h)
typedef RENDER_SWAP_BUFFERS(render_swap_buffers);
typedef RENDER_DRAW_START(render_draw_start);
typedef RENDER_DRAW_END(render_draw_end);
typedef RENDER_PUSH_TEXTURE(render_push_texture);
typedef RENDER_ALLOCATE_FRAME_BUFFER(render_allocate_frame_buffer);

//repeat repeats texture
//clamp makes the colors outside the range 0 or 1
//mirror mirrors the texture
//specified makes the colors outside the range use a specified color
//clamp_to_edge takes the value of the last edge and repeats it.
typedef enum{
	uv_adress_repeat,
	uv_adress_clamp,
	uv_adress_mirror,
	uv_adress_specified,
	uv_adress_clamp_to_edge,
}renderer_uv_adress;

typedef enum{
	filter_linear, //or point
	filter_bilineal
}renderer_filter;

typedef enum 
{
    render_api_d3d11,
    render_api_opengl,
    render_software,
}render_api;

typedef struct{
	render_api api;

	u32 texture_array_w;
	u32 texture_array_h;
	u32 texture_array_capacity;
	u32 maximum_quad_size;
	u32 back_buffer_width;
	u32 back_buffer_height;

    renderer_uv_adress uv_adress;
	renderer_filter min_filter;
	renderer_filter max_filter;
	renderer_filter mip_filter;

	u32 total_depth_peel_layers;
	u32 display_buffers_count;
}platform_renderer_init_values;

typedef enum{
	render_shader_3d,
	render_shader_2d,
}render_shader_program;

typedef struct s_platform_renderer{
    render_api type;
	render_swap_buffers *f_swap_buffers;
	render_draw_start *f_draw_start;
	render_draw_end *f_draw_end;
	render_push_texture *f_push_texture;

	u32 device;
}platform_renderer;

typedef struct s_platform_renderer_init_functions{
	render_allocate_frame_buffer *f_allocate_frame_buffer;
}platform_renderer_init_functions;

//this is what is sent to the GPU as the vertex buffer.
typedef struct {
    //Note(Agu): I might need a vec4 to send a z-bias
    vec3 location;
    vec2 uv;
    u32 color;
    u32 texture;
}render_vertex;

typedef struct{
	u32 offset;
	u32 count;
}render_locked_vertices_group;


#define TEXTUREREQUESTED (1 << 16) 
#define TEXTUREUNLOADED ((1 << 16)- 1)
#define TEXTUREWHITEID 'WWWW'
typedef struct render_texture
{
    //uint32 imageId;
	//handle
    u32 index;

    uint16 offsetX;
    uint16 offsetY;

    uint16 width;
    uint16 height;
} render_texture;


typedef enum{
	request_open,
	request_closed,
	request_ready_to_transfer,
	request_complete
}texture_request_state;


typedef struct render_texture_slot{
	u16 index;
	b16 used;
	struct render_texture_slot *next;
}render_texture_slot;

typedef struct s_texture_request{

	texture_request_state state;
	u16 avadible_texture_index;
	u16 request_index;
	u8 *pixels;
	struct s_texture_request *next;

}texture_request;


typedef struct{

	u16 texture_array_w;
	u16 texture_array_h;

	u8 *texture_request_pixels;
	u16 texture_request_count;
	u16 texture_request_max;
	texture_request *requested_textures;
	texture_request *first_free_texture_request;
	texture_request *first_texture_transfer;

	u16 texture_slots_count;
	u16 texture_slots_max;
	render_texture_slot *texture_slots;
	render_texture_slot *first_free_texture_slot;
}render_texture_data;


typedef enum{

	font_text_pad_center,
	font_text_pad_left,
	font_text_pad_right,
	font_text_pad_topright,
	font_text_pad_topleft,
	font_text_pad_bottomright,
	font_text_pad_bottomleft,

}font_text_pad;

typedef struct{
    uint16 offsetX;
    uint16 offsetY;
}font_glyph;

typedef struct {
    render_texture texture;
    u32 font_height;
    u32 glyph_count;

    font_glyph *glyphs;
    int32 *horizontal_displacements;
}font_proportional;

typedef struct {
    render_texture font;
    uint16 framewidth;
    uint16 frameheight;
    uint16 textSpacingX;
    uint16 textSpacingY;
}PixelFont;

inline render_texture
render_texture_Default(render_texture texture)
{
    texture.offsetX = 0;
    texture.offsetY = 0;
    texture.width = 512;
    texture.height = 512;
    return(texture);
}

//
// RENDER COMMANDS
//
typedef enum{
   render_command_type_undefined,
   render_command_type_clear,
   render_command_type_drawquad,
   render_command_type_PushClip,
   render_command_type_PopClip,
   render_command_type_draw_locked_vertices,
   render_command_type_switch_viewport,
}
render_command_type;

typedef struct{
    render_command_type type;
	u32 groupIndex;
}render_command_draw_locked_vertices_data;

typedef struct{
    render_command_type type;
}
render_command_SetClip;

typedef struct{
    render_command_type type;
}
render_command_PopClip;

typedef struct{
    render_command_type type;
    render_vertex vertices[4];
}
render_command_drawquad;

typedef struct{
    render_command_type type;
    real32 r, g, b, a;
}
render_command_clear;

#define render_flags_default3D (render_flags_DepthTest | render_flags_DepthPeel)
typedef enum{
	render_flags_Blending = 0x01,
	render_flags_DepthTest = 0x02,
	render_flags_DepthPeel = 0x04,
	render_flags_set_viewport_and_clip = 0x08,
	render_flags_set_display_buffer = 0x10
}render_flags;

typedef enum{
	render_camera_perspective,
	render_camera_orthographic,
	render_camera_2d,
	render_camera_scale_to_display,
}render_camera_type;

struct game_renderer;
typedef struct {
    //uint32 draw_count;
    struct game_renderer *gameRenderer;


    u32 command_count;
	render_flags render_flags;
	render_camera_type camera_type;

    u8 *commands_base;
    u8 *commands_offset;

	u32 display_buffer_index;
	u16 set_viewport_and_clip;
	u16 restore_viewport_and_clip;
	rectangle32s viewport_and_clip;
}
render_commands;

struct render_scissor;
typedef struct render_scissor{

	rectangle32s clip;
	u32 previous;

}render_scissor;

#define render_TEXTUREWH 512

typedef struct game_renderer {



   //The maximum amount of draw calls. To get the real maximum, this number
   //should be multiplied by 4.
   
   enum32(render_api) api;
   uint32 max_quad_draws;
   f32 sprite_skew;
   f32 fov;

   u16 texture_array_w;
   u16 texture_array_h;
   u32 texture_array_capacity;

   //client resolution (not window)
   uint16 os_window_width;
   uint16 os_window_height;

   //monitor display resolution
   u16 display_width;
   u16 display_height;

   //game actual resolution
   u16 back_buffer_width;
   u16 back_buffer_height;

   rectangle32s current_draw_clip;
   rectangle32s game_draw_clip;

   uint16 begin_count;
   uint16 draw_count;

   f32 camera_zoom;
   vec3 camera_position;
   vec3 camera_rotation;
   vec3 camera_x;
   vec3 camera_y;
   vec3 camera_z;

   matrix4x4 camera_transform;
   matrix4x4 camera_transform_inverse;
   matrix4x4 projection;
   matrix4x4 projection_inverse;
   matrix4x4_data projections;

   f32 camera_zoom_2d;
   matrix4x4 projection_2d;
   vec2 camera_p_2d;
   //The scissor or clip cuts the current viewport and renders only the specified part
   //scissor_push_count is the total amount of scissors pushed on the current frame
   //scissor_total_count is the total capacity of the "scissor stack"
   //scissor_stack_index is the current scissor that is being used on scissor_stack
   //scissors_on_stack is the current amount of scissors "in the stack"
   //of scissors, which means that this will decrease every time
   //pop_clip is called.
   uint16 scissor_push_count;
   uint16 scissor_total_count;

   //the current clip/scissor index
   uint16 scissor_stack_index;
   uint16 scissors_on_stack;
   
   render_scissor *scissor_stack;

   uint32 depth_peel_calls;

   u32 quads_locked_from_base;


   //Vertex locks
   //locked vertices calls are divided by groups between render_push_locked_vertices
   //and render_pop_locked_vertices
   //Boolean for knowing if the next draw calls should be locked
   u32 lock_next_vertices;
   //used when reading the render_command_draw_locked_vertices
   u32 current_locked_vertices_pushed;
   u32 locked_vertices_group_count;
   u32 locked_vertices_group_max;
   /*"Groups" of locked vertices. These contain an offset to the locked_vertices
	  array to point which part of the array this group belongs and the total
	  amount of pushed vertices.
   */
   render_locked_vertices_group *locked_vertices_groups;
   //number of times the vertex buffer was pushed to one of the swap buffers
   //this is used for drawing in every swap buffer before locking the vertices
   u32 locked_vertices_in_swap_buffer;

   u16 update_locked_vertices;
   //array of locked vertices before allocating them at the base of current_vertex_buffer 
   u32 locked_quads_max;
   u32 locked_quads_count;
   render_vertex *locked_vertices;

   //vertex buffer of the current swap buffer
   render_vertex *current_vertex_buffer;

   //total space allocated for the render commands
   u32 render_commands_buffer_size;
   u8 *render_commands_buffer_base;
   u8 *render_commands_buffer_offset;

   //color used to clear the screen
   f32 clear_color[4];

//   render_shader_program currentShaderProgram;

   //used for missing textures of debugging purposes.
   render_texture white_texture;

   //textures requested for allocating on the gpu
   //TODO: since these calls might fail. I still need
   //to think of a way to handle this.
   //u16 texture_request_count;
   //u16 texture_request_max;
   //texture_request *requested_textures;
   //texture_request *first_free_texture_request;
   //texture_request *first_texture_transfer;

   //u16 texture_slots_count;
   //u16 texture_slots_max;
   //render_texture_slot *texture_slots;
   //render_texture_slot *first_free_texture_slot;
   ////pixel buffer to contain the requested texture's pixels
   //u8 *texture_request_pixels;

   render_texture_data texture_operations;
    
}game_renderer;


//
// __Font functions
//
static inline uint8 
CharToSrc94(char c)
{
    uint8 charx = c - 32;
    return charx > 94 ? 94 : charx; 
}


inline i32
font_get_kerning_pair(
		font_proportional *font,
		u32 code0,
		u32 code1)
{
       u32 char0 = CharToSrc94(code0);
	   u32 char1 = CharToSrc94(code1);
       u32 glyph_table_index = char0 * font->glyph_count;
       i32 kerning_delta    = char1 ? font->horizontal_displacements[glyph_table_index + char1] : 0;
	   return(kerning_delta);
}

static f32 
font_GetWidestWidth(font_proportional *fontData, f32 scale)
{
	u8 c = 'W';

    u32 glyphI		    = (u32)CharToSrc94(c);
    u32 glyphTableIndex  = glyphI * fontData->glyph_count;
    i32 hDisplacement	    = fontData->horizontal_displacements[glyphTableIndex];
    f32 textX = hDisplacement * scale;
	return(textX);
}

static f32 
font_em(font_proportional *fontData, f32 scale)
{
	u8 c = 'W';

    u32 glyphI		    = (u32)CharToSrc94(c);
    u32 glyphTableIndex  = glyphI * fontData->glyph_count;
    i32 hDisplacement	    = fontData->horizontal_displacements[glyphTableIndex];
    f32 textX = hDisplacement * scale;
	return(textX);
}

#if 0
typedef struct{

	u32 index;
	u32 tableIndex;
	i32 horizontalDisplacement;
	i32 nextPairDelta;
}font_glyph_info;

inline font_glyph_info
font_GetGlyphInfo(font_proportional ,u32 arrayIndex)
{
	font_glyph_info result = {0};

	   nextChar = text[arrayIndex];
       u32 glyphI		   = (u32)CharToSrc94(c);
       u32 glyphTableIndex = glyphI * fontData->glyph_count;
       i32 hDisplacement   = fontData->horizontal_displacements[glyphTableIndex];
       i32 kerningDelta    = nextChar ? fontData->horizontal_displacements[glyphTableIndex + nextChar] : 0;
}
#endif

#define font_get_text_size(font, text) font_get_text_size_wrapped_scaled(font, F32MAX, text, 1)
#define font_get_text_size_scaled(font, scale, text) font_get_text_size_wrapped_scaled(font, F32MAX, text, scale)

inline vec2 
font_get_text_size_wrapped_scaled(font_proportional *fontData,
		                          f32 endX,
								  char *text,
								  f32 scale)
{
	u32 nextChar = 0;
	f32 maxX = 0;
    f32 textX = 0;
	f32 textY = fontData->font_height * scale; 
	u8 c = 0;
    for(u32 i = 0;
        (c = text[i]) != '\0';
        i++)
    {
	   nextChar = CharToSrc94(text[i]);
       u32 glyphI		   = (u32)CharToSrc94(c);
       u32 glyphTableIndex = glyphI * fontData->glyph_count;
       i32 hDisplacement   = fontData->horizontal_displacements[glyphTableIndex];
       i32 kerningDelta    = nextChar ? fontData->horizontal_displacements[glyphTableIndex + nextChar] : 0;
//	   Assert(kerningDelta == 0);

	   f32 nextSize = (hDisplacement + kerningDelta) * scale;

       if(c == '\n' || (textX + nextSize) > endX)
       {
           textY += fontData->font_height * scale;
		   maxX = MAX(maxX, textX);
           textX = 0;
           continue;
       }

       textX += (hDisplacement + kerningDelta) * scale;
    }  

	maxX = MAX(maxX, textX);
	vec2 result = {maxX, textY};
	return(result);
}

#define font_GetTextWidth(fontData, scale, text) font_get_remaining_width_at(fontData, 0, U32MAX, scale, text)
inline f32 
font_get_remaining_width_at(font_proportional *fontData, u32 textStart, u32 textEnd, f32 scale, char *text)
{
    u32 lastChar = 0; 
    f32 textX = 0;
	u8 c = 0;
    for(u32 i = textStart;
        i < textEnd && (c = text[i]) != '\0';
        i++)
    {
       u32 glyphI		   = (u32)CharToSrc94(c);
       u32 glyphTableIndex = glyphI * fontData->glyph_count;
       i32 hDisplacement   = fontData->horizontal_displacements[glyphTableIndex];
       i32 kerningDelta    = lastChar ? fontData->horizontal_displacements[glyphTableIndex + lastChar] : 0;
       textX += (hDisplacement + kerningDelta) * scale;
	   lastChar = glyphI;
    }  
	return(textX);
}

inline u32 
font_get_closest_index_at(font_proportional *fontData, u8 *text, f32 scale, f32 at)
{
	u32 result = 0;
	u32 nextChar = 0;
    f32 textX = 0;
	f32 textY = fontData->font_height * scale; 
	u8 c = 0;
    for(u32 i = 0;
        (c = text[i]) != '\0';
        i++)
    {
	   nextChar = CharToSrc94(text[i]);
       u32 glyphI		   = (u32)CharToSrc94(c);
       u32 glyphTableIndex = glyphI * fontData->glyph_count;
       i32 hDisplacement   = fontData->horizontal_displacements[glyphTableIndex];
       i32 kerningDelta    = nextChar ? fontData->horizontal_displacements[glyphTableIndex + nextChar] : 0;

	   f32 nextSize = (hDisplacement + kerningDelta) * scale;

	   if((textX + nextSize) > at)
	   {
		   break;
	   }
	   result++;

       textX += nextSize; 
    }  

	return(result);
}

#define font_get_text_pad_offset_vec2(fontData, size, text, scale, padOptions) font_get_text_pad_offset(fontData, size.x, size.y, text, scale, padOptions)

static vec2
font_get_text_pad_offset(
		font_proportional *fontData,
		f32 frame_size_x,
		f32 frame_size_y,
		f32 scale,
		font_text_pad padOptions,
		u8 *text)
{
	vec2 textDim = font_get_text_size_wrapped_scaled(fontData, F32MAX, text, scale);
	vec2 textOff = { (frame_size_x - textDim.x),
					 (frame_size_y - textDim.y)};
	f32 textDistance = 2.0f;
	switch(padOptions)
	{
		case font_text_pad_center:
			{
				textOff.x *= 0.5f;
				textOff.y *= 0.5f;
								 
			}break;
		case font_text_pad_left:
			{
				textOff.x = textDistance; 
				textOff.y *= 0.5f;
			}break;
		case font_text_pad_right:
			{
				textOff.x -= textDistance; 
				textOff.y *= 0.5f;

			}break;
		case font_text_pad_topleft:
			{
				textOff.x = textDistance; 
				textOff.y = textDistance;
			}break;
		case font_text_pad_topright:
			{
				textOff.x -= textDistance; 
				textOff.y = textDistance; 

			}break;
		case font_text_pad_bottomleft:
			{
				textOff.x = textDistance; 
				textOff.y -= textDistance;
			}break;
		case font_text_pad_bottomright:
			{
				textOff.x -= textDistance; 
				textOff.y -= textDistance;

			}break;

	}

	return(textOff);
}

inline f32
font_get_scaled_height(font_proportional font, f32 scale)
{
	return(font.font_height * scale);
}

static void *
render_push_to_command_buffer(game_renderer *gameRenderer, u32 size)
{ 
	Assert(gameRenderer->render_commands_buffer_offset + size < gameRenderer->render_commands_buffer_base + gameRenderer->render_commands_buffer_size);

    void *command = gameRenderer->render_commands_buffer_offset;
    gameRenderer->render_commands_buffer_offset += size; 

	return(command);
	
}

static inline matrix4x4
render_set_scaled_to_display(game_renderer *gameRenderer)
{

   f32 bsX = (f32)gameRenderer->back_buffer_width  / gameRenderer->display_width;
   f32 bsY = (f32)gameRenderer->back_buffer_height / gameRenderer->display_height;
   f32 dW = gameRenderer->display_width * bsX;
   f32 dH = gameRenderer->display_height * bsY;
   f32 wW = (f32)gameRenderer->os_window_width;
   f32 wH = (f32)gameRenderer->os_window_height;

   f32 x = (f32)dW / wW; 
   f32 y = (f32)dH / wH;


   matrix4x4 WVP = matrix4x4_Identity();
   WVP.m[0][0] = x;
   WVP.m[1][1] = y;
   WVP.m[0][3] = ((dW - wW) / wW);
   WVP.m[1][3] = (-(dH - wH) / wH);

   return(WVP);
}

static inline render_texture_slot *
render_get_texture_slot(render_texture_data *tops)
{
	render_texture_slot *result = tops->first_free_texture_slot;
	if(result)
	{
		result = tops->first_free_texture_slot;
		tops->first_free_texture_slot = result->next;
	}
	else
	{
		result = tops->texture_slots + tops->texture_slots_count;
		result->index = tops->texture_slots_count++;
		result->used = 1;
		Assert(tops->texture_slots_count < tops->texture_slots_max);
	}
	return(result);
}

static inline void
render_free_texture_slot(
		render_texture_data *tops,
		u32 index)
{
	render_texture_slot *slot_to_free = tops->texture_slots + index;

	slot_to_free->used = 0;
	slot_to_free->next = tops->first_free_texture_slot;
	tops->first_free_texture_slot = slot_to_free;
}

static inline render_texture_slot *
render_use_texture_slot(
		render_texture_data *tops,
		u32 index)
{
	render_texture_slot *slot = tops->texture_slots + index;
	if(!slot->used)
	{
		slot = render_get_texture_slot(tops);
	}
	return(slot);
}

static inline void
render_free_texture_request(
		render_texture_data *tops,
		texture_request *request)
{
	request->next = tops->first_free_texture_request;
	tops->first_free_texture_request = request;
}

static inline texture_request *
render_texture_request_start(
		render_texture_data *tops,
		render_texture_slot *texture_slot)
{
	texture_request *new_request = tops->first_free_texture_request;
	if(new_request)
	{
		new_request = tops->first_free_texture_request;
		tops->first_free_texture_request = new_request->next;
	}
	else
	{
		new_request = tops->requested_textures + tops->texture_request_count;
		new_request->request_index = tops->texture_request_count++;
		Assert(tops->texture_request_count < tops->texture_request_max);
	}
	Assert(new_request);

	new_request->pixels = (u8 *)((u32 *)tops->texture_request_pixels +
		(tops->texture_array_w * 
		 tops->texture_array_h * 
		 new_request->request_index));
	new_request->avadible_texture_index = texture_slot->index;

	new_request->next = tops->first_texture_transfer;
	tops->first_texture_transfer = new_request;

	//renderer->texture_request_count++;
	return(new_request);
}

static inline void
render_texture_request_end(
		render_texture_data *tops,
		texture_request *request)
{
	//this should be changed obviously
	request->state = request_ready_to_transfer;
	if(request->state = request_ready_to_transfer)
	{
		request->next = tops->first_texture_transfer;
		tops->first_texture_transfer = request;

	}
	else
	{
		//Implement free.
	}
}

//transfer pixels and set this to transfer to the gpu
static inline void
render_texture_request_push_pixels(
		render_texture_data *tops,
		texture_request *request,
		u8 *pixels)
{
	   u32 *src_pixels32 = (u32 *)pixels;
	   u32 *dest_pixels = (u32 *)request->pixels;
       u32 image_size = tops->texture_array_w * tops->texture_array_h * 4;
       for(u32 i = 0; i < image_size; i++)
       {
		  dest_pixels[i] = src_pixels32[i];
       }

	   request->state = request_ready_to_transfer;
}

static inline void
render_texture_request_fill_pixels(
		render_texture_data *tops,
		texture_request *request,
		u32 fill_value)
{
	   u32 *dest_pixels = (u32 *)request->pixels;
       u32 image_size = tops->texture_array_w * tops->texture_array_h * 4;
       for(u32 i = 0; i < image_size; i++)
       {
		  dest_pixels[i] = fill_value;
       }

	   request->state = request_ready_to_transfer;
}

static inline u32
renderer_allocate_and_initialize(
		memory_area *area,
		game_renderer *renderer)
{
	if(!renderer)
	{
		return(0);
	}

	if(!renderer->back_buffer_width)
	{
		renderer->back_buffer_width = 800;
	}
	if(!renderer->back_buffer_height)
	{
		renderer->back_buffer_height = 600;
	}


	if(!renderer->render_commands_buffer_size || renderer->render_commands_buffer_size < MEGABYTES(2))
	{
		//default to 2 mb
		renderer->render_commands_buffer_size = MEGABYTES(2);
	}

	return(1);
}

static inline render_texture_data 
renderer_allocate_texture_requests(
		game_renderer *game_renderer,
		memory_area *area)
{
	u32 texture_request_max = 
		game_renderer->texture_array_capacity / 2;
	u32 total_texture_request_size = texture_request_max * 
		game_renderer->texture_array_w * game_renderer->texture_array_h * 4;

	render_texture_data tops = {0};
	tops.texture_request_pixels = memory_area_push_size(
			area,
			total_texture_request_size);

	tops.requested_textures = memory_area_push_array(
			area,
			texture_request,
			texture_request_max);
	tops.texture_request_max = texture_request_max;

	tops.texture_slots_max = game_renderer->texture_array_capacity;
	tops.texture_slots = memory_area_push_array(
			area,
			render_texture_slot,
			tops.texture_slots_max);
	tops.texture_array_w = game_renderer->texture_array_w;
	tops.texture_array_h = game_renderer->texture_array_h;
	return(tops);

}

static vec2
renderer_convert_to_frame_buffer_coordinates(
		game_renderer *game_renderer,
		u32 screen_w,
		u32 screen_h,
		f32 point_x,
		f32 point_y)
{

	//first scale with the framebuffer.
    f32 scaleDifference_x = (f32)game_renderer->back_buffer_width  / screen_w;
    f32 scaleDifference_y = (f32)game_renderer->back_buffer_height / screen_h;

    vec2 result;
    result.x = point_x * scaleDifference_x;
    result.y = point_y * scaleDifference_y;
	return(result);

}

typedef enum{
	orientation_1,
	orientation_4,
	orientation_8,
	orientation_16,
	orientation_8_flip,
	orientation_16_flip,

	orientation_option_count
}sprite_orientation_option;



static inline u32
orientation_count_from_option(
		sprite_orientation_option option)
{
	u32 result = 1;
	u8 counts[orientation_option_count ] = {
	1, 4, 8, 16, 4, 8};
	if(option < orientation_option_count)
	{
		result = counts[option];
	}
	return(result);
}

typedef enum{
	frame_selection_null,
	frame_selection_rotation_x_mt,
	frame_selection_rotation_x_lt
}frame_list_selection;

typedef enum{
	model_animation_spline_near,
	model_animation_spline_linear,
	model_animation_spline_smoothin,
	model_animation_spline_smoothout,

	model_animation_spline_total
}model_animation_spline;

typedef enum{
	model_sprite_mesh,
	model_sprite_billboard
}model_sprite_type;

typedef enum{
	billboard_face_x,
	billboard_face_xy
}billboard_face_axis;


typedef struct{
	game_renderer *game_renderer;
	f32 fov;
	f32 sprite_skew;
	f32 distance_camera_target;
	u16 camera_mode;
	f32 camera_zoom;
	f32 z_fix;
	union{
		vec3 camera_rotation;
		struct{
			f32 camera_rotation_x;
			f32 camera_rotation_y;
			f32 camera_rotation_z;
		};
	};
	vec3 camera_target;
	vec3 camera_position;
}game_render_parameters;

typedef struct{

	u32 totalMiliseconds;
	u16 fx;
	u16 fy;
}sprite_animation_frame;

typedef struct{
	u32 stop_at_end;

	u16 current_frame;
	u16 frames_total;
	f32 timer_total;

    u16 frame_w;
	u16 frame_h;
	sprite_animation_frame *frames;
	
}sprite_animation;


typedef struct{
	union{
		uvs uvs;
		struct{
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
		};
	};
}model_uvs;

typedef struct{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;
}model_vertices;

typedef struct{

	union{
		struct{
	        vec3 v0;
	        vec3 v1;
	        vec3 v2;
	        vec3 v3;
		};
		model_vertices vertices;
	};


	union{
		struct{
	        vec2 uv0;
	        vec2 uv1;
	        vec2 uv2;
	        vec2 uv3;
		};
		model_uvs uvs;
	};
}model_mesh;

typedef enum{
	model_animation_keyframe_transform,
	model_animation_keyframe_frame
}model_animation_keyframe_type;

typedef struct{

	    model_animation_keyframe_type type;

		b16 switch_parent;
		u16 parent_index;
		b32 timer_frame;
		u16 frame_start;
		u16 timer_frame_repeat;
		model_animation_spline spline;

		union{
			u16 bone_index; // or bone
			u16 mesh_index;
		};
		union{
		u16 frame_list_index;
		u16 frame_key;
		};
		u16 frame_list_frame_index;
		b32 flip_h;

		vec3 offset;
		vec3 position;
		quaternion q;
		f32 rotation_x;
		f32 rotation_y;
		f32 rotation_z;

		b16 modifies_attached_model;
		u16 attached_model_index;

}model_animation_keyframe;

typedef struct{
//	u32 mesh_index;
	u16 uvs_at;
	u16 uvs_count;
}model_mesh_frame;

typedef struct{
	//index to the specified sprite sheet
	u16 sprite_index;
	u16 total_frames_count;
	union{

		u16 frames_count;
		u16 uvs_count;
	};
	union{
		u16 frames_at;
		u16 uvs_at;
	};
}model_mesh_frame_list;

typedef struct{

	u16 keyframe_count;
	u16 keyframes_at;
	u16 frame_keyframes_at;
	u16 frame_keyframe_count;
	u16 subkeys_count;
	u16 subkeys_at;

	u32 frames_total;
	u32 frames_per_ms;
	//loops between these frames
	b32 loop;
	u16 frame_loop_start;
	u16 frame_loop_end;

	b16 frame_timer;
	u16 frame_timer_repeat;

	b16 keep_timer_on_transition;
	b16 repeat;

}model_animation;

typedef struct{

	//u32 current_animation_index;
	//u16 frames_total;
	u32 frame_current;
	u32 frame_transition;
	f32 dt_current;
	f32 dt_transcurred;

}model_animation_timer;

typedef struct{

	u16 animation_count;
	u16 current_animation_index;
	model_animation *animations;//array
	u16 keyframe_uvs_max;
	u16 keyframe_uvs_count;
	model_uvs *keyframe_uvs;

	u32 mesh_frame_count;
	model_mesh_frame *mesh_frames;

	u16 keyframe_count;
	model_animation_keyframe *keyframes;
	u16 clip_count;
	model_animation_timer timer;

}model_animations;

typedef struct{

	sprite_orientation_option option;
	i16 x_rot_index;
	i16 y_rot_index;
	u32 skin_index;

	union{
		uvs uvs;
		vec2 u[4];
		struct{
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
		};
	};
	vec3 offset;

}sprite_orientation;

typedef struct{
	u16 bone_index;
	u16 skin_count;
	u16 skin;
	vec3 p;

	//for sprite_orientation
	//for multiple "skins" or clothing,
	//I probably should point to a frame list and choose from there...
	u16 sprite_sheet_index;
	u16 extra_frame_count;
	u16 frame_at;

	f32 depth_x;
	f32 depth_y;
	f32 depth_z;

	model_sprite_type type;
	u32 texture_index;
	b16 flip_h;
	b16 flip_v;

	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;

	vec3 size;
	vec3 size2;

	union{
		vec3 pivot;
		struct{
			f32 pivotX;
			f32 pivotY;
			f32 pivotZ;
		};
	};

	billboard_face_axis face_axis;

	u16 frame_list_index;
	u16 frame_list_frame_index;

	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;


}model_sprite;

typedef struct{
	u8 parent;
	u8 dof;

	u16 virtual;
	//bone to modify
	u16 virtual_attachment_bone;
	//model to modify
	u16 attached_model_index;

	u16 frame_key;
	u16 frame_key_count;
	u16 frame_key_at;

	u16 sprites_at;
	u16 sprite_count;

	vec3 p;
	vec3 pivot;
	vec3 transformed_p;
	vec3 displacement;
	vec3 normal;
	b16 two_dim;
	b16 flip_h;

	union{
		quaternion q;
		struct{
			f32 r_angle;
		};
	};
	quaternion transformed_q;

	f32 rotation_x;
	f32 rotation_y;
	f32 rotation_z;

	f32 transformed_rx;
	f32 transformed_ry;
	f32 transformed_rz;

}model_bone;

typedef struct{
	model_bone *bones;
	model_sprite *sprites;
}model_pose;

typedef struct s_model{
	u32 bone_count;
	u32 sprite_count;
	u32 total_sprite_count;
	u32 sprite_sheet_count;
	u32 uvs_count;
	u32 orientation_amount;

	union{
		model_pose pose;
		struct{
			union {
				model_bone *bones;
				model_bone *nodes;
			};
			model_sprite *sprites;
		};
	};
	model_pose rest_pose;
	sprite_orientation *uvs;
	struct render_texture **sprite_sheets_a;

	//animation data
	u32 animation_count;
	model_animation *animations;//array

	u32 frame_list_count;
	model_mesh_frame_list *mesh_frame_list;

	u16 keyframe_count;
	model_animation_keyframe *keyframes;
	u16 frame_keyframe_count;
	model_animation_keyframe *frame_keyframes;

	/*
	   u32 modifiers_count;
	   u32 modifiers_max;
	   struct{
	       u32 sprite_index;
		   u32 texture_index;
	   }keyframe_frame_modifiers*;
	*/
}model;

typedef struct{
	u32 attached_model;
	u16 parent;
	u16 child;
}model_virtual_node;

typedef struct{
	//NOTE: normally, index should be used to pick a model from the
	//"attached_models" index, but a pointer should make testing a bit easier.
	model *model;
	model_pose animated_pose;
	//bone attached to
	u32 bone_index;
	//not used for now
	u16 virtual_node_count;
	model_virtual_node *virtual_nodes;
}model_attachment_data;

typedef struct{
	model *model;
	model_pose animated_pose;
	u16 current_animation_index;
	model_animation_timer timer;

	u16 attached_models_count;
	model **attached_models;
	model_attachment_data *attach_data;
}model_render_data;

typedef struct{
	f32 timer;
	u32 current_frame;
}frame_timer;

typedef struct{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;
}mesh_points;

static void 
frame_timer_miliseconds_run(f32 *timer,
		                    u32 *current_frame,
							u32 total_frames,
		                    u32 total_miliseconds,
					        u32 loop_at_end,
		                    f32 dt)
{
    *timer += dt * 1000;
    if(*timer > total_miliseconds)
    {
        (*timer) = 0;
        (*current_frame)++;
    }
	if(*current_frame >= total_frames)
	{
		if(!loop_at_end)
		{
			(*current_frame) = total_frames - 1;
		}
		else
		{
			(*current_frame) = 0;
		}
	}
}

static void 
render_reproduce_sprite_animation(sprite_animation *sprite,
		                          f32 dt)
{
    sprite->timer_total += dt * 1000;
	sprite_animation_frame *frame_current = sprite->frames + sprite->current_frame;
    if(sprite->timer_total > frame_current->totalMiliseconds)
    {
        sprite->timer_total = 0;
        sprite->current_frame++;
    }
	if(sprite->current_frame >= sprite->frames_total)
	{
		if(sprite->stop_at_end)
		{
			sprite->current_frame = sprite->frames_total - 1;
		}
		else
		{
			sprite->current_frame = 0;
		}
	}
}


static void 
model_animation_timer_run(model_animation *animation,
		                  model_animation_timer *animation_timer,
						  u32 pause,
						  f32 dt)
{
	f32 totalTime = (animation->frames_total * 0.1f);
	u32 loops = animation->loop;
	u32 frame_loop_start = animation->frame_loop_start;
	u32 frame_loop_end   = animation->frame_loop_end;


    animation_timer->dt_current += dt;
	animation_timer->dt_transcurred += dt;
	//if(animation->totalTimer > totalTime)
	//{
	//	animation->totalTimer = totalTime;
	//}
	if(loops)
	{
	    if(animation_timer->frame_current >= frame_loop_end)
	    {
	    	animation_timer->frame_current = frame_loop_start;
	    	animation_timer->dt_current = 0;
	    	animation_timer->dt_transcurred = frame_loop_start * 0.1f;
	    }

	}
	else
	{
	    if(animation_timer->frame_current >= animation->frames_total)
	    {
			//reset timer
			if(animation->repeat)
			{
				animation_timer->frame_current = 0;
				animation_timer->dt_current = 0;
				animation_timer->dt_transcurred = 0;
			}
			//keep timer
			else
			{
				animation_timer->frame_current = animation->frames_total;
				animation_timer->dt_current = 0;
				animation_timer->dt_transcurred = animation_timer->frame_current * 0.1f;
			}
	    }
	}
	//advance frame. 0.1 == 1 "frame". It means that the smaller this number, the faster the animation will go.
    if(animation_timer->dt_current > (0.1f / animation->frames_per_ms))
    {
		animation_timer->frame_current++;
		animation_timer->dt_current = 0;
		animation_timer->dt_transcurred = animation_timer->frame_current * 0.1f;
		//advance frame
    }
	Assert(animation_timer->frame_current < 256);



	//Update frames
#if 0
    for(u32 f = 0; f < animation->keyframe_count; f++)
	{
		model_animation_keyframe *keyFrame = animation->keyframes + f;

		u32 updateFrame = animation->frame_current > keyFrame->frameStart &&
			              animation->frame_current <= keyFrame->frameEnd;
		if(updateFrame)
		{
			//what now ?
		}

	}
#endif
}
inline u32
model_hierarchy_is_looping(model *model,
		              u32 bone_index)
{
	//is it's not the root bone
	u32 hierarchy_is_looping = 0;
	if(bone_index > 0 && model->bone_count > 0)
	{
	    model_bone *bone_array  = model->bones;
	    model_bone *target_bone = bone_array + bone_index;
	    model_bone *parent_bone = bone_array + target_bone->parent;
		u32 next_bone_index     = parent_bone->parent;

		u32 h = 0;
		while(next_bone_index > 0 && 
			  !hierarchy_is_looping)
		{
			if(next_bone_index == bone_index)
			{
				hierarchy_is_looping = 1;
			}
			parent_bone     = bone_array + next_bone_index;
			next_bone_index = parent_bone->parent;

		}
	}

	return(hierarchy_is_looping);
}

inline void
model_set_parent_bone(model *model,
		              u32 target_bone_index,
					  u32 new_parent_index)
{
	    model_bone *bone_array  = model->bones;
	    model_bone *target_bone = bone_array + target_bone_index;
		u32 current_parent      = target_bone->parent;
		u32 increased           = new_parent_index > current_parent;
		u32 bone_count          = model->bone_count;
		u32 pointing_itself_fixed = 0;

		//first make sure it's not pointing to itself
		if(new_parent_index == target_bone_index)
		{
		    if(increased)	
			{
				if(new_parent_index + 1 < bone_count)
				{
					new_parent_index++;
				}
				else
				{
					new_parent_index--;
				}
			}
			else
			{
				if(new_parent_index > 0)
				{
					new_parent_index--;
				}
				else
				{
					new_parent_index++;
				}
			}
			pointing_itself_fixed = 1;
		}
		target_bone->parent = new_parent_index;

		//then check if the bone hierarchy is looping
		u32 fix_hierarchy = model_hierarchy_is_looping(model, target_bone_index);

		while(fix_hierarchy)
        {
            if(increased)
			{
				if(new_parent_index + 1 < bone_count)
				{
					new_parent_index++;
				}
				else
				{
					new_parent_index--;
					//to start decreasing on the next loop
					increased = 0;
				}
			}
			else
			{
				if(new_parent_index > 0)
				{
					new_parent_index--;
				}
				else
				{
					increased = 1;
					new_parent_index++;
				}
			}
			target_bone->parent = new_parent_index;
		    fix_hierarchy       = model_hierarchy_is_looping(model, target_bone_index);
		}
}

inline mesh_points
quad_rotate_vertices_by_orientation(u32 orientation,
		                            vec3 v0,
									vec3 v1,
									vec3 v2,
									vec3 v3)
{

	orientation %= 4;
	//flip vertices
	if(orientation == 1)
	{
		vec3 v1_copy = v1;
		v1 = v2;
		v2 = v3;
		v3 = v0;
		v0 = v1_copy;

	}
	else if(orientation == 2)
	{
		/*
		   v0 becomes v2
		   v1 becomes v3
		   v2 becomes v0
		   v3 becomes v1
		*/
		vec3 v2_copy = v2;
		vec3 v1_copy = v1;
		v1 = v3;
		v3 = v1_copy;
		v2 = v0;
		v0 = v2_copy;

	}
    else if(orientation == 3)
	{
		vec3 v3_copy = v3;
		v3 = v2;
		v2 = v1;
		v1 = v0;
		v0 = v3_copy;

	}

	mesh_points points = {0};


	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
	return(points);
}

static void
render_update_camera_rotation(game_renderer *gameRenderer)
{
   real32 rx = gameRenderer->camera_rotation.x;
   real32 ry = gameRenderer->camera_rotation.y;
   real32 rz = gameRenderer->camera_rotation.z;

   matrix4x4 camera_rotation = matrix4x4_rotation_scale(rx, ry, rz);

   gameRenderer->camera_x = matrix4x4_v3_get_column(camera_rotation, 0); 
   gameRenderer->camera_y = matrix4x4_v3_get_column(camera_rotation, 1); 
   gameRenderer->camera_z = matrix4x4_v3_get_column(camera_rotation, 2); 

}

#define render_set_camera_transform_default \
	render_update_camera_values(gameRenderer, 16, 9)
static void 
render_update_camera_values(game_renderer *gameRenderer, u32 v_x, u32 v_y)
{

	//game_renderer *gameRenderer = render_parameters->game_renderer;
	//this face zoom will make objects look smaller because what this
	// value does is that it makes the position go "farther", and objects 
	//farther from the camera will looks smaller.
	//the projection gets scaled by this value to correctly position with this one.
	//this is only useful for cameras only pointing down, since when rotating
	//on x, the "trick" is revealed
	//When the whole projection only gets scaled, the camera would look like it's
	//zooming in.
	//when the camera_position only is scaled, it looks like it's zooming out.
	//So for the projection, objects outside its view would look "smaller", and
	//in order the preserve the camera position and only make farther objects
	//smaller, both get multiplied by the same scale.
	f32 z_unscale = 1;
   gameRenderer->camera_position = 
	   vec3_scale(gameRenderer->camera_position, z_unscale);
   vec3 camera_position = gameRenderer->camera_position;
   vec3 vx = gameRenderer->camera_x; 
   vec3 vy = gameRenderer->camera_y; 
   vec3 vz = gameRenderer->camera_z; 

   matrix4x4_data cameraMatrices = 
	   matrix4x4_camera_transform(
			   vx,
			   vy,
			   vz,
			   camera_position);
#if 1
//   matrix4x4_data projectionMatrices = matrix4x4_projection_perspective(v_x, v_y, 0.01f, 10000.0f);
   f32 fov = gameRenderer->fov;
   fov = fov < 1.0f ? 1.0f : fov;
   matrix4x4_data projectionMatrices = matrix4x4_projection_perspective_fov_r(
		   degrees_to_radians_f32(fov), (16.0f / 9.0f), 1.0f, 10000.0f);
#else
   //to zoom an orthographic camera, and of its parameters get multiplied by 
   //the inverse of the zoom.
   f32 zoom_inverse = 1.0f / gameRenderer->camera_zoom;
   f32 u = 1.0f;
   f32 d = -1.0f;
   f32 l = -2.0f;
   f32 r = 2.0f;
   matrix4x4_data projectionMatrices = 
	   matrix4x4_projection_orthographic_depth(
			   u * zoom_inverse,
			   d * zoom_inverse,
			   l * zoom_inverse,
			   r * zoom_inverse);

#endif
   gameRenderer->projections = projectionMatrices;
   gameRenderer->camera_transform    = cameraMatrices.foward;
   gameRenderer->camera_transform_inverse = cameraMatrices.inverse;
   //Combine the camera translation matrix with the selected projection.
   gameRenderer->projection = matrix4x4_mul(
		   projectionMatrices.foward,
		   cameraMatrices.foward);
   gameRenderer->projection_inverse = matrix4x4_mul(
		   cameraMatrices.inverse,
		   projectionMatrices.inverse);

   matrix4x4 scale = matrix4x4_scale(z_unscale);

   gameRenderer->projection = matrix4x4_mul(gameRenderer->projection, scale);
   gameRenderer->projection_inverse = matrix4x4_mul(gameRenderer->projection_inverse, scale);

   //2d camera
   {
	   f32 zoom_inverse = 1.0f / gameRenderer->camera_zoom_2d;
	   f32 u = 1.0f;
	   f32 d = -1.0f;
	   f32 l = -1.0f;
	   f32 r = 1.0f;
	   gameRenderer->projection_2d = matrix4x4_projection_orthographic(
				   u * zoom_inverse,
				   d * zoom_inverse,
				   l * zoom_inverse,
				   r * zoom_inverse).foward;


   }
}

//#define _bias 0.45f
//#define _bias 0
static inline void
add_y_bias_to_vertices(
		vec3 *v0,
		vec3 *v1,
		vec3 *v2,
		vec3 *v3,
		f32 _bias)
{
   //v0->y += v0->z * _bias;
   //v1->y += v1->z * _bias;
   //v2->y += v2->z * _bias;
   //v3->y += v3->z * _bias;

#if 1
   v0->y += v0->z * _bias;
   v1->y += v1->z * _bias;
   v2->y += v2->z * _bias;
   v3->y += v3->z * _bias;
#else
   f32 v = arctan32(_bias / 2);
   v0->y += v0->z * (v * 1.7f);
   v1->y += v1->z * (v * 1.7f);
   v2->y += v2->z * (v * 1.7f);
   v3->y += v3->z * (v * 1.7f);

   v0->z -= v0->z * v;
   v1->z -= v1->z * v;
   v2->z -= v2->z * v;
   v3->z -= v3->z * v;
#endif

}

static game_render_parameters
render_set_initial_parameters(game_renderer *game_renderer)
{
	game_render_parameters parameters = {0};
	parameters.game_renderer = game_renderer;
	parameters.fov = 25.0f;
//	parameters.fov = 30.0f;
	parameters.camera_rotation_x = 0.1f;
	parameters.camera_rotation_y = 0.0f;
	parameters.camera_rotation_z = 0.0f;
	parameters.distance_camera_target = 400.0f;
//	parameters.sprite_skew = 0.8f;
	parameters.z_fix = 1.0f;

	game_renderer->fov = parameters.fov;
//	game_renderer->sprite_skew = parameters.sprite_skew;
	game_renderer->sprite_skew = 0.5f;

	return(parameters);
}
