#define WIN32_LOAD_RENDERER(name) platform_renderer *name(memory_area *area, HWND hwnd, platform_renderer_init_values init_values, platform_renderer_init_functions *init_functions)
typedef WIN32_LOAD_RENDERER(win32_load_renderer);

static win32_load_renderer *
win32_load_renderer_dll(u8 *file_name)
{

	HMODULE renderer_dll = LoadLibrary(file_name);
	win32_load_renderer *result = (win32_load_renderer *)GetProcAddress(
			renderer_dll, "win32_load_renderer");

	return(result);
}

static platform_renderer * 
win32_allocate_graphics(memory_area *area,
                 HWND hwnd,
                 game_renderer *game_renderer,
                 render_api type)
{
    platform_renderer *renderer = 0;
    uint32 texture_array_w = game_renderer->texture_array_w;
    uint32 texture_array_h = game_renderer->texture_array_h;
    uint32 texture_array_capacity = game_renderer->texture_array_capacity;
    uint32 maximum_quad_size = game_renderer->max_quad_draws * (sizeof(render_vertex) * 4);
    uint32 back_buffer_width = game_renderer->back_buffer_width;
    uint32 back_buffer_height = game_renderer->back_buffer_height;

	if(!game_renderer->render_commands_buffer_size || game_renderer->render_commands_buffer_size < MEGABYTES(2))
	{
		game_renderer->render_commands_buffer_size = MEGABYTES(2);
	}



	game_renderer->camera_zoom = 1.0f;
	game_renderer->locked_quads_max = 50000 ;
	u32 totalQuadsAllocated = 50000 * 4;
	game_renderer->locked_vertices = memory_area_push_array(area,
			                                             render_vertex,
														 totalQuadsAllocated);

	game_renderer->locked_vertices_group_max   = 60;
	game_renderer->locked_vertices_groups = memory_area_push_array(area,
			                                               render_locked_vertices_group,
														   game_renderer->locked_vertices_group_max);

	game_renderer->texture_operations = renderer_allocate_texture_requests(
			game_renderer, area);

	platform_renderer_init_values initial_values = {0};
	initial_values.texture_array_w = texture_array_w;
	initial_values.texture_array_h = texture_array_h;
	initial_values.texture_array_capacity = texture_array_capacity;
	initial_values.maximum_quad_size = maximum_quad_size;
	initial_values.back_buffer_width = back_buffer_width;
	initial_values.back_buffer_height = back_buffer_height;
	initial_values.display_buffers_count = 1;
	initial_values.min_filter = filter_linear;
	initial_values.max_filter = filter_linear;
	initial_values.mip_filter = filter_linear;
	//initial_values.min_filter = filter_bilineal;
	//initial_values.max_filter = filter_bilineal;
	//initial_values.mip_filter = filter_bilineal;

	//default to d3d11 for now
#if 1
	platform_renderer_init_functions init_only_functions = {0};
	u8 *renderers[] = {
	"sw_render.dll",
	"direct3d11.dll"};
    u8 *renderer_dll = renderers[type == render_software ? 0 : 1];
	game_renderer->api = type;
	win32_load_renderer *f_load_renderer = win32_load_renderer_dll(renderer_dll);
	Assert(f_load_renderer);
	renderer = f_load_renderer(
			area, hwnd, initial_values, &init_only_functions);

#else
	renderer = d3d_init(
			area, hwnd, initial_values);
#endif

    return renderer;
}

