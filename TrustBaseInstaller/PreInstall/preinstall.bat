::candle preprocesses files and returns a wixobj
::light links wixobj into .msi
::-ext WixBalExtension is used for running bundles
@ECHO ON
candle preinstall.wxs -ext WixBalExtension -ext WixUtilExtension || goto :error
light preinstall.wixobj -ext WixBalExtension -ext WixUtilExtension || goto :error
move preinstall.exe %bin% || goto :error
@ECHO OFF
goto :EOF

:error
@ECHO OFF
exit /b 1