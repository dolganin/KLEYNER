@echo off
set OUTPUT_EXECUTABLE=cleaner.exe
set BUILD_DIR=build
set INSTALL_DIR=bin
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%
cmake .. -G "MinGW Makefiles" && cmake --build . --config Release || exit /b 1
if not exist ..\%INSTALL_DIR% mkdir ..\%INSTALL_DIR%
copy /Y Release\%OUTPUT_EXECUTABLE% ..\%INSTALL_DIR%\ >nul
cd ..
rmdir /S /Q %BUILD_DIR%
@echo Build finished. Executable located at %INSTALL_DIR%\%OUTPUT_EXECUTABLE%

