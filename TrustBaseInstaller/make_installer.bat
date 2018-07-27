::candle will preprocess and compile trustbase.wxs
::light links the object from candle into a .msi
::-ext WixUIExtension needed for specifying installation directory
::-ext WixUtilExtension needed for starting runadmin.bat
::-d Define a WiX variable. Used to hook up EULA
::We use && so that if candle fails it will not execute light
@ECHO OFF
setlocal

:setupdependencies
call set_dependencies.bat
call _dep_routes.bat
del _dep_routes.bat /Q /F /S

if %failure% EQU %true% (
  echo Failed to configure dependencies, please check set_dependencies.bat to make sure the actual_ locations are correct (line 14)
  goto :EOF
)

:runpreinstall
cd PreInstall
call preinstall.bat || set failure=%true%

:removepreinstall
del "preinstall.wixobj" 2>nul
del "preinstall.wixpdb" 2>nul
cd ..
if %failure% EQU %true% (
    goto :preinstallerror
)

:runheat
call run_heat.bat

:runtrustbase
@ECHO ON
candle trustbase.wxs dir.wxs -ext WixUIExtension -ext WixUtilExtension -dMySource=%snapshot% || set failure=%true% && goto :removetrustbase
light -ext WixUIExtension -ext WixUtilExtension -dWixUILicenseRtf=./TrustBase.rtf trustbase.wixobj dir.wixobj -out TrustBase || set failure=%true% && goto :removetrustbase

:removetrustbase
@ECHO OFF
del "dir.wixobj" 2>nul
del "trustbase.wixobj" 2>nul
del "trustbase.wixpdb" 2>nul
if %failure% EQU %true%(
    del "TrustBase.msi" 2>nul
    goto :preinstallerror
)
goto :EOF

:preinstallerror
@ECHO OFF
cd %bin%
del "preinstall.exe" 2>nul
cd ..
goto :EOF