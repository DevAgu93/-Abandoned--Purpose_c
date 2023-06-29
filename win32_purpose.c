//void _fltused() {}

//#define WIN32_LEAN_AND_MEAN

#include "purpose_crt.h"
#include <windows.h>

#include <win32_virtual_keys.h>
#include "global_definitions.h"
#include <agu_random.h>
#include "purpose_math.h"
#include "global_all_use.h"
#include "purpose_memory.h"
#include "purpose_stream.h"
#include "purpose_global.h"
#include "purpose_render.h"
#include "purpose_render.c"
#include "purpose_platform.h"
#include "purpose_console.h"
#include "purpose_ui.h"
#include "purpose_ui.c"
#include "purpose_platform_layer.h"
#include "purpose_ui_win32.c"
#include "win32_render.h"


#include "purpose_render_platform.c"
//#include "win32_d3d11.c"

#include <platform_win32.c>

u64 global_performance_frequency;

//#include "purpose.c"

typedef struct {
    FILETIME dllwritetime;
    HMODULE gamedll;
    GameUpdateAndRender *UpdateAndRender;
}game_code;

inline FILETIME
GetLastFileTime(char *filename)
{
  FILETIME creationtime = {0};
  WIN32_FILE_ATTRIBUTE_DATA Filedata;
  int success = GetFileAttributesExA(filename, GetFileExInfoStandard, &Filedata);
//  Assert(success);

  creationtime = Filedata.ftLastWriteTime;
  return(creationtime);
}
inline uint32 
GetFSize(uint8 *filename)
{
  WIN32_FILE_ATTRIBUTE_DATA Filedata;
  int success = GetFileAttributesExA(filename, GetFileExInfoStandard, &Filedata);
  return(Filedata.nFileSizeLow);
}

#if 0
static platform_renderer * 
win32_initialize_opengl(
		memory_area *area,
		HWND hwnd,
		game_renderer *renderer,
		u32 texture_array_w,
		u32 texture_array_h,
		u32 texture_capacity,
		u32 max_quad_size,
		u32 back_buffer_w,
		u32 back_buffer_h)
{
	HDC device_context = GetDC(hwnd);
	HGLRC opengl_device_context = wglCreateContext(device_context);
	if(!opengl_device_context)
	{
		Assert(0);
	}
	wglMakeCurrent(device_context, opengl_device_context);
	ReleaseDC(hwnd, device_context);

	platform_renderer *p_renderer = opengl_init(
		area,
		hwnd,
		renderer,
		texture_array_w,
		texture_array_h,
		texture_capacity,
		max_quad_size,
		back_buffer_w,
		back_buffer_h);

	return(p_renderer);
}
#endif


static game_code 
win32_load_game_code(char *game_dll_name, char *game_temp_dll_name)
{
	//make a copy of the main game dll (only dev mode)
    CopyFile(game_dll_name, game_temp_dll_name, 0);

	//load the game code and select an empty function for update_and_render
	//in case the code doesn't work (crash on not dev mode).
    HMODULE code = LoadLibrary(game_temp_dll_name);
    game_code gamecode = {0};
    gamecode.UpdateAndRender = GameUpdateAndRenderStub;

    if(code)
    {
        gamecode.dllwritetime = GetLastFileTime(game_temp_dll_name);
        gamecode.gamedll = code;
        gamecode.UpdateAndRender = (GameUpdateAndRender *)GetProcAddress(code, "GameProgress");
    }
    return gamecode;
}
static void
UnloadGamecode(game_code *gamecode)
{
   gamecode->UpdateAndRender = GameUpdateAndRenderStub;
   if(gamecode->gamedll)
   {
       FreeLibrary(gamecode->gamedll);
   }
}

inline uint64 win32_get_performance_frequency()
{
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    return li.QuadPart;
}
inline uint64 win32_get_performance_counter()
{
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    return li.QuadPart;
}

inline float 
win32_get_ms_elapsed(u64 start, u64 end)
{
    return (f32)(end - start) / (f32)global_performance_frequency;
}

#if 0
static inline POINT
win32_get_mouse_screen_coordinates()
{
	POINT m_point;
	GetCursorPos(&mousePoint);
	return(m_point);
}

static inline void
win32_convert_to_client_coordinates(POINT m_point)
{
	ScreenToClient(hwnd, &mousePoint);
}
#endif
#define WM_LEFTBUTTONDOWN 0x0201

static inline uint32 
win32_read_msgs(MSG *msgs,
                HWND hwnd,
                program_input *gameInput, 
				s_game_memory *game_memory,
				u32 *is_tracking_mouse,
                int8* running)
{


	platform_api *platform = game_memory->platformApi;
	 u32 wdd = gameInput->doubleClickLeft;
	 gameInput->doubleClickedLeft = 0;
	 gameInput->doubleClickLeft = 0;
	 gameInput->tripleClickLeft = 0;
	 //gameInput->doubleClickedRight = 0;
     while(PeekMessage(msgs, 0, 0, 0, PM_REMOVE))
     {
           //ReadMsg(msgs.message, &running);
		 ui_win32_read_event(game_memory->ui, msgs);
           switch(msgs->message)
           {
             case WM_SYSKEYDOWN:
             case WM_SYSKEYUP:
             case WM_KEYDOWN:
             case WM_KEYUP:
             {
                 WPARAM keycode = msgs->wParam;
                 //NOTE: 4 are the number of keys at the moment.
                 //W and L params are 32 bit unsigned integers on x86.
                 uint32 IsDown  = (msgs->lParam & (1UL << 31)) == 0;
                 uint32 WasDown = (msgs->lParam & (1UL << 30)) != 0;
				 //Sends WM_CHAR with a translated char
				 TranslateMessage(msgs);

				 //
				 //;Process game input
				 //
                 if(IsDown != WasDown)
                 {
                     input_button *button = 0;
					 program_input *newKeyboardInput = gameInput;
                    switch(msgs->wParam)
                    {
                      case VK_LEFT:
                          {
                            button = &newKeyboardInput->left;
                          }break;
                      case VK_RIGHT:
                          {
                            button = &newKeyboardInput->right;
                          }break;
                      case VK_UP:
                          {
                            button = &newKeyboardInput->up;
                          }break;
                      case VK_DOWN:
                          {
                            button = &newKeyboardInput->down;
                          }break;
                      case VK_X:
                          {
                            button = &newKeyboardInput->attack;
                          }break;
                      case VK_Z:
                          {
                            button = &newKeyboardInput->jump;
                          }break;
                    }
                    if(button)
                    {
					  if(button->was_down != IsDown)
                   	  {
                   	      button->transition_count++;
                   	      button->was_down = IsDown;
                   	  }
                    }

					if(msgs->wParam == VK_SPACE)
					{
						gameInput->spaceBarDown = IsDown;
					}
					if(msgs->wParam == VK_MENU)
					{
						gameInput->alt = IsDown;
					}
                }
				if(IsDown)
				{
					if(msgs->wParam >= VK_F1 && msgs->wParam <= VK_F12)
					{
						uint64 fI = msgs->wParam - VK_F1;
						gameInput->f_keys[fI] = 1;
					}
					if(msgs->wParam >= 'A' && msgs->wParam <= 'Z')
					{
					}
				}

				//Just process the state.
				if(msgs->wParam == VK_SHIFT)
				{
					gameInput->shift_l = IsDown;
				}
				if(msgs->wParam == VK_CONTROL)
				{
					gameInput->ctrl_l = IsDown;
				}
				if(msgs->wParam == KEY_CODE_ENTER)
				{
					gameInput->enter = IsDown;
				}
             }break;
			 case WM_CHAR:
			 {

			 }break;
			 case WM_MOUSEWHEEL:
			 {
				 gameInput->mouse_wheel = ((int16)((msgs->wParam >> 16) & 0xffff)) / 120;

#if 0
				 uint8 buffer[128];
				 FormatText(buffer, sizeof(buffer), "Mouse wheel value is:%d with%u\n" , mouseWheelDir, msgs->wParam);
			     OutputDebugString(buffer);
#endif
			 }break;
			 case WM_LBUTTONUP:
			 case WM_RBUTTONUP:
			 {
				 u32 messageTime     = GetMessageTime();
	             u32 doubleClickTime = GetDoubleClickTime();

				 u32 doubleClicked      = (messageTime < (gameInput->lastDoubleClickTime + doubleClickTime));
				 //Doesn't check if left yet
				 if(doubleClicked)
				 {
					 gameInput->doubleClickedLeft = 1;
				 }

			 }
			 break;
			 case WM_LBUTTONDOWN:
			 case WM_RBUTTONDOWN:
			 {

				 //This process can also be done by saving the last click time and current on gameInput

				 //used to compare the time it took since last click
				 u32 messageTime     = GetMessageTime();
	             u32 doubleClickTime = GetDoubleClickTime();

				 u32 currentClickIsLeft = msgs->message == WM_LBUTTONDOWN;
				 u32 doubleClicked      = (messageTime < (gameInput->lastClickTime + doubleClickTime));
				 u32 tripleClicked      = (messageTime < (gameInput->lastDoubleClickTime + doubleClickTime));

				 u32 leftDoubleClick    = doubleClicked && (currentClickIsLeft && gameInput->lastClickWasLeft);
				 u32 leftTrippleClick   = tripleClicked && (currentClickIsLeft && gameInput->lastClickWasLeft);

				 gameInput->lastClickWasLeft  = currentClickIsLeft;
				 gameInput->doubleClickLeft   = leftDoubleClick;
				 gameInput->tripleClickLeft  = leftTrippleClick;
				 gameInput->lastClickTime     = messageTime;
				 if(leftDoubleClick)
				 {
					 gameInput->lastDoubleClickTime = messageTime;
				 }

				 if(leftTrippleClick)
				 {
					 gameInput->lastDoubleClickTime = 0;
				 }

			 }break;
			 case WM_MOUSEMOVE:
			 {
				 //int s = 0;
				 if(!(*is_tracking_mouse))
				 {
                     TRACKMOUSEEVENT mouse_event = {0};
				     mouse_event.cbSize = sizeof(TRACKMOUSEEVENT);
				     mouse_event.dwFlags         = TME_LEAVE;
				     mouse_event.hwndTrack = hwnd;
                     TrackMouseEvent(&mouse_event);

					 *is_tracking_mouse = 1;
				 }
				 platform->mouseAtWindow = 1;

			 }break;
			 case WM_MOUSELEAVE:
			 {
				 platform->mouseAtWindow = 0;
				 *is_tracking_mouse       = 0;
			 }break;
             case WM_QUIT:
             {
                *running = 0;
             }return 0;
             case WM_ACTIVATE:
             {
				 //;Remove??
                 int x = 0;
             }break;
             default:
             {
                TranslateMessage(msgs);
                DispatchMessage(msgs);
             }
           }

    }
    
    return 1;
}
//needed globals for the win32 message processing, since 
//WM_SIZE and WM_DISPLAYCHANGE won't be readed by win32_read_msgs.
static uint32 global_win32_client_w;
static uint32 global_win32_client_h;

static uint32 global_win32_display_width;
static uint32 global_win32_display_height;
LRESULT CALLBACK Win32ProcessMsg(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    switch(umsg)
    {
        case WM_QUIT:
            {
            }break;
        case WM_DESTROY:
            {
                PostQuitMessage(0);
            }break;
        case WM_CLOSE:
            {
                DestroyWindow(hwnd);
            }break;
        case WM_SIZE:
            {
              RECT clientsz = {0};
              GetClientRect(hwnd, &clientsz);
              if(clientsz.right * clientsz.bottom)
              {
                global_win32_client_w  = clientsz.right - clientsz.left;
                global_win32_client_h = clientsz.bottom - clientsz.top;
              }

            }break;
		case WM_DISPLAYCHANGE:
			{
                global_win32_display_width  = (u32)lparam;
                global_win32_display_height = (u32)(lparam >> 32);
			}break;
        default:
            {
              return DefWindowProc(hwnd, umsg, wparam, lparam);
            }
    }
    return 1;
}

//TODO: Make this compatible with multithread!
void __stdcall WinMainCRTStartup()
{
    char *cmdLine = GetCommandLineA();
    int Result = WinMain(GetModuleHandle(0), 0, cmdLine, 0);
    ExitProcess(Result);
}

static inline void
renderer_allocate()
{
}

//main
int WINAPI WinMain(HINSTANCE hinstance,
                   HINSTANCE hprevinstance,
                   LPSTR pCmdLine,
                   int nCmdShow)
{

    const char WindowName[] = "Another window!";
    WNDCLASS wnd = {0};
	HRESULT granular_time = timeBeginPeriod(1);
    
    wnd.lpfnWndProc = Win32ProcessMsg;
    wnd.hInstance = hinstance;
    wnd.lpszClassName = WindowName;


    RegisterClass(&wnd);

    HWND hwnd = CreateWindow(WindowName,
                   "Project Purpose",
                   WS_OVERLAPPEDWINDOW,
                   CW_USEDEFAULT,
                   CW_USEDEFAULT,
                   1200,
                   675,
                   0,
                   0,
                   hinstance,
                   0);
    if(!hwnd) 
    {
        return 0;
    }

    ShowWindow(hwnd, SW_SHOW);
	global_performance_frequency = win32_get_performance_frequency();

	//initial platform data
    platform_api platform = platform_initialize();
	//to detect mouse inside client
	u32 is_tracking_mouse  = 0;


    char *game_dll_name      = "purpose.dll";
    char *game_temp_dll_name = "purpose_temp.dll";
    game_code gamecode = win32_load_game_code(game_dll_name, game_temp_dll_name);

	//Select the total memory needed for the game.
    uint32 gameMemorySize = 1024*1024*96;
    memory_area main_game_memory_area = memory_area_create(gameMemorySize, win32_virtual_alloc(gameMemorySize));
    s_game_memory game_memory = {0};
    game_memory.main_memory = &main_game_memory_area;
    game_memory.platformApi = &platform; 

    HDC hdc = GetDC(hwnd);
    global_win32_display_width  = GetDeviceCaps(hdc, HORZRES);
    global_win32_display_height = GetDeviceCaps(hdc, VERTRES);
    //NOTE(Agu): This has the chance of not working. 
    i32 monitor_hz = GetDeviceCaps(hdc, VREFRESH);
	i32 game_framerate = 60;
	if(!monitor_hz)
	{
		monitor_hz = 60;
	}
	if(game_framerate < monitor_hz)
	{
		game_framerate = monitor_hz / 2;
	}

    real32 target_ms = 1.0f / game_framerate;
    real32 elapsed_ms = target_ms;
    real32 delta_time = 0;

	game_memory.target_ms = target_ms;
//
// Renderer setup
//
	u16 init_w[4] = {
		1920,
		1680,
		640,
		1200
	};
	u16 init_h[4] = {
		1080,
		945,
		480,
		675
	};
    game_renderer game_renderer = {0};
    game_renderer.back_buffer_width = init_w[0];
    game_renderer.back_buffer_height = init_h[0];

    game_renderer.texture_array_w = 512;
    game_renderer.texture_array_h = 512;
    game_renderer.texture_array_capacity = 20;
    game_renderer.max_quad_draws = 150000;

    platform_renderer *renderer = win32_allocate_graphics(
			&main_game_memory_area,
			hwnd,
			&game_renderer,
			render_api_d3d11);
    
	//Allocate a scissor buffer
	game_renderer.scissor_total_count = 42;
	game_renderer.scissor_stack = memory_area_push_array(
			&main_game_memory_area,
			render_scissor,
			game_renderer.scissor_total_count );
	//Allocate render commands.
	game_renderer.render_commands_buffer_size = MEGABYTES(4);
    game_renderer.render_commands_buffer_base   = memory_area_push_size(&main_game_memory_area, game_renderer.render_commands_buffer_size);
    game_renderer.render_commands_buffer_offset = game_renderer.render_commands_buffer_base;

    game_renderer.clear_color[0] = 0.25f;
    game_renderer.clear_color[1] = 0.75f;
    game_renderer.clear_color[2] = 1.0f;
    game_renderer.clear_color[3] = 1.0f;
  //  game_renderer.CameraProjection = ProjectionOrthographic(16, 9, 1, 1000.0f); 

    game_renderer.white_texture = renderer_allocate_white_texture(&game_renderer, renderer, &main_game_memory_area);


#if 0
    renderer_AllocateTexture(&game_renderer, renderer ,gameState->game_asset_manager, &platform, 'TSH0', 1);
    renderer_AllocateTexture(&game_renderer, renderer ,gameState->game_asset_manager, &platform, 'PF00', 3);
    renderer_AllocateTexture(&game_renderer, renderer ,gameState->game_asset_manager, &platform, 'CCAT', 4);
#endif
//    render_GetTexture(&game_renderer, renderer ,gameState->game_asset_manager, 'FP01', 5);




    program_input new_game_input = {0};
    program_input old_game_input = {0};

    uint32 mouse_button_count    = ARRAYCOUNT(new_game_input.mouse_buttons);
    uint32 keyboard_button_count = ARRAYCOUNT(new_game_input.buttons);

    MSG msgs = {0};
    int8 running = 1;

    uint64 last_performance_counter = win32_get_performance_counter();
	u64 last_cycle_count = __rdtsc();
	u64 current_cycle_count = __rdtsc();
    //MELOOP

	//TODO(Agu):Remove
	//File group searching
#if 0
	WIN32_FIND_DATA fileFound;
	HANDLE result = FindFirstFileA("data/images/*.png", &fileFound);
	if(result != INVALID_HANDLE_VALUE)
	{
		int32 nextFileFound = 0;
	   do
	  {
	  	 nextFileFound = FindNextFileA(result, &fileFound);
      }while(nextFileFound);

	}
	int32 x = 0;

	FindClose(result);

#endif
	//allocate ui
	game_memory.ui = ui_initialize(
			hwnd,
			memory_area_push_size(&main_game_memory_area, MEGABYTES(1)),
			MEGABYTES(1),
			0);
    while(running)
	{

		target_ms = game_memory.target_ms;
		//reload game code from dll after compiling
		game_memory.dll_reloaded = 0;
		FILETIME dllwritetime = GetLastFileTime(game_dll_name);
		//returns 0 if filetimes are equal, otherwise returns either -1, 1
		if(CompareFileTime(&gamecode.dllwritetime, &dllwritetime))
		{
			game_memory.dll_reloaded = 1;
			UnloadGamecode(&gamecode);
			gamecode = win32_load_game_code(game_dll_name, game_temp_dll_name);
		}
		//Always set default arrow icon
		//these two functions can be changed in case of a custom cursor.
		HCURSOR cursorImage = LoadCursor(0, IDC_ARROW);
		SetCursor(cursorImage);

		old_game_input = new_game_input;
		program_input *old_keyboard_input = &old_game_input;
		//Make sure to not process input if window is inactive.
		int32 window_is_active = GetActiveWindow() != 0;
		//ui input
		//ui_win32_begin(
		//		game_memory.ui,
		//		window_is_active,
		//		game_renderer.back_buffer_width,
		//		game_renderer.back_buffer_height
		//		);

		//game input
		for(uint32 k = 0; k < keyboard_button_count; k++)
		{
			new_game_input.buttons[k].transition_count = 0;
			new_game_input.buttons[k].was_down = 
				old_keyboard_input->buttons[k].was_down * window_is_active;
		}
		new_game_input.mouse_wheel = 0; 
		//reset f0...f11 keys
		for(u32 f_i = 0;
				f_i < 12;
				f_i++)
		{
			new_game_input.f_keys[f_i] = 0;
		}

		ui_win32_reset_input(
				game_memory.ui
				);
		//read message events from windows
		win32_read_msgs(&msgs,
				hwnd,
				&new_game_input,
				&game_memory,
				&is_tracking_mouse,
				&running);
		//stop game
		if(msgs.message == WM_QUIT)
		{
			running = 0;
		}

		ui_win32_update_input(
				game_memory.ui,
				game_renderer.back_buffer_width,
				game_renderer.back_buffer_height
				);

		//I didn't know I had this *-*
		game_renderer.os_window_width  = global_win32_client_w;
		game_renderer.os_window_height = global_win32_client_h;

		game_renderer.display_width  = global_win32_display_width;
		game_renderer.display_height = global_win32_display_height;

		//get mouse at screen coordinates
		POINT mousePoint;
		GetCursorPos(&mousePoint);
		//Convert mouse to client, ignoring title and bars.
		ScreenToClient(hwnd, &mousePoint);

		//get mouse at frame buffer coordinates
		new_game_input.mouse_x_last  = new_game_input.mouse_x;
		new_game_input.mouse_y_last  = new_game_input.mouse_y;

		vec2 new_mouse_point = renderer_convert_to_frame_buffer_coordinates(
				&game_renderer,
				global_win32_client_w,
				global_win32_client_h,
				(f32)mousePoint.x,
				(f32)mousePoint.y);
		new_game_input.mouse_x = new_mouse_point.x;
		new_game_input.mouse_y = new_mouse_point.y;

		//get mouse at game screen coordinates
		new_game_input.mouse_clip_x_last  = new_game_input.mouse_clip_x;
		new_game_input.mouse_clip_y_last  = new_game_input.mouse_clip_y;

		vec2 new_mouse_clip_point = renderer_convert_to_clip_coordinates(
				&game_renderer,
				global_win32_client_w,
				global_win32_client_h,
				(f32)mousePoint.x,
				(f32)mousePoint.y);
		new_game_input.mouse_clip_x = new_mouse_clip_point.x;
		new_game_input.mouse_clip_y = new_mouse_clip_point.y;


		//handle mouse input states
		for(uint32 b = 0; b < mouse_button_count; b++)
		{
			new_game_input.mouse_buttons[b].transition_count = 0;
			new_game_input.mouse_buttons[b].was_down =
				old_game_input.mouse_buttons[b].was_down;
		}
        //High order bit of 16 bits
		uint32 mouse_l_state = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
		uint32 mouse_r_state = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;
		uint32 mouse_m_state = (GetKeyState(VK_MBUTTON) & (1 << 15)) != 0;

		input_button *mouse_button_ptr = &new_game_input.mouse_left;    
		//Mouse left switched state
		if(mouse_l_state != mouse_button_ptr->was_down)
		{
			mouse_button_ptr->was_down = mouse_l_state != 0;
			mouse_button_ptr->transition_count++;
		}
		mouse_button_ptr = &new_game_input.mouse_right;
		if(mouse_r_state != mouse_button_ptr->was_down)
		{
			mouse_button_ptr->was_down = mouse_r_state != 0;
			mouse_button_ptr->transition_count++;
		}
		new_game_input.mouse_middle    = mouse_m_state != 0;

		platform.window_is_focused = (uint16)window_is_active;
		game_renderer.game_draw_clip = renderer_calculate_aspect_ratio(global_win32_client_w,
				global_win32_client_h,
				game_renderer.back_buffer_width,
				game_renderer.back_buffer_height);
		//Set the calculated game clip as the default
		game_renderer.current_draw_clip = game_renderer.game_draw_clip;


#if 1
		//GAME UPDATE
		gamecode.UpdateAndRender(delta_time,
				&game_memory,
				&new_game_input,
				&game_renderer);
#endif
		renderer_check_requested_textures(
				renderer,
				&game_renderer);
		renderer_draw_end(renderer, &game_renderer);

		//get the current ms elapsed and make sure to update it correctly with the game's framerate
#if 1
		f32 end_target_ms = win32_get_ms_elapsed(last_performance_counter, win32_get_performance_counter());
		if(end_target_ms < target_ms)
		{
			//re-calculate and scale it to get a correct sleep time for the cpu
#if 1

			if(granular_time == TIMERR_NOERROR)
			{
				f32 tms = target_ms / 1.0f; 
				f32 ms_elapsed = (1000 * (tms - end_target_ms));
				i32 slptime  = (i32)ms_elapsed - 0;

				if(slptime > 0)
				{
					Sleep(slptime);
				}
			}
			//re-calculate and make sure the miliseconds elapsed gets to the target_ms
//			while(win32_get_ms_elapsed(last_performance_counter, win32_get_performance_counter()) < target_ms);
			while(end_target_ms < target_ms)
			{
				end_target_ms = win32_get_ms_elapsed(last_performance_counter, win32_get_performance_counter());
			}
#else
			//Note: ask later?
				f32 tms = target_ms / 1.1f;
				f32 ms_elapsed = (1000 * (tms - end_target_ms));
				i64 sleep_time = (i64)(-ms_elapsed * 10000);

				HANDLE waitable_timer =  CreateWaitableTimerEx(
						0,	0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

				LARGE_INTEGER timer = {0};
				timer.QuadPart = sleep_time;//(-target_ms * 100000 * 100);
				if(!SetWaitableTimer(waitable_timer, &timer, 0, 0, 0, 0))
				{
					Assert(0);
				}
				WaitForSingleObject(waitable_timer, INFINITE);
			

#endif

		}
//				GetLastError(
				int s = 0;
		u64 end_counter = win32_get_performance_counter();
		u64 end_cycle_count =  __rdtsc();
#else

//		f32 end_target_ms = win32_get_ms_elapsed(last_performance_counter, win32_get_performance_counter());

		//last_performance_counter is initialized outside the loop

		//get performance counter and frecuency
		LARGE_INTEGER performance_counter;
		QueryPerformanceCounter(&performance_counter);

		LARGE_INTEGER performance_frequency;
		QueryPerformanceFrequency(&performance_frequency);

		f32 end_target_ms = (f32)(performance_counter.QuadPart - last_performance_counter) / (f32)performance_frequency.QuadPart;
		if(end_target_ms < target_ms)
		{
			//1.========ADDED THIS==========
			f32 tms = target_ms / 1.0f;
			f32 ms_elapsed = (1000 * (tms - end_target_ms));
			//===========================
			i64 sleep_time = (i64)(-ms_elapsed * 10000);

			HANDLE waitable_timer =  CreateWaitableTimerEx(
					0,	0, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);

			LARGE_INTEGER timer = {0};
			timer.QuadPart = sleep_time;//(-target_ms * 100000 * 100);
			if(!SetWaitableTimer(waitable_timer, &timer, 0, 0, 0, 0))
			{
				Assert(0);
			}
			WaitForSingleObject(waitable_timer, INFINITE);


			//2.========ADDED THIS==========
			QueryPerformanceCounter(&performance_counter);
			end_target_ms = (f32)(performance_counter.QuadPart - last_performance_counter) / (f32)performance_frequency.QuadPart;
			while(end_target_ms < target_ms)
			{
				QueryPerformanceCounter(&performance_counter);
				end_target_ms = (f32)(performance_counter.QuadPart - last_performance_counter) / (f32)performance_frequency.QuadPart;
			}
			//===========================
		}
		u64 end_counter = win32_get_performance_counter();
		LARGE_INTEGER lend_counter;
		QueryPerformanceCounter(&lend_counter);

		//calculate miliseconds, fps...
		u64 end_cycle_count =  __rdtsc();
		last_performance_counter = lend_counter.QuadPart;

#endif

		renderer_display_buffer(renderer, &game_renderer);

		u64 CurrentCounter =  end_counter - last_performance_counter;
		elapsed_ms = (f32)(CurrentCounter * 1000.0f) / global_performance_frequency;
		delta_time = (f32)CurrentCounter / global_performance_frequency;
		last_performance_counter = end_counter;

		u8 buffer[256] = {0};
		format_text(buffer, sizeof(buffer), "Elapsed_ms %f\n", elapsed_ms);
		OutputDebugStringA(buffer);

#if 0
		static test_fps = 0;
		static test_fps_frame = 0;
		static test_fps_got = 0;
		if(test_fps_frame == 100)
		{
		u8 buffer[256] = {0};
//		format_text(buffer, sizeof(buffer), "Elapsed_ms %f\n", elapsed_ms);
		format_text(buffer, sizeof(buffer), "number of 60 fps %u\n", test_fps_got);
		
		OutputDebugStringA(buffer);
		}
		if(test_fps > 10)
		{
			test_fps_frame++;
			test_fps_got += (u32)elapsed_ms == (u32)(target_ms * 1000);
		}
		test_fps++;
#endif

		u32 cycles_elapsed = (u32)(end_cycle_count - current_cycle_count);
		f64 mega_cycles_per_frame = (f64)cycles_elapsed / (1000.0f * 1000.0f);
		current_cycle_count = end_cycle_count;

		game_memory.elapsed_ms = elapsed_ms;
		game_memory.mc_per_frame = (f32)mega_cycles_per_frame;
		game_memory.cycles_elapsed = cycles_elapsed;

		memory_area_check(&main_game_memory_area);


	}
    return 0;
}
