::-g1 Generate component guids without curly braces.
::-gg Generate guids now. All components are given a guid when heat is run.
::-sfrag Suppress generation of fragments for directories and components.
::-sreg Suppress registry harvesting.
::-suid stores the filename as the compnent id, may cause issues if files are named the same, even if stored in different folders
::-cg Component group name (cannot contain spaces e.g -cg MyComponentGroup).
::-template Use template, one of: fragment, module, product
::-out output file name
@ECHO ON
heat dir %snapshot% -dr "INSTALLFOLDER" -var var.MySource -g1 -gg -sfrag -sreg -suid -cg "TrustBaseComponent" -template fragment -out dir.wxs
@ECHO OFF
pause