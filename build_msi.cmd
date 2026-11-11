cd /d "%~dp0src\PowerEconomizer\PowerEconomizerSetup"
wix build -d Platform=x64 -d ProjectDir=.\ -ext WixToolset.UI.wixext -ext WixToolset.Util.wixext -intermediatefolder ..\tmp -culture en-US -loc .\lang\Product_en-us.wxl -o ..\bin\x64\PowerEconomizerSetup_x64_en-US.msi .\Product.wxs
wix build -d Platform=x64 -d ProjectDir=.\ -ext WixToolset.UI.wixext -ext WixToolset.Util.wixext -intermediatefolder ..\tmp -culture zh-CN -loc .\lang\Product_zh-CN.wxl -o ..\bin\x64\PowerEconomizerSetup_x64_zh-CN.msi .\Product.wxs
pause
