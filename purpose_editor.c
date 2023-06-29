
//To load an external file reference always start from the offset specified in the file's header.
//read the struct ppse_external_resources_header and advance by the size of the struct
//to start. To read the path and name always start with an u32 which specifies it's length,
//then the name

/*Editor notes:

Animations:
   An animation uses several structs, the first one beign "model_animations", which
   contains how many "model_animation" structs has stored by a count and an array

model_animation:
   Contains the following properties:
   "loop" a boolean indicating if the animation will loop from frame_loop_start to frame_loop_end
   "frame_timer" will divide one ms by frame_timer_repeat to give a "frame by frame" timer effect instead of a smooth one.
   "frames_per_ms" is used to put more "model_animation_clip" inside one ms

   "total_uvs_count" is used for knowing how much memory it needs for storing
   an uvs array

   The struct then contains a count indicaing how many "model_animation_keyframe" and
   "model_animation_clip" has stored and a pointer to said arrays.

model_animation_clip:
  A clip contains two numbers indicating in which frame it will run the keyframes
  its pointing to.
  "keyframe_count" the count of how many keyframes this clip points to
  "keyframe_at" specifies which part of the model_animation_keyframe array of the
  model_animation struct will start reading from.
  "spline" is an effect applied to the timer.

  key frames:
  These are divided by the two following types: Transform and Frame.

  Transform will pick one of the model's bones to move and/or rotate them.

  Frame will instead pick one of the model's quad and change their properties for the
  rest of the animation.


  map format


*/



#include "purpose_crt.h"
#include "global_definitions.h"
#include "agu_random.h"
#include "global_all_use.h"
#include "purpose_memory.h"
#include "purpose_stream.h"
#include "purpose_platform.h"
#include "purpose_console.h"
#include "purpose_math.h"
#include "purpose_global.h"
#include "purpose_render.h"
#include "purpose_render.c"
#include "purpose_ui.h"
#include "purpose_ui.c"


#include "purpose_editor_platform.h"

#include "purpose_all.h"
#include "brains.h"
#include "purpose_assets.h"
#include "purpose_platform_layer.h"
#include "purpose.h"
#include "purpose_assets.c"
#include "model_render.h"
#include "model_render.c"

//#include "purpose_world.h"

#include "purpose_ray.c"
#include "purpose_geometry.c"
#include "purpose_game.h"

static stream_data *g_info_stream = 0;
#define Gprintf(params, ...) stream_pushf(g_info_stream, params, __VA_ARGS__)

#include "purpose_editor.h"
#include "purpose_editor_resources.h"
#include "purpose_editor_global.c"
#include "purpose_editor_cursor.c"
#include "purpose_editor_tileset.c"
#include "purpose_editor_world.c"
#include "purpose_editor_graphics.c"
#include "purpose_editor_assets.c"
#include "purpose_editor_coso.c"
#include "purpose_editor_frame_animation.c"
//#include "purpose_editor_world_old.c"
#include "purpose_console_editor_commands.c"


#define V4_RED V4(255, 0, 0, 255)
#define V4_YELLOW V4(255, 255, 0, 255)
#define V4_BLUE V4(0, 0, 255, 255)
#define V4_GREEN V4(0, 255, 0, 255)

//for trying new stuff
#define ANIMATION_NEW_V 1


//
// editor start
//


//translate_coordinates_to_tile(distance_ray_cursor,
//		                      next_tile_r_axis,
//							  next_tile_u_axis,
//							  frame_w,
//							  frame_h);
//editor_cursor_translate_ray_to_tile(rayOrigin,
//		                            rayDir,
//									frame_w,
//									frame_h);

//inline ray_cube_result
//editor_cursor_ray_vs_gizmo(
//		editor_cursor *cursor,
//		vec3 rayOrigin,
//		vec3 rayDir)
//{
//}
inline void
editor_update_camera(game_renderer *game_renderer,
		              s_editor_state *editor_state,
					  editor_input *editor_input)
{
	s_game_editor *game_editor = &editor_state->editor;
    u16 ui_is_interacting  = game_editor->ui_is_interacting; 
    u16 ui_focused         = game_editor->ui_is_focused; 
    u32 input_text_focused   = editor_state->ui->input.input_text.focused;

    u32 mouse_r_down    = input_down(editor_input->mouse_right);
    u32 mouse_l_down    = input_down(editor_input->mouse_left);

	u32 lock_camera_rotation = input_text_focused;

    f32 mouse_delta_x = editor_input->mouse_clip_x_last - editor_input->mouse_clip_x;
    f32 mouse_delta_y = editor_input->mouse_clip_y_last - editor_input->mouse_clip_y;

	f32 camera_rotation_x = game_renderer->camera_rotation.x;
	f32 camera_rotation_z = game_renderer->camera_rotation.z;

        //if(!lock_camera_rotation && editor_state->platform->window_is_focused)
        //{
        //  if(editor_input->spaceBarDown && !editor_input->ctrl_l)
        //  {

        //    f32 rx = 0;
        //    f32 rz = 0;

        //    rx += (f32)mouse_delta_y * PI * 0.0005f;
        //    rz += (f32)mouse_delta_x * PI * 0.0005f;

        //    game_renderer->camera_rotation.x += rx; 
        //    game_renderer->camera_rotation.z += rz; 
        //  }

        //}
	    //f32 xr = game_renderer->camera_rotation.x;
	    //f32 zr = game_renderer->camera_rotation.z;
	    //game_renderer->camera_rotation.z = zr < 0 ? 2.0f : zr >= 2.0f ? 0 : zr;
	    //game_renderer->camera_rotation.x = xr < 0 ? 2.0f : xr >= 2.0f ? 0 : xr;

	if(game_editor->mode == s_game_editor_world)
	{

	    //camera rotation

		game_editor->world.in_camera_mode = 0;
        if(!lock_camera_rotation && editor_state->platform->window_is_focused)
        {

		    camera_rotation_x = game_editor->world.camera_rotation_x;
		    camera_rotation_z = game_editor->world.camera_rotation_z;
            if(editor_input->spaceBarDown && mouse_l_down)
            {
			    game_editor->world.in_camera_mode = 1;

                camera_rotation_x += ((f32)mouse_delta_y * PI * 0.0005f); 
                camera_rotation_z += ((f32)mouse_delta_x * PI * 0.0005f); 
            }

	        camera_rotation_x = camera_rotation_x < 0 ? 2.0f : camera_rotation_x >= 2.0f ? 0 : camera_rotation_x;
	        camera_rotation_z = camera_rotation_z < 0 ? 2.0f : camera_rotation_z >= 2.0f ? 0 : camera_rotation_z;

            game_editor->world.camera_rotation_x = camera_rotation_x;
            game_editor->world.camera_rotation_z = camera_rotation_z;
	        game_renderer->camera_rotation.x = camera_rotation_x;
	        game_renderer->camera_rotation.z = camera_rotation_z;

        }
	

        real32 camSpeed = 6.0f;

	    if(!input_text_focused)
	    {
			vec3 camera_position = game_editor->world.camera_position;
			if(game_editor->world.camera_mode == editor_world_camera_free)
			
			{

				i32 mouseWheelValue = editor_input->mouse_wheel * 10;
		        if(!ui_focused)
		        {
		            game_editor->world.camera_distance_from_cursor -= mouseWheelValue;

				    if(game_editor->world.camera_distance_from_cursor < 0)
				    {
                        game_editor->world.camera_distance_from_cursor = 0;

				    	//instead, add the new value to the pad location
				    	vec3 add_zAxis = vec3_scale(game_renderer->camera_z, (f32)mouseWheelValue);

						game_editor->world.camera_pad_position.x -= add_zAxis.x;
						game_editor->world.camera_pad_position.y -= add_zAxis.y;
						game_editor->world.camera_pad_position.z -= add_zAxis.z;
				    }
		        }

				f32 camera_distance_from_cursor = game_editor->world.camera_distance_from_cursor;
				vec3 camera_distance_from_cursor_vec = {0, 0, camera_distance_from_cursor};


		        matrix4x4 camera_rotation = matrix4x4_rotation_scale(game_renderer->camera_rotation.x,
		   		                                                    game_renderer->camera_rotation.y,
		   						                                    game_renderer->camera_rotation.z);

		        camera_distance_from_cursor_vec = matrix4x4_v3_mul_rows(
						                       camera_rotation,
						                       camera_distance_from_cursor_vec,
											   0);

				camera_position = vec3_add(camera_distance_from_cursor_vec,
						                  game_editor->world.camera_pad_position);

				//move padding position
				if(editor_input->spaceBarDown && mouse_r_down)
				{
					f32 mouse_delta_x = editor_input->mouse_x - editor_input->mouse_x_last;
					f32 mouse_delta_y = editor_input->mouse_y - editor_input->mouse_y_last;

					f32 paddingSpeed = 0.3f;

					vec3 add_xAxis = vec3_scale(game_renderer->camera_x, mouse_delta_x * paddingSpeed);
					vec3 add_yAxis = vec3_scale(game_renderer->camera_y, mouse_delta_y * paddingSpeed);

					game_editor->world.camera_pad_position.x -= add_xAxis.x - add_yAxis.x;
					game_editor->world.camera_pad_position.y -= add_xAxis.y - add_yAxis.y;
					game_editor->world.camera_pad_position.z -= add_xAxis.z - add_yAxis.z;
				}
			}


            game_editor->world.camera_position = camera_position;
            game_renderer->camera_position = game_editor->world.camera_position;
	    }
	}
	else if(game_editor->mode == s_game_editor_tileset)
	{
		if(!ui_focused)
		{
			vec3 fixed_camera_position = {0};
			vec3 fixed_camera_rotation = {0, 0, 0.5f};
			f32 camera_distance = 0;

			camera_rotation_x = fixed_camera_rotation.x;
			camera_rotation_z = fixed_camera_rotation.z;
			game_renderer->camera_position = fixed_camera_position;
		}
	}

	game_renderer->camera_rotation.x = camera_rotation_x;
	game_renderer->camera_rotation.z = camera_rotation_z;
}



//this doesn't have smooth scrolling sadly :(
//you select text.
//you can copy from this editor and the pc clipboard. you use "*y keys to copy to the clipboard
//y alone copies onto the editor's clipboard "*y copies to the pc clipboard!
//you can set up a hotkey ofc :)

inline void
editor_read_avadible_files(s_game_editor *editor,
		                   platform_api *platform)
{
}

static void
game_editor_update_render(
		s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input,
		f32 dt)
{

	s_game_editor *game_editor = &editor_state->editor;
	game_editor->settings.vertices_cube_size = 1.0f;

	//load avadible files from disk if not using packages
	if(game_editor->reload_file_info)
	{
		game_editor->reload_file_info = 0;
	}
	editor_assets_look_for_file_changes(
			editor_state);
	switch(game_editor->mode)
	{
		case s_game_editor_model:
			{
				editor_model_update_camera(
						editor_state,
						game_renderer,
						editor_input);
			}break;
		case s_game_editor_world:
		{
		}break;
		case s_game_editor_coso:
		{
			editor_coso_update_camera(
					game_editor, game_renderer);
		}break;
	}
	
	editor_update_camera(game_renderer, editor_state, editor_input);
	render_update_camera_rotation(game_renderer);
	//update editors

	game_editor->process_input = !game_editor->ui_is_focused &&
		!editor_input->spaceBarDown && 
		editor_state->platform->mouseAtWindow;

	if(game_editor->tipTimer < game_editor->tipTotalTime)
	{
		game_editor->tipTimer += dt;
	}

	switch(game_editor->mode)
	{
		case s_game_editor_world:
			{
				editor_world_update_render(editor_state, game_renderer, editor_input, dt);
			}break;
		case s_game_editor_model:
			{
				editor_model_update_render(editor_state, game_renderer, editor_input, dt);
			}break;
		case s_game_editor_tileset:
			{
				editor_tileset_update_render(editor_state, game_renderer, editor_input, dt);
			}break;
		case s_game_editor_coso:
			{
				editor_coso_update_render(editor_state, game_renderer, editor_input, dt);
			}break;
		case editor_mode_frame_animation:
			{
				editor_franim_update_render(editor_state, game_renderer, editor_input, dt);
			}break;
	}

	game_ui *ui = ui_begin_frame(
			editor_state->ui,
			editor_state->ui_font,
			editor_state->platform,
			game_renderer,
			dt);
#if 1



	ui_widget_region_begin(ui,
			0,
			0,
			game_renderer->os_window_width,
			game_renderer->os_window_height,
			"Global panel");
	{

		ui_node *editor_bar;
		ui_set_height(ui, ui_size_em(ui, 2.0f, 1.0f))
		{
			ui_set_w_ppct(ui, 1.0f, 1.0f)
				editor_bar = ui_create_node(
						ui,
						node_background |
						node_border |
						node_clickeable,
						0);
		}
		ui_set_parent(ui, editor_bar)
		{
			ui_space_specified(ui, 2.0f, 1.0f);
			ui_set_width(ui, ui_size_text(4.0f, 1.0f))
				ui_set_height(ui, ui_size_text(4.0f, 1.0f))
				ui_set_row(ui)
				{
					u32 prev = game_editor->mode;
					u32 mode_before = game_editor->mode;
					ui_space(ui, ui_size_specified(6, 1.0f));
					ui_selectable_u32(ui, s_game_editor_world, &(u32)game_editor->mode, "World");
					ui_space(ui, ui_size_specified(6, 1.0f));
					ui_selectable_u32(ui, s_game_editor_model, &(u32)game_editor->mode, "Graphics");
					ui_space(ui, ui_size_specified(6, 1.0f));
					ui_selectable_u32(ui, s_game_editor_tileset, &(u32)game_editor->mode, "Tileset");
					ui_space(ui, ui_size_specified(6, 1.0f));
					ui_selectable_u32(ui, s_game_editor_coso, &(u32)game_editor->mode, "Entities");
					ui_space(ui, ui_size_specified(6, 1.0f));
					ui_selectable_u32(ui, editor_mode_frame_animation, &(u32)game_editor->mode, "Frame animations");
					if(mode_before != game_editor->mode)
					{
						er_explorer_close(game_editor);
					}


					if(game_editor->mode != prev)
					{
						ui_explorer_end_process(ui);
						if(game_editor->mode == s_game_editor_world)
						{
							game_editor->world.draw_locked = 0;
						}
					}

				}
		}


		switch(game_editor->mode)
		{
			case s_game_editor_world:
				{
					editor_world_update_render_ui(editor_state, game_renderer, editor_input);
				}break;
			case s_game_editor_model:
				{
					editor_model_update_render_ui(editor_state, game_renderer, editor_input);
				}break;
			case s_game_editor_tileset:
				{
					editor_tileset_update_render_ui(editor_state, game_renderer, editor_input, dt);
				}break;
			case s_game_editor_coso:
				{
					editor_coso_update_render_ui(editor_state, game_renderer, editor_input, dt);
				}break;
			case editor_mode_frame_animation:
				{
					editor_franim_update_render_ui(editor_state, game_renderer, editor_input);
				}break;
		}

		editor_assets_update_render_ui(editor_state, game_renderer, editor_input, dt);
	}
	ui_widget_region_end(ui);
#endif

	ui_show_test_panel(ui);

	//ui_show_test_panel(ui, &editor_input->text);
	//
	// update and render asset panel
	//
	// Asset editor
	//editor_asset_update_render_ui(editor_state, game_renderer, editor_input);

	//
	// Side tips panel
	//
	if(game_editor->tipTimer < 1 || editor_input->enter)//game_editor->tipTotalTime)
	{
#if 0
		ui_floating_panel_begin(ui,
				ui_panel_flags_ignore_focus,
				(f32)game_renderer->os_window_width - 200 - 40,
				(f32)game_renderer->os_window_height - 200 - 40,
				200,
				200,
				"General tips");
		{
			//ui_text(ui, "Hi!, I display tips!");
			//ui_textf(ui, "Timer:%f | total:%f", game_editor->tipTimer, game_editor->tipTotalTime);
			//ui_set_color(ui, ui_color_text, V4(0, 255, 0, 255))
			//{
			//    ui_text(ui, "Hi!, I display tips!");
			//}
			//ui_textWrapped(ui, "File saved to: c:/dev/proyectos/purpose_c/text.txt");
		}
		ui_panel_end(ui);
#endif
	}
	else
	{
		int s = 0;
	}
	//f11
	if(editor_input->f_keys[10])
	{
		ui_open_or_close_panel(ui, "Debug panel");
	}
	if(editor_input->f_keys[9])
	{
		ui_open_or_close_test_panel(ui);
	}
	if(editor_input->f_keys[4])
	{
		ui_open_or_close_panel(ui, "Content browser");
	}

	if(editor_input->f_keys[1])
	{
		er_explorer_set_process(game_editor, er_explorer_load, "Sample process");
	}


	//
	// Debug panel 
	// 

	if(_ui_window_begin(ui,
				ui_panel_flags_front_panel | ui_panel_flags_init_closed,
				(f32)game_renderer->os_window_width - 600,
				0,
				600,
				600,
				1,
				&game_editor->explorer_is_opened,
				"Debug panel"))
	{

#if 1
		ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_specified(ui, 600.0f, 1.0f)
		{
			ui_console(ui, &editor_state->debug_console, "editor_debug_console");
		}
		ui_space(ui, ui_size_specified(4, 1.0f));

		ui_set_wh_text(ui, 4.0f, 1.0f)
		{
			ui_textf(ui, "process_input: %u", game_editor->process_input);
			ui_textf(ui, "ui_is_focused: %u", game_editor->ui_is_focused);
			ui_textf(ui, "ui_input_double_clicked_left: %u", ui->input.doubleClickedLeft);
		}
		quaternion q = quaternion_from_rotations_scale(
				game_editor->sin_value,
				game_editor->tan_x,
				game_editor->tan_y);
		matrix3x3 c_r = matrix3x3_rotation_scale(
				game_editor->sin_value,
				game_editor->tan_x,
				game_editor->tan_y);

		ui_set_height(ui, ui_size_text(1.0f, 1.0f))
			ui_set_width(ui, ui_size_percent_of_parent(0.3f, 1.0f))
			{
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->sin_value, 0, "yaw");
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->tan_x, 0, "pitch");
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->tan_y, 0, "roll");

				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->w, 0, "w");
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->x, 0, "x");
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->y, 0, "y");
				ui_spinner_f32(ui, 0.1f, -100000, 100000, &game_editor->z, 0, "z");
			}

		quaternion q_test = game_editor->e_q; 
		//			q_test = quaternion_rotate_by_radian_and_maintain_length(q_test, PI * game_editor->w);
		//			game_editor->e_q = q_test;
		//q_test = quaternion_rotate_by_xyz_scale(q_test.x, q_test.y, q_test.z);
		q_test = QUAT(0, q_test.x, q_test.y, q_test.z);

		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_textf(ui, "Quaternion: {%f, %f, %f, %f}",
					q.w, q.x, q.y, q.z);
			ui_textf(ui, "Quaternion test: {%f, %f, %f, %f}",
					q_test.w, q_test.x, q_test.y, q_test.z);


			vec3 vector_to_r = {0, 0, 1};
			vec3 q_r_result  = quaternion_v3_mul_inverse_foward(q, vector_to_r);
			vec3 q_r_t_result  = quaternion_v3_mul_foward_inverse(q_test, vector_to_r);
			vec3 q_r_t_result1  = quaternion_v3_mul_inverse_foward(q_test, vector_to_r);
			vec3 m_r_result0 = matrix3x3_v3_mul_rows(c_r, vector_to_r);
			vec3 m_r_result1 = matrix3x3_v3_mul_cols_l(c_r, vector_to_r);

			ui_textf(ui, "Quaternion * vector(1, 0, 1): {%f, %f, %f}",
					q_r_result.x, q_r_result.y, q_r_result.z);
			ui_textf(ui, "Matrix_row * v: {%f, %f, %f}, matrix_column * v: {%f, %f, %f}",
					m_r_result0.x, m_r_result0.y, m_r_result0.z,
					m_r_result1.x, m_r_result1.y, m_r_result1.z
					);
			ui_textf(ui, "Quaternion test * v: {%f, %f, %f} v * quaternion test {%f, %f, %f}",
					q_r_t_result.x, q_r_t_result.y, q_r_t_result.z,
					q_r_t_result1.x, q_r_t_result1.y, q_r_t_result1.z);

			ui_space(ui, ui_size_specified(4, 1.0f));
			ui_textf(ui, "Camera position {%f, %f, %f}", game_renderer->camera_position.x, game_renderer->camera_position.y, game_renderer->camera_position.z);
			ui_textf(ui, "Camera X{%f, %f, %f}", game_renderer->camera_x.x, game_renderer->camera_x.y, game_renderer->camera_x.z);
			ui_textf(ui, "Camera Y{%f, %f, %f}", game_renderer->camera_y.x, game_renderer->camera_y.y, game_renderer->camera_y.z);
			ui_textf(ui, "Camera Z{%f, %f, %f}", game_renderer->camera_z.x, game_renderer->camera_z.y, game_renderer->camera_z.z);
			ui_textf(ui, "Camera Rotation {%f, %f, %f}", game_renderer->camera_rotation.x, game_renderer->camera_rotation.y, game_renderer->camera_rotation.z);

			vec2 cameraFoward = {0, 1};
			u32 globalOrientationIndex16 = get_orientation_index_foward(game_renderer, cameraFoward, 16);
			ui_textf(ui, "Orientation from world in 16 directions Y:%u", globalOrientationIndex16);

			u32 globalOrientationIndex8 = get_orientation_index_foward(game_renderer, cameraFoward, 8);
			ui_textf(ui, "Orientation from world in 8 directions Y:%u", globalOrientationIndex8);

			u32 globalOrientationIndex4 = get_orientation_index_foward(game_renderer, cameraFoward, 4);
			ui_textf(ui, "Orientation from world in 4 directions Y:%u", globalOrientationIndex4);

			u32 globalOrientationIndex2 = get_orientation_index_foward(game_renderer, cameraFoward, 2);
			ui_textf(ui, "Orientation from world in 2 directions Y:%u", globalOrientationIndex2);

			vec3 camera_up_down = {0, -1, 0};

			u32 orientation_index_above = get_orientation_index_up_down(
					game_renderer,
					camera_up_down,
					16);
			f32 up_down_angle = get_up_down_angle(
					game_renderer,
					camera_up_down,
					degrees_to_radians_f32(0));
			f32 up_down_angle_degrees = radians_to_degrees_f32(up_down_angle);
			//u32 is_above_45 = is_looking_down_by_degrees(
			//		game_renderer,
			//		camera_up_down,
			//		45);
			u32 is_below_45 = 0;
			u32 is_above_45 = 0;
			is_above_45 = is_looking_down_by_degrees(
					game_renderer,
					camera_up_down,
					45);

			is_below_45 = is_looking_up_by_degrees(
					game_renderer,
					camera_up_down,
					45);

			ui_textf(ui, "up down in 16 directions Y:%u", orientation_index_above);
			ui_textf(ui, "up down angle:%f as degrees:%f", up_down_angle, up_down_angle_degrees);
			ui_textf(ui, "Is above 45º:%u. Is below 45º:%u",
					is_above_45,
					is_below_45);
		}


		ui_set_width(ui, ui_size_text(1.0f, 1.0f))
			ui_set_height(ui, ui_size_text(1.0f, 1.0f))
			{
				if(ui_button(ui, "Reset camera"))
				{
					game_renderer->camera_position.x = 0; 
					game_renderer->camera_position.y = 0; 
					game_renderer->camera_position.z = 0; 
					game_renderer->camera_rotation.x = -PI * 0.5f; 
					game_renderer->camera_rotation.y = 0; 
					game_renderer->camera_rotation.z = 0; 
				}
			}
		ui_space(ui, ui_size_specified(20, 1.0f));
		ui_set_wh_text(ui, 4.0f, 1.0f)
		{
			ui_textf(ui, "ui mouse left pressed : %u", ui->mouse_l_pressed);
			ui_textf(ui, "ui_any_node_interacting : %u", ui_any_node_interacting(ui));
			ui_textf(ui, "ui_any_node_hot : %u", ui_any_node_hot(ui));

			ui_textf(ui, "mouse position {x:%f y:%f}", editor_input->mouse_x, editor_input->mouse_y);
			ui_textf(ui, "mouse clipped position {x:%f y:%f}", editor_input->mouse_clip_x, editor_input->mouse_clip_y);
			f32 mouseDeltaX = editor_input->mouse_clip_x - editor_input->mouse_clip_x_last;
			f32 mouseDeltaY = editor_input->mouse_clip_y - editor_input->mouse_clip_y_last;
			ui_textf(ui, "mouse delta {x:%f y:%f}", mouseDeltaX, mouseDeltaY); 
			ui_textf(ui, "mouse left was_down %u transition_count %u", editor_input->mouse_left.was_down, editor_input->mouse_left.transition_count);
		}

#endif
	}
	ui_panel_end(ui);





	//ui_tab_group(ui);
	//



	ui_ignore_interaction_only(ui)
	{
		ui_run_explorer(ui, &game_editor->explorer_is_opened);
	}
	editor_run_resource_explorer(game_editor, ui);
	ui_end_frame(
			ui,
			editor_state->platform->window_is_focused);

	u32 ui_is_interacting = ui_any_node_hot(ui) || ui_any_node_interacting(ui);
	u32 ui_focused = ui_is_interacting;

	game_editor->ui_is_interacting = ui_is_interacting;
	game_editor->ui_is_focused = ui_focused;


}

EDITOR_UPDATE_RENDER(editor_progress)
{
	memory_area *main_memory_area = program_memory->main_area;
	s_editor_state *editor_state = program_memory->program;
#if 1
	if(!editor_state)
	{
		//
		//initialize editor
		//
		//allocate
		program_memory->program = memory_area_clear_and_push_struct(
				main_memory_area, s_editor_state);
		editor_state = program_memory->program;
		s_game_editor *game_editor = &editor_state->editor;

		//initial editor settings
		game_editor->tipTimer = 0;
		game_editor->tipTotalTime = (u32)(0.1f * 80);
		game_editor->area = memory_area_create_from(main_memory_area, MEGABYTES(16));
		game_editor->info_stream = stream_Create(&program_memory->log_area);
		g_info_stream = &game_editor->info_stream;

		memory_area *editorArea = &game_editor->area;

		editor_state->ui = program_memory->ui;
	//	ui_initialize(
	//			memory_area_push_size(editorArea, MEGABYTES(1)),
	//				MEGABYTES(1),
	//				0);
		editor_state->platform = program_memory->platform;

		game_resource_initial_settings editor_resource_settings = {0};
		editor_resource_settings.total_slots = assets_MAX_ASSET_CAPACITY;
		editor_resource_settings.allocation_size = MEGABYTES(12);
		editor_resource_settings.operations_size = MEGABYTES(4);
		editor_resource_settings.dev_build = 1;
		editor_resource_settings.resource_folder_path = "data";
		//start up without allocating any initial assets
		editor_resource_settings.initial_assets_count = 0;
		editor_resource_settings.use_pack = 0;
		editor_resource_settings.resource_pack_file_name = "data/gameassets.pack";//"data/gameassets.pack";

		editor_state->editor_assets = assets_allocate_game_assets(
				main_memory_area,
				editor_state->platform,
				&game_renderer->texture_operations,
				editor_resource_settings,
				&program_memory->log_area);
		//load default font
		editor_state->ui_font = assets_load_and_get_font(editor_state->editor_assets,
				"data/fonts/roboto.pfnt");
		Assert(editor_state->ui_font);


		// ;clean
		game_editor->data_files_max = 200;
		game_editor->data_files = memory_area_push_array(
				&game_editor->area,
				asset_file_info,
				game_editor->data_files_max);
		game_editor->reload_file_info = 1;

		//editor font
#if 0
		game_editor->default_font = memory_area_push_struct(&game_editor->area, font_proportional, font_proportional);
		ppse_read_font_from_file(&game_editor->area,
				game_editor->platform,
				game_editor->default_font,
				"data/fonts/roboto.font");
		Assert(game_editor->default_font);
#endif
		editor_world_allocate(game_editor);


		editor_coso_allocate(game_editor, game_renderer);
		//initialize model editor
		editor_graphics_allocate(game_editor);
		// initialize tileset editor
		editor_tileset_allocate(game_editor);
		//frame animations
		editor_franim_allocate(game_editor);
		//assets
		editor_resources_init(editor_state,
				"data/");

		editor_state->debug_console = console_allocate(
				editorArea,
				editor_state->platform,
				KILOBYTES(125),
				100,
				256);
		game_editor->game_console = &editor_state->debug_console;
		game_renderer->camera_zoom = 2;



		render_set_initial_parameters(game_renderer);
	}
	if(!g_info_stream)
	{
		s_game_editor *game_editor = &editor_state->editor;
		g_info_stream = &game_editor->info_stream;
	}
#endif

	render_commands *commands = render_commands_begin_default(game_renderer);

	render_commands_clear_graphics(commands);

	render_commands_end(commands);


	game_editor_update_render(
			editor_state,
			game_renderer,
			editor_input,
			dt);


	render_update_camera_values(game_renderer, 16, 9);

	//read editor commands
	editor_console_end_frame(
			editor_state,
			editor_input);

	//log
	memory_area_reset(&program_memory->log_area);
}

int dll_main()
{
	return(1);
}
