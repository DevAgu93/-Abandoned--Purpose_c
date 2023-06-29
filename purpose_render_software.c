#include <windows.h>

#include "purpose_crt.h"
#include <global_definitions.h>
#include <gmmath.h>
#include <gm_memory.h>
#include <gm_render.h>
#include "purpose_image.c"

typedef struct{
    platform_renderer header;
	u8 *back_buffer;
}software_renderer_device;

software_renderer_device *
init_software_render(memory_area *area,
        HWND windowhand,
		platform_renderer_init_values initial_values,
		platform_renderer_init_functions *init_only_functions)
{
	u32 texture_array_w = initial_values.texture_array_w;
	u32 texture_array_h = initial_values.texture_array_h;
	u32 texture_capacity = initial_values.texture_array_capacity;
	u32 maximum_quad_size = initial_values.maximum_quad_size;
	u32 back_buffer_width = initial_values.back_buffer_width;
	u32 back_buffer_height = initial_values.back_buffer_height;

    software_renderer_device *sf_device = memory_area_push_struct(area, software_renderer_device);
	sf_device->back_buffer = memory_area_push_size(area, back_buffer_width * back_buffer_height * 4);

	return(sf_device);
}


platform_renderer *
win32_load_renderer(memory_area *area,
        HWND windowhand,
		platform_renderer_init_values initial_values,
		platform_renderer_init_functions *init_functions)
{
	return((platform_renderer *)init_software_render(
				area,
				windowhand,
				initial_values,
				init_functions));
}

int _DllMainCRTStartup()
{
	return(1);
}
