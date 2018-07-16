::candle will preprocess and compile trustbase.wxs
::light links the object from candle into a .msi
::-ext WixUIExtension needed for specifying installation directory
::-ext WixUtilExtension needed for starting runadmin.bat
::-d Define a WiX variable. Used to hook up EULA
::We use && so that if candle fails it will not execute light
@ECHO OFF
setlocal
set wkdir=%cd%
set snapshot=TrustBase_SNAPSHOT_MARCH_19_2018
set bin=%snapshot%\TrustBase-PolicyEngine\Release-bin
set failure=0

:prepareInstall
:: We use this to not corrupt the data structure of preinstall.wxs
xcopy %snapshot%\"Install me" "Install me" /Y /E /I

:runpreinstall
cd PreInstall
call preinstall.bat || set failure=1

:removepreinstall
del "preinstall.wixobj" 2>nul
del "preinstall.wixpdb" 2>nul
cd ..
if %failure% neq 0 (
    goto :preinstallerror
)

:runheat
call run_heat.bat

:runtrustbase
@ECHO ON
candle trustbase.wxs dir.wxs -ext WixUIExtension -ext WixUtilExtension -dMySource=%snapshot% || set failure=1 && goto :removetrustbase
light -ext WixUIExtension -ext WixUtilExtension -dWixUILicenseRtf=./TrustBase.rtf trustbase.wixobj dir.wixobj -out TrustBase || set failure=1 && goto :removetrustbase

:removetrustbase
@ECHO OFF
del "dir.wixobj" 2>nul
del "trustbase.wixobj" 2>nul
del "trustbase.wixpdb" 2>nul
if %failure% neq 0(
    del "TrustBase.msi" 2>nul
    goto :preinstallerror
)
pause
goto :EOF

:preinstallerror
@ECHO OFF
cd %bin%
del "preinstall.exe" 2>nul
cd ..
pause
goto :EOF