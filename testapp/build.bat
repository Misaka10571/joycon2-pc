@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\VsDevCmd.bat"
cd /d d:\projects\python\joycon2-pc\testapp
cmake --build build --config Release 2>&1 > build_log.txt
type build_log.txt
