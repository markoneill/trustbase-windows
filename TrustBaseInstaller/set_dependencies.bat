:: Save the dependencies for the bat files
:: Status Codes
set true=0
set false=1

set failure=%false%
set move_installme=%true%

:: File Dependencies
set wkdir=%cd%
set snapshot=%wkdir%\May_11th_Snapshot
set bin=%snapshot%\Release
set installme=%snapshot%\"Install me"
