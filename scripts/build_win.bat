@echo off
set BUILD_DIR=build
set INSTALL_DIR=bin
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=20 ..
cmake --build . --config Release
if not exist ..\%INSTALL_DIR% mkdir ..\%INSTALL_DIR%
copy Release\cleaner.exe ..\%INSTALL_DIR%\cleaner.exe >nul
cd ..
rd /s /q %BUILD_DIR%
