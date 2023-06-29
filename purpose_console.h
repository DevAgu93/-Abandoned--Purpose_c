typedef struct{
	u32 buffer_total_size;
	u32 buffer_used_size;
	u8 *buffer;
}console_buffer;

typedef struct s_console_command_chunk{
	u8 *data;
	u32 size;
	struct s_console_command_chunk *next;
}console_command_chunk;

typedef struct{

	platform_api *platform;
	//Debug stuff
	u32 buffer_total_size;
	u32 buffer_used_size;
	u8 *buffer;

	u32 command_requested;
	u32 command_history_char_limit;

	u32 command_history_total; //256 or double
	u32 command_history_count;
	//console_command_chunk *history;

	u32 input_buffer_max;
	u32 input_buffer_count;
	u8 *input_buffer;

	u16 commands_max;
	u32 command_chunks_consumed;

	console_command_chunk *first_command;
	console_command_chunk *last_command;

	u32 command_chunk_buffer_used;
	u32 command_chunk_buffer_max;
	u8 *command_chunk_buffer;

	u32 buffer_updated;
}s_game_console;


static inline s_game_console 
console_allocate(
		memory_area *area,
		platform_api *platform,
		u32 buffer_total_size,
		u32 commands_max,
		u32 input_buffer_character_limit)
{
	s_game_console game_console = {0};
	game_console.buffer_total_size = buffer_total_size;
	game_console.buffer = memory_area_push_size(area, buffer_total_size);

	//allocate input buffer and the total amount of characters
	game_console.input_buffer_max = input_buffer_character_limit;
	game_console.input_buffer = memory_area_push_size(area, input_buffer_character_limit);

	game_console.command_history_char_limit  = input_buffer_character_limit;
	//TODO: Allocate command history
	game_console.command_history_total = 20;
	game_console.platform = platform;

	//allocate command "chunks" where the whole command will be stored for every chunk
	game_console.command_chunk_buffer_max = input_buffer_character_limit * sizeof(console_command_chunk) * commands_max;
	game_console.command_chunk_buffer = memory_area_push_size(
			area, game_console.command_chunk_buffer_max);
	game_console.input_buffer_count = 0;

	return(game_console);
}

static inline void
console_push_input_command(s_game_console *console)
{
	u32 input_buffer_count = string_count(console->input_buffer);
	//get one chunk and fill data
	console_command_chunk *command_chunk = (console_command_chunk *)(console->command_chunk_buffer + console->command_chunk_buffer_used);
	command_chunk->data = (u8 *)(command_chunk + 1);
	//increase the buffer use
	console->command_chunk_buffer_used += sizeof(console_command_chunk) + input_buffer_count;

	memory_copy(console->input_buffer, command_chunk->data, input_buffer_count);
	//increase the command count for later consumption
	Assert(console->command_chunk_buffer_max > console->command_chunk_buffer_used);
	//check if a next command is allocated.
	if(!console->first_command)
	{
		console->first_command = command_chunk;
	}
	if(console->last_command)
	{
		console->last_command->next = command_chunk;
	}
	console->last_command = command_chunk;

	//clear input buffer
	console->input_buffer_count = 0;
	memory_clear(console->input_buffer, console->input_buffer_max);
}

static inline console_command_chunk *
console_consume_command(s_game_console *console)
{
	console_command_chunk *result = console->first_command;
	if(console->first_command)
	{
		console->first_command = console->first_command->next;
	}
	
	return(result);
}

inline void
game_console_CheckOverflow(s_game_console *game_console)
{

}

inline void
game_console_PushChar(s_game_console *game_console , u8 c)
{

}

inline void
game_console_clear(s_game_console *console)
{
	memory_zero(console->buffer, console->buffer_used_size);
	console->buffer_used_size = 0;
	console->buffer_updated = 1;
}

static void
game_console_PushText(s_game_console *game_console, u8 *text)
{
	u32 textSize = string_count(text);
	//Assert(game_console->buffer_used_size + textSize < game_console->buffer_total_size);

	if(game_console->buffer_used_size + textSize > game_console->buffer_total_size - 1)
	{
		//Copy the start of the buffer to the end
		u32 toleranceSize = textSize;

		//This deletes whatever was at the start.
		u8 *copyFrom = (game_console->buffer + toleranceSize);
		u8 *copyTo   = game_console->buffer;
	    memory_Copy(copyFrom, copyTo, game_console->buffer_used_size - toleranceSize);
		game_console->buffer_used_size -= toleranceSize;
	}
	//string_Copy(text, (game_console->buffer + game_console->buffer_used_size));
	memory_Copy(text, (game_console->buffer + game_console->buffer_used_size), (textSize - 1));

	game_console->buffer_used_size += textSize - 1;
	game_console->buffer_updated = 1;
}

static void
game_console_PushLine(s_game_console *game_console, u8 *text)
{
	if(game_console->buffer_used_size > 0)
	{
		//Put '\n' before the new line for securing it
		if(game_console->buffer[game_console->buffer_used_size - 1] != '\n')
		{
		   game_console->buffer[game_console->buffer_used_size] = '\n';
	       game_console->buffer_used_size += 1;
		}
	}
    game_console_PushText(game_console, text);
	//Add new line to end
    *(game_console->buffer + game_console->buffer_used_size) = '\n';
    game_console->buffer_used_size += 1;
}

static void
game_console_PushLinef(s_game_console *game_console, u8 *text, ...)
{
    u8 buffer[256] = {0};

    va_list args;
    va_start_m(args, text);
    u32 TextSize = format_text_list(buffer, sizeof(buffer), text, args);
    va_end_m(args);
	game_console_PushLine(game_console, buffer);
}

inline void
game_console_log_to_txt(s_game_console *console)
{
	platform_api *platform = console->platform;

	platform_file_handle txt_file = platform->f_open_file(
			"console_log.txt",
			platform_file_op_create_new);

	platform->f_write_to_file(
			txt_file,
			0,
			console->buffer_used_size,
			console->buffer);

	platform->f_close_file(txt_file);

	game_console_PushLine(
			console, "saved log file");
}

static void
game_console_push_stream(s_game_console *game_console, stream_data *stream)
{
	//stream_data *stream = &gameState->infoStream;
    stream_chunk *chunk = stream->first; 
    while(chunk)
    {
       uint8 *contents = (uint8 *)chunk->contents; 
       game_console_PushLine(game_console, contents);
       chunk = chunk->next;
    }
}

static inline void
game_console_push_and_clear_stream(s_game_console *game_console, stream_data *stream)
{
	//stream_data *stream = &gameState->infoStream;
	game_console_push_stream(game_console, stream);

	stream->first = 0;
	stream->next = 0;
}

inline void
ASSERT_log(s_game_console *c, u32 condition)
{
	if(!condition)
	{
		game_console_log_to_txt(c);
		Assert(0);
	}
}

inline void
ASSERT_stream_log(
		s_game_console *c,
		stream_data *s,
		u32 condition)
{
	if(!condition)
	{
		game_console_push_stream(c, s);
		game_console_log_to_txt(c);
		Assert(0);
	}
}

inline void
game_console_end_frame(s_game_console *c)
{
	c->buffer_updated = 0;
	c->last_command = 0;
	c->first_command = 0;
	c->command_chunk_buffer_used = 0;
}
