@echo off
cls
echo '      ============================        ' 
echo building... 

cl purpose_ray.c -O2 -arch:AVX2 /DBx64 /Fe:Ray -EHa- -Gm- -FC -GR- -Oi -Zi /W3 /nologo /FC shell32.lib kernel32.lib user32.lib gdi32.lib

rem cl ../win32_purpose.c /DBx64 /Od %compileflags% /Fo.\obj\ /Fe:Game %linkerflags% -NODEFAULTLIB -subsystem:windows 
rem cl ../ppse_assets.c /DBx64 /Fe:AssetBuilder -EHa- -Gm- -FC -GR- -Oi -Zi /W3 /nologo /FC shell32.lib kernel32.lib user32.lib gdi32.lib 
