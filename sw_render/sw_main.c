#include "..\purpose_crt.h"

#include "..\global_definitions.h"
#include <agu_random.h>
#include "..\purpose_math.h"
#include "..\global_all_use.h"
#include "..\purpose_memory.h"
#include "..\purpose_stream.h"
#include "..\purpose_platform.h"
#include "..\purpose_global.h"
#include "..\purpose_render.h"
#include "..\purpose_render.c"
#include "..\purpose_console.h"
#include "..\purpose_ui.h"
#include "..\purpose_ui.c"
#include "..\agu_timer.h"

#include "sw_platform_layer.h"


//#include "purpose_files.c"
#include "..\purpose_all.h"
#include "..\purpose_entity_states.h"
#include "..\purpose_assets.h"
#include "..\purpose_assets.c"
#include "..\purpose_ray.c"

#include "sw.h"

static void
sw_render_push_quad_2d(
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
   v1_2d.x = v1_2d.x * screenClipHalfW - 1.0f;
   v2_2d.x = v2_2d.x * screenClipHalfW - 1.0f;
   v3_2d.x = v3_2d.x * screenClipHalfW - 1.0f;

   //subtract 1 to invert the y coordinate
   v0_2d.y = 1.0f - v0_2d.y * screenClipHalfH;
   v1_2d.y = 1.0f - v1_2d.y * screenClipHalfH + 0.0f;
   v2_2d.y = 1.0f - v2_2d.y * screenClipHalfH + 0.0f;
   v3_2d.y = 1.0f - v3_2d.y * screenClipHalfH + 0.0f;

   
   vec3 v0 = {v0_2d.x, v0_2d.y, 0}; 
   vec3 v1 = {v1_2d.x, v1_2d.y, 0};
   vec3 v2 = {v2_2d.x, v2_2d.y, 0}; 
   vec3 v3 = {v3_2d.x, v3_2d.y, 0}; 


   render_push_quad(commands, texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color);

}

static void
sw_draw_sprite(program_state *program,
		render_commands *commands,
		rectangle32u fxywh,
		f32 w,
		f32 h,
		render_texture *sprite_texture,
		vec4 color)
{
	f32 x = 20;
	u32 fx = fxywh.x;
	u32 fy = fxywh.y;
	u32 fw = fxywh.w;
	u32 fh = fxywh.h;
	render_uvs uvs = render_frames_to_uv(512, 512, fx, fy, fw, fh);
	vec2 uv0 = uvs.uv0;
	vec2 uv1 = uvs.uv1;
	vec2 uv2 = uvs.uv2;
	vec2 uv3 = uvs.uv3;

	vec2 v0 = {0};
	vec2 v1 = {0};
	vec2 v2 = {0};
	vec2 v3 = {0};

	vertices2 vs = adjust_vertices_to_uvs_2d(v0, v1, v2, v3, uv0, uv1, uv2, uv3);
	v0 = vs.v0;
	v1 = vs.v1;
	v2 = vs.v2;
	v3 = vs.v3;

	v0.x = 0;
	v1.x = 0;
	v2.x = w;
	v3.x = w;

	v0.y = h;
	v1.y = 0;
	v2.y = 0;
	v3.y = h;


	//render_push_sprite(commands, sprite_texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3);
	sw_render_push_quad_2d(commands, sprite_texture, v0, v1, v2, v3, uv0, uv1, uv2, uv3, color);
}


program_PROGRESS(program_progress)
{

	memory_area *main_memory_area = area->main_memory;
	program_state *program = area->program_state;

	if(area->dll_reloaded)
	{
//		g_info_stream = &program->info_stream;
	}
	if(!program)
	{ 
		//
		// allocate program features 
		//

		area->program_state = memory_area_clear_and_push_struct(main_memory_area, struct s_program_state);
		program = area->program_state;
		program->area = main_memory_area;
		//set platfom
		program->platform   = area->platformApi;
		//set input
		program->input = program_input;
		//if debug
		//g_info_stream = &program->info_stream;

		//set up the settings to initialize the program
		game_resource_initial_settings game_resource_settings = {0};

		game_resource_settings.total_slots = assets_MAX_ASSET_CAPACITY;
		game_resource_settings.allocation_size = MEGABYTES(12);
		game_resource_settings.operations_size = MEGABYTES(4);
		game_resource_settings.dev_build = 1;
		game_resource_settings.resource_folder_path = "data";
		//start up without allocating any initial assets
		game_resource_settings.initial_assets_count = 0;
		game_resource_settings.use_pack = 0;
		game_resource_settings.resource_pack_file_name = "data/gameassets.pack";//"data/gameassets.pack";
		program->game_asset_manager = assets_allocate_game_assets(
				main_memory_area,
				program->platform,
				&game_renderer->texture_operations,
				game_resource_settings,
				0);

		//load the default font to the ui
		//   program->default_font = assets_GetFont(program->game_asset_manager,
		//		                                         assets_generate_id("data/fonts/roboto.font"),
		//												 3).font;

		program->default_font = assets_load_and_get_font(program->game_asset_manager,
				"magus.pfnt");
		Assert(program->default_font);

		program->reload_game = 1;
		//program->world = assets_GetMap(program->game_asset_manager, area->platformApi, main_memory_area,'MAP0');


		//initialize world
		//initialize program
		//
		program->random_s = random_series_create(40);
		u32 bindex = 0;
//		game_body *test_body = game_spawn_body(program, game_get_coso_id(program), &bindex);

		//for debug
		program->ui = area->ui;



		//
		//Debug and console
		//




		//
		// camera parameters
		//
		game_renderer->camera_position = V3(0, 0, 200.0f);
		game_renderer->camera_rotation.x = 0;// -0.5f;

		program = area->program_state;
		//program prototype stuff
		program->area = main_memory_area;

	}
	program->elapsed_ms = area->elapsed_ms;
	program->mc_per_frame = area->mc_per_frame;
	if(!program->target_framerate) program->target_framerate = 60;

	area->target_ms = 1.0f / (program->target_framerate);

	//
	// begin commands frame
	//

    game_renderer->clear_color[0] = 0.25f;
    game_renderer->clear_color[1] = 0.75f;
    game_renderer->clear_color[2] = 1.0f;
    game_renderer->clear_color[3] = 1.0f;
	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

	render_commands_clear_graphics(commands);

	//render_commands_SetProjection(commands);

//	render_rectangle_2d_xywh(commands,
//			1000, 20, 200, 200, V4(255, 0, 0, 255));
	{
		//draw a colored rectangle
		f32 w = 512;
		f32 y = 0;
		f32 h = 512;
		vec2 v0 = {20    , y + 300};
		vec2 v1 = {0 + 0, y + 30};
		vec2 v2 = {0 + w, y + 60};
		vec2 v3 = {0 + w, y + 300};
		vec2 uv0, uv1, uv2, uv3;
		render_fill_uvs_counter_cw(&uv0, &uv1, &uv2, &uv3);

		vec4 color0 = {255, 0, 0, 255};
		vec4 color1 = {255, 0, 0, 255};
		vec4 color2 = {0, 0, 255, 255};
		vec4 color3 = {0, 0, 255, 255};
		render_push_vertices_2d(commands,
				&game_renderer->white_texture,
				v0,
				v1,
				v2,
				v3,
				uv0,
				uv1,
				uv2,
				uv3,
				color0,
				color1,
				color2,
				color3);

//		sw_render_push_quad_2d(commands,
//				&game_renderer->white_texture,
//				v0,
//				v1,
//				v2,
//				v3,
//				uv0,
//				uv1,
//				uv2,
//				uv3,
//				color0
//				);
	}
	
	game_assets *assets = program->game_asset_manager;
	render_texture *t0 = assets_load_and_get_image(assets, "t0.png"); 
	render_texture *mdf = assets_load_and_get_image(assets, "mdf.png"); 
	render_texture *white = assets_get_white_texture(assets);

//	sw_draw_sprite(program, commands, R32U(16, 176, 16, 16), 512, 512, t0, V4(255, 255, 255, 255));
//	sw_draw_sprite(program, commands, R32U(0, 0, 32, 64), 512, 512, mdf, V4(255, 0, 255, 255));
//	sw_draw_sprite(program, commands, R32U(0, 0, 512, 512), 512, 512, white, V4(255, 0, 0, 255));
	




	render_commands_end(commands);




	//update program
	//game_update_render(program, game_renderer, program_input, dt);
	

	//post-update

	//RUN CAMERA
	game_renderer->camera_zoom_2d = 1.0f;
	render_update_camera_values(game_renderer, 16, 9);

	//Push command
	//
	//Update_Render_Debug
	//
	//get console

}

int dll_main()
{
	return(1);
}
//Never used
int _DllMainCRTStartup()
{
	void * ap = program_progress;
	return(1);
}
