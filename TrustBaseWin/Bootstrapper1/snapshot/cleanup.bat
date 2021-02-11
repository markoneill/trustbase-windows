@echo off
SETLOCAL
set base_dir=%~dp0
cd %base_dir%
set cwd=%cd%\release
set python=HKCU\SOFTWARE\Python\PythonCore\2.7\
set python2=HKLM\SOFTWARE\Python\PythonCore\2.7\
set python3=HKLM\SOFTWARE\Wow6432Node\Python\PythonCore\2.7\
set vsdist=HKLM\SOFTWARE\Classes\Installer\Dependencies\Microsoft.VS.VC_RuntimeAdditionalVSU_amd64,v14\
set PY_REGKEY=HKCU\SOFTWARE\Python\PythonCore\2.7\InstallPath
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

:preinstalls
REG QUERY %python% 
if %ERRORLEVEL% EQU 0 (
	set PY_REGKEY=HKCU\SOFTWARE\Python\PythonCore\2.7\InstallPath
	goto :getpythonpath
)
REG QUERY %python2% 
if %ERRORLEVEL% EQU 0 (
	set PY_REGKEY=HKLM\SOFTWARE\Python\PythonCore\2.7\InstallPath
	goto :getpythonpath
)
REG QUERY %python3% 
if %ERRORLEVEL% EQU 0 (
	set PY_REGKEY=HKLM\SOFTWARE\Wow6432Node\Python\PythonCore\2.7\InstallPath
	goto :getpythonpath
)

:getpythonpath
reg query %PY_REGKEY% 1>nul 2>nul || (echo Error! That python registry key doesn't exist! & exit /b 1)
set PY_NAME=
for /f "tokens=2,*" %%a in ('reg query "%PY_REGKEY%"') do (
    set PY_NAME=%%b
)

if not defined PY_NAME (echo Error getting python install path! & exit /b 1)
set PY_NAME=%PY_NAME: =+%

powershell -Command "Start-Process cmd -Verb RunAs -ArgumentList '/c taskkill /f /im PolicyEngine.exe & net stop TrustBaseWin & %PY_NAME%Scripts\pip.exe uninstall win10toast -y & %PY_NAME%Scripts\pip.exe uninstall win_inet_pton -y & %PY_NAME%Scripts\pip.exe uninstall Twisted -y & %PY_NAME%Scripts\pip.exe uninstall service_identity -y & %PY_NAME%Scripts\pip.exe uninstall pyOpenSSL -y & %PY_NAME%Scripts\pip.exe uninstall PySocks -y'"
powershell Remove-AppxPackage -Package "TrustBasePluginManager_1.0.4.0_neutral_~_9yd7xsdvvsy3g"







