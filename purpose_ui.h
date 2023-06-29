#define ui_MAXTITLELENGTH 64
#define ui_INTERACTION_LAYER_MAX 2
#define ui_interaction_layer_MAX 3
/*
   The default non-custom widget themes are:

    text color                  = white; 
    front_panel_polor            = V4(0x0f, 0x0f, 0x0f, 0xDC);
    frame_background_color     = V4(0x07, 0x0A, 0x14, alpha);
    title_color                 = V4(0x05, 0x05, 0x05, 0xf5);
    button_disabled_color        = V4(30, 177, 173, alpha);
    button_normal_color          = V4(13, 90, 200, alpha);
    button_hot_color             = V4(20, 120, 200, alpha); 
    button_interacting_color     = V4(0x0A, 0x46, 0x8C, alpha);
    scrollbar_color		      =	V4(83, 160, 200, 220);
    scrollbar_hotColor	      = V4(83, 200, 246, 220);
    scrollbar_interactingColor  = V4(83, 210, 255, 240);
    scrollbar_back_color	      =	V4(0, 0, 0, 240);
    frame_border_color           =	V4(0xff, 0xff, 0xff, 0x64);
*/

/*

   struct s_ui_main;
   struct s_ui_node;
   struct s_ui_panel;

*/
#define ui_COLOR_SCROLLBAR V4(83, 160, 200, 220)
#define ui_COLOR_F_BACKGROUND V4(0, 60, 80, 255)

typedef enum{
	node_null = 0,
	node_clickeable = 0x01,
	node_background = 0x02,
	node_border = 0x04,
	node_hover_animation = 0x08,
	node_active_animation = 0x10,
	node_text = 0x20,
	node_disabled = 0x40,
	node_clip = 0x80,
	node_overlaps_x = 0x100,
	node_overlaps_y = 0x200,
	node_scroll_x = 0x400,
	node_scroll_y = 0x800,
	node_advance_horizontally = 0x1000,
	node_text_centered = 0x2000,
	node_skip_layout_x = 0x4000,
	node_skip_layout_y = 0x8000,
	node_clickeable_overlap = 0x10000,
	node_use_extra_flags = 0x20000,
	node_readonly = 0x40000,
	node_scroll_x_skip_bounds = 0x80000,
	node_scroll_y_skip_bounds = 0x100000,
	node_skip_content_size_x = 0x200000,
	node_skip_content_size_y = 0x400000,
}ui_node_flags;
#define node_floats_x (node_skip_content_size_x | node_skip_layout_x)
#define node_floats_y (node_skip_content_size_y | node_skip_layout_y)

typedef enum{
	ui_interaction_layer_default,
	ui_interaction_layer_mid,
	ui_interaction_layer_top,
	ui_interaction_layer_top1,

	ui_interaction_layer_COUNT
}ui_interaction_layer;

typedef enum{
	uvt_u8,
	uvt_u32,
	uvt_i32,
	uvt_u16,
	uvt_i16
}ui_value_type;

typedef enum{
	size_null,
	size_specified,
	size_text,
	size_percent_of_parent,
	size_sum_of_children
}ui_node_size_type;

typedef enum{
	ui_text_input_confirm_on_enter = 0x01,
	ui_text_input_read_only = 0x02
}ui_input_text_flags;

typedef enum{
	ui_explorer_default,
	ui_explorer_Load,
	ui_explorer_Save
}ui_explorer_type;

typedef enum{
	type_rectangle,
	type_hollow_rectangle,
	type_image,
	type_line,
	type_text,
	type_triangle,
}ui_render_command_type;

#define ui_explorer_select_any (ui_explorer_select_file | ui_explorer_select_directory),
#define ui_explorer_load (ui_explorer_select_file | ui_explorer_copy_selected_file_name | ui_explorer_close_on_complete)
typedef enum{
	ui_explorer_close_on_complete = 0x01,
	ui_explorer_copy_selected_file_name = 0x02,
	ui_explorer_type_file= 0x04,
	ui_explorer_select_file = 0x10,
	ui_explorer_select_directory  = 0x08,
}ui_explorer_flags;


typedef enum{
	ui_size_Specified,
	ui_size_ToText,
}ui_size_options;

typedef enum{
	ui_scale_Stretched,
	ui_scale_PreserveRatio

}ui_scale_flags;

typedef enum{
	ui_pushcursor_set = 0x01,
	ui_pushcursor_add = 0x02,

	ui_pushcursor_screen = 0x04
}ui_pushcursor_flags;

typedef enum{
	ui_value_u16,
	ui_value_u32,
	ui_value_u64,
	ui_value_i32,
	ui_value_f32,
}ui_value_type;

#define ui_interaction_any (ui_interaction_mouse_left_down | ui_interaction_mouse_right_down | ui_interaction_mouse_middle)
typedef enum{
	ui_interaction_mouse_left_down = 0x01,
	ui_interaction_mouse_left_pressed = 0x04,
	ui_interaction_mouse_left_up = 0x08,
	ui_interaction_mouse_left_double_click = 0x10,
	ui_interaction_mouse_left_tripple_click = 0x20,
	ui_interaction_mouse_left_dragged = 0x40,

	ui_interaction_mouse_right_down = 0x80,
	ui_interaction_mouse_right_pressed = 0x100,
	ui_interaction_mouse_right_up = 0x200,
	ui_interaction_mouse_right_double_click = 0x400,
	ui_interaction_mouse_right_tripple_click = 0x800,
	ui_interaction_mouse_right_dragged = 0x1000,

	ui_interaction_mouse_middle_down = 0x2000,
	ui_interaction_mouse_middle_pressed = 0x4000,
	ui_interaction_mouse_middle_scrolled = 0x8000,
	ui_interaction_mouse_middle_dragged = 0x10000,

	ui_interaction_mouse_hover = 0x20000,
}ui_interaction_flags;

#define ui_panel_flags_front_panel (ui_panel_flags_borders | ui_panel_flags_title | ui_panel_flags_minimize | ui_panel_flags_resize | ui_panel_flags_move | ui_panel_flags_scroll_v) 
#define ui_panel_flags_default (ui_panel_flags_move | ui_panel_flags_resize)
typedef enum{
	ui_panel_flags_title           = 0x01,
	ui_panel_flags_resize          = 0x02,
	ui_panel_flags_scroll_v        = 0x04,
	ui_panel_flags_scroll_h        = 0x08,
	ui_panel_flags_move            = 0x10,
	ui_panel_flags_child           = 0x20,
	ui_panel_flags_ignore_focus    = 0x80,
	ui_panel_flags_close           = 0x100,
	ui_panel_flags_minimize        = 0x200,
	ui_panel_flags_borders         = 0x400,
	ui_panel_flags_size_to_content = 0x800,
	ui_panel_flags_invisible       = 0x1000,
	ui_panel_flags_focus_when_opened = 0x2000,
	ui_panel_flags_keep_on_front = 0x4000,
	ui_panel_flags_init_once = 0x8000,
	ui_panel_flags_ignore_layout = 0x10000,
	ui_panel_flags_close_button = 0x20000,
	ui_panel_flags_respect_layout = 0x40000,
	ui_panel_flags_init_closed = 0x80000,
	ui_panel_flags_from_overflow = 0x100000

}ui_panel_flags;

#define TITLEHEIGHT 24

#if 0

#define FP_ALPHA 240
#define PANELBORDERCOLOR    V4(255, 255, 255, FP_ALPHA)
#define FRONTPANELCOLOR     V4(13, 17, 30, FP_ALPHA)
#define CHILDPANELCOLOR     V4(07, 10, 20, FP_ALPHA)
#define TITLECOLOR          V4(13, 100, 160, FP_ALPHA)

#define BUTTONDISABLEDCOLOR V4(30, 177, 173, FP_ALPHA)
#define BUTTONNORMALCOLOR   V4(13, 90, 140, FP_ALPHA)
#define	BUTTONHOTCOLOR	    V4(20, 120, 170, FP_ALPHA) 
#define	BUTTONINTERACTINGCOLOR  V4(40, 160, 200, FP_ALPHA)

#define SCROLLBARCOLOR			  V4(83, 160, 200, 220);
#define	SCROLLBARHOTCOLOR	      V4(83, 200, 246, 220);
#define	SCROLLBARINTERACTINGCOLOR V4(83, 210, 255, 240);
#define	SCROLLBARBACKCOLOR		  V4(0, 0, 0, 240);
#endif


//the set of colors used for widget rendering.
typedef enum{
	ui_color_text,
	ui_color_background,

	ui_color_disabled,
	ui_color_hot,
	ui_color_interacting,
	ui_color_border,

	ui_color_COUNT
}ui_theme_colors;

//colors used for the pre-built widgets
typedef enum{
	ui_style_button,
	ui_style_button_bg,
	ui_style_scroll_bar,
	ui_style_scroll_bg,
}ui_style_colors;

typedef enum{
	ui_axis_x,
	ui_axis_y,

	ui_axis_COUNT,
}ui_axis;

typedef struct{
	union{
		i16 v[ui_axis_COUNT];
		struct{
			i16 x;
			i16 y;
		};
	};
}vec2_i16;

static inline vec2_i16
ui_V2(i16 x, i16 y)
{
	vec2_i16 result = {x, y};
	return(result);
}

typedef struct{
	ui_interaction_flags flags;
}ui_interaction_info;

typedef struct s_ui_render_command{
	ui_render_command_type type;
	struct s_ui_render_command *first;
	struct s_ui_render_command *next;
	union{
		struct{
			f32 x0, y0, x1, y1;
		};
		struct{
			vec2_i16 p0v;
			vec2_i16 p1v;
		};
	};
	vec2_i16 p2v;

	vec4 color;
	union
	{
		u32 texture_handle;
		f32 border_thickness;
		u8 *text;
		render_texture *texture;
	};

	vec2 p0;
	vec2 p1;
	vec2 p2;
	vec2 p3;

	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;
}ui_render_command;

typedef struct{
	ui_render_command_type type;
	vec2 p;
	vec4 color;
}ui_render_command_text;

typedef struct{
	ui_render_command_type type;
	vec2 p0;
	vec2 p1;
	vec2 p2;
	vec2 p3;
	vec4 color;
}ui_render_command_rectangle;

typedef struct{
	union{
		u32 i[ui_color_COUNT];
		u32 selected_colors_indices[ui_color_COUNT];
	};
}ui_theme_indices;

typedef struct{
	union{
		vec4 colors[ui_color_COUNT];
		struct{
			vec4 text_color;
			vec4 background_color;

			vec4 disabled_color;
			vec4 hot_color;
			vec4 interacting_color;
			vec4 border_color;
			vec4 border_unfocus_color;
		};
	};
}ui_theme;

typedef struct{
    u32 value0;
	u32 value1;
}ui_id;

typedef struct{
	ui_node_size_type type;
	f32 size_strictness;
	f32 amount;
}ui_node_size;


typedef struct s_ui_node{
	u32 last_touched_frame;
	//first child
	struct s_ui_node *first;
	struct s_ui_node *last;
	//next sibling.
	struct s_ui_node *next;
	struct s_ui_node *prev;
	struct s_ui_node *parent;

	ui_node_flags flags;
	union{
		i16 added_position[ui_axis_COUNT];
		struct{
			i16 added_x;
			i16 added_y;
		};
	};
	vec2_i16 layout_position;
	union{
		u16 size[2];
		struct{
			u16 size_x;
			u16 size_y;
		};
	};
	union{
		//for x and y
		ui_node_size size_types[2];
		struct{
			ui_node_size size_type_x;
			ui_node_size size_type_y;
		};
	};
	union{
		i16 padding[2];
		struct{
			i16 padding_x;
			i16 padding_y;
		};
	};
	b8 scroll_hits_bounds[2];
	//used to calculate the total scroll value and limit
	i16 content_size[2];
	rectangle32s region;
	u32 layout_axis;
	ui_id id;


	//persistent_data
	union{
		f32 scroll[ui_axis_COUNT];
		struct{
			f32 scroll_x;
			f32 scroll_y;
		};
	};
	union{
		f32 target_scroll[ui_axis_COUNT];
		struct{
			f32 target_scroll_x;
			f32 target_scroll_y;
		};
	};
	union{
		f32 dt_scroll[ui_axis_COUNT];
		struct{
			f32 dt_scroll_x;
			f32 dt_scroll_y;
		};
	};

	union{
		struct{
			u16 uvs_corner_index;
			i8 zoom;
			i8 zoom_target;
		};
		struct{
			u16 tab_count;
			u16 tab_index;
		};
		struct{
			b32 tree_opened;
		};
	};
	//reset on every frame
	u32 interaction_index;

	//for specifics
	union
	{
		b32 popup_opened;
		struct{
			b16 context_menu_active;
			b16 context_menu_got_opened;
		};
		struct
		{
			u16 uvs_group_selection_index;
			b8 uvs_group_down;
			b8 grid_display;
		};
	};
	b16 selection_region_down;
	b16 selection_region_hot;

	//f32 scroll_x;
	//f32 scroll_y;

	f32 hot_time;
	f32 interacted_time;
	ui_theme_indices color_indices;

	u8 *string;
	u8 *display_string;
	ui_render_command *first_render_command;
	ui_render_command *last_render_command;
}ui_node;

static u32
ui_explorer_generate_id(u8 *text)
{
	u8 text_to_low[256] = {0};
	string_copy(text, text_to_low);
    string_to_low(text_to_low);
	u32 new_id = string_kinda_hash(text_to_low);

	return(new_id);
}
typedef struct{
	u32 is_directory;
	//name id depending on path and name together
	u32 id;
	//size and date
	u32 size;
	platform_file_time date;
	u8 name[64];
	u8 path[256];
	u8 path_and_name[256];
}ui_explorer_file_attributes;
#define ui_explorer_SelectedFileName(ui) (ui->explorer->currentDirectoryFile[ui->explorer->selected_file_index].path_and_name)
typedef struct s_ui_explorer{
	//Might remove
	ui_explorer_type type;

	//Behaviour flags
	ui_explorer_flags flags;

	u8 process_name[ui_MAXTITLELENGTH];

	union {
		u32 bools[3];
		struct{
			b8 started_process;
			b8 closed;
			b8 okay_pressed;
			b8 cancel_pressed;

			b8 files_focused;
			b8 valid_file_focused;
			b8 current_selected_file_is_pack;
			b8 last_process_completed;
		};
	};

	struct ui_panel *panel;

	u32 process_id;

	u16 directory_file_count;
	bool16 update_path_files;

	platform_api *platform;

	u8 search_pattern[24];
	u8 process_file_name[128];

    u16 max_directory_length;
	u16 path_length;
    u8 directory_name[256];
	u8 full_process_path_and_name[256];

	ui_explorer_file_attributes *selected_file;
	ui_explorer_file_attributes current_directory_files[164]; 

}ui_explorer;

typedef struct{

	u16 trackIndex;
	u16 selected;
	f32 timeStart;
	f32 timeDuration;
	ui_id id;

}ui_timeline_clip;

typedef struct{

	u32 clipIndex;
	u32 selected;
	u8 *label;

	ui_id id;

}ui_timeline_clip_group_key;
//Timeline 
typedef struct{

	u8 *label;

}ui_timeline_track;

typedef struct{
	u32 selected;
	u32 frame_start;
	u16 keyframe_count;
	u16 keyframe_rendered_count;

	ui_id id;
}ui_timeline_frame_group;

typedef struct{
	u32 selected;
	u32 frame_duration;

	u16 frame_group_index;
	u16 clipIndex;

	u8 *label;
	ui_id id;
	u32 type;
}ui_timeline_frame_group_key;

typedef struct{

	//ui_command_type type;

	union{
		u8 bools[4];
		struct{
			u8 reproducing;
			u8 reproduce_interacted;
			u8 resizer_interacting;
			u8 timelineInteracting;
		};
	};

	

	f32 scrollVertical;
	f32 scroll_h;
	u32 lines_per_ms;

	f32 time_at;
	f32 time_total;

	u32 clip_max;
	u16 clip_count;
	u16 clip_selected_index;

	u32 loop_interacting;
	u16 loop_visible;
	u16 loop;
	f32 loop_start;
	f32 loop_end;

	u32 trackMax;
	u16 trackCount;

	u8 *data;
	ui_timeline_track *tracks;
	ui_timeline_clip *clips;

	u16 frame_group_count;
	u16 frame_group_max; 
	ui_timeline_frame_group *frame_groups;

	u16 frame_group_key_count;
	u16 frame_group_key_max; 
	ui_timeline_frame_group_key *frame_group_keys;

	u32 dataOffset;

	u32 clip_group_key_count;
	u16 clip_group_key_selected;
	u16 clip_group_key_got_selected;

	ui_id id;

}ui_timeline;

typedef struct{
	u16 frame_x;
	u16 frame_y;
	u16 frame_w;
	u16 frame_h;
}ui_tileset_tile;

typedef struct{
	u32 active;
	u8 *title;

	u8 *commands_base;
	u8 *commands_offset;
}ui_tab_group_tab;


typedef struct ui_image_selection ui_image_selection;
typedef struct ui_tab_group ui_tab_group;

typedef struct ui_element_persistent{

	u32 index;
      union{
		  struct{
			  u32 interacted;
			  u32 selected_file_index;
			  u32 hashed_id;

			  struct ppse_resources_file_and_folders *resources;
		  }resource_tree_explorer;

		struct ui_image_selection{

			render_texture *texture;

			u32 uvsUpdated;

			u32 selectedPixelX;
			u32 selectedPixelY;
			u32 selectedPixelW;
			u32 selectedPixelH;

			vec2 uv0_scaled;
			vec2 uv1_scaled;
			vec2 uv2_scaled;
			vec2 uv3_scaled;

			//u16 useFixedSize;
			u32 mouseWasHolding;
			f32 zoom;

			i32 clip_offset_x;
			i32 clip_offset_y;

			i32 mouseHoldX;
			i32 mouseHoldY;

			u32 displayGrid;
			i16 gridX;
			i16 gridY;
			i16 gridW;
			i16 gridH;

			u16 tileSize_w;
			u16 tileSize_h;

			u32 selectable_group_selected;
			u16 selectable_group_max;
			u16 selectable_group_count;
			struct ui_image_selection_group{

				ui_id id;
				u8 *label;
				u32 selection_type_uv;

				union{
					struct{

			            vec2 uv0_scaled;
			            vec2 uv1_scaled;
			            vec2 uv2_scaled;
			            vec2 uv3_scaled;
					};
					struct{
						u32 frame_x;
						u32 frame_y;
						u32 frame_w;
						u32 frame_h;
					};
				};

			}*selectable_group;

			
		}selectable_image;

		struct{
			u32 selectedTile;
			u16 w;
			u16 h;

			u16 tileCapacity;
			u16 tileCount;
			u32 tileSize;
			render_texture texture;
			ui_tileset_tile *tiles;


		}tileset;


	    struct{
	    	u8 *buffer;
	    	s_game_console *gameConsole;
	    	f32 scroll;
	    }console;


        struct ui_tab_group{
        	u32 active_tab_index;

        	u16 tabs_count;
			u16 tabs_max;
			ui_tab_group_tab *tabs;

			u8 *commands_base;
			u8 *commands_offset;
        }tab_group;

		ui_timeline timeline;

	};

	struct ui_element_persistent *previous;
    ui_id id;

}ui_element_persistent;


typedef enum{
	ui_layout_group = 1,
}ui_layout_flags;

struct ui_layout;
typedef struct ui_layout{

	struct ui_layout *previousLayout;
	u32 flags;
	
	u16 keepLine; 
	u16 keepCursor;

	u16 wrapLine;
	u16 keep_delta_x;
	u32 same_line;

	f32 nextYDelta;
	f32 nextXDelta;

	f32 last_x_delta;
	f32 last_y_delta;
	f32 total_advance_x;
	f32 total_advance_y;

	//settings
    f32 cursorX;
    f32 cursorY;
	f32 cursor_x_last;
	f32 cursor_y_last;


    f32 cornerX;
    f32 cornerY;
	f32 cornerEndX;
	f32 cornerEndY;

	f32 spacingX;
	f32 spacingY;
	/*
	   f32 lastSpaceUsedX;
	   f32 lastSpaceUsedY;
	   */

}ui_layout;



typedef struct ui_panel{
	uint8 *title;
	u16 notVisible;
	u16 closed;

    vec2 p;
    vec2 sz;

	bool32 frame_touched;
	u16 z_order;
	u16 call_order;
	struct ui_panel *next;
	struct ui_panel *prev;
	struct s_ui_node *root_node;
	ui_panel_flags flags;
	ui_id id;
	//ui_layout layout;
}ui_panel;

typedef struct ui_popup{
	u32 last_touched_frame;
	b32 active;
	i16 x;
	i16 y;
	ui_id id;

	struct ui_popup *next;
	struct ui_popup *prev;
}ui_popup;


typedef struct{
	uint8* commands_base;
	uint8* commands_offset;
}ui_commands;

typedef struct{
	u16 panelN;
	u16 widgetN;
	ui_commands currentCommands;
}ui_op_precedence;

typedef struct{
	platform_api *platform;
	memory_area *ui_area;

}ui_frame_data;

typedef struct{
	memory_area ui_area;
	//for now only supports the custom format.
//	font_proportional *ui_font;
	bool32 display_buffer_allocated;
	u32 display_buffer_index;
}ui_per_frame_settings;

typedef struct{
	vec4 color;
	u32 previous;
}ui_color_stack_slot;

typedef struct{
	u32 type;
	f32 *value_to_set;
	f32 set_value;
}ui_command_node;

typedef struct{
	u32 node_transition;
	union
	{
		ui_id interaction_ids[5];
		struct{
			ui_id node_hot;
			ui_id node_interacting;
			ui_id node_last_interact;
			ui_id node_last_hot;
			ui_id node_last_clicked;
		};
	};
	ui_interaction_flags interacting_flags;
	ui_interaction_flags interacted_flags;

}ui_id_interactions;

typedef struct ui_id_stack_slot{
	u32 id;
	struct ui_id_stack_slot *next;
}ui_id_stack_slot;

typedef struct ui_input{
	s_input_text input_text;

	//keyboard
	union{

		input_button buttons[30];
		struct{
			input_button up;
			input_button down;
			input_button left;
			input_button right;
			input_button q;
			input_button e;
			input_button h;
			input_button z;
			input_button x;
			input_button c;
			input_button w;
			input_button a;
			input_button s;
			input_button d;
			input_button y;
			input_button v;
			input_button r;
			input_button esc;
		};
	};

	union
	{
		platform_mouse mouse;
		struct{
			f32 mouse_clip_x;
			f32 mouse_clip_y;
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
}ui_input;

typedef struct s_ui_main{

	memory_area area;
	void *window_handle;

	render_commands *renderCommands;
	ui_input old_input;
	ui_input input;

	union
	{
		u32 bools[3];
		struct{
			b8 initialized;
			b8 mouse_l_down;
			b8 mouse_l_pressed;
			b8 mouse_l_up;

			b8 keep_interaction;
			b8 next_node_readonly;
			b8 process_hot_nodes;
			b8 input_text_entered;

			b8 input_text_focused;
			b8 ignore_interactions;
			b8 input_text_got_double_or_tripple_clicked;
			b8 allocated_display_buffer;
		};
	};

	//persistent arrays
	//if this gets to 0. keep_interaction becomes "false"
	u16 next_node_readonly_countdown;
	u16 keep_interaction_countdown;
	b16 keep_input_text_interaction;
	u8 input_text_interaction_countdown;
	u8 input_text_interaction_transition;

	u32 display_buffer_index;
	u16 back_buffer_width;
	u16 back_buffer_height;
	//for the 0 index default colors
	ui_theme theme;
	//command buffer
	//element panels
	//current processing panel for nodes
	struct ui_panel *current_panel;
	//focused panel for those who change their z order
	struct ui_panel *focused_panel;
	struct ui_panel *panel_hot;
	struct ui_panel *first_panel_closed;

	//in the stack
	u32 panel_last_avadible_slot;
	//amount of panels pushed on a frame, also used
	//to check for overflow
	u32 panel_stack_generated;
	u16 panel_stack_count;
	u16 panel_stack_max;
	struct ui_panel *panel_stack;
	//re-sorted every frame
	ui_panel **panel_stack_order_ptr;
	//game_input *input;
	u16 popup_array_count;
	u16 popup_array_max;
	u16 opened_popups_count;
	ui_popup *popup_array;
	ui_popup *first_open_popup;
	ui_id last_looked_popup;
	b16 last_looked_popup_was_opened;
	u32 root_order;

	u16 pushed_disable_count;
	u16 pushed_disable_max;
	u16 pushed_disable_total_count;
	b16 next_node_disabled;
	bool8 *disable_stack;

	//ui_element *interactedElement;

	ui_id input_text_interacting_id;
	ui_interaction_layer interaction_layer;
	ui_id_interactions interactions[ui_interaction_layer_COUNT];


	//per frame info
	f32 frame_dt;
	u32 current_frame;

	//next node info
	ui_axis current_layout_axis;
	ui_node_flags next_node_extra_flags;

	//space reserved after all allocations.
	u8* reserved_space;
	u32 reserved_space_used;
	u32 reserved_space_total;

	//u16 node_flags_stack_count;
	//u16 node_flags_stack_max;
	//ui_node_flags *

	//basic style color
	u16 theme_colors_max[ui_color_COUNT];
	u16 theme_colors_counts[ui_color_COUNT];
	u16 theme_colors_indices[ui_color_COUNT];
	union{
		ui_color_stack_slot *theme_colors[ui_color_COUNT];
		struct{
			ui_color_stack_slot *text_color_stack;
			ui_color_stack_slot *background_color_stack;

			ui_color_stack_slot *hot_color_stack;
			ui_color_stack_slot *interacting_color_stack;
			ui_color_stack_slot *border_color_stack;
		};
	};
	ui_theme_indices color_indices;

	//size stacks
	u16 node_size_stacks_counts[ui_axis_COUNT];
	u16 node_size_stacks_max[ui_axis_COUNT];
	union
	{
		ui_node_size *node_size_stacks[ui_axis_COUNT];
		struct{
			ui_node_size *nodes_size_stack_x;
			ui_node_size *nodes_size_stack_y;
		};
	};

	union{
		ui_node_size next_node_sizes[ui_axis_COUNT];
		struct{
			ui_node_size next_nodes_size_x;
			ui_node_size next_nodes_size_y;
		};
	};


	u16 nodes_count;
	u16 persistent_nodes_max;
	ui_node *persistent_nodes;
	ui_node *first_free_node;

	//Current active "parent" stack. 0 is reserved for the "root" node.
	u16 parent_stack_count;
	u16 parent_stack_max;
	ui_node **parent_stack;

	//where all starts for hierarchies
	ui_node root_node;
	//"closed" hierarchies go here and get ignored
	ui_node void_node;

	//per-node rendering commands
	u16 render_commands_count;
	u16 render_commands_max;
	ui_render_command *render_commands_buffer;
	//state transform commands
	u16 command_node_count;
	u16 command_node_max;
	ui_command_node *command_nodes;

	s_input_text *input_text;
	//require platform functionality
	ui_explorer *explorer;

	f32 mouse_hold_dt;
	i32 mouse_wheel;
	vec2 mouse_point;
	vec2 mouse_point_last;
	vec2 mouse_point_hold;

	//current font
	f32 font_scale;
	font_proportional fontp;


	u32 id_stack_max;
	u32 id_stack_count;
	ui_id_stack_slot *id_stack;
	ui_id_stack_slot *id_stack_last;

	//used for cursor "blinking"
	f32 input_text_timer;
	u16 input_text_buffer_using;
	u16 input_text_buffer_size;
	u8 input_text_buffer[256];

	matrix4x4 projection;

	u16 row_stack_count;
	u16 row_stack_max;
	ui_node **row_stack;
	u16 column_stack_count;
	u16 column_stack_max;
	ui_node **column_stack;

}game_ui;














#define ui_theme_default_text V4(0xff, 0xff, 0xff, 0xff)
inline ui_theme
ui_default_theme()
{
	ui_theme result = {0};
	f32 alpha = 255;

    result.text_color = ui_theme_default_text; 
    //result.background_color = V4(0x07, 0x0A, 0x14, alpha);
    //result.background_color = V4(13, 0x00, 11, 230);
    result.background_color = V4(15, 0x00, 5, 230);
    result.disabled_color = V4(30, 177, 173, alpha);
    //result.hot_color = V4(20, 120, 200, alpha); 
    result.hot_color = V4(0, 200, 200, alpha); 
    //result.interacting_color = V4(0x0A, 0x46, 0x8C, alpha);
    result.interacting_color = V4(9, 120, 140, alpha);
    //result.border_color =V4(0xff, 0xff, 0xff, 0x64);
    //result.border_color =V4(0, 100, 100, 255);
    result.border_color =V4(140, 0, 100, 255);
    return(result);
}

#define ui_SCROLL_BAR_COLOR V4(0x53, 160, 200, 240)
#define ui_BUTTON_NORMAL_COLOR V4(13, 90, 200, 240)

static inline game_ui * 
ui_initialize(
		void *window_handle,
		u8 *mem,
		memory_size mem_size,
		stream_data *info_stream)
{
	game_ui *ui = 0;
	if(mem_size >= MEGABYTES(1))
	{
		memory_clear(mem, mem_size);
		ui = (game_ui *)mem;
		ui->area = memory_area_create(
				mem_size - sizeof(game_ui),
				(u8 *)mem + sizeof(game_ui));
		ui->window_handle = window_handle;
	}
	else
	{
		stream_pushf(
				info_stream,
				"-- ERROR while allocating game_ui!. The minimum allocation size should be at least 1 megabyte (%u), got instead %u",
				MEGABYTES(1),
				mem_size);
	}
	return(ui);
}
