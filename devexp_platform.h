
typedef struct s_gprogram_memory{

    memory_area *main_area;
    platform_api *platform;
    void *program;
	game_ui *ui;

    f32 target_ms;
	f32 elapsed_ms;
	memory_area log_area;

}s_gprogram_memory;

typedef struct s_editor_input{

	union{
		struct _editor_input_buttons{
			input_button up;
			input_button down;
			input_button left;
			input_button right;
			input_button q;

			input_button e;
			input_button h;
			union{
				struct{
					input_button z;
					input_button x;
				};
				struct{
					input_button jump;
					input_button attack;
				};
			};
			input_button c;

			input_button w;
			input_button a;
			input_button s;
			input_button d;
			input_button y;

			input_button v;
			input_button r;
			input_button p;
			input_button n;
			input_button esc;

			input_button del;
			input_button b;
		};
		input_button buttons[sizeof(struct _editor_input_buttons) / sizeof(input_button)];
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
	b8 number_keys[10];
}editor_input;

#define GUPDATE_RENDER(name) \
	name(float dt, s_gprogram_memory *program_memory, editor_input *input, game_renderer *game_renderer)
typedef GUPDATE_RENDER(gupdate_render);
GUPDATE_RENDER(gupdate_render_void)
{
	return(0);
}
