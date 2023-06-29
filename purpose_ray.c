
typedef struct{
	f32 t_min;
	union{
		struct{
            i8 hit_x;
	        i8 hit_y;
	        i8 hit_z;
		};
	    i32 hit_value;
	};
}ray_cube_result;

typedef struct{
	f32 t_min;
	f32 t_max;
	vec3 cube_t_min;
}ray_cube_data;

typedef struct{
	vec3 ray_on_plane;
	f32 distance_to_plane;
	b32 hits;
}ray_casted_info;

typedef struct{
	u32 hit;
	f32 distance;
}ray_hit_result;

typedef struct{
	f32 distance;
	vec3 uvw;
}raycast_baycentric_result;

typedef struct s_ray{
	vec3 origin;
	vec3 direction;
}s_ray;
//
// ray functions
//

static inline ray_hit_result 
ray_open_circle_got_hit(
		s_ray ray_od,
		vec3 circle_position,
		vec3 circle_normal,
		f32 circle_radius,
		f32 circle_thickness
		)
{
	ray_hit_result result = {0};
	real32 circle_ray_dot = vec3_inner(ray_od.direction, circle_normal); 
	if(circle_ray_dot)
	{
		//check if the ray is inside the thickness of the circle.
	    vec3 distance_from_ray = vec3_sub(circle_position, ray_od.origin);
		f32 ray_distance = vec3_inner(distance_from_ray, circle_normal) / circle_ray_dot;
		vec3 ray_position_in_circle = vec3_add(ray_od.origin, vec3_scale(ray_od.direction, ray_distance));

		f32 distance_from_circle_origin_dot = vec3_inner_squared(vec3_sub(ray_position_in_circle, circle_position));
		f32 radius_sq = (circle_radius - circle_thickness) * (circle_radius - circle_thickness);
		if(distance_from_circle_origin_dot >= radius_sq &&
		   distance_from_circle_origin_dot <= (circle_radius * circle_radius))
		{
	        result.hit = 1;
			result.distance = ray_distance;
		}
	}
	return(result);
}

static raycast_baycentric_result
ray_triangle_get_uvw(vec3 ray_origin,
		             vec3 ray_direction,
					 vec3 v0,
					 vec3 v1,
					 vec3 v2)
{
	raycast_baycentric_result result = {0};
			vec3 distance_v2_v0 = vec3_sub(v2, v0);
			vec3 distance_v1_v0 = vec3_sub(v1, v0);
			vec3 distance_v0_v2 = vec3_sub(v0, v2);
			
			vec3 triangle0_normal = vec3_cross(distance_v1_v0, distance_v2_v0);
			
			f32 inner_ray_triangle0 = -vec3_inner(ray_direction, triangle0_normal);
			f32 denominator         = vec3_inner_squared(triangle0_normal);

			vec3 uvw = {0};
			if(inner_ray_triangle0)
			{

				f32 d                   = vec3_inner(triangle0_normal, v0);
				f32 inner_normal_origin = vec3_inner(triangle0_normal, ray_origin);
				f32 distance1           = (inner_normal_origin - d) / inner_ray_triangle0;
				vec3 ray0_end           = vec3_add(ray_origin, vec3_scale(ray_direction, distance1));


				vec3 edge0 = distance_v1_v0;
				vec3 edge1 = distance_v0_v2;

				vec3 distance_end_v1 = vec3_sub(ray0_end, v1);
				vec3 distance_end_v2 = vec3_sub(ray0_end, v2);

				//v1_v0
				vec3 c = vec3_cross(edge0, distance_end_v1);
				uvw.x = vec3_inner(triangle0_normal, c);

				//v0_v2
			    c = vec3_cross(edge1, distance_end_v2);
				uvw.y = vec3_inner(triangle0_normal, c);

				//move from v0-v1
				uvw.x /= denominator;
				//move from v1-v2
				uvw.y /= denominator;
				//v1-v0
				uvw.z = 1 - uvw.x - uvw.y;

				result.distance = ABS(distance1);
				result.uvw      = uvw;
			}
			return(result);
}

inline u32
uvw_is_inside(vec3 uvw)
{
	u32 result = (uvw.x > 0) &&
		         (uvw.y > 0) &&
				 (uvw.z > 0);
	return(result);
}


static ray_casted_info 
cast_ray_at_plane(vec3 ray_origin, 
		          vec3 ray_direction,
				  vec3 planeOrigin,
				  vec3 planeN)
{
	ray_casted_info result = {0};
	real32 planeRayDir = vec3_inner(ray_direction, planeN); 
	if(planeRayDir)
	{
	    vec3 distance_from_ray = vec3_sub(planeOrigin, ray_origin);
	    real32 rayDistance = vec3_inner(distance_from_ray, planeN) / planeRayDir;
	    result.ray_on_plane = vec3_add(ray_origin, vec3_f32_mul(ray_direction, rayDistance));
		result.distance_to_plane = rayDistance;
	}
	return(result);
}


inline ray_casted_info
cast_ray_at_point(vec3 ray_origin,
		          vec3 ray_direction,
				  vec3 point)
{
	ray_casted_info result = {0};

	vec3 distance_point_and_ray_origin = vec3_sub(point, ray_origin);
	f32 inner_distance_dir = vec3_inner(distance_point_and_ray_origin, ray_direction);

	vec3 ray_end = vec3_add(ray_origin, vec3_scale(ray_direction, inner_distance_dir));

	result.ray_on_plane      = ray_end;
	result.distance_to_plane = inner_distance_dir;


	return(result);
}

static ray_casted_info 
ray_circle_upfront_result(vec3 ray_origin,
		vec3 ray_dir,
		vec3 circle_p,
		f32 radius)
{
	ray_casted_info result = cast_ray_at_point(ray_origin, ray_dir, circle_p);
	vec3 distance_ray_end_point = vec3_sub(result.ray_on_plane, circle_p);
	f32 distance_inner = vec3_inner_squared(distance_ray_end_point);
	result.hits = (distance_inner) < (radius * radius);
	return(result);
}


//cast ray agains cube and get the direction it hits.
//This assumes the cube position is on the middle.
//This function doesn't tell you which direction it hit, call
//ray_cube_get_result for that instead
static ray_cube_data
ray_cube_fill_data(vec3 ray_origin,
		           vec3 ray_direction,
				   vec3 cube_position,
				   vec3 cube_size)
{
	vec3 cube_size_half = vec3_scale(cube_size, 0.5f);
	vec3 cube_min       = vec3_sub(cube_position, cube_size_half);
	vec3 cube_max       = vec3_add(cube_position, cube_size_half);

	vec3 cube_faces_min = vec3_div(vec3_sub(cube_min, ray_origin), ray_direction);
	vec3 cube_faces_max = vec3_div(vec3_sub(cube_max, ray_origin), ray_direction);

	vec3 cube_t_min;
	cube_t_min.x = MIN(cube_faces_min.x, cube_faces_max.x);
	cube_t_min.y = MIN(cube_faces_min.y, cube_faces_max.y);
	cube_t_min.z = MIN(cube_faces_min.z, cube_faces_max.z);
	vec3 cube_t_max;
	cube_t_max.x = MAX(cube_faces_min.x, cube_faces_max.x);
	cube_t_max.y = MAX(cube_faces_min.y, cube_faces_max.y);
	cube_t_max.z = MAX(cube_faces_min.z, cube_faces_max.z);

	//f32 t_min = MAX(cube_faces_min.x, MAX(cube_faces_min.y, cube_faces_min.z));
	//f32 t_max = MIN(cube_faces_max.x, MIN(cube_faces_max.y, cube_faces_max.z));
	f32 t_min = MAX(cube_t_min.x, MAX(cube_t_min.y, cube_t_min.z));
	f32 t_max = MIN(cube_t_max.x, MIN(cube_t_max.y, cube_t_max.z));

    ray_cube_data data = {0};
	data.cube_t_min    = cube_t_min;
	data.t_min         = t_min;
	data.t_max         = t_max;

	return(data);

}

//Indicates if the ray intersects with the cube and fills the directions
//and sets the direction of the face hit in right hand coordinates.
inline ray_cube_result
ray_cube_get_result(vec3 ray_origin,
		           vec3 ray_direction,
				   vec3 cube_position,
				   vec3 cube_size)
{
	ray_cube_data data = ray_cube_fill_data(ray_origin,
			                                    ray_direction,
											    cube_position,
											    cube_size);

	ray_cube_result result = {0};
	//if this is true, the cube got hit by the ray.
	if(data.t_max >= data.t_min)
	{

	    u32 hit_x = data.t_min == data.cube_t_min.x;
	    u32 hit_y = data.t_min == data.cube_t_min.y;
	    u32 hit_z = data.t_min == data.cube_t_min.z;
		result.t_min = data.t_min;

	    if(hit_x)
	    {
			//points to positive x, so the "left" face got hit..
	    	if(ray_direction.x > 0)
	    	{
	    	    result.hit_x = -1;
	    	}
	    	else
	    	{
	    	    result.hit_x = 1;
	    	}
	    }
	    else if(hit_y)
	    {
	    	if(ray_direction.y > 0)
	    	{
	    	    result.hit_y = -1;
	    	}
	    	else
	    	{
	    	    result.hit_y = 1;
	    	}
	    }
	    else if(hit_z)
	    {
	    	if(ray_direction.z > 0)
	    	{
	    	    result.hit_z = -1;
	    	}
	    	else
	    	{
	    	    result.hit_z = 1;
	    	}
	    }
	}

	return(result);
}

//only tells you if the cube got hit by the ray.
inline u32
ray_cube_intersects(vec3 ray_origin,
		              vec3 ray_direction,
				      vec3 cube_position,
				      vec3 cube_size)
{
	ray_cube_data data     = ray_cube_fill_data(ray_origin,
			                                    ray_direction,
											    cube_position,
											    cube_size);

	u32 result = data.t_max >= data.t_min;

	return(result);
}

inline i32
ray_rectangle_intersects(vec3 ray_origin,
		                 vec3 ray_direction,
						 vec3 rec_position,
						 vec2 rec_size,
						 vec3 rec_xAxis,
						 vec3 rec_yAxis,
						 vec3 rec_zAxis)
{
	ray_casted_info ray_result = cast_ray_at_plane(ray_origin, ray_direction, rec_position, rec_zAxis);
	vec3 ray_at_rectangle        = vec3_sub(ray_result.ray_on_plane, rec_position);

	f32 rec_ray_x = vec3_inner(ray_at_rectangle, rec_xAxis);
	f32 rec_ray_y = vec3_inner(ray_at_rectangle, rec_yAxis);

	vec2 rec_size_half = vec2_scale(rec_size, 0.5f);

	u32 inside = (rec_ray_x > (-rec_size_half.x) && rec_ray_x < rec_size_half.x) &&
		         (rec_ray_y > (-rec_size_half.y) && rec_ray_y < rec_size_half.y);
	return(inside);
}

#define ray_rectangle_cast_xy(ray_origin, ray_direction, ray_distance, rec_p, rec_size, x_percent, y_percent) \
	ray_rectangle_cast(ray_origin, ray_direction, ray_distance, rec_p, rec_size, V3(1, 0, 0), V3(0, 1, 0), x_percent, y_percent)\

#define ray_rectangle_fill_position_xy(ray_origin, ray_direction, ray_distance, rec_p, rec_size, x_percent, y_percent, x_ptr, y_ptr) \
	ray_rectangle_cast_fill_info(ray_origin, ray_direction, ray_distance, rec_p, rec_size, V3(1, 0, 0), V3(0, 1, 0), x_percent, y_percent, x_ptr, y_ptr, 0)
#define ray_rectangle_fill_position_xy_bl(ray_origin, ray_direction, ray_distance, rec_p, rec_size, x_ptr, y_ptr) \
	ray_rectangle_cast_fill_info(ray_origin, ray_direction, ray_distance, rec_p, rec_size, V3(1, 0, 0), V3(0, 1, 0), 0.5f, 0.5f, x_ptr, y_ptr, 0)

static f32
ray_rectangle_cast_fill_info(
		vec3 ray_origin,
		vec3 ray_direction,
		f32 ray_distance,
		vec3 rec_p,
		vec2 rec_size,
		vec3 rec_x,
		vec3 rec_y,
		f32 x_corner_percent,
		f32 y_corner_percent,
		f32 *ray_x,
		f32 *ray_y,
		b32 *hit)
{
	{
		vec3 x_size0 = vec3_scale(rec_x, .5f * x_corner_percent * rec_size.x);
		vec3 x_size1 = vec3_scale(rec_x, .5f * (1.0f - x_corner_percent) * rec_size.x);
		vec3 y_size0 = vec3_scale(rec_y, .5f * y_corner_percent * rec_size.y);
		vec3 y_size1 = vec3_scale(rec_y, .5f * (1.0f - y_corner_percent) * rec_size.y);
		rec_p = vec3_add(rec_p, x_size0);
		rec_p = vec3_sub(rec_p, x_size1);
		rec_p = vec3_add(rec_p, y_size0);
		rec_p = vec3_sub(rec_p, y_size1);
	}
	vec3 rec_z = vec3_cross(rec_x, rec_y);
	ray_casted_info ray_result = cast_ray_at_plane(ray_origin, ray_direction, rec_p, rec_z);
	//distance from the origin of the rectangle and the position
	//of the ray on the plane
	vec3 ray_at_rectangle = vec3_sub(ray_result.ray_on_plane, rec_p);

	f32 rec_ray_x = vec3_inner(ray_at_rectangle, rec_x);
	f32 rec_ray_y = vec3_inner(ray_at_rectangle, rec_y);

	vec2 rec_size_half = vec2_scale(rec_size, 0.5f);

	u32 inside = (rec_ray_x > (-rec_size_half.x) && rec_ray_x < rec_size_half.x) &&
		         (rec_ray_y > (-rec_size_half.y) && rec_ray_y < rec_size_half.y);
	if(inside && ray_distance > ray_result.distance_to_plane)
	{
		ray_distance = ray_result.distance_to_plane;
		if(hit) (*hit) = 1;
	}
	//fill the x and y positions of the ray inside the rectangle,
	//mostly used for tile editing
	if(ray_x) (*ray_x) = rec_ray_x;
	if(ray_y) (*ray_y) = rec_ray_y;

	return(ray_distance);
}

static f32
ray_rectangle_cast(
		vec3 ray_origin,
		vec3 ray_direction,
		f32 ray_distance,
		vec3 rec_p,
		vec2 rec_size,
		vec3 rec_x,
		vec3 rec_y,
		f32 x_corner_percent,
		f32 y_corner_percent)
{
	{
		vec3 x_size0 = vec3_scale(rec_x, .5f * x_corner_percent * rec_size.x);
		vec3 x_size1 = vec3_scale(rec_x, .5f * (1.0f - x_corner_percent) * rec_size.x);
		vec3 y_size0 = vec3_scale(rec_y, .5f * y_corner_percent * rec_size.y);
		vec3 y_size1 = vec3_scale(rec_y, .5f * (1.0f - y_corner_percent) * rec_size.y);
		rec_p = vec3_add(rec_p, x_size0);
		rec_p = vec3_sub(rec_p, x_size1);
		rec_p = vec3_add(rec_p, y_size0);
		rec_p = vec3_sub(rec_p, y_size1);
	}
	vec3 rec_z = vec3_cross(rec_x, rec_y);
	ray_casted_info ray_result = cast_ray_at_plane(ray_origin, ray_direction, rec_p, rec_z);
	//distance from the origin of the rectangle and the position
	//of the ray on the plane
	vec3 ray_at_rectangle = vec3_sub(ray_result.ray_on_plane, rec_p);

	f32 rec_ray_x = vec3_inner(ray_at_rectangle, rec_x);
	f32 rec_ray_y = vec3_inner(ray_at_rectangle, rec_y);

	vec2 rec_size_half = vec2_scale(rec_size, 0.5f);

	u32 inside = (rec_ray_x > (-rec_size_half.x) && rec_ray_x < rec_size_half.x) &&
		         (rec_ray_y > (-rec_size_half.y) && rec_ray_y < rec_size_half.y);
	if(inside && ray_distance > ray_result.distance_to_plane)
	{
		ray_distance = ray_result.distance_to_plane;
	}

	return(ray_distance);
}

inline ray_hit_result
ray_quad_get_hit_result(vec3 ray_origin,
		            vec3 ray_direction,
					vec3 v0,
					vec3 v1,
					vec3 v2,
					vec3 v3,
					f32 ray_hit_distance)
{

	raycast_baycentric_result triangle_result_0 = ray_triangle_get_uvw(ray_origin,
	                                                                   ray_direction,
		                                                               v0,
		                                                               v1,
		                                                               v2);
	raycast_baycentric_result triangle_result_1 = ray_triangle_get_uvw(ray_origin,
	                                                                   ray_direction,
		                                                               v0,
		                                                               v2,
		                                                               v3);
    vec3 uvw0 = triangle_result_0.uvw;
    vec3 uvw1 = triangle_result_1.uvw;

	u32 inside0 = uvw_is_inside(uvw0);
	u32 inside1 = uvw_is_inside(uvw1);

	u32 is_closest_mesh = 0;
    
    if(inside0 || inside1)
    {
		f32 triangle_distance_0 = triangle_result_0.distance;
		f32 triangle_distance_1 = triangle_result_1.distance;

		if(inside0 && triangle_distance_0 < ray_hit_distance)
		{
			is_closest_mesh = 1;
            ray_hit_distance = triangle_distance_0;

		}
		else if(inside1 && triangle_distance_1 < ray_hit_distance)
		{
			is_closest_mesh = 1;
            ray_hit_distance = triangle_distance_1;
		}
    }

	ray_hit_result result = {0};
	result.hit = is_closest_mesh;
	result.distance = ray_hit_distance;

	return(result);
}


//
// ray functions__
//
