static void
editor_console_push_command(struct s_editor_state *editor_state,
		                  s_game_console *console,
						  u8 *command)
{
	u32 arg_count = string_count_words(command);

	if(arg_count)
	{
		string_to_low(command);
		u8 args[256] = {0};
		u32 next_arg_index = 0;

		u32 next_arg_length = string_copy_word_from_index(
				command,
				args,
				next_arg_index);


		if(string_compare(args, "help"))
		{
			game_console_PushLinef(
					console, "\n\n"
					"clear: clears the console buffer\n"
					"SaveLog: saves the contents of the console buffer to a .txt file");
			    game_console_PushLinef(
			    		console, "\n"
			    		"ResetUi: Resets the ui interactions in case of a bug."); 
		}
		else if(string_compare(args, "clear"))
		{
			game_console_clear(console);
		}
		else if(string_compare(args, "savelog"))
		{
			game_console_log_to_txt(
					console);
		}
	}
	else
	{
		game_console_PushText(console, "\n");
	}

	//For commands
#if 0
	s_game_console *debug_console      = &gameState->debug_console;
	game_console_chunk *pushedChunk = debug_console->history + debug_console->historyCount++;

	pushedChunk->size = textSize;
	pushedChunk->at   = gameState->debugBufferUsedSize;
#endif

}

static inline void
editor_console_end_frame(
		s_editor_state *editor_state,
		struct s_editor_input *game_input
		)
{
	s_game_console *console = &editor_state->debug_console;

	console_command_chunk *next_command = 0;
	while(next_command = console_consume_command(console))
	{
		editor_console_push_command(editor_state,
				console,
				next_command->data);
	}
	game_console_end_frame(console);

	game_console_push_and_clear_stream(
			console, &editor_state->editor.info_stream);
	game_console_push_and_clear_stream(
			console, &editor_state->editor_assets->info_stream);
	memory_area_reset(
			editor_state->editor.info_stream.area);
}

