/*
Notes:
-- Locking vertices --
In order to "lock" a geometry, the renderer first placed said vertices on a group
that points to an array of "locked" vertices, and later puts them all at the start
of the main vertex buffer for every swap buffer. Instead of restarting the count to 0,
every frame will start with the total amount of locked vertices pushed from every group.

In order to restart ALL locked groups, you call render_refresh_locked_vertices(...).
Normally you put a boolean like "draws_are_locked" to avoid calling it on every frame.
Then to lock the next draw calls you call render_push_locked_vertices(...) and
render_pop_locked_vertices(...) between the draw calls.
ej:
if(!game.draw_locked)
{
    render_refresh_locked_vertices(commands);
}

render_push_locked_vertices(commands);
if(!game.draw_locked)
{
    ... draw calls.
}

//this could be on a separate function
if(!game.draw_locked)
{
    ... draw calls.
}
render_pop_locked_vertices(commands);
*/
#define PushCommand(commands, type, ptr) ptr = (type *)commands->commands_offset;\
                                               commands->commands_offset += sizeof(type);\
                                               commands->command_count++
#define PushCommand2(commands, typestruct, typeenum) (typeenum*)commands->commands_offset = typeenum;\
                                                      commands->commands_offset += sizeof(typeenum)\
                                                      commands->commands_offset += sizeof(typestruct);\
                                                      commands->command_count++

typedef struct{
	vec2 min;
	vec2 max;
}render_uv_minmax;

typedef struct{
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
}render_uvs;

typedef struct{
	u16 frame_x;
	u16 frame_y;
	u16 frame_w;
	u16 frame_h;
}render_uv_frames;

typedef struct{
	vec3 v0;
	vec3 v1;
	vec3 v2;
	vec3 v3;
}vertices3;

typedef struct{
	vec2 v0;
	vec2 v1;
	vec2 v2;
	vec2 v3;
}vertices2;


static void
render_push_quad(render_commands *commands, render_texture *texture, vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, vec4 color);
static void
render_push_vertices(render_commands *commands, render_texture *texture, vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, vec4 color0, vec4 color1, vec4 color2, vec4 color3);
static void
render_push_quad_2d(render_commands *commands, render_texture *texture, vec2 v0_2d, vec2 v1_2d, vec2 v2_2d, vec2 v3_2d, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, vec4 color);
static void
render_push_vertices_2d(render_commands*, render_texture*, vec2 v0_2d, vec2 v1_2d, vec2 v2_2d, vec2 , vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, vec4 color0, vec4 color1, vec4 color2, vec4 color3);
static inline render_uvs
render_flip_uvs_horizontally(vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3);
static inline void
render_fill_uvs_counter_cw(vec2 *uv0_ptr, vec2 *uv1_ptr, vec2 *uv2_ptr, vec2 *uv3_ptr);
static inline render_uvs
render_frames_to_uv(u32 texture_width, u32 texture_height, u32 frame_x, u32 frame_y, u32 frame_w, u32 frame_h);
static inline vertices3 
adjust_vertices_to_uvs_3d(vec3 v0, vec3 v1, vec3 v2, vec3 v3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3);
static inline vertices2
adjust_vertices_to_uvs_2d(vec2 v0, vec2 v1, vec2 v2, vec2 v3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3);
inline vec3
vertices_get_mid_point_3d(vec3 v0, vec3 v1, vec3 v2, vec3 v3);
inline vec2
vertices_get_mid_point_2d(vec2 v0, vec2 v1, vec2 v2, vec2 v3);

inline vec2
vertices_get_mid_point_2d(vec2 v0, vec2 v1, vec2 v2, vec2 v3)
{

    vec2 distance_v2_v0 = {
		v2.x - v0.x,
		v2.y - v0.y};

    
    vec2 midPoint = {  
	                    v0.x + 0.5f * (distance_v2_v0.x),
	                    v0.y + 0.5f * (distance_v2_v0.y),
	};
	return(midPoint);

}

inline vec3
vertices_get_mid_point_3d(vec3 v0, vec3 v1, vec3 v2, vec3 v3)
{

    vec3 distance_v2_v0 = {
		v2.x - v0.x,
		v2.y - v0.y,
		v2.z - v0.z};

    
    vec3 midPoint = {  
	                    v0.x + 0.5f * (distance_v2_v0.x),
	                    v0.y + 0.5f * (distance_v2_v0.y),
	                    v0.z + 0.5f * (distance_v2_v0.z),
	};
	return(midPoint);
}

static inline vertices2
adjust_vertices_to_uvs_2d(
		vec2 v0,
		vec2 v1,
		vec2 v2,
		vec2 v3,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{

		vertices2 result = {0};

		vec2 mp = vertices_get_mid_point_2d(v0, v1, v2, v3);

		v0 = mp;
		v0.x = mp.x - ABS(uv3.x - uv0.x) * 512 * 0.5f;
		v0.y = mp.y + ABS(uv0.y - uv1.y) * 512 * 0.5f;

		v1 = mp;
		v1.x = mp.x - ABS(uv2.x - uv1.x) * 512 * 0.5f;
		v1.y = mp.y - ABS(uv0.y - uv1.y) * 512 * 0.5f;

		v2 = mp;
		v2.x = mp.x + ABS(uv2.x - uv1.x) * 512 * 0.5f;
		v2.y = mp.y - ABS(uv3.y - uv2.y) * 512 * 0.5f;

		v3 = mp;
		v3.x = mp.x + ABS(uv3.x - uv0.x) * 512 * 0.5f;
		v3.y = mp.y + ABS(uv3.y - uv2.y) * 512 * 0.5f;

		result.v0 = v0;
		result.v1 = v1;
		result.v2 = v2;
		result.v3 = v3;

		return(result);
}

static inline vertices3 
adjust_vertices_to_uvs_3d(
		vec3 v0,
		vec3 v1,
		vec3 v2,
		vec3 v3,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{

		vertices3 result = {0};

		vec3 mp = vertices_get_mid_point(v0, v1, v2, v3);

		v0 = mp;
		v0.x = mp.x - ABS(uv3.x - uv0.x) * 512 * 0.5f;
		v0.z = mp.z - ABS(uv0.y - uv1.y) * 512 * 0.5f;

		v1 = mp;
		v1.x = mp.x - ABS(uv2.x - uv1.x) * 512 * 0.5f;
		v1.z = mp.z + ABS(uv0.y - uv1.y) * 512 * 0.5f;

		v2 = mp;
		v2.x = mp.x + ABS(uv2.x - uv1.x) * 512 * 0.5f;
		v2.z = mp.z + ABS(uv3.y - uv2.y) * 512 * 0.5f;

		v3 = mp;
		v3.x = mp.x + ABS(uv3.x - uv0.x) * 512 * 0.5f;
		v3.z = mp.z - ABS(uv3.y - uv2.y) * 512 * 0.5f;

		result.v0 = v0;
		result.v1 = v1;
		result.v2 = v2;
		result.v3 = v3;

		return(result);
}

inline int32
render_point_inside_area(vec2 mouseP,real32 x0,real32 y0,real32 x1,real32 y1)
{
    int32 result = (mouseP.x <= x1 && mouseP.x >= x0) &&
                   (mouseP.y <= y1 && mouseP.y >= y0);
    return(result);
}

static inline void
render_fill_uvs_counter_cw(
		vec2 *uv0_ptr,
		vec2 *uv1_ptr,
		vec2 *uv2_ptr,
		vec2 *uv3_ptr)
{
	vec2 uv0 = {0.0f, 1.0f};
	vec2 uv1 = {0.0f, 0.0f};
	vec2 uv2 = {1.0f, 0.0f};
	vec2 uv3 = {1.0f, 1.0f};
	*uv0_ptr = uv0;
	*uv1_ptr = uv1;
	*uv2_ptr = uv2;
	*uv3_ptr = uv3;
}

static inline render_uvs
render_flip_uvs_horizontally(vec2 uv0,
		                     vec2 uv1,
							 vec2 uv2,
							 vec2 uv3)
{
	vec2 uv0_copy = uv0;
	vec2 uv1_copy = uv1;
	uv0 = uv3; 
	uv1 = uv2; 
	uv2 = uv1_copy;
	uv3 = uv0_copy;

	render_uvs result;
	result.uv0 = uv0;
	result.uv1 = uv1;
	result.uv2 = uv2;
	result.uv3 = uv3;

	return(result);
}

static inline void 
render_flip_and_fill_uvs_horizontally(vec2 *uv0,
		                     vec2 *uv1,
							 vec2 *uv2,
							 vec2 *uv3)
{
	vec2 uv0_copy = *uv0;
	vec2 uv1_copy = *uv1;
	*uv0 = *uv3; 
	*uv1 = *uv2; 
	*uv2 = uv1_copy;
	*uv3 = uv0_copy;
}

static inline render_uv_minmax  
render_frames_to_uv_min_max(
		u32 texture_width,
		u32 texture_height,
		u32 frame_x,
		u32 frame_y,
		u32 frame_w,
		u32 frame_h)
{
   u32 textureW = texture_width;
   u32 textureH = texture_height;
   f32 OneOverW = 1.0f / texture_width;
   f32 OneOverH = 1.0f / texture_height;
   render_uv_minmax result = {0};

   result.min.x = (f32)(frame_x)  * OneOverW;
   result.min.y = (f32)(frame_y)  * OneOverH;
   result.max.x = result.min.x + ((f32)(frame_w) * OneOverW);
   result.max.y = result.min.y + ((f32)(frame_h) * OneOverH);

   return(result);
}

static inline render_uvs
render_frames_to_uv(
		u32 texture_width,
		u32 texture_height,
		u32 frame_x,
		u32 frame_y,
		u32 frame_w,
		u32 frame_h)
{
	render_uvs uvs = {0};

   u32 textureW = texture_width;
   u32 textureH = texture_height;
   f32 OneOverW = 1.0f / texture_width;
   f32 OneOverH = 1.0f / texture_height;

   uvs.uv1.x = (f32)(frame_x) * OneOverW;
   uvs.uv1.y = (f32)(frame_y) * OneOverH;
   uvs.uv3.x = uvs.uv1.x + ((f32)(frame_w) * OneOverW);
   uvs.uv3.y = uvs.uv1.y + ((f32)(frame_h) * OneOverH);

   uvs.uv0.x = uvs.uv1.x;
   uvs.uv0.y = uvs.uv3.y;

   uvs.uv2.x = uvs.uv3.x;
   uvs.uv2.y = uvs.uv1.y;

   return(uvs);

}

static inline void
render_fill_uvs_from_frames(
		u32 texture_width,
		u32 texture_height,
		u32 frame_x,
		u32 frame_y,
		u32 frame_w,
		u32 frame_h,
		vec2 *uv0_ptr,
		vec2 *uv1_ptr,
		vec2 *uv2_ptr,
		vec2 *uv3_ptr
		)
{
   u32 textureW = texture_width;
   u32 textureH = texture_height;
   f32 OneOverW = 1.0f / texture_width;
   f32 OneOverH = 1.0f / texture_height;

   vec2 uv0;
   vec2 uv1;
   vec2 uv2;
   vec2 uv3;

   uv1.x = (f32)(frame_x) * OneOverW;
   uv1.y = (f32)(frame_y) * OneOverH;
   uv3.x = uv1.x + ((f32)(frame_w) * OneOverW);
   uv3.y = uv1.y + ((f32)(frame_h) * OneOverH);

   uv0.x = uv1.x;
   uv0.y = uv3.y;

   uv2.x = uv3.x;
   uv2.y = uv1.y;

   if(uv0_ptr){
	   *uv0_ptr = uv0;
   }
   if(uv1_ptr){
	   *uv1_ptr = uv1;
   }
   if(uv2_ptr){
	   *uv2_ptr = uv2;
   }
   if(uv3_ptr){
	   *uv3_ptr = uv3;
   }

}

static inline render_uv_frames
render_uv_min_max_to_frames(
	    u32 texture_width,
		u32 texture_height,
		vec2 uv_min,
		vec2 uv_max)
{
   u32 texture_w = texture_width;
   u32 texture_h = texture_height;
   render_uv_frames result = {0};

   vec2 distance_uv_max_min = vec2_sub(uv_max, uv_min);

   result.frame_x = (u16)(uv_min.x * texture_w);
   result.frame_y = (u16)(uv_min.y * texture_h);
   result.frame_w = (u16)(distance_uv_max_min.x * texture_w);
   result.frame_h = (u16)(distance_uv_max_min.y * texture_h);

   return(result);
}

static inline render_uv_frames
render_uv_to_frames(
		u32 texture_width,
		u32 texture_height,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{
   u32 texture_w = texture_width; 
   u32 texture_h = texture_height;
   render_uv_frames result = {0};


   vec2 uv_min = {0};
   vec2 uv_max = {0};

   uv_min.x = MIN(uv0.x, uv1.x);
   uv_min.x = MIN(uv_min.x, MIN(uv2.x, uv3.x));
   uv_min.y = MIN(uv0.y, uv1.y);
   uv_min.y = MIN(uv_min.y, MIN(uv2.y, uv3.y));

   uv_max.x = MAX(uv0.x, uv1.x);
   uv_max.x = MAX(uv_max.x, MAX(uv2.x, uv3.x));
   uv_max.y = MAX(uv0.y, uv1.y);
   uv_max.y = MAX(uv_max.y, MAX(uv2.y, uv3.y));

   vec2 distance_uv_max_min = vec2_sub(uv_max, uv_min);

   result.frame_x = (u16)(uv_min.x * texture_w);
   result.frame_y = (u16)(uv_min.y * texture_h);
   result.frame_w = (u16)(distance_uv_max_min.x * texture_w);
   result.frame_h = (u16)(distance_uv_max_min.y * texture_h);

   return(result);
}

static inline void
render_fill_frames_from_uvs(
		u32 texture_w,
		u32 texture_h,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3,
		u32 *fx,
		u32 *fy,
		u32 *fw,
		u32 *fh
		)
{
	render_uv_frames uv_frames = render_uv_to_frames(texture_w, texture_h, uv0, uv1, uv2, uv3);
	*fx = uv_frames.frame_x;
	*fy = uv_frames.frame_y;
	*fw = uv_frames.frame_w;
	*fh = uv_frames.frame_h;
}

static inline rectangle32s 
render_get_current_clip(render_commands *commands)
{
	rectangle32s clipRec = commands->gameRenderer->current_draw_clip;
	if(commands->gameRenderer->scissors_on_stack > 0)
	{
	    i32 clipI = commands->gameRenderer->scissor_stack_index;
		clipRec = commands->gameRenderer->scissor_stack[clipI].clip;
	}
	return(clipRec);
}

static inline rectangle32s 
render_get_current_clip_to_game_coordinates(render_commands *commands)
{
	//Get renderer
	game_renderer *gameRenderer = commands->gameRenderer;
    //Calculated game clip
	rectangle32s *gameClip = &gameRenderer->current_draw_clip;

	real32 clipScaleX = (real32)gameRenderer->back_buffer_width  / (real32)(gameClip->x1 - gameClip->x0);
	real32 clipScaleY = (real32)gameRenderer->back_buffer_height / (real32)(gameClip->y1 - gameClip->y0); 


	int32 clipI = gameRenderer->scissor_stack_index;
	rectangle32s clipRec = render_get_current_clip(commands);
    // convert screen coordinates to game coordinates
    clipRec.x0 = (i32)((clipRec.x0 - gameClip->x0) * clipScaleX);
    clipRec.y0 = (i32)((clipRec.y0 - gameClip->y0) * clipScaleY);
    clipRec.x1 = (i32)((clipRec.x1 - gameClip->x0) * clipScaleX);
    clipRec.y1 = (i32)((clipRec.y1 - gameClip->y0) * clipScaleY);

	return(clipRec);
}

static inline int32 
render_rectangle_is_inside_clip(
		render_commands *commands,
		real32 x0,
		real32 y0,
		real32 x1,
		real32 y1)
{
   rectangle32s current_clip = render_get_current_clip_to_game_coordinates(commands);

   int32 result = x0 < current_clip.w && y0 < current_clip.h &&
	              x1 > current_clip.x && y1 > current_clip.y;
   return(result);
}

#define render_MouseOverRecClipped_GameCoordsXYWH(commands, mousePoint, x, y, w, h) render_MouseOverRecClipped_GameCoords(commands, mousePoint, x, y, x + w, y + h)
#define render_MouseOverRecClippedXYWH(commands, mousePoint, x, y, w, h) render_mouse_over_rec_clipped(commands, mousePoint, x, y, x + w, y + h)
static inline int32 
_render_mouse_over_rec_clipped(render_commands *commands,
		                    rectangle32s current_clip,
							vec2 mousePoint,
							real32 x0,
							real32 y0,
							real32 x1, real32 y1)
{

	game_renderer *gameRenderer = commands->gameRenderer;

	//Clamp values
	x0 = x0 < current_clip.x ? current_clip.x : x0;
	y0 = y0 < current_clip.y ? current_clip.y : y0;
	x1 = x1 > current_clip.w ? current_clip.w : x1;
	y1 = y1 > current_clip.h ? current_clip.h : y1;
    u32 result = render_point_inside_area(mousePoint, x0, y0, x1, y1); 
	if(result)
	{
		uint32 s = 0;
	}

    return(result);
}

static inline u32
render_point_inside_clip(render_commands *commands,
		                 vec2 point)
{
	rectangle32s current_clip = render_get_current_clip(commands);
	f32 x0 = (f32)current_clip.x;
	f32 y0 = (f32)current_clip.y;
	f32 x1 = (f32)current_clip.w;
	f32 y1 = (f32)current_clip.h;

    u32 result = render_point_inside_area(point, x0, y0, x1, y1); 

	return(result);
}

static inline int32 
render_MouseOverRecClipped_GameCoords(render_commands *commands, vec2 mousePoint, real32 x0, real32 y0, real32 x1, real32 y1)
{

	rectangle32s current_clip = render_get_current_clip_to_game_coordinates(commands);
    i32 result = _render_mouse_over_rec_clipped(commands, current_clip, mousePoint, x0, y0, x1, y1);

    return(result);
}

static inline int32 
render_mouse_over_rec_clipped(render_commands *commands, vec2 mousePoint, real32 x0, real32 y0, real32 x1, real32 y1)
{

	rectangle32s current_clip = render_get_current_clip(commands);
    i32 result = _render_mouse_over_rec_clipped(commands, current_clip, mousePoint, x0, y0, x1, y1);

    return(result);
}

static inline void
render_commands_SetClipAndViewport(render_commands *renderCommands, i32 x0, i32 y0, i32 x1, i32 y1)
{

   renderCommands->set_viewport_and_clip = 1;
   renderCommands->viewport_and_clip.x0 = x0;
   renderCommands->viewport_and_clip.y0 = y0;
   renderCommands->viewport_and_clip.x1 = x1;
   renderCommands->viewport_and_clip.y1 = y1;

   renderCommands->gameRenderer->current_draw_clip = renderCommands->viewport_and_clip;
}

static inline uint32
render_pack_color_rgba(vec4 color)
{
    uint32 result = (((uint32)color.x << 0) |
                     ((uint32)color.y << 8) |
                     ((uint32)color.z << 16) |
                     ((uint32)color.w << 24));
    return(result);
}

#define render_commands_PushCommand(renderCommands, type, command) (command *)_render_commands_PushCommand(renderCommands, type, sizeof(command))

static void *
_render_commands_PushCommand(render_commands *renderCommands,
		                     render_command_type type,
							 u32 size)
{
	Assert(renderCommands->commands_offset + size < renderCommands->commands_base + renderCommands->gameRenderer->render_commands_buffer_size);

    void *command = renderCommands->commands_offset;
    renderCommands->commands_offset += size; 

    renderCommands->command_count++;
    *(render_command_type *)command = type;

	return(command);
}

//Note(Agu): this is used to draw the debug rectangle and see the clip.
static void
render_rectangle_2d_xywh(render_commands *c, real32 x, real32 y, real32 w, real32 h, vec4 color);
#define _render_draw_rectangle_2D(...) render_rectangle_2d_xywh(__VA_ARGS__)


#define render_commands_PushClip_XYWH(commands, x, y, w, h) render_commands_PushClip(commands, x, y, x + w, y + h)
#define render_commands_PushClip_GameCoords_XYWH(commands, x, y, w, h) render_commands_PushClip_gameCoords(commands, x, y, x + w, y + h)
static void
render_commands_PushClip(render_commands *commands,
						 int32 x0,
						 int32 y0,
						 int32 x1,
						 int32 y1)
{

	game_renderer *gameRenderer = commands->gameRenderer;
	Assert(x0 <= x1 && y0 <= y1);
	render_command_SetClip *setClip = render_commands_PushCommand(commands ,render_command_type_PushClip, render_command_SetClip);

	//setClip->x0 = x0; //+ commands->gameRenderer->DrawClip.x;
	//setClip->y0 = y0; //- commands->gameRenderer->DrawClip.y;
	//setClip->x1 = x1; // + commands->gameRenderer->DrawClip.x;
	//setClip->y1 = y1; // + commands->gameRenderer->DrawClip.y;


	u32 sI = gameRenderer->scissor_push_count;

	render_scissor *scissor_stack = gameRenderer->scissor_stack;
	scissor_stack[sI].clip.x = x0;
	scissor_stack[sI].clip.y = y0;
	scissor_stack[sI].clip.w = x1;
	scissor_stack[sI].clip.h = y1;
	if(gameRenderer->scissors_on_stack)
	{
		u32 previousIndex = gameRenderer->scissor_stack_index;
		scissor_stack[sI].previous = previousIndex;
	}
	gameRenderer->scissor_stack_index = gameRenderer->scissor_push_count;

	gameRenderer->scissor_push_count++;
	gameRenderer->scissors_on_stack++;

	Assert(gameRenderer->scissor_push_count <= gameRenderer->scissor_total_count);

}

//Translates clipped coordinates to screen
static inline void
render_commands_PushClip_GameCoords(render_commands *commands, int32 x0, int32 y0, int32 x1,int32 y1)
{
	game_renderer *gameRenderer = commands->gameRenderer;
	rectangle32s *gameClip = &gameRenderer->current_draw_clip;
	f32 clipScaleX = (real32)(gameClip->x1 - gameClip->x0) / (real32)gameRenderer->back_buffer_width;
	f32 clipScaleY = (real32)(gameClip->y1 - gameClip->y0) / (real32)gameRenderer->back_buffer_height;
	//Add the clip borders to get the correct coordinates
	x0 = (int32)(x0 * clipScaleX + gameClip->x0); 
	y0 = (int32)(y0 * clipScaleY + gameClip->y0); 
	x1 = (int32)(x1 * clipScaleX + gameClip->x0);
	y1 = (int32)(y1 * clipScaleY + gameClip->y0);

#if 0
	//
	//this is what direct3d sees as the clip rectangle, the scale is for translating the coordinates to the game.
	//
	vec4 dbgRecColor = {0xff, 0xff, 0, 168};
	vec4 clip = vec4_from_rec(gameRenderer->DrawClip); 
	real32 rX = (x - clip.x) / clipScaleX;
	real32 rY = (y - clip.y) / clipScaleY;
	real32 rW = w / clipScaleX;
	real32 rH = h / clipScaleY;
	_render_draw_rectangle_2D(commands, rX, rY, rW, rH, dbgRecColor );
	_render_draw_rectangle_2D(commands, rX, rY, 8, 8, V4(255, 0, 0, 255));
#endif
    render_commands_PushClip(commands, x0, y0, x1, y1);
}

static void
render_commands_PopClip(render_commands *commands)
{
	game_renderer *gameRenderer = commands->gameRenderer;
	Assert(gameRenderer->scissors_on_stack);

	//set current scissor to the one "pointed by the popped.
	render_scissor *scissorPopped = gameRenderer->scissor_stack + gameRenderer->scissor_stack_index;
	gameRenderer->scissor_stack_index = scissorPopped->previous;

	gameRenderer->scissors_on_stack--;

	render_command_PopClip *setClip = render_commands_PushCommand(commands ,render_command_type_PopClip, render_command_PopClip);
}

static int32
render_commands_PushClipIfInsideLast(render_commands *commands, int32 x0, int32 y0, int32 x1, int32 y1)
{

	//Adjust clip coordinates.
	int32 displays = render_rectangle_is_inside_clip(commands, (f32)x0, (f32)y0, (f32)x1, (f32)y1);
	rectangle32s currentScissor = render_get_current_clip_to_game_coordinates(commands);

	if(displays)
	{
	   if(x0 < currentScissor.x0)
	   {
	   	 x0 = currentScissor.x0;
	   }
       if(y0 < currentScissor.y0)
	   {
	   	 y0 = currentScissor.y0;
	   }

	   if(x1 > currentScissor.x1)
	   {
	   	 x1 = currentScissor.x1;
	   }
       if(y1 > currentScissor.y1)
	   {
	   	 y1 = currentScissor.y1;
	   }
	   render_commands_PushClip(commands, x0, y0, x1, y1);
	}

	return(displays);

}

#define render_commands_PushClipInsideLast_XYWH(commands, x, y, w ,h) render_commands_push_clip_inside_last(commands, x, y, (x + w), (y + h))

static void 
_render_commands_push_clip_inside_last(render_commands *commands, rectangle32s currentScissor, int32 x0, int32 y0, int32 x1, int32 y1)
{

	//Adjust clip coordinates.
	x0 = x0 < currentScissor.x0 ? currentScissor.x0 : x0 > currentScissor.x1 ? currentScissor.x1 : x0;
	y0 = y0 < currentScissor.y0 ? currentScissor.y0 : y0 > currentScissor.y1 ? currentScissor.y1 : y0;
	x1 = x1 > currentScissor.x1 ? currentScissor.x1 : x1 < x0 ? x0 : x1;
	y1 = y1 > currentScissor.y1 ? currentScissor.y1 : y1 < y0 ? y0 : y1;
	render_commands_PushClip(commands, x0, y0, x1, y1);
}
static inline void
render_commands_push_clip_inside_last(render_commands *commands, int32 x0, int32 y0, int32 x1, int32 y1)
{
	_render_commands_push_clip_inside_last(commands, render_get_current_clip(commands), x0, y0, x1, y1);
}
static inline void
render_commands_push_clip_inside_last_in_game_coords(render_commands *commands, int32 x0, int32 y0, int32 x1, int32 y1)
{
	_render_commands_push_clip_inside_last(commands, render_get_current_clip_to_game_coordinates(commands), x0, y0, x1, y1);
}

static void 
render_commands_clear_graphics(render_commands *commands)
{
    render_command_clear *clearCommand = render_commands_PushCommand(commands, render_command_type_clear, render_command_clear);
}


static inline vec2
render_scale_to_display(render_commands *commands, vec2 p)
{
	game_renderer *gameRenderer = commands->gameRenderer;
   p.x = (p.x / gameRenderer->back_buffer_width)  * gameRenderer->display_width;
   p.y = (p.y / gameRenderer->back_buffer_height) * gameRenderer->display_height;

   return(p);
}

static inline void 
render_commands_set_identity(render_commands *commands)
{
    game_renderer *gameRenderer = commands->gameRenderer;
   //matrix4x4 WVP = matrix4x4_Identity(); 
}

static inline matrix4x4
render_commands_set_identity_scale(render_commands *commands, f32 s)
{
    game_renderer *gameRenderer = commands->gameRenderer;
   matrix4x4 WVP = matrix4x4_Identity();
   WVP.m[2][2] = 1;
   WVP.m[3][3] = s;

   return(WVP);
}

static inline matrix4x4
render_set_orthographic(
		game_renderer *gameRenderer,
		f32 top,
		f32 bot,
		f32 left,
		f32 right)
{
   //matrix4x4 WVP = matrix4x4_ProjectionOrthographic(w, h);
   matrix4x4 WVP = matrix4x4_projection_orthographic(1.0f, -1.0f, -1.0f, 1.0f).foward;
   return(WVP);
}

static inline void
render_push_to_locked_vertices(game_renderer *gameRenderer, render_vertex *vertices)
{
	Assert(gameRenderer->update_locked_vertices)

	u32 count = gameRenderer->locked_quads_count * 4;

	gameRenderer->locked_vertices[count + 0] = vertices[0];
	gameRenderer->locked_vertices[count + 1] = vertices[1];
	gameRenderer->locked_vertices[count + 2] = vertices[2];
	gameRenderer->locked_vertices[count + 3] = vertices[3];

	gameRenderer->locked_quads_count += 1;

	Assert(gameRenderer->locked_quads_count < gameRenderer->locked_quads_max);

	render_locked_vertices_group *currentGroup = gameRenderer->locked_vertices_groups + gameRenderer->current_locked_vertices_pushed;
	currentGroup->count++;
	
	
}

static void
render_push_billboard(render_commands *commands,
                      render_texture *texture,
                      vec3 v0,
                      vec3 v1,
                      vec3 v2,
                      vec3 v3,
                      vec2 uv0,
                      vec2 uv1,
                      vec2 uv2,
                      vec2 uv3,
                      vec4 color)
{

    game_renderer *gameRenderer = commands->gameRenderer;
#if 0

   vec4 v0c = V4(v0.x, v0.y, v0.z, 1);
   vec4 v1c = V4(v1.x, v1.y, v1.z, 1);
   vec4 v2c = V4(v2.x, v2.y, v2.z, 1);
   vec4 v3c = V4(v3.x, v3.y, v3.z, 1);

   matrix4x4 WVP = matrix4x4_mul(gameRenderer->projection, gameRenderer->camera_transform);
   matrix4x4 WVP2 = gameRenderer->projection;
   v1c = matrix4x4_v4_mul_rows(WVP2,v1c);
   v2c = matrix4x4_v4_mul_rows(WVP2,v2c);
   v0c = matrix4x4_v4_mul_rows(WVP2,v0c);
   v3c = matrix4x4_v4_mul_rows(WVP2,v3c);

   //v1c = vec4Divide(v1c, v1c.w);
   //v2c = vec4Divide(v2c, v2c.w);
   //v0c = vec4Divide(v0c, v0c.w);
   //v3c = vec4Divide(v3c, v3c.w);

   uint32 clipu = 1;
   uint32 clipd = -1;
   uint32 clipl = -1;
   uint32 clipr = 1;

#endif


   uint32 color32 = render_pack_color_rgba(color);
   uint32 textureId = texture->index;
   //i32 index_add = (texture->height / gameRenderer->texture_array_h) - 1;
   //if(index_add > 0)
   //{
   //    textureId += index_add;
   //}

   //render_vertex v0 = {v0.x, v0.y, v0.z, uvMin.x , uvMax.y , Color32, textureId};
   //render_vertex v1 = {v1.x, v1.y, v1.z, uvMin.x , uvMin.y , Color32, textureId};
   //render_vertex v2 = {v2.x, v2.y, v2.z, uvMax.x , uvMin.y , Color32, textureId};
   //render_vertex v3 = {v3.x, v3.y, v3.z, uvMax.x , uvMax.y , Color32, textureId};

   //goes in counter-clockwise order starting from the "bottom left" corner
   render_vertex v0_clipped = {0};
   render_vertex v1_clipped = {0};
   render_vertex v2_clipped = {0};
   render_vertex v3_clipped = {0};


   v0_clipped.location = v0;
   v0_clipped.uv       = uv0;
   v0_clipped.color    = color32;
   v0_clipped.texture  = textureId;

   v1_clipped.location = v1;
   v1_clipped.uv       = uv1;
   v1_clipped.color    = color32;
   v1_clipped.texture  = textureId;

   v2_clipped.location = v2;
   v2_clipped.uv       = uv2;
   v2_clipped.color    = color32;
   v2_clipped.texture  = textureId;

   v3_clipped.location = v3;
   v3_clipped.uv       = uv3;
   v3_clipped.color    = color32;
   v3_clipped.texture  = textureId;

   if(!gameRenderer->lock_next_vertices)
   {

       //render_command_drawquad *drawquad = (render_command_drawquad *)commands->commands_offset;
       render_command_drawquad *drawquad = render_commands_PushCommand(commands,
		                                                           render_command_type_drawquad,
																   render_command_drawquad);
       drawquad->vertices[0] = v0_clipped;
       drawquad->vertices[1] = v1_clipped;
       drawquad->vertices[2] = v2_clipped;
       drawquad->vertices[3] = v3_clipped;
   }
   else
   {
	   //render_vertex locked_vertices[4];

       //locked_vertices[0] = v0_clipped;
       //locked_vertices[1] = v1_clipped;
       //locked_vertices[2] = v2_clipped;
       //locked_vertices[3] = v3_clipped;

	    Assert(gameRenderer->update_locked_vertices)

	    u32 count = gameRenderer->locked_quads_count * 4; // * 4 ?

	    gameRenderer->locked_vertices[count + 0] = v0_clipped;
	    gameRenderer->locked_vertices[count + 1] = v1_clipped;
	    gameRenderer->locked_vertices[count + 2] = v2_clipped;
	    gameRenderer->locked_vertices[count + 3] = v3_clipped;

	    gameRenderer->locked_quads_count += 1;

	    Assert(gameRenderer->locked_quads_count < gameRenderer->locked_quads_max);

	    render_locked_vertices_group *currentGroup = gameRenderer->locked_vertices_groups + gameRenderer->current_locked_vertices_pushed;
	    currentGroup->count++;
   }


   /*
   commands->command_count++;
   (*(game_renderer *)(commands->gameRenderer)).draw_count++;

   commands->commands_offset += sizeof(render_command_drawquad);
   */
}

#define render_push_sprite(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3) \
	render_push_quad(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, V4(255, 255, 255, 255))

static void
render_push_vertices(render_commands *commands,
                      render_texture *texture,
                      vec3 v0,
                      vec3 v1,
                      vec3 v2,
                      vec3 v3,
                      vec2 uv0,
                      vec2 uv1,
                      vec2 uv2,
                      vec2 uv3,
                      vec4 color0,
                      vec4 color1,
                      vec4 color2,
                      vec4 color3
					  )
{

    game_renderer *gameRenderer = commands->gameRenderer;
   //Arbitrary uv offset
   //uv offset based on distance

   uint32 color32_0 = render_pack_color_rgba(color0);
   uint32 color32_1 = render_pack_color_rgba(color1);
   uint32 color32_2 = render_pack_color_rgba(color2);
   uint32 color32_3 = render_pack_color_rgba(color3);
   uint32 textureId = texture->index;
   //i32 index_add = (texture->height / gameRenderer->texture_array_h) - 1;
   //if(index_add > 0)
   //{
   //    textureId += index_add;
   //}

   //render_vertex v0 = {v0.x, v0.y, v0.z, uvMin.x , uvMax.y , Color32, textureId};
   //render_vertex v1 = {v1.x, v1.y, v1.z, uvMin.x , uvMin.y , Color32, textureId};
   //render_vertex v2 = {v2.x, v2.y, v2.z, uvMax.x , uvMin.y , Color32, textureId};
   //render_vertex v3 = {v3.x, v3.y, v3.z, uvMax.x , uvMax.y , Color32, textureId};

   //goes in counter-clockwise order starting from the "bottom left" corner
   render_vertex v0_clipped = {0};
   render_vertex v1_clipped = {0};
   render_vertex v2_clipped = {0};
   render_vertex v3_clipped = {0};


   v0_clipped.location = v0;
   v0_clipped.uv       = uv0;
   v0_clipped.color    = color32_0;
   v0_clipped.texture  = textureId;

   v1_clipped.location = v1;
   v1_clipped.uv       = uv1;
   v1_clipped.color    = color32_1;
   v1_clipped.texture  = textureId;

   v2_clipped.location = v2;
   v2_clipped.uv       = uv2;
   v2_clipped.color    = color32_2;
   v2_clipped.texture  = textureId;

   v3_clipped.location = v3;
   v3_clipped.uv       = uv3;
   v3_clipped.color    = color32_3;
   v3_clipped.texture  = textureId;

   if(!gameRenderer->lock_next_vertices)
   {

       //render_command_drawquad *drawquad = (render_command_drawquad *)commands->commands_offset;
       render_command_drawquad *drawquad = render_commands_PushCommand(commands,
		                                                           render_command_type_drawquad,
																   render_command_drawquad);
       drawquad->vertices[0] = v0_clipped;
       drawquad->vertices[1] = v1_clipped;
       drawquad->vertices[2] = v2_clipped;
       drawquad->vertices[3] = v3_clipped;
   }
   else
   {

	    Assert(gameRenderer->update_locked_vertices)

	    u32 count = gameRenderer->locked_quads_count * 4; // * 4 ?

	    gameRenderer->locked_vertices[count + 0] = v0_clipped;
	    gameRenderer->locked_vertices[count + 1] = v1_clipped;
	    gameRenderer->locked_vertices[count + 2] = v2_clipped;
	    gameRenderer->locked_vertices[count + 3] = v3_clipped;

	    gameRenderer->locked_quads_count += 1;

	    Assert(gameRenderer->locked_quads_count < gameRenderer->locked_quads_max);

	    render_locked_vertices_group *currentGroup = gameRenderer->locked_vertices_groups + gameRenderer->current_locked_vertices_pushed;
	    currentGroup->count++;
   }

}
static void
render_push_quad(render_commands *commands,
                      render_texture *texture,
                      vec3 v0,
                      vec3 v1,
                      vec3 v2,
                      vec3 v3,
                      vec2 uv0,
                      vec2 uv1,
                      vec2 uv2,
                      vec2 uv3,
                      vec4 color)
{

    game_renderer *gameRenderer = commands->gameRenderer;
#if 0

   vec4 v0 = V4(BL3.x, BL3.y, BL3.z, 1);
   vec4 v1 = V4(TL3.x, TL3.y, TL3.z, 1);
   vec4 v2 = V4(TR3.x, TR3.y, TR3.z, 1);
   vec4 v3 = V4(BR3.x, BR3.y, BR3.z, 1);

   matrix4x4 WVP = Mult_M(gameRenderer->CameraProjection, gameRenderer->CameraTransform );
   v1 = Mult_M_V4(WVP,v1);
   v2 = Mult_M_V4(WVP,v2);
   v0 = Mult_M_V4(WVP,v0);
   v3 = Mult_M_V4(WVP,v3);

   v1 = vec4Divide(v1, v1.w);
   v2 = vec4Divide(v2, v2.w);
   v0 = vec4Divide(v0, v0.w);
   v3 = vec4Divide(v3, v3.w);

   uint32 clipu = 1;
   uint32 clipd = -1;
   uint32 clipl = -1;
   uint32 clipr = 1;

 #if 0
   static char buffer[100];
   format_text(buffer, 100, "v1 %f: y: %f :z %f en %d \n \0", v1.x, v1.y, v1.z, frame_y);
   OutputDebugString(buffer);

 #endif
 #if 0
   v1 = Mult_M_V4(gameRenderer->CameraTransform,v1);
   v2 = Mult_M_V4(gameRenderer->CameraTransform,v2);
   v0 = Mult_M_V4(gameRenderer->CameraTransform,v0);
   v3 = Mult_M_V4(gameRenderer->CameraTransform,v3);

   v1 = Mult_M_V4(gameRenderer->CameraProjection,v1);
   v2 = Mult_M_V4(gameRenderer->CameraProjection,v2);
   v0 = Mult_M_V4(gameRenderer->CameraProjection,v0);
   v3 = Mult_M_V4(gameRenderer->CameraProjection,v3);
 #endif
#endif
   //Arbitrary uv offset
#if 0

   //Note(Agu): 0.0004f- 0.0006f fits well with a sampler mod of 1.5f
   //Calculated uv offset
   //real32 texeloffset = 0.1f / gameRenderer->back_buffer_width; 
   if(commands->applyUvOffset)
   {
     real32 texeloffset = 0.0000f;
     uvMin.x += texeloffset;
     uvMin.y += texeloffset;
     uvMax.x -= texeloffset * 2.0f;
     uvMax.y -= texeloffset * 2.0f;
   }
#endif
#if 0

   //
   //Offset uvs to filter correctly
   //
   //real32 Ar = (real32)gameRenderer->back_buffer_width / gameRenderer->back_buffer_height;
   real32 Ar = (real32)gameRenderer->back_buffer_width / gameRenderer->back_buffer_height;
   real32 FilterWidth = 1.0f + (6.5f * 0.1f);
   vec2 QuadSz = {v3.x - v0.x, v1.y - v0.y}; 
   vec3 xhalf = {QuadSz.x * 0.5f, QuadSz.y * 0.5f, 1};
   real32 PixelOffH = 0.12f;
   PixelOffH = 0.1f;
   //Note(Agu): Get the texture width.
   real32 texeloffset = PixelOffH / 512.0f;
   texeloffset *= Ar * FilterWidth; 
   uvMin.x += texeloffset;
   uvMin.y += texeloffset;
   uvMax.x -= texeloffset * 2.0f;
   uvMax.y -= texeloffset * 2.0f;
#endif
   //uv offset based on distance
#if 0

   vec4 BL4 = V4(v0.x, v0.y, v0.z, 1);
   vec4 TL4 = V4(v1.x, v1.y, v1.z, 1);
   vec4 TR4 = V4(v2.x, v2.y, v2.z, 1);
   vec4 BR4 = V4(v3.x, v3.y, v3.z, 1);

   matrix4x4 WVP = commands->projection;

   TL4 = matrix4x4_v4_mul_rows(WVP,TL4);
   TR4 = matrix4x4_v4_mul_rows(WVP,TR4);
   BL4 = matrix4x4_v4_mul_rows(WVP,BL4);
   BR4 = matrix4x4_v4_mul_rows(WVP,BR4);
   real32 TLW = TL4.w;
   real32 TRW = TR4.w;
   real32 BLW = BL4.w;
   real32 BRW = BR4.w;

   if(TLW)
   {

   }
   real32 texeloffset = 0.0002f;

   //v1 = vec4Divide(v1, v1.w);
   //v2 = vec4Divide(v2, v2.w);
   //v0 = vec4Divide(v0, v0.w);
   //v3 = vec4Divide(v3, v3.w);
#endif
#if 0
   v0 = vec3_scale(v0, 2);
   v1 = vec3_scale(v1, 2);
   v2 = vec3_scale(v2, 2);
   v3 = vec3_scale(v3, 2);
#endif

   uint32 color32 = render_pack_color_rgba(color);
   uint32 textureId = texture->index;
   //i32 index_add = (texture->height / gameRenderer->texture_array_h) - 1;
   //if(index_add > 0)
   //{
   //    textureId += index_add;
   //}

   //render_vertex v0 = {v0.x, v0.y, v0.z, uvMin.x , uvMax.y , Color32, textureId};
   //render_vertex v1 = {v1.x, v1.y, v1.z, uvMin.x , uvMin.y , Color32, textureId};
   //render_vertex v2 = {v2.x, v2.y, v2.z, uvMax.x , uvMin.y , Color32, textureId};
   //render_vertex v3 = {v3.x, v3.y, v3.z, uvMax.x , uvMax.y , Color32, textureId};

   //goes in counter-clockwise order starting from the "bottom left" corner
   render_vertex v0_clipped = {0};
   render_vertex v1_clipped = {0};
   render_vertex v2_clipped = {0};
   render_vertex v3_clipped = {0};


   v0_clipped.location = v0;
   v0_clipped.uv       = uv0;
   v0_clipped.color    = color32;
   v0_clipped.texture  = textureId;

   v1_clipped.location = v1;
   v1_clipped.uv       = uv1;
   v1_clipped.color    = color32;
   v1_clipped.texture  = textureId;

   v2_clipped.location = v2;
   v2_clipped.uv       = uv2;
   v2_clipped.color    = color32;
   v2_clipped.texture  = textureId;

   v3_clipped.location = v3;
   v3_clipped.uv       = uv3;
   v3_clipped.color    = color32;
   v3_clipped.texture  = textureId;

   if(!gameRenderer->lock_next_vertices)
   {

       //render_command_drawquad *drawquad = (render_command_drawquad *)commands->commands_offset;
       render_command_drawquad *drawquad = render_commands_PushCommand(commands,
		                                                           render_command_type_drawquad,
																   render_command_drawquad);
       drawquad->vertices[0] = v0_clipped;
       drawquad->vertices[1] = v1_clipped;
       drawquad->vertices[2] = v2_clipped;
       drawquad->vertices[3] = v3_clipped;
   }
   else
   {
	   //render_vertex locked_vertices[4];

       //locked_vertices[0] = v0_clipped;
       //locked_vertices[1] = v1_clipped;
       //locked_vertices[2] = v2_clipped;
       //locked_vertices[3] = v3_clipped;

	    Assert(gameRenderer->update_locked_vertices)

	    u32 count = gameRenderer->locked_quads_count * 4; // * 4 ?

	    gameRenderer->locked_vertices[count + 0] = v0_clipped;
	    gameRenderer->locked_vertices[count + 1] = v1_clipped;
	    gameRenderer->locked_vertices[count + 2] = v2_clipped;
	    gameRenderer->locked_vertices[count + 3] = v3_clipped;

	    gameRenderer->locked_quads_count += 1;

	    Assert(gameRenderer->locked_quads_count < gameRenderer->locked_quads_max);

	    render_locked_vertices_group *currentGroup = gameRenderer->locked_vertices_groups + gameRenderer->current_locked_vertices_pushed;
	    currentGroup->count++;
   }


   /*
   commands->command_count++;
   (*(game_renderer *)(commands->gameRenderer)).draw_count++;

   commands->commands_offset += sizeof(render_command_drawquad);
   */
}


static inline void
render_push_quad_uv_min_max(render_commands *commands,
                      render_texture *texture,
                      vec3 v0,
                      vec3 v1,
                      vec3 v2,
                      vec3 v3,
                      vec2 uvMin,
                      vec2 uvMax,
                      vec4 color)
{


   vec2 uv0 = {0};
   vec2 uv1 = {0};
   vec2 uv2 = {0};
   vec2 uv3 = {0};

   uv0.x = uvMin.x;
   uv0.y = uvMax.y;

   uv1.x = uvMin.x;
   uv1.y = uvMin.y;

   uv2.x = uvMax.x;
   uv2.y = uvMin.y;

   uv3.x = uvMax.x;
   uv3.y = uvMax.y;

   render_push_quad(commands,
                    texture,
                    v0,
                    v1,
                    v2,
                    v3,
                    uv0,
                    uv1,
                    uv2,
                    uv3,
                    color);
}


static inline void
render_push_quad_uv_min_max_frames(render_commands *commands,
                                          render_texture *texture,
                                          vec3 v0,
                                          vec3 v1,
                                          vec3 v2,
                                          vec3 v3,
                                          u32 frame_x,
                                          u32 frame_y,
                                          u32 frame_w,
                                          u32 frame_h,
										  vec4 color)
{

   render_uv_minmax uvs = render_frames_to_uv_min_max(
		   commands->gameRenderer->texture_array_w,
		   commands->gameRenderer->texture_array_h,
                                              frame_x,
                                              frame_y,
                                              frame_w,
                                              frame_h);
   render_push_quad_uv_min_max(commands,
                         texture,
                         v0,
                         v1,
                         v2,
                         v3,
                         uvs.min,
                         uvs.max,
                         color);

}


#define render_commands_begin_default(game_renderer) render_commands_begin(game_renderer, render_flags_DepthTest | render_flags_DepthPeel)
//#define render_commands_begin_2d(game_renderer) render_commands_begin(game_renderer, render_flags_Blending)
#define render_commands_begin_no_depth(game_renderer) render_commands_begin(game_renderer, render_flags_DepthPeel)

static render_commands*  
render_commands_begin(game_renderer *gameRenderer, render_flags flags)
{
  //Assert(gameRenderer->begin_count < MAXRENDERCOMMANDS);
  //render_commands *commands = &gameRenderer->rcommands[gameRenderer->begin_count++];
	//TODO(Agu): Use memory_area instead ?
  render_commands *commands = (render_commands *)gameRenderer->render_commands_buffer_offset;
  (uint8 *)gameRenderer->render_commands_buffer_offset += sizeof(render_commands);

  commands->commands_offset = gameRenderer->render_commands_buffer_offset;
  commands->commands_base = gameRenderer->render_commands_buffer_offset;
  commands->command_count = 0;
  commands->gameRenderer = gameRenderer;
  commands->set_viewport_and_clip = 0;
  commands->render_flags = flags;
  commands->camera_type = render_camera_perspective;
  return commands;
}

static render_commands *
render_commands_begin_2d(game_renderer *renderer)
{
	render_commands *commands = render_commands_begin(renderer, render_flags_Blending);
	commands->camera_type = render_camera_2d;
	return(commands);
}

static void 
render_commands_end(render_commands *commands)
{
    game_renderer *gameRenderer = commands->gameRenderer;

    gameRenderer->render_commands_buffer_offset = commands->commands_offset;
	gameRenderer->depth_peel_calls += (commands->render_flags & render_flags_DepthPeel) > 0;
	gameRenderer->begin_count++;
}

static inline void
render_commands_restore_viewport_at_end(render_commands *commands)
{
	commands->restore_viewport_and_clip = 1;
}


static void
render_commands_DrawQuad(render_commands *commands,
						 render_texture *texture,
						 vec3 v0,
						 vec3 v1,
						 vec3 v2,
						 vec3 v3,
						 uint32 frame_x, uint32 frame_y, uint32 frame_w, uint32 frame_h)
{

	game_renderer *gameRenderer = commands->gameRenderer;
   render_uv_minmax uvs = render_frames_to_uv_min_max(
		   commands->gameRenderer->texture_array_w,
		   commands->gameRenderer->texture_array_h,
                                              frame_x,
                                              frame_y,
                                              frame_w,
                                              frame_h);
	render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvs.min, uvs.max, V4(0xff, 0xff, 0xff, 0xff));
}

#define debug_render_line_up(commands, p0, p1) \
	render_draw_line_up(commands, p0, p1, V4(255, 0, 0, 255), 1)
static void
render_draw_line_up(render_commands *commands,
				  vec3 p1,
				  vec3 p2,
				  vec4 color,
				  real32 thickness)
{
   game_renderer *gameRenderer = commands->gameRenderer;
   //Note(Agu):Temporal uvMin/max "fake" coordinates
   vec2 uvMin = {0, 0};
   vec2 uvMax = {1, 1};
   //top face
   vec3 pDist = {
       p2.x - p1.x,
       p2.y - p1.y,
       p2.z - p1.z
   };
   vec3 pCross = vec3_normalize_safe(vec3_cross(pDist, gameRenderer->camera_z));
   pCross = vec3_f32_mul(pCross, thickness);
#if 1
   vec3 v0 = vec3_add(p1,pCross); 
   vec3 v1 = vec3_sub(p1,pCross); 

   vec3 v2 = vec3_sub(p2,pCross); 
   vec3 v3 = vec3_add(p2,pCross);

#else
   vec3 v0 = vec3_sub(p1, vec3_scale(gameRenderer->camera_x, thickness));
   vec3 v3 = vec3_add(p1, vec3_scale(gameRenderer->camera_x, thickness));

   vec3 v1 = vec3_sub(p2, vec3_scale(gameRenderer->camera_y, thickness)); 
   vec3 v2 = vec3_add(p2, vec3_scale(gameRenderer->camera_y, thickness));
#endif

   render_texture *texture = &gameRenderer->white_texture;
   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}

#if 0
static void
render_draw_line( render_commands *commands,
         vec3 p1,
         vec3 p2,
         vec4 color,
		 real32 lineSz,
		 real32 lineDelta)
         
{
   game_renderer *gameRenderer = commands->gameRenderer;
   //Temporal uvMin/max "fake" coordinates
   vec2 uvMin = {0, 0};
   vec2 uvMax = {1, 1};
   //top face
   vec3 pDifference = vec3_normalize_safe(vec3_sub(p2, p1));
   real32 ax = pDifference.x;
   real32 ay = pDifference.y;
   pDifference.x = ay;
   pDifference.y = ax;
   pDifference.x *= lineSz;
   pDifference.y *= lineSz;
   pDifference.z *= lineSz;

   real32 lineDelta2 = 1.0f - lineDelta;

   vec3 lineDif = {pDifference.x * lineDelta,
				   pDifference.y * lineDelta,
				   pDifference.z * lineDelta};

   vec3 v0 = vec3_add(p1, lineDif);
   vec3 v3 = vec3_add(p2, lineDif);
   lineDif = V3(pDifference.x * lineDelta2,
				pDifference.y * lineDelta2,
				pDifference.z * lineDelta2);
   vec3 v1 = vec3_sub(p1,lineDif); 
   vec3 v2 = vec3_sub(p2,lineDif); 

#if 0
   real32 posDelta = 0;
   pDifference.x *= posDelta;
   pDifference.y *= posDelta;
   pDifference.z *= posDelta;

   v0 = vec3_add(v0 ,pDifference); 
   v1 = vec3_add(v1 ,pDifference); 
   v2 = vec3_add(v2 ,pDifference); 
   v3 = vec3_add(v3 ,pDifference);
#endif

   render_texture *texture = &gameRenderer->white_texture;
   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}
#else
static void
render_draw_line( render_commands *commands,
         vec3 p1,
         vec3 p2,
		 vec3 z_axis,
         vec4 color,
		 real32 lineSz,
		 real32 lineDelta)
         
{
   game_renderer *gameRenderer = commands->gameRenderer;
   //Temporal uvMin/max "fake" coordinates
   vec2 uvMin = {0, 0};
   vec2 uvMax = {1, 1};
   //top face
   vec3 pDifference = vec3_normalize_safe(vec3_sub(p2, p1));
   vec3 points_cross = vec3_cross(pDifference, z_axis);
   real32 ax = pDifference.x;
   real32 ay = pDifference.y;

   points_cross = vec3_scale(points_cross, lineSz);

   vec3 v0 = vec3_sub(p1, points_cross);
   vec3 v1 = vec3_sub(p2, points_cross);
   vec3 v2 = vec3_add(p2, points_cross);
   vec3 v3 = vec3_add(p1, points_cross);
#if 0
   pDifference.x = ay;
   pDifference.y = ax;
   pDifference.x *= lineSz;
   pDifference.y *= lineSz;
   pDifference.z *= lineSz;

   real32 lineDelta2 = 1.0f - lineDelta;

   vec3 lineDif = {pDifference.x * lineDelta,
				   pDifference.y * lineDelta,
				   pDifference.z * lineDelta};

   vec3 v0 = vec3_add(p1, lineDif);
   vec3 v3 = vec3_add(p2, lineDif);
   lineDif = V3(pDifference.x * lineDelta2,
				pDifference.y * lineDelta2,
				pDifference.z * lineDelta2);
   vec3 v1 = vec3_sub(p1,lineDif); 
   vec3 v2 = vec3_sub(p2,lineDif); 

   real32 posDelta = 0;
   pDifference.x *= posDelta;
   pDifference.y *= posDelta;
   pDifference.z *= posDelta;

   v0 = vec3_add(v0 ,pDifference); 
   v1 = vec3_add(v1 ,pDifference); 
   v2 = vec3_add(v2 ,pDifference); 
   v3 = vec3_add(v3 ,pDifference);
#endif

   render_texture *texture = &gameRenderer->white_texture;
   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}
#endif

static void
render_vertices_edges(render_commands *renderCommands,
		                vec3 v0,
						vec3 v1,
						vec3 v2,
						vec3 v3,
						vec4 color,
						f32 thickness)
{

    render_draw_line_up(renderCommands,
				      v1,
				      v0,
				      color,
				      thickness);

    render_draw_line_up(renderCommands,
				      v1,
				      v2,
				      color,
				      thickness);

    render_draw_line_up(renderCommands,
				      v2,
				      v3,
				      color,
				      thickness);

    render_draw_line_up(renderCommands,
				      v0,
				      v3,
				      color,
				      thickness);

}

static inline void 
render_vertices_colored(render_commands *commands,
		                vec3 v0,
						vec3 v1,
						vec3 v2,
						vec3 v3,
						vec4 color)
{
    render_push_quad_uv_min_max(commands,
			                  &commands->gameRenderer->white_texture,
							  v0,
							  v1,
							  v2,
							  v3,
							  V2(0, 0),
							  V2(1, 1),
							  color);
}

static inline void
render_rectangle_colored(
		render_commands *commands,
		vec3 p,
		vec2 size,
		vec3 x_axis,
		vec3 y_axis,
		vec4 color)
{
	x_axis.x *= size.x;
	x_axis.y *= size.x;
	x_axis.z *= size.x;

	y_axis.x *= size.y;
	y_axis.y *= size.y;
	y_axis.z *= size.y;

	vec3 v0 = 
	{
		p.x - (x_axis.x + y_axis.x) * 0.5f,
		p.y - (x_axis.y + y_axis.y) * 0.5f,
		p.z - (x_axis.z + y_axis.z) * 0.5f,
	};


	vec3 v2 = 
	{
		p.x + (x_axis.x + y_axis.x) * 0.5f,
		p.y + (x_axis.y + y_axis.y) * 0.5f,
		p.z + (x_axis.z + y_axis.z) * 0.5f,
	};

	vec3 v1 = 
	{
	    v0.x,
		v0.y + (y_axis.y),
		v0.z + (y_axis.z)
	};

	vec3 v3 = 
	{
	    v2.x,
		v2.y - (y_axis.y),
		v2.z - (y_axis.z)
	};

	render_push_quad(commands,
                     &commands->gameRenderer->white_texture,
					 v0,
					 v1,
					 v2,
					 v3,
					 v2(0, 0),
					 v2(0, 1),
					 v2(1, 1),
					 v2(1, 0),
					 color);
}

#define render_rectangle_bl(commands, p, size, x_axis, y_axis, color) \
	render_rectangle(commands, p, size, x_axis, y_axis, 1.0f, 1.0f, color)
#define render_rectangle_xy_bl(commands, p, size, color) \
	render_rectangle(commands, p, size, V3(1, 0, 0), V3(0, 1, 0), 1.0f, 1.0f, color)
static void
render_rectangle(
		render_commands *commands,
		vec3 p,
		vec2 size,
		vec3 x_axis,
		vec3 y_axis,
		f32 x_corner_percent,
		f32 y_corner_percent,
		vec4 color)
{
	{
		vec3 x_size0 = vec3_scale(x_axis, .5f * x_corner_percent * size.x);
		vec3 x_size1 = vec3_scale(x_axis, .5f * (1.0f - x_corner_percent) * size.x);
		vec3 y_size0 = vec3_scale(y_axis, .5f * y_corner_percent * size.y);
		vec3 y_size1 = vec3_scale(y_axis, .5f * (1.0f - y_corner_percent) * size.y);
		p = vec3_add(p, x_size0);
		p = vec3_sub(p, x_size1);
		p = vec3_add(p, y_size0);
		p = vec3_sub(p, y_size1);
	}
	
	x_axis.x *= size.x;
	x_axis.y *= size.x;
	x_axis.z *= size.x;

	y_axis.x *= size.y;
	y_axis.y *= size.y;
	y_axis.z *= size.y;

	vec3 v0 = 
	{
		p.x - (x_axis.x + y_axis.x) * 0.5f,
		p.y - (x_axis.y + y_axis.y) * 0.5f,
		p.z - (x_axis.z + y_axis.z) * 0.5f,
	};


	vec3 v2 = 
	{
		p.x + (x_axis.x + y_axis.x) * 0.5f,
		p.y + (x_axis.y + y_axis.y) * 0.5f,
		p.z + (x_axis.z + y_axis.z) * 0.5f,
	};

	vec3 v1 = 
	{
	    v0.x,
		v0.y + (y_axis.y),
		v0.z + (y_axis.z)
	};

	vec3 v3 = 
	{
	    v2.x,
		v2.y - (y_axis.y),
		v2.z - (y_axis.z)
	};

	render_push_quad(commands,
                     &commands->gameRenderer->white_texture,
					 v0,
					 v1,
					 v2,
					 v3,
					 v2(0, 0),
					 v2(0, 1),
					 v2(1, 1),
					 v2(1, 0),
					 color);
}

#define render_rectangle_borders(renderCommands, p, sz, x_axis, y_axis, thickness, color) \
render_hollow_rec(renderCommands, p, sz, x_axis, y_axis, 0.0f, 0.0f, thickness, color)

static void
render_hollow_rec(render_commands *commands,
				        vec3 p,
		                vec2 size,
				        vec3 x_axis,
				        vec3 y_axis,
						f32 x_corner,
						f32 y_corner,
				        real32 thickness,
				        vec4 color)
{
	//Optimize.
    vec2 uvMin = {0, 0};
    vec2 uvMax = {1, 1};
	render_texture *wt = &commands->gameRenderer->white_texture;

	//vec3 nSz = recSz; 
	vec3 xAxisN = x_axis; 
	vec3 yAxisN = y_axis; 

	xAxisN.x *= thickness;
	xAxisN.y *= thickness;
	xAxisN.z *= thickness;

	yAxisN.x *= thickness;
	yAxisN.y *= thickness;
	yAxisN.z *= thickness;

	x_axis.x *= (size.x * 0.5f);
	x_axis.y *= (size.x * 0.5f);
	x_axis.z *= (size.x * 0.5f);

	y_axis.x *= (size.y * 0.5f);
	y_axis.y *= (size.y * 0.5f);
	y_axis.z *= (size.y * 0.5f);

	vec3 corner_xyAxis = vec3_scale(x_axis, x_corner);
	vec3 corner_yAxis  = vec3_scale(y_axis, y_corner);
	corner_xyAxis      = vec3_add(corner_xyAxis, corner_yAxis);

	vec3 v0 = vec3_sub(p, vec3_add(x_axis, y_axis)); 
	vec3 v2 = vec3_add(p, vec3_add(x_axis, y_axis)); 
	vec3 v1 = vec3_sub(p, x_axis); 
	v1 = vec3_add(v1, y_axis); 
	vec3 v3 = vec3_add(p, x_axis);
	v3 = vec3_sub(v3, y_axis);
	//bottom of the rectangle
	vec3 b_v0 = v0;
	vec3 b_v1 = vec3_add(v0, yAxisN); 
	vec3 b_v2 = vec3_add(v3, yAxisN); 
	vec3 b_v3 = v3; 

    //left side of the rectangle
	vec3 l_v0 = v0;
	vec3 l_v1 = v1; 
	vec3 l_v2 = vec3_add(v1, xAxisN); 
	vec3 l_v3 = vec3_add(v0, xAxisN); 

	//right side
	vec3 r_v0 = v3; 
	vec3 r_v1 = v2; 
	vec3 r_v2 = vec3_add(v2, xAxisN); 
	vec3 r_v3 = vec3_add(v3, xAxisN); 
	r_v0 = vec3_sub(r_v0, xAxisN);
	r_v1 = vec3_sub(r_v1, xAxisN);
	r_v2 = vec3_sub(r_v2, xAxisN);
	r_v3 = vec3_sub(r_v3, xAxisN);

	//top side
	vec3 t_v0 = v1; 
	vec3 t_v1 = vec3_add(v1, yAxisN); 
   //Note(Agu): Extra adds to complete the rectangle.
	vec3 t_v2 = vec3_add(vec3_add(v2, yAxisN), xAxisN); 
	vec3 t_v3 = vec3_add(v2, xAxisN); 
	t_v0 = vec3_sub(t_v0, yAxisN);
	t_v1 = vec3_sub(t_v1, yAxisN);
	t_v2 = vec3_sub(t_v2, vec3_add(xAxisN, yAxisN));
	t_v3 = vec3_sub(t_v3, vec3_add(xAxisN, yAxisN));

	b_v0 = vec3_add(b_v0, corner_xyAxis);
	b_v3 = vec3_add(b_v3, corner_xyAxis);
	b_v1 = vec3_add(b_v1, corner_xyAxis);
	b_v2 = vec3_add(b_v2, corner_xyAxis);

	l_v0 = vec3_add(l_v0, corner_xyAxis);
	l_v3 = vec3_add(l_v3, corner_xyAxis);
	l_v1 = vec3_add(l_v1, corner_xyAxis);
	l_v2 = vec3_add(l_v2, corner_xyAxis);

	r_v0 = vec3_add(r_v0, corner_xyAxis);
	r_v3 = vec3_add(r_v3, corner_xyAxis);
	r_v1 = vec3_add(r_v1, corner_xyAxis);
	r_v2 = vec3_add(r_v2, corner_xyAxis);

	t_v0 = vec3_add(t_v0, corner_xyAxis);
	t_v3 = vec3_add(t_v3, corner_xyAxis);
	t_v1 = vec3_add(t_v1, corner_xyAxis);
	t_v2 = vec3_add(t_v2, corner_xyAxis);

   render_push_quad_uv_min_max(commands, wt, b_v0, b_v1, b_v2, b_v3, uvMin, uvMax, color);
   render_push_quad_uv_min_max(commands, wt, l_v0, l_v1, l_v2, l_v3, uvMin, uvMax, color);
   render_push_quad_uv_min_max(commands, wt, r_v0, r_v1, r_v2, r_v3, uvMin, uvMax, color);
   render_push_quad_uv_min_max(commands, wt, t_v0, t_v1, t_v2, t_v3, uvMin, uvMax, color);

}

static inline void
render_cube(
		render_commands *commands,
		vec3 p,
		vec3 cube_size,
		vec4 color)
{
	vec3 cube_size_half = vec3_scale(cube_size, 0.5f);

	vec3 p_z_max = {
		p.x,
		p.y,
		p.z + cube_size_half.z
	};

	vec3 p_z_min = {
		p.x,
		p.y,
		p.z - cube_size_half.z
	};

	vec3 p_y_max = {
		p.x,
		p.y + cube_size_half.y,
		p.z
	};

	vec3 p_y_min = {
		p.x,
		p.y - cube_size_half.y,
		p.z
	};

	vec3 p_x_max = {
		p.x + cube_size_half.x,
		p.y,
		p.z
	};

	vec3 p_x_min = {
		p.x - cube_size_half.x,
		p.y,
		p.z
	};

	render_rectangle_colored(
		commands,
		p_x_min,
		V2(cube_size.y, cube_size.z),
		V3(0, 1, 0),
	    V3(0, 0, 1),	
		color);

	render_rectangle_colored(
		commands,
		p_x_max,
		V2(cube_size.y, cube_size.z),
		V3(0, 1, 0),
	    V3(0, 0, 1),	
		color);

#if 1
	render_rectangle_colored(
		commands,
		p_y_min,
		V2(cube_size.x, cube_size.z),
		V3(1, 0, 0),
	    V3(0, 0, 1),
		color);

	render_rectangle_colored(
		commands,
		p_y_max,
		V2(cube_size.x, cube_size.z),
		V3(1, 0, 0),
	    V3(0, 0, 1),
		color);

	render_rectangle_colored(
		commands,
		p_z_min,
		V2(cube_size.x, cube_size.y),
		V3(1, 0, 0),
	    V3(0, 1, 0),
		color);

	render_rectangle_colored(
		commands,
		p_z_max,
		V2(cube_size.x, cube_size.y),
		V3(1, 0, 0),
	    V3(0, 1, 0),
		color);
#endif
}

static inline void
render_rectangle_borders_corner(render_commands *commands,
				        vec3 p,
		                vec2 size,
				        vec3 x_axis,
				        vec3 y_axis,
				        vec4 color,
				        real32 thickness)
{

}

//#define render_cube_borders(commands, p, size, color, thickness)\
//	render_cube_borders_axes(commands, p, size, V3(1, 0, 0), V3(0, 1, 0), V3(0, 0, 1), color, thickness)

static inline void
render_cube_borders(
		render_commands *render_commands,
		vec3 cube_position,
		vec3 cube_size,
		f32 border_thickness,
		vec4 color
		)
{
	vec3 size_half = vec3_scale(cube_size, 0.5f);

	vec3 next_border_position = cube_position;
	//top
	next_border_position.z += size_half.z;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.x, cube_size.y),
			V3(1, 0, 0),
			V3(0, 1, 0),
			border_thickness,
			color);
	//bottom
	next_border_position.z -= cube_size.z;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.x, cube_size.y),
			V3(1, 0, 0),
			V3(0, -1, 0),
			border_thickness,
			color);
	//restore
	next_border_position.z = cube_position.z;
	//x negative face
	f32 xn = cube_position.x - size_half.x;
	next_border_position.x = xn;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.y, cube_size.z),
			V3(0, -1, 0),
			V3(0, 0, 1),
			border_thickness,
			color);
	//x positive face
	next_border_position.x += cube_size.x;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.y, cube_size.z),
			V3(0, 1, 0),
			V3(0, 0, 1),
			border_thickness,
			color);
	//restore
	next_border_position.x = cube_position.x;
	//y negative face
	next_border_position.y = cube_position.y - size_half.y;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.x, cube_size.z),
			V3(1, 0, 0),
			V3(0, 0, 1),
			border_thickness,
			color);
	//y_positive_face
	next_border_position.y = cube_position.y + size_half.y;
	render_rectangle_borders(
			render_commands,
			next_border_position,
			V2(cube_size.x, cube_size.z),
			V3(-1, 0, 0),
			V3(0, 0, 1),
			border_thickness,
			color);
}

static inline void
render_cube_borders_axes(render_commands *commands,
				   vec3 P,
				   vec3 size,
				   vec3 x_axis,
				   vec3 y_axis,
				   vec3 zAxis,
				   vec4 color,
				   f32 thickness)
{
	vec3 sizeHalf = vec3_scale(size, 0.5f);

	vec3 PU = {
		P.x + (zAxis.x * sizeHalf.z),
		P.y + (zAxis.y * sizeHalf.y),
		P.z + (zAxis.z * sizeHalf.z)
	};

	vec3 PD = {
		P.x - (zAxis.x * sizeHalf.z),
		P.y - (zAxis.y * sizeHalf.y),
		P.z - (zAxis.z * sizeHalf.z)
	};

	vec3 PFoward = {
		P.x + (y_axis.x * sizeHalf.z),
		P.y + (y_axis.y * sizeHalf.y),
		P.z + (y_axis.z * sizeHalf.z)
	};

	vec3 PBack = {
		P.x - (y_axis.x * sizeHalf.z),
		P.y - (y_axis.y * sizeHalf.y),
		P.z - (y_axis.z * sizeHalf.z)
	};

	vec3 PSideR = {
		P.x + (x_axis.x * sizeHalf.z),
		P.y + (x_axis.y * sizeHalf.y),
		P.z + (x_axis.z * sizeHalf.z)
	};

	vec3 PSideL = {
		P.x - (x_axis.x * sizeHalf.z),
		P.y - (x_axis.y * sizeHalf.y),
		P.z - (x_axis.z * sizeHalf.z)
	};


    //side faces
    render_rectangle_borders(
			commands,
			PSideR,
			V2(size.y, size.z),
			y_axis, zAxis,thickness, color);
    render_rectangle_borders(
			commands,
			PSideL,
			V2(size.y, size.z), y_axis, zAxis, thickness, color);
	//front faces
    render_rectangle_borders(commands, PFoward, V2(size.x, size.z), x_axis, zAxis, thickness, color);
    render_rectangle_borders(commands, PBack, V2(size.x, size.y), x_axis, zAxis, thickness, color);
    //Up/Down
    render_rectangle_borders(commands, PU, V2(size.x, size.y), x_axis, y_axis, thickness, color);
    render_rectangle_borders(commands, PD, V2(size.x, size.y), x_axis, y_axis, thickness, color);

	render_cube(
			commands,
			PU,
			V3(1, 1, 1),
			V4(255, 0, 0, 255));
}


static inline void
render_cube_borders_rotated(render_commands *commands,
				   vec3 P,
				   vec3 size,
				   f32 pitch,
				   f32 yaw,
				   f32 roll,
				   vec4 color,
				   f32 thickness)
{
	matrix4x4 cubeRotation = matrix4x4_rotation_scale(pitch, yaw, roll);
    vec3 cubeX = matrix4x4_v3_get_column(cubeRotation, 0); 
    vec3 cubeY = matrix4x4_v3_get_column(cubeRotation, 1); 
    vec3 cubeZ = matrix4x4_v3_get_column(cubeRotation, 2); 
	render_cube_borders_axes(commands, P, size, cubeX, cubeY, cubeZ, color, thickness);
}

#define render_draw_rectangle_colored_axes_sized(commands, p, x_axis, y_axis, color)\
	render_draw_rectangle(commands, &commands->gameRenderer->white_texture, p, x_axis, y_axis, V2(0, 0), V2(1, 1), color)
#define render_draw_rectangle_colored(commands, p, size, x_axis, y_axis, color)\
	render_draw_rectangle(commands, &commands->gameRenderer->white_texture, p, vec3_scale(x_axis, size.x), vec3_scale(y_axis, size.y), V2(0, 0), V2(1, 1), color)
static void
render_draw_rectangle(render_commands *commands,
				 render_texture *texture,
				 vec3 p,
				 vec3 x_axis,
				 vec3 y_axis,
				 vec2 uvMin,
				 vec2 uvMax,
				 vec4 color)
						 
{
	//x to x
	x_axis.x *= 0.5f;
	x_axis.y *= 0.5f;
	x_axis.z *= 0.5f;
	y_axis.x *= 0.5f;
	y_axis.y *= 0.5f;
	y_axis.z *= 0.5f;

	vec3 v0 = vec3_sub(p, vec3_add(x_axis, y_axis)); 
	vec3 v2 = vec3_add(p, vec3_add(x_axis, y_axis)); 
	vec3 v1 = vec3_sub(p, x_axis); 
	v1 = vec3_add(v1, y_axis); 
	vec3 v3 = vec3_add(p, x_axis);
	v3 = vec3_sub(v3, y_axis);

   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}
static void
render_draw_hollow_rec_up(
		render_commands *commands,
		vec3 p,
		f32 w,
		f32 h,
		vec3 x_axis,
		vec3 y_axis,
		f32 x_corner,
		f32 y_corner,
		f32 thickness,
		vec4 color)
{
	vec3 xa0 = x_axis;
	vec3 ya0 = y_axis;
	vec3 xa1 = x_axis;
	vec3 ya1 = y_axis;
	f32 xc1 = 1.0f - x_corner;
	f32 xc0 = x_corner;
	f32 yc1 = 1.0f - y_corner;
	f32 yc0 = y_corner;

	xa0.x *= xc0 * w * .5f;
	xa0.y *= yc0 * w * .5f;
	ya0.x *= xc0 * h * .5f;
	ya0.y *= yc0 * h * .5f;

	xa1.x *= xc1 * w * .5f;
	xa1.y *= yc1 * w * .5f;
	ya1.x *= xc1 * h * .5f;
	ya1.y *= yc1 * h * .5f;

	vec3 thickness_x = vec3_scale(x_axis, thickness);
	vec3 thickness_y = vec3_scale(y_axis, thickness);
//	p = vec3_sub(p, vec3_scale(xa0, .5f));
//	p = vec3_sub(p, vec3_scale(ya0, .5f));
//	p = vec3_add(p, vec3_scale(xa1, .5f));
//	p = vec3_add(p, vec3_scale(ya1, .5f));
	//bottom left to top left
	vec3 p0 = vec3_sub(p, vec3_add(xa0, ya0));
	vec3 p1 = vec3_sub(p, xa0);
	p1 = vec3_add(p1, ya1);
	p0 = vec3_add(p0, thickness_x);
	p1 = vec3_add(p1, thickness_x);
	render_draw_line_up(commands, p0, p1, color, thickness);
	//top left to top right
	p0 = vec3_sub(p, xa0);
	p0 = vec3_add(p0, ya1);
	p1 = vec3_add(p, xa1);
	p1 = vec3_add(p1, ya1);
	p0 = vec3_add(p0, thickness_x);
	p0 = vec3_add(p0, thickness_y);
	p1 = vec3_add(p1, thickness_x);
	p1 = vec3_add(p1, thickness_y);
	render_draw_line_up(commands, p0, p1, color, thickness);
	//top right to bottom right
	p0 = vec3_add(p, xa1);
	p0 = vec3_add(p0, ya1);
	p1 = vec3_add(p, xa1);
	p1 = vec3_sub(p1, ya0);
	p0 = vec3_add(p0, thickness_x);
	p0 = vec3_add(p0, thickness_y);
	p1 = vec3_add(p1, thickness_x);
//	p1 = vec3_add(p1, thickness_y);
	render_draw_line_up(commands, p0, p1, color, thickness);
	//bottom left to bottom right
	p0 = vec3_sub(p, xa0);
	p0 = vec3_sub(p0, ya0);
	p1 = vec3_add(p, xa1);
	p1 = vec3_sub(p1, ya0);
	p0 = vec3_add(p0, thickness_x);
	p0 = vec3_add(p0, thickness_y);
	p1 = vec3_add(p1, thickness_x);
	p1 = vec3_add(p1, thickness_y);
	render_draw_line_up(commands, p0, p1, color, thickness);
}


// ;support more circle vertices!
#define render_Circle(commands, circlePosition, x_axis, y_axis, radius, thickness, color) \
	render_Circle_(commands, circlePosition, x_axis, y_axis, radius, 4, thickness, color)
static void
render_Circle_(render_commands *commands,
			  vec3 p,
			  vec3 x_axis,
			  vec3 y_axis,
			  f32 radius,
			  u32 circles,
			  f32 thickness,
			  vec4 color)
						 
{
	game_renderer *gameRenderer = commands->gameRenderer;
	render_texture *texture = &gameRenderer->white_texture;
	vec2 uvMin = {0};
	vec2 uvMax = {1, 1};
    vec3 zAxis = vec3_cross(x_axis, y_axis);
	u32 circleCount  = circles;
	f32 circleFactor = (radius / circleCount);


   vec3 xWorld = {1, 0, 0};
   vec3 yWorld = {0, 1, 0};

   vec3 xWorldHalf = vec3_scale(xWorld, 0.5f);
   vec3 yWorldHalf = vec3_scale(yWorld, 0.5f);

   vec3 yRadius = vec3_scale(yWorld, radius * 2 / circleCount * 0.75f);

	f32 rotation = (PI / 2) / circles;

	matrix4x4 axesRotation = matrix4x4_Rotation_Axes_Columns(x_axis, y_axis, zAxis); 
	matrix4x4 camRotation = matrix4x4_rotation_scale(gameRenderer->camera_rotation.x, gameRenderer->camera_rotation.y, gameRenderer->camera_rotation.z); 
	vec3 lineP0 = {0};//vec3_scale(xWorld, radius * circleFactor);
	     //lineP0 = vec3_sub(lineP0, vec3_scale(yWorld, radius * circleFactor * 1));

	     //lineP0 = vec3_sub(lineP0, vec3_scale(yWorld, radius * circleFactor * 1));

	lineP0.x += radius;
	lineP0.y -= radius / circleCount;

	xWorldHalf = vec3_scale(xWorldHalf, thickness);
	yWorldHalf = vec3_scale(yWorldHalf, thickness);

    vec2 rotationX = {cos32(rotation), -sin32(rotation)};
	vec2 rotationY = vec2_Perpendicular(rotationX);
	for(u32 c = 0;
			c < (4 * circles);
		    //c < 8;
			c++)
	{
 //camera facing
		                  //else size adapted to axes

		vec3 lineP1 = vec3_add(lineP0, yRadius);

	    vec3 v0 = vec3_sub(V3(0, 0, 0), xWorldHalf); 
		     v0 = vec3_add(v0, yWorldHalf);

	    vec3 v3 = vec3_add(V3(0, 0, 0), xWorldHalf); 
		     v3 = vec3_sub(v3, yWorldHalf);

			 //Rotate to make the circle
		f32 xW0 = vec2_inner(V2(xWorldHalf.x, xWorldHalf.y), rotationX);
		f32 yW0 = vec2_inner(V2(xWorldHalf.x, xWorldHalf.y), rotationY);

		f32 xW1 = vec2_inner(V2(yWorldHalf.x, yWorldHalf.y), rotationX);
		f32 yW1 = vec2_inner(V2(yWorldHalf.x, yWorldHalf.y), rotationY);

		xWorldHalf.x = xW0;
		xWorldHalf.y = yW0;

		yWorldHalf.x = xW1;
		yWorldHalf.y = yW1;

	    vec3 v1 = vec3_sub(V3(0, 0, 0), xWorldHalf);  
		     v1 = vec3_add(v1, yWorldHalf);

	    vec3 v2 = vec3_add(V3(0, 0, 0), xWorldHalf);  
		     v2 = vec3_sub(v2, yWorldHalf);

			 //make the vertices follow the camera
		v0 =  matrix4x4_v3_mul_rows(camRotation, v0, 0);
		v1 =  matrix4x4_v3_mul_rows(camRotation, v1, 0);
		v2 =  matrix4x4_v3_mul_rows(camRotation, v2, 0);
		v3 =  matrix4x4_v3_mul_rows(camRotation, v3, 0);


		vec3 lineP0R = matrix4x4_v3_mul_rows(axesRotation, lineP0, 0);
		vec3 lineP1R = matrix4x4_v3_mul_rows(axesRotation, lineP1, 0);

		//Comment this to connect the circles
		v0 = vec3_add(v0, lineP0R);
		v3 = vec3_add(v3, lineP0R);
		v1 = vec3_add(v1, lineP1R);
		v2 = vec3_add(v2, lineP1R);

		lineP0 = lineP1;

		//Comment this to directly send the sides.
		//render_draw_line_up(commands, vec3_add(lineP0R, p), vec3_add(lineP1R, p), color, thickness);


		//rotate y radius
		f32 xR1 = vec2_inner(V2(yRadius.x, yRadius.y), rotationX);
		f32 yR1 = vec2_inner(V2(yRadius.x, yRadius.y), rotationY);

		yRadius.x = xR1;
		yRadius.y = yR1;


		//Add final world position
		v0 = vec3_add(v0, p);
		v3 = vec3_add(v3, p);
		v1 = vec3_add(v1, p);
		v2 = vec3_add(v2, p);

        render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);

	}
}

static void
render_circle_upfront(render_commands *renderCommands,
			          vec3 p,
			          f32 radius,
			          f32 thickness,
			          vec4 color)
{
	render_Circle(renderCommands,
			      p,
				  renderCommands->gameRenderer->camera_x,
				  renderCommands->gameRenderer->camera_y,
				  radius,
				  thickness,
				  color);
}

#define render_draw_rectangleTextured(commands, texture, p, x_axis, y_axis, frame_x, frame_y, frame_w, frame_h) render_draw_rectangleTexturedColor(commands, texture, p, x_axis, y_axis, frame_x, frame_y, frame_w, frame_h, vec4_all(255))
static inline void
render_draw_rectangleTexturedColor(render_commands *commands,
						 render_texture *texture,
						 vec3 p,
						 vec3 x_axis,
						 vec3 y_axis,
						 uint32 frame_x, uint32 frame_y, uint32 frame_w, uint32 frame_h, vec4 color)
{
   render_uv_minmax uvs = render_frames_to_uv_min_max(
		   commands->gameRenderer->texture_array_w,
		   commands->gameRenderer->texture_array_h,
                                              frame_x,
                                              frame_y,
                                              frame_w,
                                              frame_h);
    render_draw_rectangle(commands, texture, p, x_axis, y_axis, uvs.min, uvs.max , color);
}

static inline void
render_draw_rectangleFilled(
		render_commands *commands,
		vec3 p,
		vec3 x_axis,
		vec3 y_axis,
		vec4 color)
{
   vec2 uvMin = {0, 0};
   vec2 uvMax = {1, 1};
   render_texture *wt = &commands->gameRenderer->white_texture;
   render_draw_rectangle(commands, wt, p, x_axis, y_axis, uvMin, uvMax,color);
}

static void
render_draw_cube(
         render_commands *commands,
         vec3 pos,
         vec3 radius,
		 vec4 color)
{

	render_texture *texture = &commands->gameRenderer->white_texture;

	vec3 radius_h = vec3_scale(radius, 0.5f);
	//top
	vec3 v0 = vec3_add(pos, V3(-radius_h.x, -radius_h.y, radius_h.z));
	vec3 v1 = vec3_add(pos, V3(-radius_h.x, radius_h.y, radius_h.z));
	vec3 v2 = vec3_add(pos, V3(radius_h.x, radius_h.y, radius_h.z));
	vec3 v3 = vec3_add(pos, V3(radius_h.x, -radius_h.y, radius_h.z));

	vec2 uv0 = {0.0f, 1.0f};
	vec2 uv1 = {0.0f, 0.0f};
	vec2 uv2 = {1.0f, 0.0f};
	vec2 uv3 = {1.0f, 1.0f};

	render_push_quad(commands,
			texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);

	vec3 yp_v0 = vec3_add(pos, V3(-radius_h.x, -radius_h.y, -radius_h.z));
	vec3 yp_v1 = vec3_add(pos, V3(-radius_h.x, -radius_h.y, radius_h.z));
	vec3 yp_v2 = vec3_add(pos, V3(radius_h.x,  -radius_h.y, radius_h.z));
	vec3 yp_v3 = vec3_add(pos, V3(radius_h.x,  -radius_h.y, -radius_h.z));

	vec3 xn_v0 = vec3_add(pos, V3(-radius_h.x, radius_h.y,  -radius_h.z));
	vec3 xn_v1 = vec3_add(pos, V3(-radius_h.x, radius_h.y,  radius_h.z));
	vec3 xn_v2 = vec3_add(pos, V3(-radius_h.x, -radius_h.y, radius_h.z));
	vec3 xn_v3 = vec3_add(pos, V3(-radius_h.x, -radius_h.y, -radius_h.z));

	vec3 xp_v0 = vec3_add(pos, V3(radius_h.x, -radius_h.y,  -radius_h.z));
	vec3 xp_v1 = vec3_add(pos, V3(radius_h.x, -radius_h.y,  radius_h.z));
	vec3 xp_v2 = vec3_add(pos, V3(radius_h.x, radius_h.y, radius_h.z));
	vec3 xp_v3 = vec3_add(pos, V3(radius_h.x, radius_h.y, -radius_h.z));


	render_push_quad(commands,
			texture,
			yp_v0,
			yp_v1,
			yp_v2,
			yp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color );

	render_push_quad(commands,
			texture,
			xp_v0,
			xp_v1,
			xp_v2,
			xp_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);

	render_push_quad(commands,
			texture,
			xn_v0,
			xn_v1,
			xn_v2,
			xn_v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);
}

static void
render_draw_sprite_up0(
           render_commands *commands,
           render_texture *texture,
           vec3 v0,
           vec3 v1,
           vec3 v2,
           vec3 v3,
           vec2 uv0,
           vec2 uv1,
           vec2 uv2,
           vec2 uv3)

{
   int text_id = texture->index;

   game_renderer *gameRenderer = commands->gameRenderer;

   //f32 b = 0.7071067811865475244f;
   f32 b = 1.0f;
   //v0.z = v0.z * b;
   //v3.z = v3.z * b;
   //v1.z = v1.z * b;
   //v2.z = v2.z * b;

   //v0.y += v0.z * b;
   //v1.y += v1.z * b;
   //v2.y += v2.z * b;
   //v3.y += v3.z * b;

   //v0.z *= ;
   //v3.z *= ;

   //v1 = vec3_add(v0, vec3_scale( distance_v1_v0, y_axis.y));
   //v2 = vec3_add(v3, vec3_scale( distance_v2_v3, y_axis.y));
	//vec3 mid_point = vertices_get_mid_point(v0,
//	   					                 v1,
//	   					                 v2,
//	   					                 v3);


   vec4 wcolor = {255,255,255,255};
   render_push_quad(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, wcolor); 
}



static void
render_draw_sprite_upright(
           render_commands *commands,
           render_texture *texture,
           vec3 p,
           vec2 xyAxis, //or size
		   real32 xDelta,
		   real32 yDelta,
           uint32 frame_x,
           uint32 frame_y,
           uint32 frame_w,
           uint32 frame_h)
{
	int text_id = texture->index;

	game_renderer *gameRenderer = commands->gameRenderer;

	//real32 frameScale = (real32)frame_h / frame_w;
	//xyAxis.y *= frameScale;
	//xyAxis.x *= frameScale;
	vec3 XAxis = {0};
	render_uv_minmax uvs = render_frames_to_uv_min_max(
			commands->gameRenderer->texture_array_w,
			commands->gameRenderer->texture_array_h,
			frame_x,
			frame_y,
			frame_w,
			frame_h);
	vec3 YAxis = {0};
	//Where specified axes aim
	vec3 XAxisN = {xyAxis.x, 0, 0};
	//Point y to z
	vec3 YAxisN = {0, 0, xyAxis.y};
	vec3 XAxisR = vec3_f32_mul(gameRenderer->camera_x, xyAxis.x);
	vec3 YAxisR = vec3_f32_mul(gameRenderer->camera_y, xyAxis.y);

#if 0
	XAxis = Mult_V3_S(rotationx, size.x);
	YAxis = Mult_V3_S(rotationy, size.y);
#else
	XAxis = vec3_Lerp(XAxisN, xDelta, XAxisR);
	YAxis = vec3_Lerp(YAxisN, yDelta, YAxisR);
#endif

	vec3 v0 = p; 
	vec3 v1 = {0}; 
	vec3 v2 = {0}; 
	vec3 v3 = {0};

	v1.x = p.x - (XAxis.x * 0.5f) + (YAxis.x * 0.5f);
	v1.y = p.y - (XAxis.y * 0.5f) + (YAxis.y * 0.5f);
	v1.z = p.z - (XAxis.z * 0.5f) + (YAxis.z * 0.5f);

	v2.x = p.x + (XAxis.x * 0.5f) + (YAxis.x * 0.5f);
	v2.y = p.y + (XAxis.y * 0.5f) + (YAxis.y * 0.5f);
	v2.z = p.z + (XAxis.z * 0.5f) + (YAxis.z * 0.5f);

	v0.x = p.x - (XAxis.x * 0.5f) - (YAxis.x * 0.5f);
	v0.y = p.y - (XAxis.y * 0.5f) - (YAxis.y * 0.5f);
	v0.z = p.z - (XAxis.z * 0.5f) - (YAxis.z * 0.5f);

	v3.x = p.x + (XAxis.x * 0.5f) - (YAxis.x * 0.5f);
	v3.y = p.y + (XAxis.y * 0.5f) - (YAxis.y * 0.5f);
	v3.z = p.z + (XAxis.z * 0.5f) - (YAxis.z * 0.5f);

	vec4 wcolor = {255,255,255,255};
	render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvs.min, uvs.max, wcolor); 
}

static void
render_DrawSprite(
           render_commands *commands,
           render_texture *texture,
           vec3 p,
           vec3 size,
           uint32 frame_x,
           uint32 frame_y,
           uint32 frame_w,
           uint32 frame_h)
{
   int text_id = texture->index;
   game_renderer *gameRenderer = commands->gameRenderer;
   real32 frameScale = (real32)frame_h / frame_w;
   render_uv_minmax uvs = render_frames_to_uv_min_max(
		   commands->gameRenderer->texture_array_w,
		   commands->gameRenderer->texture_array_h,
                                              frame_x,
                                              frame_y,
                                              frame_w,
                                              frame_h);

   vec4 wColor = vec4_all(255);
   vec3 hSz = {size.x * 0.5f, size.y * 0.5f, size.z * 0.5f};
   hSz.z *= frameScale;
   vec3 v1 = {p.x - hSz.x, p.y + hSz.y, p.z + hSz.z};
   vec3 v2 = {p.x + hSz.x, p.y + hSz.y, p.z + hSz.z};
   vec3 v0 = {p.x - hSz.x, p.y - hSz.y, p.z - hSz.z};
   vec3 v3 = {p.x + hSz.x, p.y - hSz.y, p.z - hSz.z};

   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvs.min, uvs.max, wColor);
}

#define render_color_2d(commands, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color)\
	render_push_quad_2d(commands, &commands->gameRenderer->white_texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color)
static void
render_push_quad_2d(
         render_commands *commands,
         render_texture *texture,
         vec2 v0_2d,
         vec2 v1_2d,
         vec2 v2_2d,
         vec2 v3_2d,
         vec2 uv0,
         vec2 uv1,
         vec2 uv2,
         vec2 uv3,
         vec4 color)
{


   //NOTE(Agu): This goes from -1, 1 to 1, -1. (top left to bottom right)
   game_renderer *gameRenderer = commands->gameRenderer;
   real32 screenClipHalfW = 1.0f / (gameRenderer->back_buffer_width * 0.5f);
   real32 screenClipHalfH = 1.0f / (gameRenderer->back_buffer_height * 0.5f);

   v0_2d.x = v0_2d.x * screenClipHalfW - 1.0f;
   v0_2d.y = 1.0f - v0_2d.y * screenClipHalfH;

   v1_2d.x = v1_2d.x * screenClipHalfW - 1.0f;
   v1_2d.y = 1.0f - v1_2d.y * screenClipHalfH + 0.0f;

   v2_2d.x = v2_2d.x * screenClipHalfW - 1.0f;
   v2_2d.y = 1.0f - v2_2d.y * screenClipHalfH + 0.0f;

   v3_2d.x = v3_2d.x * screenClipHalfW - 1.0f;
   v3_2d.y = 1.0f - v3_2d.y * screenClipHalfH + 0.0f;

   
   vec3 v0 = {v0_2d.x, v0_2d.y, 0}; 
   vec3 v1 = {v1_2d.x, v1_2d.y, 0};
   vec3 v2 = {v2_2d.x, v2_2d.y, 0}; 
   vec3 v3 = {v3_2d.x, v3_2d.y, 0}; 


   render_push_quad(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color);
}

static void
render_push_vertices_2d(
         render_commands *commands,
         render_texture *texture,
         vec2 v0_2d,
         vec2 v1_2d,
         vec2 v2_2d,
         vec2 v3_2d,
         vec2 uv0,
         vec2 uv1,
         vec2 uv2,
         vec2 uv3,
         vec4 color0,
         vec4 color1,
         vec4 color2,
         vec4 color3
		 )
{


   //NOTE(Agu): This goes from -1, 1 to 1, -1. (top left to bottom right)
   game_renderer *gameRenderer = commands->gameRenderer;
   real32 screenClipHalfW = 1.0f / (gameRenderer->back_buffer_width * 0.5f);
   real32 screenClipHalfH = 1.0f / (gameRenderer->back_buffer_height * 0.5f);

   v0_2d.x = v0_2d.x * screenClipHalfW - 1.0f;
   v0_2d.y = 1.0f - v0_2d.y * screenClipHalfH;

   v1_2d.x = v1_2d.x * screenClipHalfW - 1.0f;
   v1_2d.y = 1.0f - v1_2d.y * screenClipHalfH + 0.0f;

   v2_2d.x = v2_2d.x * screenClipHalfW - 1.0f;
   v2_2d.y = 1.0f - v2_2d.y * screenClipHalfH + 0.0f;

   v3_2d.x = v3_2d.x * screenClipHalfW - 1.0f;
   v3_2d.y = 1.0f - v3_2d.y * screenClipHalfH + 0.0f;

   
   vec3 v0 = {v0_2d.x, v0_2d.y, 0}; 
   vec3 v1 = {v1_2d.x, v1_2d.y, 0};
   vec3 v2 = {v2_2d.x, v2_2d.y, 0}; 
   vec3 v3 = {v3_2d.x, v3_2d.y, 0}; 


   render_push_vertices(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color0, color1, color2, color3);
}


static void
render_push_quad_2d_uv_min_max(
         render_commands *commands,
         render_texture *texture,
         vec2 v0_2d,
         vec2 v1_2d,
         vec2 v2_2d,
         vec2 v3_2d,
         vec2 uvMin,
         vec2 uvMax,
         vec4 color)
{


   //NOTE(Agu): This goes from -1, 1 to 1, -1. (top left to bottom right)
   game_renderer *gameRenderer = commands->gameRenderer;
   real32 screenClipHalfW = 1.0f / (gameRenderer->back_buffer_width * 0.5f);
   real32 screenClipHalfH = 1.0f / (gameRenderer->back_buffer_height * 0.5f);

   v0_2d.x = v0_2d.x * screenClipHalfW - 1.0f;
   v0_2d.y = 1.0f - v0_2d.y * screenClipHalfH;

   v1_2d.x = v1_2d.x * screenClipHalfW - 1.0f;
   v1_2d.y = 1.0f - v1_2d.y * screenClipHalfH + 0.0f;

   v2_2d.x = v2_2d.x * screenClipHalfW - 1.0f;
   v2_2d.y = 1.0f - v2_2d.y * screenClipHalfH + 0.0f;

   v3_2d.x = v3_2d.x * screenClipHalfW - 1.0f;
   v3_2d.y = 1.0f - v3_2d.y * screenClipHalfH + 0.0f;

   
   vec3 v0 = {v0_2d.x, v0_2d.y, 0}; 
   vec3 v1 = {v1_2d.x, v1_2d.y, 0};
   vec3 v2 = {v2_2d.x, v2_2d.y, 0}; 
   vec3 v3 = {v3_2d.x, v3_2d.y, 0}; 


   render_push_quad_uv_min_max(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}

static void
render_push_quad_2d_p01(
         render_commands *commands,
         render_texture *texture,
         vec2 p0,
         vec2 p1,
         vec2 uv0,
         vec2 uv1,
         vec2 uv2,
         vec2 uv3,
         vec4 color)
{
   vec2 v0 = p0; 
   vec2 v1 = {p0.x, p1.y};
   vec2 v2 = p1; 
   vec2 v3 = {p1.x, p0.y};
   render_push_quad_2d(commands,
		   texture,
		   v0,
		   v1,
		   v2,
		   v3,
		   uv0,
		   uv1,
		   uv2,
		   uv3,
		   color);
}

static void
render_draw_quad_2d_uv_min_max_wh(
         render_commands *commands,
         render_texture *texture,
         vec2 position,
         vec2 size,
         vec2 uvMin,
         vec2 uvMax,
         vec4 color)
{
   vec2 v1 = {position.x         , position.y};
   vec2 v2 = {position.x + size.x, position.y};
   vec2 v0 = {position.x         , position.y + size.y};
   vec2 v3 = {position.x + size.x, position.y + size.y};
   render_push_quad_2d_uv_min_max(
		   commands,
		   texture,
		   v0,
		   v1,
		   v2,
		   v3,
		   uvMin,
		   uvMax,
		   color);
}

static void
render_sprite_2d(
		render_commands *commands,
		render_texture *texture,
		vec2 p,
		vec2 size,
		u16 fx,
		u16 fy,
		u16 fw,
		u16 fh,
		vec4 color)
{
	vec2 v0 = {p.x, p.y + size.y};
	vec2 v1 = {p.x, p.y};
	vec2 v2 = {p.x + size.x, p.y};
	vec2 v3 = {p.x + size.x, p.y + size.y};
	vec2 uv0, uv1, uv2, uv3;
	render_fill_uvs_from_frames(
			texture->width,
			texture->height,
			fx,
			fy,
			fw,
			fh,
			&uv0,
			&uv1,
			&uv2,
			&uv3
			);

	render_push_quad_2d(
			commands,
			texture,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);

}

static void
render_draw_sprite_2D(
             render_commands *commands,
             render_texture *texture,
             vec2 position,
             vec2 size,
             vec4 color,
             uint32 frame_x,
             uint32 frame_y,
             uint32 frame_w,
             uint32 frame_h)
{
   game_renderer *gameRenderer = commands->gameRenderer;
   int text_id = texture->index;

   frame_x += texture->offsetX;
   frame_y += texture->offsetY;
   render_uv_minmax uvs = render_frames_to_uv_min_max(
		   commands->gameRenderer->texture_array_w,
		   commands->gameRenderer->texture_array_h,
           frame_x,
           frame_y,
           frame_w,
           frame_h);

   render_draw_quad_2d_uv_min_max_wh(commands, texture, position, size, uvs.min, uvs.max, color); 
}

#define render_draw_sprite_2d_p01(commands, texture, p0, p1, uv0, uv1, uv2, uv3) render_push_quad_2d_p01(commands, texture, p0, p1, uv0, uv1, uv2, uv3, vec4_all(255))

static void
render_draw_sprite_2d_frames_wh();


#define render_commands_white_texture(commands) (commands->gameRenderer->white_texture)
static void
render_rectangle_2d(
		render_commands *commands,
		f32 x0,
		f32 y0,
		f32 x1,
		f32 y1,
		vec4 color)
{
	vec2 v0 = {x0, y1};
	vec2 v1 = {x0, y0};
	vec2 v2 = {x1, y0};
	vec2 v3 = {x1, y1};
	vec2 uv0, uv1, uv2, uv3;
	render_fill_uvs_counter_cw(&uv0, &uv1, &uv2, &uv3);
	render_texture *white = &render_commands_white_texture(commands);

	render_push_quad_2d(
			commands,
			white,
			v0,
			v1,
			v2,
			v3,
			uv0,
			uv1,
			uv2,
			uv3,
			color);
}
static void
render_rectangle_2d_ps(
             render_commands *commands,             
             vec2 position,
             vec2 size,
             vec4 color)
{
   game_renderer *gameRenderer = commands->gameRenderer;

   vec2 uvMin = {0};
   vec2 uvMax = {1.0f, 1.0f};

   render_texture white_texture = gameRenderer->white_texture;

   render_draw_quad_2d_uv_min_max_wh(commands, &white_texture, position, size, uvMin, uvMax, color); 
}

static void
render_rectangle_2d_xywh(
		render_commands *commands,
		real32 x,
		real32 y,
		real32 w,
		real32 h,
		vec4 color)
{
    vec2 pos = {x, y};
    vec2 size = {w, h};
    render_rectangle_2d_ps(commands, pos, size, color);   
}


#define render_rectangle_borders_2D_u32(commands, x, y, w, h, thickness, color) render_rectangle_borders_2D(commands, (f32)x, (f32)y, (f32)w, (f32)h, thickness, color)
static void
render_rectangle_borders_2D(
		render_commands *commands,
		real32 x,
		real32 y,
		real32 w,
		real32 h,
		real32 thickness,
		vec4 color)
{
    vec2 br_to_tr = {thickness, h};
    vec2 BRToBL = {w, thickness};

	
	//right line
    vec2 next_line_position = {x + w - thickness, y};
    render_rectangle_2d_ps(commands,
			next_line_position,
			br_to_tr,
			color);
	//bottom line
	next_line_position.x = x;
	next_line_position.y += h - thickness;
    render_rectangle_2d_ps(commands, next_line_position, BRToBL, color);   

    next_line_position.x = x;
	next_line_position.y = y;
    vec2 TLToTR = {w, thickness};
	//Add thickness to complete the border
    vec2 TLToBL = {thickness, h };

	//top line
    render_rectangle_2d_ps(commands, next_line_position, TLToTR, color);   
	//To not collision with the previous line and complete this border
	//next_line_position.y += thickness;
	//left line
    render_rectangle_2d_ps(commands,
			next_line_position,
			TLToBL,
			color);   
}

static inline void
render_rectangle_borders_2D_vec2(render_commands *commands, vec2 p, vec2 recSz,real32 thickness, vec4 color)
{
	render_rectangle_borders_2D(commands, p.x, p.y, recSz.x, recSz.y, thickness, color);
}

static void
render_Triangle2D(render_commands *commands, vec2 p1, vec2 p2, vec2 p3, vec4 color)
{
	vec2 uvMin = {0};
	vec2 uvMax = {1, 1};

#if 0
	vec2 v0 = p1;
	vec2 v3 = p2; 
	vec2 v1 = p3;
	vec2 v2 = p3;
#else
	vec2 v0 = p3;
	vec2 v3 = p3; 
	vec2 v1 = p2;
	vec2 v2 = p1;
#endif

	render_texture white_texture = commands->gameRenderer->white_texture;
   render_push_quad_2d_uv_min_max(commands, &white_texture, v0, v1, v2, v3, uvMin, uvMax, color);
}

static void
render_Triangle2DAngle(render_commands *commands,
		vec2 p,
		f32 size,
		f32 angle,
		vec4 color)
{

   size *= 0.5f;

   vec2 rotationX = {cos32(angle), sin32(angle)};

   vec2 triangleX = vec2_scale(rotationX, size);
   vec2 triangleY = vec2_Perpendicular(triangleX); 

   vec2 p0 = {p.x - triangleY.x,
	          p.y - triangleY.y};

   vec2 p1 = {p.x - triangleX.x + triangleY.x,
	          p.y - triangleX.y + triangleY.y};

   vec2 p2 = {p.x + triangleX.x + triangleY.x,
	          p.y + triangleX.y + triangleY.y};

   render_Triangle2D(commands, p0, p1, p2, color);
}

#if 0
static inline void
render_Line2D(render_commands *commands, vec2 lP0, vec2 lP1, real32 thickness, vec4 color)
{
   render_texture white_texture = gameRenderer->white_texture;
   render_draw_quad_2d_uv_min_max_wh(commands, &white_texture, position, size, uvMin, uvMax, color); 

   real32 hSize = thickness * 0.5f;
   vec2 hP0 = {lP0.x + hSize, lP0.y + hSize};
   vec2 hP1 = {lP1.x + hSize, lP1.y + hSize};

   vec3 v1 = {hP1.x - hSize, hP1.y + o.y, 0};
   vec3 v2 = {hP1.x + hSize, hP1.y + o.y, 0};
   vec3 v0 = {lP1.x, p.y      , 0};
   vec3 v3 = {lP1.x + o.x, p.y      , 0};
}
#endif

static inline void
_render_line_2d(render_commands *commands,
		       vec2 lP0,
			   vec2 lP1,
			   real32 thickness,
			   vec4 color, 
			   f32 displacement)
{
	game_renderer *gameRenderer = commands->gameRenderer;
   //render_texture white_texture = gameRenderer->white_texture;
	f32 half_thickness = thickness * 0.5f;

	vec2 distance_l0_l1 = vec2_normalize(vec2_sub(lP1, lP0));
	vec2 lines_yAxis = vec2_Perpendicular(distance_l0_l1);
	lines_yAxis.x *= thickness * 0.5f;
	lines_yAxis.y *= thickness * 0.5f;

   vec2 v0 = {lP0.x + lines_yAxis.x, lP0.y + lines_yAxis.y};
   vec2 v1 = {lP0.x - lines_yAxis.x, lP0.y - lines_yAxis.y};
   vec2 v2 = {lP1.x - lines_yAxis.x, lP1.y - lines_yAxis.y};
   vec2 v3 = {lP1.x + lines_yAxis.x, lP1.y + lines_yAxis.y};

   //this makes it start from the top-left corner
   v0.x += lines_yAxis.x * displacement;
   v1.x += lines_yAxis.x * displacement;
   v2.x += lines_yAxis.x * displacement;
   v3.x += lines_yAxis.x * displacement;

   v0.y += lines_yAxis.y * displacement;
   v1.y += lines_yAxis.y * displacement;
   v2.y += lines_yAxis.y * displacement;
   v3.y += lines_yAxis.y * displacement;
   v0 = vec2_round_to_int(v0);
   v1 = vec2_round_to_int(v1);
   v2 = vec2_round_to_int(v2);
   v3 = vec2_round_to_int(v3);

   vec2 uvMin = {0};
   vec2 uvMax = {1, 1};

   render_texture *white_texture = &gameRenderer->white_texture;
   render_push_quad_2d_uv_min_max(commands, white_texture, v0, v1, v2, v3, uvMin, uvMax, color);
   //render_push_quad_uv_min_max2D(commands, texture, v0, v1, v2, v3, uvMin, uvMax, color);
}

static inline void
render_line_2d_down(render_commands *commands,
		       vec2 lP0,
			   vec2 lP1,
			   real32 thickness,
			   vec4 color)
{
    _render_line_2d(commands,
    		        lP0,
    			    lP1,
    			    thickness,
    			    color, 
    			    1);
}

static inline void
render_line_2d_center(render_commands *commands,
		       vec2 lP0,
			   vec2 lP1,
			   real32 thickness,
			   vec4 color)
{
    _render_line_2d(commands,
    		        lP0,
    			    lP1,
    			    thickness,
    			    color, 
    			    0);
}

static inline void
render_line_2d_up(render_commands *commands,
		       vec2 lP0,
			   vec2 lP1,
			   real32 thickness,
			   vec4 color)
{
    _render_line_2d(commands,
    		        lP0,
    			    lP1,
    			    thickness,
    			    color, 
    			    -1);
}



#define render_text_2d(commands, fontData, x0, y0, x1, y1, scale, color, text) render_text_2d_All(commands, fontData, x0, y0, x1, y1, scale, color, text)
#define render_text_2d_no_wrap(commands, fontData, p0X, p0Y, scale, color, text) render_text_2d(commands, fontData, p0X, p0Y, F32MAX, F32MAX, scale, color, text)
static void
render_text_2d_All(render_commands *commands,
		      font_proportional *fontData,
			  f32 startX,
			  f32 startY,
			  f32 endX,
			  f32 endY,
			  f32 scale,
			  vec4 color,
			  uint8 *text)
{
    char c = text[0];
    vec2 characterLocation = {0};
    vec2 characterSize     = {0};
    f32 textX     = 0;
	f32 textY     = 0; 
	u32 next_character = 0;
	real32 textSizeX = 0;
	real32 textSizeY = 0; 
	//This never changes
    characterSize.y  = fontData->font_height * scale;

    for(uint32 i = 0;
            (c = text[i]) != '\0';
            i++)
    {
	   next_character = CharToSrc94(text[i]);
       u32 glyph_index		   = (u32)CharToSrc94(c);
       u32 glyph_table_index = glyph_index * fontData->glyph_count;
       i32 horizontal_displacement   = fontData->horizontal_displacements[glyph_table_index];
       i32 kerning_pair    = next_character ? fontData->horizontal_displacements[glyph_table_index + next_character] : 0;
//	   Assert(kerning_pair == 0);

       characterLocation.x = textX + startX;
       characterLocation.y = textY + startY; 
       characterSize.x     = horizontal_displacement * scale;

       if(c == '\n' || (characterLocation.x + characterSize.x) > endX)
       {
           textY     += characterSize.y; 
		   textSizeX = MAX(textSizeX, textX + characterSize.x);
           textX     = 0;
		   if(c == '\n')
		   {
              continue;
		   }

           characterLocation.x = textX + startX;
           characterLocation.y = textY + startY; 
       }

       textX += (characterSize.x) + kerning_pair * scale;


       font_glyph glyph = fontData->glyphs[glyph_index];
       u32 frame_x = glyph.offsetX;
       u32 frame_y = glyph.offsetY;
       render_draw_sprite_2D(commands,
                           &fontData->texture,
                           characterLocation,
                           characterSize,
                           color,
                           frame_x, frame_y, horizontal_displacement, fontData->font_height);

    }  

	textSizeX = MAX(textSizeX, textX + characterSize.x);
	textSizeY = textY + characterSize.y; 
}

static void
render_text_2d_clamped(render_commands *commands,
		      font_proportional *fontData,
			  f32 startX,
			  f32 startY,
			  f32 endX,
			  f32 endY,
			  f32 scale,
			  u32 start_index,
			  u32 end_index,
			  vec4 color,
			  uint8 *text)
{
	if(start_index > end_index)
	{
		return;
	}
    char c = text[start_index];
    vec2 characterLocation = {0};
    vec2 characterSize     = {0};
    f32 textX     = 0;
	f32 textY     = 0; 
	u32 next_character = 0;
	real32 textSizeX = 0;
	real32 textSizeY = 0; 
	//This never changes
    characterSize.y  = fontData->font_height * scale;

    for(uint32 i = 0;
            (c = text[i]) != '\0' &&
			c != text[end_index];
            i++)
    {
	   next_character = CharToSrc94(text[i]);
       u32 glyph_index= (u32)CharToSrc94(c);
       u32 glyph_table_index = glyph_index * fontData->glyph_count;
       i32 horizontal_displacement = fontData->horizontal_displacements[glyph_table_index];
       i32 kerning_pair = next_character ? fontData->horizontal_displacements[glyph_table_index + next_character] : 0;
//	   Assert(kerning_pair == 0);

       characterLocation.x = textX + startX;
       characterLocation.y = textY + startY; 
       characterSize.x     = horizontal_displacement * scale;

       if(c == '\n')
       {
           textY     += characterSize.y; 
		   textSizeX = MAX(textSizeX, textX + characterSize.x);
           textX     = 0;
		   if(c == '\n')
		   {
              continue;
		   }

           characterLocation.x = textX + startX;
           characterLocation.y = textY + startY; 
       }

       textX += (characterSize.x) + kerning_pair * scale;


       font_glyph glyph = fontData->glyphs[glyph_index];
       u32 frame_x = glyph.offsetX;
       u32 frame_y = glyph.offsetY;
       render_draw_sprite_2D(commands,
                           &fontData->texture,
                           characterLocation,
                           characterSize,
                           color,
                           frame_x, frame_y, horizontal_displacement, fontData->font_height);

    }  

	textSizeX = MAX(textSizeX, textX + characterSize.x);
	textSizeY = textY + characterSize.y; 
}


static inline void
render_refresh_locked_vertices(render_commands *renderCommands)
{
	game_renderer *gameRenderer = renderCommands->gameRenderer;
	gameRenderer->update_locked_vertices       = 1;
	gameRenderer->locked_quads_count        = 0;
	gameRenderer->locked_vertices_group_count           = 0;
	gameRenderer->locked_vertices_in_swap_buffer = 0;
	gameRenderer->quads_locked_from_base = 0;
}
//the next draw calls after this one will be "locked" for the next frames
//the amount drawn will be pushed at the start of the vertex buffer
//and the total amount locked from the base will be used by the renderer
//in order to not completely restore the vertex buffer. This is mostly useful
//for reducing the cpu usage in case of a big amount of draw calls.
static void
render_push_locked_vertices(render_commands *renderCommands)
{
	game_renderer *gameRenderer = renderCommands->gameRenderer;
	gameRenderer->current_locked_vertices_pushed = gameRenderer->locked_vertices_group_count++;

	Assert(gameRenderer->locked_vertices_group_count < gameRenderer->locked_vertices_group_max);
	if(gameRenderer->update_locked_vertices)
	{
	    gameRenderer->lock_next_vertices = 1;

	    render_locked_vertices_group *currentGroup = gameRenderer->locked_vertices_groups + 
			gameRenderer->current_locked_vertices_pushed;
	    
	    currentGroup->offset = gameRenderer->locked_quads_count;
		currentGroup->count  = 0;
	}


	//Tell the renderer which group to read from
    render_command_draw_locked_vertices_data *drawLockedVerticesCommand = render_commands_PushCommand(
			                                                       renderCommands,
		                                                           render_command_type_draw_locked_vertices,
																   render_command_draw_locked_vertices_data);

	//Set the current index
	drawLockedVerticesCommand->groupIndex = gameRenderer->current_locked_vertices_pushed;
}

static void
render_pop_locked_vertices(render_commands *renderCommands)
{
	renderCommands->gameRenderer->lock_next_vertices = 0;
}

static void
render_allocate_locked_vertices(game_renderer *gameRenderer)
{
	/*
	   Make push_pop locked_vertices
	   make a struct to know where to read from
	   reset the allocatedLockedVerticesCount on every lock reset
	   while lock_next_vertices is true, allocate the next draw calls to the locked group
	*/
	gameRenderer->update_locked_vertices = 0;
	if(!gameRenderer->current_vertex_buffer)
	{
		return;
	}
	u32 locked_vertices_in_swap_buffers = gameRenderer->locked_vertices_in_swap_buffer;
	u32 locked_quads_count   = gameRenderer->locked_quads_count;

	if(gameRenderer->locked_vertices_group_count)
	{
	    gameRenderer->quads_locked_from_base = locked_quads_count;
	}
	else
	{
		gameRenderer->quads_locked_from_base = 0;
	}

	if(locked_vertices_in_swap_buffers < render_MAX_SWAP_BUFFERS)
	{
	    u32 vertex_index = 0;
		u32 totalVerticesPushed = locked_quads_count * 4;

	    render_vertex *current_vertex_buffer = gameRenderer->current_vertex_buffer;
	    render_vertex *locked_vertices      = gameRenderer->locked_vertices;
		while(vertex_index < totalVerticesPushed)
	    {
	    	//

	    	current_vertex_buffer[vertex_index + 0] = locked_vertices[vertex_index + 0];
	    	current_vertex_buffer[vertex_index + 1] = locked_vertices[vertex_index + 1];
	    	current_vertex_buffer[vertex_index + 2] = locked_vertices[vertex_index + 2];
	    	current_vertex_buffer[vertex_index + 3] = locked_vertices[vertex_index + 3];

	    	vertex_index += 4;

	    }
			Assert(vertex_index < (gameRenderer->max_quad_draws * 4));

	    gameRenderer->locked_vertices_in_swap_buffer++;
	}
}

static inline vec3
render_offset_to_camera_z(
		game_renderer *renderer,
		vec3 p,
		f32 scale)
{
	vec3 cam_z_scaled = vec3_scale(renderer->camera_z, scale);

	vec3 result = vec3_add(p, cam_z_scaled);
	return(result);
}

inline void
render_scale_vertices_by_uvs_2d(
		render_texture *texture,
		vec2 *v0_ptr,
		vec2 *v1_ptr,
		vec2 *v2_ptr,
		vec2 *v3_ptr,
		vec2 uv0,
		vec2 uv1,
		vec2 uv2,
		vec2 uv3)
{

		vec2 v0 = *v0_ptr;
		vec2 v1 = *v1_ptr;
		vec2 v2 = *v2_ptr;
		vec2 v3 = *v3_ptr;
		//Just a note to remind, the y gets flipped to respect
		//the fact that y > 0 goes "down"
		f32 tw = texture->width * 0.5f;
		f32 th = texture->height * -0.5f;

		vec2 mp = vertices_get_mid_point_2d(v0, v1, v2, v3);

		v0 = mp;
		v0.x = mp.x - ABS(uv3.x - uv0.x) * tw;
		v0.y = mp.y - ABS(uv0.y - uv1.y) * th;

		v1 = mp;
		v1.x = mp.x - ABS(uv2.x - uv1.x) * tw;
		v1.y = mp.y + ABS(uv0.y - uv1.y) * th;

		v2 = mp;
		v2.x = mp.x + ABS(uv2.x - uv1.x) * tw;
		v2.y = mp.y + ABS(uv3.y - uv2.y) * th;

		v3 = mp;
		v3.x = mp.x + ABS(uv3.x - uv0.x) * tw;
		v3.y = mp.y - ABS(uv3.y - uv2.y) * th;

		*v0_ptr = v0;
		*v1_ptr = v1;
		*v2_ptr = v2;
		*v3_ptr = v3;

}
inline vec3
render_mouse_coordinates_to_world(
		game_renderer *gameRenderer,
		vec2 mousePoint,
		real32 distanceFromCam)
{
    matrix4x4 projection    = gameRenderer->projection;
    matrix4x4 projection_inverse = gameRenderer->projection_inverse;

   vec3 camDist = vec3_sub(gameRenderer->camera_position, vec3_f32_mul(gameRenderer->camera_z, distanceFromCam));
   vec4 probeZ  = {camDist.x, camDist.y, 0.0f, 1.0f};
   probeZ = matrix4x4_v4_mul_rows(projection, probeZ);

   //clamp rom {0, - 1} to {-1, 1}
   real32 mouseClipX  = (mousePoint.x / (gameRenderer->back_buffer_width * 0.5f)) - 1.0f;
   real32 mouseClipY  = 1.0f - (mousePoint.y / (gameRenderer->back_buffer_height * 0.5f));
   f32 mouseClipX2  = 2.0f * mousePoint.x / gameRenderer->back_buffer_width - 1.0f;
   f32 mouseClipY2  = 2.0f * mousePoint.y / gameRenderer->back_buffer_height - 1.0f;
   Assert(mouseClipX == mouseClipX2);
   mouseClipX *= probeZ.w;
   mouseClipY *= probeZ.w;

   vec4 mouse4      = {mouseClipX, mouseClipY, probeZ.z, probeZ.w};
   mouse4           = matrix4x4_v4_mul_rows(projection_inverse, mouse4);
   vec3 mouseResult = {mouse4.x, mouse4.y, mouse4.z};

   return(mouseResult);
}
