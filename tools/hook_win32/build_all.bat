@echo off
taskkill /im hook_win32.exe /f 2> nul
pushd %cd%
cd hook_backend
call build_dll.bat
if errorlevel 1 popd & exit /B
popd

pushd %cd%
mkdir build\MinGW_14_2_0_64_bit-Release 2> nul
cd build\MinGW_14_2_0_64_bit-Release
call ..\..\run_cmake.bat -DCMAKE_BUILD_TYPE=Release
make
if errorlevel 1 popd & exit /B
popd

mkdir dist\hook_win32 2> nul
copy build\MinGW_14_2_0_64_bit-Release\hook_win32.exe dist\hook_win32\hook_win32.exe
windeployqt dist\hook_win32\hook_win32.exe
xcopy /E /I /Y hook_backend\bin\* dist\hook_win32\hook_backend