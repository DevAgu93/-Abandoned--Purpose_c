let SessionLoad = 1
let s:so_save = &g:so | let s:siso_save = &g:siso | setg so=0 siso=0 | setl so=-1 siso=-1
let v:this_session=expand("<sfile>:p")
silent only
silent tabonly
cd c:/Dev/Proyectos/Purpose_c
if expand('%') == '' && !&modified && line('$') <= 1 && getline(1) == ''
  let s:wipebuf = bufnr('%')
endif
set shortmess=aoO
argglobal
%argdel
edit win32_purpose.c
let s:save_splitbelow = &splitbelow
let s:save_splitright = &splitright
set splitbelow splitright
wincmd _ | wincmd |
vsplit
1wincmd h
wincmd _ | wincmd |
split
1wincmd k
wincmd w
wincmd w
let &splitbelow = s:save_splitbelow
let &splitright = s:save_splitright
wincmd t
let s:save_winminheight = &winminheight
let s:save_winminwidth = &winminwidth
set winminheight=0
set winheight=1
set winminwidth=0
set winwidth=1
exe '1resize ' . ((&lines * 36 + 37) / 75)
exe 'vert 1resize ' . ((&columns * 169 + 135) / 270)
exe '2resize ' . ((&lines * 36 + 37) / 75)
exe 'vert 2resize ' . ((&columns * 169 + 135) / 270)
exe 'vert 3resize ' . ((&columns * 100 + 135) / 270)
argglobal
if bufexists("C:/NeoVim/share/nvim/runtime/doc/nvim_terminal_emulator.txt") | buffer C:/NeoVim/share/nvim/runtime/doc/nvim_terminal_emulator.txt | else | edit C:/NeoVim/share/nvim/runtime/doc/nvim_terminal_emulator.txt | endif
if &buftype ==# 'terminal'
  silent file C:/NeoVim/share/nvim/runtime/doc/nvim_terminal_emulator.txt
endif
balt win32_purpose.c
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal nofen
silent! normal! zE
let &fdl = &fdl
let s:l = 46 - ((25 * winheight(0) + 18) / 36)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 46
normal! 049|
wincmd w
argglobal
balt purpose_render_commands.c
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
silent! normal! zE
let &fdl = &fdl
let s:l = 454 - ((25 * winheight(0) + 18) / 36)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 454
normal! 0
wincmd w
argglobal
if bufexists("term://c:/Dev/Proyectos/Purpose_c//14796:C:/WINDOWS/system32/cmd.exe") | buffer term://c:/Dev/Proyectos/Purpose_c//14796:C:/WINDOWS/system32/cmd.exe | else | edit term://c:/Dev/Proyectos/Purpose_c//14796:C:/WINDOWS/system32/cmd.exe | endif
if &buftype ==# 'terminal'
  silent file term://c:/Dev/Proyectos/Purpose_c//14796:C:/WINDOWS/system32/cmd.exe
endif
balt purpose_render.h
setlocal fdm=manual
setlocal fde=0
setlocal fmr={{{,}}}
setlocal fdi=#
setlocal fdl=0
setlocal fml=1
setlocal fdn=20
setlocal fen
let s:l = 186 - ((45 * winheight(0) + 36) / 73)
if s:l < 1 | let s:l = 1 | endif
keepjumps exe s:l
normal! zt
keepjumps 186
normal! 0
wincmd w
3wincmd w
exe '1resize ' . ((&lines * 36 + 37) / 75)
exe 'vert 1resize ' . ((&columns * 169 + 135) / 270)
exe '2resize ' . ((&lines * 36 + 37) / 75)
exe 'vert 2resize ' . ((&columns * 169 + 135) / 270)
exe 'vert 3resize ' . ((&columns * 100 + 135) / 270)
tabnext 1
badd +454 purpose_D3D11.c
badd +22 purpose.h
badd +298 purpose.c
badd +32 purpose_render_commands.c
badd +439 win32_purpose.c
badd +73 ~/AppData/Local/nvim/init.vim
badd +5 purpose_ui.c
badd +115 purpose_render.h
badd +164 purpose_render_platform.c
badd +1 w
badd +0 term://c:/Dev/Proyectos/Purpose_c//14796:C:/WINDOWS/system32/cmd.exe
if exists('s:wipebuf') && len(win_findbuf(s:wipebuf)) == 0 && getbufvar(s:wipebuf, '&buftype') isnot# 'terminal'
  silent exe 'bwipe ' . s:wipebuf
endif
unlet! s:wipebuf
set winheight=1 winwidth=20 shortmess=filnxtToOF
let &winminheight = s:save_winminheight
let &winminwidth = s:save_winminwidth
let s:sx = expand("<sfile>:p:r")."x.vim"
if filereadable(s:sx)
  exe "source " . fnameescape(s:sx)
endif
let &g:so = s:so_save | let &g:siso = s:siso_save
set hlsearch
doautoall SessionLoadPost
unlet SessionLoad
" vim: set ft=vim :
