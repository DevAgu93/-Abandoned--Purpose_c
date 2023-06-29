static world_tileset
test_create_tileset(program_state *program, u8 *image_path);
static game_world
test_allocate_map(memory_area *area, u32 w, u32 h);
static game_world
test_allocate_testing_map(memory_area *area);

static game_world
test_allocate_map(memory_area *area, u32 w, u32 h)
{
	game_world result = {0};
	u32 tile_count = w * h;
	result.tiles = memory_area_push_array(area, world_tile, tile_count);
}

static game_world
test_allocate_testing_map(memory_area *area)
{
	game_world result = {0};
	u32 w = 50;
	u32 h = 50;
	u32 tile_count = w * h;

	result.tiles = memory_area_push_array(area, world_tile, tile_count);
	return(result);
}
