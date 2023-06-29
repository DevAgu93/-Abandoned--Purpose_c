inline rectangle32s
renderer_calculate_aspect_ratio(uint32 WindowWidth, uint32 WindowHeight, uint32 RenderWidth, uint32 RenderHeight)
{
    real32 OptimalW = (real32)WindowHeight * ((real32)RenderWidth / (real32)RenderHeight);
    real32 OptimalH = (real32)WindowWidth * ((real32)RenderHeight / (real32)RenderWidth);

    rectangle32s result = {0};

    //Hits window width, should add top/bottom borders.
    if(OptimalW > WindowWidth)
    {
        //scale down to render size
        real32 wscale = (real32)RenderHeight / WindowHeight;
        uint32 scaledOptimalH = RoundToInt32(OptimalH * wscale);

        result.x = 0;
        result.y = RoundToInt32((RenderHeight - scaledOptimalH) * 0.5f); 
        result.w = RenderWidth;
        result.h = result.y + scaledOptimalH;

    }else
    {
        real32 hscale = (real32)RenderWidth / WindowWidth; 
        uint32 scaledOptimalW = RoundToInt32(OptimalW * hscale);

        result.x = RoundToInt32((RenderWidth - scaledOptimalW) * 0.5f); 
        result.y = 0; 
        result.w = result.x + scaledOptimalW; 
        result.h = RenderHeight; 
    }
    return result;
}

inline rectangle32s
renderer_calculate_aspect_ratio2(uint32 WindowWidth, uint32 WindowHeight, uint32 RenderWidth, uint32 RenderHeight)
{
    real32 OptimalW = (real32)WindowHeight * ((real32)RenderWidth / (real32)RenderHeight);
    real32 OptimalH = (real32)WindowWidth * ((real32)RenderHeight / (real32)RenderWidth);

    rectangle32s result = {0};

    //Hits window width, should add top/bottom borders.
    if(OptimalW > RenderWidth)
    {
        //scale down to render size
        real32 wscale = (real32)RenderHeight / WindowHeight;
        uint32 scaledOptimalH = RoundToInt32(OptimalH * wscale);

        result.x = 0;
        result.y = RoundToInt32((scaledOptimalH - RenderHeight) * 0.5f); 
        result.w = RenderWidth;
        result.h = result.y + scaledOptimalH;

    }else
    {
        real32 hscale = (real32)RenderWidth / WindowWidth; 
        uint32 scaledOptimalW = RoundToInt32(OptimalW * hscale);

        result.x = RoundToInt32((RenderWidth - scaledOptimalW) * 0.5f); 
        result.y = 0; 
        result.w = result.x + scaledOptimalW; 
        result.h = RenderHeight; 
    }
    return result;
}

static vec2
renderer_convert_to_clip_coordinates(
		game_renderer *game_renderer,
		u32 screen_w,
		u32 screen_h,
		f32 point_x,
		f32 point_y)
{

	//first scale with the framebuffer.
    f32 scaleDifference_x  = (f32)game_renderer->back_buffer_width  / screen_w;
    f32 scaleDifference_y  = (f32)game_renderer->back_buffer_height / screen_h;
	//Scale to clipped coordinates
    f32 aspectRatioScale_x = (f32)game_renderer->back_buffer_width  / (game_renderer->game_draw_clip.x1 - game_renderer->game_draw_clip.x0); 
    f32 aspectRatioScale_y = (f32)game_renderer->back_buffer_height / (game_renderer->game_draw_clip.y1 - game_renderer->game_draw_clip.y0); 

    f32 scaledx = point_x * scaleDifference_x;
    f32 scaledy = point_y * scaleDifference_y;

	//Draw clip x and y are the black border sizes.
    point_x = (scaledx - game_renderer->game_draw_clip.x) * aspectRatioScale_x;
    point_y = (scaledy - game_renderer->game_draw_clip.y) * aspectRatioScale_y;

    vec2 result = {point_x, point_y};
	return(result);
}

static render_texture
renderer_allocate_white_texture(game_renderer *game_renderer,
                              platform_renderer *platformRenderer,
                              memory_area *area)
{
	temporary_area t_area = temporary_area_begin(area);

   u32 texture_array_w = game_renderer->texture_array_w;
   u32 texture_array_h = game_renderer->texture_array_h;
   u32 image_size = texture_array_w * texture_array_h * 4;
   u32 *whitemem = (u32 *)memory_area_push_size(area, image_size);
   for(u32 i = 0; i < image_size; i++)
   {
       whitemem[i] = 0xffffffff;
   }
   platformRenderer->f_push_texture(
		   platformRenderer,
		   (u8 *)whitemem,
		   texture_array_w,
		   texture_array_h,
		   4,
		   0);

  temporary_area_end(&t_area);

  render_texture renderTexture = {0};
  renderTexture.index = 0;
  renderTexture.offsetX = 0;
  renderTexture.offsetX = 0;
  renderTexture.width = 512;
  renderTexture.height = 512;

  return(renderTexture);
}


static void 
renderer_allocate_image_from_request(game_renderer *game_renderer,
                                     platform_renderer *platformRenderer,
                                     texture_request *texture_request)
{
	//in case of supporting larger textures, I must detect if it's going
	//to occuppy more that one slot, where in such case I have to
	//look for two consecutive free slots instead
   render_texture result      = {0};

   u32 imageW = game_renderer->texture_array_w; 
   u32 imageH = game_renderer->texture_array_h;


   platformRenderer->f_push_texture(
		   platformRenderer,
		   texture_request->pixels,
		   imageW,
		   imageH,
		   4,
		   texture_request->avadible_texture_index);


   texture_request->state = request_open;

   //the file handle from the request points to a file on disk, so close.
}

static void
renderer_check_requested_textures(platform_renderer *platformRenderer,
                                game_renderer *game_renderer)
{
	//Check valid pointer to texture
	render_texture_data *tops = &game_renderer->texture_operations;
	texture_request *request = tops->first_texture_transfer;
    while(request)
    {
		//Point where to locate the asset
       renderer_allocate_image_from_request(game_renderer,
                                  platformRenderer,
                                  request);

	   //cache the next open request
	   texture_request *next = request->next;
	   //free the current one
	   render_free_texture_request(
			   tops, request);

	   //next request
	   request = next;
	   tops->first_texture_transfer = request;
    }
}

static void 
renderer_draw_end(platform_renderer *renderer, game_renderer *game_renderer)
{ 
	//The renderer uses this
	game_renderer->scissor_stack_index = 0;
    
	renderer->f_draw_start(renderer, game_renderer);
	render_allocate_locked_vertices(game_renderer);
	renderer->f_draw_end(renderer, game_renderer);

	game_renderer->locked_vertices_group_count = 0;

    game_renderer->render_commands_buffer_offset = game_renderer->render_commands_buffer_base;
    game_renderer->draw_count         = 0;
    game_renderer->begin_count        = 0;
	game_renderer->depth_peel_calls    = 0;
	game_renderer->scissor_push_count  = 0;
	game_renderer->scissors_on_stack = 0;
	//game_renderer->current_draw_clip = game_renderer->game_draw_clip;
    //Clear backgorund to black
}
static void 
renderer_display_buffer(platform_renderer *renderer, game_renderer *game_renderer)
{
	renderer->f_swap_buffers(renderer, game_renderer);
}
