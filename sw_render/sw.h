

typedef struct s_camera{
	vec3 position;
	vec3 rotation;
	f32 fov;
}s_camera;

typedef struct s_program_state{

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
	game_ui *ui;
	random_series random_s;
	//render
	game_render_parameters render_parameters;

	//debug stuff
		f32 elapsed_ms;
		f32 mc_per_frame;
}program_state;
