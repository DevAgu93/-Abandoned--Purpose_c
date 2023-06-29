//
// __Input text
//

//44 in total
#define KEY_CODE_BACKSPACE 0x08
#define KEY_CODE_ENTER 0x0D
#define KEY_CODE_SPACE 32
#define KEY_CODE_ESC 0x1B

#define KEY_CODE_LEFT  0x25
#define KEY_CODE_UP    0x26
#define KEY_CODE_RIGHT 0x27
#define KEY_CODE_DOWN  0x28

#define KEY_CODE_0 0x30
#define KEY_CODE_1 0x31
#define KEY_CODE_2 0x32
#define KEY_CODE_3 0x33

#define KEY_CODE_4 0x34
#define KEY_CODE_5 0x35
#define KEY_CODE_6 0x36
#define KEY_CODE_7 0x37
#define KEY_CODE_8 0x38
#define KEY_CODE_9 0x39

#define KEY_CODE_A 0x41
#define KEY_CODE_B 0x42
#define KEY_CODE_C 0x43
#define KEY_CODE_D 0x44

#define KEY_CODE_E 0x45
#define KEY_CODE_F 0x46
#define KEY_CODE_G 0x47
#define KEY_CODE_H 0x48

#define KEY_CODE_I 0x49
#define KEY_CODE_J 0x4A
#define KEY_CODE_K 0x4B
#define KEY_CODE_L 0x4C

#define KEY_CODE_M 0x4D
#define KEY_CODE_N 0x4E
#define KEY_CODE_O 0x4F
#define KEY_CODE_P 0x50

#define KEY_CODE_Q 0x51
#define KEY_CODE_R 0x52
#define KEY_CODE_S 0x53
#define KEY_CODE_T 0x54

#define KEY_CODE_U 0x55
#define KEY_CODE_V 0x56
#define KEY_CODE_W 0x57
#define KEY_CODE_X 0x58

#define KEY_CODE_Y 0x59
#define KEY_CODE_Z 0x5A

typedef struct{
	u16 is_down;
	u16 was_down;
}input_keystate;

#define input_text_BUFFERSIZE 1024
typedef struct s_input_text{
	uint8 key;
	uint8 lastKey;
	u32 current_key_code;
	bool32 target_buffer_modified;

	uint16 cursor_position_l;
	uint16 cursor_position_r;
	//uint16 cursorWidth;

	u8 *target_buffer;
	u32 *target_buffer_count;
	u32 target_buffer_max;
	//;Enums?
	u16 entered;
	u16 cursor_moved;

	u16 focused;
	u16 got_focus;

	u32 clipboardSizeUsed;

	input_keystate key_state;

	u32 last_key_count;
	u32 key_count;

	b16 shift_l;
	b8 ctrl_l;
	b8 enter;

	uint8 clipboardBuffer[128];
}s_input_text;


//
// __Input text 
//

typedef struct {
   u16 was_down;
   u16 transition_count;
} input_button;

typedef struct s_input_state 
{
    union {
        input_button buttons[18];
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
			input_button p;
			input_button esc;
        };
    };
} InputState;

typedef struct{
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
}platform_mouse;

#define input_MouseClipped(gameInput) V2(gameInput->mouse_clip_x, gameInput->mouse_clip_y)
#define input_Mouse(gameInput) V2(gameInput->mouse_x, gameInput->mouse_y);

typedef struct {
    void *handle;
}platform_file_handle;

typedef struct{
	void *handle;
}platform_file_search;

typedef union{
	u64 value;
	struct
	{
		u16 year; //1601- 0827
		u8 month; //1-12
		u8 day; //1-31

		u16 hour_minute; //hour: 0-60; minute: 0-60
		u16 second_ms; //sec:0-59. 6 bits; ms:0-999. 10 bits
	};

}platform_file_time;
#define platform_file_time_seconds(pft) (pft.second_ms >> 10)
#define platform_file_time_ms(pft) (pft.second_ms & 0x3ff)
#define platform_file_time_hour(pft) (pft.hour_minute >> 8)
#define platform_file_time_minute(pft) (pft.hour_minute & 0xff)
#define pft_seconds platform_file_time_seconds
#define pft_ms platform_file_time_ms
#define pft_hour platform_file_time_hour
#define pft_minute platform_file_time_minute

typedef struct{
	u8 *name;
	u32 size;
	u32 is_directory;
	platform_file_time write_time;
}platform_file_search_info;


typedef struct {
	u32 size;
	platform_file_time date;
}platform_file_min_attributes;

typedef struct{
	u32 is_directory;
	//size and date
	union
	{
	    platform_file_min_attributes info;
		struct{
			u32 size;
			platform_file_time date;
		};
	};
	u8 name[256];
	u8 path[256];
}platform_file_attributes;

typedef struct{
	u8 *contents;
	u32 size;
}platform_entire_file;

#define platform_file_op_create_new (platform_file_op_create | platform_file_op_read | platform_file_op_share | platform_file_op_write)
#define platform_file_op_edit (platform_file_op_read | platform_file_op_share | platform_file_op_write)
#define platform_file_op_edit_or_create (platform_file_op_open_or_create | platform_file_op_read | platform_file_op_share | platform_file_op_write)

typedef enum{
	platform_file_op_create = 0x01,
	platform_file_op_read   = 0x02,
	platform_file_op_write  = 0x04,
	platform_file_op_share  = 0x08,
	platform_file_op_open_or_create = 0x10
}platform_file_op;
typedef enum{
	open_file_result_nothing = 0,
	open_file_result_not_found = 1,
	open_file_result_already_exists = 2,
	open_file_result_overwrote_existing = 3,
	open_file_result_access_denied = 4
}platform_open_file_result;


#define PLATFORM_ALLOC(name) void * name(u32 size)
typedef PLATFORM_ALLOC(platform_allocate_memory);

#define PLATFORM_FREE(name) name(void *memory)
typedef PLATFORM_FREE(platform_free_memory);

#define PLATFORM_MOVE_FILE(name) name(u8 *file_path, u8 *new_path)
typedef PLATFORM_MOVE_FILE(platform_move_file);

#define PLATFORM_READ_FROM_FILE(name) void * name(platform_file_handle file_handle, uint64 offset, uint64 size, void *destination)
typedef PLATFORM_READ_FROM_FILE(platform_read_from_file);

#define PLATFORM_WRITE_TO_FILE(name) name(platform_file_handle file_handle, uint64 offset, uint64 size, void *contents)
typedef PLATFORM_WRITE_TO_FILE(platform_write_to_file);

#define PLATFORM_OPEN_FILE_FROM_PATH(name) platform_file_handle name(uint8 *path, platform_file_op fileOp)
typedef PLATFORM_OPEN_FILE_FROM_PATH(platform_open_file);

#define PLATFORM_CLOSE_FILE(name) void name(platform_file_handle file_handle)
typedef PLATFORM_CLOSE_FILE(platform_close_file);

#define PLATFORM_GET_FILE_INFO(name) platform_file_min_attributes name(platform_file_handle file_handle)
typedef PLATFORM_GET_FILE_INFO(platform_f_get_file_info);

#define read_time_stamp_COUNTER __rdtsc()

//platform_find_first_file() and platform_find_next_file() will fill a platform_file_search_info
//struct with some of the file's data

//Looks for the first file with the specified pattern and returns a handle to use with
//platform_find_next_file and platform_find_close.
#define PLATFORM_FIND_FIRST_FILE(name) \
	platform_file_search name(u8 *pattern, platform_file_search_info *fileSearchInfo)
typedef PLATFORM_FIND_FIRST_FILE(platform_find_first_file);

#define PLATFORM_FIND_NEXT_FILE(name)\
	u32 name(platform_file_search fileSearch, platform_file_search_info *fileSearchInfo)
typedef PLATFORM_FIND_NEXT_FILE(platform_find_next_file);

#define PLATFORM_FIND_CLOSE(name)\
	void name(platform_file_search fileSearch)
typedef PLATFORM_FIND_CLOSE(platform_find_close);


typedef struct{

    u16 window_is_focused;
	u16 mouseAtWindow;

    platform_allocate_memory *Alloc;
    platform_free_memory *Free;
    platform_open_file *f_open_file;
    platform_close_file *f_close_file;
    platform_read_from_file *f_read_from_file;
    platform_write_to_file  *f_write_to_file;
    platform_f_get_file_info  *f_get_file_info;
	platform_move_file *f_move_file;

	platform_find_first_file *f_find_first_file;
	platform_find_next_file  *f_find_next_file;
	platform_find_close     *f_find_close;
}platform_api;

typedef struct{
	platform_api *platform;
	memory_area *area;
	temporary_area temporary_area;

    platform_file_search search_handle;

	platform_file_search_info first_file;
    platform_file_search_info current_file;

	b16 found_next_file;
	b16 scan_sub_directories;

	u16 current_directory_index;
	u16 reading_directories_count;
    platform_file_attributes *sub_directories;
	u8 next_full_directory[256];

}platform_file_scan_work;

//fills all the function pointers defined and returns a new platform_api struct
inline platform_api
platform_initialize();

inline platform_entire_file
platform_read_entire_file_handle(platform_api *platform,
		                         platform_file_handle file_handle,
								 memory_area *area)
{
	platform_entire_file entireFile = {0};
	//get file size
	platform_file_min_attributes file_info = platform->f_get_file_info(file_handle);
	u32 file_size = file_info.size;

	//push size to area
	entireFile.contents = memory_area_push_size(area, file_size);
	entireFile.size     = file_size;

	//read all data
	platform->f_read_from_file(file_handle, 0, file_size, entireFile.contents);

	return(entireFile);

}
inline u8 
input_down(input_button map)
{
    return(map.was_down > 0);
}

#define input_WasDown(map) input_pressed(map)
#define input_pressed_up(map) ((!map.was_down && map.transition_count))
inline u8 
input_pressed(input_button map)
{
    return(map.was_down && map.transition_count);
}



inline u8
input_up(input_button map)
{
    return(!map.was_down && map.transition_count);
}
#if 0
inline int 
KeyPushed(input_button map)
{
    return map.state & KEYPUSHED; 
}
#endif

static inline void
input_text_reset_cursor(s_input_text *input_text)
{
	input_text->cursor_position_l = 0;
	input_text->cursor_position_r = 0;
}
inline i32 
input_text_shift_buffer_r(s_input_text *input_text, u32 from, u32 amount)
{
  i32 loop = amount > 0;
  i32 got_shifted = 0;
  u32 shift_remaining_count = amount;
  u32 key_count = input_text->key_count;
  while(loop)
  {
	 shift_remaining_count--;
	u32 cursorI = from + (key_count - from); 
    while(cursorI > from) 
	{
	 input_text->target_buffer[cursorI] = input_text->target_buffer[cursorI - 1];
	 cursorI--;
	 got_shifted = 1;
    }
	 loop = shift_remaining_count && got_shifted; 
	 key_count++;
  }
  input_text->key_count += got_shifted * amount;
  return(got_shifted);
}

//shift the text buffer from the specified position by the specified amount
inline i32
input_text_shift_buffer_l(s_input_text *input_text, u32 from, u32 amount)
{
	amount = amount > input_text->key_count ? input_text->key_count : amount;
	i32 loop = amount > 0; 
	i32 got_shifted = 0;
	u32 shift_remaining_count = amount;
	u32 key_count = input_text->key_count;
	while(loop)
	{
		shift_remaining_count--;
		u32 cursorI = from; 
		while(cursorI < key_count)
		{
			//set the "current" letter as the next one
			//if having a text "Hola" and cursorI is 0, then the final output would be
			//"oola", and if this started with a key_count of 4, this ends up being
			//"ola\0".
			input_text->target_buffer[cursorI] = input_text->target_buffer[cursorI + 1];
			cursorI++;
			got_shifted = 1;
		}
		loop = (shift_remaining_count && got_shifted) || (!key_count);
		key_count--;
	}
	input_text->key_count -= got_shifted * amount;
	return(got_shifted);
}

static void
input_text_Copy(s_input_text *input_text, u32 cursorStart, u32 cursorEnd)
{
	//;Clean this
	u32 clipboardSize = 128;
	u32 distanceToEnd = cursorEnd - cursorStart;
  	uint8 *dest = input_text->clipboardBuffer;
  	uint8 *src  = input_text->target_buffer + cursorStart;
  	u32 size = clipboardSize < distanceToEnd ?
		clipboardSize : distanceToEnd;
	//u32 size = (clipboardSize - 1)  < distanceToEnd ? clipboardSize : distanceToEnd;
  	u32 i = 0;
	uint8 c = src[0];

  	while(c != '\0' && 
		  i < size){
  		c = src[i];
  		dest[i] = c;
  		i++;
      }
    //;Add null character to end
    input_text->clipboardBuffer[i] = '\0';
    input_text->clipboardSizeUsed = i;
}

//sets a target buffer to write and restores
//the cursor positions.
static void
input_text_set_target(
		s_input_text *input_text,
		u8 *target_buffer,
		u32 *target_buffer_count,
		u32 target_buffer_max)
{
	input_text->target_buffer = target_buffer;
	input_text->target_buffer_max = target_buffer_max;
	input_text->target_buffer_count = target_buffer_count;
	if(target_buffer_count)
	{
		input_text->key_count = (*target_buffer_count);
	}
	else
	{
		input_text->key_count = string_count(target_buffer) - 1;
	}
	input_text->cursor_position_l = 0;
	input_text->cursor_position_r = 0;
}

//use the normal target buffer
static void
input_text_restore_buffer(s_input_text *input_text)
{
	input_text->target_buffer = 0;
	input_text->target_buffer_max = 0;
	input_text->key_count = 0;
	input_text->target_buffer_count = 0;
}

static inline void
input_text_restore_and_clear_buffer(s_input_text *input_text)
{
	memory_clear(
			input_text->target_buffer,
			input_text->key_count);
}

static inline u32 
input_text_push_text(s_input_text *input_text, u8 *text)
{
	u32 c = 0;
	u32 i = 0;
	while((c = text[i]) != '\0' && i < input_text->target_buffer_max)
	{
		input_text->target_buffer[i + input_text->key_count++] =
			text[i++];
	}
	return(i > 0);
}

static inline void
input_text_clear_target_buffer(s_input_text *input_text)
{
	memory_clear(
			input_text->target_buffer, input_text->key_count);
}

static void
input_text_update(s_input_text *input_text)
{
	input_text->entered = 0;
	input_text->last_key_count = input_text->key_count;

    input_keystate *key_state = &input_text->key_state;
	u8 key = input_text->key;
	u8 current_key_code = input_text->current_key_code;
	u32 is_down = input_text->key_state.is_down;
	u32 was_down = input_text->key_state.was_down;
	//Process this whenever I actually fkin press it.
	//;Cleanup.
	if(!input_text->focused)
	{
		input_text->got_focus   = 0;
		return;
	}
	if(!input_text->target_buffer)
	{
		input_text_restore_buffer(input_text);
	}
	input_text->focused  = 0;
	input_text->got_focus = 1;

	//TODO(Agu): Handle selected text and clipboard. 
	if(is_down && !was_down)
	{
	   if(current_key_code == KEY_CODE_ESC)
	   {
		   input_text->cursor_position_r = input_text->cursor_position_l;
	   }
	   if(current_key_code == KEY_CODE_ENTER)
	   {
		   input_text->entered = 1;
	   }
	}
	//limit the cursor positions.

	//;Hold time is one once the keyboard starts "repeating"
	if(is_down)
	{
			u32 clipboardSize = 128 - 1;
			u32 cursorStart = input_text->cursor_position_l; 
  			u32 cursorEnd   = input_text->cursor_position_r;
			//;Reserve '\0' for clipboardSize
			u32 clipboardSizeUsed = input_text->clipboardSizeUsed;
  			if(input_text->cursor_position_l >= input_text->cursor_position_r)
  			{
  			    cursorStart = cursorEnd;
  			    cursorEnd = input_text->cursor_position_l;
  			}

			u32 distanceToEnd = cursorEnd - cursorStart;

			//type normally
	    if(key >= ' ' && key <= '}')
		{
			if(input_text->key_count < input_text->target_buffer_max)
			{
				//;Get rid of the selected part
				input_text_shift_buffer_l(input_text, cursorStart, distanceToEnd);
				//;Move contents of the buffer if cursor isn't at the end
				input_text->cursor_position_l = cursorStart;
				if(!input_text_shift_buffer_r(input_text, cursorStart, 1))
				{
					input_text->key_count++;
				}
				input_text->target_buffer[input_text->cursor_position_l++] = key;
				input_text->cursor_position_r = input_text->cursor_position_l; 
			}
		}
		else if(key == KEY_CODE_BACKSPACE)
		{
			//only delete one characer
			if(!distanceToEnd && cursorStart > 0)
			{
			  input_text_shift_buffer_l(input_text, cursorStart - 1, 1);
			  input_text->cursor_position_l--;
			  input_text->cursor_position_r = input_text->cursor_position_l; 
			  //Add null character to end
#if 0
			  if(cursorStart == input_text->key_count)
			  {
				  input_text->target_buffer[input_text->cursor_position_l] = '\0';
			  }
#endif
			}
			else if(distanceToEnd)
			{
			  //;Get rid of the selected part
			  input_text_shift_buffer_l(input_text, cursorStart, distanceToEnd);
			  input_text->cursor_position_r = input_text->cursor_position_l; 
			}
		}
		else if(input_text->ctrl_l)
		{
			//;Cut, copy, paste
			//;TODO(Agu): CHECK FOR OVERFLOW.

			if(current_key_code == KEY_CODE_A){
				input_text->cursor_position_l = 0;
				input_text->cursor_position_r = input_text->key_count;
			}
			else if(current_key_code == KEY_CODE_X)
			{
  				if(distanceToEnd != 0)
  				{
				    input_text_Copy(input_text, cursorStart, cursorEnd);

					input_text->cursor_position_l	  = cursorStart;
					input_text->cursor_position_r = cursorStart;
				    input_text_shift_buffer_l(input_text, cursorStart, distanceToEnd);
				}
//Shift
			}
			else if(current_key_code == KEY_CODE_C)
			{
  				if(distanceToEnd != 0)
  				{
				   input_text_Copy(input_text, cursorStart, cursorEnd);
  				}
			}
			else if(current_key_code == KEY_CODE_V)
			{
				//;Shift selected characters.
				cursorEnd -= distanceToEnd;
				input_text_shift_buffer_l(input_text, cursorStart, distanceToEnd);
				if(!input_text_shift_buffer_r(input_text, cursorStart, input_text->clipboardSizeUsed))
				{
					input_text->key_count  += input_text->clipboardSizeUsed; 
				}

				//paste
  				uint8 *src  = input_text->clipboardBuffer;
  				uint8 *dest = input_text->target_buffer + cursorStart;
  				u32 i = 0;
				uint8 c = src[0]; 
				while(c != '\0' && 
					  i < input_text->clipboardSizeUsed) {
					c = src[i]; 
					dest[i] = c;
				    i++;
				}
				input_text->cursor_position_l	  = cursorEnd + i;
				input_text->cursor_position_r = cursorEnd + i;
			}

		}
		//else if(key == KEY_CODE_DOWN)

	}

	u32 distance = input_text->cursor_position_l != input_text->cursor_position_r;
	input_text->cursor_moved = 0;
	if(current_key_code == KEY_CODE_LEFT)
	{
		//only moved if inside the bounds 
		if(input_text->cursor_position_l > 0)
		{
			//cursor got moved
			input_text->cursor_moved = 1;
			//only move the left part for selection
		    if(input_text->shift_l)
		    {
		    	input_text->cursor_position_l--;
		    }
		    else
		    {
				//go to the leftmost position of both cursors
		    	if(input_text->cursor_position_r < input_text->cursor_position_l)
		    	{
		    		input_text->cursor_position_l = input_text->cursor_position_r;
		    	}
		    	else
		    	{
		    		if(!distance)
		    		{
		    		  input_text->cursor_position_l--;
		    		}
		    	}
		    }
		}
		//;restore mark
		if(!input_text->shift_l)
		{
		   input_text->cursor_position_r = input_text->cursor_position_l;
		}

	}
	//else if(key == KEY_CODE_UP)
	else if(current_key_code == KEY_CODE_RIGHT)
	{
		  if(input_text->cursor_position_l < input_text->key_count)
		  {
			  //cursor got moved
			  input_text->cursor_moved = 1;

			  if(input_text->shift_l)
			  {
			  	  input_text->cursor_position_l++;
			  }
			  else
			  {
				  //go to the rightmost cursor
			  	  if(input_text->cursor_position_r > input_text->cursor_position_l)
			  	  {
			  	    input_text->cursor_position_l = input_text->cursor_position_r;
			  	  }
			  	  else
			  	  {
			  	  	  if(!distance)
			  	  	  {
			  	  	     input_text->cursor_position_l++;
			  	  	  }
			  	  }
			  }
		  }
		  //;restore mark
		  if(!input_text->shift_l)
		  {
		     input_text->cursor_position_r = input_text->cursor_position_l;
		  }
	}

	if(input_text->target_buffer_count)
	{
		*input_text->target_buffer_count = input_text->key_count;
	}
}


inline platform_file_scan_work
platform_file_scan_begin(
		platform_api *platform,
		memory_area *area,
		b32 scan_sub_directories,
		u8 *directory_name
		)
{
	platform_file_scan_work scan_work   = {0};
	scan_work.platform                  = platform;
	scan_work.area                      = area;
	scan_work.reading_directories_count = 1;
	scan_work.temporary_area            = temporary_area_begin(area);
	scan_work.scan_sub_directories      = scan_sub_directories;

	scan_work.sub_directories = memory_area_clear_and_push_array(
			area,
			platform_file_attributes,
			200);
	//start from the specified directory
	string_copy(directory_name, scan_work.sub_directories[0].path);
	if(!string_ends_with_char(directory_name, '/'))
	{
		string_add(
				"/",
				scan_work.sub_directories[0].path,
				sizeof(scan_work.sub_directories[0].path));
	}

	return(scan_work);
}

inline void
platform_file_scan_end(platform_file_scan_work *scan_work)
{
	temporary_area_end(&scan_work->temporary_area);
	scan_work->area = 0;
}


static inline u32
platform_scan_first_file(
		platform_file_scan_work *scan_work,
		u8 *search_pattern)
{
	platform_api *platform = scan_work->platform;
	//start new search
	//now read from the search pattern!
	u8 file_search_pattern[256] = {0};
	format_text(file_search_pattern,
			sizeof(file_search_pattern),
			"%s%s",
			scan_work->next_full_directory, search_pattern);

	u32 found_file = 0;
	//first scan for files twice to ignore the "." and ".." "directories"
	platform_file_search_info *file_search_data = &scan_work->first_file;
	platform_file_search file_search = 
	platform->f_find_first_file(file_search_pattern,
				file_search_data);
	if(file_search.handle)
	{
		if(string_compare(file_search_data->name, "."))
		{
			platform->f_find_next_file(file_search, file_search_data);
			found_file = platform->f_find_next_file(file_search, file_search_data);
		}
		else
		{
			found_file = 1;
		}
	}

	scan_work->found_next_file = found_file;
	scan_work->search_handle = file_search;
	scan_work->current_file.name = 0;

	return(found_file);
}
//scans a new directory from the directories array
//returns true if the directory index is less than the total count
inline u32
platform_scanning_directory(platform_file_scan_work *scan_work)
{
	platform_api *platform = scan_work->platform;
	platform_file_attributes *sub_directory_array = scan_work->sub_directories;

	platform_file_attributes *next_directory = sub_directory_array + 
		scan_work->current_directory_index;
	scan_work->current_directory_index++;
	//use the directory name and read its files and sub directories
	u8 *next_full_directory = scan_work->next_full_directory;
	string_clear(next_full_directory);
	string_concadenate(next_directory->path,
			           next_directory->name,
					   next_full_directory,
					   sizeof(scan_work->next_full_directory));

	//include search pattern
	u8 file_search_pattern[256] = {0};


	b32 still_reading_directories = scan_work->current_directory_index - 1 < scan_work->reading_directories_count;
	//scan sub directories and add them to the list
	if(still_reading_directories && scan_work->scan_sub_directories)
	{
		format_text(file_search_pattern,
				sizeof(file_search_pattern),
				"%s%s*",
				next_directory->path, next_directory->name);

		platform_file_search_info out_directory_search = {0};
		//start scanning for sub-directories
		platform_file_search dir_search = platform->f_find_first_file(
				file_search_pattern,
				&out_directory_search);
		//ignore '.' and '..' sub directories
		if(dir_search.handle)
		{
			if(string_compare(out_directory_search.name, "."))
			{
				platform->f_find_next_file(dir_search, &out_directory_search);
				platform->f_find_next_file(dir_search, &out_directory_search);
			}
			b32 found_next_dir = 1;
			while(found_next_dir)
			{
				if(out_directory_search.is_directory)
				{
					//add new sub directory
					platform_file_attributes *directory_file = 
						(scan_work->sub_directories +
						 scan_work->reading_directories_count);

					directory_file->is_directory = 1;
					directory_file->info.date = out_directory_search.write_time;

					//add the ending slash to the name
					string_concadenate(out_directory_search.name,
							"/",
							directory_file->name,
							sizeof(directory_file->name));

					//put the current scanning directory as the path
					string_copy(
							scan_work->next_full_directory,
							directory_file->path);

					scan_work->reading_directories_count++;

				}
				found_next_dir = platform->f_find_next_file(dir_search, &out_directory_search);
			}
		}
	}
	return(scan_work->current_directory_index - 1 < scan_work->reading_directories_count);
}

inline u32
platform_scanning_next_file(platform_file_scan_work *scan_work)
{
	platform_api *platform = scan_work->platform;

	//if this is the first file detected
	if(!scan_work->current_file.name)
	{
		scan_work->current_file = scan_work->first_file;
	}
	else
	{
		//read next file
	    platform_file_search_info *file_search_data = &scan_work->current_file;
        scan_work->found_next_file = platform->f_find_next_file(scan_work->search_handle, file_search_data);
	}

	return(scan_work->found_next_file != 0);
}

inline u32
platform_write_stream_to_file(
		platform_api *platform,
		stream_data *contents_stream,
		u8 *path_and_name)
{
	u32 success = 0;

	platform_file_handle file_result = platform->f_open_file(
			path_and_name,
			platform_file_op_create_new);
	if(file_result.handle)
	{
		success = 1;
        stream_chunk *chunk = contents_stream->first; 
        if(chunk)
        {
			u32 data_offset = 0;
            while(chunk)
            {
               u8 *contents  = (u8 *)chunk->contents; 
			   u32 data_size = chunk->size;

               platform->f_write_to_file(
		     		  file_result, 
		     		  data_offset,
		     		  data_size,
					  contents);
               chunk = chunk->next;
		       //advance for the next chunk
		       data_offset += data_size;
            }
        }

		platform->f_close_file(file_result);
	}
	return(success);
}
