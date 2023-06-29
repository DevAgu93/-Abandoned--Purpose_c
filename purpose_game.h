inline void
game_update_camera(
		game_renderer *game_renderer,
		game_render_parameters *render_parameters,
		b32 apply_bounds)
{

	vec3 camera_target = render_parameters->camera_target;



//	distance_camera_target = 160.0f;
	f32 distance_camera_target = render_parameters->distance_camera_target * render_parameters->z_fix;
	//limit the camera target

	vec3 final_camera_distance = {0, 0, distance_camera_target};


	quaternion camera_rotation_m = quaternion_from_rotations_scale(
			render_parameters->camera_rotation_x,
			render_parameters->camera_rotation_y,
			render_parameters->camera_rotation_z);

	final_camera_distance = quaternion_v3_mul_foward_inverse(
			                       camera_rotation_m,
			                       final_camera_distance);

	//vec3 prev = camera_position;
	//make the camera follow the player
	render_parameters->camera_position = vec3_add(final_camera_distance,
			V3(camera_target.x, camera_target.y, 0));
//	vec3 delta = vec3_sub(camera_position, prev);

	if(apply_bounds)
	{
		vec3 vx = game_renderer->camera_x; 
		vec3 vy = game_renderer->camera_y; 
		vec3 vz = game_renderer->camera_z; 

		matrix4x4_data cameraMatrices = 
			matrix4x4_camera_transform(
					vx,
					vy,
					vz,
					render_parameters->camera_position);
		matrix4x4 projection = matrix4x4_mul(
				game_renderer->projections.foward,
				cameraMatrices.foward);
		vec4 plane_b = vec4_PlaneBottom(projection); 
		vec4 plane_t = vec4_PlaneTop(projection); 

		if(plane_b.w >= 0)
		{
			render_parameters->camera_position.y += plane_b.w / plane_b.y;
		}
		//480 es un limite hardcodeado final del mapa
		//dif es la diferencia entre plane_t.w y el limite
		//plane_t.y dado a la inclinación de la cámara, es un número negativo
		if(plane_t.w >= 480)
		{
			f32 dif = plane_t.w - 480;
			render_parameters->camera_position.y += dif / plane_t.y;
		}
	}




}
