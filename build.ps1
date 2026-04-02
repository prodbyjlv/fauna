$msbuild = "C:\Program Files\Microsoft Visual Studio\18\Insiders\MSBuild\Current\Bin\MSBuild.exe"
$sln = "C:\Users\Joshua\Documents\FAUNA PROJECT\FAUNA\Builds\VisualStudio2026\FAUNA.sln"

& $msbuild $sln /p:Configuration=Release /p:Platform=x64 /t:Build
