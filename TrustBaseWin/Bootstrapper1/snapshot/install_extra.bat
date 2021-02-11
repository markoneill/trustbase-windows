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
goto :preinstalls

:faliure
PAUSE
EXIT /B 1

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
set /a err=err+1
PAUSE
goto :lastcheck

:getpythonpath
reg query %PY_REGKEY% || (echo Error! That python registry key doesn't exist! & exit /b 1)
set PY_NAME=
for /f "tokens=2,*" %%a in ('reg query "%PY_REGKEY%"') do (
    set PY_NAME=%%b
)

if not defined PY_NAME (echo Error getting python install path! & exit /b 1)
set PY_NAME=%PY_NAME: =+%


:lastcheck
REG QUERY %vsdist% || set /a err=err+2

:runbundle
set pwd=%cd%
cd %cwd%
if %err% equ 1 (
    echo python was installed
) 
if %err% equ 2 (
    echo vsredist was installed
)
if %err% equ 3 (
    echo installed both python and vsredist
)


:starttrustbase
cd %short_name% 
certutil -addstore -f "TrustedPeople" "release\PluginManager_1.0.4.0_x86_x64_arm.cer"
powershell Add-AppxPackage -Path "%short_name%\release\PluginManager_1.0.4.0_x86_x64_arm.appxbundle"
IF %ERRORLEVEL% GEQ 1 goto :faliure
powershell start shell:AppsFolder\TrustBasePluginManager_9yd7xsdvvsy3g!App
cd %pwd%\InstallFilesForPlugins
%PY_NAME%Scripts\pip.exe install incremental
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install PySocks-1.6.8.tar.gz
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install pyOpenSSL-19.0.0-py2.py3-none-any.whl
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install service_identity-18.1.0-py2.py3-none-any.whl
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install Twisted-18.9.0.tar.bz2
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install win_inet_pton-1.0.1.tar.gz
IF %ERRORLEVEL% GEQ 1 goto :faliure
%PY_NAME%Scripts\pip.exe install win10toast-0.9-py2.py3-none-any.whl
IF %ERRORLEVEL% GEQ 1 goto :faliure