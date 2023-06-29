typedef enum{
   ui_rc_undefined,
   ui_rc_clear,
   ui_rc_draw_quad,
   ui_rc_push_clip,
   ui_rc_pop_clip,
   ui_rc_draw_locked_vertices,
   ui_rc_switch_viewport,

}gmui_rc; //render commands

typedef struct{
	int s;
}ui_render_commands;


typedef struct {
    vec3 location;
    vec2 uv;
    u32 color;
    u32 texture;
}gmui_render_vertex;

typedef struct ui_platform_render{
	int s;
}ui_platform_render;
static inline uint32
gmui_pack_color_rgba(vec4 color)
{
    uint32 result = (((uint32)color.x << 0) |
                     ((uint32)color.y << 8) |
                     ((uint32)color.z << 16) |
                     ((uint32)color.w << 24));
    return(result);
}

static void
gmui_push_quad(ui_render_commands *commands,
               render_texture *texture,
               vec3 v0,
               vec3 v1,
               vec3 v2,
               vec3 v3,
               vec2 uv0,
               vec2 uv1,
               vec2 uv2,
               vec2 uv3,
               vec4 color)
{

   uint32 color32 = gmui_pack_color_rgba(color);
   uint32 textureId = texture->index;


   //goes in counter-clockwise order starting from the "bottom left" corner
   gmui_render_vertex v0_clipped = {0};
   gmui_render_vertex v1_clipped = {0};
   gmui_render_vertex v2_clipped = {0};
   gmui_render_vertex v3_clipped = {0};


   v0_clipped.location = v0;
   v0_clipped.uv = uv0;
   v0_clipped.color    = color32;
   v0_clipped.texture  = textureId;

   v1_clipped.location = v1;
   v1_clipped.uv       = uv1;
   v1_clipped.color    = color32;
   v1_clipped.texture  = textureId;

   v2_clipped.location = v2;
   v2_clipped.uv       = uv2;
   v2_clipped.color    = color32;
   v2_clipped.texture  = textureId;

   v3_clipped.location = v3;
   v3_clipped.uv       = uv3;
   v3_clipped.color    = color32;
   v3_clipped.texture  = textureId;

}
