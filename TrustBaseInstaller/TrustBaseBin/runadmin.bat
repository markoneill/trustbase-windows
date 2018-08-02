@ECHO OFF
SETLOCAL
set base_dir=%~dp0
cd %base_dir%
set cwd=%cd%\release
set python=HKEY_CURRENT_USER\SOFTWARE\Python\PythonCore\2.7\
set vsdist=HKEY_LOCAL_MACHINE\SOFTWARE\Classes\Installer\Dependencies\Microsoft.VS.VC_RuntimeAdditionalVSU_amd64,v14\
set err=0

:preinstalls
REG QUERY %python% 1>nul 2>nul || set /a err=err+1
REG QUERY %vsdist% 1>nul 2>nul || set /a err=err+2

if %err% neq 0 (
    goto :runbundle
)
goto :starttrustbase

:runbundle
set pwd=%cd%
cd %cwd%
if %err% equ 1 (
    echo installing python
) 
if %err% equ 2 (
    echo installing vsredist
)
if %err% equ 3 (
    echo installing both python and vsredist
)
powershell -Command "Start-Process .\preinstall.exe -Verb RunAs -Wait" 1>nul 2>nul
cd %pwd%

:starttrustbase

set init=0
for %I in (.) do (
    if %init% EQU 0 (
        set short_name=%~sI
    	set init=1
    ) 

    if %init% NEQ 0 (
        set short_name=%~sI /a
    )
)

rem RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 .\release\TrustBaseWin\TrustBaseWin.inf
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/C cd %short_name% && RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 .\release\TrustBaseWin\TrustBaseWin.inf'"
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c net start TrustBaseWin'"
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/k cd %cwd% && PolicyEngine.exe'"
exit