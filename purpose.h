#define requested_entities_MAX 50
#define delete_entities_MAX 50
typedef struct s_camera{
	vec3 position;
	vec3 rotation;
	f32 fov;
}s_camera;


typedef struct s_game_variables s_game_variables;

typedef struct s_game_state{

    memory_area *area;
	//asset_font default_font;
    font_proportional *default_font;

	u16 target_framerate;
	b16 reload_game;
	u32 game_loaded;
	b8 debug_display_console;
	b8 debug_display_ui;
	//game_main main_game;

    game_assets *game_asset_manager;
	program_input *input;

	//debug and log
    platform_api *platform;

	memory_area info_stream_area;
    stream_data info_stream;
	s_game_console debug_console;
	game_ui *ui;

	random_series random_s;
	//render
	game_render_parameters render_parameters;

	//debug stuff
		f32 elapsed_ms;
		f32 mc_per_frame;
		u32 cycles_elapsed;
	struct{
		union{
			u32 bools[2];
			struct{
				u8 free_camera;
				u8 interacting_with_ui;
				b8 debug_display_colliders;
				b8 debug_display_entity_colliders;

				b8 use_camera_bounds;
			};
		};
		s_camera camera;
//		debug_commands *commands;
	}debug;

	//world, I could use multiple maps

	u16 map_count;
	u16 current_map_index;
	game_world *maps;
	b16 switch_map;
	u16 next_map_index;
	b32 faded;
	f32 fade_alpha;
	f32 fade_speed;
	vec3 map_swap_p;


	//physics
	u16 body_count;
	u16 body_max;
	//capaz de simular UN o multiples "mundos" a la vez
	//capaz de intercambiarse cuerpos de otros mundos.
	physics_zone *physics_zones;
	u32 active_zone_index;

	game_body *bodies;
	game_body *first_free_body;
	game_body *first_body;
	u32 body_id;
	game_contact_points temp_contact_point;

	u16 body_solve_count;
	u16 body_solve_max;
	game_body_solve *body_solves;
	u16 body_tile_solve_count;
	u16 body_tile_solve_max;
	game_tile_solve *body_tile_solves;

	u16 collision_signal_count;
	u16 collision_signal_max;
	game_collision_signal *collision_signals;

	//brains
	u32 brains_memory_used;
	u32 brains_memory_size;
	u8 *brains_memory;
	state_main *first_brain;
	state_main *first_free_brain;
	memory_block_main states_memory;

	//entities
	u32 next_entity_id;
	u16 entities_count;
	u16 entities_max;
	world_entity *entities;
	world_entity *first_free_entity;
	world_entity *first_entity;
	world_entity *last_entity;

	u16 requested_entities_count;
	entity_spawn_parameters requested_entities[requested_entities_MAX];
	u16 deleting_entities_count;
	world_entity *entities_to_delete[delete_entities_MAX];

	u32 player_entity_index;
	world_entity *player_entity;

	//misc
	game_body *controlling_body;
	world_entity *focused_entity;
	state_main *test_focused_brain;

	u16 detections_count;
	u16 detections_max;
	cosos_detection *detections;

	u32 active_events_count;
	game_event *active_events;

}program_state;
