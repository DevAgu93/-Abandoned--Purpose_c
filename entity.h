static coso_id 
game_get_coso_id(program_state *program);
static void
update_render_entities(program_state *program, program_input *input, game_renderer *game_renderer, render_commands *debug_commands, f32 dt);
static world_entity *
game_create_entity(program_state *program);
static void
game_remove_entity(program_state *program, world_entity *entity);
static world_entity * 
game_create_test_entity(program_state *program);
static world_entity * 
game_create_test_entity2(program_state *program);
static world_entity * 
game_create_new_entity(program_state *program, coso_flags flags);
static world_entity * 
game_create_player_entity(program_state *program);
static void 
request_entity_spawn(program_state *program, entity_spawn_parameters params);
static void 
request_entity_delete(program_state *program, world_entity *entity);
static void
spawn_requested_entities(program_state *program);
static world_entity *
get_child_with_id1(world_entity *parent, u32 id1);

static u32
generate_entity_id(u32 n)
{
	u32 r = kinda_hash_u32(n, 21301);
	return(r);
}
