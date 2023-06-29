@echo off
cls
echo building... 

set dx_libs= d3d11.lib dxgi.lib d3dcompiler.lib dxguid.lib
set opengl_libs= opengl32.lib
set common=..\..\common

set compileflags= -EHa- -Gm- -FC -GR- -Oi -Zi /W3 /nologo /FC /GS- /Gs9999999
set linkerflags= /EHsc /link /opt:icf /opt:ref -incremental:no "/STACK:0x100000,0x100000"
set libpath= 
REM set dateandtime=%date:~5,2%%date:~8,2%%date:~11,4%_%time:~0,2%%time:~3,2%%time:~6,2%
REM careful with the numbers
set dateandtime=%date:~5,2%%date:~8,2%%date:~11,4%%time:~3,2%%time:~6,2%
rem game
set v[0]=0
rem editor
set v[1]=1
rem direct3d11 renderer
set v[2]=1
rem asset builder
set v[3]=1
rem opengl renderer (doesn't exist)
set v[4]=0
rem software renderer (never actually made)
set v[5]=0
rem devexp
set v[10]=1

IF NOT EXIST ..\bin\ (mkdir ..\bin)

echo %v[0]%
echo (%v[0]%EQU1)
:_0
IF %v[0]%==1 (goto game_build)
:_1
IF %v[1]%==1 (goto editor_build)
:_2
IF %v[2]%==1 (goto renderers_build)
:_3
IF %v[5]%==1 (goto software_renderer_build)
:_5
IF %v[3]%==1 (goto asset_builder_build)
rem goto renderers_build
rem game build
goto end


:game_build
REM -Oi
  pushd ..\bin
  IF NOT EXIST obj\ (mkdir obj)
  IF NOT EXIST pdb\ (mkdir pdb)
  rem x64 build Bx64
REM comment to skip the complete editor build
rem goto 1
  del .\pdb\purpose*.pdb > NUL 2> NUL
rem goto editor_build
  echo building game on windows...
  cl ../src/win32_purpose.c /I%common% /DBx64 /Od %compileflags% /Fo.\obj\ /Fe:Game %linkerflags% /fixed -PDB:.\pdb\ -NODEFAULTLIB -subsystem:windows shell32.lib kernel32.lib user32.lib gdi32.lib  %opengl_libs% Winmm.lib
:game_build1
  echo: 
  echo building game dll...
  cl ../src/purpose.c /DBx64 /I%common% /Od %compileflags% /Fo.\obj\ /MT %linkerflags%  -PDB:.\pdb\purpose%dateandtime%.pdb -NODEFAULTLIB -subsystem:windows /OUT:purpose.dll /EXPORT:GameProgress /Entry:dll_main 
  echo:
rem  ^ucrt.lib
rem  renderer dll
  popd
goto _1


:editor_build
rem editor build
  pushd ..\bin
  del .\pdb\purpose_e*.pdb > NUL 2> NUL
  echo:
  echo building editor on windows... 
  cl ../src/win32_purpose_editor.c /I%common% /DBx64 /Od %compileflags% /Fe:purpose_e %linkerflags% /fixed -PDB:.\pdb\ -NODEFAULTLIB -subsystem:windows shell32.lib kernel32.lib user32.lib gdi32.lib
  echo:
  echo building editor dll...

  cl ../src/purpose_editor.c /DBx64 /I%common% /Od %compileflags% /Fo.\obj\ /MT %linkerflags%  -PDB:.\pdb\purpose_e%dateandtime%.pdb -NODEFAULTLIB -subsystem:windows /OUT:purpose_editor.dll /EXPORT:editor_progress /Entry:dll_main
  popd
  goto _2

  :renderers_build
  rem renderer
  pushd ..\bin
  cl ../src/win32_d3d11.c /DBx64 /I%common% %compileflags%  %linkerflags% -NODEFAULTLIB /DLL -PDB:.\pdb\ /OUT:direct3d11.dll /EXPORT:win32_load_renderer %dx_libs%
  popd

  goto _3

  :software_renderer_build
  rem renderer
  pushd ..\bin
  cl ../src/purpose_render_software.c /DBx64 /I%common% %compileflags%  %linkerflags% -NODEFAULTLIB /DLL -PDB:.\pdb\ /OUT:software_render.dll /EXPORT:win32_load_renderer %dx_libs%
  popd

  goto _5

:asset_builder_build
rem skip ppse assets
rem
  IF NOT EXIST ..\bin\asset_builder\ (mkdir ..\bin\asset_builder\)
pushd ..\bin\asset_builder
  IF NOT EXIST obj\ (mkdir obj)
  IF NOT EXIST pdb\ (mkdir pdb)
rem TODO: Remove crt from this one as well...
cl ../../src/purpose_assets_files.c /I../%common% /DBx64 /Fe:ab %compileflags% %linkerflags% /fixed -PDB:.\pdb\ shell32.lib kernel32.lib user32.lib gdi32.lib libvcruntime.lib
popd

goto end 

:end

rem NOTE: pdb folders and name can be changed with -PDB:(path and name) linker flag
rem /fixed only works for .exe files
