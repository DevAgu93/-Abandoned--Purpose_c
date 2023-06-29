#include <gl/gl.h>
#define GL_COLOR_BUFFER_BIT 0x00004000

typedef unsigned int GLenum;
typedef unsigned char GLboolean;
typedef unsigned int GLbitfield;
typedef signed char GLbyte;
typedef short GLshort;
typedef int GLint;
typedef int GLsizei;
typedef unsigned char GLubyte;
typedef unsigned short GLushort;
typedef unsigned int GLuint;
typedef float GLfloat;
typedef float GLclampf;
typedef double GLdouble;
typedef double GLclampd;
typedef void GLvoid;

inline void glClear (GLbitfield mask);
inline void glClearAccum (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
inline void glClearColor (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
inline void glClearDepth (GLclampd depth);
inline void glClearIndex (GLfloat c);
inline void glClearStencil (GLint s);
//Stuff to do
typedef struct{
    platform_renderer header;	
	u32 current_frame_buffer;
}opengl_device;

opengl_device *
opengl_init(memory_area *area,
        HWND windowhand,
		platform_renderer_init_values initial_values,
		platform_renderer_init_functions *init_only_functions)
{
    opengl_device *gl_device = memory_area_push_struct(area, opengl_device);
	return(gl_device);
}
