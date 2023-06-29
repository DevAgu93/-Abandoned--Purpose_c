
#include "purpose_crt.h"
#include "global_definitions.h"
#include "agu_random.h"
#include "global_all_use.h"
#include "purpose_memory.h"
#include "purpose_stream.h"
#include "purpose_platform.h"
//#include <gm_input_text.c>
#include "purpose_global.h"
#include "purpose_console.h"
#include "purpose_math.h"
#include "purpose_render.h"
#include "purpose_render.c"
#include "purpose_ui.h"
#include "purpose_ui.c"



#include "purpose_platform_layer.h"
#include "purpose_all.h"
#include "purpose_entity_states.h"
#include "purpose_assets.h"
#include "purpose.h"
#include "purpose_assets.c"

//#include "purpose_h"
#include "purpose_model_render.c"

#include "purpose_ray.c"
#include "purpose_geometry.c"
#include "purpose_game.h"
#include "agu_timer.h"

#include "devexp_platform.h"

static f32 global_target_framerate = 0;

typedef struct game_coso{
	u32 id;
	game_body *body;
	state_main *brain;
}game_coso;

typedef struct{

	f32 elapsed_ms;

	platform_api *platform;
	game_ui *ui;
	font_proportional *ui_font;
	s_game_console debug_console;
	memory_area *area;
	u16 program_w;
	u16 program_h;
	f32 target_framerate;

	struct s_game_assets *assets;
	b32 two_dim;

	//for 2d
	
		u32 bone_count2d;
		struct exp_bone_2d *bones_2d;
		u32 bone_to_ik_2d;
		b8 apply_ik_2d;
		vec2 ik_vec_2d;

		vec2 point;
		f32 point_angle;
		vec2 wall_p;
		vec2 wall_normal;

		vec2 l0;
		vec2 l1;
		vec2 l2;
	
	//angle directions
	u32 max_directions;
	vec2 center_point;
	
	//poisson disc sampling
	random_series random_s;
	f32 sampling_r;
	f32 min_sampling;
	f32 cell_wh;
	u16 grid_w;
	u16 grid_h;
	i16 *grid;
	u16 active_points_max;
	u16 active_points_count;
	i16 *active_points;
	u16 final_points_max;
	u16 final_points_count;
	vec2 *final_points;
	u8 *final_colors;
	u32 points_gen_count;
	b32 generate_poisson;
	f32 poisson_timer;

	//for 3d
	u32 bone_count;
	struct exp_bone *bones;
	//rest
	vec3 ik_vec;
	//camera
	vec3 camera_p;
	vec3 camera_r;
	f32 camera_rotation_x;
	f32 camera_rotation_y;
	f32 camera_rotation_z;
	b8 in_camera_mode;
	b8 apply_ik;
	u32 bone_to_ik;
	f32 camera_distance_from_cursor;

	vec3 camera_pad_position;

	//physics

	//misc

}devexp_state;

#include "devexp_2d_bones.c"

typedef struct exp_bone{
	u16 parent;
	vec3 p;
	vec3 pivot;
	vec3 transformed_p;
	vec3 displacement;
	vec3 normal;

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

	union{
		vec3 transformed_displacement;
		struct{
	f32 transformed_displacement_x;
	f32 transformed_displacement_y;
	f32 transformed_displacement_z;
		};
	};
}exp_bone;











static void
editor_model_apply_ik(
		devexp_state *program,
		game_renderer *game_renderer,
		vec3 target)
{
	//s_model_editor *model_editor = &editor->model;
	exp_bone *bones = program->bones;

	exp_bone *bone = bones + program->bone_to_ik;
	target = program->ik_vec;
	exp_bone *parent_bone = bones + bone->parent;
	//distance between target and end position
	exp_bone *ascending_bone = parent_bone;
	u32 previous_parent = bone->parent;
	u32 prev = previous_parent;
	//no ascendants
	if(ascending_bone == bone)
	{
		return;
	}

	f32 b = 0;
	do
	{

		b += vec3_length(ascending_bone->p);

		prev = previous_parent;
		previous_parent = ascending_bone->parent;
		ascending_bone = bones + ascending_bone->parent;
	}while(prev != previous_parent);
	exp_bone *root_bone = ascending_bone;


	ascending_bone = parent_bone;
	previous_parent = bone->parent;
	prev = previous_parent;
	vec3 au = vec3_normalize(bone->p);
	vec3 r = {0};
	f32 angle = 0;
	do
	{
		parent_bone = bones + ascending_bone->parent;
		f32 a = 5.0f;//vec3_length(ascending_bone->p);
		b -= a;//vec3_length(ascending_bone->p);
		if(1)
		{
			b = vec3_length(vec3_sub(parent_bone->transformed_p, root_bone->transformed_p));
			f32 c = vec3_length(vec3_sub(target, ascending_bone->transformed_p));
			//f32 theta = arccos32(-((b * b) - (a * a) - (c * c)) / (2 * a * c));
			f32 theta = arccos32(((a * a) + (c * c) - (b * b)) / (2 * a * c));
			vec3 cu = vec3_normalize(vec3_sub(target, ascending_bone->p));
			angle = arccos32(vec3_inner(au, cu)) - theta;
			r = vec3_cross(au, cu);
		}
		else
		{
			break;
		}

		if(parent_bone == bone)
		{
			continue;
		}
		if(previous_parent != bone->parent)
		{
			//			break;
		}
		ascending_bone->q = quaternion_rotated_at(r.x, r.y, r.z, angle);
		//		editor_keyframe->base.offset = quaternion_v3_mul_inverse_foward(kf->base.q, editor_keyframe->base.offset);

		prev = previous_parent;
		previous_parent = ascending_bone->parent;
		ascending_bone = bones + ascending_bone->parent;
	}while(prev != previous_parent);
}

static void
editor_model_apply_ik_new(
		devexp_state *program,
		game_renderer *game_renderer,
		vec3 target,
		render_commands *commands)
{
#if 1
	//TODO: gather lengths!
	exp_bone *bones = program->bones;
	exp_bone *bone = bones + program->bone_to_ik;
	u16 selected_bone_index = program->bone_to_ik;
	//start with the parent of the target bone
	exp_bone *ascending_bone = bones + bone->parent;
	target = program->ik_vec;
	//distance between target and end position
	//no ascendants, no ik
	if(ascending_bone == bone)
	{
		return;
	}

	temporary_area tarea = temporary_area_begin(program->area);

	u32 number_of_passes = 1000;
	//backward_points
	vec3 *points_b = memory_area_clear_and_push_array(program->area,
			vec3, program->bone_count);
	//foward points
	vec3 *points_f = memory_area_clear_and_push_array(program->area,
			vec3, program->bone_count);
	//lengths
	f32 *distances = memory_area_push_array(program->area, f32 ,program->bone_count);
	//indices for last foward pass
	u16 *indices = memory_area_push_array(program->area, u16, program->bone_count);

	f32 b = 0;
	f32 total_hierarchy_length = 0;
	u32 index = program->bone_count - 1;
	distances[index] = vec3_length(bone->p);
	indices[index] = bone->parent;
	points_f[index--] = bone->transformed_p;
	total_hierarchy_length = vec3_length(bone->p);
	//
	//fill foward points and lengths
	//
	//
	{
		//make sure to not loop twice on the "root"
		u32 prev_parent_index = bone->parent;
		u32 last_parent_index = prev_parent_index;
		do
		{
			points_f[index] = ascending_bone->transformed_p;
			distances[index] = vec3_length(ascending_bone->p);
			total_hierarchy_length += index ? distances[index] : 0;
			indices[index] = ascending_bone->parent;
			index--;

			last_parent_index = prev_parent_index;
			prev_parent_index = ascending_bone->parent;

			ascending_bone = bones + ascending_bone->parent;
		}while(last_parent_index != prev_parent_index);
	}

	
	exp_bone *root_bone = ascending_bone;
	
	//
	//set target inside the reach
	//
	vec3 distance_target_root = vec3_sub(target, root_bone->transformed_p);
	f32 distance_tr_length = vec3_length(distance_target_root);
	if(total_hierarchy_length < distance_tr_length)
	{
		target = vec3_scale(vec3_normalize(distance_target_root), total_hierarchy_length);
		target = vec3_add(target, root_bone->transformed_p);
	}
	index = program->bone_count - 1;
	//can't reach target!
	points_b[index] = target;
	vec3 f_end = points_f[index];
	{
		u32 passes_index = 0;
		f32 distance_end_target = vec3_length(vec3_sub(target, f_end));
		while(passes_index < 1000 && distance_end_target > 0.001f)
		{
			//loop back and forth
			index = program->bone_count - 1;
			vec3 end = points_b[index];
			//backward pass
			while(index)
			{
				u32 parent_i = index - 1;
				//backwards, start from end and adjust coordinates
				vec3 p0 = points_b[index];
				vec3 p1 = points_f[parent_i];
				vec3 distance_parent_bone = vec3_sub(p1, p0);
				vec3 distance_parent_bone_n = vec3_normalize(distance_parent_bone);
				f32 dpb_length = distances[index];
				//new position of parent
				vec3 p_parent = vec3_scale(distance_parent_bone_n, dpb_length);
				p_parent = vec3_add(end, p_parent);
				end = p_parent;
				points_b[parent_i] = p_parent;

				index--;
			}
			index = 0;
			//foward pass
			while(index < program->bone_count - 1)
			{
				u32 child_i = index + 1;
				vec3 p0 = points_f[index];
				vec3 p1 = points_b[child_i];
				vec3 distance_p10 = vec3_normalize(vec3_sub(p1, p0));
				f32 dpb_length = distances[child_i];
				vec3 p2 = vec3_add(p0, vec3_scale(distance_p10, dpb_length));
				points_f[child_i] = p2;
				index++;
			}
			passes_index++;

			f_end = points_f[program->bone_count - 1];
			distance_end_target = vec3_length(vec3_sub(target, f_end));
		}

		index = 0;
		while(index < program->bone_count2d)
		{
			vec3 point = points_f[index];
			render_draw_cube(commands,
					point,
					vec3_all(3),
					V4(255, 0, 0, 255));


			index++;
		}
		//fill rotations
		f32 total_angle = 0;
		//total rotation of quaternions
		quaternion q_total = {1, 0, 0, 0};
		for(u32 ni = 1; ni < program->bone_count;
				ni++)
		{
			exp_bone *bone = bones + indices[ni];
			exp_bone *child = bones + indices[ni + 1];
			//hierarchy_p = vec2_add(child->p);
			vec3 p0 = points_f[ni - 1];
			vec3 p1 = points_f[ni];
			vec3 d_rp = vec3_normalize(vec3_sub(p1, p0));
			//convert unit vectors to quaternions (function on gmmath, explanation on math study)
			bone->q = quaternion_from_vector(d_rp);
//			bone->q = quaternion_rotated_at(d_rp.x, d_rp.y, d_rp.z, PI);
			vec3 up = {0, 0, 1}; 
			vec3 cross = vec3_cross(up, d_rp);
			f32 w = vec3_inner(up, d_rp);
			if(w == -1)
			{
				bone->q = QUAT(1, 0, 0, 0);
			}
			else
			{
//				quaternion q = quaternion_from_vectors(up, d_rp);
				bone->q.vector = cross;
				bone->q.w = 1.0f + w;
				bone->q = quaternion_normalize_safe(bone->q);
				//cancel total rotation
				bone->q = quaternion_mul(quaternion_conjugate(q_total), bone->q);
			}
			q_total = quaternion_mul(q_total, bone->q);
		}
	}


	temporary_area_end(&tarea);
	//l = individual arm length
	//d = total_distance [0, l * 2]
	//shand = (cos(arm_angle) * d, sin(arm_angle) * d)
#endif
}




typedef struct{
	vec3 rotation;
	f32 angle;
	vec3 p;
	vec3 displacement;
	vec3 p_before;
	vec3 unrotated;
	quaternion q;
}exp_model_transform;











static exp_model_transform
exp_get_foward_transform_quaternion(
		exp_bone *bone_array,
		u32 bone_index)
{

	vec3 bone_transformed_position = {0}; 
	vec3 unrotated_end_position = {0};
	vec3 hierarchy_offset   = {0};
	quaternion final_rotated_quaternion = {1, 0, 0, 0};

//	exp_bone *bone_array = model.bones;
	exp_bone *target_bone = bone_array + bone_index;
	u32 isRoot = bone_index == 0 || (bone_index == target_bone->parent);
	if(!isRoot)
	{
	    u32 parent_index = target_bone->parent;
	    exp_bone *parent_bone = bone_array + target_bone->parent;
		final_rotated_quaternion = parent_bone->q;

	    u32 parentIsRoot      = parent_bone == (bone_array + parent_index);
		u32 last_parent_index = bone_index;
	    //loop start
	    do
	    {

	        vec3 parent_position     = parent_bone->p;
	        vec3 parent_displacement = parent_bone->displacement;

            bone_transformed_position = vec3_add(bone_transformed_position, parent_position);
	        unrotated_end_position = vec3_add(unrotated_end_position, parent_position);
	        hierarchy_offset   = vec3_add(hierarchy_offset, parent_displacement);

	        //the root bone points to itself
	         parentIsRoot = parent_index == 0 ||
		   	                parent_index == bone_index ||
		   				    parent_bone->parent == last_parent_index;

			last_parent_index = parent_index;
	        //now get the parent from the parent's model;
	        parent_index = parent_bone->parent;

	        //if the parent bone has another parent,
			//then rotate the current one by it's angle
	        if(!parentIsRoot)
	        {
               parent_bone = bone_array + parent_index;

			   quaternion ascending_quaternion = parent_bone->q;
		       final_rotated_quaternion = quaternion_mul(ascending_quaternion, final_rotated_quaternion);
	      	 //rotate the current position by the previous rotation
	           bone_transformed_position = quaternion_v3_mul_foward_inverse(
					   ascending_quaternion, bone_transformed_position);
	      			                       
	        }
	    }while(!parentIsRoot);

	}
//	quaternion h_q = quaternion_create_rotation_xyz(
//			total_rotation_x, total_rotation_y, total_rotation_z);
	quaternion h_q = final_rotated_quaternion;


	//the offset of the bone depends on the total rotation.
	vec3 target_bone_offset = quaternion_v3_mul_foward_inverse(h_q, target_bone->p); 
	//add displacements, those should be zero if this was a bind model
	target_bone_offset = vec3_add(target_bone_offset, target_bone->displacement);
	target_bone_offset = vec3_add(target_bone_offset, hierarchy_offset);
	//Add the final rotated offset
	bone_transformed_position = vec3_add(bone_transformed_position, target_bone_offset); 
	unrotated_end_position = vec3_add(target_bone->p, unrotated_end_position);

    exp_model_transform result = {0};
	//end rotated position
    result.p = bone_transformed_position;
    result.displacement = vec3_sub(bone_transformed_position, unrotated_end_position);
	result.q = final_rotated_quaternion;
	result.unrotated = unrotated_end_position;
    return(result);
}

static exp_model_transform
exp_model_get_foward_transform_quaternion(
		exp_bone *bone_array,
		u32 bone_index)
{
//	exp_bone *bone_array = model.bones;
	exp_bone *target_bone = bone_array + bone_index;
	exp_model_transform result = {0};
	//get the hierarchy transform of the parent
	exp_model_transform parent_transform = exp_get_foward_transform_quaternion(
		    bone_array,	
			target_bone->parent);
	if(target_bone->parent != bone_index)
	{
		exp_bone *parent_bone = bone_array + target_bone->parent;

		result.q = quaternion_mul(parent_transform.q, parent_bone->q);

		vec3 p_add = quaternion_v3_mul_foward_inverse(result.q, target_bone->p);
		result.p_before = parent_transform.p;
		vec3 total_transform = vec3_add(parent_transform.p, p_add);

		result.displacement = p_add; 
		result.p = total_transform;
	}
	else
	{
		return(parent_transform);
		
	}
	return(result);
}


static void
exp_fill_bone_transformed_data(
		game_renderer *game_renderer,
		devexp_state *program)
{
	u32 bone_count = program->bone_count;
	exp_bone *bones = program->bones;
	for(u32 b = 0; b < bone_count; b++)
	{
		exp_bone *bone = bones + b;
		bone->normal = V3(0, -1, 0);
	}

	for(u32 b = 0; b < bone_count; b++)
	{
		exp_bone *bone = bones + b;
		exp_model_transform bone_transform = 
			exp_model_get_foward_transform_quaternion(
					bones,
					b);

		//cancel this if parent is the same?
		bone->transformed_q = quaternion_mul(bone_transform.q, bone->q);
		vec3 dir_default = {0, 1, 0};
		bone->normal =  quaternion_v3_mul_foward_inverse(bone->transformed_q, bone->normal);
		bone->transformed_p = bone_transform.p;
		bone->transformed_p = vec3_add(bone->transformed_p, bone->displacement);
	}
}

















static void
update_camera(game_renderer *game_renderer,
		devexp_state *program,
		editor_input *editor_input)
{
    u16 ui_is_interacting  = ui_any_interaction(program->ui);
    u16 ui_focused         = 0;//program->ui_is_focused; 
    u32 input_text_focused   = program->ui->input.input_text.focused;

    u32 mouse_r_down    = input_down(editor_input->mouse_right);
    u32 mouse_l_down    = input_down(editor_input->mouse_left);

	u32 lock_camera_rotation = input_text_focused;

    f32 mouse_delta_x = editor_input->mouse_clip_x_last - editor_input->mouse_clip_x;
    f32 mouse_delta_y = editor_input->mouse_clip_y_last - editor_input->mouse_clip_y;

	f32 camera_rotation_x = game_renderer->camera_rotation.x;
	f32 camera_rotation_z = game_renderer->camera_rotation.z;

        //if(!lock_camera_rotation && program->platform->window_is_focused)
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

	if(!ui_is_interacting)
	{

	    //camera rotation

		program->in_camera_mode = 0;
        if(!lock_camera_rotation && program->platform->window_is_focused)
        {

		    camera_rotation_x = program->camera_rotation_x;
		    camera_rotation_z = program->camera_rotation_z;
            if(editor_input->spaceBarDown && mouse_l_down)
            {
			    program->in_camera_mode = 1;

                camera_rotation_x += ((f32)mouse_delta_y * PI * 0.0005f); 
                camera_rotation_z += ((f32)mouse_delta_x * PI * 0.0005f); 
            }

	        camera_rotation_x = camera_rotation_x < 0 ? 2.0f : camera_rotation_x >= 2.0f ? 0 : camera_rotation_x;
	        camera_rotation_z = camera_rotation_z < 0 ? 2.0f : camera_rotation_z >= 2.0f ? 0 : camera_rotation_z;

            program->camera_rotation_x = camera_rotation_x;
            program->camera_rotation_z = camera_rotation_z;
	        game_renderer->camera_rotation.x = camera_rotation_x;
	        game_renderer->camera_rotation.z = camera_rotation_z;

        }
	

        real32 camSpeed = 6.0f;

	    if(!input_text_focused)
	    {
			vec3 camera_p = program->camera_p;
			{

				i32 mouseWheelValue = editor_input->mouse_wheel * 10;
		        if(!ui_focused)
		        {
		            program->camera_distance_from_cursor -= mouseWheelValue;

				    if(program->camera_distance_from_cursor < 0)
				    {
                        program->camera_distance_from_cursor = 0;

				    	//instead, add the new value to the pad location
				    	vec3 add_zAxis = vec3_scale(game_renderer->camera_z, (f32)mouseWheelValue);

						program->camera_pad_position.x -= add_zAxis.x;
						program->camera_pad_position.y -= add_zAxis.y;
						program->camera_pad_position.z -= add_zAxis.z;
				    }
		        }

				f32 camera_distance_from_cursor = program->camera_distance_from_cursor;
				vec3 camera_distance_from_cursor_vec = {0, 0, camera_distance_from_cursor};


		        matrix4x4 camera_rotation = matrix4x4_rotation_scale(game_renderer->camera_rotation.x,
		   		                                                    game_renderer->camera_rotation.y,
		   						                                    game_renderer->camera_rotation.z);

		        camera_distance_from_cursor_vec = matrix4x4_v3_mul_rows(
						                       camera_rotation,
						                       camera_distance_from_cursor_vec,
											   0);

				camera_p = vec3_add(camera_distance_from_cursor_vec,
						                  program->camera_pad_position);

				//move padding position
				if(editor_input->spaceBarDown && mouse_r_down)
				{
					f32 mouse_delta_x = editor_input->mouse_x - editor_input->mouse_x_last;
					f32 mouse_delta_y = editor_input->mouse_y - editor_input->mouse_y_last;

					f32 paddingSpeed = 0.3f;

					vec3 add_xAxis = vec3_scale(game_renderer->camera_x, mouse_delta_x * paddingSpeed);
					vec3 add_yAxis = vec3_scale(game_renderer->camera_y, mouse_delta_y * paddingSpeed);

					program->camera_pad_position.x -= add_xAxis.x - add_yAxis.x;
					program->camera_pad_position.y -= add_xAxis.y - add_yAxis.y;
					program->camera_pad_position.z -= add_xAxis.z - add_yAxis.z;
				}
			}


            program->camera_p = camera_p;
            game_renderer->camera_position = program->camera_p;
	    }
	}

	game_renderer->camera_rotation.x = camera_rotation_x;
	game_renderer->camera_rotation.z = camera_rotation_z;

	render_update_camera_rotation(game_renderer);
}

static void
update_render_ui_triangle_coords(devexp_state *program, editor_input *input, game_renderer *game_renderer);
static void
update_render_ui_ik(devexp_state *program, editor_input *input, game_renderer *game_renderer);
static void
update_render_ui_ui_fix(devexp_state *program, editor_input *input, game_renderer *game_renderer);
static void
update_render_ui_angle_dir(devexp_state *program, editor_input *input, game_renderer *game_renderer);
static void
update_render_ui_poisson(devexp_state *program, editor_input *input, game_renderer *game_renderer);
static void
update_render_ui_brains(devexp_state *program, editor_input *input, game_renderer *game_renderer, f32 dt);
static void
update_render_cosos(devexp_state *program, editor_input *input, game_renderer *game_renderer);

static void
update_render_ui_ui_fix(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_node *box = 0;
		ui_set_wh_ppct(ui, .5f, 1.0f)
		{
			box = ui_node_box(ui, 0);
		}
		ui_set_parent(ui, box)
		{
			static u32 vs[3] = {0};
			ui_set_row(ui)
			{
				ui_textf(ui, "v0:%u v1:%u v2:%u",
						vs[0], 
						vs[1], 
						vs[2]
						);
			}
			for(u32 x = 0; x < 3; x++)
			{
				ui_push_id_u32(ui, x);
				ui_id popid = ui_node_id_from_string(ui, "popup_fix");
				ui_set_w_em(ui, 12.0f, 1.0f)
				ui_drop_down(ui, popid, "dd") ui_set_h_text(ui, 4.0f, 1.0f)
				{
					for(u32 y = 0; y < 3; y++)
					{
						if(ui_selectablef(ui, 0, "option%u", y))
						{
							vs[x] = y;
							ui_popup_close(ui, popid);
						}
					}
				}
				ui_pop_id(ui);
			}
		}
		ui_text(ui, "Another popup");
		ui_set_w_em(ui, 12.0f, 1.0f)
		{
			ui_id popid_main = ui_node_id_from_string(ui, "popup_fix_main");
			ui_popup_open(ui, 100, 100, popid_main);
			ui_popup(ui, popid_main)
			{
				ui_node *box = 0;
				ui_set_wh_soch(ui, 1.0f) box = ui_node_box(ui, 0);
				ui_id popid = ui_node_id_from_string(ui, "popup_fix0");
				ui_drop_down(ui, popid, "dd 0") ui_set_h_text(ui, 4.0f, 1.0f)
				{
					for(u32 y = 0; y < 3; y++)
					{
						if(ui_selectablef(ui, 0, "option%u", y))
						{
						}
					}
				}
				ui_id popid1 = ui_node_id_from_string(ui, "popup_fix1");
				if(ui_button(ui, "Press me!"))
				{
					ui_popup_open_or_close(ui, 200, 100, popid1);
				}
				ui_popup(ui, popid1)
				{
					ui_node *box1 = 0;
					ui_set_wh_soch(ui, 1.0f) box1 = ui_label(ui, 0);
					for(u32 t = 0; t < 4; t++)
					{
						ui_textf(ui, "test %u", t);
					}
				}
			}
		}

	}
}
static void
update_render_ui_ik(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_set_w_specified(ui, 200.0f, 1.0f) ui_set_h_soch(ui, 1.0f)
		{
			//3d
			ui_set_row(ui)
			{
				if(!program->two_dim)
				{
						ui_space_ppct(ui, .3f, 1.0f);
					ui_node *lbox = ui_node_box(ui, 0);
					ui_set_parent(ui, lbox) ui_set_wh_text(ui, 4.0f, 1.0f)
					{
						ui_text(ui, "kinematic_point");
						b32 changed = 0;
						ui_set_row(ui)
						{
							changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &program->ik_vec.x, "selected_ik_pos_x");
							changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &program->ik_vec.y, "selected_ik_pos_y");
							changed |= ui_drag_f32(ui, 0.1f, -10000.0f, 10000.0f, &program->ik_vec.z, "selected_ik_pos_z");
						}
						ui_checkbox(ui, &program->apply_ik, "apply_ik");
						if(ui_button(ui, "set to last bone"))
						{
							exp_bone *bone = program->bones + program->bone_to_ik;
							program->ik_vec = bone->transformed_p;
						}
					}
				}
				else
				{
					ui_set_wh_soch(ui, 1.0f)
					{
						ui_node *bones_box = ui_node_box(ui, 0);
						ui_set_parent(ui, bones_box) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							for(u32 b = 0; b < program->bone_count2d; b++)
							{
								exp_bone_2d *bone = program->bones_2d + b;
								ui_textf(ui, "bone%u rot:");
								ui_push_id_u32(ui, b);
								ui_drag_f32(ui, PI / 32.0f, -PI, PI, &bone->rot, "bone_rot");
								ui_pop_id(ui);
							}
						}
					}
					ui_space_ppct(ui, .3f, 1.0f);
					ui_node *lbox = ui_node_box(ui, 0);
					ui_set_parent(ui, lbox) ui_set_wh_text(ui, 4.0f, 1.0f)
					{
						ui_text(ui, "kinematic_point 2d");
						b32 changed = 0;
						ui_set_row(ui)
						{
							changed |= ui_drag_f32(ui, 1.0f, -10000.0f, 10000.0f, &program->ik_vec_2d.x, "selected_ik_pos2d_x");
							changed |= ui_drag_f32(ui, 1.0f, -10000.0f, 10000.0f, &program->ik_vec_2d.y, "selected_ik_pos2d_y");
						}
						ui_checkbox(ui, &program->apply_ik_2d, "apply_ik_2d");
						if(ui_button(ui, "set to last bone"))
						{
							exp_bone_2d *bone = program->bones_2d + program->bone_to_ik;
							program->ik_vec_2d = bone->transformed_p;
						}
						ui_textf(ui, "point_angle: %f", program->point_angle);
						ui_set_w_em(ui, 12.0f, 1.0f)
						{
							ui_node *drag_button = ui_create_node(ui, node_clickeable | node_text | node_background | node_hover_animation | node_active_animation, "Drag me");
							if(ui_node_mouse_l_down(ui, drag_button))
							{
								program->point_angle += ui_mouse_delta(ui).x * 0.04f * PI;
							}
						}
						vec2 point_direction = {cos32(program->point_angle), sin32(program->point_angle)};
						f32 point_wall_dot = vec2_inner(point_direction, program->wall_normal);
						ui_textf(ui, "dot product: %f", point_wall_dot);
					}
				}
				//2d
				ui_space_ppct(ui, 1.0f, 0.0f);
				ui_node *cb_panel = 0;
				ui_set_wh_soch(ui, 1.0f)
				{
					cb_panel = ui_node_box(ui, 0);
				}
				ui_set_parent(ui, cb_panel)
				{
					ui_checkbox(ui, &program->two_dim, "2d mode");
				}
			}
		}

		ui_set_wh_ppct(ui, 1.0f, 1.0f)
		{
		}
	}
}

static void
update_render_ui_angle_dir(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_node *cb_panel = 0;
		ui_set_wh_soch(ui, 1.0f)
		{
			cb_panel = ui_node_box(ui, 0);
		}
		ui_set_parent(ui, cb_panel)
		{
			ui_checkbox(ui, &program->two_dim, "2d mode");
			vec2 cp = program->center_point; 
			vec2 mcp = vec2_sub(input->mouse_clip, cp);

			f32 dir = program->max_directions / 2.0f;
			//offset by this dir value.
			f32 mcp_angle_start = 0 + (PI / dir) / 2;
			f32 mcp_angle = mcp_angle_start + arctan232(mcp.x, mcp.y);
			if(mcp_angle < 0)
			{
				mcp_angle = mcp_angle + (PI * 2);
			}

			f32 angle_dir = (PI) / dir;
			i32 index = (i32)(mcp_angle / angle_dir);

			ui_textf(ui, "mcp angle: %f", mcp_angle);
			ui_textf(ui, "direction_index: %d", index);
		}
	}

	ui_set_wh_ppct(ui, 1.0f, 1.0f)
	{
	}
}

static f32
edge_function(vec2 p,vec2 v0, vec2 v1)
{
	f32 result = (p.x - v0.x) * (v1.y - v0.y) - (p.y - v0.y) * (v1.x - v0.x);
	return(result);
}
 
typedef struct{
	f32 c01;
	f32 c12;
	f32 c20;
}barycentric_coords;

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
area_tri(vec2 p,vec2 v0, vec2 v1)
{
	f32 result = edge_function(p, v0, v1);

	return(result);
}

static void
update_render_ui_triangle_coords(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_node *cb_panel = 0;
		ui_set_wh_soch(ui, 1.0f)
		{
			cb_panel = ui_node_box(ui, 0);
		}
		ui_set_parent(ui, cb_panel)
		{
			ui_checkbox(ui, &program->two_dim, "2d mode");
			vec2 cp = program->center_point; 
			vec2 mcp = vec2_sub(input->mouse_clip, cp);

			ui_textf(ui, "point: {%f, %f}", program->point.x, program->point.y);
			vec2 p = program->point;
			vec2 v0 = program->l0;
			vec2 v1 = program->l1;
			vec2 v2 = program->l2;
			f32 r01 = edge_function(p, v1, v0);
			f32 r12 = edge_function(p, v2, v1);
			f32 r20 = edge_function(p, v0, v2);
			f32 area = ABS(area_tri(v0, v1, v2));
			vec3 barycentric_coords = get_barycentric_coordinates(p, v0, v1, v2);
			f32 a01 = barycentric_coords.x;
			f32 a12 = barycentric_coords.y;
			f32 a20 = barycentric_coords.z;
			ui_textf(ui, "r01: {%f}\nr12: {%f}\nr20 {%f}\ninside: %u", r01, r12, r20, (r01 > 0 && r12 > 0 && r20 > 0));
			ui_textf(ui, "total area: %f, others {%f, %f, %f} sum: %f", area, a01, a12, a20, (a01 + a12 + a20));
		}
	}
}

static void
update_render_ui_poisson(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
	game_ui *ui = program->ui;
	ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		ui_node *cb_panel = 0;
		ui_set_wh_soch(ui, 1.0f)
		{
			cb_panel = ui_node_box(ui, 0);
		}
		ui_set_parent(ui, cb_panel)
		{
			ui_checkbox(ui, &program->two_dim, "2d mode");
			if(ui_button(ui, "generate_poisson"))
			{
				program->generate_poisson = 1;
			}
			ui_text(ui, "sampling_r");
			if(ui_spinner_f32(ui, 1, program->min_sampling, 100, &program->sampling_r, 0, "poisson_sampling_r"))
			{
				program->cell_wh = (f32)program->sampling_r / sqrt32(2);
			}
		}
	}

	ui_set_wh_ppct(ui, 1.0f, 1.0f)
	{
	}
}


static state_line *
brain_get_state_line(state_main *brain, u32 index)
{
	state_node *current_state = brain->states + brain->current;
	state_line *line = brain->state_lines + current_state->state_do_at + index;
	return(line);
}

static state_action_line *
brain_action_line(state_main *brain, state_action *action, u32 index)
{
	state_action_line *al = brain->action_lines + action->action_lines_at + index;
	return(al);
}

static void
update_render_ui_brains(devexp_state *program, editor_input *input, game_renderer *game_renderer, f32 dt)
{
	game_ui *ui = program->ui;
	ui_node *box = 0;
	ui_set_wh_soch(ui, 1.0f) box = ui_node_box(ui, 0);
	ui_set_parent(ui, box) ui_set_wh_text(ui, 4.0f, 1.0f)
	{
		Assert(dt < 0.09f);
		ui_textf(ui, "target_framerate");
		ui_spinner_f32(ui, 15, 15, 60, &program->target_framerate, 0, "target_framerate_p");
		ui_textf(ui, "elapsed_ms: %f9", program->elapsed_ms);
		ui_textf(ui, "dt: %f9", dt);
	}
}

static void
update_render(devexp_state *program, editor_input *input, game_renderer *game_renderer, f32 dt)
{
	if(!program->two_dim)
	{
		render_commands *commands = render_commands_begin_default(game_renderer);

		exp_fill_bone_transformed_data(
				game_renderer,
				program);
		if(program->apply_ik)
		{
			editor_model_apply_ik_new(
					program,
					game_renderer,
					vec3_all(0),
					commands);
		}
		vec4 bone_color = {25, 95, 0, 255};
		//draw bones
		for(u32 b = 0; b < program->bone_count; b++)
		{
			exp_bone *bone = program->bones + b;

			if(b != 0)
			{
				exp_bone *parent = program->bones + bone->parent;
				f32 line_alpha = 255;
				render_draw_line_up(commands,
						bone->transformed_p,
						parent->transformed_p,
						bone_color,
						0.08f);
			}
			render_circle_upfront(commands,
					bone->transformed_p, 
					.5f,
					.2f,
					bone_color);
		}
		//draw kinematic point
		render_draw_cube(commands, program->ik_vec, vec3_all(2), V4(255, 0, 0, 255));






		//background

#if 1
		vec4 backRectangleColor = {20, 20, 20, 255};
		render_draw_rectangle_colored_axes_sized(commands,
				V3(0, 0, 0),
				V3(1024, 0, 0),
				V3(0, 1024, 0),
				backRectangleColor);

		f32 model_bk_size = 1024.0f;
		vec3 background_edge = {
			0 - 1024 * 0.5f, 0 - 1024 * 0.5f, 0};

		f32 line_x = background_edge.x;
		f32 line_y = background_edge.y;
		for(u32 y = 0; y  < 32; y++)
		{
			render_draw_line(
					commands,
					V3(line_x, line_y, 0.5f),
					V3(line_x + model_bk_size, line_y , 0.3f),
					V3(0, 0, 1.0f),
					V4(120, 120, 120, 120),
					0.5f,
					1);
			line_y += 32;
		}
		line_x = background_edge.x;
		line_y = background_edge.y;
		for(u32 x = 0; x < 32; x++)
		{
			//f32 line_x 
			render_draw_line(
					commands,
					V3(line_x, line_y, 0.5f),
					V3(line_x, line_y + model_bk_size, 0.3f),
					V3(0, 0, 1.0f),
					V4(120, 120, 120, 120),
					0.5f,
					1);
			line_x += 32;
		}

		update_camera(game_renderer, program, input);
#endif
		render_commands_end(commands);
	}
	else
	{
		//devexp_update_normals_2d(program, input, game_renderer);
		//devexp2d_update_dirs(program, input, game_renderer);
		devexp2d_update_poisson(program, input, game_renderer, dt);
	}
}

static void
update_render_triangle_coords(devexp_state *program, editor_input *input, game_renderer *game_renderer, f32 dt)
{
	program->point = input->mouse_clip;
	vec2 p = program->point;
	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

	render_rectangle_2d_xywh(commands, p.x, p.y, 40, 40, V4(255, 0, 0, 255));
	//draw points
	vec2 p0 = {40, 800};
	vec2 p1 = {500, 400};
	vec2 p2 = {800, 800};

	vec4 color = {255, 255, 0, 255};
	render_line_2d_center(commands, p0, p1, 1.0f, color);
	render_line_2d_center(commands, p1, p2, 1.0f, color);
	render_line_2d_center(commands, p2, p0, 1.0f, color);
	program->l0 = p0;
	program->l1 = p1;
	program->l2 = p2;

	render_commands_end(commands);
}

static void
update_render_ui(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{
}

GUPDATE_RENDER(gprogram_progress)
{
	if(dt > 0.05f)
	{
		dt = 0;
	}

	devexp_state *program = program_memory->program;
	//allocate program
	if(!program)
	{
		program_memory->program = memory_area_clear_and_push_struct(
				program_memory->main_area, devexp_state);
		program = program_memory->program;
		program->platform = program_memory->platform;
		program->ui = program_memory->ui;
		program->program_w = game_renderer->back_buffer_width;
		program->program_h = game_renderer->back_buffer_height;

		game_resource_initial_settings editor_resource_settings = {0};
		editor_resource_settings.total_slots = assets_MAX_ASSET_CAPACITY;
		editor_resource_settings.allocation_size = MEGABYTES(12);
		editor_resource_settings.operations_size = MEGABYTES(4);
		editor_resource_settings.dev_build = 1;
		editor_resource_settings.resource_folder_path = "";
		//start up without allocating any initial assets
		editor_resource_settings.initial_assets_count = 0;
		editor_resource_settings.use_pack = 0;


		program->assets = assets_allocate_game_assets(
				program_memory->main_area,
				program->platform,
				&game_renderer->texture_operations,
				editor_resource_settings,
				&program_memory->log_area);

		program->ui_font = assets_load_and_get_font(program->assets,
				"magus.pfnt");

		program->camera_p.z = 20.0f;
		program->camera_distance_from_cursor = 60.0f;
		program->bones = memory_area_push_array(program_memory->main_area, exp_bone, 50);
		program->bones_2d = memory_area_push_array(program_memory->main_area, exp_bone_2d, 50);
		program->area = program_memory->main_area;

		program->bone_count = 4;
		//set 3d bone parameters
		{
			exp_bone *bone = program->bones + 0;
			bone->p.z = 5.0f;
			bone->q = quaternion_identity();

			bone = program->bones + 1;
			bone->p.z = 5.0f;
			bone->q = quaternion_identity();

			bone = program->bones + 2;
			bone->p.z = 5.0f;
			bone->q = quaternion_identity();
			bone->parent = 1;

			bone = program->bones + 3;
			bone->p.z = 5.0f;
			bone->q = quaternion_identity();
			bone->parent = 2;

			program->bone_to_ik = 3;
		}

		program->bone_count2d = 4;
		//set 2d bone parameters
		{
			exp_bone_2d *bone = program->bones_2d + 0;
			bone->p.x = 50.0f;
			bone->p.y = 500.0f;
			bone->rot = -PI / 4;

			bone = program->bones_2d + 1;
			bone->p.y = 50.0f;
			bone->rot = 0;

			bone = program->bones_2d + 2;
			bone->p.y = 50.0f;
			bone->rot = 0;
			bone->parent = 1;

			bone = program->bones_2d + 3;
			bone->p.y = 50.0f;
			bone->rot = 0;
			bone->parent = 2;

			program->bone_to_ik_2d = 3;
		}

		//dot test
		program->point.x = 500;
		program->point.y = 500;
		program->wall_p.x = 700;
		program->wall_p.y = 500;
		program->wall_normal = V2(-1, 0);
		
		//angle directions
		program->max_directions = 16;
		program->center_point.x = 800;
		program->center_point.y = 400;

		//poisson
		if(0)
		{
			program->sampling_r = 20;
			program->cell_wh = (u32)program->sampling_r / sqrt32(2);
			program->min_sampling = (f32)program->cell_wh;
			program->grid_w = (u16)(game_renderer->back_buffer_width / program->cell_wh);
			program->grid_h = (u16)(game_renderer->back_buffer_height / program->cell_wh);
			program->grid = memory_area_clear_and_push_array(program_memory->main_area, i16, 
					program->grid_w * program->grid_h);
			u32 grid_count = program->grid_w * program->grid_h;
			for(u32 g = 0; g < grid_count; g++)
			{
				program->grid[g] = -1;
			}
			program->active_points_max = 4000;
			program->active_points = memory_area_clear_and_push_array(program_memory->main_area, i16,
					program->active_points_max);
			program->random_s = random_series_create(28349283);
			program->final_points_max = 4000;
			program->final_points = memory_area_clear_and_push_array(program_memory->main_area, vec2, 
					program->active_points_max);
			program->final_colors = memory_area_clear_and_push_array(program_memory->main_area, u8, 
					program->active_points_max);


			//add point in the grid 
			program->active_points_count++;
			program->final_points_count++;
			vec2 *fac = program->final_points + 0;
			program->active_points[0] = 0;
			fac->x = random_get_f32_between(&program->random_s,
					0, game_renderer->back_buffer_width);
			fac->y = random_get_f32_between(&program->random_s,
					0, game_renderer->back_buffer_height);

			//based on the first point activate the cell it felt in 
			u32 grid_x = (u32)(fac->x / program->cell_wh);
			u32 grid_y = (u32)(fac->y / program->cell_wh);
			u32 grid_index = grid_x + (grid_y * program->grid_w);
			program->grid[grid_index] = 0;

			//points to generate
			program->points_gen_count = 30;
			program->generate_poisson = 1;
		}

		//game prototype stuff


		//
	}
	//3d/2d stuff
	if(!program->target_framerate) program->target_framerate = 60;
		global_target_framerate = program->target_framerate;

	program_memory->target_ms = calc_target_ms(program->target_framerate);//1.0f / (program->target_framerate * 2);
	program->elapsed_ms = program_memory->elapsed_ms;

	render_commands *commands = render_commands_begin_default(game_renderer);
	game_renderer->clear_color[0] = 0;
	game_renderer->clear_color[1] = 0;
	game_renderer->clear_color[2] = 0;
	render_commands_clear_graphics(commands);

	render_commands_end(commands);

	//update_render(program, input, game_renderer, dt);
	update_render_triangle_coords(program, input, game_renderer, dt);
	//update_render_bodies(program, input, game_renderer, dt);
	//update_render_entities(program, input, game_renderer, dt);
	//end render
	game_renderer->fov = 25.0f;
	render_update_camera_values(game_renderer, 16, 9);

	//ui stuff
	game_ui *ui = ui_begin_frame(
			program_memory->ui,
			program->ui_font,
			program->platform,
			game_renderer,
			dt);


	ui_widget_region_begin(ui,
			0,
			0,
			game_renderer->os_window_width,
			game_renderer->os_window_height,
			"widget region");
	//update_render_ui_ik(program, input, game_renderer);
	update_render_ui_triangle_coords(program, input, game_renderer);
//	update_render_ui_ui_fix(program, input, game_renderer);
	//update_render_ui_angle_dir(program, input, game_renderer);
//	update_render_ui_poisson(program, input, game_renderer);
	//update_render_ui_physics(program, input, game_renderer);
//	update_render_ui_brains(program, input, game_renderer, dt);
	ui_widget_region_end(ui);

	ui_end_frame(ui, program->platform->window_is_focused);
}


int dll_main()
{
	return(1);
}

