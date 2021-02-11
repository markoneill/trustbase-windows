@echo off
set base_dir=%~dp0
cd %base_dir%
set cwd=%cd%\release
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c cd %base_dir% & cleanup.bat & Pause'"