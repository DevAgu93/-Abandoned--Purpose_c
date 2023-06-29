typedef struct{
	b8 free;
}sw_texture;

typedef struct{
    platform_renderer header;
	u8 *back_buffer;

	u16 texture_array_width;
	u16 texture_array_height;
	u16 texture_count;
	u16 texture_max;
	sw_texture *texture_slots;
	u8 *texture_buffer;
}software_renderer_device;
