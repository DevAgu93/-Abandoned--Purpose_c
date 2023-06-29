/*
   Most of these functions are used by the old world editor and the model editor.
   So maybe I should move them to their corresponding places once I clean this shit up.
*/

typedef struct{
	vec3 x;
	vec3 y;
	vec3 z;
}vertices_axes;

inline vec3
quad_get_normal(vec3 v0,
		        vec3 v1,
				vec3 v2,
				vec3 v3)
{
	vec3 x_axis = vec3_sub(v3, v0);

	vec3 y_axis = vec3_sub(v0, v2);

	vec3 normal = vec3_normalize(vec3_cross(x_axis, y_axis));
	return(normal);
}

inline mesh_points
quad_rotate_from_degrees(vec3 rotation_point,
		                 i32 rotation_x,
		                 i32 rotation_y,
		                 i32 rotation_z,
						 vec3 v0,
						 vec3 v1,
						 vec3 v2,
						 vec3 v3)
{

	//the rotation values should depend on how much I'm willing to rotate with each apply
	matrix3x3 rotation_apply_matrix = matrix3x3_rotation_degrees(
	 		                                          rotation_x,
	 		                                          rotation_y,
	 		                                          rotation_z
	 												  );

	vec3 tileOrigin =  vertices_get_mid_point(
	 										v0,
	 										v1,
	 										v2,
	 										v3);
	//distance from the center of the tile
	vec3 rotation_origin ={
	 	tileOrigin.x - rotation_point.x,
	 	tileOrigin.y - rotation_point.y,
	 	tileOrigin.z - rotation_point.z,
	};

	rotation_origin =  matrix3x3_v3_mul_rows(rotation_apply_matrix, rotation_origin);

	v0 = vec3_sub(v0, tileOrigin);
	v1 = vec3_sub(v1, tileOrigin);
	v2 = vec3_sub(v2, tileOrigin);
	v3 = vec3_sub(v3, tileOrigin);

	v0 = matrix3x3_v3_mul_rows(rotation_apply_matrix, v0);
	v1 = matrix3x3_v3_mul_rows(rotation_apply_matrix, v1);
	v2 = matrix3x3_v3_mul_rows(rotation_apply_matrix, v2);
	v3 = matrix3x3_v3_mul_rows(rotation_apply_matrix, v3);

	v0 = vec3_add(v0, rotation_point);
	v1 = vec3_add(v1, rotation_point);
	v2 = vec3_add(v2, rotation_point);
	v3 = vec3_add(v3, rotation_point);

	v0 = vec3_add(v0, rotation_origin);
	v1 = vec3_add(v1, rotation_origin);
	v2 = vec3_add(v2, rotation_origin);
	v3 = vec3_add(v3, rotation_origin);

	mesh_points rotated_points = {0};
	rotated_points.v0 = v0;
	rotated_points.v1 = v1;
	rotated_points.v2 = v2;
	rotated_points.v3 = v3;

	return(rotated_points);
}

inline model_mesh
quad_flip_diagonal(vec3 v0,
		           vec3 v1,
				   vec3 v2,
				   vec3 v3,
				   vec2 uv0,
				   vec2 uv1,
				   vec2 uv2,
				   vec2 uv3)
{


	vec3 v1_copy = v1;
	v1 = v2;
	v2 = v3;
	v3 = v0;
	v0 = v1_copy;

	vec2 uv0_copy = uv0;
	uv0 = uv1;
	uv1 = uv2;
	uv2 = uv3;
	uv3 = uv0_copy;

	model_mesh mesh = {0};

	mesh.v0 = v0;
	mesh.v1 = v1;
	mesh.v2 = v2;
	mesh.v3 = v3;

	mesh.uv0 = uv0;
	mesh.uv1 = uv1;
	mesh.uv2 = uv2;
	mesh.uv3 = uv3;

	return(mesh);
}


//
// general useful functions__
//

inline void
draw_crosshair_cursor(render_commands *render_commands,
		              vec3 p,
		              f32 alpha)
{
  vec4 xAxis_color = V4(0xff, 0xff, 0xff, alpha);
  vec4 yAxis_color = V4(0xff, 0x00, 0x00, alpha);
  vec4 zAxis_color = V4(0x00, 0x00, 0x00, alpha);
  // old colors
  //V4(255, 0, 0, alpha)
  //V4(0, 255, 0, alpha)
  //V4(0, 0, 0, alpha), 
  vec3 xAxis = {1, 0, 0};
  vec3 yAxis = {0, 1, 0};
  vec3 zAxis = {0, 0, 1};
  f32 cursorDistanceLimit = U16MAX;
  vec3 xAxis_scaled = vec3_f32_mul(xAxis, cursorDistanceLimit);
  vec3 yAxis_scaled = vec3_f32_mul(yAxis, cursorDistanceLimit);
  vec3 zAxis_scaled = vec3_f32_mul(zAxis, cursorDistanceLimit);

  vec3 p0 = vec3_sub(p, xAxis_scaled);
  vec3 p1 = vec3_add(p, xAxis_scaled);
  render_draw_line_up(render_commands, p0, p1, xAxis_color, 0.2f);

  p0 = vec3_sub(p, yAxis_scaled);
  p1 = vec3_add(p, yAxis_scaled);
  render_draw_line_up(render_commands, p0, p1, yAxis_color, 0.2f);

  p0 = vec3_sub(p, zAxis_scaled);
  p1 = vec3_add(p, zAxis_scaled);
  render_draw_line_up(render_commands, p0, p1, zAxis_color, 0.2f);

}

inline void
DrawAxes(render_commands *render_commands,
		 vec3 p,
		 vec3 xAxis,
		 vec3 yAxis,
		 vec3 zAxis,
		 f32 alpha)
{
  vec4 xAxis_color = V4(0xff, 0x00, 0x00, alpha);
  vec4 yAxis_color = V4(0xff, 0xff, 0xff, alpha);
  vec4 zAxis_color = V4(0x00, 0x00, 0x00, alpha);
  // old colors
  //V4(255, 0, 0, alpha)
  //V4(0, 255, 0, alpha)
  //V4(0, 0, 0, alpha), 
  render_draw_line_up(render_commands, p, vec3_add(vec3_f32_mul(xAxis, 20), p), xAxis_color, 0.5f);
  render_draw_line_up(render_commands, p, vec3_add(vec3_f32_mul(yAxis, 20), p), yAxis_color, 0.5f);
  render_draw_line_up(render_commands, p, vec3_add(vec3_f32_mul(zAxis, 20), p), zAxis_color, 0.5f);

}

inline void
editor_highlight_selected_mesh(render_commands *commands,
							   editor_mesh_selection *mesh_selection,
							   vec3 v0,
							   vec3 v1,
							   vec3 v2,
							   vec3 v3,
							   u32 hide,
							   f32 vertices_cube_size)
{
		f32 border_line_thickness = 0.1f;

         vec4 selected_meshesColor = {0xff, 0x00, 0x00, 60};
	   	 vec4 verticesColor = {0xff, 0xff, 0xff, 0xff};
	   	 vec4 verticesSelectedColor = {0x00, 0x00, 0xff, 0xff};
	   	 vec4 verticesHotColor = {0xff, 0x00, 0x00, 0xff};
	     vec4 nextLineColor = {0};
	     //For more visibility 
	     if(hide)
	     {
	          verticesColor.w         = 20;
	          verticesSelectedColor.w = 20;
	          verticesHotColor.w      = 20;
	          selected_meshesColor.w    = 20;
	     }

	   	render_vertices_colored(commands,
	   				            v0,
	   				            v1,
	   				            v2,
	   				            v3,
	   							selected_meshesColor);

          render_draw_line_up(commands,
	   		               v0,
	   					   v3,
	   					   verticesColor,
	   					   border_line_thickness);

          render_draw_line_up(commands,
	   		               v0,
	   					   v1,
	   					   verticesColor,
	   					   border_line_thickness);

          render_draw_line_up(commands,
	   		               v1,
	   					   v2,
	   					   verticesColor,
	   					   border_line_thickness);

          render_draw_line_up(commands,
	   		               v3,
	   					   v2,
	   					   verticesColor,
	   					   border_line_thickness);

	      vec4 nextVertexColor = mesh_selection->v0_selected ?
	      	                   verticesSelectedColor : 
	      					   verticesColor;
	      render_draw_cube(commands, v0, vec3_all(vertices_cube_size), nextVertexColor);

	      nextVertexColor = mesh_selection->v1_selected ? verticesSelectedColor : verticesColor;
	      render_draw_cube(commands, v1, vec3_all(vertices_cube_size), nextVertexColor);

	      nextVertexColor = mesh_selection->v2_selected ? verticesSelectedColor : verticesColor;
	      render_draw_cube(commands, v2, vec3_all(vertices_cube_size), nextVertexColor);

	      nextVertexColor = mesh_selection->v3_selected ? verticesSelectedColor : verticesColor;
	      render_draw_cube(commands, v3, vec3_all(vertices_cube_size), nextVertexColor);
}


inline asset_file_info *
editor_get_disk_file_info_from_id(s_game_editor *game_editor, u32 asset_id)
{
	u32 packed_assets_count = game_editor->data_files_count;
	asset_file_info *packed_assets = game_editor->data_files;
	asset_file_info *result     = 0;
	for(u32 a = 0; a < packed_assets_count; a++)
	{
		if(packed_assets[a].data.id == asset_id)
		{
			result = packed_assets + a;
			break;
		}
	}
	return(result);
}



//
// cursor functions__
//
inline mesh_points
editor_get_sprite_billboard_points(game_renderer *renderer,
								   model_sprite *current_mesh
		                           )
{
	vec3 v0 = current_mesh->v0;
	vec3 v1 = current_mesh->v1;
	vec3 v2 = current_mesh->v2;
	vec3 v3 = current_mesh->v3;

	mesh_points points = {0};
	points.v0 = v0;
	points.v1 = v1;
	points.v2 = v2;
	points.v3 = v3;

    if(current_mesh->type == model_sprite_billboard)
	{
	    //get correct vertices points based on the camera facing.

	    vec3 pivot_offset = {
	    	current_mesh->pivotX,
	    	current_mesh->pivotY,
	    	current_mesh->pivotZ,
	    };
	    if(current_mesh->face_axis == billboard_face_x)
	    {
	    	//points = get_mesh_billboard_points_x(renderer, v0, v1, v2, v3, pivot_offset);
	    }
	    else
	    {

	    //	points = get_mesh_billboard_points(renderer, v0, v1, v2, v3, pivot_offset);
	    }
	}

	return(points);
}


static inline void
editor_rotate_quaternion_by_mouse_delta(
		editor_input *input,
		quaternion *rotation_q,
		vec3 rotation_axis)
{
	f32 mouse_delta_x = input->mouse_clip_x_last - input->mouse_clip_x;

	quaternion rotation_quaternion = 
		quaternion_rotated_at(
				rotation_axis.x,
				rotation_axis.y,
				rotation_axis.z,
				mouse_delta_x * 0.001f * PI);
	*rotation_q = quaternion_mul(
			*rotation_q, rotation_quaternion);

}

//returns the hit axes index and fills the hit distance
static inline u32
editor_render_and_rotate_quaternion(
		render_commands *commands,
		s_ray ray_od,
		editor_input *input,
		vec3 gizmo_position,
		quaternion *rotation_q,
		u32 *selected_axis,
		u32 hide,
		u32 process_input,
		u32 allow_selection,
		f32 *ray_hit_distance)
{
	f32 gizmo_cube_size  = 2.0f;
	f32 unselected_alpha = 180.0f;
	vec4 circle_x_color  = {255, 0, 0, unselected_alpha};
	vec4 circle_y_color  = {255, 255, 255, unselected_alpha};
	vec4 circle_z_color  = {0, 0, 0, unselected_alpha};
	circle_x_color.w = *selected_axis == 2 ? 200.0f : 100.0f;
	circle_y_color.w = *selected_axis == 1 ? 200.0f : 100.0f;
	circle_z_color.w = *selected_axis == 0 ? 200.0f : 100.0f;

	enum axis_to_select{
		axis_z = 0,
		axis_y = 1,
		axis_x = 2
	};

	vec3 x_axis = quaternion_v3_mul_foward_inverse(*rotation_q, V3(1, 0, 0));
	vec3 y_axis = quaternion_v3_mul_foward_inverse(*rotation_q, V3(0, 1, 0));
	vec3 z_axis = quaternion_v3_mul_foward_inverse(*rotation_q, V3(0, 0, 1));
	//hide this while moving the camera
	if(hide)
	{
		circle_x_color.w        = 20;
		circle_y_color.w        = 20;
		circle_z_color.w        = 20;
	}
	f32 circle_thickness = 0.6f;
	u32 hit_axes_index = 3;

	if(*selected_axis >= 3)
	{
		ray_hit_result x_hit = ray_open_circle_got_hit(
				ray_od,
				gizmo_position,
				x_axis,
				8,
				circle_thickness
				);
		ray_hit_result y_hit = ray_open_circle_got_hit(
				ray_od,
				gizmo_position,
				y_axis,
				8,
				circle_thickness
				);
		ray_hit_result z_hit = ray_open_circle_got_hit(
				ray_od,
				gizmo_position,
				z_axis,
				8,
				circle_thickness
				);
		f32 hit_distance = 10000;

		if(x_hit.hit)
		{
			circle_x_color.w = 200;
			hit_distance = x_hit.distance;
			hit_axes_index = 2;
		}
		else if(y_hit.hit)
		{
			circle_y_color.w = 200;
			hit_distance = y_hit.distance;
			hit_axes_index = 1;
		}
		else if(z_hit.hit)
		{
			circle_z_color.w = 200;
			hit_distance = z_hit.distance;
			hit_axes_index = 0;
		}
		if(ray_hit_distance)
		{
			*ray_hit_distance = hit_distance;
		}
	}


	//z_axis_dircle
	render_Circle(commands,
			gizmo_position, 
			x_axis,
			y_axis,
			8,
			circle_thickness,
			circle_z_color);

	//x_axis_dircle
	render_Circle(commands,
			gizmo_position, 
			y_axis,
			z_axis,
			8,
			circle_thickness,
			circle_x_color);

	//y_axis_dircle
	render_Circle(commands,
			gizmo_position, 
			x_axis,
			z_axis,
			8,
			circle_thickness,
			circle_y_color);
	if(hit_axes_index < 3)
	{
		*selected_axis = hit_axes_index;
	}

	if(*selected_axis < 3 && process_input && input_down(input->mouse_left))
	{
		f32 mouse_delta_x = input->mouse_clip_x_last - input->mouse_clip_x;
		f32 mouse_delta_y = input->mouse_clip_y_last - input->mouse_clip_y;
		switch(*selected_axis)
		{
			//z
			case 0:
				{
					quaternion q0 = quaternion_rotated_at(
							0, 0, 1, mouse_delta_x * 0.001f * PI);
					*rotation_q = quaternion_mul(
							*rotation_q, q0);
				}break;
				//y
			case 1:
				{
					quaternion q0 = quaternion_rotated_at(
							0, 1, 0, mouse_delta_x * 0.001f * PI);
					*rotation_q = quaternion_mul(
							*rotation_q, q0);
				}break;
				//x
			case 2:
				{
					quaternion q0 = quaternion_rotated_at(
							1, 0, 0, mouse_delta_x * 0.001f * PI);
					*rotation_q = quaternion_mul(
							*rotation_q, q0);
				}break;
		}

	}
	else
	{
		*selected_axis = 4;
	}

	return(hit_axes_index);
}

static void
editor_draw_background(
		render_commands *commands)
{
	//editor background
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
}

#define e_array_remove_and_shift(memory, type, count, index) \
	e_memory_remove_and_shift(memory, sizeof(type), (index), (count))
static u32 
e_memory_remove_and_shift(
		void *memory,
		u32 size,
		u32 index,
		u32 count)
{
	u8 *memory8 = (u8 *)memory;
	if(index < count)
	{
		for(u32 h = index; h < count - 1; h++)
		{
			u32 index0 = h * size;
			u32 index1 = (h + 1) * size;
			memory_copy(memory8 + index1, memory8 + index0, size);
		}
		count--;
	}
	return(count);
}

static b32 
editor_array_remove_and_shift(void *e_array, u32 index)
{
	editor_array *array = e_array;
	u32 count_before = array->count;
	array->count = e_memory_remove_and_shift(
			array->base, array->size_of_type, array->count, index);

	return(array->count != count_before);
}

#define editor_array_add(array, type) (type *)_editor_array_add(array)
#define editor_array_add_clear(array, type) (type *)_editor_array_add_clear(array)
static void *
_editor_array_add(editor_array *array)
{
	u8 *result = 0;
	if(array->count < array->max)
	{
		result = array->base + (array->size_of_type * array->count);
		array->count++;
	}
	return(result);
}

static void *
_editor_array_add_clear(editor_array *array)
{
	u8 *result = 0;
	if(array->count < array->max)
	{
		result = array->base + (array->size_of_type * array->count);
		array->count++;

		memory_clear(result, array->size_of_type);
	}
	return(result);
}








// ui helpers

static void
editor_ui_input_text_name(
		game_ui *ui,
		editor_name_chunks *names,
		u32 index,
		u8 *label)
{
	u8 *name = names->chunks[index].name;
	ui_input_text(ui, 0, name, names->length - 1, label);
}
