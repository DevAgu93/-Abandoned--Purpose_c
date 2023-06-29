#define calc_target_ms(target_framerate) (1.0f / (target_framerate * 2.0f))

static void 
run_frame_timer_total_ms(f32 *timer,
		        u32 *current_frame,
				u32 total_frames,
		        u32 total_miliseconds,
				u32 loop_at_end,
		        f32 dt)
{
    *timer += dt * 1000;
    if(*timer > total_miliseconds)
    {
        (*timer) = 0;
        (*current_frame)++;
    }
	if(*current_frame >= total_frames)
	{
		if(!loop_at_end)
		{
			(*current_frame) = total_frames - 1;
		}
		else
		{
			(*current_frame) = 0;
		}
	}
}
