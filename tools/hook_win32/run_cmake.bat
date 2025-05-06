@echo off
set QT6_DIR=F:\Env\Qt\6.9.0\mingw_64\lib\cmake\Qt6
cmake -G "MinGW Makefiles" -S %~dp0 -B . %*