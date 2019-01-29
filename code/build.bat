@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -nologo -Z7 -O2 ..\code\main.cpp 

popd