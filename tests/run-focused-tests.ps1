param(
    [string]$Core = (Resolve-Path "$PSScriptRoot/../../..").Path,
    [string]$VsDevCmd = 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/VsDevCmd.bat',
    [switch]$CompileModule
)
$ErrorActionPreference = 'Stop'
$moduleRoot = Split-Path $PSScriptRoot
$utf8 = New-Object System.Text.UTF8Encoding($false)
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('roleplay-tests-' + [guid]::NewGuid())
[IO.Directory]::CreateDirectory($scratch) | Out-Null
[IO.File]::WriteAllText("$scratch/environment.cmd", "@call `"$VsDevCmd`" -arch=x64 >nul`r`n@set`r`n")
cmd /c "$scratch/environment.cmd" | ForEach-Object {
    # Some launchers supply both PATH and Path; retain the developer prompt's uppercase PATH.
    if ($_ -cmatch '^Path=') { return }
    if ($_ -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}

# Extract production functions verbatim, without linking worldserver or opening real cards/DBs.
function Function-Source([string]$source, [string]$signature) {
    $start = $source.IndexOf($signature)
    if ($start -lt 0) { throw "Missing function: $signature" }
    $open = $source.IndexOf('{', $start)
    $depth = 1
    $end = $open + 1
    while ($depth -and $end -lt $source.Length) {
        if ($source[$end] -eq '{') { $depth++ }
        if ($source[$end] -eq '}') { $depth-- }
        $end++
    }
    if ($depth) { throw "Unbalanced function: $signature" }
    return $source.Substring($start, $end - $start)
}
$bot = [IO.File]::ReadAllText("$moduleRoot/src/rp_bot.cpp")
$phase = [IO.File]::ReadAllText("$moduleRoot/src/rp_phase1.cpp")
$registration = [regex]::Match($phase, '\{\s*"sheet",\s*(\w+),\s*SEC_PLAYER,\s*Console::No\s*\}')
if (!$registration.Success) { throw 'Missing SEC_PLAYER sheet registration' }
$adapter = $registration.Groups[1].Value
$code = [IO.File]::ReadAllText("$PSScriptRoot/sheet_test_support.h") + "`n"
foreach ($signature in @('std::string_view Trim(', 'std::string_view TakeToken(', 'std::string Lower(',
    'bool IsSheetOperation(', 'bool ReadCard(', 'bool WriteCard(', 'bool BackupCard(',
    'bool HandleBotSheet(')) {
    $code += (Function-Source $bot $signature) + "`n"
}
$code += (Function-Source $phase "bool $adapter(") + "`n"
$code += "Acore::Impl::ChatCommands::CommandInvoker SheetCommand($adapter);`n"
$code += [IO.File]::ReadAllText("$PSScriptRoot/sheet_test_cases.h")
[IO.File]::WriteAllText("$scratch/sheet.cpp", $code, $utf8)

# Use the installed core's real typed-command headers, not a imitation parser.
$includeDirs = Get-ChildItem "$Core/src/common", "$Core/src/server" -Directory -Recurse |
    Select-Object -ExpandProperty FullName
$includeDirs += @("$Core/src/common", "$Core/src/server", "$Core/deps/fmt/include", 'C:/local/boost_1_81_0')
$flags = @('/nologo', '/std:c++20', '/EHsc', '/utf-8', '/DFMT_HEADER_ONLY', '/DBOOST_ALL_NO_LIB')
foreach ($dir in $includeDirs) { $flags += "/I$dir" }
& cl @flags "$scratch/sheet.cpp" "/Fo$scratch/sheet.obj" "/Fe$scratch/sheet.exe"
if ($LASTEXITCODE) { throw 'Sheet regression compilation failed' }
& "$scratch/sheet.exe" $scratch
if ($LASTEXITCODE) { throw 'Sheet regression failed' }
& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/death_knight_policy_test.cpp" `
    "/Fo$scratch/policy.obj" "/Fe$scratch/policy.exe"
if ($LASTEXITCODE) { throw 'Existing policy test compilation failed' }
& "$scratch/policy.exe"
if ($LASTEXITCODE) { throw 'Existing policy test failed' }
$blood = [IO.File]::ReadAllText("$moduleRoot/src/blood_knight/BloodKnight.cpp")
$refresh = Function-Source $blood 'void Refresh('
if ($refresh.IndexOf('EnsureShieldSupport(player);') -lt 0 -or
    $refresh.IndexOf('EnsureShieldSupport(player);') -gt $refresh.IndexOf('!Settings.ManaEnable')) {
    throw 'Shield repair must precede the mana-only return'
}
$hooks = [IO.File]::ReadAllText("$moduleRoot/src/blood_knight/BloodKnightScripts.cpp")
$loadHook = Function-Source $hooks 'bool OnPlayerCheckItemInSlotAtLoadInventory('
if (!$loadHook.Contains('EnsureShieldSupport(player);') -or !$loadHook.Contains('return true;')) {
    throw 'Inventory loading must repair shields and retain normal equipment validation'
}
if ((Function-Source $hooks 'void OnPlayerUpdate(').Contains('EnsureShieldSupport')) {
    throw 'Shield initialization must not run on update ticks'
}
$shield = [IO.File]::ReadAllText("$PSScriptRoot/shield_test_support.h") + "`n"
$shield += (Function-Source $blood 'bool IsBloodKnight(') + "`n"
$shield += (Function-Source $blood 'void EnsureShieldSupport(') + "`n"
$shield += [IO.File]::ReadAllText("$PSScriptRoot/shield_test_cases.h")
[IO.File]::WriteAllText("$scratch/shield.cpp", $shield, $utf8)
& cl /nologo /std:c++20 /EHsc "$scratch/shield.cpp" "/Fo$scratch/shield.obj" "/Fe$scratch/shield.exe"
if ($LASTEXITCODE) { throw 'Shield regression compilation failed' }
& "$scratch/shield.exe"
if ($LASTEXITCODE) { throw 'Shield regression failed' }
Write-Host "Disposable test artifacts: $scratch"
if ($CompileModule) {
    # Compile the changed translation units against the installed core's generated include paths.
    # No linking, installation, CMake regeneration, database access or server process is involved.
    [xml]$project = Get-Content "$Core/build/modules/modules.vcxproj"
    $group = $project.Project.ItemDefinitionGroup | Select-Object -First 1
    $compileFlags = @('/nologo', '/std:c++20', '/EHsc', '/utf-8', '/bigobj', '/c',
        '/DMOD_PLAYERBOTS', '/DNOMINMAX', '/DWIN32', '/D_WINDOWS', '/D_WIN64',
        '/DNO_CORE_FUNCS', '/DBOOST_ALL_NO_LIB', '/DBOOST_ASIO_NO_DEPRECATED',
        '/D_CRT_SECURE_NO_WARNINGS', '/DSFMT_MEXP=19937', '/DHAVE_SSE2')
    foreach ($dir in ($group.ClCompile.AdditionalIncludeDirectories -split ';')) {
        if (Test-Path $dir) { $compileFlags += "/I$dir" }
    }
    foreach ($source in @('rp_phase1.cpp', 'rp_bot.cpp', 'blood_knight/BloodKnight.cpp',
        'blood_knight/BloodKnightScripts.cpp')) {
        $object = [IO.Path]::GetFileNameWithoutExtension($source)
        & cl @compileFlags "$moduleRoot/src/$source" "/Fo$scratch/$object.obj"
        if ($LASTEXITCODE) { throw "Module compilation failed: $source" }
    }
}
