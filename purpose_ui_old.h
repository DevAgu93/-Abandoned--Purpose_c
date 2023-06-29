#define ui_MAXTITLELENGTH 64

typedef enum{
	ui_command_type_button,
	ui_command_type_imagebutton,
	ui_command_type_slider,
	ui_command_type_drag,
	ui_command_type_checkbox,
	ui_command_type_combobox,
	ui_command_type_text,
	ui_command_type_childpanel,
	ui_command_type_endchildpanel,
	ui_command_type_drop_down,
	ui_command_type_selectable,
	ui_command_type_updown,
	ui_command_type_image,
	ui_command_type_tooltip,
	ui_command_type_inputtext,
	ui_command_type_tab_group,
	ui_command_type_tab,

	ui_command_type_console_log,
	
	ui_command_type_selectable_grid,
	ui_command_type_selectable_image,
	ui_command_type_timeline,
	ui_command_type_explorer,
	ui_command_type_tileset,
	ui_command_type_selectable_directions,
	ui_command_type_resource_explorer,

	ui_command_type_get_frame_size,

    ui_command_type_offsetcommands,
	ui_command_type_pushcursorposition,
	ui_command_type_popcursor,
	ui_command_type_keeplinepush,
	ui_command_type_keeplinepop,
	ui_command_type_keepcursorpush,
	ui_command_type_keepcursorpop,
	ui_command_type_separator,
	ui_command_type_setelementsize,
	ui_command_type_setdisable,
    ui_command_type_next_line_set,
	ui_command_type_same_line,
	ui_command_type_group_set,

    ui_command_type_set_text_color,
    ui_command_last_element_read,

}ui_command_type;

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
}ui_node_flags;

typedef enum{
	size_null,
	size_specified,
	size_text,
	size_parent,
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
	ui_explorer_flags_close_on_complete      = 0x01,
	ui_explorer_flags_copy_selected_file_name = 0x02
}ui_explorer_flags;

#define ui_explorer_process_default_load (ui_explorer_process_select_file)
typedef enum{
	ui_explorer_process_type_file   = 0x01,
	ui_explorer_process_select_any  = 0x02,
	ui_explorer_process_select_file = 0x04,
	ui_explorer_process_select_directory  = 0x08,
}ui_explorer_process_type;

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

#define ui_interaction_any (ui_interaction_mouse_left | ui_interaction_mouse_right | ui_interaction_mouse_middle)
typedef enum{
	ui_interaction_mouse_left   = 0x01,
	ui_interaction_mouse_right  = 0x02,
	ui_interaction_double_click = 0x04,
	ui_interaction_mouse_middle = 0x08,
	ui_interaction_forced       = 0x10
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
	ui_panel_flags_tooltip         = 0x40,
	ui_panel_flags_ignore_focus    = 0x80,
	ui_panel_flags_close           = 0x100,
	ui_panel_flags_minimize        = 0x200,
	ui_panel_flags_borders         = 0x400,
	ui_panel_flags_size_to_content = 0x800,
	ui_panel_flags_invisible       = 0x1000,
	ui_panel_flags_focus_when_opened = 0x2000,

}ui_panel_flags;
#define ui_panel_flags_NoInteraction (ui_panel_flags_tooltip)

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
typedef struct{



   vec4 textColor;
   vec4 frontPanelColor;
   vec4 frame_background_color;

   vec4 frameBorderColor;
   vec4 titleColor;

   vec4 button_disabled_color;
   vec4 button_normal_color;
   vec4	button_hot_color;
   vec4	button_interacting_color;

   vec4 scrollbarColor;
   vec4	scrollbarHotColor;
   vec4	scrollbarInteractingColor;
   vec4	scrollbarBackColor;

}ui_theme;

typedef struct{
    u32 value;
}ui_id;

typedef struct{
	ui_node_size_type type;
	f32 percent;
	f32 amount;
}ui_node_size;


typedef struct s_ui_node{
	//first child
	struct s_ui_node *first;
	struct s_ui_node *last;
	//next sibling.
	struct s_ui_node *next;
	struct s_ui_node *prev;
	struct s_ui_node *parent;

	ui_node_flags flags;
	i32 layout_position_x;
	i32 layout_position_y;
	union{
		u32 size[2];
		struct{
			u32 size_x;
			u32 size_y;
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
	rectangle32s region;
	ui_id id;
	u8 *data;
}ui_node;

static inline ui_id
ui_generate_id(u8 *string)
{
    ui_id result = {0};
    result.value = string_kinda_hash(string);
    return(result);
}

static inline ui_id
ui_id_from_stringf(u8 *string, ...)
{
   u8 buffer[256] = {0};
   va_list args;
   va_start_m(args, string);
   FormatTextList(buffer, sizeof(buffer), string, args);
   va_end_m(args);

   ui_id result = ui_generate_id(buffer);
   return(result);
}

static inline ui_id
ui_id_from_number(u32 v)
{
    ui_id result = {0};
    result.value = v; 
	return(result);
}

struct ui_panel;
typedef struct{
	ui_command_type type;


	uint8 *label;
	void *value;

	union{

		u32 persistent_element_array_index;

		struct{
			f32 scrollVerticalValue;
			vec2 totalContentSize;
			vec2 frameSize;
			vec2 scrollSize;
		}scroll;

		struct{
			u32 selectable_active;

			f32 buttonW;
			f32 buttonH;

			union{

			    	struct{
						vec2 uv0;
						vec2 uv1;
						vec2 uv2;
						vec2 uv3;
			    	};
			};
			render_texture *texture;
		}image_button;

        struct{
			struct ui_panel *panel;
        }child_panel;

		struct{
			f32 regionW;
			f32 regionH;
			struct ui_panel *panel;
		}tooltip;

		struct{
			u32 transition;
			vec2 size;
			struct ui_panel *panel;
		}drop_down;

		struct{
			u32 length;
			u32 wrap;
			vec4 color;
		}text;

		struct{
			f32 x;
			f32 y;
			ui_pushcursor_flags flags;
		}layout;

		struct{
		    u16 selected;
			u16 borders;
		}selection;

		struct{
		    f32 distance;
		}separator;

		struct{
			//u16 increment;
			//u16 decrement;
			//u32 valueUpdated;
			ui_id incrementId;
			ui_id decrementId;
			ui_id input_id;

			ui_input_text_flags textInputFlags;

			ui_value_type valueType;

			union{
				u32 value_u32;
				u16 value_u16;
				f32 value_f32;
				i32 value_i32;
			};
		}updown;

		struct{
			render_texture *source;
			f32 w;
			f32 h;
		}image;

		struct{
			u32 tileDisplacementX;
			u32 tileDisplacementY;
			f32 scale;
			f32 tileGridSize;
			f32 sizeX;
			f32 sizeY;
			vec2 *selectedP;
		}grid;

		struct{
			ui_value_type valueType;
			void *value;
			u8 *format;
		}slider;

		struct{
			union{
			  u8 *dest;
			  u16 value_u16;
			  u32 value_u32;
			  f32 value_f32;
			};

            u32 valueUpdated;
			u32 textLimit;
            ui_value_type type;
		}input;

		struct{
			u16 push;
			ui_size_options sizeOption;
			f32 width;
			f32 height;
		}elementsize;

		struct{
			u32 wrap;
		}keepline;

		struct{
			u16 condition;
			u16 pop;
			u32 disable_stack_index;
		}disable;

		struct{
			u32 push;
		}next_line;
		struct{
			vec4 color;
		}text_color;

		struct{
			struct ui_panel *panel;
		}tab;

		struct{
			u8 *title;
		}selectable_tab;

		struct{
			u32 directions;
			u32 selected_direction;
		}selectable_directions;

		struct{
			u32 remaining;
			vec2 size;
		}get_frame_size;

		struct{
			u32 begin;
		}layout_group;


	};
    ui_id id;

}ui_element;

typedef struct{
	ui_command_type type;
	u32 persistent_element_array_index;
}ui_element_persistent_index;

#define ui_explorer_SelectedFileName(ui) (ui->explorer->currentDirectoryFile[ui->explorer->selectedFileIndex].path_and_name)
typedef struct s_ui_explorer{
	//Might remove
	ui_explorer_type type;

	//Behaviour flags
	ui_explorer_flags flags;
	//process 
	ui_explorer_process_type processType;

	u8 processTitle[ui_MAXTITLELENGTH];

	union {
		u8 bools[8];
		struct{
			u8 started_process;
			u8 closed;
			u8 okayPressed;
			u8 cancelPressed;

			u8 files_focused;
			u8 fileGotSelected;
			u8 currentSelectedFileIsPack;
			u8 lastProcessCompleted;
		};
	};

	struct ui_panel *panel;

	u32 processId;

	u16 currentDirectoryFileCount;
	u16 updatePathFiles;

	platform_api *platform;

	u8 searchPattern[24];
	u8 explorerFileName[128];

    u16 maximumDirectoryLength;
	u16 pathNameLength;
    u8 currentDirectory[256];

    u8 selectedFilePathAndName[256];

	u16 selectedFileIndex;
    platform_file_info_details currentDirectoryFiles[164]; 

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

	ui_command_type type;

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
	bool32 notInitialized;
	bool32 close_button;

	ui_node *commands_base;
	ui_node *commands_offset;

	u16 scrollInteracting;
	u16 scrollInteracted;

	u16 notVisible;
	u16 closed;
	u32 was_closed;

    vec2 p;
    vec2 sz;
	vec2 totalContentSize;
	vec4 frameSpace;

	real32 cornerOffsetX;
	real32 cornerOffsetY;

	real32 scroll_v;
	real32 scroll_h;

	struct ui_panel *parentPanel;
	struct s_ui_node *root_node;
	ui_panel_flags flags;
	ui_id id;
	//ui_layout layout;
}ui_panel;


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

    render_commands *renderCommands;
	vec3 reserved_vec;

	memory_area *area;

	union
	{
		u32 bools[5];
		struct{
			u8 mouseOverPanel;
			u8 mouseDownL;
			u8 mousePressedL;
			u8 mouseClickedL;

			u8 keepInteraction;
			u8 keepProcessingHotElements;
			u8 activeDropDown;
	        u8 panelOverflow;

	        u8 initializePanelClosed;
	        u8 initializePanelMinimized;
	        u8 initialized;
           	u8 switchPanelClose;

           	u8 forcePanelFocus;
           	u8 setPanelPosition;
           	u8 setPanelSize;
			u8 input_text_focused;

	        u8 input_text_entered;
			u8 ignore_interactions;
			u8 input_text_got_double_or_tripple_clicked;
			u8 allocated_display_buffer;
		};
	};

	u32 display_buffer_index;
	u32 reservedIdValue;
	ui_theme theme;
//command buffer

	u16 persistentElementsMax;
	u16 persistentElementsCount;
	ui_element_persistent *persistentElements;

//layouts to place elements
	u16 layoutAmountPushed;
	u16 layoutStackCount;
	u32 layoutAmountBeyondStack;
	ui_layout *currentLayout; 
	ui_layout *layoutStack;
//element panels
	u32 forced_panel_focus;
	struct ui_panel *current_panel;
	struct ui_panel *focused_panel;
	struct ui_panel *panel_hot;

	u32 panel_stack_count;
	u16 panel_last_avadible_slot;
	u16 panel_push_amount;
	u16 *panel_stack_order;
	struct ui_panel *panel_stack;

	u8* reserved_space;
	u32 reserved_space_used;
	u32 reserved_space_total;
	//game_input *input;
	ui_element_persistent *last_persistent_element;

//panel settings for current and initialization
	f32 next_panel_x;
	f32 next_panel_y;
	f32 next_panel_w;
	f32 next_panel_h;

	u16 pushed_disable_count;
	u16 pushed_disable_max;
	u16 pushed_disable_total_count;
	u16 pushed_disable_true_count;
	u8 *disable_stack;
	//ui_element *interactedElement;
	u32 element_transition;

	u16 pushed_element_size;
	ui_size_options pushed_element_size_option;
	f32 pushed_element_w;
	f32 pushed_element_h;

	ui_id element_forced_interacting;
	ui_id element_forced_last_hot;
	ui_id element_forced_hot;

    ui_id element_hot;
    ui_id element_interacting;
    ui_id element_last_interact;
    ui_id element_last_hot;

	ui_interaction_flags interacting_flags;
	ui_interaction_flags interacted_flags;
	ui_interaction_flags forced_interacting_flags;
	ui_id lastElementPushed;

	ui_id last_interacted_drop_down;








	ui_node_size next_nodes_size_x;
	ui_node_size next_nodes_size_y;

	ui_node *commands_base;
	ui_node *commands_offset;
	u32 commandsTotalSize;

	u32 nodes_count;
	u32 persistent_nodes_max;
	ui_node *persistent_nodes;

	u32 parent_stack_count;
	u32 parent_stack_max;
	ui_node **parent_stack;
	ui_node *first_node;

	ui_node *last_pushed_node;
	ui_node root_node;

	s_input_text *input_text;
	game_input *input;
	ui_explorer *explorer;

	i32 mouse_wheel;
    vec2 mouse_point;
    vec2 mouse_point_last;

    f32 font_scale;
    font_proportional fontp;

	f32 input_text_timer;
	u8 inputTextBuffer[256];

	matrix4x4 projection;

}game_ui;

#define ui_theme_default_text V4(0xff, 0xff, 0xff, 0xff)
inline ui_theme
ui_DefaultTheme()
{
	ui_theme result = {0};
	f32 alpha = 240;

    result.textColor                  = ui_theme_default_text; 
    result.frontPanelColor            = V4(0x0f, 0x0f, 0x0f, 0xDC);
    result.frame_background_color     = V4(0x07, 0x0A, 0x14, alpha);
    result.titleColor                 = V4(0x05, 0x05, 0x05, 0xf5);
    result.button_disabled_color        = V4(30, 177, 173, alpha);
    result.button_normal_color          = V4(13, 90, 200, alpha);
    result.button_hot_color             = V4(20, 120, 200, alpha); 
    result.button_interacting_color     = V4(0x0A, 0x46, 0x8C, alpha);
    result.scrollbarColor		      =	V4(83, 160, 200, 220);
    result.scrollbarHotColor	      = V4(83, 200, 246, 220);
    result.scrollbarInteractingColor  = V4(83, 210, 255, 240);
    result.scrollbarBackColor	      =	V4(0, 0, 0, 240);
    result.frameBorderColor           =	V4(0xff, 0xff, 0xff, 0x64);
    return(result);
}

//explorer functions
static void
ui_explorer_set_path(game_ui *ui, u8 *path)
{
	ui_explorer *explorer = ui->explorer;
	//Advance
	u32 path_length     = string_count(path);
	//Remove null character
	u32 current_path_length = string_count(explorer->currentDirectory);
	u32 newLength = current_path_length;
	Assert(newLength < 260);


	memory_clear(explorer->currentDirectory, current_path_length);
	string_copy(path, explorer->currentDirectory);
	
	explorer->currentDirectory[path_length - 1] = '/';
	explorer->pathNameLength = path_length + 1;
	explorer->updatePathFiles = 1;
}

inline void
ui_reset_interactions(game_ui *ui);
