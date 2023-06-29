static rectangle32s
render_aspect_ratio_fit(uint32 window_w, uint32 window_h, uint32 render_w, uint32 render_h)
{
	rectangle32s result = {0};
	//Si son mayores que sus
	f32 optimal_h = (f32)window_w * ((f32)render_h / render_w);
	f32 optimal_w = (f32)window_h * ((f32)render_w / render_h);

	//if the optimal width goes beyond the display width,
	//then we should keep it as it is and scale the height
	//to keep the ratio
	if(optimal_w > window_w)
	{
		f32 h_space = (window_h - optimal_h) * .5f;
		result.x = 0;
		result.y = (i32)h_space;
		result.w = (i32)window_w;
		result.h = (i32)optimal_h;
	}
	else
	{
		//scale height to the new optimal width
		f32 h_scale = (f32)window_w / render_w;
		//get the remaining empty space and divide it by two in order to offset the final x output
		f32 w_space = (window_w - optimal_w) * .5f;
		result.x = (i32)w_space;
		result.y = 0;
		result.w = (i32)optimal_w;
		result.h = (i32)window_h;
	}
	return(result);
}

static void 
sw_renderer_display_buffer(HDC hdc, platform_renderer *renderer, game_renderer *game_renderer)
{
//	renderer->f_swap_buffers(renderer, game_renderer);
	software_renderer_device *sw_device = (software_renderer_device *)renderer;
	//d3d11_draw_end(r, game_renderer);
	//scissors
	i32 c0 = game_renderer->game_draw_clip.x;
	i32 c1 = game_renderer->game_draw_clip.y;
	i32 c2 = game_renderer->game_draw_clip.w;
	i32 c3 = game_renderer->game_draw_clip.h;
	c0 = 0;
	c1 = 0;
	c2 = game_renderer->back_buffer_width;
	c3 = game_renderer->back_buffer_height;

	//viewport
	i32 v0 = 0;
	i32 v1 = 0;
	i32 v2 = game_renderer->os_window_width;
	i32 v3 = game_renderer->os_window_height;

//	rectangle32s dest_aspect_ratio = renderer_calculate_aspect_ratio(
//			game_renderer->os_window_width, game_renderer->os_window_height,
//			game_renderer->back_buffer_width, game_renderer->back_buffer_height);

	rectangle32s dest_aspect_ratio = render_aspect_ratio_fit(
			game_renderer->os_window_width, game_renderer->os_window_height,
			game_renderer->back_buffer_width, game_renderer->back_buffer_height);

	v0 = dest_aspect_ratio.x;
	v1 = dest_aspect_ratio.y;
	v2 = dest_aspect_ratio.w;
	v3 = dest_aspect_ratio.h;

	BITMAPINFO bm_info = {0};
	BITMAPINFOHEADER *bm_header = &bm_info.bmiHeader;
	bm_header->biSize = sizeof(bm_info.bmiHeader);
	bm_header->biWidth = game_renderer->back_buffer_width;
	bm_header->biHeight = -game_renderer->back_buffer_height;
	bm_header->biPlanes = 1;
	bm_header->biBitCount = 32;
	bm_header->biCompression = BI_RGB;
	bm_header->biSizeImage = 0;
	bm_header->biXPelsPerMeter = 0;
	bm_header->biYPelsPerMeter = 0;

	//clear borders.
	//sides
	PatBlt(hdc, 0, 0, v0, game_renderer->os_window_height, BLACKNESS);
	PatBlt(hdc, v0 + v2, 0, v2, game_renderer->os_window_height, BLACKNESS);
	//top and bottom
	PatBlt(hdc, 0, 0, game_renderer->os_window_width, v1, BLACKNESS);
	PatBlt(hdc, 0, v1 + v3, game_renderer->os_window_width, v3, BLACKNESS);
	SetBkColor(hdc, RGB(0, 0, 0));
	//display
	StretchDIBits(hdc,
			v0, v1, v2, v3,
			c0, c1, c2, c3,
			sw_device->back_buffer,
			&bm_info,
			DIB_RGB_COLORS,
			SRCCOPY);
}
