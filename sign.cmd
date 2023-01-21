copy "%~dp0src\PowerEconomizer\bin\x64\MTRelease\PowerEconomizer.exe" "%~dp0conf\"
"%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" sign /sha1 ff8a8160116e35775506d0dd86360935877aaf3b /fd SHA512 /t http://timestamp.digicert.com/ "%~dp0conf\PowerEconomizer.exe"
pause
