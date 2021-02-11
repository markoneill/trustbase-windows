@ECHO OFF
SETLOCAL
set base_dir=%~dp0
cd %base_dir%
set cwd=%cd%\release
set err=0

:: Get the shortname of the base_dir
set init=0
for %%I in (.) do (
    echo %shortname%
    if %init% EQU 0 (
        set short_name=%%~sI
    	set init=1
    ) 

    if %init% NEQ 0 (
        set short_name=%%~sI /a
    )
)
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c cd %cwd% & ProcessStarter.exe PolicyEngine.exe -bg'"