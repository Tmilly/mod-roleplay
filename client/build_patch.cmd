@echo off
setlocal
set "MODULE_DIR=%~dp0"
set "MPQ_EDITOR=C:\Gaming\WarcraftLegacies24p\buildTools\MPQEditor\MPQEditor.exe"
set "OUTPUT=%MODULE_DIR%build\patch-R.MPQ"
set "SCRIPT=%MODULE_DIR%build\mpq-build.txt"

if not exist "%MPQ_EDITOR%" (
  echo MPQEditor was not found at %MPQ_EDITOR%
  exit /b 1
)
if not exist "%MODULE_DIR%build\DBFilesClient\Spell.dbc" (
  echo Build the synchronized DBC files first.
  exit /b 1
)

>"%SCRIPT%" echo new "%OUTPUT%" 64
>>"%SCRIPT%" echo add "%OUTPUT%" "%MODULE_DIR%build\DBFilesClient\*" "DBFilesClient" /auto
>>"%SCRIPT%" echo close
"%MPQ_EDITOR%" /console "%SCRIPT%"
exit /b %errorlevel%
