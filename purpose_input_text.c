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

	uint8 clipboardBuffer[128];
}s_input_text;

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
	 loop = shift_remaining_count && got_shifted;
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
input_text_update(game_input *gameInput)
{
	s_input_text *input_text = &gameInput->input_text;
	input_text->entered = 0;
	input_text->last_key_count = input_text->key_count;

    input_keystate *key_state = &input_text->key_state;
	u8 key = input_text->key;
	u8 current_key_code = input_text->current_key_code;
	u32 is_down   = input_text->key_state.is_down;
	u32 was_down  = input_text->key_state.was_down;
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
		else if(gameInput->ctrl_l)
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
		    if(gameInput->shift_l)
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
		if(!gameInput->shift_l)
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

			  if(gameInput->shift_l)
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
		  if(!gameInput->shift_l)
		  {
		     input_text->cursor_position_r = input_text->cursor_position_l;
		  }
	}

	if(input_text->target_buffer_count)
	{
		*input_text->target_buffer_count = input_text->key_count;
	}
}

