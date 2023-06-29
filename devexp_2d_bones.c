typedef struct exp_bone_2d{
	u16 parent;
	vec2 p;
	vec2 transformed_p;
	vec2 displacement;
	vec2 normal;

	f32 rot;

	f32 transformed_rot;

	union{
		vec2 transformed_displacement;
		struct{
	f32 transformed_displacement_x;
	f32 transformed_displacement_y;
	f32 transformed_displacement_z;
		};
	};
}exp_bone_2d;

static void
editor_model_apply_ik2d2(
		devexp_state *program,
		game_renderer *game_renderer,
		vec2 target)
{
#if 1
	//s_model_editor *model_editor = &editor->model;
	exp_bone_2d *bones = program->bones_2d;

	exp_bone_2d *bone = bones + program->bone_to_ik;
	target = program->ik_vec_2d;
	exp_bone_2d *parent_bone = bones + bone->parent;
	//distance between target and end position
	exp_bone_2d *ascending_bone = parent_bone;
	u32 prev_parent_index = bone->parent;
	u32 last_parent_index = prev_parent_index;
	//no ascendants
	if(ascending_bone == bone)
	{
		return;
	}

	f32 b = 0;
	do
	{

		b += vec2_length(ascending_bone->p);

		last_parent_index = prev_parent_index;
		prev_parent_index = ascending_bone->parent;
		ascending_bone = bones + ascending_bone->parent;
	}while(last_parent_index != prev_parent_index);
	exp_bone_2d *root_bone = ascending_bone;


	ascending_bone = parent_bone;
	prev_parent_index = bone->parent;
	last_parent_index = prev_parent_index;
	vec2 au = vec2_normalize(bone->p);
	vec2 r = {0};
	f32 angle = 0;
	do
	{
		parent_bone = bones + ascending_bone->parent;
		f32 a = 50.0f;//vec2_length(ascending_bone->p);
		b -= a;//vec2_length(ascending_bone->p);
		if(1)
		{
			b = vec2_length(vec2_sub(parent_bone->transformed_p, root_bone->transformed_p));
			f32 c = vec2_length(vec2_sub(target, ascending_bone->transformed_p));
			vec2 cu = vec2_normalize(vec2_sub(target, ascending_bone->p));
			//f32 theta = arccos32(-((b * b) - (a * a) - (c * c)) / (2 * a * c));
			f32 theta = arccos32(((a * a) + (c * c) - (b * b)) / (2 * a * c));
			f32 theta1 = arccos32(((a * a) + (b * b) - (c * c)) / (2 * a * b));
			if(0 && ((a * a) + (b * b) - (c * c) > 0))
			{
				angle = PI - theta1;
			}
			else
			{
				angle = arccos32(vec2_inner(au, cu)) - theta;
			}
//			r = vec3_cross(au, cu);
		}
		else
		{
			break;
		}

		if(parent_bone == bone)
		{
			continue;
		}
		if(prev_parent_index != bone->parent)
		{
			//			break;
		}
		ascending_bone->rot = -angle;
		//		editor_keyframe->base.offset = quaternion_v3_mul_inverse_foward(kf->base.q, editor_keyframe->base.offset);

		last_parent_index = prev_parent_index;
		prev_parent_index = ascending_bone->parent;
		ascending_bone = bones + ascending_bone->parent;
	}while(last_parent_index != prev_parent_index);
#endif
}

static void
editor_model_apply_ik2d(
		devexp_state *program,
		game_renderer *game_renderer,
		vec2 target,
		render_commands *debug_commands)
{
#if 1
	//TODO: gather lengths!
	exp_bone_2d *bones = program->bones_2d;
	exp_bone_2d *bone = bones + program->bone_to_ik;
	u16 selected_bone_index = program->bone_to_ik;
	//start with the parent of the target bone
	exp_bone_2d *ascending_bone = bones + bone->parent;
	target = program->ik_vec_2d;
	//distance between target and end position
	//no ascendants, no ik
	if(ascending_bone == bone)
	{
		return;
	}

	temporary_area tarea = temporary_area_begin(program->area);

	u32 number_of_passes = 1000;
	//backward_points
	vec2 *points_b = memory_area_clear_and_push_array(program->area,
			vec2, program->bone_count2d);
	//foward points
	vec2 *points_f = memory_area_clear_and_push_array(program->area,
			vec2, program->bone_count2d);
	//lengths
	f32 *distances = memory_area_push_array(program->area, f32 ,program->bone_count2d);
	//indices for last foward pass
	u16 *indices = memory_area_push_array(program->area, u16, program->bone_count2d);

	f32 b = 0;
	f32 total_hierarchy_length = 0;
	u32 index = program->bone_count2d - 1;
	distances[index] = vec2_length(bone->p);
	indices[index] = bone->parent;
	points_f[index--] = bone->transformed_p;
	total_hierarchy_length = vec2_length(bone->p);
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
			distances[index] = vec2_length(ascending_bone->p);
			total_hierarchy_length += index ? distances[index] : 0;
			indices[index] = ascending_bone->parent;
			index--;

			last_parent_index = prev_parent_index;
			prev_parent_index = ascending_bone->parent;

			ascending_bone = bones + ascending_bone->parent;
		}while(last_parent_index != prev_parent_index);
	}

	
	exp_bone_2d *root_bone = ascending_bone;
	
	//
	//set target inside the reach
	//
	vec2 distance_target_root = vec2_sub(target, root_bone->transformed_p);
	f32 distance_tr_length = vec2_length(distance_target_root);
	if(total_hierarchy_length < distance_tr_length)
	{
		target = vec2_scale(vec2_normalize(distance_target_root), total_hierarchy_length);
		target = vec2_add(target, root_bone->transformed_p);
	}
	index = program->bone_count2d - 1;
	//can't reach target!
	points_b[index] = target;
	vec2 f_end = points_f[index];
	{
		u32 passes_index = 0;
		f32 distance_end_target = vec2_length(vec2_sub(target, f_end));
		while(passes_index < 1000 && distance_end_target > 0.001f)
		{
			//loop back and forth
			index = program->bone_count2d - 1;
			vec2 end = points_b[index];
			//backward pass
			while(index)
			{
				u32 parent_i = index - 1;
				//backwards, start from end and adjust coordinates
				vec2 p0 = points_b[index];
				vec2 p1 = points_f[parent_i];
				vec2 distance_parent_bone = vec2_sub(p1, p0);
				vec2 distance_parent_bone_n = vec2_normalize(distance_parent_bone);
				f32 dpb_length = distances[index];
				//new position of parent
				vec2 p_parent = vec2_scale(distance_parent_bone_n, dpb_length);
				p_parent = vec2_add(end, p_parent);
				end = p_parent;
				points_b[parent_i] = p_parent;

				index--;
			}
			index = 0;
			//foward pass
			while(index < program->bone_count2d - 1)
			{
				u32 child_i = index + 1;
				vec2 p0 = points_f[index];
				vec2 p1 = points_b[child_i];
				vec2 distance_p10 = vec2_normalize(vec2_sub(p1, p0));
				f32 dpb_length = distances[child_i];
				vec2 p2 = vec2_add(p0, vec2_scale(distance_p10, dpb_length));
				points_f[child_i] = p2;
				index++;
			}
			passes_index++;

			f_end = points_f[program->bone_count2d - 1];
			distance_end_target = vec2_length(vec2_sub(target, f_end));
		}

		index = 0;
		while(index < program->bone_count2d)
		{
			vec2 point = points_f[index];
			render_rectangle_2d_xywh(debug_commands,
					point.x, 
					point.y, 
					14,
					14,
					V4(0, 0, 255, 255));


			index++;
		}
		//fill rotations
		index = program->bone_count2d - 1;
		{
			//make sure to not loop twice on the "root"
			u32 prev_parent_index = bone->parent;
			u32 last_parent_index = prev_parent_index;
			ascending_bone = bones + (bone->parent - 0);
			index;
			for(;index; index--)
			{
				vec2 point = points_f[index];
				vec2 point1 = points_f[index - 1];
				vec2 d_rp = vec2_normalize(vec2_sub(point, point1));
				f32 angle0 = arctan232(d_rp.x, d_rp.y);
				if(program->apply_ik_2d || index <= 2)
//				ascending_bone->rot = angle2;
				last_parent_index = prev_parent_index;
				prev_parent_index = ascending_bone->parent;

				ascending_bone = bones + ascending_bone->parent;
			}
			index = 0;
			vec2 hierarchy_p = {0};
#if 1
			f32 total_angle = 0;
			for(u32 ni = 1; ni < program->bone_count2d;
					ni++)
			{
				exp_bone_2d *bone = bones + indices[ni];
				exp_bone_2d *child = bones + indices[ni + 1];
				//hierarchy_p = vec2_add(child->p);
				vec2 p0 = points_f[ni - 1];
				vec2 p1 = points_f[ni];
				vec2 d_rp = vec2_normalize(vec2_sub(p1, p0));
				f32 angle0 = arctan232(d_rp.x, d_rp.y);
				f32 angle2 = arctan232(-d_rp.y, d_rp.x);
				f32 angle3 = arctan232(d_rp.y, -d_rp.x);
				bone->rot = -angle0 - total_angle;
				total_angle += bone->rot;
//				points_f[ni] = vec2_rot(points_f[ni]);

				int s = 0;

			}
#endif

		}
	}


	temporary_area_end(&tarea);
	//l = individual arm length
	//d = total_distance [0, l * 2]
	//shand = (cos(arm_angle) * d, sin(arm_angle) * d)
#endif
}



typedef struct{
	vec2 rotation;
	f32 angle;
	vec2 p;
	vec2 displacement;
	vec2 p_before;
	vec2 unrotated;
}exp_model_transform_2d;











static exp_model_transform_2d
exp_get_foward_transform2d(
		exp_bone_2d *bone_array,
		u32 bone_index)
{

	vec2 bone_transformed_position = {0}; 
	vec2 unrotated_end_position = {0};
	vec2 hierarchy_offset   = {0};
	f32 total_r = 0;

//	exp_bone_2d *bone_array = model.bones;
	exp_bone_2d *target_bone = bone_array + bone_index;
	u32 isRoot = bone_index == 0 || (bone_index == target_bone->parent);
	if(!isRoot)
	{
	    u32 parent_index = target_bone->parent;
	    exp_bone_2d *parent_bone = bone_array + target_bone->parent;

	    u32 parentIsRoot      = parent_bone == (bone_array + parent_index);
		u32 last_parent_index = bone_index;
	    //loop start
	    do
	    {

	        vec2 parent_position = parent_bone->p;
	        vec2 parent_displacement = parent_bone->displacement;
			total_r += parent_bone->rot;

            bone_transformed_position = vec2_add(bone_transformed_position, parent_position);
	        unrotated_end_position = vec2_add(unrotated_end_position, parent_position);
	        hierarchy_offset = vec2_add(hierarchy_offset, parent_displacement);

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

	      	 //rotate the current position by the previous rotation
	        //   bone_transformed_position = vec3_mul(quaternion_v3_mul_foward_inverse(
			//		   ascending_quaternion, bone_transformed_position);
	           bone_transformed_position = vec2_rotate(bone_transformed_position, parent_bone->rot);
	      			                       
	        }
	    }while(!parentIsRoot);

	}
	bone_transformed_position = bone_transformed_position;//vec2_rotate(bone_transformed_position, total_r);
	//the offset of the bone depends on the total rotation.
	vec2 target_bone_offset = vec2_rotate(target_bone->p, total_r); 
	//add displacements, those should be zero if this was a bind model
	target_bone_offset = vec2_add(target_bone_offset, target_bone->displacement);
	target_bone_offset = vec2_add(target_bone_offset, hierarchy_offset);
	//Add the final rotated offset
	bone_transformed_position = vec2_add(bone_transformed_position, target_bone_offset); 
	unrotated_end_position = vec2_add(target_bone->p, unrotated_end_position);

    exp_model_transform_2d result = {0};
	//end rotated position
    result.p = bone_transformed_position;
    result.displacement = vec2_sub(bone_transformed_position, unrotated_end_position);
	result.unrotated = unrotated_end_position;
	result.angle = total_r;
    return(result);
}

static exp_model_transform_2d
exp_model_get_foward_transform2d(
		exp_bone_2d *bone_array,
		u32 bone_index)
{
	exp_bone_2d *target_bone = bone_array + bone_index;
	exp_model_transform_2d result = {0};
	//get the hierarchy transform of the parent
	exp_model_transform_2d parent_transform = exp_get_foward_transform2d(
		    bone_array,	
			target_bone->parent);
	if(target_bone->parent != bone_index)
	{
		exp_bone_2d *parent_bone = bone_array + target_bone->parent;

		result.angle = parent_transform.angle + parent_bone->rot;

		vec2 p_add = vec2_rotate(target_bone->p, result.angle);
		result.p_before = parent_transform.p;
		vec2 total_transform = vec2_add(parent_transform.p, p_add);

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
exp_fill_bone_transformed_data2d(
		game_renderer *game_renderer,
		devexp_state *program)
{
	u32 bone_count = program->bone_count;
	exp_bone_2d *bones = program->bones_2d;
	for(u32 b = 0; b < bone_count; b++)
	{
		exp_bone_2d *bone = bones + b;
		bone->normal = V2(0, -1);
	}

	for(u32 b = 0; b < bone_count; b++)
	{
		exp_bone_2d *bone = bones + b;
		exp_model_transform_2d bone_transform = 
			exp_model_get_foward_transform2d(
					bones,
					b);

		//cancel this if parent is the same?
		bone->transformed_rot = bone_transform.angle + bone->rot;
		bone->normal =  vec2_rotate(bone->normal, bone->transformed_rot);
		bone->transformed_p = bone_transform.p;
		bone->transformed_p = vec2_add(bone->transformed_p, bone->displacement);
	}
}


static void
devexp_update_ik_2d(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{

	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;
	if(1)
	{
		exp_fill_bone_transformed_data2d(
				game_renderer,
				program);
		if(1 || program->apply_ik_2d)
		{
			editor_model_apply_ik2d(
					program,
					game_renderer,
					V2(0, 0),
					commands);
		}
		vec4 bone_color = {25, 95, 0, 255};
		//draw bones
		for(u32 b = 0; b < program->bone_count2d; b++)
		{
			exp_bone_2d *bone = program->bones_2d + b;

			if(b != 0)
			{
				exp_bone_2d *parent = program->bones_2d + bone->parent;
				f32 line_alpha = 255;
				render_line_2d_center(commands,
						bone->transformed_p,
						parent->transformed_p,
						0.08f,
						bone_color);
			}
			render_rectangle_2d_xywh(commands,
					bone->transformed_p.x, 
					bone->transformed_p.y, 
					10,
					10,
					bone_color);
		}
		//draw kinematic point
		render_rectangle_2d_xywh(commands,
				program->ik_vec_2d.x,
				program->ik_vec_2d.y,
				10,
				10,
				V4(255, 0, 0, 255));
	}

	render_rectangle_2d_xywh(commands,40, 400, 60, 60, V4(255, 0, 0, 255));

	render_commands_end(commands);
}

static void
devexp_update_normals_2d(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{

	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

	render_rectangle_2d_xywh(commands,
			program->point.x,
			program->point.y,
			30,
			30,
			V4(255, 0, 0, 255));
	
	//draw point and direction
	vec2 point_direction = {cos32(program->point_angle), sin32(program->point_angle)};
	vec2 p0 = vec2_add(program->point, V2(15, 15));
	vec2 p1 = vec2_add(p0, vec2_scale(point_direction, 200.0f));
	render_line_2d_center(commands,
			p0,
			p1,
			10.0f,
			V4(255, 255, 255, 255));
	//draw wall
	program->wall_p.y = 500 - 100;
	render_rectangle_2d_xywh(commands,
			program->wall_p.x,
			program->wall_p.y,
			30,
			200,
			V4(0, 0, 0, 255));

	vec2 p2 = program->wall_p;
	vec2 dir_n = point_direction;
	dir_n.x = -dir_n.x;
	dir_n.y = -dir_n.y;
	f32 point_wall_dot = vec2_inner(dir_n, program->wall_normal);
	vec2 p3 = vec2_add(p2, vec2_scale(vec2_add(point_direction, vec2_scale(program->wall_normal, point_wall_dot * 2)), 200));
	render_line_2d_center(commands,
			p2,
			p3,
			10.0f,
			V4(0, 255, 0, 255));

	render_commands_end(commands);
}

static void
devexp2d_update_dirs(devexp_state *program, editor_input *input, game_renderer *game_renderer)
{

	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

	//draw center point
	vec2 cp = program->center_point; 
	render_rectangle_2d_xywh(commands,
			cp.x,
			cp.y,
			10,
			10,
			V4(255, 0, 0, 255));
	//draw mouse from cp
	vec2 mcp = vec2_add(cp, vec2_sub(input->mouse_clip, cp));
	render_line_2d_center(commands,
			cp,
			mcp,
			2.0f,
			V4(255, 0, 0, 255));

	u32 max_dir = program->max_directions; 
	f32 dir = max_dir / 2.0f;
	f32 angle = (PI / dir) / 2;
	for(; angle < (PI * 2);)
	{
		vec2 av = {cos32(angle), sin32(angle)};
		av = vec2_scale(av, 200.0f);
		vec2 avcp = vec2_add(cp, av);
		f32 angle_add = PI / dir;
		angle += angle_add;

		render_line_2d_center(commands,
				cp,
				avcp,
				3.0f,
				V4(0, 255, 0, 255));
	}
	

	render_commands_end(commands);
}

static void
devexp2d_update_poisson(devexp_state *program, editor_input *input, game_renderer *game_renderer, f32 dt)
{

	render_commands *commands = render_commands_begin_2d(game_renderer);
	commands->camera_type = render_camera_2d;
	game_renderer->camera_zoom_2d = 1.0f;

	{
		if(program->generate_poisson)
		{
			program->active_points_count = 0;
			program->final_points_count = 0;
			u32 grid_count = program->grid_w * program->grid_h;

			for(u32 g = 0; g < grid_count; g++)
			{
				program->grid[g] = -1;
			}
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
			program->generate_poisson = 0;
		}

		u32 points_before_add = 1;
		union{
			u32 w[3];
			struct{
				u32 r_weight;
				u32 b_weight;
				u32 g_weight;
			};
		}rgbw;

		rgbw.w[0] = 1;
        rgbw.w[1] = 1;
        rgbw.w[2] = 100;
		u32 rgbw_count = ARRAYCOUNT(rgbw.w);
		u32 sum_of_weights = 0;
		for(u32 w = 0;w < rgbw_count; w++)
		{
			sum_of_weights += rgbw.w[w];
		}

		//generate points based on the current active points
		while(program->active_points_count)
		{
			for(u32 ap = 0; ap < program->active_points_count; ap++)
			{
				i32 parent_p_index = program->active_points[ap];
				vec2 parent_p = program->final_points[parent_p_index];
				b32 keep_sample = 0;
				for(u32 p = 0; p < program->points_gen_count; p++)
				{
					//choose radius between r-r/2
					f32 radius = random_get_f32_between(&program->random_s,
							program->sampling_r, program->sampling_r * 2);
					//choose random angle
					f32 angle = random_get_f32_between(&program->random_s, 0, PI * 2);
					//get the final position of this point
					vec2 position = {cos32(angle), sin32(angle)};
					position.x *= radius;
					position.y *= radius;
					position = vec2_add(parent_p, position);
					i32 grid_x = (i32)(position.x / program->cell_wh);
					i32 grid_y = (i32)(position.y / program->cell_wh);
					if(grid_x > 0 && grid_x < program->grid_w &&
					   grid_y > 0 && grid_y < program->grid_h)
					{
						//make sure this grid isn't "active"
						u32 grid_index = grid_x + (grid_y * program->grid_w);
						i16 grid_value = program->grid[grid_index];
						if(grid_value != -1)
						{
							continue;
						}

						b32 cancel_point = 0;
						//check neightbour points inside the bounds
						for(i32 iy = -1; iy <= 1; iy++)
						{
							i32 index_y = grid_y + iy;
							//check bounds
							if(index_y < 0) index_y++;
							if(index_y >= program->grid_w) break;

							for(i32 ix = -1; ix <= 1; ix ++)
							{
								i32 index_x = grid_x + ix;
								//check bounds
								if(index_y < 0) index_x++;
								if(index_x >= program->grid_w) continue;

								u32 n_index = index_x + (index_y * program->grid_w);
								//don't compare to yourself
								if(n_index != grid_index)
								{
									//this grid isn't active anymore if its index is -1
									i32 n_grid_value = program->grid[n_index];
									//compare distances if a point is inside this grid
									if(n_grid_value != -1)
									{
										vec2 p1 = program->final_points[n_grid_value];
										//distances_squared
										f32 distance = vec2_inner_squared(vec2_sub(position, p1));
										if(distance < (radius * radius))
										{
											cancel_point = 1;
										}
									}
								}
							}
						}
						//add point if it's not closer to any
						if(!cancel_point)
						{
							u32 final_index = program->final_points_count;
							program->final_points[final_index] = position;
							program->grid[grid_index] = final_index;
							//add this to the active points
							program->active_points[program->active_points_count] = final_index;
							program->final_points_count++;
							program->active_points_count++;
							keep_sample = 1;
							Assert(program->final_points_count < program->final_points_max);
							//choose a color
							i32 color_i = random_get_u32_between(&program->random_s, 0, sum_of_weights);
							for(u32 c = 0; c < rgbw_count; c++)
							{
								if(color_i < (i32)rgbw.w[c])
								{
									program->final_colors[final_index] = c;
									break;
								}
								else
								{
									color_i -= rgbw.w[c];
								}
								Assert(color_i >= 0);
							}
							//since we added this points, we have to activate this grid cell
						}
					}
				}
				//remove this from the active list
				if(!keep_sample)
				{
					program->active_points[ap] = -1;
				}
			}
			//remove inactive points
			u32 ip = 0;
			while(ip < program->active_points_count)
			{
				//shift the array
				if(program->active_points[ip] == -1)
				{
					for(u32 i = ip; i < (u32)(program->active_points_count - 1); i++)
					{
						i32 copy = program->active_points[i];
						program->active_points[i] = program->active_points[i + 1];
						program->active_points[i + 1] = copy;
					}
					program->active_points_count--;
				}
				else
				{
					ip++;
				}
			}
		}
	}

//	program->grid_h = 20;
	//draw grid cells
	//for(i32 y = 0; y < 0 + program->grid_h; y++)
	//{
	//	f32 at_y = y * program->cell_wh;
	//	for(i32 x = 0; x < 0 + program->grid_w; x++)
	//	{
	//		f32 at_x = x * program->cell_wh;
	//		u32 index = x + (y * program->grid_w);
	//		vec4 color = {255, 255, 255, 120};
	//		if(program->grid[index] != 0)
	//		{
//	//			color.w = 255;
	//		}
	//		render_rectangle_borders_2D(commands,
	//				(f32)at_x,
	//				(f32)at_y,
	//				(f32)program->cell_wh,
	//				(f32)program->cell_wh,
	//				1.0f,
	//				color);

	//	}
	//}
	//draw active points
	for(u32 a = 0; a < program->final_points_count; a++)
	{
		vec2 *p = program->final_points + a;
		u8 color_v = program->final_colors[a];
		vec4 color = {0, 0, 0, 255};
		if(color_v == 0)
		{
			color.r = 255;
		}
		else if(color_v == 1)
		{
			color.g = 255;
		}
		else if(color_v == 2)
		{
			color.b = 255;
		}
		render_rectangle_2d_xywh(commands,
				p->x,
				p->y,
				6,
				6,
				color);

	}
	

	render_commands_end(commands);
}
