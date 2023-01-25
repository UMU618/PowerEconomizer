copy "%~dp0src\PowerEconomizer\bin\x64\Release\en-US\PowerEconomizerSetup.msi" "%~dp0conf\PowerEconomizerSetup_en-US.msi"
"%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" sign /sha1 ff8a8160116e35775506d0dd86360935877aaf3b /fd SHA512 /t http://timestamp.digicert.com/ "%~dp0conf\PowerEconomizerSetup_en-US.msi"

copy "%~dp0src\PowerEconomizer\bin\x64\Release\zh-CN\PowerEconomizerSetup.msi" "%~dp0conf\PowerEconomizerSetup_zh-CN.msi"
"%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.22621.0\x64\signtool.exe" sign /sha1 ff8a8160116e35775506d0dd86360935877aaf3b /fd SHA512 /t http://timestamp.digicert.com/ "%~dp0conf\PowerEconomizerSetup_zh-CN.msi"
pause
