@echo off
set curpath=%~dp0
cd /D %curpath%

git add . && git commit -m "initial commit"
