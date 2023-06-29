#define DELTA_SPEED(dt)  dt * 49
static inline u32
kinda_hash_u32(u32 h, u32 seed) {
	h ^= h >> 16;
	h += seed;
	h *= 0x3243f6a9U;
	h ^= h >> 16;
	return(h);
}

typedef enum{
	map_entity_display,
	map_entity_event,
	map_entity_mark,
}entity_type;

typedef enum{
	danio_layer_null,
	danio_layer_player = 0x01,
	danio_layer_enemy = 0x02,
}danio_layer;

typedef struct{
	b8 valid;
	u8 next;
}ptr_16;

typedef struct{
	i16 x;
	i16 y;
	i16 z;
}vec3_16;

static void *
ptr_16_next(ptr_16 ptr, void *mem, u32 size_of_type)
{
	void *result = 0;
	u8 *mem8 = mem;
	result = mem8 + (ptr.next * size_of_type);

	return(result);
}

static void *
ptr_16_prev(ptr_16 ptr, void *mem, u32 size_of_type)
{
	void *result = 0;
	u8 *mem8 = mem;
	result = mem8 - (ptr.next * size_of_type);

	return(result);
}

typedef union{
	u64 id;
	struct{
		u32 id0;
		u32 id1;
	};
}coso_id;

typedef enum{
	coso_null             = 0x01,
	coso_damage           = 0x02,
	coso_body             = 0x04,
	coso_traversable      = 0x08,
	coso_lifetime         = 0x10,
	coso_no_gravity       = 0x20,
	coso_move_with_parent = 0x40,
	coso_frame_lifetime   = 0x80,
	coso_hitpoints        = 0x100,
	coso_render           = 0x200
}coso_flags;

typedef enum{
	shape_cube,
	shape_sphere
}game_shape_type;

typedef struct{
	game_shape_type type;
	vec3 p;
	union{
		f32 radius;
		vec3 size;
		struct{
			f32 size_x;
			f32 size_y;
			f32 size_z;
		};
	};
}game_shape;

typedef struct{
	coso_id id1;
	b16 frame_timer;
	u16 frames_before_hit;
}damage_contact;

typedef struct{

	b16 frame_timer;
	u16 frames_before_hit;
	b32 sweep;
	f32 sweep_angle;
}attack_data;

typedef struct{
	coso_flags flags;
	game_shape_type shape;
	vec3 p;
	vec3 relative_p;
	vec3 size;
	vec2 dir;
	f32 speed;
	f32 radius;

	f32 life_time;
	u32 id1;
	struct world_entity *parent;

	u32 hitpoints;

	//damage data
	f32 distance;
	f32 start_angle;
	f32 end_angle;
	i32 side;

	b32 sweep;
	u32 impact;
	b16 dmg_frame_time;
	u16 frames_before_hit;
	
}entity_spawn_parameters;

//physics

typedef struct cubes_overlap_result{
	union{
		i32 side_value;
		i32 value;
		struct{
			i8 side_x;
			i8 side_y;
			i8 side_z;
		};
	};
}cubes_overlap_result;


typedef struct active_body{
	struct active_body *next;
	struct active_body *prev;
	struct game_body *body;
}active_body;

typedef struct{
	b32 active;
	u32 body_count;
	active_body *active_bodies;
}physics_zone;

typedef struct game_body{
	struct game_body *next;
	struct game_body *prev;

	game_shape shape;
	void *user_data;
	u32 id;
	//maybe flags layer.
	b8 traversable;
	b8 collides;
	b8 grounded;
	u8 collided_count;
	b16 ignore_gravity;
	b16 move_with_parent;

	//position of the closest collider below this body
	f32 nearest_z_min;
	f32 nearest_z_max;
	vec3 p;
	vec3 v;
	vec3 p_past;
	vec3 correction;
	f32 z_speed;
	f32 speed;
	f32 speed_max;
	f32 bounce_factor;
	f32 weight;

}game_body;

typedef struct game_contact_points{

	vec3 correction;
	vec3 normal;
	f32 penetration;
	b32 got_contact;
}game_contact_points;

typedef struct{
	game_body *body0;
	vec3 tp;
	vec3 ts;
	game_contact_points contact_data;
}game_tile_solve;

typedef struct{
	game_body *body0;
	game_body *body1;
	game_contact_points contact_data;
}game_body_solve;


typedef struct{
	game_body *body0;
	game_body *body1;
}game_collision_signal;

typedef struct game_danio{
	enum32(danio_layer) layer;
	b16 marked_for_free;
	u16 index;

	f32 lifetime;
	u32 parent_id;
	vec3 position;
	vec3 velocity;
	i16 size_x;
	i16 size_y;
	i16 size_z;
	u32 hit_times;

	struct game_danio *next;
}game_danio;

typedef struct{
	u16 id;
	u16 hit_amount;
	f32 hit_time;
}game_danio_detection;

typedef struct{
	union{
		b8 off;
		f32 magnitude;
	};
}world_harea_tile;

typedef struct{
	u32 type;
	union{
		vec3 p;
		struct{
			f32 x;
			f32 y;
			f32 z;
		};
	};
	u16 w;
	u16 h;

	u32 random_seed;
	world_harea_tile *tiles;
}world_harea;
// Tileset
//points to tileset and its terrain
typedef struct{

	u16 tileset_index;
	u16 tileset_terrain_index;
	i16 height;
	u16 tileset_terrain_frame;
	u32 wall_frame;
	//autotile data
	b16 is_autoterrain;
	u16 autoterrain_index;
	u16 autoterrain_layer;

}world_tile;
/*
   pt uses a height map to  render the tile's height.
   Might do the same for that and ai?
*/
typedef struct entity_detection_data{
	b32 is_player;
	f32 distance_squared;
	u16 group;
	u16 index;
	vec3 position;
}entity_detection_data;

typedef enum{
	cube,
	slope_floor_t,
	slope_floor_b,
	slope_floor_r,
	slope_floor_l,
	slope_wall_only_tl,
	slope_wall_only_tr,
	slope_wall_only_bl,
	slope_wall_only_br,
	slope_wall_tl,
	slope_wall_tr,
	slope_wall_bl,
	slope_wall_br,

	terrain_shapes_count

}tileset_terrain_shape;

static inline void
tileset_terrain_shape_fill_names(
		u8 *shape_types[])
{
}

typedef struct{
	u32 bit;
	u32 terrain_index;
}tileset_autotile_tile;

typedef struct{
	u16 capacity;
	u16 indices_at;
	u16 terrain_group;
	u16 extra_layers;
	u16 *indices;
}tileset_autoterrain;

typedef struct{
	b8 repeats;
	b8 use_floor;
	b8 use_wall;
	b8 reserved;
	u16 wall_index;

	//u16 fx;
	//u16 fy;
	//u16 fw;
	//u16 fh;
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;

	u16 capacity;
	u16 shape;
	u32 sides;
	u32 terrain_group;

	union{
	u16 mesh_count;
	u16 uvs_count;
	};
	u16 uvs_at_vertices_at;
}s_tileset_terrain;

typedef struct{
	u16 uvs_at;
	u16 uvs_count;
	u16 extra_frames_x;
	u16 extra_frames_y;
	b32 repeat;
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
}tileset_wall;


typedef struct world_tileset{
	render_texture *image;

	u16 terrain_count;
	u16 terrain_max;
	s_tileset_terrain *terrain;

	u16 wall_count;
	tileset_wall *walls;
	u32 uvs_count;
	union{

		model_mesh *uvs;
		model_mesh *meshes;
	};

	u16 autoterrain_max;
	u16 autoterrain_count;
	tileset_autoterrain *autoterrains;

	u16 autoterrain_indices_max;
	u16 autoterrain_indices_count;
	u16 *autoterrain_indices;

	u16 w;
	u16 h;
	u32 tileSize;

	u32 count;
	u32 capacity;

}world_tileset;

typedef struct{
	vec3 position;
}map_mark;

typedef struct{
	u32 type;
	vec3 vector3;
	vec3 size3;
	rectangle32s area;

	u32 next_map_index;
}event_line;

typedef struct{
	u32 type;
	vec3 vector3;
	vec3 size3;
	rectangle32s area;
}event_condition;

typedef struct game_event{
	struct game_event *next;
	struct game_event *prev;

	struct game_event *next_ad;
	struct game_event *prev_ad;
	b32 eliminate_when_finished;
	b16 is_global;
	u16 map_id;
	u16 conditions_at;
	u16 condition_count;
	event_condition *conditions;
	u16 lines_at;
	u16 line_count;
	u32 current_line;
	event_line *lines;
}game_event;

typedef struct connected_events{
	struct connected_events *next;
	struct connected_events *prev;
	u32 condition_count;
}conected_events;

typedef struct{
	u16 event_count;
	game_event *events;
	game_event *first_running_event;
	game_event *inactive_events;
	game_event *active_events;

	u32 line_count;
	event_line *lines;
	u32 condition_count;
	event_condition *conditions;
}event_main;

//detections
struct world_entity;

typedef struct{
	f32 distance_squared;
	i16 side_x;
	i16 side_y;
	b32 overlap;
	//coso_id id0;
	//coso_id id1;
	struct world_entity *id0;
	struct world_entity *id1;
}cosos_detection;



#define tileset_terrain_uvs(tileset, terrain) (tileset->uvs + terrain->uvs_at_vertices_at)

static void
et_fill_capacity_data(
		u32 capacity,
		u32 *w,
		u32 *h)
{
	if(capacity == 16)
	{
		*w = 6;
		*h = 3;
	}
	else if(capacity == 48)
	{
		*w = 8;
		*h = 6;
	}
	else 
	{
		*w = 1;
		*h = 1;
	}
}


static inline u8
autoterrain_index_from_mask16(u32 mask_value)
{
	u8 i[16] = {0};

	i[0] = 15;
	i[1] = 11;
	i[2] = 5;
	i[3] = 14;
	i[4] = 3;
	i[5] = 12;
	i[6] = 4;
	i[7] = 13;
	i[8] = 9;
	i[9] = 10;
	i[10] = 2;
	i[11] = 8;
	i[12] = 0;
	i[13] = 6;
	i[14] = 1;
	i[15] = 7;
	/*
	   15, 11, 5, 14, 3, 12,
	   4,  13, 9, 10, 2, 14,
	   0,   6, 1,  7
	*/
	u32 index = i[mask_value];
	return(index);
}

static inline u8
autoterrain_index_from_mask46(u32 masked_amount, u32 mask_value)
{
	/*
	   for 16:
	   0, 1, 0
	   2, 0, 4
	   0, 8, 0

	   resulting image from values:
	   0 = m corners
       1 = b corners l,d,r
	   2 = r corners u,d,r
	   3 = br corners r,d
	   4 = r corners u,d,r
	   5 = bl corners l,d
	   6 = m corners d,u
	   7 = b corners d
	   8 = t corners l,u,r
	   9 = m corners r,l
	   10 = tr corners t,r
	   11 = m corners r
	   12 = tl corners l,u
	   13 = m corners l
	   14 = m corners u
	   15 = m no corners

	   for 46:
	   1, 2, 4,
	   8, 0, 16
	   32, 64, 128
	   //should first check edges and then nearby edges

	   0 = m ldlr (no corners)
	   1 = slope ld
	   2 = edge ltr
	   4 = slope rd
	   8 = edge tlb
	   16 = edge trb
	   32 = slope rd (same as 4)
	   64 = edge ltr
	   128 = slope ld (same as 1)
	   3 = slope tr
	   7 = edge t
	   6 = slope tl

	   10 = edge lt (no corner tl)
	   5 = X
	   132 = X

	*/
#if 1
	u8 indices[256] = {0};

	indices[0] = 47;
	indices[2] = 1;
	indices[3] = indices[2];
	indices[6] = indices[2];
	indices[7] = indices[2];
	indices[8] = 2;
	indices[9] = indices[8];
	indices[10] = 3;
	indices[11] = 4;
	indices[14] = indices[10];
	indices[15] = 4;
	indices[16] = 5;
	indices[18] = 6;
	indices[19] = 6;
	indices[20] = indices[16];
	indices[22] = 7;
	indices[23] = 7;
	indices[24] = 8;
	indices[26] = 9;
	indices[27] = 10;
	indices[30] = 11;
	indices[31] = 12;
	indices[40] = indices[8];
	indices[41] = indices[8];
	indices[43] = indices[11];
	indices[64] = 13;
	indices[66] = 14;
	indices[72] = 15;
	indices[74] = 16;
	indices[75] = 17;
	indices[80] = 18;
	indices[82] = 19;
	indices[86] = 20; 
	indices[88] = 21; 
	indices[90] = 22; 
	indices[91] = 23; 
	indices[94] = 24; 
	indices[95] = 25; 
	indices[96] = indices[64]; 
	indices[104] = 26; 
	indices[105] = indices[104]; 
	indices[106] = 27; 
	indices[107] = 28; 
	indices[112] = indices[80];
	indices[120] = 29; 
	indices[122] = 30; 
	indices[123] = 31; 
	indices[126] = 32; 
	indices[127] = 33; 
	indices[144] = indices[16];
	indices[148] = indices[16];
	indices[150] = indices[22];
	indices[189] = indices[24];
	indices[191] = indices[31];
	indices[192] = indices[64];
	indices[200] = indices[72];
	indices[208] = 34; 
	indices[212] = indices[208]; 
	indices[210] = 35; 
	indices[214] = 36; 
	indices[216] = 37; 
	indices[218] = 38; 
	indices[219] = 39; 
	indices[222] = 40; 
	indices[223] = 41;
	indices[224] = indices[64]; 
	indices[231] = indices[66];
	indices[232] = indices[104];
	indices[239] = indices[107];
	indices[240] = indices[208]; 
	indices[248] = indices[2014]; 
	indices[248] = 42; 
	indices[250] = 43; 
	indices[251] = 44; 
	indices[254] = 45; 
	indices[255] = 46;
	u32 index = indices[mask_value];
#else
	u32 index = (u32)((f32)mask_value / (f32)masked_amount);
#endif
	return(index);
}



