@ECHO OFF
SETLOCAL
set cwd=%cd%\TrustBase-PolicyEngine\Release-bin
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
RUNDLL32.EXE SETUPAPI.DLL,InstallHinfSection DefaultInstall 132 .\TrustBase-TrafficInterceptor\release\TrustBaseWin.inf
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c net start TrustBaseWin'"
powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/k cd %cwd% && PolicyEngine.exe'"
pause
exit