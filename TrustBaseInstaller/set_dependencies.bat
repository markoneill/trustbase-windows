:: Save the dependencies for the bat files
setlocal

:: Status Codes
set true=0
set false=1

set failure=%false%
set move_installme=%true%

:: File Dependencies
set wkdir=%cd%
set snapshot=%wkdir%\TrustBase_SNAPSHOT_MARCH_19_2018
set bin=%snapshot%\TrustBase-PolicyEngine\Release-bin
set installme=%snapshot%\"Install me"
