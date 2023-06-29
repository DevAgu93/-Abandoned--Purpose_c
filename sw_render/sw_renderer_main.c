#include <windows.h>

#include "..\purpose_crt.h"
#include "..\global_definitions.h"
#include "..\purpose_math.h"
#include "..\purpose_memory.h"
#include "..\purpose_global.h"
#include "..\purpose_render.h"
#include "sw_renderer.h"


//#define software_renderer_texture_pixels(sw, index) (sw->texture_buffer + (sw->texture_array_width * sw->texture_array_height * 4 * index));
RENDER_SWAP_BUFFERS(sw_swap_buffers);
RENDER_DRAW_START(sw_draw_start);
RENDER_DRAW_END(sw_draw_end);
RENDER_PUSH_TEXTURE(sw_push_texture);
software_renderer_device *
init_software_render(memory_area *area, HWND windowhand, platform_renderer_init_values initial_values, platform_renderer_init_functions *init_only_functions);
inline vec4
sw_rgba_unpack(uint32 pack);
static f32
edge_function(vec2 p,vec2 v0, vec2 v1);
static vec3
get_barycentric_coordinates(vec2 p, vec2 v0, vec2 v1, vec2 v2);

static vec3
get_barycentric_coordinates(vec2 p, vec2 v0, vec2 v1, vec2 v2)
{
	f32 area = edge_function(v2, v1, v0);
	//area made by v1, v0, p, increases as it goes to v2
	f32 a01 = edge_function(p, v1, v0) / area;
	//area made by v2, v1, p, increases as it goes to v0
	f32 a12 = edge_function(p, v2, v1) / area;
	//area made by v0, v2, p, increases as it goes to v1
	f32 a20 = edge_function(p, v0, v2) / area;

	vec3 result = {a01, a12, a20};
	return(result);

}

static f32
edge_function(vec2 p,vec2 v0, vec2 v1)
{
	f32 result = (p.x - v0.x) * (v1.y - v0.y) - (p.y - v0.y) * (v1.x - v0.x);
	return(result);
}

inline vec4
sw_rgba_unpack(uint32 pack)
{
    vec4 result = {
        (real32)((pack >> 0) & 0xff),
        (real32)((pack >> 8) & 0xff),
        (real32)((pack >> 16) & 0xff),
        (real32)((pack >> 24) & 0xff)
    };
    return(result);
}
static void
sw_read_render_commands(software_renderer_device *sw_device,
                game_renderer *game_renderer,
                render_commands *render_commands);
static u8 *
software_renderer_texture_pixels(software_renderer_device *sw_device, u32 index);

static u8 *
software_renderer_texture_pixels(software_renderer_device *sw, u32 index)
{
	u8 *result = (sw->texture_buffer + (sw->texture_array_width * sw->texture_array_height * 4 * index));
	return(result);
}

static void
sw_read_render_commands(software_renderer_device *sw_device,
                game_renderer *game_renderer,
                render_commands *render_commands)
#if 1
{

	u32 errorTestIndex = 0;
	u8 *command = render_commands->commands_base;
	rectangle32s currentClip = {0};
	while(command != render_commands->commands_offset)
	{
		errorTestIndex++;
		render_command_type *renderheader = (render_command_type *)command;
		//Get the data here.
		u32 offset = 0;
		switch(*renderheader)
		{
			case render_command_type_clear:
				{
					render_command_clear *data = (render_command_clear *)command;
					offset = sizeof(render_command_clear);
					//d3d_clear(directDevice, game_renderer->clear_color);
					//Clear screen
				}break;
			case render_command_type_drawquad:
				{
					render_command_drawquad *data = (render_command_drawquad *)command;
					offset = sizeof(render_command_drawquad);
					//DrawQuad
					render_vertex *vertices = data->vertices;

					vec3 v0 = vertices[0].location;
					vec3 v1 = vertices[1].location;
					vec3 v2 = vertices[2].location;
					vec3 v3 = vertices[3].location;

					vec2 uv0 = vertices[0].uv;
					vec2 uv1 = vertices[1].uv;
					vec2 uv2 = vertices[2].uv;
					vec2 uv3 = vertices[3].uv;
					//scale vertices for now
					f32 bbw = game_renderer->back_buffer_width * .5f;
					f32 bbh = game_renderer->back_buffer_height * .5f;
					f32 bbwc = game_renderer->back_buffer_width;
					f32 bbhc = game_renderer->back_buffer_height;
					{
						vec4 v0_4 = V4(v0.x, v0.y, v0.z, 1);
						vec4 v1_4 = V4(v1.x, v1.y, v1.z, 1);
						vec4 v2_4 = V4(v2.x, v2.y, v2.z, 1);
						vec4 v3_4 = V4(v3.x, v3.y, v3.z, 1);

						/*Nota: Para 3d la cámara para que mire hacia abajo
						  debe ser rotado por 0.5 en x.
						*/
						matrix4x4 wvp = game_renderer->projection;
//						v1_4 = matrix4x4_v4_mul_cols(v1_4,wvp);
//						v2_4 = matrix4x4_v4_mul_cols(v2_4,wvp);
//						v0_4 = matrix4x4_v4_mul_cols(v0_4,wvp);
//						v3_4 = matrix4x4_v4_mul_cols(v3_4,wvp);

						v1_4 = matrix4x4_v4_mul_rows(wvp,v1_4);
						v2_4 = matrix4x4_v4_mul_rows(wvp,v2_4);
						v0_4 = matrix4x4_v4_mul_rows(wvp,v0_4);
						v3_4 = matrix4x4_v4_mul_rows(wvp,v3_4);

						v1_4 = vec4_f32_div(v1_4, v1_4.w);
						v2_4 = vec4_f32_div(v2_4, v2_4.w);
						v0_4 = vec4_f32_div(v0_4, v0_4.w);
						v3_4 = vec4_f32_div(v3_4, v3_4.w);
					}


					v0.x = v0.x * bbw + bbw;
					v1.x = v1.x * bbw + bbw;
					v2.x = v2.x * bbw + bbw;
					v3.x = v3.x * bbw + bbw;

					f32 y_bias_ppct = 0.0f;
					v0.y = (1.0f - v0.y) * bbh - (bbh * y_bias_ppct);
					v1.y = (1.0f - v1.y) * bbh - (bbh * y_bias_ppct);
					v2.y = (1.0f - v2.y) * bbh - (bbh * y_bias_ppct);
					v3.y = (1.0f - v3.y) * bbh - (bbh * y_bias_ppct);

					



					//scale uvs
					uv0.x *= sw_device->texture_array_width;
					uv0.y *= sw_device->texture_array_height;

					uv1.x *= sw_device->texture_array_width;
					uv1.y *= sw_device->texture_array_height;

					uv2.x *= sw_device->texture_array_width;
					uv2.y *= sw_device->texture_array_height;

					uv3.x *= sw_device->texture_array_width;
					uv3.y *= sw_device->texture_array_height;

					//find the minimum x and the maximum x

					f32 x_min = v0.x;
					f32 y_min = v0.y;
					f32 x_max = v0.x;
					f32 y_max = v0.y;
					//find the bounding box of this quad
					vec2 v0_2 = {v0.x, v0.y};
					vec2 v1_2 = {v1.x, v1.y};
					vec2 v2_2 = {v2.x, v2.y};
					vec2 v3_2 = {v3.x, v3.y};

					vec2 vs[] = {v0_2, v1_2, v2_2, v3_2};
					for(u32 m = 1; m < 4; m++)
					{
						//TODO: convert to 2d coordiantes
						vec2 vec = vs[m];
						x_min = vec.x < x_min ? vec.x : x_min;
						x_max = vec.x > x_max ? vec.x : x_max;

						y_min = vec.y < y_min ? vec.y : y_min;
						y_max = vec.y > y_max ? vec.y : y_max;
					}

					i32 x0 = (i32)x_min;
					i32 x1 = (i32)x_max;
					i32 y0 = (i32)y_min;
					i32 y1 = (i32)y_max;

					//i32 x0 = (i32)v0.x;
					//i32 x1 = (i32)v3.x;
					//i32 y0 = (i32)v1.y;
					//i32 y1 = (i32)v0.y;

					if(x0 < 0)
					{
						x0 = 0;
					}
					if(x1 > game_renderer->back_buffer_width)
					{
						i32 sub = x1 - game_renderer->back_buffer_width;
						x1 = game_renderer->back_buffer_width;
					}
					if(y0 < 0)
					{
						y0 = 0;
					}
					if(y1 > game_renderer->back_buffer_height)
					{
						i32 sub = y1 - game_renderer->back_buffer_height;
						y1 = game_renderer->back_buffer_height;
					}
					//unpack color
					vec4 color = sw_rgba_unpack(data->vertices[0].color);
					vec4 color0 = sw_rgba_unpack(data->vertices[0].color);
					vec4 color1 = sw_rgba_unpack(data->vertices[1].color);
					vec4 color2 = sw_rgba_unpack(data->vertices[2].color);
					vec4 color3 = sw_rgba_unpack(data->vertices[3].color);

					color0.r /= 255;
					color0.g /= 255;
					color0.b /= 255;
					color0.a /= 255;

					color1.r /= 255;
					color1.g /= 255;
					color1.b /= 255;
					color1.a /= 255;

					color2.r /= 255;
					color2.g /= 255;
					color2.b /= 255;
					color2.a /= 255;

					color3.r /= 255;
					color3.g /= 255;
					color3.b /= 255;
					color3.a /= 255;

					color.r /= 255;
					color.g /= 255;
					color.b /= 255;
					color.a /= 255;
					//pick texture pixels
					u8 *t_pixels = software_renderer_texture_pixels(sw_device, data->vertices[0].texture); 
					u8 *t_pixels_start = t_pixels;

					u32 pitch = game_renderer->back_buffer_width * 4;
					i32 distance_x_total = x1 - x0;
					f32 c_inv = 1.0f / 255;
					//texture coordinates
					f32 tx0 = uv0.x;
					f32 tx1 = uv3.x;
					f32 ty0 = uv0.y;
					f32 ty1 = uv1.y;

					//numero de pasos que los texels avanzan
					f32 x_uv_advance = (tx1 - tx0) / (x1 - x0);
					f32 y_uv_advance = ABS((ty1 - ty0) / (y1 - y0));
					f32 tx = tx0;
					f32 ty = ty1;

					//vertices colors
					vec4 vcolor0 = sw_rgba_unpack(data->vertices[0].color);
					vcolor0 = vec4_f32_div(vcolor0, 255);
					vec4 vcolor1 = sw_rgba_unpack(data->vertices[1].color);
					vcolor1 = vec4_f32_div(vcolor1, 255);
					vec4 vcolor2 = sw_rgba_unpack(data->vertices[2].color);
					vcolor2 = vec4_f32_div(vcolor2, 255);
					vec4 vcolor3 = sw_rgba_unpack(data->vertices[3].color);
					vcolor3 = vec4_f32_div(vcolor3, 255);
					for(i32 y = y0; y < y1; y++)
					{
						u8 *pixels_at = sw_device->back_buffer + ((x0 * 4) + y * pitch);
						i32 distance_y = y1 - y;
						for(i32 x = x0; x < x1; x++)
						{
							//check if the current pixels is inside the quad
							vec2 p = {x, y};
							vec3 bcoords012 = get_barycentric_coordinates(p, v0_2, v1_2, v2_2);
							vec3 bcoords013 = get_barycentric_coordinates(p, v2_2, v3_2, v0_2);
//							f32 i10 = edge_function(p, v1_2, v0_2);
//							f32 i21 = edge_function(p, v2_2, v1_2);
//							f32 i03 = edge_function(p, v0_2, v3_2);
//							f32 i32 = edge_function(p, v3_2, v2_2);
//							b32 p_inside = i10 >= 0 && i21 >= 0 && i03 >= 0 && i32 >= 0;
							b32 p_inside0 = bcoords012.x * bcoords012.y * bcoords012.z >= 0;
							b32 p_inside1 = bcoords013.x * bcoords013.y * bcoords013.z >= 0;
							if(p_inside0 || p_inside1)
							{
							f32 distance_x = (f32)(x1 - x) / (f32)distance_x_total;;

							vec4 p0 = sw_rgba_unpack(*(u32 *)t_pixels);
							vec4 p1 = sw_rgba_unpack(*(u32 *)pixels_at);
							p0 = vec4_scale(p0, c_inv);
							p1 = vec4_scale(p1, c_inv);

							f32 r0 = p0.b;
							f32 g0 = p0.g;
							f32 b0 = p0.r;
							//blend
							//colors are inverted!
#if 1
							//conventional alpha, not pre-multiplied.
							f32 sa = p0.w;
							f32 da = p1.w;
							f32 isa = 1.0f - sa;
							p0.r = (r0 * sa) + (p1.r * isa);
							p0.g = (g0 * sa) + (p1.g * isa);
							p0.b = (b0 * sa) + (p1.b * isa);

							//imitando el shader
							vec4 vcolor = vcolor0;

							p0.r *= vcolor.b;
							p0.g *= vcolor.g;
							p0.b *= vcolor.r;

							u8 rc = (u8)((p0.r) * 255);
							u8 gc = (u8)((p0.g) * 255);
							u8 bc = (u8)((p0.b) * 255);
							u8 ra = (u8)((isa * da + sa) * 255);

#elif 0
							//Alfa solo oscurese lo que tiene detras
							p0.r = (r0) + (p1.r * (1.0f - p0.w));
							p0.g = (g0) + (p1.g * (1.0f - p0.w));
							p0.b = (b0) + (p1.b * (1.0f - p0.w));

							u8 rc = (u8)((p0.r) * 255);
							u8 gc = (u8)((p0.g) * 255);
							u8 bc = (u8)((p0.b) * 255);
#else
							f32 ial = (1.0f - p1.w) * p0.w + p1.w;
							f32 al = (1.0f - p1.w) * p0.w + p1.w;
							p0.r = (r0) + (p1.r * (1.0f - al));
							p0.g = (g0) + (p1.g * (1.0f - al));
							p0.b = (b0) + (p1.b * (1.0f - al));

							u8 rc = (u8)((p0.r) * 255);
							u8 gc = (u8)((p0.g) * 255);
							u8 bc = (u8)((p0.b) * 255);
#endif

							//texel_color.r = f32_lerp(color0.r, distance_x, color3.r) * 255;
							//texel_color.g = f32_lerp(color0.g, distance_x, color3.g) * 255;
							//texel_color.b = f32_lerp(color0.b, distance_x, color3.b) * 255;
							//texel_color.a = f32_lerp(color0.a, distance_x, color3.a) * 255;
							u32 color = (rc << 0) | (gc << 8) | (bc << 16) | (0xff << ra);
							*(u32 *)pixels_at = color;

							}

							u32 t_index = (u32)(tx + ((u32)ty * sw_device->texture_array_width));
							t_pixels = t_pixels_start + (4 * t_index);
							pixels_at += 4;
							tx += x_uv_advance;
							//TODO: handle tx going outside bounds
						}
						ty += y_uv_advance;
						tx = tx0;
						//t_pixels++;
					}
//					d3d11_push_quad(game_renderer, data->vertices, batch->pushedQuadsToBuffer); 

				}break;

			case render_command_type_PushClip:
				{
					render_command_SetClip *data = (render_command_SetClip *)command;
					offset = sizeof(render_command_SetClip);

					//Get the first scissor from the stack
					//render_scissor *scissorPushed = game_renderer->scissor_stack + batch->scissor_push_count;
					//i32 cX0 = scissorPushed->clip.x0;
					//i32 cY0 = scissorPushed->clip.y0;
					//i32 cX1 = scissorPushed->clip.x1;
					//i32 cY1 = scissorPushed->clip.y1;

					//batch->scissor_current = batch->scissor_push_count;
					//batch->scissors_on_stack++;
					//batch->scissor_push_count++;

					////;Cleanup
					//u32 tempScissorStackCount = game_renderer->scissor_total_count; 
					//Assert(batch->scissors_on_stack < tempScissorStackCount);
					////
					////Draw current quads before switching clip.
					////
					//d3d11_draw_indexed(directDevice, batch);
					//d3d_SetClip(directDevice, cX0, cY0, cX1, cY1);

					//currentClip.x0 = cX0;
					//currentClip.y0 = cY0;
					//currentClip.x1 = cX1;
					//currentClip.y1 = cY1;
					////DrawQuad

				}break;
			case render_command_type_PopClip:
				{
					offset = sizeof(render_command_PopClip);
					//batch->scissors_on_stack--;

					//u32 tempScissorStackCount = game_renderer->scissor_total_count; 
					//Assert(batch->scissors_on_stack < tempScissorStackCount);
					////
					////Restore clip
					////Pop stack?
					//i32 cX0 = 0; 
					//i32 cY0 = 0; 
					//i32 cX1 = 0; 
					//i32 cY1 = 0; 

					//render_scissor *scissor_stack = game_renderer->scissor_stack;
					//if(batch->scissors_on_stack > 0)
					//{
					//	batch->scissor_current = scissor_stack[batch->scissor_current].previous;
					//	u32 sI = batch->scissor_current;
					//	cX0 = scissor_stack[sI].clip.x;
					//	cY0 = scissor_stack[sI].clip.y;
					//	//These where already clipadded before.
					//	cX1 = scissor_stack[sI].clip.w;
					//	cY1 = scissor_stack[sI].clip.h;


					//}
					//else
					//{
					//	cX0 = batch->current_draw_clip.x; 
					//	cY0 = batch->current_draw_clip.y;
					//	cX1 = batch->current_draw_clip.w;
					//	cY1 = batch->current_draw_clip.h;
					//}
					//d3d11_draw_indexed(directDevice, batch);
					//d3d_SetClip(directDevice, cX0, cY0, cX1, cY1);

					//currentClip.x0 = cX0;
					//currentClip.y0 = cY0;
					//currentClip.x1 = cX1;
					//currentClip.y1 = cY1;


				}break;
			case render_command_type_draw_locked_vertices:
				{
					offset = sizeof(render_command_draw_locked_vertices_data);
					//render_command_draw_locked_vertices_data *data = (render_command_draw_locked_vertices_data *)command;

					//u32 lockGroupIndex = data->groupIndex;

					//Assert(lockGroupIndex < game_renderer->locked_vertices_group_count);

					//render_locked_vertices_group *lockedGroup = game_renderer->locked_vertices_groups + data->groupIndex;


					//if(!processingPeeling)
					//{
					//	//Draw previous vertices (+1 extra draw call)
					//	d3d11_draw_indexed(directDevice, batch);

					//	d3d_draw_indexed_offset(directDevice, lockedGroup->offset, lockedGroup->count);
					//}
					//else
					//{
					//	batch->reservedLockedGroups[batch->reservedLockedGroupsCount++] = lockGroupIndex;
					//}
				}break;
			default:
				{
					Assert(0);
				}
		}
		command += offset;
		offset = 0;
	}
	Assert(errorTestIndex == render_commands->command_count);
	//TODO:Get data and header in order to execute the given command.
}
#endif




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
	sf_device->back_buffer = memory_area_clear_and_push(area, back_buffer_width * back_buffer_height * 4);
	sf_device->header.f_draw_start = sw_draw_start;
	sf_device->header.f_draw_end = sw_draw_end;
	sf_device->header.f_swap_buffers = sw_swap_buffers;
	sf_device->header.f_push_texture = sw_push_texture;
	sf_device->texture_array_width = texture_array_w;
	sf_device->texture_array_height = texture_array_h;

	sf_device->texture_max = texture_capacity;
	memory_size texture_buffer_size = texture_array_w * texture_array_h * 4 * sf_device->texture_max;
	sf_device->texture_buffer = memory_area_clear_and_push(area, texture_buffer_size);

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

RENDER_PUSH_TEXTURE(sw_push_texture)
{
	software_renderer_device *sw_device = (software_renderer_device *)r;

    render_texture result = {0};
    result.width = w;
    result.height = h;
    result.index = index;
    
//    d3d11_push_texture_image(directDevice, &TEMPimageData, index);
	u32 tw = sw_device->texture_array_width;
	u32 th = sw_device->texture_array_height;
	u8 *t_buffer = sw_device->texture_buffer + (index * tw * th * 4);
	u8 *pixels1 = pixels;
	u32 pitch = tw * 4;
	for(u32 y = 0; y < h; y++)
	{
		u8 *t_buffer_at = t_buffer;
		u8 *pixels_at = pixels1;
		for(u32 x = 0; x < w; x++)
		{
			t_buffer_at[0] = pixels_at[0];
			t_buffer_at[1] = pixels_at[1];
			t_buffer_at[2] = pixels_at[2];
			t_buffer_at[3] = pixels_at[3];
			t_buffer_at += 4;
			pixels_at += 4;
		}
		t_buffer += pitch;
		pixels1 += pitch;
	}
	sw_device->texture_count++;


    return(result);
}

RENDER_SWAP_BUFFERS(sw_swap_buffers)
{
	//d3d11_swap_buffers(r, game_renderer);
}

RENDER_DRAW_START(sw_draw_start)
{
	//d3d11_draw_start(r, game_renderer);
}

RENDER_DRAW_END(sw_draw_end)
{ 
	software_renderer_device *sw_device = (software_renderer_device *)r;

	u8 *buffer = sw_device->back_buffer;
	u32 pitch = game_renderer->back_buffer_width * 4;
	u8 rgba[4] = {
		(u8)(game_renderer->clear_color[2] * 255),
		(u8)(game_renderer->clear_color[1] * 255),
		(u8)(game_renderer->clear_color[0] * 255),
		(u8)(game_renderer->clear_color[3] * 255),
	};
	//clear
	for(u32 y = 0; y < game_renderer->back_buffer_height; y++)
	{
		u8 *buffer8 = buffer;
		for(u32 x = 0; x < game_renderer->back_buffer_width; x++)
		{
//			*buffer8 = 0xff0000ff;
			buffer8[0] = rgba[0];
			buffer8[1] = rgba[1];
			buffer8[2] = rgba[2];
			buffer8[3] = rgba[3];

			buffer8 += 4;
		}
		buffer += pitch;
	}

   u32 processing_render_commands = game_renderer->begin_count > 0;
   render_commands *next_render_commands = 
	   (render_commands *)game_renderer->render_commands_buffer_base;
   u32 command_index = 0;

   //read render commands
   while(processing_render_commands)
   {
       render_commands *current_processing_commands = next_render_commands;
	   next_render_commands = (render_commands *)current_processing_commands->commands_offset;

	   command_index++;
	   processing_render_commands = (command_index < game_renderer->begin_count);

	   sw_read_render_commands(sw_device, game_renderer, current_processing_commands);
   }
	//d3d11_draw_end(r, game_renderer);
	//scissors
//	i32 c0 = game_renderer->game_draw_clip.x;
//	i32 c1 = game_renderer->game_draw_clip.x;
//	i32 c2 = game_renderer->game_draw_clip.x;
//	i32 c3 = game_renderer->game_draw_clip.x;
//	//viewport
//	i32 v0 = c0;
//	i32 v1 = c1;
//	i32 v2 = c2;
//	i32 v3 = c3;
//
//	BITMAPINFO bm_info = {0};
//	BITMAPINFOHEADER *bm_header = &bm_info.bmiHeader;
//	bm_header->biSize = sizeof(bm_info);
//	bm_header->biWidth = game_renderer->back_buffer_width;
//	StretchDIBits(sw_device->device_context,
//			c0, c1, c2, c3,
//			v0, v1, v2, v3,
//			sw_device->back_buffer,
//			&bm_info,
//			DIB_RGB_COLORS,
//			SRCCOPY);
}


int _DllMainCRTStartup()
{
	return(1);
}
