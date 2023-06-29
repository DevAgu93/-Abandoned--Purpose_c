static inline void
ui_win32_read_event(
		game_ui *ui,
		MSG *msgs)
{
	ui_input *input = &ui->input;

	s_input_text *input_text = &input->input_text;
	input_keystate *key_state = &input_text->key_state;

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
					uint32 IsDown = (msgs->lParam & (1UL << 31)) == 0;
					uint32 WasDown = (msgs->lParam & (1UL << 30)) != 0;
					//Sends WM_CHAR with a translated char
//					TranslateMessage(msgs);

					//
					//;Process game input
					//
					if(IsDown != WasDown)
					{
						input_button *button = 0;
						switch(msgs->wParam)
						{
							case VK_LEFT:
								{
									button = &input->left;
								}break;
							case VK_RIGHT:
								{
									button = &input->right;
								}break;
							case VK_UP:
								{
									button = &input->up;
								}break;
							case VK_DOWN:
								{
									button = &input->down;
								}break;
							case VK_Z:
								{
									button = &input->z;
								}break;
							case VK_X:
								{
									button = &input->x;
								}break;
							case VK_Y:
								{
									button = &input->y;
								}break;
							case VK_C:
								{
									button = &input->c;
								}break;
							case VK_W:
								{
									button = &input->w;
								}break;
							case VK_S:
								{
									button = &input->s;
								}break;
							case VK_D:
								{
									button = &input->d;
								}break;
							case VK_A:
								{
									button = &input->a;
								}break;
							case VK_Q:
								{
									button = &input->q;
								}break;
							case VK_E:
								{
									button = &input->e;
								}break;
							case VK_H:
								{
									button = &input->h;
								}break;
							case VK_V:
								{
									button = &input->v;
								}break;
							case VK_R:
								{
									button = &input->r;
								}break;
							case VK_ESCAPE:
								{
									button = &input->esc;
								}
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
							input->spaceBarDown = IsDown;
						}
						if(msgs->wParam == VK_MENU)
						{
							input->alt = IsDown;
						}
					}
					if(IsDown)
					{
						if(msgs->wParam >= VK_F1 && msgs->wParam <= VK_F12)
						{
							uint64 fI = msgs->wParam - VK_F1;
						}
						if(msgs->wParam >= 'A' && msgs->wParam <= 'Z')
						{
							// input_text->keyCode = (uint32)keycode; 
						}
						input_text->current_key_code = (uint32)keycode; 
					}

					//Just process the state.
					if(msgs->wParam == VK_SHIFT)
					{
						input->shift_l = IsDown;
						input_text->shift_l = IsDown;
					}
					if(msgs->wParam == VK_CONTROL)
					{
						input->ctrl_l = IsDown;
						input_text->ctrl_l = IsDown;
					}
					if(msgs->wParam == KEY_CODE_ENTER)
					{
						input->enter = IsDown;
					}
				}break;
		case WM_CHAR:
			{

					WPARAM keycode = msgs->wParam;
					uint32 IsDown  = (msgs->lParam & (1UL << 31)) == 0;
					uint32 WasDown = (msgs->lParam & (1UL << 30)) != 0;
					//this only works with characters and special chacters
					input_text->key    = (uint8)msgs->wParam;
					key_state->is_down  = IsDown;
					key_state->was_down = WasDown;

			}break;
		case WM_MOUSEWHEEL:
			{
				input->mouse_wheel = ((int16)((msgs->wParam >> 16) & 0xffff)) / 120;

#if 0
				uint8 buffer[128];
				format_text(buffer, sizeof(buffer), "Mouse wheel value is:%d with%u\n" , mouseWheelDir, msgs->wParam);
				OutputDebugString(buffer);
#endif
			}break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			{
				u32 message_time     = GetMessageTime();
				u32 double_click_time = GetDoubleClickTime();

				u32 doubleClicked      = (message_time < (input->lastDoubleClickTime + double_click_time));
				//Doesn't check if left yet
				if(doubleClicked)
				{
					input->doubleClickedLeft = 1;
				}

			}
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{

				//This process can also be done by saving the last click time and current on input

				//used to compare the time it took since last click
				u32 message_time     = GetMessageTime();
				u32 double_click_time = GetDoubleClickTime();

				u32 current_click_is_left = msgs->message == WM_LBUTTONDOWN;
				u32 doubleClicked = (message_time < (input->lastClickTime + double_click_time));
				u32 tripleClicked = (message_time < (input->lastDoubleClickTime + double_click_time));

				u32 leftDoubleClick = doubleClicked && (current_click_is_left && input->lastClickWasLeft);
				u32 leftTrippleClick = tripleClicked && (current_click_is_left && input->lastClickWasLeft);

				input->lastClickWasLeft  = current_click_is_left;
				input->doubleClickLeft   = leftDoubleClick;
				input->tripleClickLeft  = leftTrippleClick;
				input->lastClickTime     = message_time;
				if(leftDoubleClick)
				{
					input->lastDoubleClickTime = message_time;
				}

				if(leftTrippleClick)
				{
					input->lastDoubleClickTime = 0;
				}

			}break;
	}
}

static inline void
ui_win32_reset_input(
		game_ui *ui)
{
	int32 window_is_active = GetActiveWindow() != 0;
	ui_input *old_input = &ui->old_input;
	ui_input *input = &ui->input;

	ui->old_input = ui->input;

	//reset input time after switching old input
	input->doubleClickedLeft = 0;
	input->doubleClickLeft = 0;
	input->tripleClickLeft = 0;

	u32 key_count = ARRAYCOUNT(input->buttons);
	for(uint32 k = 0; k < key_count; k++)
	{
		input->buttons[k].transition_count = 0;
		input->buttons[k].was_down = 
			old_input->buttons[k].was_down * window_is_active;
	}
	input->mouse_wheel = 0; 
	//reset input text
	input->input_text.lastKey = input->input_text.key;
	input->input_text.current_key_code = 0; 
	input->input_text.key = 0;

}

static inline void
ui_win32_update_input(
		game_ui *ui,
		u32 back_buffer_width,
		u32 back_buffer_height)
{
	ui_input *input = &ui->input;
	ui_input *old_input = &ui->old_input;
	ui->back_buffer_width = back_buffer_width;
	ui->back_buffer_height = back_buffer_height;

	//handle mouse input
	u32 mouse_button_count = ARRAYCOUNT(ui->input.mouse_buttons);
	for(uint32 b = 0; b < mouse_button_count; b++)
	{
		input->mouse_buttons[b].transition_count = 0;
		input->mouse_buttons[b].was_down =
			old_input->mouse_buttons[b].was_down;
	}

	//High order bit of 16 bits
	//
	// mouse input
	//

	RECT clientsz = {0};
	GetClientRect(ui->window_handle, &clientsz);
	u32 client_w = 0;
	u32 client_h = 0;
	if(clientsz.right * clientsz.bottom)
	{
		client_w  = clientsz.right - clientsz.left;
		client_h = clientsz.bottom - clientsz.top;
	}
	//get mouse at screen coordinates
	POINT win32_mouse_point;
	GetCursorPos(&win32_mouse_point);
	//Convert mouse to client, ignoring title and bars.
	ScreenToClient(ui->window_handle, &win32_mouse_point);

	//get mouse at frame buffer coordinates
	ui->input.mouse_x_last = ui->input.mouse_x;
	ui->input.mouse_y_last = ui->input.mouse_y;

    f32 scale_dif_x = (f32)ui->back_buffer_width / client_w;
    f32 scale_dif_y = (f32)ui->back_buffer_height / client_h;

#if 1
	ui->input.mouse_x = win32_mouse_point.x * scale_dif_x;
	ui->input.mouse_y = win32_mouse_point.y * scale_dif_y;
#endif

	uint32 mouse_l_state = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
	uint32 mouse_r_state = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;
	uint32 mouse_m_state = (GetKeyState(VK_MBUTTON) & (1 << 15)) != 0;

	input_button *mouse_button_ptr = &input->mouse_left;    
	//Mouse left switched state
	if(mouse_l_state != mouse_button_ptr->was_down)
	{
		mouse_button_ptr->was_down = mouse_l_state != 0;
		mouse_button_ptr->transition_count++;
	}
	mouse_button_ptr = &input->mouse_right;
	if(mouse_r_state != mouse_button_ptr->was_down)
	{
		mouse_button_ptr->was_down = mouse_r_state != 0;
		mouse_button_ptr->transition_count++;
	}
	input->mouse_middle    = mouse_m_state != 0;
}





static inline void
ui_win32_begin(
		game_ui *ui,
		b32 window_is_active,
		u32 back_buffer_width,
		u32 back_buffer_height)
{
	ui->back_buffer_width = back_buffer_width;
	ui->back_buffer_height = back_buffer_height;
	ui_input *old_input = &ui->old_input;
	ui_input *input = &ui->input;

	ui->old_input = ui->input;

	//reset input time after switching old input
	input->doubleClickedLeft = 0;
	input->doubleClickLeft = 0;
	input->tripleClickLeft = 0;

	u32 key_count = ARRAYCOUNT(input->buttons);
	for(uint32 k = 0; k < key_count; k++)
	{
		input->buttons[k].transition_count = 0;
		input->buttons[k].was_down = 
			old_input->buttons[k].was_down * window_is_active;
	}
	input->mouse_wheel = 0; 

	//handle mouse input
	u32 mouse_button_count = ARRAYCOUNT(ui->input.mouse_buttons);
	for(uint32 b = 0; b < mouse_button_count; b++)
	{
		input->mouse_buttons[b].transition_count = 0;
		input->mouse_buttons[b].was_down =
			old_input->mouse_buttons[b].was_down;
	}

	//High order bit of 16 bits
	//
	// mouse input
	//

	RECT clientsz = {0};
	GetClientRect(ui->window_handle, &clientsz);
	u32 client_w = 0;
	u32 client_h = 0;
	if(clientsz.right * clientsz.bottom)
	{
		client_w  = clientsz.right - clientsz.left;
		client_h = clientsz.bottom - clientsz.top;
	}
	//get mouse at screen coordinates
	POINT win32_mouse_point;
	GetCursorPos(&win32_mouse_point);
	//Convert mouse to client, ignoring title and bars.
	ScreenToClient(ui->window_handle, &win32_mouse_point);

	//get mouse at frame buffer coordinates
	ui->input.mouse_x_last = ui->input.mouse_x;
	ui->input.mouse_y_last = ui->input.mouse_y;

    f32 scale_dif_x = (f32)ui->back_buffer_width / client_w;
    f32 scale_dif_y = (f32)ui->back_buffer_height / client_h;

#if 1
	ui->input.mouse_x = win32_mouse_point.x * scale_dif_x;
	ui->input.mouse_y = win32_mouse_point.y * scale_dif_y;
#endif

	uint32 mouse_l_state = (GetKeyState(VK_LBUTTON) & (1 << 15)) != 0;
	uint32 mouse_r_state = (GetKeyState(VK_RBUTTON) & (1 << 15)) != 0;
	uint32 mouse_m_state = (GetKeyState(VK_MBUTTON) & (1 << 15)) != 0;

	input_button *mouse_button_ptr = &input->mouse_left;    
	//Mouse left switched state
	if(mouse_l_state != mouse_button_ptr->was_down)
	{
		mouse_button_ptr->was_down = mouse_l_state != 0;
		mouse_button_ptr->transition_count++;
	}
	mouse_button_ptr = &input->mouse_right;
	if(mouse_r_state != mouse_button_ptr->was_down)
	{
		mouse_button_ptr->was_down = mouse_r_state != 0;
		mouse_button_ptr->transition_count++;
	}
	input->mouse_middle    = mouse_m_state != 0;

	//reset input text
	input->input_text.lastKey = input->input_text.key;
	input->input_text.current_key_code = 0; 
	input->input_text.key = 0;


}
