
//
// Sprite
//
/*
Notes: 
To get the billboard points only on the x-axis, you call get_mesh_billboard_points_x,
passing the vertices as parameter and a pivot point LOCAL to the mesh (think of it as if it
was comming from the middle of said mesh). get_mesh_billboard_points will give the vertices
points facing to the camera at the x and y axis

To get a quaternion hierarchy rotation, you cal sprite_model_get_foward_transform_quaternion,
or sprite_model_get_foward_transform_quaternion_pre_rotated if an aditional rotation wants
to be added as extra. The function internally gets the foward transform of the parent bone
in order to get the absolute rotation and position of said parent, and combines the results
with the selected bone index (the child).
*/

typedef struct s_sprite_model_transform{
	vec3 rotation;
	f32 angle;
	vec3 p;
	vec3 displacement;
	vec3 p_before;
	vec3 unrotated;
	quaternion q;
}sprite_model_transform;

#define sprite_model_get_foward_transform_quaternion(bone_array, i) \
	model_get_foward_transform_quaternion(bone_array, i, 0)

static inline u32
model_get_view_index(vec2 vd)
{
//	u32 index = 0;
//	if(vd.x == 0 && vd.y < 0) index = 0;
//	if(vd.x < 1 && vd.y > 1) index = 1;

}

static inline i32
model_bone_pitch_index(
		model_bone *bone)
{
	f32 max_pitch_dirs = 8.0f;
	i32 x_rot_angle_index = 0;
	{
		f32 x_angle = arctan232(bone->normal.z, -bone->normal.y);

		f32 pi_ost = PI / max_pitch_dirs;
		x_rot_angle_index = (i32)(x_angle / pi_ost);
	}

	return(x_rot_angle_index);
}

static sprite_model_transform
_model_get_foward_transform_quaternion(
		model_bone *bone_array,
		u32 bone_index,
		f32 added_rotation_z)
{

	vec3 bone_transformed_position = {0}; 
	vec3 unrotated_end_position = {0};
	vec3 hierarchy_offset   = {0};
	quaternion final_rotated_quaternion = {1, 0, 0, 0};

//	model_bone *bone_array = model.bones;
	model_bone *target_bone = bone_array + bone_index;
	u32 isRoot = bone_index == 0 || (bone_index == target_bone->parent);
	if(!isRoot)
	{
	    u32 parent_index = target_bone->parent;
	    model_bone *parent_bone = bone_array + target_bone->parent;
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

    sprite_model_transform result = {0};
	//end rotated position
    result.p            = bone_transformed_position;
    result.displacement = vec3_sub(bone_transformed_position, unrotated_end_position);
	result.q = final_rotated_quaternion;
	result.unrotated = unrotated_end_position;
    return(result);
}

static sprite_model_transform
model_get_foward_transform_quaternion(
		model_bone *bone_array,
		u32 bone_index,
		f32 added_rotation_z)
{
//	model_bone *bone_array = model.bones;
	model_bone *target_bone = bone_array + bone_index;
	sprite_model_transform result = {0};
	//get the hierarchy transform of the parent
	sprite_model_transform parent_transform = _model_get_foward_transform_quaternion(
		    bone_array,	
			target_bone->parent,
			added_rotation_z);
	if(target_bone->parent != bone_index)
	{
		model_bone *parent_bone = bone_array + target_bone->parent;

		result.q = quaternion_mul(parent_transform.q, parent_bone->q);

		vec3 p_add = quaternion_v3_mul_foward_inverse(result.q, target_bone->p);
		result.p_before = parent_transform.p;
		vec3 total_transform = vec3_add(parent_transform.p, p_add);
//		vec3 total_transform = vec3_add(parent_transform.p, target_bone->p);
		//total_transform = vec3_add(total_transform, target_bone->displacement);

		result.displacement = p_add; 
		result.p = total_transform;
	}
	else
	{
		return(parent_transform);
		
	}
	return(result);
}


static vec3
model_hierarchy_position_from_bones(
		model_bone *bone_array,
		u32 bone_array_count,
		u32 bone_index)
{

	u32 isRoot = bone_index == 0;
	vec3 unrotated_end_position = {0};
	model_bone *target_bone = bone_array + bone_index;

	if(!isRoot)
	{
	   u32 parent_index = target_bone->parent;
	   model_bone *parent_bone = bone_array + target_bone->parent;

	   u32 parentIsRoot = 0;
	   u32 last_parent_index = bone_array_count;
	   //loop start
	  do
	  {
	      vec3 parent_position = parent_bone->p;
		  unrotated_end_position = vec3_add(unrotated_end_position, parent_position);

	      parentIsRoot = parent_index == 0 ||
			             parent_index == bone_index ||
						 parent_bone->parent == last_parent_index;

		  last_parent_index = parent_index;
	      //now get the parent from the parent's model;
	      parent_index = parent_bone->parent;

		  //the root bone points to itself
          parent_bone = bone_array + parent_index;
	      

	  }while(!parentIsRoot);

	}

	unrotated_end_position = vec3_add(target_bone->p, unrotated_end_position);

   return(unrotated_end_position);
}

static vec3
sprite_model_get_hierarchy_position(model model,
		                            u32 bone_index)
{

	u32 isRoot = bone_index == 0;
	vec3 unrotated_end_position = {0};
	model_bone *bone_array = model.bones;
	model_bone *target_bone = bone_array + bone_index;

	if(!isRoot)
	{
	   u32 parent_index = target_bone->parent;
	   model_bone *parent_bone = bone_array + target_bone->parent;

	   u32 parentIsRoot = 0;
	   u32 last_parent_index = model.bone_count;
	   //loop start
	  do
	  {
	      vec3 parent_position = parent_bone->p;
		  unrotated_end_position = vec3_add(unrotated_end_position, parent_position);

	      parentIsRoot = parent_index == 0 ||
			             parent_index == bone_index ||
						 parent_bone->parent == last_parent_index;

		  last_parent_index = parent_index;
	      //now get the parent from the parent's model;
	      parent_index = parent_bone->parent;

		  //the root bone points to itself
          parent_bone = bone_array + parent_index;
	      

	  }while(!parentIsRoot);

	}

	unrotated_end_position = vec3_add(target_bone->p, unrotated_end_position);

   return(unrotated_end_position);
}




static void
model_debug_display_nodes(
		render_commands *commands,
		model *bind_model)
{
	if(!bind_model || !bind_model->bone_count)
	{
		return;
	}

	u32 bone_count = bind_model->bone_count;
	u16 bone_got_hitted   = 0;
	u16 bone_hitted_index = 0;
	// display and select nodes
	vec4 node_selected     = {140, 255, 140, 255};
	vec4 node_normal_color = {255, 255, 255, 40};
	for(u32 b = 0;
			b < bone_count;
			b++)
	{
		model_bone *current_bone = bind_model->nodes + b;
		vec3 bone_position       = current_bone->p;

		//if in transforms preview mode.
		vec3 node_rotation = {current_bone->rotation_x,
			current_bone->rotation_y,
			current_bone->rotation_z};

		vec3 node_x_axis = {1, 0, 0};
		vec3 node_y_axis = {0, 1, 0};
		vec3 node_z_axis = {0, 0, 1};

		{
			sprite_model_transform bone_transform = sprite_model_get_foward_transform_quaternion(
					bind_model->pose.bones,
					b); 
			bone_position = bone_transform.p;
			node_rotation = vec3_add(node_rotation, bone_transform.rotation);
		}

		matrix3x3 hierarchy_rotation_matrix = matrix3x3_rotation_scale(node_rotation.x, node_rotation.y, node_rotation.z);
		node_x_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 0);
		node_y_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 1);
		node_z_axis = matrix3x3_v3_get_row(hierarchy_rotation_matrix, 2);

		vec4 node_color = node_normal_color;
		//select or modify node color based on the current state or mode
		node_color = node_selected;
		//		DrawAxes(
		//				commands,
		//				bone_position,
		//				node_x_axis,
		//				node_y_axis,
		//				node_z_axis,
		//				node_color.w);
		node_color.w = 140;

		//if this is not the root bone
		if(b != 0)
		{
			u32 node_linked_index     = current_bone->parent;
			model_bone *linked_node   = bind_model->nodes + node_linked_index;
			vec3 linked_node_position = linked_node->p;
			{
				sprite_model_transform bone_transform = sprite_model_get_foward_transform_quaternion(
						bind_model->pose.bones,
						node_linked_index); 
				linked_node_position = bone_transform.p;
				linked_node_position = vec3_add(linked_node_position, bind_model->nodes[node_linked_index].displacement);
			}
			f32 line_alpha = 140;

			render_draw_line_up(commands,
					bone_position,
					linked_node_position,
					V4(255, 0, 0, line_alpha),
					0.2f);
		}

		bone_position = vec3_add(bone_position, current_bone->displacement);
		//render cube after line
		render_draw_cube(commands,
				bone_position, 
				vec3_all(1),
				node_color);

	}
}


inline mesh_points
get_mesh_billboard_points_all(game_renderer *gameRenderer,
		                    vec3 v0,
		                    vec3 v1,
		                    vec3 v2,
		                    vec3 v3,
							vec3 pivot_point,
							vec3 origin_offset
							)
{

    mesh_points points = {0};
	matrix3x3 camera_rotation = matrix3x3_rotation_scale(gameRenderer->camera_rotation.x,// + gameRenderer->camera_rotation.x, ,
	                                                     gameRenderer->camera_rotation.y,
					                                     gameRenderer->camera_rotation.z);
	//rotate the final bone position to the camera and use it as origin for the vertices
	vec3 mid_point = vertices_get_mid_point(v0,
	   					                 v1,
	   					                 v2,
	   					                 v3);

	//use the mid point as origin to place the bone on the correct location
	v0 = vec3_sub(v0, mid_point);
	v1 = vec3_sub(v1, mid_point);
	v2 = vec3_sub(v2, mid_point);
	v3 = vec3_sub(v3, mid_point);

	//swap y and z to correctly face the camera
	f32 v0_y = v0.y;
	f32 v1_y = v1.y;
	f32 v2_y = v2.y;
	f32 v3_y = v3.y;
	v0.y = v0.z;
	v1.y = v1.z;
	v2.y = v2.z;
	v3.y = v3.z;

	v0.z = v0_y;
	v1.z = v1_y;
	v2.z = v2_y;
	v3.z = v3_y;

	//pivot offset choses from which point it rotates
	v0 = vec3_sub(v0, pivot_point);
	v1 = vec3_sub(v1, pivot_point);
	v2 = vec3_sub(v2, pivot_point);
	v3 = vec3_sub(v3, pivot_point);

	v0 = matrix3x3_v3_mul_rows(camera_rotation, v0);
	v1 = matrix3x3_v3_mul_rows(camera_rotation, v1);
	v2 = matrix3x3_v3_mul_rows(camera_rotation, v2);
	v3 = matrix3x3_v3_mul_rows(camera_rotation, v3);

	v0 = vec3_add(v0, pivot_point);
	v1 = vec3_add(v1, pivot_point);
	v2 = vec3_add(v2, pivot_point);
	v3 = vec3_add(v3, pivot_point);
	
	v0 = vec3_add(mid_point, v0);
	v1 = vec3_add(mid_point, v1);
	v2 = vec3_add(mid_point, v2);
	v3 = vec3_add(mid_point, v3);

	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
	return(points);
}

inline mesh_points
get_mesh_billboard_points_depth(game_renderer *gameRenderer,
		                    vec3 v0,
		                    vec3 v1,
		                    vec3 v2,
		                    vec3 v3,
							vec3 pivot_point,
							f32 z_depth
							)
{

    mesh_points points = {0};
#if 0
	matrix3x3 camera_rotation = matrix3x3_rotation_scale(gameRenderer->camera_rotation.x,// + gameRenderer->camera_rotation.x, ,
	                                                     gameRenderer->camera_rotation.y,
					                                     gameRenderer->camera_rotation.z);
	//rotate the final bone position to the camera and use it as origin for the vertices
	vec3 mid_point = vertices_get_mid_point(v0,
	   					                 v1,
	   					                 v2,
	   					                 v3);

	//vec3 distance_v3_v0 = vec3_sub(v3, v0);
	//vec3 distance_v1_v0 = vec3_sub(v1, v0);

	//vec3 sprite_y_axis  = vec3_Normalize(distance_v1_v0);
	////vec3 sprite_z_axis  = distance_camera_mid;
	//vec3 sprite_x_axis  = vec3_Normalize(distance_v3_v0);

	//sprite_x_axis = matrix3x3_v3_mul_rows(camera_rotation, sprite_x_axis)
    //sprite_y_axis = matrix3x3_v3_mul_rows(camera_rotation, sprite_y_axis)

	//use the mid point as origin to place the bone on the correct location
	v0 = vec3_sub(v0, mid_point);
	v1 = vec3_sub(v1, mid_point);
	v2 = vec3_sub(v2, mid_point);
	v3 = vec3_sub(v3, mid_point);

	//swap y and z to correctly face the camera
	f32 v0_y = v0.y;
	f32 v1_y = v1.y;
	f32 v2_y = v2.y;
	f32 v3_y = v3.y;
	v0.y = v0.z;
	v1.y = v1.z;
	v2.y = v2.z;
	v3.y = v3.z;

	v0.z = v0_y;
	v1.z = v1_y;
	v2.z = v2_y;
	v3.z = v3_y;

	//pivot offset choses from which point it rotates
	v0 = vec3_add(v0, pivot_point);
	v1 = vec3_add(v1, pivot_point);
	v2 = vec3_add(v2, pivot_point);
	v3 = vec3_add(v3, pivot_point);

	v0 = matrix3x3_v3_mul_rows(camera_rotation, v0);
	v1 = matrix3x3_v3_mul_rows(camera_rotation, v1);
	v2 = matrix3x3_v3_mul_rows(camera_rotation, v2);
	v3 = matrix3x3_v3_mul_rows(camera_rotation, v3);

	//v0 = vec3_add(v0, pivot_point);
	//v1 = vec3_add(v1, pivot_point);
	//v2 = vec3_add(v2, pivot_point);
	//v3 = vec3_add(v3, pivot_point);
	
	v0 = vec3_add(mid_point, v0);
	v1 = vec3_add(mid_point, v1);
	v2 = vec3_add(mid_point, v2);
	v3 = vec3_add(mid_point, v3);

	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
#else
	vec3 camera_x = gameRenderer->camera_x;
	vec3 camera_y = gameRenderer->camera_y;
	vec3 mid_point = vertices_get_mid_point(v0,
	   					                 v1,
	   					                 v2,
	   					                 v3);

	f32 size_z = v1.z - v0.z;
	f32 size_x = v3.x - v0.x;
	f32 lc = vec3_length(vec3_sub(mid_point, gameRenderer->camera_position));
	f32 depth_n = lc / (lc - z_depth);
	if(depth_n)
	{
		size_x /= depth_n;
		size_z /= depth_n;
	}
//	mid_point = vec3_sub(mid_point, pivot_point);
	f32 size_x0 = size_x + pivot_point.x;
	f32 size_y0 = size_z + pivot_point.y;
	f32 size_x1 = size_x - pivot_point.x;
	f32 size_y1 = size_z - pivot_point.y;
	vec3 x_scaled_r = vec3_scale(camera_x, size_x1 * 0.5f);
	vec3 y_scaled_b = vec3_scale(camera_y, size_y0 * 0.5f);
	vec3 x_scaled_l = vec3_scale(camera_x, size_x0 * 0.5f);
	vec3 y_scaled_t = vec3_scale(camera_y, size_y1 * 0.5f);

//	x_scaled_r = vec3_add(x_scaled_r, pivot_point);
//	y_scaled_t = vec3_add(y_scaled_t, pivot_point);
//	x_scaled_l = vec3_sub(x_scaled_l, pivot_point);
//	y_scaled_b = vec3_sub(y_scaled_b, pivot_point);

	v0 = vec3_sub(mid_point, x_scaled_l);
	v0 = vec3_sub(v0, y_scaled_b);

	v1 = vec3_sub(mid_point, x_scaled_l);
	v1 = vec3_add(v1, y_scaled_t);

	v2 = vec3_add(mid_point, x_scaled_r);
	v2 = vec3_add(v2, y_scaled_t);

	v3 = vec3_add(mid_point, x_scaled_r);
	v3 = vec3_sub(v3, y_scaled_b);

	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
#endif
	return(points);
}

inline mesh_points
get_mesh_billboard_points(game_renderer *gameRenderer,
		                    vec3 v0,
		                    vec3 v1,
		                    vec3 v2,
		                    vec3 v3,
							vec3 pivot_point
							)
{

    mesh_points points = {0};
#if 0
	matrix3x3 camera_rotation = matrix3x3_rotation_scale(gameRenderer->camera_rotation.x,// + gameRenderer->camera_rotation.x, ,
	                                                     gameRenderer->camera_rotation.y,
					                                     gameRenderer->camera_rotation.z);
	//rotate the final bone position to the camera and use it as origin for the vertices
	vec3 mid_point = vertices_get_mid_point(v0,
	   					                 v1,
	   					                 v2,
	   					                 v3);

	//vec3 distance_v3_v0 = vec3_sub(v3, v0);
	//vec3 distance_v1_v0 = vec3_sub(v1, v0);

	//vec3 sprite_y_axis  = vec3_Normalize(distance_v1_v0);
	////vec3 sprite_z_axis  = distance_camera_mid;
	//vec3 sprite_x_axis  = vec3_Normalize(distance_v3_v0);

	//sprite_x_axis = matrix3x3_v3_mul_rows(camera_rotation, sprite_x_axis)
    //sprite_y_axis = matrix3x3_v3_mul_rows(camera_rotation, sprite_y_axis)

	//use the mid point as origin to place the bone on the correct location
	v0 = vec3_sub(v0, mid_point);
	v1 = vec3_sub(v1, mid_point);
	v2 = vec3_sub(v2, mid_point);
	v3 = vec3_sub(v3, mid_point);

	//swap y and z to correctly face the camera
	f32 v0_y = v0.y;
	f32 v1_y = v1.y;
	f32 v2_y = v2.y;
	f32 v3_y = v3.y;
	v0.y = v0.z;
	v1.y = v1.z;
	v2.y = v2.z;
	v3.y = v3.z;

	v0.z = v0_y;
	v1.z = v1_y;
	v2.z = v2_y;
	v3.z = v3_y;

	//pivot offset choses from which point it rotates
	v0 = vec3_add(v0, pivot_point);
	v1 = vec3_add(v1, pivot_point);
	v2 = vec3_add(v2, pivot_point);
	v3 = vec3_add(v3, pivot_point);

	v0 = matrix3x3_v3_mul_rows(camera_rotation, v0);
	v1 = matrix3x3_v3_mul_rows(camera_rotation, v1);
	v2 = matrix3x3_v3_mul_rows(camera_rotation, v2);
	v3 = matrix3x3_v3_mul_rows(camera_rotation, v3);

	//v0 = vec3_add(v0, pivot_point);
	//v1 = vec3_add(v1, pivot_point);
	//v2 = vec3_add(v2, pivot_point);
	//v3 = vec3_add(v3, pivot_point);
	
	v0 = vec3_add(mid_point, v0);
	v1 = vec3_add(mid_point, v1);
	v2 = vec3_add(mid_point, v2);
	v3 = vec3_add(mid_point, v3);

	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
#else
	vec3 camera_x = gameRenderer->camera_x;
	vec3 camera_y = gameRenderer->camera_y;
	f32 size_z = v1.z - v0.z;
	f32 size_x = v3.x - v0.x;
	vec3 mid_point = vertices_get_mid_point(v0,
	   					                 v1,
	   					                 v2,
	   					                 v3);
//	mid_point = vec3_sub(mid_point, pivot_point);
	f32 size_x0 = size_x + pivot_point.x;
	f32 size_y0 = size_z + pivot_point.y;
	f32 size_x1 = size_x - pivot_point.x;
	f32 size_y1 = size_z - pivot_point.y;
	vec3 x_scaled_r = vec3_scale(camera_x, size_x1 * 0.5f);
	vec3 y_scaled_b = vec3_scale(camera_y, size_y0 * 0.5f);
	vec3 x_scaled_l = vec3_scale(camera_x, size_x0 * 0.5f);
	vec3 y_scaled_t = vec3_scale(camera_y, size_y1 * 0.5f);

//	x_scaled_r = vec3_add(x_scaled_r, pivot_point);
//	y_scaled_t = vec3_add(y_scaled_t, pivot_point);
//	x_scaled_l = vec3_sub(x_scaled_l, pivot_point);
//	y_scaled_b = vec3_sub(y_scaled_b, pivot_point);

	v0 = vec3_sub(mid_point, x_scaled_l);
	v0 = vec3_sub(v0, y_scaled_b);

	v1 = vec3_sub(mid_point, x_scaled_l);
	v1 = vec3_add(v1, y_scaled_t);

	v2 = vec3_add(mid_point, x_scaled_r);
	v2 = vec3_add(v2, y_scaled_t);

	v3 = vec3_add(mid_point, x_scaled_r);
	v3 = vec3_sub(v3, y_scaled_b);

	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
#endif
	return(points);
}

inline mesh_points
get_mesh_billboard_points_x(game_renderer *gameRenderer,
		                    vec3 v0,
		                    vec3 v1,
		                    vec3 v2,
		                    vec3 v3,
							vec3 pivot_point
							)
{
	mesh_points points = {0};


	vec3 mid_point = vertices_get_mid_point(v0,
	   					                    v1,
	   					                    v2,
	   					                    v3);
	//mid_point = vec3_add(mid_point, pivot_point);

	vec3 distance_camera_mid = vec3_sub(mid_point, gameRenderer->camera_position);
//	distance_camera_mid.z = 0;
	vec3 distance_v1_v0      = vec3_sub(v1, v0);
	vec3 sprite_y_axis       = distance_v1_v0;
	vec3 sprite_z_axis       = distance_camera_mid;
	vec3 sprite_x_axis       = vec3_cross(sprite_z_axis, sprite_y_axis);

	vec3 distance_v3_v0 = vec3_sub(v3, v0);
	f32 y_scale = vec3_length(distance_v1_v0);
	f32 x_scale = vec3_length(distance_v3_v0);

	sprite_x_axis = vec3_normalize(sprite_x_axis);
	sprite_y_axis = vec3_normalize(sprite_y_axis);

	vec3 x_scaled = vec3_scale(sprite_x_axis, x_scale);
	vec3 y_scaled = vec3_scale(sprite_y_axis, y_scale);

	//use lerp instead?
	f32 original_x_inner = vec3_inner_squared(distance_v1_v0);
	f32 original_y_inner = vec3_inner_squared(distance_v3_v0);

	f32 scale_x_inner = original_x_inner / (original_x_inner + pivot_point.x * 2);
	f32 scale_y_inner = original_y_inner / (original_y_inner + pivot_point.y * 2);

	//scale_x_inner /= original_x_inner;
	//scale_y_inner /= original_y_inner;
	scale_x_inner *= 0.5f;
	scale_y_inner *= 0.5f;
	//scale_x_inner = pivot_point.x < 0 ? -scale_x_inner : scale_x_inner;
	//scale_y_inner = pivot_point.x < 0 ? -scale_y_inner : scale_y_inner;

	//v0 = vec3_sub(mid_point, vec3_scale(x_scaled, 0.5f)); 
	//v0 = vec3_sub(v0, vec3_scale(y_scaled, 0.5f)); 
	//v1 = vec3_add(v0, y_scaled);
	//v2 = vec3_add(v1, x_scaled);
	//v3 = vec3_add(v0, x_scaled);

	f32 scale_l_f = scale_x_inner;
	f32 scale_r_f = 1.0f - scale_l_f;
	f32 scale_t_f = 0.5f;
	f32 scale_b_f = 1.0f - scale_t_f;

	v0 = vec3_sub(mid_point, vec3_scale(x_scaled, scale_l_f)); 
	v0 = vec3_sub(v0, vec3_scale(y_scaled, scale_b_f)); 

	v1 = vec3_sub(mid_point, vec3_scale(x_scaled, scale_l_f)); 
	v1 = vec3_add(v1, vec3_scale(y_scaled, scale_t_f)); 

	v2 = vec3_add(mid_point, vec3_scale(x_scaled, scale_r_f)); 
	v2 = vec3_add(v2, vec3_scale(y_scaled, scale_t_f)); 

	v3 = vec3_add(mid_point, vec3_scale(x_scaled, scale_r_f)); 
	v3 = vec3_sub(v3, vec3_scale(y_scaled, scale_b_f)); 

//	vec3 v0t = vec3_sub(mid_point, vec3_scale(sprite_x_axis, x_scale / 2.0f));

	v0.z += pivot_point.z;
	v1.z += pivot_point.z;
	v2.z += pivot_point.z;
	v3.z += pivot_point.z;


	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;
	return(points);
}


inline u32
get_orientation_index_foward_by_angle(
		f32 rotation, u32 maxDirections)
{
	//u32 indicesLH[16] = {0,1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 0 };
	//u32 indices[16] = {0,15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 };
	u32 finalIndex = 0;
#if 0
	if(maxDirections)
	{
	    //u32 indicesI[16] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 0 };
	    u32 indicesI[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
	    u32 orientation = 0;
	    f32 gameDirectionsDiv = 16.0f / TWOPI;
	    u32 indexFix = 16 / maxDirections;

	    u32 orientationIndex = (u32)(rotation * gameDirectionsDiv) ;
	    finalIndex = (u32)(indicesI[orientationIndex] * (maxDirections / 16.0f));
	    finalIndex *= indexFix;
	    Assert(finalIndex < 16);
	}
#else
	f32 dir = maxDirections / 2.0f;
	//same as angle_dir below
	f32 angle_add = maxDirections ? (PI / dir / 2) : 0;
	rotation += angle_add;
	if(rotation < 0)
	{
		rotation = rotation + (PI * 2);
	}
	f32 angle_dir = (PI) / dir;
	finalIndex = (i32)(rotation / angle_dir);
#endif
	return(finalIndex);
}

inline u32
get_orientation_index_foward(
		game_renderer *gameRenderer,
		vec2 looking_direction,
		u32 max_directions)
{
		  vec2 xAxisCam = 
		  {
			  gameRenderer->camera_x.x, 
			  gameRenderer->camera_x.y 
		  };
		  vec2 yAxisCam = 
		  {
			  -gameRenderer->camera_z.x,
			  -gameRenderer->camera_z.y,
		  };

		  f32 dotPCx = vec2_inner(xAxisCam, looking_direction);
		  f32 dotPCy = vec2_inner(yAxisCam, looking_direction);

		  //Scale looking direction to the z position. might not be as precise as the axes above.

		  f32 finalAngle = arctan232(dotPCx, dotPCy);

		  //if(finalAngle < 0)
		  //{
		  //    finalAngle = finalAngle + (PI * 2);
		  //}
		  u32 finalIndex = get_orientation_index_foward_by_angle(finalAngle, max_directions);
		  //now flip based on index x

		  return(finalIndex);
}

inline f32
get_up_down_angle(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		f32 angle_offset)
{

	vec2 looking_direction_2 = 
	{
		looking_direction.z,
		looking_direction.y
	};
	vec2 xAxisCam = 
	{
	    gameRenderer->camera_y.y,
	    gameRenderer->camera_y.z 
	};
	vec2 yAxisCam = 
	{
	    gameRenderer->camera_z.y,
	    gameRenderer->camera_z.z,
	};

	f32 dotPCx = vec2_inner(xAxisCam, looking_direction_2);
	f32 dotPCy = vec2_inner(yAxisCam, looking_direction_2);

	//Scale looking direction to the z position. might not be as precise as the axes above.
	f32 finalAngle = arctan232(dotPCx, dotPCy) + angle_offset;

	if(finalAngle < 0)
	{
	    finalAngle = finalAngle + (PI * 2);
	}
	if(finalAngle > (PI * 2))
	{
		finalAngle -= PI * 2;
	}
	//now flip based on index x

	return(finalAngle);
}

static inline f32
get_x_angle(vec3 looking_direction)
{
	return(0);
}

inline u32
get_orientation_index_up_down(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		u32 max_directions)
{
	f32 finalAngle = get_up_down_angle(
			gameRenderer, looking_direction, 0);
	u32 finalIndex = get_orientation_index_foward_by_angle(finalAngle, max_directions);
	//now flip based on index x

	return(finalIndex);
}

static u32
is_looking_down_by_radians(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		f32 angle)
{
	u32 is_looking_down = 0;
	//get the up down direction angle
	f32 up_down_angle = get_up_down_angle(
			gameRenderer,
			looking_direction, 0);

	f32 final_angle = up_down_angle + angle;
	//subtract PI * 2 since the angle starts from there
	f32 angle_starting_from_zero = final_angle - PI;

	is_looking_down = (angle_starting_from_zero) > 0 && angle_starting_from_zero < (angle * 2);

	return(is_looking_down);
}

inline u32
is_looking_down_by_degrees(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		f32 degrees)
{
	f32 angle = degrees_to_radians_f32(degrees);

	u32 is_looking_down = is_looking_down_by_radians(
		                  gameRenderer,
		                  looking_direction,
		                  angle);

	return(is_looking_down);
}

static u32
is_looking_up_by_radians(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		f32 angle)
{
	u32 is_looking_down = 0;
	//get the up down direction angle
	f32 up_down_angle = get_up_down_angle(
			gameRenderer,
			looking_direction,
			angle);

	f32 final_angle = up_down_angle + angle;
	//subtract PI * 2 since the angle starts from there
	f32 angle_starting_from_zero = up_down_angle;

	is_looking_down = angle_starting_from_zero < (angle * 2);
	                  

	return(is_looking_down);
}

inline u32
is_looking_up_by_degrees(
		game_renderer *gameRenderer,
		vec3 looking_direction,
		f32 degrees)
{
	f32 angle = degrees_to_radians_f32(degrees);

	u32 is_looking_down = is_looking_up_by_radians(
		                  gameRenderer,
		                  looking_direction,
		                  angle);

	return(is_looking_down);
}




inline void
adjust_vertices_to_uvs(
		               render_texture *texture,
		               vec3 *v0_ptr,
		               vec3 *v1_ptr,
					   vec3 *v2_ptr,
					   vec3 *v3_ptr,
					   vec2 uv0,
					   vec2 uv1,
					   vec2 uv2,
					   vec2 uv3)
{

		vec3 v0 = *v0_ptr;
		vec3 v1 = *v1_ptr;
		vec3 v2 = *v2_ptr;
		vec3 v3 = *v3_ptr;

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

		*v0_ptr = v0;
		*v1_ptr = v1;
		*v2_ptr = v2;
		*v3_ptr = v3;

}

static void
model_fill_bone_transformed_data(
		game_renderer *game_renderer,
		model_pose animated_pose,
		u32 bone_count,
		vec2 model_facing_direction)
{
	f32 facing_direction_angle_q = (PI - arctan232(model_facing_direction.x, model_facing_direction.y));
	f32 facing_direction_angle = (PI - arctan232(model_facing_direction.x, model_facing_direction.y)) / PI;
	quaternion direction_quaternion = quaternion_from_rotations_radians(0, 0, facing_direction_angle_q);
	//rotate the "root" by the direction quaternion to affect the whole skeleton
	if(bone_count)
	{
		model_bone *bone = animated_pose.bones + 0;
		bone->q = quaternion_mul(direction_quaternion, bone->q);
	}
	for(u32 b = 0; b < bone_count; b++)
	{
		model_bone *bone = animated_pose.bones + b;
		//bone->q = quaternion_from_rotations_radians(
		//		bone->rotation_x,
		//		bone->rotation_y,
		//		bone->rotation_z
		//		);
//		quaternion qx = quaternion_rotated_at(1, 0, 0, bone->rotation_x);
//		quaternion qy = quaternion_rotated_at(0, 1, 0, bone->rotation_y);
//		quaternion qz = quaternion_rotated_at(0, 0, 1, bone->rotation_z);
//		bone->q = quaternion_mul(bone->q, qx);
//		bone->q = quaternion_mul(bone->q, qy);
//		bone->q = quaternion_mul(bone->q, qz);
		bone->displacement = quaternion_v3_mul_foward_inverse(direction_quaternion, bone->displacement);
		bone->normal = V3(0, -1, 0);
	}

	for(u32 b = 0; b < bone_count; b++)
	{
		model_bone *bone = animated_pose.bones + b;
		sprite_model_transform bone_transform = 
			model_get_foward_transform_quaternion(
					animated_pose.bones,
					b,
					0);

		//cancel this if parent is the same?
		bone->transformed_q = quaternion_mul(bone_transform.q, bone->q);
		vec3 dir_default = {0, 1, 0};
		bone->normal =  quaternion_v3_mul_foward_inverse(bone->transformed_q, bone->normal);
//		bone->normal = quaternion_v3_mul_foward_inverse(bone->q, dir_default);
		if(bone->two_dim)
		{
			vec3 add_p = bone_transform.displacement;
			add_p.z = 0;
			bone->transformed_p = bone_transform.p_before;
			bone->transformed_p = vec3_add(bone->transformed_p, add_p);
			add_p.x = 0;
			add_p.y = 0;
			add_p.z = bone_transform.displacement.z;
//			add_p = quaternion_v3_mul_foward_inverse(q_cam, add_p);
			vec3 camera_y_depth = vec3_scale(game_renderer->camera_y, add_p.z);
			bone->transformed_p = vec3_add(bone->transformed_p, camera_y_depth);
//			bone->transformed_p = vec3_add(bone->transformed_p, quaternion_v3_mul_foward_inverse(q_cam, bone_transform.displacement));
		}
		else
		{
			bone->transformed_p = bone_transform.p;
			bone->transformed_p = vec3_add(bone->transformed_p, bone->displacement);
		}
		
		if(bone->virtual)
		{
			//pick attached bone and apply transformation
			//if the bone is virtual, whatever change made to it will be reflected on the attached model
			//selected bone only if it's avadible.
		}
	}
}

static void
model_animation_animate_model_new(
		game_renderer *game_renderer,
		struct s_model model,
		               model_pose animated_pose,
					   model_animation *animation,
					   f32 totalTimer,
					   u32 run,
					   vec2 view_direction,
					   f32 dt)
{

	model_animation_keyframe *keyframe_array = model.keyframes;
	model_animation_keyframe *frame_keyframe_array = model.frame_keyframes;
	model_pose bind_pose = model.pose;

	u32 bone_count = model.bone_count;
	u32 sprite_count = model.sprite_count;

	u32 runAnimations = run && bone_count;
	u32 frame_list_count = model.frame_list_count;

	//u32 bone_count = loaded_model->bone_count;
	//model *model     = &loaded_model->model;
	//model *bindModel = &loaded_model;

	//Reset to bind model
	for(u32 b = 0; b < sprite_count; b++)
	{
		animated_pose.sprites[b] = bind_pose.sprites[b];
	}
	for(u32 b = 0; b < bone_count; b++)
	{
		animated_pose.bones[b] = bind_pose.bones[b];
	}

		
    if(runAnimations)
	{
		if(animation->frame_timer)
		{
			f32 f_frame_repeat = 0.1f / animation->frame_timer_repeat;
			totalTimer = (f32)(i32)(totalTimer / f_frame_repeat) * f_frame_repeat;
		}

		u32 k  = 0;
		u32 at = animation->keyframes_at;
		//transform keyframes

		while(k < animation->keyframe_count)
		{
			u32 index0 = k ? k - 1 : 0;
			u32 index1 = k;
			model_animation_keyframe *kf0 = keyframe_array + at + index0;
			model_animation_keyframe *kf1 = keyframe_array + at + index1;
			model_animation_keyframe bkf = {0};
			bkf.bone_index = kf1->bone_index;
			bkf.q.w = 1;
			b32 new_group = !k || kf0->bone_index != kf1->bone_index;
			kf0 = new_group ? 
				&bkf : kf0;

			u32 frame_at = (u32)(totalTimer * 10.0f);
			//total duration of this key frame
			u32 frame_start0 = new_group ? 0 : kf0->frame_start;
			f32 time_start = new_group ? 0.0f : 0.1f * kf0->frame_start;
			u32 timer_frame_repeat = kf1->timer_frame_repeat;
			f32 time_total = 
				0.1f * (kf1->frame_start - frame_start0);
			//Assert(time_total >= 0);
			f32 time_at    = totalTimer - time_start;
			time_at = time_at < 0 ? 0 : 
				!new_group && time_at > time_total ? time_total : time_at;


			b32 valid_index = kf1->bone_index < bone_count;
			if(valid_index && time_total >= 0)
			{
				if(kf1->timer_frame)
				{
					f32 total_frames_t = time_total / timer_frame_repeat;
					time_at = (f32)((i32)(time_at / total_frames_t)) * total_frames_t;
				}
				//Result used for spline operations
				//pick pointing bone
				//model_sprite *spriteBone = model.sprites + keyframe->bone_index;
				model_bone *target_bone = animated_pose.bones + kf1->bone_index;
				model_bone *bind_bone = bind_pose.bones + kf1->bone_index;
				//
				if(kf1->switch_parent && totalTimer >= time_start)
				{
					target_bone->parent = (u8)kf1->parent_index;
				}

				//u16 start u16 end
				f32 deltaTime2 = !time_total ? 1 : (f32)(time_at) / (time_total);
				switch(kf1->spline)
				{

					case model_animation_spline_near:
						{
							//boolean result to f32
							f32 time_start1 = new_group ? 0.0f : 0.1f * kf1->frame_start;
							deltaTime2 = (f32)(totalTimer >= time_start1);
						}break;
					case model_animation_spline_linear:
						{
							deltaTime2 = (f32)(u32)(deltaTime2 * 10.0f);
							deltaTime2 *= 0.1f;
						}break;
					case model_animation_spline_smoothin:
						{
							deltaTime2 *= deltaTime2;
						}break;
					case model_animation_spline_smoothout:
						{
							deltaTime2 = 1.0f - ((1.0f - deltaTime2) * (1.0f - deltaTime2));
						}break;
					default:
						{
							deltaTime2 = 1;
						}break;

				}
				//     deltaTime2 *= deltaTime2; 


				f32 displacementX = kf1->offset.x;
				f32 displacementY = kf1->offset.y;
				f32 displacementZ = kf1->offset.z;

				//target_bone->displacement.x = f32_lerp(target_bone->displacement.x, deltaTime2, displacementX);
				//target_bone->displacement.y = f32_lerp(target_bone->displacement.y, deltaTime2, displacementY);
				//target_bone->displacement.z = f32_lerp(target_bone->displacement.z, deltaTime2, displacementZ);

				//target_bone->rotation_x = f32_lerp(target_bone->rotation_x, deltaTime2, kf0->rotation_x);
				//target_bone->rotation_y = f32_lerp(target_bone->rotation_y, deltaTime2, kf0->rotation_y);
				//target_bone->rotation_z = f32_lerp(target_bone->rotation_z, deltaTime2, kf0->rotation_z);
			//	target_bone->q = quaternion_nlerp(target_bone->q, kf1->q, deltaTime2);
				deltaTime2 = deltaTime2 > 1.0f ? 1.0f : deltaTime2;
				if(totalTimer >= time_start)
				{
					f32 dx = f32_lerp(kf0->offset.x, deltaTime2, kf1->offset.x);
					f32 dy = f32_lerp(kf0->offset.y, deltaTime2, kf1->offset.y);
					f32 dz = f32_lerp(kf0->offset.z, deltaTime2, kf1->offset.z);

					//target_bone->displacement.x = f32_lerp(bind_bone->displacement.x, deltaTime2, dx);
					//target_bone->displacement.y = f32_lerp(bind_bone->displacement.y, deltaTime2, dy);
					//target_bone->displacement.z = f32_lerp(bind_bone->displacement.z, deltaTime2, dz);
//					target_bone->displacement.x = bind_bone->displacement.x + dx;
//					target_bone->displacement.y = bind_bone->displacement.y + dy;
//					target_bone->displacement.z = bind_bone->displacement.z + dz;
					f32 rx = f32_lerp(kf0->rotation_x, deltaTime2, kf1->rotation_x);
					f32 ry = f32_lerp(kf0->rotation_y, deltaTime2, kf1->rotation_y);
					f32 rz = f32_lerp(kf0->rotation_z, deltaTime2, kf1->rotation_z);

					f32 px = f32_lerp(kf0->position.x, deltaTime2, kf1->position.x);
					f32 py = f32_lerp(kf0->position.y, deltaTime2, kf1->position.y);
					f32 pz = f32_lerp(kf0->position.z, deltaTime2, kf1->position.z);

					target_bone->p.x = bind_bone->p.x + dx;
					target_bone->p.y = bind_bone->p.y + dy;
					target_bone->p.z = bind_bone->p.z + dz;
					//target_bone->displacement.x = bind_bone->displacement.x + dx;
					//target_bone->displacement.y = bind_bone->displacement.y + dy;
					//target_bone->displacement.z = bind_bone->displacement.z + dz;
					target_bone->rotation_x = bind_bone->rotation_x + rx;
					target_bone->rotation_y = bind_bone->rotation_y + ry;
					target_bone->rotation_z = bind_bone->rotation_z + rz;

					quaternion qr = quaternion_nlerp(kf0->q, kf1->q, deltaTime2);
					target_bone->q = quaternion_mul(bind_bone->q, qr);
				}
			}
			k++;
		}
		//frame keyframes
		k = 0;
#if 0
		while(k < animation->frame_keyframe_count)
		{
			u32 index0 = animation->frame_keyframes_at + k;
			model_animation_keyframe *kf0 = frame_keyframe_array + index0;

			//total duration of this key frame
			f32 time_start = 0.1f * kf0->frame_start;

			b32 valid_index = kf0->mesh_index < sprite_count;
			if(valid_index && totalTimer >= time_start)
			{
				if(kf0->frame_list_index < frame_list_count)
				{
					model_sprite *sprite = animated_pose.sprites + kf0->mesh_index;
					sprite->frame_list_index = kf0->frame_list_index;
					sprite->frame_list_frame_index = kf0->frame_list_frame_index;
				}
			}
			k++;
		}
#else
		//sub keyframes
		while(k < animation->frame_keyframe_count)
		{
			u32 index0 = animation->frame_keyframes_at + k;
			model_animation_keyframe *kf0 = frame_keyframe_array + index0;

			//total duration of this key frame
			f32 time_start = 0.1f * kf0->frame_start;

			b32 valid_index = kf0->bone_index < bone_count;
			if(valid_index && totalTimer >= time_start)
			{
				model_bone *bone = animated_pose.bones + kf0->bone_index;
				if(kf0->frame_key <= bone->frame_key_count)
				{
					bone->frame_key = kf0->frame_list_index;
					bone->flip_h = kf0->flip_h;
				}
			}
			k++;
		}
#endif
	}

	//transform bones

	//for(u32 b = 0; b < bone_count; b++)
	//{
	//	model_bone *bone = animated_pose.bones + b;
	//	sprite_model_transform bone_transform = 
	//		model_get_foward_transform_quaternion(
	//				animated_pose.bones,
	//				b,
	//				0);

	//	bone->transformed_p = bone_transform.p;
	//	bone->q = quaternion_mul(bone->q, bone_transform.q);

	//	bone->transformed_displacement_x = bone_transform.displacement.x;
	//	bone->transformed_displacement_y = bone_transform.displacement.y;
	//	bone->transformed_displacement_z = bone_transform.displacement.z;

	//}

	model_fill_bone_transformed_data(
			game_renderer,
			animated_pose,
			bone_count,
			view_direction);
}


static void
model_reset_pose(
		model model, model_pose pose)
{
	for(u32 b = 0; b < model.sprite_count; b++)
	{
		pose.sprites[b] = model.pose.sprites[b];
	}
	for(u32 b = 0; b < model.bone_count; b++)
	{
		pose.bones[b] = model.pose.bones[b];
	}
}

static void
model_render(game_renderer *gameRenderer,
		render_commands *render_commands,
		model model_meshes,
		model_pose pose,
		vec3 model_world_position,
		vec2 model_facing_direction)
{

	//create angle and matrix to rotate the character depending on where its looking
	f32 facing_direction_angle = (PI - arctan232(model_facing_direction.x, model_facing_direction.y)) / PI;
	//matrix3x3 direction_matrix = matrix3x3_rotation_scale(
	//		0, 0, facing_direction_angle);
	render_texture **textures = model_meshes.sprite_sheets_a;

//	for(u32 b  =0; b < model_meshes.bone_count; b++)
//	{
//		//pose.bones.p = ;
//		model_bone *bone = pose.bones + b;
//		bone->p = matrix3x3_v3_mul_rows(direction_matrix, bone->p);
//	}

	//gather data
	u32 frame_list_count = model_meshes.frame_list_count;
	u32 sprite_sheet_count = model_meshes.sprite_sheet_count;

	if(!sprite_sheet_count) return;

	u32 bone_count = model_meshes.bone_count;
	u32 sprite_count = model_meshes.sprite_count;

	model_sprite *sprites = pose.sprites;
	model_bone *bones = pose.bones;
	sprite_orientation *uvs_array = model_meshes.uvs;
	model_mesh_frame_list *frame_lists = model_meshes.mesh_frame_list;

	for(u32 b = 0; b < model_meshes.bone_count; b++)
	{
		model_bone *reference_bone = bones + b;
		for(u32 m = 0; m < reference_bone->sprite_count; m++)
		{

			u32 sprite_index = reference_bone->sprites_at + m;
			model_sprite *current_sprite = sprites + sprite_index;
			u32 bone_index = current_sprite->bone_index;

			//get texture from selected sprite

			//for now, always the head
			//Always look up?
			vec3 modelWorldP = {0}; 

			//Get foward final position and rotation in model space
			vec3 model_transformed_position = reference_bone->transformed_p;

			vec3 bone_displacement = {
				reference_bone->transformed_p.x + current_sprite->p.x,
				reference_bone->transformed_p.y + current_sprite->p.y,
				reference_bone->transformed_p.z + current_sprite->p.z
			};
			quaternion bone_rotation = bone_index < bone_count ? 
				reference_bone->transformed_q : QUAT(1, 0, 0, 0);


			vec3 v0 = {0};
			vec3 v1 = {0};
			vec3 v2 = {0};
			vec3 v3 = {0};
			v0 = vec3_add(v0, bone_displacement);
			v1 = vec3_add(v1, bone_displacement);
			v2 = vec3_add(v2, bone_displacement);
			v3 = vec3_add(v3, bone_displacement);


			render_texture *selected_texture = textures[0];
			u32 uvs_count = 0;
			u32 uvs_at = 0;
			uvs sprite_uvs = {0};
			sprite_orientation uvs_orientation = {0};

			selected_texture = textures[current_sprite->sprite_sheet_index];

				uvs_count = 1 + current_sprite->extra_frame_count;
				uvs_at = current_sprite->frame_at;
				sprite_uvs = uvs_array[uvs_at].uvs;
				uvs_orientation = uvs_array[uvs_at];
			//remove check ?
			u32 frame_list_frame = 0;
			

			if(current_sprite->type == model_sprite_billboard)
			{

				//build matrix in order to know the normal of the sprite
				//look_matrix is only used to get the normal for the view direction of the sprite for now.
				//sprite_normal_new = quaternion_v3_mul_foward_inverse(
				//		bone_rotation, sprite_normal_new);
				vec3 sprite_normal_new = reference_bone->normal; 
				vec3 sprite_normal_new2 = {1, 0, 0}; 
				vec3 sprite_normal_new3 = {0, 0, 1}; 
				sprite_normal_new2 = quaternion_v3_mul_foward_inverse(reference_bone->transformed_q, sprite_normal_new2);
				sprite_normal_new3 = quaternion_v3_mul_foward_inverse(reference_bone->transformed_q, sprite_normal_new3);

				//mi intento con dot.
				f32 x_dot = vec3_inner(sprite_normal_new, V3(0, 0, 1)) * PI;
				f32 y_dot = vec3_inner(sprite_normal_new2, V3(0, 0, 1)) * PI;
				f32 z_dot = vec3_inner(sprite_normal_new3, V3(0, -1, 0)) * PI;
				f32 i_inv = TWOPI / 16.0f;
				i32 y_index = (i32)(y_dot / i_inv);

				//u32 uvs_count = current_sprite->uvs_count;
				//u32 uvs_at = current_sprite->uvs_at;
				i32 x_rot_angle_index = (i32)(x_dot / i_inv);
				i32 y_rot_angle_index = y_index;
				i32 z_rot_angle_index = (i32)(z_dot / i_inv);
				//{
				//	f32 normal_y = ABS(sprite_normal_new.y);
				//	f32 normal_x = ABS(sprite_normal_new2.x);
				//	f32 x_angle = arctan232(sprite_normal_new.z, normal_y);
				//	f32 y_angle = arctan232(sprite_normal_new2.z, normal_x);

				//	f32 pi_ost = PI / 16.0f;
				//	y_rot_angle_index = (i32)(y_angle / pi_ost);
				//}

				//select the default one
				uvs_orientation = uvs_array[uvs_at + frame_list_frame];
				//choose the first orientation with the specified skin
				model_bone *bone = reference_bone;
				u32 bone_skin = bone->frame_key;
				//check if this sprite contains a skin with this value.
				if(bone_skin)
				for(u32 s = 1; s < (u32)(1 + current_sprite->extra_frame_count); s++)
				{
					sprite_orientation *ori = uvs_array + uvs_at + s;
					if(ori->skin_index == bone_skin)
					{
						uvs_orientation = *ori;
						break;
					}
				}

				i32 x_rot_index_choosen = 0;
				i32 y_rot_index_choosen = 0;
				//look for an orientation with different pitch index pitch index
				for(u32 f = 0; f < (u32)(1 + current_sprite->extra_frame_count); f++)
				{
					sprite_orientation current_frame = uvs_array[uvs_at + f];
					//prioritize skin before x/y rot
					if(current_frame.skin_index != bone_skin)
					{
						continue;
					}
					if(x_rot_angle_index < 0)
					{
						//if negative angle
						//prioritize skin
				//		b32 choosen = 
				//			(current_frame.skin_index == bone_skin &&
				//			 x_rot_angle_index < current_frame.x_rot_index &&
				//			 x_rot_index_choosen > current_frame.x_rot_index)
						if(x_rot_angle_index < current_frame.x_rot_index
								&& x_rot_index_choosen > current_frame.x_rot_index)
						{
							uvs_orientation = current_frame;
							x_rot_index_choosen = current_frame.x_rot_index;
						}
					}
					else
					{
						if(x_rot_angle_index > current_frame.x_rot_index
								&& x_rot_index_choosen < current_frame.x_rot_index)
						{
							uvs_orientation = current_frame;
							x_rot_index_choosen = current_frame.x_rot_index;
						}
					}
					if(y_rot_angle_index < 0)
					{
						//if negative angle
						if(y_rot_angle_index < current_frame.y_rot_index
								&& y_rot_index_choosen > current_frame.y_rot_index)
						{
							uvs_orientation = current_frame;
							y_rot_index_choosen = current_frame.y_rot_index;
						}
					}
					else
					{
						if(y_rot_angle_index > current_frame.y_rot_index
								&& y_rot_index_choosen < current_frame.y_rot_index)
						{
							uvs_orientation = current_frame;
							y_rot_index_choosen = current_frame.y_rot_index;
						}
					}
				}

				uvs_count = 1;
				u8 counts[orientation_option_count ] = {
					1, 4, 8, 16, 8, 16};
				if(uvs_orientation.option < orientation_option_count)
				{
					uvs_count = counts[uvs_orientation.option];
				}

				vec2 view0 = V2(-sprite_normal_new.x, -sprite_normal_new.y);
				vec2 view1 = V2(-model_facing_direction.x, -model_facing_direction.y);
				u32 sprite_uvs_index = get_orientation_index_foward(
						gameRenderer,
						view0,
						uvs_count);
				u32 sprite_uvs_index1 = get_orientation_index_foward(
						gameRenderer,
						view1,
						uvs_count);

				b32 flip_uvs = 0;
				//might need to fix this one as well.
				if(uvs_orientation.option == orientation_16_flip && sprite_uvs_index > 8)
				{
					sprite_uvs_index = 8 - (sprite_uvs_index - 8);
					flip_uvs = sprite_uvs_index < 7 && sprite_uvs_index > 1;
				}
				else if(uvs_orientation.option == orientation_8_flip && sprite_uvs_index > 4)
				{
					sprite_uvs_index = 4 - (sprite_uvs_index - 4);
					flip_uvs = sprite_uvs_index < 4 && sprite_uvs_index > 0;
				}


				//get the quad orientation depending on the index
				//              sprite_orientation *uvs_orientation = 
				//	          	current_sprite->orientations + current_sprite->orientationIndex;

				vec2 uv0 = uvs_orientation.uv0;
				vec2 uv1 = uvs_orientation.uv1;
				vec2 uv2 = uvs_orientation.uv2;
				vec2 uv3 = uvs_orientation.uv3;
#if 0
				if(x_rot_angle_index > 2)
				{
					render_fill_uvs_from_frames(512, 512, 
							168,
							182,
							4,
							4,
							&uv0,
							&uv1,
							&uv2,
							&uv3);
				}
				if(x_rot_angle_index < -2)
				{
					render_fill_uvs_from_frames(512, 512, 
							155,
							182,
							4,
							4,
							&uv0,
							&uv1,
							&uv2,
							&uv3);
				}
				if(x_rot_angle_index < -4)
				{
					render_fill_uvs_from_frames(512, 512, 
							162,
							182,
							5,
							4,
							&uv0,
							&uv1,
							&uv2,
							&uv3);
				}
#endif
				//advance uvs by frame
				{
					u32 fx = 0;
					u32 fy = 0;
					u32 fw = 0;
					u32 fh = 0;
					render_fill_frames_from_uvs(
							selected_texture->width,
							selected_texture->height,
							uv0,
							uv1,
							uv2,
							uv3,
							&fx,
							&fy,
							&fw,
							&fh
							);
					uv0.x += (f32)fw / selected_texture->width * sprite_uvs_index;
					uv1.x += (f32)fw / selected_texture->width * sprite_uvs_index;
					uv2.x += (f32)fw / selected_texture->width * sprite_uvs_index;
					uv3.x += (f32)fw / selected_texture->width * sprite_uvs_index;
					if(flip_uvs)
					{
						render_flip_and_fill_uvs_horizontally(
								&uv0,
								&uv1,
								&uv2,
								&uv3);
					}
				}


				adjust_vertices_to_uvs(
						selected_texture,
						&v0,
						&v1,
						&v2,
						&v3,
						uv0,
						uv1,
						uv2,
						uv3);

#if 1

				//rotation point
				vec3 pivotPoint = {
					0,
					0,
					0,
				};
				vec3 pivot = vec3_add(reference_bone->transformed_p, current_sprite->pivot);

				//Scale and push to camera z axis
				//Only move the quad horizontal sizes to face the camera, preserve Y
				if(current_sprite->face_axis == billboard_face_x)
				{

					//rotate by the skeleton and hierarchy
					//put vertices to the origin of the world
					v0 = vec3_sub(v0, pivot);
					v1 = vec3_sub(v1, pivot);
					v2 = vec3_sub(v2, pivot);
					v3 = vec3_sub(v3, pivot);


					v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
					v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
					v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
					v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);


					v0 = vec3_add(pivot, v0);
					v1 = vec3_add(pivot, v1);
					v2 = vec3_add(pivot, v2);
					v3 = vec3_add(pivot, v3);



					//v0 = matrix3x3_v3_mul_rows(direction_matrix, v0);
					//v1 = matrix3x3_v3_mul_rows(direction_matrix, v1);
					//v2 = matrix3x3_v3_mul_rows(direction_matrix, v2);
					//v3 = matrix3x3_v3_mul_rows(direction_matrix, v3);

					//add the world position to correctly face the camera
					v0 = vec3_add(model_world_position, v0);
					v1 = vec3_add(model_world_position, v1);
					v2 = vec3_add(model_world_position, v2);
					v3 = vec3_add(model_world_position, v3);


					mesh_points points = get_mesh_billboard_points_x(gameRenderer,
							v0,
							v1,
							v2,
							v3,
							pivotPoint);

					v0 = points.v0;
					v1 = points.v1;
					v2 = points.v2;
					v3 = points.v3;


				}
				else
				{

					v0 = vec3_sub(v0, model_transformed_position);
					v1 = vec3_sub(v1, model_transformed_position);
					v2 = vec3_sub(v2, model_transformed_position);
					v3 = vec3_sub(v3, model_transformed_position);

#if 1
					vec3 mid = vertices_get_mid_point(
							v0,
							v1,
							v2,
							v3);
					vec3 d_v0_m = vec3_sub(v0,mid);
					vec3 d_v1_m = vec3_sub(v1,mid);
					vec3 d_v2_m = vec3_sub(v2,mid);
					vec3 d_v3_m = vec3_sub(v3,mid);

					mid = quaternion_v3_mul_foward_inverse(bone_rotation, mid);

					v0 = vec3_add(d_v0_m, mid);
					v1 = vec3_add(d_v1_m, mid);
					v2 = vec3_add(d_v2_m, mid);
					v3 = vec3_add(d_v3_m, mid);
#else

					v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
					v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
					v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
					v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);
#endif
					v0 = vec3_add(model_transformed_position, v0);
					v1 = vec3_add(model_transformed_position, v1);
					v2 = vec3_add(model_transformed_position, v2);
					v3 = vec3_add(model_transformed_position, v3);


					mesh_points points = get_mesh_billboard_points_depth(gameRenderer,
							v0,
							v1,
							v2,
							v3,
							current_sprite->pivot,
							current_sprite->depth_z
							);
					//				v0 = vec3_add(v0, current_sprite->pivot);
					//				v1 = vec3_add(v1, current_sprite->pivot);
					//				v2 = vec3_add(v2, current_sprite->pivot);
					//				v3 = vec3_add(v3, current_sprite->pivot);

					v0 = points.v0;
					v1 = points.v1;
					v2 = points.v2;
					v3 = points.v3;

					//add final world position
					v0 = vec3_add(model_world_position, v0);
					v1 = vec3_add(model_world_position, v1);
					v2 = vec3_add(model_world_position, v2);
					v3 = vec3_add(model_world_position, v3);

				}
				//else
				//{

				//	//rotate by the skeleton and hierarchy
				//	v0 = vec3_sub(v0, model_transformed_position);
				//	v1 = vec3_sub(v1, model_transformed_position);
				//	v2 = vec3_sub(v2, model_transformed_position);
				//	v3 = vec3_sub(v3, model_transformed_position);

				//	//NOTE: Not everything is multiplying by PI

				//	v0 = quaternion_v3_mul_foward_inverse(rotation_q, v0);
				//	v1 = quaternion_v3_mul_foward_inverse(rotation_q, v1);
				//	v2 = quaternion_v3_mul_foward_inverse(rotation_q, v2);
				//	v3 = quaternion_v3_mul_foward_inverse(rotation_q, v3);

				//	v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
				//	v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
				//	v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
				//	v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);
				//	add_y_bias_to_vertices(
				//			&v0,
				//			&v1,
				//			&v2,
				//			&v3);

				//	v0 = vec3_add(model_transformed_position, v0);
				//	v1 = vec3_add(model_transformed_position, v1);
				//	v2 = vec3_add(model_transformed_position, v2);
				//	v3 = vec3_add(model_transformed_position, v3);

				//	//add final world position
				//	v0 = vec3_add(model_world_position, v0);
				//	v1 = vec3_add(model_world_position, v1);
				//	v2 = vec3_add(model_world_position, v2);
				//	v3 = vec3_add(model_world_position, v3);
				//}

				//
				// UNTESTED/UNFINISHED
				//
				//after all rotations, get billboard points
				add_y_bias_to_vertices(
						&v0,
						&v1,
						&v2,
						&v3,
						render_commands->gameRenderer->sprite_skew);
				vec3 distance_model_camera_normalized = vec3_normalize_safe(vec3_sub(gameRenderer->camera_position, model_transformed_position));
				vec3 depth_xAxis = vec3_cross(distance_model_camera_normalized, V3(0, 0, 1));

				vec3 camera_x_depth = vec3_scale(depth_xAxis, current_sprite->depth_x);
				vec3 camera_y_depth = vec3_scale(gameRenderer->camera_y, current_sprite->depth_y);
				//vec3 camera_z_depth = vec3_scale(gameRenderer->camera_z, current_sprite->depth_z);
				vec3 camera_z_depth = vec3_scale(distance_model_camera_normalized, current_sprite->depth_z);
				f32 add_z = camera_x_depth.z + camera_y_depth.z + camera_z_depth.z;
				{

					vec3 mp = vertices_get_mid_point(v0, v1, v2, v3);
					vec3 v0d = vec3_sub(mp, v0);
					vec3 v1d = vec3_sub(mp, v1);
					vec3 v2d = vec3_sub(mp, v2);
					vec3 v3d = vec3_sub(mp, v3);
					mp = vec3_add(camera_z_depth, mp);

					v0 = vec3_sub(mp, v0d);
					v1 = vec3_sub(mp, v1d);
					v2 = vec3_sub(mp, v2d);
					v3 = vec3_sub(mp, v3d);

					if(m == 0 && 0)
					{
						vec3 mid = vertices_get_mid_point(
								v0,
								v1,
								v2,
								v3);
						render_draw_cube(render_commands, mid, V3(4, 4, 4), V4(255, 255, 255, 255));
					}


					//v0 = vec3_add(camera_z_depth, v0);
					//v1 = vec3_add(camera_z_depth, v1);
					//v2 = vec3_add(camera_z_depth, v2);
					//v3 = vec3_add(camera_z_depth, v3);
				}

				//			vec3 orientation_offset = quaternion_v3_mul_inverse_foward(q_cam, uvs_orientation.offset);
				if(current_sprite->face_axis == billboard_face_x)
				{
				}
				else
				{
					vec3 orientation_offset = vec3_scale(gameRenderer->camera_z, uvs_orientation.offset.z);
					vec3 orientation_offset_x = vec3_scale(gameRenderer->camera_x, uvs_orientation.offset.x);
					vec3 orientation_offset_y = vec3_scale(gameRenderer->camera_y, uvs_orientation.offset.y);
					v0 = vec3_add(orientation_offset, v0);
					v1 = vec3_add(orientation_offset, v1);
					v2 = vec3_add(orientation_offset, v2);
					v3 = vec3_add(orientation_offset, v3);

					v0 = vec3_add(orientation_offset_x, v0);
					v1 = vec3_add(orientation_offset_x, v1);
					v2 = vec3_add(orientation_offset_x, v2);
					v3 = vec3_add(orientation_offset_x, v3);

					v0 = vec3_add(orientation_offset_y, v0);
					v1 = vec3_add(orientation_offset_y, v1);
					v2 = vec3_add(orientation_offset_y, v2);
					v3 = vec3_add(orientation_offset_y, v3);
				}


#endif


					if(current_sprite->flip_h)
					{
						render_flip_and_fill_uvs_horizontally(&uv0,
								&uv1,
								&uv2,
								&uv3);
					}

					render_push_billboard(render_commands,
							selected_texture,
							v0,
							v1,
							v2,
							v3,
							uv0,
							uv1,
							uv2,
							uv3,
							V4(255, 255, 255 ,255));

			}
			else if(current_sprite->type == model_sprite_mesh)
			{
				//add specified size
				{
#if 1
					f32 shx = current_sprite->size.x * 0.5f;
					f32 shy = current_sprite->size.y * 0.5f;
					f32 shz = current_sprite->size.z * 0.5f;

					f32 shx2 = current_sprite->size2.x * 0.5f;
					f32 shy2 = current_sprite->size2.y * 0.5f;
					f32 shz2 = current_sprite->size2.z * 0.5f;

					v0.x -= shx;
					v0.y -= shy;
					v0.z -= shz;

					v1.x -= shx;
					v1.y += shy;
					v1.z += shz;

					v2.x += shx;
					v2.y += shy;
					v2.z += shz;

					v3.x += shx;
					v3.y -= shy;
					v3.z -= shz;

					v0.y += shy2;
					v0.z -= shz2;

					v1.y += shy2;
					v1.z += shz2;

					v2.y -= shy2;
					v2.z += shz2;

					v3.y -= shy2;
					v3.z -= shz2;
#else
					f32 shx = current_sprite->size.x;
					f32 shy = current_sprite->size.y;
					f32 shz = current_sprite->size.z;

					f32 shx2 = current_sprite->size2.x;
					f32 shy2 = current_sprite->size2.y;
					f32 shz2 = current_sprite->size2.z;

					v1.y += shy;
					v1.z += shz;

					v2.x += shx;
					v2.y += shy;
					v2.z += shz;

					v3.x += shx;

					v1.y += shy2;
					v1.z += shz2;
					v0.y += shy2;
					v2.z += shz2;
#endif
				}

				v0 = vec3_sub(v0, model_transformed_position);
				v1 = vec3_sub(v1, model_transformed_position);
				v2 = vec3_sub(v2, model_transformed_position);
				v3 = vec3_sub(v3, model_transformed_position);

				//NOTA: Esto gira los sprites junto a su correspondiente nodo
				//v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
				//v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
				//v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
				//v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);

				v0 = vec3_add(model_transformed_position, v0);
				v1 = vec3_add(model_transformed_position, v1);
				v2 = vec3_add(model_transformed_position, v2);
				v3 = vec3_add(model_transformed_position, v3);

				//add final world position
				v0 = vec3_add(model_world_position, v0);
				v1 = vec3_add(model_world_position, v1);
				v2 = vec3_add(model_world_position, v2);
				v3 = vec3_add(model_world_position, v3);

				add_y_bias_to_vertices(
						&v0,
						&v1,
						&v2,
						&v3,
						render_commands->gameRenderer->sprite_skew);

				if(sprite_sheet_count)
				{

					render_push_quad(render_commands,
							selected_texture, 
							v0,
							v1,
							v2,
							v3,
							uvs_orientation.uv0,
							uvs_orientation.uv1,
							uvs_orientation.uv2,
							uvs_orientation.uv3,
							V4(255, 255, 255 ,255));
				}

			}
		}
	}


}


static void
old_model_render(game_renderer *gameRenderer,
		render_commands *render_commands,
		model model_meshes,
		model_pose pose,
		vec3 model_world_position,
		vec2 model_facing_direction)
{

	//create angle and matrix to rotate the character depending on where its looking
	f32 facing_direction_angle = (PI - arctan232(model_facing_direction.x, model_facing_direction.y)) / PI;
	//matrix3x3 direction_matrix = matrix3x3_rotation_scale(
	//		0, 0, facing_direction_angle);
	render_texture **textures = model_meshes.sprite_sheets_a;

//	for(u32 b  =0; b < model_meshes.bone_count; b++)
//	{
//		//pose.bones.p = ;
//		model_bone *bone = pose.bones + b;
//		bone->p = matrix3x3_v3_mul_rows(direction_matrix, bone->p);
//	}

	//gather data
	u32 frame_list_count = model_meshes.frame_list_count;
	u32 sprite_sheet_count = model_meshes.sprite_sheet_count;
	u32 bone_count = model_meshes.bone_count;
	u32 sprite_count = model_meshes.sprite_count;

	model_sprite *sprites = pose.sprites;
	model_bone *bones = pose.bones;
	sprite_orientation *uvs_array = model_meshes.uvs;
	model_mesh_frame_list *frame_lists = model_meshes.mesh_frame_list;

	for(u32 m = 0; m < sprite_count; m++)
	{

		model_sprite *current_sprite = sprites + m;
		u32 bone_index = current_sprite->bone_index;

		//get texture from selected sprite

		model_bone *reference_bone = bones + bone_index;
		model_bone *bone = reference_bone;
		u32 bone_skin = bone->frame_key;
		//check if this sprite contains a skin with this value.
		for(u32 s = 0; s < current_sprite->skin_count; s++)
		{
			model_sprite *sprite = sprites + m + s;
			if(sprite->skin == bone_skin)
			{
				current_sprite = sprite;
				break;
			}
		}
		//for now, always the head
		//Always look up?
		vec3 modelWorldP = {0}; 

		//Get foward final position and rotation in model space
		vec3 model_transformed_position = reference_bone->transformed_p;

		vec3 bone_displacement = {
			reference_bone->transformed_p.x + current_sprite->p.x,
			reference_bone->transformed_p.y + current_sprite->p.y,
			reference_bone->transformed_p.z + current_sprite->p.z
		};
		quaternion bone_rotation = bone_index < bone_count ? 
			reference_bone->transformed_q : QUAT(1, 0, 0, 0);


		vec3 v0 = {0};
		vec3 v1 = {0};
		vec3 v2 = {0};
		vec3 v3 = {0};
		v0 = vec3_add(v0, bone_displacement);
		v1 = vec3_add(v1, bone_displacement);
		v2 = vec3_add(v2, bone_displacement);
		v3 = vec3_add(v3, bone_displacement);
		

		render_texture *selected_texture = textures[0];
		u32 uvs_count = 0;
		u32 uvs_at = 0;
		uvs sprite_uvs = {0};
		sprite_orientation uvs_orientation = {0};

		model_mesh_frame_list *frame_list = current_sprite->frame_list_index < frame_list_count ?
			frame_lists + current_sprite->frame_list_index : 0;

		if(frame_list)
		{
			uvs_count = frame_list->uvs_count;
			uvs_at = frame_list->uvs_at;
			sprite_uvs = uvs_array[uvs_at].uvs;
			uvs_orientation = uvs_array[uvs_at];
			selected_texture = textures[frame_list->sprite_index];
		}
			//remove check ?
			u32 frame_list_frame = current_sprite->frame_list_frame_index >= frame_list->total_frames_count ?
				frame_list->total_frames_count - 1 : current_sprite->frame_list_frame_index;

		if(current_sprite->type == model_sprite_billboard)
		{

			//build matrix in order to know the normal of the sprite
			//look_matrix is only used to get the normal for the view direction of the sprite for now.
			vec3 sprite_normal_new = {0, 1, 0};
			//sprite_normal_new = quaternion_v3_mul_foward_inverse(
			//		bone_rotation, sprite_normal_new);
			sprite_normal_new = reference_bone->normal; 
			vec3 sprite_normal_new2 = {1, 0, 0}; 
			sprite_normal_new2 = quaternion_v3_mul_foward_inverse(reference_bone->transformed_q, sprite_normal_new2);

			//mi intento con dot.
			f32 x_dot = vec3_inner(sprite_normal_new, V3(0, 0, 1)) * PI;
			f32 y_dot = vec3_inner(sprite_normal_new2, V3(0, 0, 1)) * PI;
			f32 i_inv = TWOPI / 16.0f;
			i32 y_index = (i32)(y_dot / i_inv);

			//u32 uvs_count = current_sprite->uvs_count;
			//u32 uvs_at = current_sprite->uvs_at;
			i32 x_rot_angle_index = (i32)(x_dot / i_inv);;
			i32 y_rot_angle_index = y_index;
			//{
			//	f32 normal_y = ABS(sprite_normal_new.y);
			//	f32 normal_x = ABS(sprite_normal_new2.x);
			//	f32 x_angle = arctan232(sprite_normal_new.z, normal_y);
			//	f32 y_angle = arctan232(sprite_normal_new2.z, normal_x);

			//	f32 pi_ost = PI / 16.0f;
			//	y_rot_angle_index = (i32)(y_angle / pi_ost);
			//}
			//select the default one
			uvs_orientation = uvs_array[uvs_at + frame_list_frame];
			i32 x_rot_index_choosen = 0;
 			i32 y_rot_index_choosen = 0;
			//look for an orientation with different pitch index pitch index
#if 0
			for(u32 f = 0; f < frame_list->total_frames_count; f++)
			{
				sprite_orientation current_frame = uvs_array[uvs_at + f];
				if(x_rot_angle_index < 0)
				{
					//if negative angle
					if(x_rot_angle_index < current_frame.x_rot_index
						&& x_rot_index_choosen > current_frame.x_rot_index)
					{
						uvs_orientation = current_frame;
						x_rot_index_choosen = current_frame.x_rot_index;
					}
				}
				else
				{
					if(x_rot_angle_index > current_frame.x_rot_index
					   && x_rot_index_choosen < current_frame.x_rot_index)
					{
						uvs_orientation = current_frame;
						x_rot_index_choosen = current_frame.x_rot_index;
					}
				}
			}
#else
			for(u32 f = 0; f < frame_list->total_frames_count; f++)
			{
				sprite_orientation current_frame = uvs_array[uvs_at + f];
				if(x_rot_angle_index < 0)
				{
					//if negative angle
					if(x_rot_angle_index < current_frame.x_rot_index
						&& x_rot_index_choosen > current_frame.x_rot_index)
					{
						uvs_orientation = current_frame;
						x_rot_index_choosen = current_frame.x_rot_index;
					}
				}
				else
				{
					if(x_rot_angle_index > current_frame.x_rot_index
					   && x_rot_index_choosen < current_frame.x_rot_index)
					{
						uvs_orientation = current_frame;
						x_rot_index_choosen = current_frame.x_rot_index;
					}
				}
				if(y_rot_angle_index < 0)
				{
					//if negative angle
					if(y_rot_angle_index < current_frame.y_rot_index
						&& y_rot_index_choosen > current_frame.y_rot_index)
					{
						uvs_orientation = current_frame;
						y_rot_index_choosen = current_frame.y_rot_index;
					}
				}
				else
				{
					if(y_rot_angle_index > current_frame.y_rot_index
					   && y_rot_index_choosen < current_frame.y_rot_index)
					{
						uvs_orientation = current_frame;
						y_rot_index_choosen = current_frame.y_rot_index;
					}
				}
			}
#endif

			uvs_count = 1;
			u8 counts[orientation_option_count ] = {
				1, 4, 8, 16, 8, 16};
			if(uvs_orientation.option < orientation_option_count)
			{
				uvs_count = counts[uvs_orientation.option];
			}

			u32 sprite_uvs_index = get_orientation_index_foward(
					gameRenderer,
					V2(-sprite_normal_new.x, -sprite_normal_new.y),
					uvs_count);
			sprite_uvs_index = (u32)(sprite_uvs_index * (uvs_count / 16.0f));
			b32 flip_uvs = 0;
			if(uvs_orientation.option == orientation_16_flip && sprite_uvs_index > 8)
			{
				sprite_uvs_index = 8 - (sprite_uvs_index - 8);
				flip_uvs = sprite_uvs_index < 7 && sprite_uvs_index > 1;
			}
			else if(uvs_orientation.option == orientation_8_flip && sprite_uvs_index > 3)
			{
				sprite_uvs_index = 4 - (sprite_uvs_index - 3);
				flip_uvs = sprite_uvs_index < 4 && sprite_uvs_index > 0;
			}


			//get the quad orientation depending on the index
			//              sprite_orientation *uvs_orientation = 
			//	          	current_sprite->orientations + current_sprite->orientationIndex;

			vec2 uv0 = uvs_orientation.uv0;
			vec2 uv1 = uvs_orientation.uv1;
			vec2 uv2 = uvs_orientation.uv2;
			vec2 uv3 = uvs_orientation.uv3;
#if 0
			if(x_rot_angle_index > 2)
			{
				render_fill_uvs_from_frames(512, 512, 
						168,
						182,
						4,
						4,
						&uv0,
						&uv1,
						&uv2,
						&uv3);
			}
			if(x_rot_angle_index < -2)
			{
				render_fill_uvs_from_frames(512, 512, 
						155,
						182,
						4,
						4,
						&uv0,
						&uv1,
						&uv2,
						&uv3);
			}
			if(x_rot_angle_index < -4)
			{
				render_fill_uvs_from_frames(512, 512, 
						162,
						182,
						5,
						4,
						&uv0,
						&uv1,
						&uv2,
						&uv3);
			}
#endif
			//advance uvs by frame
			{
				u32 fx = 0;
				u32 fy = 0;
				u32 fw = 0;
				u32 fh = 0;
				render_fill_frames_from_uvs(
						selected_texture->width,
						selected_texture->height,
						uv0,
						uv1,
						uv2,
						uv3,
						&fx,
						&fy,
						&fw,
						&fh
						);
				uv0.x += (f32)fw / selected_texture->width * sprite_uvs_index;
				uv1.x += (f32)fw / selected_texture->width * sprite_uvs_index;
				uv2.x += (f32)fw / selected_texture->width * sprite_uvs_index;
				uv3.x += (f32)fw / selected_texture->width * sprite_uvs_index;
				if(flip_uvs)
				{
					render_flip_and_fill_uvs_horizontally(
							&uv0,
							&uv1,
							&uv2,
							&uv3);
				}
			}


			adjust_vertices_to_uvs(
					selected_texture,
					&v0,
					&v1,
					&v2,
					&v3,
					uv0,
					uv1,
					uv2,
					uv3);

#if 1

			//rotation point
			vec3 pivotPoint = {
			0,
			0,
			0,
			};
			vec3 pivot = vec3_add(reference_bone->transformed_p, current_sprite->pivot);

			//Scale and push to camera z axis
			//Only move the quad horizontal sizes to face the camera, preserve Y
			if(current_sprite->face_axis == billboard_face_x)
			{

				//rotate by the skeleton and hierarchy
				//put vertices to the origin of the world
				v0 = vec3_sub(v0, pivot);
				v1 = vec3_sub(v1, pivot);
				v2 = vec3_sub(v2, pivot);
				v3 = vec3_sub(v3, pivot);


				v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
				v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
				v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
				v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);


				v0 = vec3_add(pivot, v0);
				v1 = vec3_add(pivot, v1);
				v2 = vec3_add(pivot, v2);
				v3 = vec3_add(pivot, v3);



				//v0 = matrix3x3_v3_mul_rows(direction_matrix, v0);
				//v1 = matrix3x3_v3_mul_rows(direction_matrix, v1);
				//v2 = matrix3x3_v3_mul_rows(direction_matrix, v2);
				//v3 = matrix3x3_v3_mul_rows(direction_matrix, v3);

				//add the world position to correctly face the camera
				v0 = vec3_add(model_world_position, v0);
				v1 = vec3_add(model_world_position, v1);
				v2 = vec3_add(model_world_position, v2);
				v3 = vec3_add(model_world_position, v3);


				mesh_points points = get_mesh_billboard_points_x(gameRenderer,
						v0,
						v1,
						v2,
						v3,
						pivotPoint);

				v0 = points.v0;
				v1 = points.v1;
				v2 = points.v2;
				v3 = points.v3;


			}
			else
			{

				v0 = vec3_sub(v0, model_transformed_position);
				v1 = vec3_sub(v1, model_transformed_position);
				v2 = vec3_sub(v2, model_transformed_position);
				v3 = vec3_sub(v3, model_transformed_position);

#if 1
                vec3 mid = vertices_get_mid_point(
						v0,
						v1,
						v2,
						v3);
				vec3 d_v0_m = vec3_sub(v0,mid);
				vec3 d_v1_m = vec3_sub(v1,mid);
				vec3 d_v2_m = vec3_sub(v2,mid);
				vec3 d_v3_m = vec3_sub(v3,mid);

				mid = quaternion_v3_mul_foward_inverse(bone_rotation, mid);

				v0 = vec3_add(d_v0_m, mid);
				v1 = vec3_add(d_v1_m, mid);
				v2 = vec3_add(d_v2_m, mid);
				v3 = vec3_add(d_v3_m, mid);
#else

				v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
				v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
				v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
				v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);
#endif
				v0 = vec3_add(model_transformed_position, v0);
				v1 = vec3_add(model_transformed_position, v1);
				v2 = vec3_add(model_transformed_position, v2);
				v3 = vec3_add(model_transformed_position, v3);


				mesh_points points = get_mesh_billboard_points_depth(gameRenderer,
						v0,
						v1,
						v2,
						v3,
					current_sprite->pivot,
					current_sprite->depth_z
						);
//				v0 = vec3_add(v0, current_sprite->pivot);
//				v1 = vec3_add(v1, current_sprite->pivot);
//				v2 = vec3_add(v2, current_sprite->pivot);
//				v3 = vec3_add(v3, current_sprite->pivot);

				v0 = points.v0;
				v1 = points.v1;
				v2 = points.v2;
				v3 = points.v3;

				//add final world position
				v0 = vec3_add(model_world_position, v0);
				v1 = vec3_add(model_world_position, v1);
				v2 = vec3_add(model_world_position, v2);
				v3 = vec3_add(model_world_position, v3);

			}
			//else
			//{

			//	//rotate by the skeleton and hierarchy
			//	v0 = vec3_sub(v0, model_transformed_position);
			//	v1 = vec3_sub(v1, model_transformed_position);
			//	v2 = vec3_sub(v2, model_transformed_position);
			//	v3 = vec3_sub(v3, model_transformed_position);

			//	//NOTE: Not everything is multiplying by PI

			//	v0 = quaternion_v3_mul_foward_inverse(rotation_q, v0);
			//	v1 = quaternion_v3_mul_foward_inverse(rotation_q, v1);
			//	v2 = quaternion_v3_mul_foward_inverse(rotation_q, v2);
			//	v3 = quaternion_v3_mul_foward_inverse(rotation_q, v3);

			//	v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
			//	v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
			//	v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
			//	v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);
			//	add_y_bias_to_vertices(
			//			&v0,
			//			&v1,
			//			&v2,
			//			&v3);

			//	v0 = vec3_add(model_transformed_position, v0);
			//	v1 = vec3_add(model_transformed_position, v1);
			//	v2 = vec3_add(model_transformed_position, v2);
			//	v3 = vec3_add(model_transformed_position, v3);

			//	//add final world position
			//	v0 = vec3_add(model_world_position, v0);
			//	v1 = vec3_add(model_world_position, v1);
			//	v2 = vec3_add(model_world_position, v2);
			//	v3 = vec3_add(model_world_position, v3);
			//}

			//
			// UNTESTED/UNFINISHED
			//
				//after all rotations, get billboard points
				add_y_bias_to_vertices(
						&v0,
						&v1,
						&v2,
						&v3,
						render_commands->gameRenderer->sprite_skew);
			vec3 distance_model_camera_normalized = vec3_normalize_safe(vec3_sub(gameRenderer->camera_position, model_transformed_position));
			vec3 depth_xAxis = vec3_cross(distance_model_camera_normalized, V3(0, 0, 1));

			vec3 camera_x_depth = vec3_scale(depth_xAxis, current_sprite->depth_x);
			vec3 camera_y_depth = vec3_scale(gameRenderer->camera_y, current_sprite->depth_y);
			//vec3 camera_z_depth = vec3_scale(gameRenderer->camera_z, current_sprite->depth_z);
			vec3 camera_z_depth = vec3_scale(distance_model_camera_normalized, current_sprite->depth_z);
			f32 add_z = camera_x_depth.z + camera_y_depth.z + camera_z_depth.z;
			{

				vec3 mp = vertices_get_mid_point(v0, v1, v2, v3);
				vec3 v0d = vec3_sub(mp, v0);
				vec3 v1d = vec3_sub(mp, v1);
				vec3 v2d = vec3_sub(mp, v2);
				vec3 v3d = vec3_sub(mp, v3);
				mp = vec3_add(camera_z_depth, mp);

				v0 = vec3_sub(mp, v0d);
				v1 = vec3_sub(mp, v1d);
				v2 = vec3_sub(mp, v2d);
				v3 = vec3_sub(mp, v3d);

				if(m == 0 && 0)
				{
					vec3 mid = vertices_get_mid_point(
						v0,
						v1,
						v2,
						v3);
					render_draw_cube(render_commands, mid, V3(4, 4, 4), V4(255, 255, 255, 255));
				}


				//v0 = vec3_add(camera_z_depth, v0);
				//v1 = vec3_add(camera_z_depth, v1);
				//v2 = vec3_add(camera_z_depth, v2);
				//v3 = vec3_add(camera_z_depth, v3);
			}

//			vec3 orientation_offset = quaternion_v3_mul_inverse_foward(q_cam, uvs_orientation.offset);
			if(current_sprite->face_axis == billboard_face_x)
			{
			}
			else
			{
			vec3 orientation_offset = vec3_scale(gameRenderer->camera_z, uvs_orientation.offset.z);
			vec3 orientation_offset_x = vec3_scale(gameRenderer->camera_x, uvs_orientation.offset.x);
			vec3 orientation_offset_y = vec3_scale(gameRenderer->camera_y, uvs_orientation.offset.y);
			v0 = vec3_add(orientation_offset, v0);
			v1 = vec3_add(orientation_offset, v1);
			v2 = vec3_add(orientation_offset, v2);
			v3 = vec3_add(orientation_offset, v3);

			v0 = vec3_add(orientation_offset_x, v0);
			v1 = vec3_add(orientation_offset_x, v1);
			v2 = vec3_add(orientation_offset_x, v2);
			v3 = vec3_add(orientation_offset_x, v3);

			v0 = vec3_add(orientation_offset_y, v0);
			v1 = vec3_add(orientation_offset_y, v1);
			v2 = vec3_add(orientation_offset_y, v2);
			v3 = vec3_add(orientation_offset_y, v3);
			}


#endif


			if(sprite_sheet_count)
			{
				if(current_sprite->flip_h)
				{
					render_flip_and_fill_uvs_horizontally(&uv0,
							&uv1,
							&uv2,
							&uv3);
				}

				render_push_billboard(render_commands,
						selected_texture,
						v0,
						v1,
						v2,
						v3,
						uv0,
						uv1,
						uv2,
						uv3,
						V4(255, 255, 255 ,255));
					
			}
		}
		else if(current_sprite->type == model_sprite_mesh)
		{
			//add specified size
			{
#if 1
				f32 shx = current_sprite->size.x * 0.5f;
				f32 shy = current_sprite->size.y * 0.5f;
				f32 shz = current_sprite->size.z * 0.5f;

				f32 shx2 = current_sprite->size2.x * 0.5f;
				f32 shy2 = current_sprite->size2.y * 0.5f;
				f32 shz2 = current_sprite->size2.z * 0.5f;

				v0.x -= shx;
				v0.y -= shy;
				v0.z -= shz;

				v1.x -= shx;
				v1.y += shy;
				v1.z += shz;

				v2.x += shx;
				v2.y += shy;
				v2.z += shz;

				v3.x += shx;
				v3.y -= shy;
				v3.z -= shz;

				v0.y += shy2;
				v0.z -= shz2;

				v1.y += shy2;
				v1.z += shz2;

				v2.y -= shy2;
				v2.z += shz2;

				v3.y -= shy2;
				v3.z -= shz2;
#else
				f32 shx = current_sprite->size.x;
				f32 shy = current_sprite->size.y;
				f32 shz = current_sprite->size.z;

				f32 shx2 = current_sprite->size2.x;
				f32 shy2 = current_sprite->size2.y;
				f32 shz2 = current_sprite->size2.z;

				v1.y += shy;
				v1.z += shz;

				v2.x += shx;
				v2.y += shy;
				v2.z += shz;

				v3.x += shx;

				v1.y += shy2;
				v1.z += shz2;
				v0.y += shy2;
				v2.z += shz2;
#endif
			}

			v0 = vec3_sub(v0, model_transformed_position);
			v1 = vec3_sub(v1, model_transformed_position);
			v2 = vec3_sub(v2, model_transformed_position);
			v3 = vec3_sub(v3, model_transformed_position);

			v0 = quaternion_v3_mul_foward_inverse(bone_rotation, v0);
			v1 = quaternion_v3_mul_foward_inverse(bone_rotation, v1);
			v2 = quaternion_v3_mul_foward_inverse(bone_rotation, v2);
			v3 = quaternion_v3_mul_foward_inverse(bone_rotation, v3);

			v0 = vec3_add(model_transformed_position, v0);
			v1 = vec3_add(model_transformed_position, v1);
			v2 = vec3_add(model_transformed_position, v2);
			v3 = vec3_add(model_transformed_position, v3);

			//add final world position
			v0 = vec3_add(model_world_position, v0);
			v1 = vec3_add(model_world_position, v1);
			v2 = vec3_add(model_world_position, v2);
			v3 = vec3_add(model_world_position, v3);

				add_y_bias_to_vertices(
						&v0,
						&v1,
						&v2,
						&v3,
						render_commands->gameRenderer->sprite_skew);

			if(sprite_sheet_count)
			{

				render_push_quad(render_commands,
						selected_texture, 
						v0,
						v1,
						v2,
						v3,
						uvs_orientation.uv0,
						uvs_orientation.uv1,
						uvs_orientation.uv2,
						uvs_orientation.uv3,
						V4(255, 255, 255 ,255));
			}

		}
	}


}

static void
model_render_meshes(render_commands *commands,
		                   game_assets *assets,
						   model bind_model,
						   vec3 model_world_position,
						   vec2 model_looking_direction)
{
	bind_model.bone_count = 0;
	model_render(
			commands->gameRenderer,
			commands,
			bind_model,
			bind_model.pose,
			model_world_position,
			model_looking_direction);
}

static inline vec2
model_get_view_direction(
		model *model,
		vec2 view_direction)
{
	f32 angle = arctan232(view_direction.x, view_direction.y);
	i32 dir = SIGN(angle);
	//angle = (f32)(i32)(angle * model->orientation_amount) / model->orientation_amount;
	Assert(model->orientation_amount);
	f32 angle_div = TWOPI / model->orientation_amount;
	angle = (f32)(i32)(ABS(angle) / angle_div + 0.01f) * angle_div;
	angle *= dir;
	vec2 view_result = {
		sin32(angle),
		cos32(angle),
	};
	return(view_result);
}

static void
model_render_attached_models(
		render_commands *commands,
		model_render_data *model_data,
		vec3 position,
		vec2 view_direction)
{
	//render attached models
	for(u32 a = 0; a < model_data->attached_models_count; a++)
	{
		model_attachment_data *attachment_data = model_data->attach_data + a;
		model *attached_model = attachment_data->model;
		if(attached_model)
		{
			Assert(attachment_data->bone_index < model_data->model->bone_count);
			//get transformed bone
			model_bone *parent_bone = model_data->animated_pose.bones + 
				attachment_data->bone_index;
			//reset
			model_reset_pose(*attached_model, attachment_data->animated_pose);
			position = vec3_add(position, parent_bone->transformed_p);
			//transform the root bone based on the attached bone
			if(attached_model->bone_count)
			{
				model_bone *bone = attachment_data->animated_pose.bones + 0;
				bone->q = quaternion_mul(parent_bone->transformed_q, bone->q);
			}
			for(u32 b = 0; b < attachment_data->virtual_node_count; b++)
			{
				model_virtual_node *vnode = attachment_data->virtual_nodes + b;
				//ignore if not avadible
				if(vnode->child >= attachment_data->model->bone_count &&
				   vnode->parent >= model_data->model->bone_count)
				{
					continue;
				}

				model_bone *bone = attachment_data->animated_pose.bones + vnode->child;
				model_bone *parent_transformed = model_data->animated_pose.bones + vnode->parent;

				bone->q = quaternion_mul(parent_transformed->transformed_q, bone->q);
				bone->frame_key = parent_transformed->frame_key;
				bone->flip_h = parent_transformed->flip_h;
			}
			model_fill_bone_transformed_data(
					commands->gameRenderer,
					attachment_data->animated_pose,
					attached_model->bone_count,
					V2(0, -1));
			//for now, just render without any animation
			model_render(commands->gameRenderer,
					commands,
					*attached_model,
					attachment_data->animated_pose,
					position,
					view_direction);

		}
	}
}

static void
model_animate_and_render(render_commands *commands,
		model_render_data *model_data,
		vec3 position,
		vec2 view_direction,
		f32 dt)
{

	view_direction = model_get_view_direction(model_data->model, view_direction);

	model_pose animated_pose = model_data->animated_pose;
	if(model_data->current_animation_index < model_data->model->animation_count)
	{
		model_animation_timer *animation_timer = &model_data->timer;
		model_animation *animation = model_data->model->animations + model_data->current_animation_index;

		f32 current_time = animation_timer->dt_transcurred;
		model_animation_timer_run(animation, animation_timer, 1, dt);

		model_animation_animate_model_new(
				commands->gameRenderer,
				*model_data->model,
				model_data->animated_pose,
				animation,
				current_time,
				1,
				view_direction,
				dt);
	}
	else
	{
		//pose = model_data->model->pose;
		//Reset to bind model
		for(u32 b = 0; b < model_data->model->sprite_count; b++)
		{
			animated_pose.sprites[b] = model_data->model->sprites[b];
		}
		for(u32 b = 0; b < model_data->model->bone_count; b++)
		{
			animated_pose.bones[b] = model_data->model->bones[b];
		}
		model_fill_bone_transformed_data(commands->gameRenderer, animated_pose, model_data->model->bone_count, view_direction);
//		animated_pose.bones[0].transformed_q = quaternion_identity();
	}
	model_render(commands->gameRenderer,
			commands,
			*model_data->model,
			animated_pose,
			position,
			view_direction);

	//render attached models
#if 1
	for(u32 a = 0; a < model_data->attached_models_count; a++)
	{
		model_attachment_data *attachment_data = model_data->attach_data + a;
		model *attached_model = attachment_data->model;
		if(attached_model)
		{
			Assert(attachment_data->bone_index < model_data->model->bone_count);
			//get transformed bone
			model_bone *parent_bone = model_data->animated_pose.bones + 
				attachment_data->bone_index;
			//reset
			model_reset_pose(*attached_model, attachment_data->animated_pose);
			//transform the attached pose based on the bone
			for(u32 b = 0; b < attached_model->bone_count; b++)
			{
				//attached model bone
				model_bone *bone = attachment_data->animated_pose.bones + b;
				bone->q = quaternion_mul(parent_bone->transformed_q, bone->q);
				bone->p = vec3_add(bone->p, parent_bone->transformed_p);
			}
			model_fill_bone_transformed_data(
					commands->gameRenderer,
					attachment_data->animated_pose,
					attached_model->bone_count,
					V2(0, -1));
			//for now, just render without any animation
			model_render(commands->gameRenderer,
					commands,
					*attached_model,
					attachment_data->animated_pose,
					position,
					view_direction);

		}
	}
#endif

//	DrawAxes(render_commands, vec3_all(0),
//			 V3(1, 0, 0),
//			 V3(0, 1, 0),
//			 V3(0, 0, 1),
//			 255);

}

inline model
render_allocate_bind_model(memory_area *area, model *target_model)
{
	model bind_model = *target_model;
	bind_model.sprites = memory_area_push_array(
			area,
			model_sprite,
			target_model->sprite_count);
	bind_model.bones = memory_area_push_array(
			area,
			model_bone,
			target_model->bone_count);

	bind_model.uvs = memory_area_push_array(
			area,
			sprite_orientation,
			target_model->uvs_count);

	for(u32 s = 0; s < target_model->bone_count; s++)
	{
	    bind_model.bones[s] = target_model->bones[s];	
	}
	for(u32 s = 0; s < target_model->sprite_count; s++)
	{
	    bind_model.sprites[s] = target_model->sprites[s];	
	}

	return(bind_model);
}

static model_pose
render_allocate_pose(
		memory_area *area,
		model model)
{
	model_pose bind_pose = {0};
	bind_pose.sprites = memory_area_push_array(
			area,
			model_sprite,
			model.sprite_count);
	bind_pose.bones = memory_area_push_array(
			area,
			model_bone,
			model.bone_count);

	for(u32 s = 0; s < model.bone_count; s++)
	{
	    bind_pose.bones[s] = model.bones[s];	
	}
	for(u32 s = 0; s < model.sprite_count; s++)
	{
	    bind_pose.sprites[s] = model.sprites[s];	
	}

	return(bind_pose);
}


static void
render_debug_displace_camera(
		s_camera *camera,
		program_input *game_input_state,
		vec3 camera_x,
		vec3 camera_y,
		vec3 camera_z)
{
	u32 mouse_l_down = input_down(game_input_state->mouse_left);
	u32 mouse_r_down = input_down(game_input_state->mouse_right);
	//rotate model camera
	if(mouse_l_down)
	{
		f32 mouse_delta_x = game_input_state->mouse_x - game_input_state->mouse_x_last;
		f32 mouse_delta_y = game_input_state->mouse_y - game_input_state->mouse_y_last;

		camera->rotation.x += ((f32)mouse_delta_y * PI * 0.0005f); 
		camera->rotation.z += ((f32)mouse_delta_x * PI * 0.0005f); 
		camera->rotation.x = camera->rotation.x < 0 ? 2.0f : camera->rotation.x >= 2.0f ? 0 : camera->rotation.x;
		camera->rotation.z = camera->rotation.z < 0 ? 2.0f : camera->rotation.z >= 2.0f ? 0 : camera->rotation.z;
	}
	else if(mouse_r_down)
	{
		f32 mouse_delta_x = game_input_state->mouse_x - game_input_state->mouse_x_last;
		f32 mouse_delta_y = game_input_state->mouse_y - game_input_state->mouse_y_last;

		f32 paddingSpeed = 0.02f * 20 * 0.5f;
		paddingSpeed = (paddingSpeed > 0.2f) ? 
			0.2f : (paddingSpeed < 0.02f) ? 0.02f : paddingSpeed;

		vec3 add_xAxis = vec3_scale(camera_x, mouse_delta_x * paddingSpeed);
		vec3 add_yAxis = vec3_scale(camera_y, mouse_delta_y * paddingSpeed);

		camera->position.x -= add_xAxis.x - add_yAxis.x;
		camera->position.y -= add_xAxis.y - add_yAxis.y;
		camera->position.z -= add_xAxis.z - add_yAxis.z;

		//game_editor->model.cameraDistance = vec3_length(game_editor->model.camera_position);
	}

	i32 mouseWheelValue = game_input_state->mouse_wheel * 10;
	vec3 add_zAxis = vec3_scale(camera_z, (f32)mouseWheelValue);

	camera->position.x -= add_zAxis.x;
	camera->position.y -= add_zAxis.y;
	camera->position.z -= add_zAxis.z;

}

static model_render_data
model_allocate_render_data(
		memory_area *area,
		model *model)
{
	model_render_data result = {0};
	result.model = model;
	result.animated_pose = render_allocate_pose(area,
		*model);
	return(result);
}


inline void
model_animation_switch_animation_old(
		model_animations *animations, u32 new_index)
{
	if(new_index < animations->animation_count &&
			new_index != animations->current_animation_index)
	{
		u32 c = animations->current_animation_index;
		if(!animations->animations[c].keep_timer_on_transition)
		{
			animations->timer.frame_current = 0;
			animations->timer.dt_current = 0;
			animations->timer.dt_transcurred = 0;
		}
		animations->current_animation_index = new_index;
	}
}


static void
model_switch_animation(
		model_render_data *model_render,
		u32 new_index)
{
	model *model = model_render->model;
	model_animation *animations = model->animations;
	u32 animation_count = model->animation_count;
	model_animation_timer *timer = &model_render->timer;
	u32 old_index = model_render->current_animation_index;

	u32 index_choosen = old_index;
	if(new_index < animation_count && old_index != new_index)
	{
		u32 c = old_index;
		if(!animations[c].keep_timer_on_transition)
		{
			timer->frame_current = 0;
			timer->dt_current = 0;
			timer->dt_transcurred = 0;
		}
		index_choosen = new_index;
	}

	model_render->current_animation_index = index_choosen;
}

