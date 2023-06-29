

typedef struct s_game_input{
    union {
        input_button buttons[6];
        struct{
            input_button up;
            input_button down;
            input_button left;
            input_button right;
			input_button attack;
			input_button jump;
        };
    };

	union
	{
		platform_mouse mouse;
		struct{
			union{
				vec2 mouse_clip;
				struct{
					f32 mouse_clip_x;
					f32 mouse_clip_y;
				};
			};
			f32 mouse_clip_x_last;
			f32 mouse_clip_y_last;

			f32 mouse_x;
			f32 mouse_y;
			f32 mouse_x_last;
			f32 mouse_y_last;

			union
			{
				input_button mouse_buttons[2];
				struct{
					input_button mouse_left;
					input_button mouse_right;
				};
			};
		};
	};

	union{
		u8 bools[12];
		struct{
			u8 enter;
			u8 ctrl_l;
			u8 shift_l;
			u8 alt;

			u8 doubleClickLeft;
			u8 doubleClickedLeft;
			u8 lastClickWasLeft;
			u8 tripleClickLeft;

			u8 mouse_middle;
			u8 spaceBarDown;
		};
	};
	i16 mouse_wheel;

	u32 lastClickTime;
	u32 lastDoubleClickTime;
	u32 lastDoubleClickedTime;



	u8 f_keys[12];
}program_input;


struct game_state;
typedef struct s_game_memory{

    memory_area *main_memory;
	b32 dll_reloaded;
    void *game_state_data;
    platform_api *platformApi;
	game_ui *ui;

    f32 target_ms;

		f32 elapsed_ms;
		f32 mc_per_frame;
		u32 cycles_elapsed;

}s_game_memory;



#define GAMEPROGRESS(fname) fname(float dt, s_game_memory *area, program_input *program_input, game_renderer *game_renderer)
typedef GAMEPROGRESS(GameUpdateAndRender); //For function pointers
GAMEPROGRESS(GameUpdateAndRenderStub)
{
    return 0;
}
