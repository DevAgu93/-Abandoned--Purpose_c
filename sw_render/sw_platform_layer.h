
typedef struct s_program_input{
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
typedef struct s_program_main_memory{

    memory_area *main_memory;
	b32 dll_reloaded;
    void *program_state;
    platform_api *platformApi;
	game_ui *ui;

    f32 target_ms;
	f32 elapsed_ms;
	f32 mc_per_frame;

}s_program_main_memory;



#define program_PROGRESS(fname) fname(float dt, s_program_main_memory *area, program_input *program_input, game_renderer *game_renderer)
typedef program_PROGRESS(program_update_and_render); //For function pointers
program_PROGRESS(program_update_and_render_stub)
{
    return 0;
}
