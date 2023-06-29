
static void
detect_near_entities(struct s_game_state *program, struct world_entity *entity);

static void
detect_near_entities(struct s_game_state *program, struct world_entity *entity)
{
	//use flags if wanting detection of another struct ?
	program->detections_count = 0;
	for(world_entity *ent0 = program->first_entity; ent0; ent0 = ent0->next)
	{
//		ent0->detections_at = program->detections_count;
		ent0->detections_count = 0;
		ent0->detections = program->detections + program->detections_count;

		for(world_entity *ent1 = program->first_entity; ent1; ent1 = ent1->next)
		{
			if(ent0 == ent1 || (!ent0->body || !ent1->body)) break;

			vec3 p0 = ent0->body->p;
			vec3 p1 = ent1->body->p;

			vec3 dist = vec3_sub(p1, p0);
			f32 distance = vec3_inner_squared(dist);

			cosos_detection *detection = program->detections + program->detections_count;
			detection->distance_squared = distance;
			//detection->id0 = ent0->id;
			//detection->id1 = ent1->id;
			detection->id0 = ent0;
			detection->id1 = ent1;
			detection->side_x = dist.x > 0 ? 1 : dist.x < 0 ? -1 : 0;
			detection->side_y = dist.y > 0 ? 1 : dist.y < 0 ? -1 : 0;
			cubes_overlap_result overlap = cubes_overlap(
					p0, ent0->body->shape.size,
					p1, ent1->body->shape.size);
			detection->overlap = 0;
			if(overlap.side_value == 0)
			{
				detection->overlap = 1;
			}

			program->detections_count++;
			ent0->detections_count++;
			Assert(program->detections_count < program->detections_max);
		}
	}
}
