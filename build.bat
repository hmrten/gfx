@echo off

set dwarn=-wd4706 -wd4127 -wd4996 -wd4100 -wd4201
set cflags=-Od -Oi -fp:fast -Z7 -MD -GS- -FC -W4 -nologo -DNOMINMAX %dwarn%
set lflags=-nodefaultlib -dynamicbase:no -opt:ref -map -nologo
set libs=kernel32.lib user32.lib gdi32.lib msvcrt.lib

if not exist out mkdir out
pushd out
cl -DGFX_WIN32 %cflags% ..\test.c -link -subsystem:windows %lflags% %libs%
popd
