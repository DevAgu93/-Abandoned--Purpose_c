@echo off
cls
echo building... 

set dx_libs= d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib
set common=..\..\common

set compileflags= -EHa- -Gm- -FC -GR- -Oi -Zi /W3 /nologo /FC /GS- /Gs9999999
set linkerflags= /EHsc /link /opt:icf /opt:ref -incremental:no "/STACK:0x100000,0x100000"
set libpath= 
REM set dateandtime=%date:~5,2%%date:~8,2%%date:~11,4%_%time:~0,2%%time:~3,2%%time:~6,2%
REM careful with the numbers
REM or just use %random%
set dateandtime=%date:~5,2%%date:~8,2%%date:~11,4%%time:~3,2%%time:~6,2%
rem game
set v[0]=1
rem software renderer
set v[1]=1
rem direct3d11 renderer
set v[2]=0

IF NOT EXIST ..\bin\ (mkdir ..\bin)

echo %v[0]%
echo (%v[0]%EQU1)
:_0
IF %v[0]%==1 (goto game_build)
:_1
IF %v[1]%==1 (goto software_renderer_build)
:_2
IF %v[2]%==1 (goto d3d11)
:_3
rem goto renderers_build
rem game build
goto end


:game_build
REM -Oi
  IF NOT EXIST ..\..\sw_bin\ (mkdir ..\..\sw_bin)
  pushd ..\..\sw_bin
  IF NOT EXIST obj\ (mkdir obj)
  IF NOT EXIST pdb\ (mkdir pdb)
  rem x64 build Bx64
REM comment to skip the complete editor build
rem goto 1
rem "delete all pdb files and don't output the command
  del .\pdb\sw*.pdb > NUL 2> NUL
rem goto editor_build
  echo building sw renderer...
  cl ../src/sw_render/sw_win32.c /I%common% /DBx64 /Od %compileflags% /Fo.\obj\ /Fe:sw %linkerflags% /fixed -PDB:.\pdb\ -NODEFAULTLIB -subsystem:windows shell32.lib kernel32.lib user32.lib gdi32.lib Winmm.lib
:game_build1
  echo: 
  echo building game dll...
  cl ../src/sw_render/sw_main.c /DBx64 /I%common% /Od %compileflags% /Fo.\obj\ /MT %linkerflags%  -PDB:.\pdb\sw_renderer%dateandtime%.pdb -NODEFAULTLIB -subsystem:windows /OUT:sw.dll /EXPORT:program_progress /Entry:dll_main 
  echo:
rem  ^ucrt.lib
rem  renderer dll
  popd
goto _1


  :software_renderer_build
  rem renderer
  pushd ..\..\sw_bin\
  cl ../src/sw_render/sw_renderer_main.c /DBx64 /I%common% %compileflags%  %linkerflags% -NODEFAULTLIB /DLL -PDB:.\pdb\ /OUT:sw_render.dll /EXPORT:win32_load_renderer %dx_libs%
  popd

  goto _2

  :d3d11
  pushd ..\..\sw_bin\
  cl ../src/win32_d3d11.c /DBx64 /I%common% %compileflags%  %linkerflags% -NODEFAULTLIB /DLL -PDB:.\pdb\ /OUT:direct3d11.dll /EXPORT:win32_load_renderer %dx_libs%
  popd

  goto _3


goto end 

:end

rem NOTE: pdb folders and name can be changed with -PDB:(path and name) linker flag
rem /fixed only works for .exe files
