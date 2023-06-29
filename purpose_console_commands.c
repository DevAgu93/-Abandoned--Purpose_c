static void
game_console_push_command(struct s_game_state *game_state_data,
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
					"loadmap (name): reloads the world and loads the specified map if found\n");

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
		if(string_compare(args, "loadmap"))
		{
			if(arg_count > 2)
			{

				struct s_game_assets *assets = game_state_data->game_asset_manager;
//				game_world *w = &game_state_data->world;
//				//get the path and name
//				string_copy_word_from_index(
//						command,
//						args,
//						1);
//				asset_map_data *a_map = assets_load_and_get_map(
//						assets,
//						args);
//				if(a_map)
//				{
//					asset_map_to_world_map(a_map, w);
//				}
//				else
//				{
//					game_console_PushLinef(
//							console,
//							"Could not load the map file \"%s\"",
//							args);
//				}
			}
			else
			{
				game_console_PushLinef(
						console,
						"Not enough arguments given for \"%s\"",
						args);
			}

		}
		else
		{
			game_console_PushLinef(console, "The command \"%s\", is not a valid command", args);
		}

	}

	//For commands
#if 0
	s_game_console *debug_console      = &gameState->debug_console;
	game_console_chunk *pushedChunk = debug_console->history + debug_console->historyCount++;

	pushedChunk->size = textSize;
	pushedChunk->at   = gameState->debugBufferUsedSize;
#endif

}


