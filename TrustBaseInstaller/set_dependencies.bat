ECHO OFF
setlocal enabledelayedexpansion

:: Save the dependencies for the bat files
:: Status Codes
set true=0
set false=1

set failure=%false%
set output=_dep_routes.bat

:: Actual Locations for Dependencies
:: These are the dependencies for the make_installer file to work
set actual_snapshot=TrustBaseBin\May_11th_Snapshot
set actual_bin=%actual_snapshot%\Release
set actual_installme=TrustBaseBin\"Install me"
set actual_runadmin=TrustBaseBin\runadmin.bat
set actual_TrustBaseWin_CAT=%actual_bin%\TrustBaseWin\trustbasewin.cat
set actual_TrustBaseWin_CER=%actual_bin%\TrustBaseWin\TrustBaseWin.cer
set actual_TrustBaseWin_INF=%actual_bin%\TrustBaseWin\TrustBaseWin.inf
set actual_TrustBaseWin_SYS=%actual_bin%\TrustBaseWin\TrustBaseWin.sys

:: Assumed Locations for Dependencies
set assumed_snapshot=snapshot
set assumed_bin=%assumed_snapshot%\release
set assumed_installme=PreInstall\Dependencies
set assumed_runadmin=%assumed_snapshot%\runadmin.bat
set assumed_TrustBaseWin_CAT=%assumed_bin%\TrustBaseWin\TrustBaseWin.cat
set assumed_TrustBaseWin_CER=%assumed_bin%\TrustBaseWin\TrustBaseWin.cer
set assumed_TrustBaseWin_INF=%assumed_bin%\TrustBaseWin\TrustBaseWin.inf
set assumed_TrustBaseWin_SYS=%assumed_bin%\TrustBaseWin\TrustBaseWin.sys

:DefineDepMap
:: Sets up dependencies map
set dep[0,name]=snapshot
set dep[0,assumed]=%assumed_snapshot%
set dep[0,actual]=%actual_snapshot%
set dep[0,isfile]=%false%

set dep[1,name]=bin
set dep[1,assumed]=%assumed_bin%
set dep[1,actual]=%actual_bin%
set dep[1,isfile]=%false%

set dep[2,name]=installme
set dep[2,assumed]=%assumed_installme%
set dep[2,actual]=%actual_installme%
set dep[2,isfile]=%false%

set dep[3,name]=runadmin
set dep[3,assumed]=%assumed_runadmin%
set dep[3,actual]=%actual_runadmin%
set dep[3,isfile]=%true%

set dep[4,name]=trustbasewin_CAT
set dep[4,assumed]=%assumed_TrustBaseWin_CAT%
set dep[4,actual]=%actual_TrustBaseWin_CAT%
set dep[4,isfile]=%true%

set dep[5,name]=trustbasewin_CER
set dep[5,assumed]=%assumed_TrustBaseWin_CER%
set dep[5,actual]=%actual_TrustBaseWin_CER%
set dep[5,isfile]=%true%

set dep[6,name]=trustbasewin_INF
set dep[6,assumed]=%assumed_TrustBaseWin_INF%
set dep[6,actual]=%actual_TrustBaseWin_INF%
set dep[6,isfile]=%true%

set dep[7,name]=trustbasewin_SYS
set dep[7,assumed]=%assumed_TrustBaseWin_SYS%
set dep[7,actual]=%actual_TrustBaseWin_SYS%
set dep[7,isfile]=%true%

set "x=0"

:GetDepMapLen
if "!dep[%x%,assumed]!" NEQ "" (
	set /a "x+=1"
	goto :GetDepMapLen
)

set depMapLen=%x%

:VerifyDependencies
set /a numIters=%depMapLen%-1

:: Check to make sure that the provided files exist
for /l %%i in (0,1,%numIters%) do (
  if %failure% EQU %false% (
    call :ValidateActualExists !dep[%%i,assumed]! !dep[%%i,actual]!
  )
)

if %failure% EQU %true% (
    GOTO :VerificationError
)


:: Create Files in expected directories for the installer to use
for /l %%i in (0,1,%numIters%) do (
  if %failure% EQU %false% (
    call :EnsureExistance !dep[%%i,assumed]! !dep[%%i,actual]! !dep[%%i,isfile]!
  )
)

if %failure% EQU %true% (
    GOTO :VerificationError
)

:: Create BatFile to load dependencies
echo @ECHO OFF > %output%
for /l %%i in (0,1,%numIters%) do (
  call :AddRoute !dep[%%i,name]! !dep[%%i,assumed]!
)

call :AddRoute failure %false%
call :AddRoute true %true%
call :AddRoute false %false%
call :AddRoute dependencies_output_file %output%

GOTO :EOF

:ValidateActualExists
set assumed=%~1
set actual=%~2

if exist %actual% (
	echo EXISTS %actual%
) else (
	echo FILE NOT FOUND: %actual%
	set failure=%true%
) 

exit /B 0

:EnsureExistance
set assumed=%~1
set actual=%~2
set isfile=%~3

if %assumed% EQU %actual% (
    echo These are the same
) else (

	rem If there is a case insensitivity error, correct it
    if exist %assumed% (
		rem xcopy %assumed% %assumed%_1 /Y /E /I
		>nul del %assumed% /Q /F /S
		>nul rmdir %assumed% /S /Q
	)
	
	rem Ensure the file is where it needs to be
	if %isfile% EQU %true% (
		echo F | xcopy %actual% %assumed% /Y /E /I >nul
	) else (
		>nul xcopy %actual% %assumed% /Y /E /I
	)
	
)

exit /B 0

:AddRoute
set name=%~1
set value=%~2

echo set %name%=%value% >> %output%

exit /B 0

:VerificationError
echo @ECHO OFF > %output%
echo set true=%true% >> %output%
echo set failure=%true% >> %output%
GOTO :EOF