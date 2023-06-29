typedef struct{
	u32 hit;
    f32 hit_distance;
    editor_mesh_edge hovering_edge;
}editor_mouse_mesh_selection;


inline f32
scalar_advance_by_no_minus_zero(f32 s, i32 number)
{

	s -= (s < -1.0001f) * number;
	s = (f32)((i32)(s / number) * number);

	return(s);

}

inline f32
scalar_advance_by(f32 s, i32 number)
{

	s = (f32)((i32)(s / number) * number);

	return(s);

}

inline void
editor_cursor_move_selected_mesh_with_gizmo(
		editor_cursor_memory *cursor_memory,
		editor_cursor *cursor,
		model_vertices *m_vertices)
{
	//look for the closest hot vertex
	editor_hot_vertices_info *hv = cursor_memory->hot_vertices;
	u32 closest_vertex_index     = cursor_memory->hot_vertices_count;
	f32 h_distance_to_camera = 1e6;
	for(u32 h = 0;
			h < cursor_memory->hot_vertices_count;
			h++)
	{
		editor_hot_vertices_info *current_h = cursor_memory->hot_vertices + h;
		if(current_h->mesh_index != cursor->vertex_to_move_with_gizmo)
		{
			if(current_h->distance_to_ray < h_distance_to_camera)
			{
				h_distance_to_camera = current_h->distance_to_ray;
				closest_vertex_index = h;
			}

		}
	}
	if(closest_vertex_index < cursor_memory->hot_vertices_count)
	{
		editor_hot_vertices_info *closest_h = cursor_memory->hot_vertices + closest_vertex_index;
		u32 h_sv = closest_h->selection_value;

		model_vertices *closest_hot_mesh = cursor_memory->meshes[closest_h->mesh_index].vertices;
		vec3 v0 = closest_hot_mesh->v0;
		vec3 v1 = closest_hot_mesh->v1;
		vec3 v2 = closest_hot_mesh->v2;
		vec3 v3 = closest_hot_mesh->v3;

		vec3 final_position = cursor->gizmo_position;

		if(h_sv & tile_edge_v0)
		{
			final_position = v0;
		}
		else if(h_sv & tile_edge_v1)
		{
			final_position = v1;
		}
		else if(h_sv & tile_edge_v2)
		{
			final_position = v2;
		}
		else if(h_sv & tile_edge_v3)
		{
			final_position = v3;
		}
		else if(h_sv & tile_edge_u)
		{
			final_position = vec3_add(v1, vec3_scale(vec3_sub(v2, v1), 0.5f));
		}
		else if(h_sv & tile_edge_d)
		{
			final_position = vec3_add(v0, vec3_scale(vec3_sub(v3, v0), 0.5f));
		}
		else if(h_sv & tile_edge_l)
		{
			final_position = vec3_add(v0, vec3_scale(vec3_sub(v1, v0), 0.5f));
		}
		else if(h_sv & tile_edge_r)
		{
			final_position = vec3_add(v3, vec3_scale(vec3_sub(v2, v3), 0.5f));
		}
		else
		{
			final_position = vertices_get_mid_point(v0, v1, v2, v3);
		}
		cursor->gizmo_position = final_position;

		//read the mesh's selection data
		editor_mesh_selection *mesh_to_move_selection = cursor_memory->selected_meshes + cursor->vertex_to_move_with_gizmo;
		vec3 gizmo_delta = vec3_sub(cursor->gizmo_position, cursor->gizmo_position_last);
		//selected vertices to move
		//move selected vertices by delta
		if(mesh_to_move_selection->v0_selected)
		{
			m_vertices->v0 = vec3_add(m_vertices->v0, gizmo_delta);
		}
		if(mesh_to_move_selection->v1_selected)
		{
			m_vertices->v1 = vec3_add(m_vertices->v1, gizmo_delta);
		}
		if(mesh_to_move_selection->v2_selected)
		{
			m_vertices->v2 = vec3_add(m_vertices->v2, gizmo_delta);
		}
		if(mesh_to_move_selection->v3_selected)
		{
			m_vertices->v3 = vec3_add(m_vertices->v3, gizmo_delta);
		}

	}
}
inline void
editor_cursor_memory_move_selected_mesh(
		editor_cursor_memory *cursor_memory,
		u32 selection_index,
		vec3 delta)
{
	if(selection_index < cursor_memory->selected_meshes_count)
	{
        editor_mesh_selection *s_data = cursor_memory->selected_meshes + selection_index;
		u32 mesh_index = s_data->index; 

		model_vertices *v = cursor_memory->meshes[mesh_index].vertices;

		if(s_data->v0_selected)
		{
			v->v0 = vec3_add(v->v0, delta);
		}
		if(s_data->v1_selected)
		{
			v->v1 = vec3_add(v->v1, delta);
		}
		if(s_data->v2_selected)
		{
			v->v2 = vec3_add(v->v2, delta);
		}
		if(s_data->v3_selected)
		{
			v->v3 = vec3_add(v->v3, delta);
		}

	}
}

inline void
editor_cursor_memory_select_amount(editor_cursor_memory *cursor_memory, u32 amount)
{
	u32 a = 0;
	u32 total_amount = MIN(amount, cursor_memory->selected_meshes_max);
	cursor_memory->selected_meshes_count = amount;
	while(a < total_amount)
	{
		editor_mesh_selection *mesh_selection = cursor_memory->selected_meshes + a;
		mesh_selection->index = a;
		mesh_selection->v0_selected = 1;
		mesh_selection->v1_selected = 1;
		mesh_selection->v2_selected = 1;
		mesh_selection->v3_selected = 1;

		a++;
	}
}

inline u32
editor_cursor_memory_add_mesh(
		editor_cursor_memory *cursor_memory,
		model_vertices *new_vertices)
{
	u32 success = 0;
	if(cursor_memory->meshes_count < cursor_memory->meshes_max)
	{
		success = 1;
		cursor_memory->meshes[cursor_memory->meshes_count].vertices = new_vertices;
	}
	cursor_memory->meshes_count++;
	return(success);
}

inline void
editor_cursor_deselect_hot_vertices(editor_cursor_memory *cursor_memory)
{
	u32 h = 0;
	for(u32 h = 0;
			h < cursor_memory->hot_vertices_count;
			h++)
	{
		editor_hot_vertices_info *hot_vertices_current = cursor_memory->hot_vertices + h;

		u32 mesh_selection_index = hot_vertices_current->selection_index;
		editor_mesh_selection *selection_current = cursor_memory->selected_meshes + mesh_selection_index;

		//deselect all hot vertices
		selection_current->v0_selected = selection_current->v0_selected && !(hot_vertices_current->selection_value & tile_edge_v0);
		selection_current->v1_selected = selection_current->v1_selected && !(hot_vertices_current->selection_value & tile_edge_v1);
		selection_current->v2_selected = selection_current->v2_selected && !(hot_vertices_current->selection_value & tile_edge_v2);
		selection_current->v3_selected = selection_current->v3_selected && !(hot_vertices_current->selection_value & tile_edge_v3);
	}
}

inline void
editor_cursor_select_hot_vertices(editor_cursor_memory *cursor_memory)
{
	u32 h = 0;
	for(u32 h = 0;
			h < cursor_memory->hot_vertices_count;
			h++)
	{
		editor_hot_vertices_info *hot_vertices_current = cursor_memory->hot_vertices + h;

		u32 mesh_selection_index = hot_vertices_current->selection_index;
		editor_mesh_selection *selection_current = cursor_memory->selected_meshes + mesh_selection_index;

		//deselect all hot vertices
		selection_current->v0_selected = selection_current->v0_selected || (hot_vertices_current->selection_value & tile_edge_v0);
		selection_current->v1_selected = selection_current->v1_selected || (hot_vertices_current->selection_value & tile_edge_v1);
		selection_current->v2_selected = selection_current->v2_selected || (hot_vertices_current->selection_value & tile_edge_v2);
		selection_current->v3_selected = selection_current->v3_selected || (hot_vertices_current->selection_value & tile_edge_v3);
	}
}


inline editor_cursor_memory
editor_cursor_allocate_memory(memory_area *area)
{
	editor_cursor_memory result = {0};
	result.meshes_max = 10000;

	return(result);
}

inline void
editor_cursor_memory_add_selection(
		editor_cursor_memory *cursor_memory,
		u32 mesh_index)
{

	editor_mesh_selection *new_selection = cursor_memory->selected_meshes + cursor_memory->selected_meshes_count;
    new_selection->index = mesh_index;

	//select all vertices by default
	new_selection->v0_selected = 1;
	new_selection->v1_selected = 1;
	new_selection->v2_selected = 1;
	new_selection->v3_selected = 1;
	cursor_memory->selected_meshes_count++;

//for later use
//	Assert(mesh_index < cursor_memory->meshes_count);
}
inline void
editor_cursor_memory_deselect(editor_cursor_memory *cursor_memory, u32 selection_index)
{
	u32 s = selection_index;
	while(s < (u32)(cursor_memory->selected_meshes_count - 1))
	{
	    cursor_memory->selected_meshes[s] = cursor_memory->selected_meshes[s + 1];
	    s++;
	}
	cursor_memory->selected_meshes_count--;
}

inline void
editor_cursor_memory_initialize_frame(memory_area *frame_area, editor_cursor_memory *cursor_memory)
{
	if(!cursor_memory->meshes_max)
	{
		cursor_memory->meshes_max = 100;
	}
	if(!cursor_memory->selected_meshes_max)
	{
	    cursor_memory->selected_meshes_max = 100;
	}
	if(!cursor_memory->hot_vertices_max)
	{
	    cursor_memory->hot_vertices_max    = 10;
	}

	u16 selected_meshes_count = cursor_memory->selected_meshes_count;
	u16 selected_meshes_max   = cursor_memory->selected_meshes_max;
	u16 hot_vertices_count    = cursor_memory->hot_vertices_count;
	u16 hot_vertices_max      = cursor_memory->hot_vertices_max;
	u16 meshes_max            = cursor_memory->meshes_max;

	u32 meshes_overflow        = cursor_memory->meshes_count >= cursor_memory->meshes_max - 1;
	u32 selected_mesh_overflow = selected_meshes_count >= selected_meshes_max - 1;
	u32 hot_vertices_overflow  = hot_vertices_count >= hot_vertices_max;

	editor_hot_vertices_info *hot_vertices_array = cursor_memory->hot_vertices;

	if(meshes_overflow)
	{
		meshes_max = cursor_memory->meshes_count + 5;
	}

	cursor_memory->meshes = (editor_mesh *)memory_area_push_array(
			frame_area,
			memory_size,
			cursor_memory->meshes_max);
	//expand meshes array before the selections
	if(meshes_max > cursor_memory->meshes_max)
	{
		//shift the memory area to not lose information
		//on the selections array
		memory_area_shift_size_r(
				frame_area,
				sizeof(memory_size) * 5);
		cursor_memory->meshes_max = meshes_max;
		//push the new size
		memory_area_clear_and_push(
				frame_area, sizeof(memory_size) * 5);
	}

	cursor_memory->selected_meshes= memory_area_push_array(frame_area, editor_mesh_selection, selected_meshes_max);
	editor_mesh_selection *selected_meshes_array = cursor_memory->selected_meshes;
	if(selected_mesh_overflow)
	{
	    editor_mesh_selection zeroSelection = {0};
		//expand the array and initialize the selections to zero
	    for(u32 t = 0;
	   		 t < 4;
	   		 t++)
	    {
	        selected_meshes_array[selected_meshes_max + t] = zeroSelection;
	    }
	    selected_meshes_max += 4;
	    cursor_memory->selected_meshes_max = selected_meshes_max;
		//shift array for hot vertices ??
	}


	if(hot_vertices_overflow)
	{
	    hot_vertices_max = hot_vertices_count;
	}
	cursor_memory->hot_vertices = memory_area_push_array(frame_area, editor_hot_vertices_info, hot_vertices_max);

    //cursor_memory->meshes_count          = 0;
	cursor_memory->hot_vertices_count = 0;
	//cursor_memory->selected_meshes_count = 0;

}

inline vec3
editor_cursor_move_at_axes(editor_cursor *cursor,
		                   vec3 rayOrigin,
		                   vec3 rayDir,
						   vec3 pointFrom)
{
	
	vec3 paintCursor_uAxis    = cursor->uAxis;
	vec3 paintCursor_rAxis    = cursor->rAxis;
	vec3 paintCursor_normal   = vec3_cross(paintCursor_rAxis,
				                           paintCursor_uAxis);
	f32 paintCursorTileSize   = cursor->tile_displacement;
	//position against cursor grid
	ray_casted_info rayResult = cast_ray_at_point(rayOrigin,
				                                 rayDir,
												 pointFrom);
	vec3 mouseToGrid = rayResult.ray_on_plane;

	vec3 mouse_cursor_delta = vec3_sub(mouseToGrid, pointFrom);

	vec3 move_delta_position = {0};

	move_delta_position.x = vec3_inner(mouse_cursor_delta, paintCursor_rAxis);
	move_delta_position.y = vec3_inner(mouse_cursor_delta, paintCursor_uAxis);
	move_delta_position.x = scalar_advance_by(move_delta_position.x, (i32)paintCursorTileSize);
	move_delta_position.y = scalar_advance_by(move_delta_position.y, (i32)paintCursorTileSize); 

	matrix3x3 move_delta_rotation = matrix3x3_from_vec_col(paintCursor_rAxis,
		 		                                           paintCursor_uAxis,
		 												   paintCursor_normal);

    //move_delta_position2 = matrix3x3_v3_mul_cols(move_delta_position, move_delta_positionRotation);
    move_delta_position = matrix3x3_v3_mul_rows(move_delta_rotation, move_delta_position);

	return(move_delta_position);
}

inline void
editor_cursor_move_gizmo_with_mouse_at_axes(
		editor_cursor *cursor,
		vec3 ray_origin,
		vec3 ray_dir)
{
	vec3 move_delta_position = editor_cursor_move_at_axes(cursor,
		                                                  ray_origin,
														  ray_dir,
														  cursor->gizmo_position);
	cursor->gizmo_position_last = cursor->gizmo_position;
	cursor->gizmo_position = vec3_add(move_delta_position, cursor->gizmo_position);
}

inline editor_mouse_mesh_selection
editor_cursor_update_selected_mesh(vec3 rayOrigin,
		                           vec3 rayDir,
						           vec3 v0,
						           vec3 v1,
						           vec3 v2,
						           vec3 v3,
						           u32 selection_index,
						           f32 ray_hit_distance,
						           editor_cursor_memory *cursor_memory,
								   f32 vertices_cube_size)
{
		editor_mesh_selection *selection_data = cursor_memory->selected_meshes + selection_index;
		u32 tile_index                        = selection_data->index;


		raycast_baycentric_result triangle_result_0 = ray_triangle_get_uvw(rayOrigin,
		                                                                   rayDir,
			                                                               v0,
			                                                               v1,
			                                                               v2);
		raycast_baycentric_result triangle_result_1 = ray_triangle_get_uvw(rayOrigin,
		                                                                   rayDir,
			                                                               v0,
			                                                               v2,
			                                                               v3);
        vec3 uvw0 = triangle_result_0.uvw;
        vec3 uvw1 = triangle_result_1.uvw;

		//since a quad is divided by two triangles, detecting ray vs
		//both triangles is needed
		u32 inside0 = uvw_is_inside(uvw0);
		u32 inside1 = uvw_is_inside(uvw1);
      

		f32 vertex_radius = 2 * 2;
		f32 vertex_cube_sizes = 4;

		u32 isClosestMesh = 0;


		ray_cube_result ray_result_v0 = ray_cube_get_result(rayOrigin, rayDir, v0, vec3_all(vertices_cube_size));
		ray_cube_result ray_result_v1 = ray_cube_get_result(rayOrigin, rayDir, v1, vec3_all(vertices_cube_size));
		ray_cube_result ray_result_v2 = ray_cube_get_result(rayOrigin, rayDir, v2, vec3_all(vertices_cube_size));
		ray_cube_result ray_result_v3 = ray_cube_get_result(rayOrigin, rayDir, v3, vec3_all(vertices_cube_size));

		u8 inside_v0 = ray_result_v0.hit_value != 0;//ray_cube_intersects(rayOrigin, rayDir, current_tile->v0, vec3_all(vertex_cube_sizes));
		u8 inside_v1 = ray_result_v1.hit_value != 0;//ray_cube_intersects(rayOrigin, rayDir, current_tile->v1, vec3_all(vertex_cube_sizes));
		u8 inside_v2 = ray_result_v2.hit_value != 0;//ray_cube_intersects(rayOrigin, rayDir, current_tile->v2, vec3_all(vertex_cube_sizes));
		u8 inside_v3 = ray_result_v3.hit_value != 0;//ray_cube_intersects(rayOrigin, rayDir, current_tile->v3, vec3_all(vertex_cube_sizes));

		f32 distance_to_vertex    = ray_hit_distance;
		editor_mesh_edge hot_edge = 0;
		u32 hot_vertices_selection_value = 0;

		if(inside_v0)
		{
			distance_to_vertex = ray_result_v0.t_min;
			hot_edge           = tile_edge_v0;
			hot_vertices_selection_value |= tile_edge_v0;
		}
		else if(inside_v1)
		{
			distance_to_vertex = ray_result_v1.t_min;
			hot_edge           = tile_edge_v1;
			hot_vertices_selection_value |= tile_edge_v1;
		}
		else if(inside_v2)
		{
			distance_to_vertex = ray_result_v2.t_min;
			hot_edge           = tile_edge_v2;
			hot_vertices_selection_value |= tile_edge_v2;
		}
		else if(inside_v3)
		{
			distance_to_vertex = ray_result_v3.t_min;
			hot_edge           = tile_edge_v3;
			hot_vertices_selection_value |= tile_edge_v3;
		}

		//add to hot vertices array
		
		//check if got the closest vertex
		isClosestMesh = distance_to_vertex < ray_hit_distance;
      
		//if it's fully inside the quad
        if(inside0 || inside1)
        {
			f32 triangle_distance_0 = triangle_result_0.distance;
			f32 triangle_distance_1 = triangle_result_1.distance;
			u32 insideTile = 0;

			if(inside0 && triangle_distance_0 < distance_to_vertex)
			{
				insideTile    = 1;
                distance_to_vertex = triangle_distance_0;

			}
			else if(inside1 && triangle_distance_1 < distance_to_vertex)
			{
				insideTile    = 1;
                distance_to_vertex = triangle_distance_1;
			}
            
            if(insideTile)
            {
				isClosestMesh = 1;


				f32 edge_range = 0.05f;
				f32 vertex_range = 0.10f;
				if(inside0)
				{
				    if(uvw0.z <= edge_range)
				    {
				    	hot_edge = tile_edge_u;
						hot_vertices_selection_value |= tile_edge_u;
				    }
				    else if(uvw0.x <= edge_range)
				    {
				    	hot_edge = tile_edge_l;
						hot_vertices_selection_value |= tile_edge_l;
				    }
				}

				if(inside1)
				{
				    if(uvw1.y <= edge_range)
				    {
				    	hot_edge = tile_edge_d;
						hot_vertices_selection_value |= tile_edge_d;
				    }
				    else if(uvw1.z <= edge_range)
				    {
				    	hot_edge = tile_edge_r;
						hot_vertices_selection_value |= tile_edge_r;
				    }
				}
            }
        }

		u32 hotTilesOverflow = cursor_memory->hot_vertices_count >= cursor_memory->hot_vertices_max;
		if(hot_vertices_selection_value)
		{
			if(!hotTilesOverflow)
			{
				editor_hot_vertices_info *mesh_hot_vertices = cursor_memory->hot_vertices + cursor_memory->hot_vertices_count;

				mesh_hot_vertices->selection_index = selection_index;
				mesh_hot_vertices->mesh_index      = tile_index;
				//mesh_hot_vertices->v0_selected     = inside_v0 || (hot_edge == tile_edge_u || hot_edge == tile_edge_l);
				//mesh_hot_vertices->v1_selected     = inside_v1 || (hot_edge == tile_edge_u || hot_edge == tile_edge_r);
				//mesh_hot_vertices->v2_selected     = inside_v2 || (hot_edge == tile_edge_d || hot_edge == tile_edge_r);
				//mesh_hot_vertices->v3_selected     = inside_v3 || (hot_edge == tile_edge_d || hot_edge == tile_edge_l);
				mesh_hot_vertices->selection_value = hot_vertices_selection_value;
				mesh_hot_vertices->distance_to_ray = distance_to_vertex;
			}
			cursor_memory->hot_vertices_count++;
		}

        editor_mouse_mesh_selection result = {0};

		result.hit        = isClosestMesh;
		result.hovering_edge = hot_edge;
		result.hit_distance  = distance_to_vertex;

		return(result);
}

static void
editor_cursor_set_axes_by_camera(game_renderer *gameRenderer, editor_cursor *cursor)
{
		 //
		 // up and right axis
		 // _crosshair axes based on camera 

		 f32 camera_z_x = gameRenderer->camera_z.x;
		 f32 camera_z_y = gameRenderer->camera_z.y;
		 f32 camera_z_z = gameRenderer->camera_z.z;

		 f32 camera_x_x = gameRenderer->camera_x.x;
		 f32 camera_x_y = gameRenderer->camera_x.y;
		 f32 camera_x_z = gameRenderer->camera_x.z;

		 f32 camera_y_x = gameRenderer->camera_y.x;
		 f32 camera_y_y = gameRenderer->camera_y.y;
		 f32 camera_y_z = gameRenderer->camera_y.z;

		 f32 camera_z_x_sq = camera_z_x * camera_z_x;
		 f32 camera_z_y_sq = camera_z_y * camera_z_y;
		 f32 camera_z_z_sq = camera_z_z * camera_z_z;


		 u32 to_x = (camera_z_x_sq > camera_z_y_sq) &&
			        (camera_z_x_sq > camera_z_z_sq);

		 u32 to_y = (camera_z_y_sq > camera_z_x_sq) &&
			        (camera_z_y_sq > camera_z_z_sq);

		 u32 to_z = (camera_z_z_sq > camera_z_x_sq) &&
			        (camera_z_z_sq > camera_z_y_sq);

		 if(to_x)
		 {
		     cursor->uAxis = V3(0, 0, 1);
			 i32 x_dir = SIGN(camera_x_y);
		     cursor->rAxis = V3(0, (f32)x_dir, 0);
		 }
		 else if(to_y)
		 {
		     cursor->uAxis = V3(0, 0, 1);
			 i32 x_dir = SIGN(camera_x_x);
		     cursor->rAxis = V3((f32)x_dir, 0, 0);
		 }
		 else if(to_z)
		 {

		     f32 camera_x_x_sq = camera_x_x * camera_x_x;
		     f32 camera_y_x_sq = camera_y_x * camera_y_x;

			 if(camera_x_x_sq > camera_y_x_sq)
			 {
				 i32 x_dir = SIGN(camera_x_x);
				 i32 y_dir = SIGN(camera_y_y);
				 cursor->rAxis = V3((f32)x_dir, 0, 0);
				 cursor->uAxis = V3(0, (f32)y_dir, 0);
			 }
			 else
			 {
				 i32 x_dir = SIGN(camera_x_y);
				 i32 y_dir = SIGN(camera_y_x);
				 cursor->rAxis = V3(0, (f32)x_dir, 0);
				 cursor->uAxis = V3((f32)y_dir, 0, 0);
			 }
		 }
		 else
		 {
		     cursor->uAxis = V3(0, 1, 0);
		     cursor->rAxis = V3(1, 0, 0);
		 }
}

inline u32 
editor_cursor_input_movement_by_camera(game_renderer *gameRenderer,
		                               editor_input *editor_input,
									   editor_cursor *cursor,
									   f32 dt)
{

	u32 moved = 0;

	u8 down_u = input_down(editor_input->w);
	u8 down_d = input_down(editor_input->s);
	u8 down_l = input_down(editor_input->a);
	u8 down_r = input_down(editor_input->d);

	u8 pressed_u = input_pressed(editor_input->w);
	u8 pressed_d = input_pressed(editor_input->s);
	u8 pressed_l = input_pressed(editor_input->a);
	u8 pressed_r = input_pressed(editor_input->d);
	f32 tile_displacement = cursor->tile_displacement;


	u32 process_movement = cursor->move_down_timer == 0;
	if(down_u || down_d || down_r || down_l)
	{
		cursor->move_down_timer += dt;
	}
	else
	{
		cursor->move_down_timer = 0;
	}

	
	if(cursor->move_down_timer > 0.4f)
	{
		cursor->move_down_timer = 0.35f;
		process_movement = 1;
	}


   if((down_u || down_d) && process_movement)
   {

	    moved = 1;
		vec3 cursor_p  = cursor->position; 

	    f32 camera_z_u_inner = vec3_inner(gameRenderer->camera_z, V3(0, 1, 0));
	    f32 camera_z_r_inner = vec3_inner(gameRenderer->camera_z, V3(1, 0, 0));
	    i32 side_x = SIGN(camera_z_r_inner);
	    i32 side_y = SIGN(camera_z_u_inner);
	    if(down_d)
	    {
	    	side_x = -side_x;
	    	side_y = -side_y;
	    }

	    if(editor_input->shift_l)
	    {
	        cursor_p.z = (f32)(cursor_p.z - tile_displacement * side_y);
	    }
	    else if((camera_z_u_inner * camera_z_u_inner) > 
	            (camera_z_r_inner * camera_z_r_inner))
	    {
	        cursor_p.y = (f32)(cursor_p.y - tile_displacement * side_y);
	    }
	    else
	    {
	        cursor_p.x = (f32)(cursor_p.x - tile_displacement * side_x);
	    }

	    cursor->position = cursor_p;
   }
   if((down_r || down_l) && process_movement)
   {

	   moved = 1;
		vec3 cursor_p  = cursor->position; 

		f32 camera_z_u_inner = vec3_inner(gameRenderer->camera_z, V3(0, 1, 0));
		f32 camera_z_r_inner = vec3_inner(gameRenderer->camera_z, V3(1, 0, 0));
		//invert inners
		i32 side_x = SIGN(camera_z_u_inner);
		i32 side_y = -SIGN(camera_z_r_inner);
		if(down_l)
		{
		    side_x = -side_x;
		    side_y = -side_y;
		}


		if((camera_z_u_inner * camera_z_u_inner) > 
		   (camera_z_r_inner * camera_z_r_inner))
		{
		    cursor_p.x = (f32)(cursor_p.x - tile_displacement * side_x);
		}
		else
		{
		    cursor_p.y = (f32)(cursor_p.y - tile_displacement * side_y);
		}

		cursor->position = cursor_p;
   }
   return(moved);
}

inline void
editor_cursor_set_mesh_axes_by_orientation(editor_cursor *cursor)
{
	u32 tile_orientation = cursor->mesh_orientation;

	vec3 next_tile_uAxis = cursor->uAxis;
	vec3 next_tile_rAxis = cursor->rAxis;

	if(tile_orientation == 1)
	{
	    vec3 rAxis_copy = next_tile_rAxis;
	    vec3 uAxis_copy = next_tile_uAxis;

	    //flip to keep orientation
	    next_tile_rAxis.x = -uAxis_copy.x;
	    next_tile_rAxis.y = -uAxis_copy.y;
	    next_tile_rAxis.z = -uAxis_copy.z;

	    next_tile_uAxis.x = rAxis_copy.x;
	    next_tile_uAxis.y = rAxis_copy.y;
	    next_tile_uAxis.z = rAxis_copy.z;

	}
	else if(tile_orientation == 2)
	{
	    vec3 rAxis_copy = next_tile_rAxis;
	    vec3 uAxis_copy = next_tile_uAxis;

	    //flip to keep orientation
	    next_tile_rAxis.x = -rAxis_copy.x;
	    next_tile_rAxis.y = -rAxis_copy.y;
	    next_tile_rAxis.z = -rAxis_copy.z;

	    next_tile_uAxis.x = -uAxis_copy.x;
	    next_tile_uAxis.y = -uAxis_copy.y;
	    next_tile_uAxis.z = -uAxis_copy.z;

	}
    else if(tile_orientation == 3)
	{
	    vec3 rAxis_copy = next_tile_rAxis;
	    vec3 uAxis_copy = next_tile_uAxis;

	    //flip to keep orientation
	    next_tile_rAxis.x = uAxis_copy.x;
	    next_tile_rAxis.y = uAxis_copy.y;
	    next_tile_rAxis.z = uAxis_copy.z;

	    next_tile_uAxis.x = -rAxis_copy.x;
	    next_tile_uAxis.y = -rAxis_copy.y;
	    next_tile_uAxis.z = -rAxis_copy.z;

	}

	cursor->mesh_rAxis = next_tile_rAxis;
	cursor->mesh_uAxis = next_tile_uAxis;
}

inline vec3
editor_cursor_translate_ray_to_tile_wh(editor_cursor *cursor,
		                               vec3 rayOrigin,
		                               vec3 rayDir,
									   i32 tile_w,
									   i32 tile_h)
{

	//get the mouse position on the cursor depending on it's axes
	vec3 cursor_normal = vec3_cross(cursor->rAxis, cursor->uAxis);
	ray_casted_info ray_cursor_result = cast_ray_at_plane(rayOrigin,
				                                         rayDir,
												         cursor->position,
												         cursor_normal);
	vec3 mouse_at_cursor = ray_cursor_result.ray_on_plane;

	vec3 distance_ray_cursor = vec3_sub(mouse_at_cursor, cursor->position);

	vec3 next_tile_position = {0};
	//advance the position by the tile values and rotate them according to the cursor's axes
	next_tile_position.x = vec3_inner(distance_ray_cursor, cursor->mesh_rAxis);
	next_tile_position.y = vec3_inner(distance_ray_cursor, cursor->mesh_uAxis);
	next_tile_position.x = scalar_advance_by_no_minus_zero(next_tile_position.x, tile_w);
	next_tile_position.y = scalar_advance_by_no_minus_zero(next_tile_position.y, tile_h); 

	matrix3x3 next_tile_rotation = matrix3x3_from_vec_col(cursor->mesh_rAxis,
			                                              cursor->mesh_uAxis,
													      cursor_normal);

	//next_tile_position2 = matrix3x3_v3_mul_cols(next_tile_position, next_tile_positionRotation);
	next_tile_position = matrix3x3_v3_mul_rows(next_tile_rotation, next_tile_position);
	next_tile_position = vec3_round_to_int(next_tile_position);
	next_tile_position = vec3_add(next_tile_position, cursor->position);

	return(next_tile_position);

}


inline void
editor_render_gizmo(render_commands *commands,
		            vec3 gizmo_position,
					u32 is_hot,
					u32 hide)
{
	f32 gizmo_cube_size = 2.0f;
	 f32 unselected_alpha = 180.0f;
	 vec4 rotationMidPointColor = {255, 255, 0, 180};
	 vec4 circle_u_color        = {255, 255, 255, unselected_alpha};
	 vec4 circle_r_color        = {0, 0, 255, unselected_alpha};
	 if(is_hot)
	 {
		 rotationMidPointColor.w = 255;
	 }
	 //hide this while moving the camera
	 if(hide)
	 {
		 circle_u_color.w        = 20;
		 circle_r_color.w        = 20;
		 rotationMidPointColor.w = 20;
	 }

	 render_draw_cube(commands,
			                  gizmo_position, 
						      vec3_all(gizmo_cube_size),	
							  rotationMidPointColor);

    // render_Circle(commands,
	//               gizmo_position, 
	//               V3(1, 0, 0),
	//               V3(0, 0, 1),
	//               8,
	//               0.4f,
	//               circle_u_color);

    // render_Circle(commands,
	//               gizmo_position, 
	//               V3(1, 0, 0),
	//               V3(0, 1, 0),
	//               8,
	//               0.4f,
	//               circle_r_color);

}


static void
editor_cursor_rotate_paint_mesh_by_orientation(editor_cursor *cursor)
{
	vec3 v0 = cursor->mesh.v0;
	vec3 v1 = cursor->mesh.v1;
	vec3 v2 = cursor->mesh.v2;
	vec3 v3 = cursor->mesh.v3;
	u32 tile_orientation = cursor->mesh_orientation;
	//flip vertices
	if(tile_orientation == 1)
	{
		vec3 v1_copy = v1;
		v1 = v2;
		v2 = v3;
		v3 = v0;
		v0 = v1_copy;

	}
	else if(tile_orientation == 2)
	{
		vec3 v2_copy = v2;
		vec3 v1_copy = v1;
		v1 = v3;
		v3 = v1_copy;
		v2 = v0;
		v0 = v2_copy;

	}
    else if(tile_orientation == 3)
	{
		vec3 v3_copy = v3;
		v3 = v2;
		v2 = v1;
		v1 = v0;
		v0 = v3_copy;

	}

	cursor->mesh.v0 = v0;
	cursor->mesh.v1 = v1;
	cursor->mesh.v2 = v2;
	cursor->mesh.v3 = v3;
}

