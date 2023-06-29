/*This file should be used alongside the platform layer to implement window's functions
  for every platform function
*/

static inline platform_file_time
win32_filetime_convert(FILETIME w32_file_time)
{
	platform_file_time result = {0};
	result.value = ((uint64)w32_file_time.dwHighDateTime << 32) | (w32_file_time.dwLowDateTime);

	SYSTEMTIME sys_time = {0};
	FileTimeToSystemTime(
			&w32_file_time,
			&sys_time);

	result.year = (u16)sys_time.wYear;
	result.month = (u8)sys_time.wMonth;
	result.day = (u8)sys_time.wDay;
	result.hour_minute = (u16)((sys_time.wHour << 8) | (sys_time.wMinute));
	result.second_ms = (u16)((sys_time.wSecond << 10) | (sys_time.wMilliseconds));

	return(result);
}
//Non-platform functions
inline void 
win32_FillSearchInfo(
		platform_file_search_info *fileSearchInfo,
		WIN32_FIND_DATAA w32_file_found)
{

	fileSearchInfo->name = w32_file_found.cFileName;
    fileSearchInfo->size = ((uint64)w32_file_found.nFileSizeHigh << 32) | (w32_file_found.nFileSizeLow); 
    //fileSearchInfo->write_time = ((uint64)write_time.dwHighDateTime << 32) | (write_time.dwLowDateTime); 
   //get last write struct.
	//fill time
	FILETIME write_time = w32_file_found.ftLastWriteTime;
    fileSearchInfo->write_time = win32_filetime_convert(write_time);

	if(w32_file_found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		fileSearchInfo->is_directory = 1;
	}
	else
	{
		fileSearchInfo->is_directory = 0;
	}
}






PLATFORM_MOVE_FILE(win32_move_file)
{
	u32 success = MoveFileA(file_path, new_path);
	return(success);
}
//platform functions
PLATFORM_ALLOC(win32_virtual_alloc)
{
    void *memory = VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    return memory;
}
PLATFORM_FREE(win32_virtual_free)
{
    VirtualFree(memory, 0, MEM_RELEASE);
    return(0);
}

PLATFORM_OPEN_FILE_FROM_PATH(win32_open_file)
{
    platform_file_handle result = {0};

	DWORD dwDesiredAccess = GENERIC_READ;
	DWORD dwShareMode = 0;
	DWORD dwCreationDisposition = OPEN_EXISTING;

//	platform_file_op_create = 1,
//	platform_file_op_read = 2,
//	platform_file_op_write = 4
	if(fileOp & platform_file_op_create)
	{
		dwCreationDisposition = CREATE_ALWAYS;
	}
	if(fileOp & platform_file_op_open_or_create)
	{
		dwCreationDisposition = OPEN_ALWAYS;
	}
	if(fileOp & platform_file_op_write)
	{
		dwDesiredAccess |= GENERIC_WRITE;
	}
	if(fileOp & platform_file_op_read | platform_file_op_share)
	{
		dwDesiredAccess |= GENERIC_READ;
		dwShareMode		|= FILE_SHARE_READ;
	}
	if(fileOp & platform_file_op_share)
	{
		dwShareMode		|= FILE_SHARE_WRITE;
		dwShareMode		|= FILE_SHARE_READ;
		//dwShareMode		|= FILE_SHARE_DELETE;
		dwDesiredAccess |= GENERIC_READ;
	}

    HANDLE handle = CreateFile(path,
                               dwDesiredAccess, 
                               dwShareMode,
                               0, //Note(Agu): For other aplications to use
							   dwCreationDisposition,
                               0,
                               0);

	DWORD error = GetLastError();
	platform_open_file_result open_result = 0;
    if(handle != INVALID_HANDLE_VALUE)
    {
         result.handle = handle;
		 if(error == ERROR_FILE_EXISTS)
		 {
			 open_result = open_file_result_overwrote_existing;
		 }
    }
    else {
		if(error == ERROR_FILE_NOT_FOUND)
		{
			open_result = open_file_result_not_found;
		}
		else if(error == ERROR_ALREADY_EXISTS)
		{
			open_result = open_file_result_already_exists;
		}
		else
		{
			open_result = open_file_result_access_denied;
		}
		u32 f = 0;
    }
    return(result);
}

PLATFORM_READ_FROM_FILE(win32_read_from_file)
{
//Note(Agu): I might need to open the file with FILE_FLAG_OVERLAPPED to use OVERLAPPED struct
    uint32 bytesread = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset     = (uint32)(offset & 0xffffffff);
    overlapped.OffsetHigh = (uint32)((offset >> 32) & 0xffffffff);
    ReadFile(file_handle.handle, destination, (uint32)size, &bytesread, &overlapped);
    
    return(0);
}

PLATFORM_WRITE_TO_FILE(win32_write_to_file)
{
    uint32 bytesWritten = 0;
    OVERLAPPED overlapped = {0};
    overlapped.Offset     = (uint32)(offset & 0xffffffff);
    overlapped.OffsetHigh = (uint32)((offset >> 32) & 0xffffffff);
    WriteFile(file_handle.handle, contents, (uint32)size, &bytesWritten, &overlapped);
    
    return(bytesWritten);
}


PLATFORM_GET_FILE_INFO(win32_get_file_info)
{

   BY_HANDLE_FILE_INFORMATION w32_file_information = {0};
   //fill info.
   GetFileInformationByHandle(file_handle.handle, &w32_file_information);
   //get last write struct.
    platform_file_min_attributes result = {0};
    result.size = ((uint64)w32_file_information.nFileSizeHigh << 32) | (w32_file_information.nFileSizeLow); 

   FILETIME write_time = w32_file_information.ftLastWriteTime;
    result.date = win32_filetime_convert(write_time);
    return(result);

}

PLATFORM_CLOSE_FILE(win32_close_file)
{
    CloseHandle(file_handle.handle);
}


PLATFORM_FIND_FIRST_FILE(win32_find_first_file)
{
    platform_file_search file_handle = {0};
    
    WIN32_FIND_DATAA w32_file_found;
    //HANDLE result = FindFirstFileA(pattern, &w32_file_found);
    HANDLE result = FindFirstFileExA (pattern, FindExInfoBasic, &w32_file_found, FindExSearchNameMatch, 0, FIND_FIRST_EX_LARGE_FETCH);
    if(result != INVALID_HANDLE_VALUE)
    {
		file_handle.handle = result;
		win32_FillSearchInfo(fileSearchInfo ,w32_file_found);
    }
    return(file_handle);
}

PLATFORM_FIND_NEXT_FILE(win32_find_next_file)
{
    WIN32_FIND_DATAA w32_file_found;
    u32 result = FindNextFileA(fileSearch.handle, &w32_file_found);
	if(result)
	{
		win32_FillSearchInfo(fileSearchInfo ,w32_file_found);
   }
   return(result);
}

PLATFORM_FIND_CLOSE(win32_find_close)
{
	FindClose(fileSearch.handle);
}

inline platform_api
platform_initialize()
{
	platform_api platform = {0};

    platform.Alloc = win32_virtual_alloc;

    platform.Free             = win32_virtual_free;
    platform.f_read_from_file = win32_read_from_file;
    platform.f_write_to_file  = win32_write_to_file;
    platform.f_close_file     = win32_close_file;
    platform.f_open_file      = win32_open_file;
    platform.f_get_file_info  = win32_get_file_info;
	platform.f_find_first_file = win32_find_first_file;
	platform.f_find_next_file  = win32_find_next_file;
	platform.f_find_close      = win32_find_close;
	platform.f_move_file = win32_move_file;

	return(platform);
}
