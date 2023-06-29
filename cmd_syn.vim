syn match CMD_WARNING "warning \w\d\+:"
syn match CMD_ERROR "error \w\d\+:"
hi _WARNING_COLOR guifg=#ffff00 gui=underline
hi _ERROR_COLOR guifg=#ff0000 gui=underline
highlight link CMD_WARNING _WARNING_COLOR 
highlight link CMD_ERROR _ERROR_COLOR 
