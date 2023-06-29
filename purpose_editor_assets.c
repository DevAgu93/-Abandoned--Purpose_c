
typedef struct{
	ppse_model_header header;
	model *model;
}model_file_updater;

static void
editor_update_model_files(
		s_editor_state *editor_state)
{
//	memory_area *editor_area = &editor_state->editor.area;
//	platform_api *platform = editor_state->platform;
//	u32 model_index = 0;
//	game_resource_attributes *resource = editor_resources_get_next_loaded_resource_of_type(
//			editor_state,
//			asset_type_model,
//			&model_index);
//	model *model = &resource->asset_key->model;
//
//	
//	
//	platform_file_handle file = platform->f_open_file(resource->path_and_name, platform_file_op_edit);
//	//backup the file
//	{
//		u8 resource_bak_name[256] = "data_bak/";
//		string_add(resource->name, resource_bak_name, 256);
//
//		temporary_area temp_file_area = temporary_area_begin(editor_area);
//
//		platform_entire_file entire_file = platform_read_entire_file_handle(platform, file, editor_area);
//
//		platform_file_handle bak = platform->f_open_file(resource_bak_name, platform_file_op_create_new);
//		platform->f_write_to_file(bak, 0, entire_file.size, entire_file.contents);
//		platform->f_close_file(bak);
//
//		temporary_area_end(&temp_file_area);
//	}
//
//	{
//		ppse_model_header model_file_header = {0};
//		platform->f_read_from_file(file, 0, sizeof(model_file_header), &model_file_header);
//	}
//	platform->f_close_file(file);
}


static void
editor_resources_register(
		s_editor_state *editor_state, 
		asset_type type,
		u32 signature,
		b32 needs_composite_resources)
{
	s_game_editor *game_editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &game_editor->asset;

	game_resource_info *resource_info = resource_editor->resources_info + type;
	Assert(resource_info->type == 0);
	resource_info->type = type;
	resource_info->signature = signature;
	resource_info->signature_encoded = FormatEncodeU32(signature);
	resource_info->needs_composite_resources = needs_composite_resources;
	//convert signature to format
	union{
		u32 value;
		u8 value8[4];
	}type_value = {0};
	type_value.value = resource_info->signature_encoded;
	//get rid of white spaces
	while(type_value.value8[0] == '\0')
	{
		type_value.value >>= 8;
	}
	resource_info->signature_encoded = type_value.value;
	//register format for file creation
    memory_copy(type_value.value8, resource_editor->resource_formats[type], 4);
}

static void
er_add_additional_format(
		s_editor_state *editor_state, 
		asset_type type,
		u32 signature)
{
	s_game_editor *game_editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &game_editor->asset;
	game_resource_info *resource_info = resource_editor->resources_info + type;
	Assert(resource_info->type);
	Assert(resource_info->additional_formats_count < 2);
	//make sure the value uses all the bits
	union{
		u32 value;
		u8 value8[4];
	}type_value = {0};
	type_value.value = FormatEncodeU32(signature);
	//get rid of white spaces
	while(type_value.value8[0] == '\0')
	{
		type_value.value >>= 8;
	}
	resource_info->additional_formats[resource_info->additional_formats_count++] = type_value.value;
}


static void
editor_resources_init(
		s_editor_state *editor_state,
		u8 *data_path)
{
	s_game_editor *game_editor = &editor_state->editor;
	memory_area *editor_area = &game_editor->area;
	//register assets
	editor_resources_register(editor_state, asset_type_image, 'pimg', 0);
	er_add_additional_format(editor_state, asset_type_image, 'png');
	er_add_additional_format(editor_state, asset_type_image, 'bmp');
	editor_resources_register(editor_state, asset_type_tileset, 'pptl', 1);
	editor_resources_register(editor_state, asset_type_entity, 'pcos', 1);
	editor_resources_register(editor_state, asset_type_font, 'pfnt', 1);
	editor_resources_register(editor_state, asset_type_map, 'ppmp', 1);
	editor_resources_register(editor_state, asset_type_model, 'ppmo', 1);
//	editor_resources_register(editor_state, asset_type_frame_animation, 'pfan', 1);
	//========================
	// initialize asset editor
	//
	struct s_game_resource_editor *resource_editor = &game_editor->asset;
	resource_editor->content_area = memory_area_create_from(editor_area, KILOBYTES(256));
	resource_editor->resources_max = 200;
	resource_editor->resources = memory_area_push_array(
			editor_area, game_resource_attributes, resource_editor->resources_max);
	resource_editor->resource_explorer_closed = 1;
	resource_editor->composite_resources_max = 400;
	resource_editor->composite_resources_count = 0;
	resource_editor->composite_resources = memory_area_push_array(
			editor_area, game_composite_resource, resource_editor->resources_max);
	//directories
	resource_editor->directories_max = 100;
	resource_editor->directories = memory_area_push_array(editor_area,
			game_resources_directory, resource_editor->directories_max);
	//explorer files
	resource_editor->explorer_file_max = 100;
	resource_editor->explorer_files = memory_area_push_array(editor_area,
			resource_explorer_file, resource_editor->explorer_file_max);
	//resource handles
	resource_editor->r_handle_max = resource_editor->resources_max;
	resource_editor->r_handles = memory_area_push_array(
			editor_area, game_resource_handle, resource_editor->r_handle_max);

	resource_editor->auto_check_files = 1;

	resource_editor->log_buffer_max = KILOBYTES(126);
	resource_editor->log_buffer = memory_area_push_size(editor_area, resource_editor->log_buffer_max);
	//pushes text to log buffer
	resource_editor->log_stream_area = memory_area_create(KILOBYTES(5), 
			memory_area_push_size(editor_area, KILOBYTES(5)));
	resource_editor->log_stream = stream_Create(&resource_editor->log_stream_area);
	//=======================
	platform_api *platform = editor_state->platform;
	resource_editor->imported_resources_file = editor_state->platform->f_open_file(
			"imported_resources.prdt",
			platform_file_op_read | platform_file_op_write | platform_file_op_open_or_create);
	//get header, make sure it exists, else create a new one
	Assert(resource_editor->imported_resources_file.handle);

	platform->f_read_from_file(
			resource_editor->imported_resources_file,
			0,
			sizeof(ppse_editor_resources_header),
			&resource_editor->file_header);
	if(resource_editor->file_header.signature != ppse_editor_resources_SIGNATURE)
	{
		resource_editor->file_header.signature = ppse_editor_resources_SIGNATURE;
		//allocate and save new header
		platform->f_write_to_file(
				resource_editor->imported_resources_file,
				0,
				sizeof(ppse_editor_resources_header),
				&resource_editor->file_header);
		//editor_assets_scan_data_folder(
		//		editor_state,
		//		"data/");
		//editor_resources_save_imported_resources(
		//		editor_state);
	}
	else
	{
		//read resources from file and import
		u32 read_offset = sizeof(ppse_editor_resources_header);
		for(u32 r = 0; r < resource_editor->file_header.total_resources_count; r++)
		{
			ppse_editor_resource resource_source = {0};
			platform->f_read_from_file(resource_editor->imported_resources_file,
					read_offset, sizeof(ppse_editor_resource), &resource_source);
			read_offset += sizeof(ppse_editor_resource);

			game_resource_attributes *imported_resource = editor_resource_create_slot(
					editor_state,
					resource_source.type,
					resource_source.source);
			imported_resource->index_in_file = r;
			imported_resource->imported_for_game = 1;
		}

		//load type data and check composite resources
		for(u32 r = 0; r < resource_editor->resources_count; r++)
		{
			editor_resources_reimport(editor_state, resource_editor->resources + r);
		}
		//fix errors
		for(u32 r = 0; r < resource_editor->resources_count; r++)
		{
			editor_resource_check_for_validation(
					editor_state,
					 resource_editor->resources + r);
			editor_resource_update_write_time(editor_state, resource_editor->resources + r);
			editor_resources_reimport(editor_state, resource_editor->resources + r);
		}
	}

	//set resource explorer root directory
	er_set_root_directory(resource_editor, "data/");
	resource_editor->update_path = 1;
}

static void
_update_tileset_headers(
		platform_api *platform,
		u8 *path_and_name)
{
#if 0 
	ppse_tileset_header old_header = {0};
	ppse_new_tileset_header new_header = {0};
	Assert(sizeof(old_header) == sizeof(new_header));

	platform_file_handle file = platform->f_open_file(
			path_and_name, platform_file_op_edit);

	platform->f_read_from_file(
			file,
			0,
			sizeof(old_header),
			&old_header);


	new_header.header.signature = old_header.signature;
	new_header.header.version = old_header.version;
	new_header.header.composite_resource_count = 1;
	new_header.header.offset_to_composite_resources = old_header.offset_to_image_source;

	new_header.terrain_count = old_header.terrain_count;
	new_header.autoterrain_count = old_header.autoterrain_count;
	new_header.mesh_count = old_header.mesh_count;
	new_header.offset_to_terrain_data = old_header.offset_to_terrain_data;
	new_header.offset_to_image_source = old_header.offset_to_image_source;

	new_header.offset_to_terrain_names = old_header.offset_to_terrain_names;
	new_header.autoterrain_indices_capacity = old_header.autoterrain_indices_capacity;
	new_header.offset_to_autoterrain_data = old_header.offset_to_autoterrain_data;
	new_header.offset_to_autoterrain_names = old_header.offset_to_autoterrain_names;

	platform->f_write_to_file(file,
			0,
			sizeof(new_header),
			&new_header);

	platform->f_close_file(file);
#endif
}

static void
editor_assets_rename_resource(
		s_editor_state *editor_state,
		game_resource_attributes *target_resource)
{
	struct s_game_assets *asset_manager = editor_state->editor_assets;
	s_game_editor *game_editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &game_editor->asset;
	u8 *rename_buffer = resource_editor->rename_buffer;
	stream_data *info_stream = &game_editor->info_stream;

	platform_api *platform = editor_state->platform;

	u8 new_name[32] = {0};
	string_copy(rename_buffer, new_name);
	b32 error = 0;
	b32 move_error = 0;
	b32 id_error = 0;
	b32 length_error = 0;
	u32 new_name_count = string_count(new_name);
	//check if old name will be the same at the end
	if(new_name_count > 1 && !string_compare(new_name, target_resource->name))
	{
		//generate new path and name!
		u8 new_path_and_name[256] = {0};
		u8 new_name_and_type[64] = {0};
		u8 *type = target_resource->file_type;
		//extract old name and get path
		path_fill_directory(target_resource->path_and_name, new_path_and_name);
		//add name
		string_concadenate(".", type, new_name_and_type, 64);
		string_add(new_name, new_path_and_name, 256);
		string_add(new_name_and_type, new_path_and_name, 256);
		u32 new_path_and_name_count = string_count(new_path_and_name);
		if(new_path_and_name_count < sizeof(target_resource->path_and_name))
		{
			b32 already_exists = 0;
			u32 i = 0;
			u32 new_id = assets_generate_id(new_path_and_name);
			//check if this id is avadible
			while(i < resource_editor->resources_count)
			{
				game_resource_attributes *other_resource = resource_editor->resources + i;
				if(other_resource != target_resource &&
						other_resource->id == new_id)
				{
					already_exists = 1;
					break;
				}
				i++;
			}
			//name is avadible!
			if(!already_exists)
			{
				b32 success = platform->f_move_file(target_resource->path_and_name, new_path_and_name);
				if(success)
				{
					u32 old_id = target_resource->id;
					u32 name_length = sizeof(target_resource->name);
					u32 pn_length = sizeof(target_resource->path_and_name);
					//clear names
					memory_clear(target_resource->name, name_length);
					memory_clear(target_resource->path_and_name, pn_length);
					target_resource->id = new_id;
					memory_copy(new_name, target_resource->name, name_length);
					memory_copy(new_path_and_name, target_resource->path_and_name, pn_length);
					//update asset id if loaded
					if(target_resource->asset_key)
					{
						target_resource->asset_key->id = new_id;
					}
					//rename the source on the file if imported
					if(target_resource->imported_for_game)
					{
						ppse_editor_resource target_resource_in_file = {0};
						memory_copy(
								target_resource->path_and_name,
								target_resource_in_file.source,
								sizeof(target_resource_in_file.source));
						target_resource_in_file.type = target_resource->type;
						target_resource_in_file.source;
						u32 rw_offset = sizeof(ppse_editor_resources_header) + 
							(sizeof(ppse_editor_resource) * target_resource->index_in_file);
						editor_state->platform->f_write_to_file(
								resource_editor->imported_resources_file,
								rw_offset,
								sizeof(ppse_editor_resource),
								&target_resource_in_file);
					}

					//the name was changed, now look for composite references
					for(u32 a = 0; a < resource_editor->resources_count; a++)
					{
						//Skip if it's the same
						game_resource_attributes *r = resource_editor->resources + a;
						if(r != target_resource)
						{
							for(u32 c = 0; c < r->composite_resource_count; r++)
							{
								//read composite resource
								game_composite_resource *composite_resource_data = 
									r->composite_resources + c;

								game_resource_attributes *composite_resource = 
									composite_resource_data->attributes;
								//this resources is referenced by another one!
								if(composite_resource && composite_resource == target_resource)
								{
									//temporary?
									platform_file_handle resource_file = 
										platform->f_open_file(r->path_and_name, platform_file_op_edit);
									//get common_header
									ppse_asset_header resource_file_header = ppse_read_common_header_handle(
											platform, resource_file);
									//get composite resource header
									u32 composite_resources_offset = resource_file_header.offset_to_composite_resources;
									u32 composite_resource_count = resource_file_header.composite_resource_count;
									ppse_composite_resource composite_resource_name = {0};
									u32 count = 0;
									while(count < composite_resource_count &&
											composite_resource_name.id != old_id)
									{
										platform->f_read_from_file(
												resource_file,
												composite_resources_offset,
												sizeof(ppse_composite_resource),
												&composite_resource_name);
										if(composite_resource_name.id == old_id)
										{
											//clear old name
											memory_clear(composite_resource_name.path_and_name, sizeof(composite_resource_name.path_and_name));
											//fill data
											memory_copy(
													target_resource->path_and_name,
													composite_resource_name.path_and_name,
													sizeof(composite_resource_name.path_and_name));
											composite_resource_name.id = new_id;
											//write updated header
											platform->f_write_to_file(
													resource_file,
													composite_resources_offset,
													sizeof(ppse_composite_resource),
													&composite_resource_name);

										}

										composite_resources_offset += sizeof(ppse_composite_resource);

										count++;
									}
									//update composite resource path and name
									memory_clear(
											composite_resource_data->path_and_name,
											sizeof(composite_resource_data->path_and_name));
									memory_copy(
											target_resource->path_and_name,
											composite_resource_data->path_and_name,
											sizeof(composite_resource->path_and_name));
									platform->f_close_file(resource_file);
								}
							}
						}
					}
				}
				else
				{
					error = 1;
					move_error = 1;
				}
			}
			else
			{
				error = 1;
				id_error = 1;
			}
		}
		else
		{
			error = 1;
			length_error = 1;
		}
	}
	if(error)
	{
		stream_pushf(info_stream, "Error while renaming \"%s\" to \"%s\"!:",
				target_resource->name, new_name);
		if(id_error)
		{
			stream_pushf(info_stream, "- The new id generated clashes with another asset!");
		}
		else if(move_error)
		{
			stream_pushf(info_stream, "- Access for renaming the file was denied!");
		}
		else if(length_error)
		{
			stream_pushf(info_stream, "- The capacity of the new name exceeds the current one (126)!");
		}
	}
}


static inline void
editor_assets_look_for_file_changes(
		s_editor_state *editor_state)
{
	s_game_editor *editor = &editor_state->editor;
	platform_api *platform = editor_state->platform;
	struct s_game_assets *asset_manager = editor_state->editor_assets;

	struct s_game_resource_editor *resource_editor = &editor->asset;
	if(resource_editor->auto_check_files)
	{
		if(resource_editor->file_check_time > 60)
		{
			resource_editor->file_check_time = 0;
			//compare dates!
			for(u32 a = 0;
					a < resource_editor->resources_count;
					a++)
			{
				game_resource_attributes *resource = resource_editor->resources + a;
				if(resource->error_flags != file_result_success)
				{
					//		continue;
				}
				platform_file_handle resource_file = platform->f_open_file(
						resource->path_and_name,
						platform_file_op_read | platform_file_op_share);
				if(resource_file.handle)
				{
					platform_file_min_attributes file_attributes = platform->f_get_file_info(
							resource_file);
					if(file_attributes.date.value != resource->write_time.value)
					{
						//reload!
						resource->write_time.value = file_attributes.date.value;
						editor_resource_fill_type_data(editor_state, resource);
					}
				}
				else
				{
					//print error
				}
				platform->f_close_file(resource_file);
			}
		}
		resource_editor->file_check_time++;
	}
	
}

static inline void
editor_assets_update_render_ui(
		s_editor_state *editor_state,
		game_renderer *game_renderer,
		editor_input *editor_input,
		f32 dt)
{
	game_ui *ui = editor_state->ui;
	struct s_game_editor *editor = &editor_state->editor;
	struct s_game_resource_editor *resource_editor = &editor->asset;

	if(ui_panel_begin(ui,
				ui_panel_flags_move |
				ui_panel_flags_init_closed,
				400,
				400,
				600,
				600,
				"Content browser"))
	{
		ui_node *resources_folders_node;
		ui_node *resources_list_node;
		ui_node *selected_resource_data;
		ui_set_wh_soch(ui, 1.0f) ui_set_row_n(ui)
		ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
		{
			ui_id import_popup_id = ui_id_from_string("import_popup");
			ui_set_wh_soch(ui, 1.0f)
			{
				ui_popup(ui, import_popup_id)
				{
					b32 clicked_ok = 0;
					u32 text_input_length = 32;
					ui_interaction_only(ui)
					{
						ui_node *region = ui_node_box(ui, 0);
						ui_set_parent(ui, region) ui_set_w_em(ui, text_input_length, 1.0f) ui_set_h_em(ui, 2.0f, 1.0f)
						{
							ui_text(ui, "Import path and name:");
							ui_set_row(ui)
							{
								ui_input_text(ui,
										0,
										resource_editor->import_path,
										text_input_length,
										"import_path_ti");
								ui_set_w_text(ui, 4.0f, 1.0f)
								{
									if(ui_button(ui, "..."))
									{
										ui_explorer_set_process(ui,
												ui_explorer_load, "Select resource to import");
									}
								}
							}
							ui_set_w_text(ui, 4.0f, 1.0f)
							{
								if(ui_button(ui, "Ok"))
								{
									clicked_ok = 1;
								}
								ui_space_specified(ui, 4.0f, 1.0f);
								if(ui_button(ui, "Cancel"))
								{
									ui_popup_close(ui, import_popup_id);
								}
							}
						}
					}
					//try to import
					if(clicked_ok)
					{
						game_resource_attributes *new_imported_resource = editor_resources_import_from_path(editor_state,
									resource_editor->import_path);
						if(new_imported_resource)
						{
							editor_resource_save(editor_state, new_imported_resource);
							ui_popup_close(ui, import_popup_id);
						}
					}
					//check explorer process
					if(ui_explorer_check_process(ui, "Select resource to import"))
					{
						u8 *name = ui->explorer->full_process_path_and_name;
						string_clear(resource_editor->import_path);
						string_copy(name, resource_editor->import_path);
					}
				}
			}
			if(ui_button(ui, "Create new"))
			{
			}
			ui_space_specified(ui, 6.0f, 1.0f);
			if(ui_button(ui, "Import"))
			{
				f32 mx = ui->mouse_point.x;
				f32 my = ui->mouse_point.y;
				ui_popup_open(ui,
						(i16)mx, (i16)my,
						import_popup_id);
			}
			ui_space_specified(ui, 6.0f, 1.0f);
			ui_push_disable_if(ui, resource_editor->browser_selected_resource == 0);
			{
				if(ui_button(ui, "Delete selected"))
				{
					editor_resources_free_resource(
							editor_state,
							resource_editor->browser_selected_resource);
					resource_editor->browser_selected_resource = 0;
				}
			}
			ui_pop_disable(ui);
			ui_checkbox(ui, &resource_editor->auto_check_files, "auto_check_files");

			//space for text at the end
			ui_space_ppct(ui, 1.0f, 0.0f);


			ui_space_specified(ui, 1.0f, 1.0f);
			ui_textf(ui, "Count before reload: %u/60", resource_editor->file_check_time);
		}
		ui_set_w_ppct(ui, 0.5f, 0.0f) ui_set_h_ppct(ui, 1.0f, 0.0f)
		{
			ui_set_row(ui)
			{
				ui_set_w_ppct(ui, 0.3f, 0.0f)
					resources_folders_node= ui_node_box(ui, "resources_directories");

				ui_set_w_ppct(ui, 0.3f, 0.0f)
					resources_list_node = ui_box_with_scroll(ui, "resources_list");
				ui_set_w_ppct(ui, 0.4f, 0.0f)
				selected_resource_data = ui_node_box(ui, "selected_resource_data");
			}
		}
		//directory list
		ui_set_parent(ui, resources_folders_node)
		{
			ui_set_w_ppct(ui, 1.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
			{
				for(u32 d = 0; d < resource_editor->directories_count; d++)
				{
					game_resources_directory *dir = resource_editor->directories + d;
					ui_selectable(ui, 0, dir->name);
				}
			}
		}
		//resource list
		ui_set_parent(ui, resources_list_node)
		{
			for(game_resource_attributes *game_resource = resource_editor->first;
					game_resource;
					game_resource = game_resource->next)
			{
				b32 is_selected = resource_editor->browser_selected_resource ==
					game_resource;

				ui_set_h_text(ui, 4.0f, 1.0f) ui_set_w_ppct(ui, 1.0f, 0.0f)
				{
					if(!game_resource->load_success)
					ui_set_color(ui, ui_color_text, V4(255, 0, 0, 255))
					{
						if(ui_selectablef(ui,
									is_selected,
									"%s",
									game_resource->path_and_name))
						{
							resource_editor->browser_selected_resource = game_resource;
						}
					}
					else
					{
						if(ui_selectablef(ui, is_selected, "%s", game_resource->path_and_name))
						{
							resource_editor->browser_selected_resource = game_resource;
						}
					}
				}
			}
		}
		//show data if avadible
		ui_set_parent(ui, selected_resource_data)
		{
			if(resource_editor->browser_selected_resource)
			{
				game_resource_attributes *game_resource = 
							resource_editor->browser_selected_resource;
				ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
				{
					//errors
					{
						ui_node *error_list_node = 0;
						ui_set_wh_soch(ui, 1.0f) ui_set_interaction_layer(ui, ui_interaction_layer_top)
						{
							error_list_node = ui_create_node(ui, node_clickeable, "Errors##hover");
						}
						ui_set_parent(ui, error_list_node) 
						ui_set_wh(ui, ui_size_text(4.0f, 1.0f))
						ui_set_row(ui)
						{
							ui_text(ui, "Errors");
							if(game_resource->error_flags != 0) ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
							{
								ui_text(ui, "(!)");
							}
							else
							{
								ui_text(ui, "(0)");
							}
						}

						if(ui_node_mouse_hover(ui, error_list_node))
						{
							ui_tool_tip_mouse_begin(ui, "error_list_tooltip");
							{
								ui_node *errors_list = 0;
								ui_set_wh_soch(ui, 1.0f)
								{
									errors_list = ui_node_box(ui, 0);
								}
								ui_set_parent(ui, errors_list)
								{
									if(game_resource->load_success) ui_set_color(ui, ui_color_text, V4(0, 255, 0, 255))
									{
										ui_text(ui, "All good!");
									}
									else
									{
										ui_set_color(ui, ui_color_text, V4(255, 0, 0, 255))
										{
											if(game_resource->error_flags & file_result_invalid_handle)
											{
												ui_text(ui, "- Could not open file (invalid_handle)");
											}
											if(game_resource->error_flags & file_result_signature_error)
											{
												ui_text(ui, "- Signature error (signature_error)");
											}
											if(game_resource->error_flags & file_result_version_error)
											{
												ui_text(ui, "- Version error (version_error)");
											}
											if(game_resource->error_flags & file_result_composite_resource_not_found)
											{
												ui_text(ui, "- Composite resources missing (composite_resource_not_found)");
											}
											if(game_resource->error_flags & file_result_composite_resource_signature_failed)
											{
												ui_text(ui, "- Composite resource signature error (composite_resource_signature_failed)");
											}
											if(game_resource->error_flags & file_result_composite_resource_load_error)
											{
												ui_text(ui, "- One of its needed resources failed to load (composite_resource_load_error)");
											}
										}
									}
								}
							}
							ui_tool_tip_end(ui);
						}
					}

					ui_space_specified(ui, 4.0f, 1.0f);

					ui_text(ui, "Path and name:");
					ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
					{
						ui_text(ui, game_resource->path_and_name);
					}
					ui_text(ui, "Name:");
					ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
					ui_set_w_em(ui, 32.0f, 1.0f) ui_set_h_text(ui, 4.0f, 1.0f)
					{
						if(resource_editor->renaming_resource)
						{
							ui_input_text(ui,
									0,
									resource_editor->rename_buffer,
									sizeof(resource_editor->rename_buffer),
									"Rename resource");
						}
						else
						{
							ui_text(ui, game_resource->name);
						}
					}
					//Confirm and cancel renaming buttons.
					if(resource_editor->renaming_resource)
					{
						ui_push_row(ui, 1.0f, 1.0f);
						{
							if(ui_button(ui, "Confirm#resource_rename"))
							{
								editor_assets_rename_resource(
										editor_state, game_resource);
								resource_editor->renaming_resource = 0;
							}
							if(ui_button(ui, "Cancel#resource_rename"))
							{
								resource_editor->renaming_resource = 0;
							}
						}
						ui_pop_row(ui);
					}
					else
					{
						if(ui_button(ui, "Rename##resource_rename"))
						{
							resource_editor->renaming_resource = 1;
							string_copy(game_resource->name,
									resource_editor->rename_buffer);
						}
					}
					ui_text(ui, "Write time:");
					ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
					{
						u32 hour = pft_hour(game_resource->write_time);
						u32 min = pft_minute(game_resource->write_time);
						u32 sec = pft_seconds(game_resource->write_time);
						u32 ms = pft_ms(game_resource->write_time);
						ui_textf(ui, "%u/%u/%u | %u:%u:%u.%u",
								game_resource->write_time.day,
								game_resource->write_time.month,
								game_resource->write_time.year,
								hour,
								min,
								sec,
								ms
								);
					}
					ui_text(ui, "Type:");
					ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
					{
						u8 *resource_types[asset_type_COUNT] = {
						"Undefined",
						"Image",
						"Font",
						"Sound",
						"Map",
						"Model",
						"Animation",
						"Tileset",
						"Entity"
						};
						ui_text(ui, resource_types[game_resource->type]);
					}
					ui_text(ui, "Type version:");
					u32 type_version = asset_type_version(game_resource->type);
					ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
					{
						ui_textf(ui, "%u", type_version);
					}
					ui_text(ui, "Version:");
					vec4 version_color = {255, 255, 0, 255};
					if(game_resource->error_flags == file_result_version_error)
					{
						version_color.g = 0;
					}
					ui_set_color(ui, ui_color_text, version_color)
					{
						ui_textf(ui, "%u", game_resource->version);
					}
					//per-type information
					switch(game_resource->type)
					{
						case asset_type_tileset:
							{
								ui_text(ui, "Texture :");

								if(game_resource->tileset.texture)
								{
									ui_set_color(ui, ui_color_text, V4(255, 255, 0, 255))
									{
										ui_text(ui, game_resource->tileset.texture->path_and_name);
									}
								}
								else
								{
									ui_set_color(ui, ui_color_text, V4(255, 0, 0, 255))
									{
										ui_text(ui, "None");
									}
								}
							}break;
					}
					//display composite resources
					if(game_resource->composite_resource_count)
					{
						ui_node *composite_resources_title = 0;
						ui_node *composite_resources_list = 0;
						ui_set_w_ppct(ui, 1.0f, 0.0f)
						{
							ui_set_h_text(ui, 4.0f, 1.0f)
								composite_resources_title = ui_label_ex(ui, node_text | node_text_centered, "Composite resources#label");
							ui_set_h_ppct(ui, 1.0f, 0.0f)
								composite_resources_list = ui_node_box(ui, 0);
						}

						//show list of composite resources
						ui_set_parent(ui, composite_resources_list)
						for(u32 c = 0; c < game_resource->composite_resource_count; c++)
						{
							game_composite_resource *composite_resource = 
								game_resource->composite_resources + c;
							if(ui_selectablef(ui, 0, "%s##composite_resource%u",
										composite_resource->path_and_name, c))
							{
								/*
								   game_resources_directory *dir = resources_editor->first_directory;
								   while(dir)
								   {
								       if(dir->first)
									   {
									       dir = dir->first;
									   }
									   else if(dir->next)
									   {
									       dir = dir->next;
									   }
									   else
									   {
									       do
										   {
										       dir = dir->parent;
										   }
									       while(dir && !dir->next)
										   if(dir)
										   {
										       dir = dir->next;
										   }
									   }
								   }
								*/
							}
						}
					}

				}
				switch(game_resource->type)
				{
					case asset_type_image:
						{
							ui_set_wh(ui, ui_size_specified(512, 1.0f))
							{
								vec2 uv0, uv1, uv2, uv3;
								render_fill_uvs_counter_cw(&uv0, &uv1, &uv2, &uv3);
								ui_image_uvs(ui,
										&game_resource->asset_key->image,
										uv0,
										uv1,
										uv2,
										uv3);
							}
						}break;
				}
			}

		}
	}
	ui_panel_end(ui);
}

