@echo off
set curpath=%~dp0
cd /D %curpath%

git tag 1.0.0
git tag

pause