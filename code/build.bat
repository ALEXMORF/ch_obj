@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl -nologo -FC -Z7 -O2 ..\code\main.cpp 

popd