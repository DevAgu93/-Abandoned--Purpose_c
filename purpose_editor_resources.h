#define _formats_w_dot {\
	".ppmp",\
	".ppmo",\
	".ppan",\
	".pptl",\
	".pcos"\
}
#define _formats_for_search {\
	"*.ppmp",\
	"*.ppmo",\
	"*.ppan",\
	"*.pptl",\
	"*.pcos"\
}

#define er_model(res) ((res->asset_key) ? &res->asset_key->model : 0)

static inline game_resources_directory *
editor_resources_look_for_directory(
		s_game_resource_editor *resource_editor,
		u32 directory_id)
{
	game_resources_directory *dir = 0;
	for(u32 d = 0; d < resource_editor->directories_count; d++)
	{
		if(resource_editor->directories[d].id == directory_id)
		{
			dir = resource_editor->directories + d;
			break;
		}
	}
	return(dir);
}

static void
editor_resources_link_directories(
		s_game_resource_editor *resource_editor)
{
}
static inline game_resources_directory *
editor_resources_create_directory(
		s_game_resource_editor *resource_editor,
		u8 *path_and_name)
{
	game_resources_directory *dir = 0;
	u8 *current_path_and_name = path_and_name;
	if(resource_editor->directories_count < resource_editor->directories_max)
	{
		u32 dir_id = assets_generate_id(path_and_name);
		dir = editor_resources_look_for_directory(resource_editor, dir_id);
		game_resources_directory *next_dir = dir;
		game_resources_directory *child_dir = 0;
		while(dir_id)
		{
			if(!next_dir)
			{
				Assert(resource_editor->directories_count < resource_editor->directories_max);
				next_dir = resource_editor->directories +
					resource_editor->directories_count++;
				memory_clear(next_dir, sizeof(*next_dir));
				if(!dir) dir = next_dir;

				//copy full path and name and get rid of the last slash
				u8 pan_copy[128] = {0};
				string_copy(current_path_and_name, pan_copy);
				u32 count = string_count(pan_copy);
				if(count - 1)
				{
					pan_copy[count - 2] = '\0';
				}
				//extract directory name
				path_fill_directory(pan_copy, next_dir->path);
				//extract last slash
				path_fill_file_name(pan_copy, next_dir->name);
				memory_copy(current_path_and_name, next_dir->path_and_name, sizeof(next_dir->path_and_name));
				next_dir->id = dir_id;
				//for parent creation
				dir_id = assets_generate_id(next_dir->path);
				current_path_and_name = next_dir->path;

				//if a child was created previous to this dir
				if(child_dir)
				{
					if(!next_dir->last)
					{
						next_dir->last = child_dir;
					}
					child_dir->next = next_dir->first;
					next_dir->first = child_dir;
					child_dir->parent = next_dir;
				}
				child_dir = next_dir;
				{
					//parent dir
					game_resources_directory *parent_dir = editor_resources_look_for_directory(resource_editor, dir_id);
					if(parent_dir)
					{
						next_dir->parent = parent_dir;
						if(!parent_dir->last)
						{
							parent_dir->last = next_dir;
						}
						next_dir->next = parent_dir->first;
						parent_dir->first = next_dir;
					}
					next_dir = parent_dir;
				}
			}
			else
			{
				dir_id = 0;
			}
		}
	}
	return(dir);
}


static game_composite_resource *
er_get_composite_resource_slots(
		s_editor_state *editor_state,
		u32 count)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	game_composite_resource *result = 0;
	game_composite_resource *first = resource_editor->composite_resources;

	//zero
	if(!count) return result;

	//the result is the first one
	if(!resource_editor->composite_resources_count)
	{
		result = first;
		result->first = result;
		result->last = result + count - 1;
		resource_editor->composite_resources_count += count;
	}
	else //look for free resources
	{
		game_composite_resource *next = first->next_slots;
		game_composite_resource *prev = first;
		b32 found_space = 0;
		while(!found_space)
		{
			if(next)
			{
				u8 *start = (u8 *)(prev->last + 1);
				u8 *end = (u8 *)(next);
				u32 space_between = (u32)(end - start);
				u32 count_between = space_between / (sizeof(game_composite_resource) * count);
				if(count_between >= count)
				{
					found_space = 1;
					result = next;
					result->last = result + count - 1;
					result->first = result;
					if(result->next_slots)
					{
						result->next_slots = result->last + 1;
					}
				}
				//calculate space
				prev = next;
				next = next->next_slots;
			}
			else
			{
				result = resource_editor->composite_resources + resource_editor->composite_resources_count;
				result->first = result;
				result->last = result + count - 1;
				result->prev_slots = prev;
				result->next_slots = 0;
				prev->next_slots = result;
				resource_editor->composite_resources_count += count;
				found_space = 1;
			}
		}
	}

	Assert(resource_editor->composite_resources_count < resource_editor->composite_resources_max);
	return(result);
}

static inline void
er_free_composite_resources_from_slot(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	if(resource->composite_resources)
	{
		game_composite_resource *prev = resource->composite_resources->prev_slots;
		game_composite_resource *next = resource->composite_resources->next_slots;
		prev->next_slots = next;
		if(next)
		{
			next->prev_slots = prev;
		}
		else//this is the last one, subtract the count
		{
			game_composite_resource *end = resource->composite_resources->last + 1;
			u32 count = (u32)(end - resource->composite_resources);
			editor_state->editor.asset.composite_resources_count -= count;

		}
	}
}

static b32 
er_check_composite_resources_names(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{

	platform_api *platform = editor_state->platform;

	b32 at_least_one_header_modified = 0;
	if(resource->type >= 4)
	{
		platform_file_handle resource_file = platform->f_open_file(
				resource->path_and_name,
				platform_file_op_edit);
		//get common_header
		ppse_asset_header resource_file_header = ppse_read_common_header_handle(
				platform, resource_file);
		//get composite resource header
		u32 composite_resources_offset = resource_file_header.offset_to_composite_resources;
		u32 composite_resource_count = resource_file_header.composite_resource_count;
		ppse_composite_resource source = {0};

		u32 count = 0;
		while(count < composite_resource_count)
		{
			platform->f_read_from_file(
					resource_file,
					composite_resources_offset,
					sizeof(ppse_composite_resource),
					&source);
			//check for naming errors!
			u8 c = 0;
			u32 i = 0;
			b32 header_modified = 0;
			while((c = source.path_and_name[i]) != '\0')
			{
				if(c == '\\')
				{
					header_modified = 1;
					source.path_and_name[i] = '/';
					//shift the name buffer
					if(source.path_and_name[i + 1] == '\\')
					{
						u32 j = i + 1;
						u8 oc = 0;
						while(source.path_and_name[j] != '\0')
						{
							source.path_and_name[j] = source.path_and_name[j + 1];
							j++;
						}
					}
				}
				//remove double slashes
				else if(c == '/' && source.path_and_name[i + 1] == '/')
				{
					header_modified = 1;
					//shift the name buffer
					u32 j = i + 1;
					u8 oc = 0;
					while(source.path_and_name[j] != '\0')
					{
						source.path_and_name[j] = source.path_and_name[j + 1];
						j++;
					}
				}
				i++;
			}
			if(header_modified)
			{
				at_least_one_header_modified = 1;
				source.id = assets_generate_id(resource->path_and_name);
				platform->f_write_to_file(
						resource_file,
						composite_resources_offset,
						sizeof(ppse_composite_resource),
						&source);
			}

			composite_resources_offset += sizeof(ppse_composite_resource);

			count++;
		}
		platform->f_close_file(resource_file);
	}

	return(at_least_one_header_modified);
}

static void
er_set_root_directory(
		s_game_resource_editor *resource_editor,
		u8 *path_and_name)
{
	u32 directory_id = assets_generate_id(path_and_name);
	game_resources_directory *dir = editor_resources_look_for_directory(resource_editor, directory_id);
	if(!dir)
	{
		dir = editor_resources_create_directory(resource_editor, path_and_name);
	}
	resource_editor->root_dir = dir;
	resource_editor->explorer_current_dir = dir;
	memory_copy(dir->path_and_name, resource_editor->explorer_current_path, sizeof(resource_editor->explorer_current_path));
}

static void
er_update_directory(
		s_game_resource_editor *resource_editor)
{
	game_resources_directory *dir = resource_editor->explorer_current_dir;//editor_resources_look_for_directory(resource_editor, directory_id);
	u32 directory_id = dir->id;

	resource_editor->explorer_file_count = 0;
	resource_editor->explorer_file_is_selected = 0;
	resource_editor->explorer_updated_path = 1;
	if(dir)
	{
		//add directories
		game_resources_directory *child_dir = dir->first;
		while(child_dir)
		{
			resource_explorer_file *file = resource_editor->explorer_files + 
				resource_editor->explorer_file_count++;
			file->is_directory = 1;
			file->directory = child_dir;
			child_dir = child_dir->next;
		}
		//add resources
		for(u32 a = 0; a < resource_editor->resources_count; a++)
		{
			game_resource_attributes *resource = resource_editor->resources + a;
			//don't add this resource if the type doesn't mach the reestricted one
			if(resource_editor->explorer_flags & er_explorer_reestrict_to_type &&
			   resource->type != resource_editor->explorer_reestricted_type)
			{
				continue;
			}
			if(resource->directory_id == dir->id)
			{
				//add file
				resource_explorer_file *file = resource_editor->explorer_files + 
					resource_editor->explorer_file_count++;
				file->is_directory = 0;
				file->resource = resource;
			}
		}
		resource_editor->root_dir->child_in_hierarchy = 0;
		dir->child_in_hierarchy = 0;
		game_resources_directory *child = dir;
		while(child->parent)
		{
			child->parent->child_in_hierarchy = child;
			child = child->parent;
		}
	}
	resource_editor->explorer_current_dir = dir;
}

static inline void
editor_resources_extract_directory(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	u32 directory_id = assets_generate_id(resource->path);
	if(directory_id)
	{
		//look for existing directory
		game_resources_directory *dir = editor_resources_look_for_directory(resource_editor, directory_id);
		for(u32 d = 0; d < resource_editor->directories_count; d++)
		{
			if(resource_editor->directories[d].id == directory_id)
			{
				dir = resource_editor->directories + d;
			}
		}
		//no directory with this name was added! generate a new one!
		if(!dir)
		{
			u8 path_copy[126] = {0};
			string_copy(
					resource->path,
					path_copy);
			dir = editor_resources_create_directory(resource_editor, path_copy);
			dir->first_resource = resource;
		}
		else
		{
			if(!dir->first_resource)
			{
				dir->first_resource = resource;
			}
			else
			{
				dir->last_resource->next_in_directory = resource;
			}
		}

		resource->prev_in_directory = dir->last_resource;
		dir->last_resource = resource;
	}
}

static inline game_resource_attributes *
editor_resources_get_new_slot(
		s_editor_state *editor_state)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;

	game_resource_attributes *resource = resource_editor->first_free_resource;
	if(resource)
	{
		resource_editor->first_free_resource = resource->next;
		memory_clear(resource, sizeof(game_resource_attributes));
	}
	else if(resource_editor->resources_max > resource_editor->resources_count)
	{
		resource_editor->resources_loaded_count++;
		resource = resource_editor->resources + resource_editor->resources_count++;
	}
	else //log
	{
	}

	if(!resource_editor->first)
	{
		resource_editor->first = resource;
	}
	else
	{
		resource_editor->last->next = resource;
	}
	resource->prev = resource_editor->last;
	//resource->next = resource_editor->last;
	resource_editor->last = resource;
	return(resource);

}
static inline game_resource_attributes *
er_look_for_resource(
		s_game_editor *game_editor,
		u8 *path_and_name)
{
	u32 resource_id = assets_generate_id(
			path_and_name);
	struct s_game_resource_editor *resource_editor = &game_editor->asset;
	game_resource_attributes *result = 0;

	for(game_resource_attributes *r = resource_editor->first;
			r; r = r->next)
	{
		if(r->id == resource_id)
		{
			result = r; 
			break;
		}
	}
	return(result);
}


static void
editor_assets_fill_path_info(
		game_resource_attributes *game_resource,
		u8 *path_and_name)
{
	u32 path_and_name_count = string_count(path_and_name);
	memory_copy(
			path_and_name,
			game_resource->path_and_name,
			path_and_name_count);

	path_fill_file_name_and_type(
			game_resource->path_and_name,
			game_resource->name,
			game_resource->file_type);
	game_resource->id = assets_generate_id(game_resource->path_and_name);

	path_fill_directory(path_and_name, game_resource->path);
	game_resource->directory_id = assets_generate_id(game_resource->path);

}

static inline void
editor_assets_clear_and_fill_path_info(
		game_resource_attributes *game_resource,
		u8 *path_and_name)
{
	memory_clear(game_resource->path_and_name, sizeof(game_resource->path_and_name));
	memory_clear(game_resource->name, sizeof(game_resource->name));
	memory_clear(game_resource->file_type, sizeof(game_resource->file_type));
	memory_clear(game_resource->path, sizeof(game_resource->path));

	editor_assets_fill_path_info(game_resource, path_and_name);
}

static inline void
er_explorer_close(s_game_editor *game_editor)
{
	game_editor->asset.resource_explorer_closed = 1;
}


static inline void
editor_ui_open_resource_search(game_ui *ui)
{
	ui_open_panel(ui, "__resource_search_panel__");
}
#define editor_ui_close_resource_search(ui) ui_close_panel(ui, "__resource_search_panel__")
#define editor_resource_explorer_file_name(resource_editor) (resource_editor->explorer_output)
#define editor_resource_explorer_output(game_editor) (game_editor->asset.explorer_output)
#define er_explorer_output editor_resource_explorer_output 

static inline void
er_explorer_set_process(
		s_game_editor *editor,
		er_explorer_flags er_flags,
		u8 *process_name)
{
	s_game_resource_editor *resource_editor = &editor->asset;

	string_copy(
			process_name,
			resource_editor->explorer_process_name);
	resource_editor->explorer_process_id = string_kinda_hash(
			process_name);
	resource_editor->explorer_process_completed = 0;
	resource_editor->resource_explorer_closed = 0;
	resource_editor->explorer_valid_file_focused = 0;
	resource_editor->explorer_file_is_selected = 0;
	resource_editor->explorer_flags = er_flags;
	resource_editor->update_path = 1;
	memory_clear(resource_editor->explorer_process_input,
			sizeof(resource_editor->explorer_process_input));
	memory_clear(resource_editor->explorer_output,
			sizeof(resource_editor->explorer_output));
}

#define er_explorer_set_process_reestricted editor_resource_explorer_set_process_reestricted
static inline void
editor_resource_explorer_set_process_reestricted(
		s_game_editor *editor, er_explorer_flags er_flags, asset_type type, u8 *process_name)
{
	s_game_resource_editor *resource_editor = &editor->asset;
	er_explorer_set_process(
			editor, er_flags | er_explorer_reestrict_to_type, process_name);
	resource_editor->explorer_reestricted_type = type;
}

static inline b32
editor_resource_explorer_process_completed(
		s_game_editor *editor,
		u8 *process_name)
{
	s_game_resource_editor *resource_editor = &editor->asset;
	u32 process_id = string_kinda_hash(process_name);
	b32 success = 0;
	if(resource_editor->explorer_process_id == process_id && 
		resource_editor->explorer_process_completed)
	{
		resource_editor->explorer_process_completed = 0;
		success = 1;
		resource_editor->resource_explorer_closed = (resource_editor->explorer_flags & er_explorer_close_on_complete) != 0;
	}
	return(success);
}

static inline void
editor_run_resource_explorer(
		s_game_editor *editor,
		game_ui *ui)
{
	s_game_resource_editor *resource_editor = &editor->asset;
	ui_explorer *explorer = ui->explorer;
	//u32 kindaHash = string_kinda_hash(id);
	//ui->explorer->process_id = kindaHash;
	//Special flags for explorer
	ui_panel_flags panel_flags = 
		(ui_panel_flags_title | 
		 ui_panel_flags_move | 
		 ui_panel_flags_close | 
		 ui_panel_flags_size_to_content |
		 ui_panel_flags_init_once |
		 ui_panel_flags_focus_when_opened);

	ui_panel *panel = ui_push_root_for_rendering(
			ui,
			200,
			200,
			400,
			400,
			panel_flags,
			"__resource_search_panel__");

	if(resource_editor->resource_explorer_closed)
	{
		ui_close_panel_ptr(ui, panel);
	}
	if(panel->closed && !resource_editor->resource_explorer_closed)
	{
		ui_open_panel_ptr(ui, panel);
	}
	if(panel->closed)
	{
		ui_panel_end(ui);
		return;
	}
	ui_next_nodes_interaction_only_begin(ui);

	if(resource_editor->update_path)
	{
		//based on the path, look for the next directory

		resource_editor->update_path = 0;
		er_update_directory(
				resource_editor);
			
	}

	b32 move_panel = 0;
	bool32 ex_file_got_selected = 0;
	bool32 ex_clicked_file = 0;
	bool8 ex_clicked_ok = 0;
	bool8 ex_clicked_cancel = 0;
	b8 ex_file_double_clicked = 0;
	//select new file when clicked

	ui_node *explorer_title;
	ui_node *explorer_bg;

	ui_node *explorer_contents_node;
	//ui_node *explorer_files_folders;

	ui_set_width(ui, ui_size_percent_of_parent(1.0f, 1.0f))
	ui_set_height(ui, ui_size_specified(TITLEHEIGHT, 1.0f))
	{
		explorer_title = ui_title_bar(ui);
		ui_set_parent(ui, explorer_title)
		{
			ui_space_specified(ui, 4.0f, 1.0f);
			ui_set_row(ui)
			{
				ui_space_specified(ui, 4.0f, 1.0f);
				ui_node *explorer_process = ui_create_node(ui,
						node_text,
						0);
				ui_node_set_display_string(ui,
						explorer_process,
					    resource_editor->explorer_process_name);	
			}
		}
		//move if holding the mouse left
		if(ui_node_mouse_l_down(ui, explorer_title))
		{
			move_panel = 1;
		}
	}
	//background
	ui_set_color(ui, ui_color_background, V4(0x0f, 0x0f, 0x0f, 0xDC))
	ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 0.0f))
	{
		explorer_bg = ui_create_node(ui,
				node_background | 
				node_clickeable,
				"__EX_BG__");
		explorer_bg->padding_x = 6;
		explorer_bg->padding_y = 6;
		//move if holding the mouse left
		if(ui_node_mouse_l_down(ui, explorer_bg))
		{
			move_panel = 1;
		}

	}
	ui_set_parent(ui, explorer_bg)
	{

		//show the current visited directories
		ui_set_w_soch(ui, 0.0f)ui_set_h_soch(ui, 1.0f)ui_set_row_n(ui) ui_set_wh_text(ui, 4.0f, 1.0f) ui_extra_flags(ui, node_border)
		{
//			ui_space_ppct(ui, 1.0f, 0.0f);
			game_resources_directory *dir = resource_editor->root_dir;
			u32 i = 0;
			while(dir)
			{
				if(ui_buttonf(ui, "%s >##explorer_dir%u", dir->name, i) &&
					resource_editor->explorer_current_dir != dir)
				{
					resource_editor->explorer_current_dir = dir;
					resource_editor->update_path = 1;
				}
				dir = dir->child_in_hierarchy;
				i++;
			}
//			ui_space_ppct(ui, 1.0f, 0.0f);
		}
		//files, folders...
		ui_set_wh(ui, ui_size_percent_of_parent(1.0f, 0.0f))
		{
			ui_set_row(ui)
			{
				ui_space_specified(ui, 6.0f, 1.0f);
				explorer_contents_node = ui_create_node(ui,
						0,
						"__EX_WIDGETS__");
				ui_space_specified(ui, 6.0f, 1.0f);
			}
		}
		
		//files, folders, search bar...
		ui_set_parent(ui, explorer_contents_node)
		{
			ui_set_row(ui);
			ui_set_wh_ppct(ui, 1.0f, 0.0f)
			{
				//ui_node *resource_list_node = ui_create_node(ui,
				//				node_scroll_y |
				//				node_clip |
				//				node_background |
				//				node_border,
				//				"__FILES_FOLDERS__"); 

				//ui_node *resource_list_node = ui_node_box(ui,
				//				"__FILES_FOLDERS__"); 
				//ui_set_parent(ui, resource_list_node) ui_set_column(ui)
				ui_node *box_node = ui_box_with_scroll(ui, "__FILES_FOLDERS__");
				ui_set_parent(ui, box_node)
				{
					u32 a = 0;
					for(u32 f = 0; f < resource_editor->explorer_file_count; f++)
					{
						resource_explorer_file *file = resource_editor->explorer_files + f;

						b32 active = 0;
						u8 *name = 0;
						if(file->is_directory)
						{
							game_resources_directory *dir = file->directory;
							name = dir->name;
						}
						else
						{
							game_resource_attributes *resource = file->resource;
							//skip assets that can't be loaded
							if(!resource->load_success)
							{
								continue;
							}
							name = resource->name;
						}
						active = resource_editor->explorer_file_is_selected &&
							resource_editor->explorer_selected_file_index == f;

						ui_node *file_node;
						//selectable file
						ui_set_w_ppct(ui, 1.0f, 0.0f) ui_set_h_text(ui, 4.0f, 0.0f)
						{
							file_node = ui_selectable_boxf(
									ui, 
									active,
									"__RFILE__ %u",
									f);
							file_node->padding_x = 2;
							file_node->padding_y = 2;
						}
						//file icon and name
						ui_set_parent(ui, file_node) ui_set_row(ui) ui_set_wh_text(ui, 4.0f, 1.0f)
						{
							ui_set_w_specified(ui, 12.0f, 1.0f)
							{
								ui_node *img_node = ui_create_node(ui, 0, 0);
								//directory temp icon
								if(file->is_directory)
								{
									ui_text(ui, name);
									ui_node_push_rectangle_wh(ui,
											img_node,
											0, 2, 12, 8, V4(0, 160, 0, 255));
								}
								else
								{
									ui_textf(ui, "%s.%s", name, file->resource->file_type);
									//file temp icon 
									ui_node_push_rectangle_wh(ui,
											img_node,
											0, 0, 8, 12, V4(255, 255, 255, 255));
								}
							}
						}
						//file interaction
						ui_usri file_node_usri = ui_usri_from_node(ui, file_node);

						//don't process this if the path got updated
						if(!resource_editor->explorer_updated_path)
						{
							if(ui_usri_mouse_l_pressed(file_node_usri))
							{
								//							resource_editor->explorer_selected_resource = resource;
								//							resource_editor->explorer_valid_file_focused = 1;
								resource_editor->explorer_file_is_selected = 1;
								resource_editor->explorer_selected_file_index = f;

								if(!file->is_directory)
								{
									ex_file_got_selected = 1;
								}
							}
							if(ui_usri_mouse_l_double_clicked(file_node_usri))
							{
								ex_file_double_clicked = 1;
							}
						}
					}

				}
			}
			ui_space_specified(ui, 4.0f, 1.0f);
			//process input for naming files.
			ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
			{
				ui_input_text(ui,
						0,
						resource_editor->explorer_process_input,
						game_resource_path_and_name_MAX - 1,
						"resources_explorer_input");
			}
		}


		ui_set_w_em(ui, 8.0f, 1.0f)
		ui_set_h_text(ui, 4.0f, 1.0f)
		ui_set_row(ui)
		{
			ui_space_ppct(ui, 1.0f, 0.0f);
			ex_clicked_ok = ui_button(ui, "ok##__EX_OK__");
			ui_space_specified(ui, 4.0f, 0.0f);
			ex_clicked_cancel = ui_button(ui, "cancel##__EX_CANCEL__");
			ui_space_specified(ui, 4.0f, 0.0f);
		}
		ui_space_specified(ui, 4.0f, 0.0f);
	}
	ui_panel_end(ui);





	//determine the process result depending on the selected file

	resource_explorer_file *selected_file = 0;
	if(resource_editor->explorer_file_is_selected)
	{
		selected_file = resource_editor->explorer_files + resource_editor->explorer_selected_file_index;
	}
	if(selected_file && !selected_file->is_directory && ex_file_got_selected && 
	   resource_editor->explorer_flags & er_explorer_copy_selected_file_name)
	{
		memory_clear(
				resource_editor->explorer_process_input,
				sizeof(resource_editor->explorer_process_input));
		string_copy(selected_file->resource->path_and_name,
				resource_editor->explorer_process_input);
		string_copy(selected_file->resource->path_and_name,
				resource_editor->explorer_output);
	}

	if(ex_clicked_cancel)
	{
		resource_editor->resource_explorer_closed = 1;
		resource_editor->explorer_valid_file_focused = 0;
		resource_editor->explorer_file_is_selected = 0;
		ui_close_panel_ptr(ui, panel);
	}
	if((ex_clicked_ok || ex_file_double_clicked) && selected_file)
	{
		if(selected_file->is_directory)
		{
			resource_editor->explorer_current_dir = selected_file->directory;
			resource_editor->update_path = 1;
		}
		else
		{
			if(resource_editor->explorer_flags & er_explorer_select_file)
			{
					resource_editor->explorer_process_completed = 1;
			}
			else
			{
				resource_editor->explorer_process_completed = 1;
			}
		}
	}
	//for saving
	if(ex_clicked_ok && !(resource_editor->explorer_flags & er_explorer_select_file))
	{
				resource_editor->explorer_process_completed = 1;
	}
	if(resource_editor->explorer_process_completed)
	{
		memory_clear(resource_editor->explorer_output, sizeof(resource_editor->explorer_output));
		if(!string_starts_with(resource_editor->explorer_process_input, resource_editor->root_dir->path_and_name))
		{
			string_copy(resource_editor->root_dir->path_and_name, resource_editor->explorer_output);
		}
		string_add(resource_editor->explorer_process_input,
				resource_editor->explorer_output,
				sizeof(resource_editor->explorer_output));
	}

	if(move_panel)
	{
		vec2 mouse_delta = ui_mouse_delta(ui);
		panel->p.x += mouse_delta.x;
		panel->p.y += mouse_delta.y;
	}
	if(resource_editor->explorer_process_completed && 
	   resource_editor->explorer_flags & er_explorer_close_on_complete)
	{
		resource_editor->resource_explorer_closed = 1;
	}

	resource_editor->explorer_updated_path = 0;
	//bool8 close_on_complete = explorer->flags & ui_explorer_flags_close_on_complete;
	//bool8 copy_selected_file_name = explorer->flags & ui_explorer_flags_copy_selected_file_name;
	//bool8 process_select_file = explorer->process_type & ui_explorer_process_select_file;
	//bool8 process_select_any = explorer->process_type & ui_explorer_process_select_any;
	//b8 process_type_name = explorer->process_type & ui_explorer_process_type_file;


	ui_next_nodes_interaction_only_end(ui);
}

static inline u32
editor_write_composite_resource_header(
		platform_api *platform,
		platform_file_handle file,
		u32 data_offset,
		u8 *path_and_name)
{
	ppse_composite_resource cr_name = {0};
	//generate id
	cr_name.signature = ppse_composite_resource_SIGNATURE;
	cr_name.id = assets_generate_id(path_and_name);
	//copy path and name
	string_copy(path_and_name,
			cr_name.path_and_name);
	platform->f_write_to_file(
			file,
			data_offset,
			sizeof(ppse_composite_resource),
			&cr_name);
	//advance to editor data
	data_offset += sizeof(ppse_composite_resource);
	return(data_offset);
}

static void
er_fill_path_and_name(
		asset_type type,
		u8 *path_and_name,
		u8 *new_path_and_name)
{
	Assert(type > 4);
	string_copy(path_and_name, new_path_and_name);
	u8 *formats[asset_type_COUNT] = _formats_w_dot;

	u32 format_index = type - asset_type_map;
	string_copy(path_and_name, new_path_and_name);
	if(!string_ends_with(new_path_and_name, formats[format_index]))
	{
		string_add(formats[format_index], new_path_and_name, 255);
	}
}

static game_resource_attributes *
editor_resource_copy_attributes(
		s_editor_state *editor_state,
		game_resource_attributes *resource_to_copy,
		b32 reeplace_existing,
		u8 *paste_path_and_name)
{
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;


	u8 new_path_and_name[256] = {0};
	er_fill_path_and_name(resource_to_copy->type, new_path_and_name, paste_path_and_name);
	u32 new_id = assets_generate_id(new_path_and_name);
	game_resource_attributes *result = 0;

	if(new_id != resource_to_copy->id)
	{
		result = er_look_for_resource(editor,
				new_path_and_name);
		if(!result || (result && reeplace_existing))
		{
			if(!result)
			{
				result = editor_resources_get_new_slot(editor_state); 
			}
			*result = *resource_to_copy;
			//set new path and name
			editor_assets_clear_and_fill_path_info(
					result,
					new_path_and_name);
	editor_resources_extract_directory(
			editor_state,
			result);
			//file doesn't exist yet.
			result->load_success = 0;
			result->error_flags |= file_result_needs_creating;
		}
	}
}

static void
editor_resource_check(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	//if(!resource->load_success)
	//{
	//	if(resource->error_flags & file_result_needs_creating)
	//}
}

static void
editor_resource_reload_path(
		s_editor_state *editor_state)
{
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;
}

static void
editor_resource_fill_slot_data(
		s_editor_state *editor_state,
		game_resource_attributes *game_resource,
		asset_type type,
		b32 reeplace_existing,
		u8 *path_and_name)
{
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	//in case of an existing slot
	if(reeplace_existing)
	{
		//keep the slot connections
		game_resource_attributes *next = game_resource->next;
		game_resource_attributes *prev = game_resource->prev;
		game_resource_attributes *next_in_dir = game_resource->next_in_directory;
		game_resource_attributes *prev_in_dir = game_resource->prev_in_directory;
		game_resource_attributes *next_of_type = game_resource->next_of_type;
		game_resource_attributes *prev_of_type = game_resource->prev_of_type;
		memory_clear(
				game_resource, sizeof(game_resource_attributes));
		//restore connections
		game_resource->next = next;
		game_resource->prev = prev;
		game_resource->next_in_directory = next_in_dir;
		game_resource->prev_in_directory = prev_in_dir;
		game_resource->next_of_type = next_of_type;
		game_resource->prev_of_type = prev_of_type;
	}

	b32 needs_composite_resources = resource_editor->resources_info[type].needs_composite_resources;

	if(needs_composite_resources)
	{
		game_resource->error_flags |= file_result_composite_resources_missing;
		game_resource->load_success = 0;
	}
	else
	{
		game_resource->load_success = 1;
	}
	game_resource->type = type;
	game_resource->version = asset_type_version(type);
	//get full path and name
	editor_assets_fill_path_info(
			game_resource, path_and_name);
	editor_resources_extract_directory(
			editor_state,
			game_resource);

	platform_file_handle resource_file = platform->f_open_file(
			game_resource->path_and_name, platform_file_op_read);
	if(resource_file.handle)
	{
		platform_file_min_attributes atr = platform->f_get_file_info(resource_file);
		game_resource->write_time = atr.date;
		platform->f_close_file(resource_file);
	}

}

static game_resource_attributes *
editor_resource_create_slot(
		s_editor_state *editor_state,
		asset_type type,
		u8 *path_and_name)
{
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	game_resource_attributes *game_resource = 0;


	//make sure this resource doesn't exist
	game_resource = er_look_for_resource(
			&editor_state->editor, path_and_name);
	if(!game_resource)
	{
		game_resource = editor_resources_get_new_slot(editor_state);

		editor_resource_fill_slot_data(
				editor_state,
				game_resource,
				type,
				0,
				path_and_name);

	}

	return(game_resource);
}

static void
editor_resource_check_composite_resources(
		s_editor_state *editor_state,
		game_resource_attributes *game_resource)
{
	s_game_editor *editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &editor->asset;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_assets *asset_manager = editor_state->editor_assets;

	platform_file_handle file_handle = platform->f_open_file(
			game_resource->path_and_name,
			platform_file_op_read | platform_file_op_share);
	if(!file_handle.handle)
	{
		game_resource->error_flags = file_result_invalid_handle;
		game_resource->load_success = 0;
		return;
	}
	//self explanatory
	b32 has_composite_resources = 0;
	has_composite_resources = resource_editor->resources_info[game_resource->type].needs_composite_resources;
	//this is a hand-made file format with their
	//common header ppse_asset_header
	//number of expected composite resources if any

	//if it has composite resources, then it hash ppse_asset_header
	if(has_composite_resources)
	{
		ppse_asset_header asset_header = {0};

		platform->f_read_from_file(
				file_handle,
				0,
				sizeof(asset_header),
				&asset_header);
		//fill data
		game_resource->version = asset_header.version;

		{
			if(!game_resource->composite_resources ||
					(asset_header.composite_resource_count > game_resource->composite_resource_count))
			{
				er_free_composite_resources_from_slot(editor_state, game_resource);
				//look for free space on the array
				game_resource->composite_resource_count = asset_header.composite_resource_count;
				game_resource->composite_resources = er_get_composite_resource_slots(
						editor_state, game_resource->composite_resource_count);
			}
			game_resource->composite_resource_count = asset_header.composite_resource_count;

			u32 offset_to_composite_header = asset_header.offset_to_composite_resources;
			for(u32 c = 0; c < asset_header.composite_resource_count; c++)
			{
				ppse_composite_resource composite_resource_name = {0};
				platform->f_read_from_file(
						file_handle,
						offset_to_composite_header,
						sizeof(ppse_composite_resource),
						&composite_resource_name);
				offset_to_composite_header += sizeof(ppse_composite_resource);

				//Assert(composite_resource_name.signature == ppse_composite_resource_SIGNATURE);
				//			read resources header!
				if(composite_resource_name.signature == ppse_composite_resource_SIGNATURE)
				{
					//resource got previously loaded and is valid.
					game_resource_attributes *existing_resource = er_look_for_resource(
							editor, composite_resource_name.path_and_name);

					game_composite_resource	*cr = game_resource->composite_resources + c;
					cr->not_found = 1;
					cr->attributes = existing_resource;
					if(existing_resource)
					{
						cr->not_found = 0;
						string_copy(composite_resource_name.path_and_name, cr->path_and_name);
						if(!existing_resource->load_success)
						{
							game_resource->error_flags |= file_result_composite_resource_load_error;
						}
					}
					else
					{
						game_resource->error_flags |= file_result_composite_resource_not_found;

					}
				}
				else //error
				{
					game_resource->error_flags |= file_result_composite_resource_signature_failed;
				}
			}

			//count the loaded composite resources and fill the needed data for every type
			switch(game_resource->type)
			{
				//nothing for these two
				case asset_type_image: 
				case asset_type_sound: 
					{
					}break;
				case asset_type_tileset:
					{
						//tilesets only have one external texture
						game_resource->tileset.texture = game_resource->composite_resources;

					}break;
				case asset_type_map:
					{
						//for this one, pick the map header from the file
						//and put the counts
						ppse_map_file_header header = {0};
						platform->f_read_from_file(
								file_handle,
								0,
								sizeof(header),
								&header);
						game_resource->map.tileset_count = header.tileset_count;
						game_resource->map.model_count = header.model_count;
						//models go after tilesets, entities go after models...
						game_composite_resource *gcr_array = game_resource->composite_resources;
						game_resource->map.tilesets = gcr_array; 
						game_resource->map.models = gcr_array + game_resource->map.tileset_count;
					}break;
				case asset_type_model:
					{
						//models only have external sprite sheets
						game_resource->model.texture_count = game_resource->composite_resource_count;
						game_resource->model.textures = game_resource->composite_resources;
					}break;
				case asset_type_entity:
					{
						//entities need an external model and "attached" ones
						//the first one is always the model
						game_resource->coso.model = game_resource->composite_resources;
					}break;
				default:
					{
						Assert(0); //you forgot something...
					}break;
			}

		}

		//determine success
	}
	game_resource->load_success = game_resource->error_flags == 0;

	//close file
	platform->f_close_file(
			file_handle);
}

static void
editor_resource_check_for_validation(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	s_game_editor *editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &editor->asset;
	stream_data *info_stream = &editor->info_stream;

	if(resource->error_flags & file_result_composite_resource_load_error)
	{
		b32 keep_error = 0;
		for(u32 c = 0; !keep_error && c < resource->composite_resource_count; c++)
		{
			game_composite_resource *cr = resource->composite_resources + c;
			if(!cr->attributes->load_success)
			{
				keep_error = 1;
			}
		}
		if(!keep_error)
		{
			resource->error_flags ^= file_result_composite_resource_load_error;
		}
	}
	resource->load_success = resource->error_flags == file_result_success;
}

static inline
editor_resource_update_write_time(
		s_editor_state *editor_state,
		game_resource_attributes *game_resource)
{
	platform_api *platform = editor_state->platform;
	//set write time
	platform_file_handle file = platform->f_open_file(
		game_resource->path_and_name,platform_file_op_read);
	platform_file_min_attributes file_attributes = platform->f_get_file_info(
			file);
	game_resource->write_time = file_attributes.date;
	platform->f_close_file(file);
}

static void
editor_resource_fill_type_data(
		s_editor_state *editor_state,
		game_resource_attributes *game_resource)
{
	s_game_editor *editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &editor->asset;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_assets *asset_manager = editor_state->editor_assets;
	//reset error flags and re-scan
	game_resource->error_flags = 0;

	editor_resource_check_composite_resources(editor_state, game_resource);

	game_resource->load_success = game_resource->error_flags == 0;

	//load as asset if everything is okay
	if(game_resource->load_success)
	{

	   game_resource->asset_key = assets_reload_or_allocate(
	    		asset_manager,
	    		game_resource->type,
	    		game_resource->path_and_name);

//	   platform_file_handle file = platform->f_open_file(game_resource->path_and_name,
//			   platform_file_op_read);
	   //might need to split some of these...
		//switch(game_resource->type)
		//{
		//	case asset_type_image:
		//		{
		//		}break;
		//	case asset_type_tileset:
		//		{
		//			game_resource->tileset.texture = game_resource->composite_resources;
		//		}break;
		//	case asset_type_model:
		//		{
		//			game_resource->model.texture_count = game_resource->composite_resource_count;
		//			game_resource->model.textures = game_resource->composite_resources;
		//		}break;
		//	case asset_type_map:
		//		{
		//			game_composite_resource *gcr_array = game_resource->composite_resources;
		//			game_resource->map.tileset_count = 0;
		//			game_resource->map.model_count = 0;
		//			//set tilesets
		//			u32 i = 0;
		//			while(i < game_resource->composite_resource_count)
		//			{
		//				if(gcr_array[i].attributes->type != asset_type_tileset) break;

		//				game_resource->map.tileset_count++;
		//				i++;
		//			}
		//			while(i < game_resource->composite_resource_count)
		//			{
		//				if(gcr_array[i].attributes->type != asset_type_model) break;

		//				game_resource->map.model_count++;
		//				i++;
		//			}
		//			game_resource->map.tilesets = gcr_array; 
		//			game_resource->map.models = gcr_array + game_resource->map.tileset_count;
		//		}break;
		//}
		//platform->f_close_file(file);
	}
}

static void
editor_resources_reimport(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	struct s_game_assets *asset_manager = editor_state->editor_assets;
	editor_resource_update_write_time(editor_state, resource);
	editor_resource_fill_type_data(editor_state, resource);
}

//detects resource type based on the format
static inline asset_type
editor_get_resource_type_by_path_and_name(
		s_game_resource_editor *resource_editor, u8 *path_and_name)
{
	asset_type type = 0;
	//copy data
	union{
		u32 value;
		u8 value8[4];
	}type_value = {0};
	//get extension
	u32 extension_length = path_get_extension(
			path_and_name,
			type_value.value8,
			4);
	//format
#if 0
	type_value.value = FormatEncodeU32(type_value.value);
	//get rid of gaps
	u32 difference = 4 - extension_length;
	while(difference)
	{
		type_value.value >>= 8;
		difference--;
	}
	//compare with types
	switch(type_value.value)
	{
		case 'png': case 'bmp': case'pimg':
			{
				type = asset_type_image;
			}break;
		case 'pptl':
			{
				type = asset_type_tileset;
			}break;
		case 'ppmo':
			{
				type = asset_type_model;
			}break;
		case 'ppmp':
			{
				type = asset_type_map;
			}break;
		case 'pcos':
			{
				type = asset_type_entity;
			}break;
		default:
			{
				type = 0;
			}break;
	}
#else
	if(extension_length)
	{
		for(u32 t = 0;!type && t < asset_type_COUNT; t++)
		{
			game_resource_info *info = resource_editor->resources_info + t;
			if(type_value.value == info->signature_encoded)
			{
				type = info->type;
			}
			else
			{
				for(u32 a = 0;!type && a < info->additional_formats_count; a++)
				{
					if(type_value.value == info->additional_formats[a])
					{
						type = info->type;
					}
				}
			}
		}
	}
#endif
	return(type);
}



static inline game_resource_attributes * 
editor_resources_import_from_path(
		s_editor_state *editor_state,
		u8 *path_and_name)
{
	stream_data *info_stream = &editor_state->editor.info_stream;
	platform_api *platform = editor_state->platform;
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;

	game_resource_attributes *game_resource = er_look_for_resource(
			&editor_state->editor, path_and_name);
	asset_type type = editor_get_resource_type_by_path_and_name(
			resource_editor,path_and_name);
	if(!game_resource && type != 0)
	{
		platform_file_handle file = platform->f_open_file(
				path_and_name, platform_file_op_read);
		if(file.handle)
		{
			platform_file_min_attributes file_attributes = platform->f_get_file_info(
					file);	

			game_resource = editor_resources_get_new_slot(editor_state);

			editor_assets_fill_path_info(
					game_resource,
					path_and_name);
			game_resource->type = type;
			game_resource->version = asset_type_version(type);
			editor_resources_extract_directory(
					editor_state,
					game_resource);

			game_resource->write_time = file_attributes.date;
			platform->f_close_file(file);
			editor_resource_fill_type_data(editor_state, game_resource);
		}
		else
		{
			stream_pushf(info_stream, "Error while importing \"%s\" the file could not be found or opened!", path_and_name);
		}
	}
	else if(game_resource) //already imported
	{
		editor_resources_reimport(editor_state, game_resource);
		stream_pushf(info_stream, "WARNING! the resource \"%s\" was already imported. The existing one will be reeplaced by default.", path_and_name);
	}
	return(game_resource);
}

static inline void
editor_resources_save_imported_resources(
		s_editor_state *editor_state)
{

	platform_api *platform = editor_state->platform;
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	//write new header with resources count
	resource_editor->file_header.total_resources_count = resource_editor->resources_loaded_count;
	platform->f_write_to_file(
			resource_editor->imported_resources_file,
			0,
			sizeof(ppse_editor_resources_header),
			&resource_editor->file_header);

	u32 write_offset = sizeof(ppse_editor_resources_header);

	u32 a = 0;
	for(game_resource_attributes *r = resource_editor->first;
			r;
			r = r->next)
	{
		game_resource_attributes *resource = r;
		resource->index_in_file = a++;
		resource->imported_for_game = 1;
		//write new resource to the import data file
		ppse_editor_resource ppse_new_resource = {0};
		string_copy(resource->path_and_name, ppse_new_resource.source);
		ppse_new_resource.type = resource->type;


		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				write_offset,
				sizeof(ppse_editor_resource),
				&ppse_new_resource);

		write_offset += sizeof(ppse_editor_resource);
	}
}

static inline void
editor_resources_import_and_save()
{
}

static game_resource_attributes * 
editor_resource_create(
		s_editor_state *editor_state,
		asset_type type,
		u32 reeplace_existing,
		u8 *path_and_name)
{
#if 1
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	game_resource_attributes *game_resource = 0;

	platform_file_handle resource_file = {0};
	if(type >= asset_type_map)
	{
		u8 new_path_and_name[256] = {0};
		string_copy(path_and_name, new_path_and_name);
		u8 *formats[asset_type_COUNT] = _formats_w_dot;
		u32 format_index = type - asset_type_map;
//		string_copy(path_and_name, new_path_and_name);
		if(!string_ends_with(new_path_and_name, formats[format_index]))
		{
			string_add(formats[format_index], new_path_and_name, 255);
		}
		//make sure this resource doesn't exist
		game_resource = er_look_for_resource(
				&editor_state->editor, new_path_and_name);
		if(!game_resource || (game_resource && reeplace_existing))
		{
			resource_file = platform->f_open_file(
					new_path_and_name, platform_file_op_create_new);
			if(resource_file.handle)
			{
				ppse_asset_header common_header = {0};
				//write common header
				//common_header.id = assets_generate_id(new_path_and_name);
				common_header.signature = asset_type_signature(type);
				common_header.version = asset_type_version(type);
				platform->f_write_to_file(
						resource_file,
						0,
						sizeof(ppse_asset_header),
						&common_header);
				platform->f_close_file(resource_file);

				//use a new slot 
				if(!game_resource)
				{
					game_resource = editor_resource_create_slot(editor_state, type, new_path_and_name);
				}
				else
				{
					editor_resource_fill_slot_data(editor_state, game_resource, type, 1, new_path_and_name);
				}
			}
		}
	}
	else
	{
		stream_pushf(info_stream, "Error when creating a new asset at %s, expected a value > %u but got a type %u",
				path_and_name, asset_type_map, type);
	}
#endif

	return(game_resource);
}

static void
editor_resource_save(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{

	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	if(resource && !resource->imported_for_game)
	{
		//save individual resource to the file
		resource->imported_for_game = 1;
		resource->index_in_file = resource_editor->file_header.total_resources_count;

		//advance
		u32 write_offset = sizeof(ppse_editor_resources_header) +
			(sizeof(ppse_editor_resource) * resource->index_in_file);

		//write new resource to the import data file
		ppse_editor_resource ppse_new_resource = {0};
		string_copy(resource->path_and_name, ppse_new_resource.source);
		ppse_new_resource.type = resource->type;

		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				write_offset,
				sizeof(ppse_editor_resource),
				&ppse_new_resource);


		//increase count
		resource_editor->file_header.total_resources_count++;
		
		//write new header with resources count
		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				0,
				sizeof(ppse_editor_resources_header),
				&resource_editor->file_header);


	}
}

static game_resource_attributes * 
editor_resource_create_and_save(
		s_editor_state *editor_state,
		asset_type type,
		u32 reeplace_existing,
		u8 *path_and_name)
{

	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	game_resource_attributes *resource = editor_resource_create(
			editor_state, type, reeplace_existing, path_and_name);

	if(resource && !resource->imported_for_game)
	{
		//save individual resource to the file
		resource->imported_for_game = 1;
		resource->index_in_file = resource_editor->file_header.total_resources_count;

		//advance
		u32 write_offset = sizeof(ppse_editor_resources_header) +
			(sizeof(ppse_editor_resource) * resource->index_in_file);

		//write new resource to the import data file
		ppse_editor_resource ppse_new_resource = {0};
		string_copy(resource->path_and_name, ppse_new_resource.source);
		ppse_new_resource.type = resource->type;

		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				write_offset,
				sizeof(ppse_editor_resource),
				&ppse_new_resource);


		//increase count
		resource_editor->file_header.total_resources_count++;
		
		//write new header with resources count
		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				0,
				sizeof(ppse_editor_resources_header),
				&resource_editor->file_header);


	}
	return(resource);
}

static void
editor_assets_scan_data_folder(
		s_editor_state *editor_state,
		u8 *import_path)
{
	//initial needed data
	s_game_editor *editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &editor->asset;
	platform_api *platform = editor_state->platform;
	stream_data *info_stream = &editor->info_stream;
	struct s_game_assets *asset_manager = editor_state->editor_assets;

	u8 *resource_types[] = {
		"*.png",
		"*.bmp",
		"*.ppimg",
		"*.pptl", 
		"*.pcos"
//		"*.ppmo"
	};
	asset_type types[asset_type_COUNT] = {
	asset_type_image,
	asset_type_image,
	asset_type_image,
	asset_type_tileset,
	asset_type_entity,
	};
	u32 format_index = 0;
	u32 format_count = ARRAYCOUNT(resource_types);

	platform_file_scan_work scan_work = platform_file_scan_begin(
			platform,
			&editor->area,
			1,
			import_path);
	stream_pushf(info_stream, "-- Starting to read the game's content folder...");

	while(platform_scanning_directory(&scan_work))
	{
		format_index = 0;
	    while(format_index < format_count)
		{
		    platform_scan_first_file(&scan_work, resource_types[format_index]);
		    while(platform_scanning_next_file(&scan_work))
		    {
		        platform_file_search_info current_file = scan_work.current_file;

				u8 path_and_name[256] = {0}; 
				string_concadenate(
						scan_work.next_full_directory,
						current_file.name,
						path_and_name,
						sizeof(path_and_name));
				
				game_resource_attributes *game_resource = 
				 editor_resource_create_slot(editor_state, types[format_index], path_and_name);
				editor_resource_check_composite_resources(editor_state, game_resource);
		    }
			format_index++;
		}
	}

	//Load them according to their type
	for(game_resource_attributes *r = resource_editor->first;
			r;
			r = r->next)
	{
		editor_resources_reimport(editor_state, r);
	}

	platform_file_scan_end(&scan_work);
}

static void
er_free_resource_reference(
		game_resource_attributes *resource)
{
	if(resource->reference_count)
	{
		resource->reference_count--;
	}
}

static b32 
editor_resources_free_resource(
		s_editor_state *editor_state,
		game_resource_attributes *resource)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	platform_api *platform = editor_state->platform;
	b32 success = 0;

	if(resource->imported_for_game && !resource->reference_count)
	{
		success = 1;
		resource->imported_for_game = 0;
		//wipe name from the imported resources file
		//get the last header of the file
		u32 last_index_on_file = resource_editor->file_header.total_resources_count - 1;
		if(resource_editor->file_header.total_resources_count > 1 &&
		   resource->index_in_file != last_index_on_file)
		{
			//look for the resource that contains this index
			game_resource_attributes *last_resource_on_file = 0;
			//linear search
			for(game_resource_attributes *r = resource_editor->first;
					r && !last_resource_on_file;
					r = r->next)
			{
				if(r->index_in_file == last_index_on_file)
				{
					last_resource_on_file = r;
				}
			}
			u32 current_header_offset = sizeof(ppse_editor_resources_header) +
				sizeof(ppse_editor_resource) * resource->index_in_file; 
			//generate header
			ppse_editor_resource reeplace_header = {0};
			memory_copy(last_resource_on_file->path_and_name,
					reeplace_header.source,
					sizeof(reeplace_header.source));
			reeplace_header.type = last_resource_on_file->type;
			//update index
			last_resource_on_file->index_in_file = resource->index_in_file;
			//reeplace deleted header
			platform->f_write_to_file(
					resource_editor->imported_resources_file,
					current_header_offset,
					sizeof(ppse_editor_resource),
					&reeplace_header);
		}
		resource_editor->resources_loaded_count--;
		//decrease count and update header
		resource_editor->file_header.total_resources_count--;
		platform->f_write_to_file(resource_editor->imported_resources_file,
				0,
				sizeof(ppse_editor_resources_header),
				&resource_editor->file_header);
	}

	if(resource == resource_editor->first)
	{
		resource_editor->first = resource->next;
	}
	else if(resource == resource_editor->last)
	{
		resource_editor->last = resource->prev;
	}
	if(resource->prev)
	{
		resource->prev->next = resource->next;
	}
	if(resource->next)
	{
		resource->next->prev = resource->prev;
	}
	//delete from directory
	game_resources_directory *dir = editor_resources_look_for_directory(resource_editor,
			resource->directory_id);

	//look for a handle who references this resource
	if(dir)
	{
		if(resource == dir->first_resource)
		{
			if(resource->next_in_directory)
			{
				resource->next_in_directory->prev_in_directory = resource->prev_in_directory;
			}
		}
		else if(resource == dir->last_resource)
		{
			if(resource->prev_in_directory)
			{
				resource->prev_in_directory->next_in_directory = resource->next_in_directory;
			}
		}
		else
		{
			resource->next_in_directory->prev_in_directory = resource->prev_in_directory;
			resource->prev_in_directory->next_in_directory = resource->next_in_directory;
		}
		dir->first_resource = dir->first_resource == resource ? 
			resource->next_in_directory : dir->first_resource;
		dir->last_resource = dir->last_resource == resource ? 
			0 : dir->last_resource;
	}

	er_free_composite_resources_from_slot(editor_state, resource);

	resource->composite_resources = 0;
	resource->next_free_resource = resource_editor->first_free_resource;
	resource_editor->first_free_resource = resource;
	return(success);
}

static inline game_resource_attributes * 
editor_resources_get_next_loaded_resource_of_type(
		s_editor_state *editor_state,
		asset_type type,
		u32 *index)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	game_resource_attributes *resource = 0;
	while(*index < resource_editor->resources_count && !resource)
	{
		game_resource_attributes *loop_resource = resource_editor->resources + *index;
		if(loop_resource->load_success && loop_resource->type == type)
		{
			resource = loop_resource;
		}
		(*index)++;
	}
	return(resource);
}

static void
update_model_files(
		s_editor_state *editor_state)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;

	u32 index = 0;
	for(game_resource_attributes *resource = editor_resources_get_next_loaded_resource_of_type(editor_state, asset_type_model, &index);
			resource;resource = editor_resources_get_next_loaded_resource_of_type(editor_state, asset_type_model, &index))
	{
		//temporary_area temp_area = temporary_area_begin(&editor_state->editor.area);
		////read the entire file twice
		//platform_api *platform = editor_state->platform;
		//temporary_area_end(&temp_area);
	}
}

static void
er_free_handle(
		s_editor_state *editor_state,
		game_resource_handle *handle)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	handle->reference_count--;
}

static game_resource_handle *
er_look_for_resource_handle(
		s_editor_state *editor_state,
		game_resource_attributes *r)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	//look for a resource that points to this attribute 
	game_resource_handle *r_handle = 0;
	for(u32 h = 0; h < resource_editor->r_handle_count; h++)
	{
		game_resource_handle *r_handle1 = resource_editor->r_handles + h;
		//found handle without references
		if(r_handle->r == r)
		{
			r_handle = r_handle1;
			break;
		}
	}
	return(r_handle);
}

static game_resource_handle *
er_get_resource_handle_slot(
		s_editor_state *editor_state,
		game_resource_attributes *r)
{
	struct s_game_resource_editor *resource_editor = &editor_state->editor.asset;
	//start by looking for a slot that points to this resource
	game_resource_handle *r_handle = er_look_for_resource_handle(editor_state, r);
	if(!r_handle)
	{
		//look for a free resource that contains no references
		for(u32 h = 0; h < resource_editor->r_handle_count; h++)
		{
			game_resource_handle *r_handle1 = resource_editor->r_handles + h;
			//found handle without references
			if(r_handle1->reference_count == 0)
			{
				r_handle = r_handle1;
				break;
			}
		}
	}
	//no free handle found, so add a new one!
	if(!r_handle)
	{
		if(resource_editor->r_handle_count < resource_editor->r_handle_max)
		{
			r_handle = resource_editor->r_handles + resource_editor->r_handle_count;
			resource_editor->r_handle_count++;
			//clean data
			r_handle->reference_count = 0;
			r_handle->r = r;
		}
	}
	r_handle->r = r;
	r_handle->reference_count++;
	return(r_handle);
}

static void
er_dereference_resource(
		game_resource_attributes *r)
{
	if(r && r->reference_count)
	{
		r->reference_count--;
	}
}

static game_resource_attributes * 
er_reference_resource(
		s_game_editor *editor,
		u8 *path_and_name)
{
	game_resource_attributes *r = er_look_for_resource(
			editor, path_and_name);
	//resource exists!
	if(r)
	{
		r->reference_count++;
	}
	return(r);
}

static game_resource_attributes * 
er_reference_existing_resource(
		game_resource_attributes *r)
{
	//resource exists!
	if(r)
	{
		r->reference_count++;
	}
	return(r);
}
