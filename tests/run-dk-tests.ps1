param(
    [string]$Core = (Resolve-Path "$PSScriptRoot/../../..").Path,
    [string]$VsDevCmd = 'C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/VsDevCmd.bat',
    [switch]$CompileModule
)
$ErrorActionPreference = 'Stop'
$moduleRoot = Split-Path $PSScriptRoot
$utf8 = New-Object Text.UTF8Encoding($false)
$scratch = Join-Path ([IO.Path]::GetTempPath()) ('roleplay-dk-tests-' + [guid]::NewGuid())
[IO.Directory]::CreateDirectory($scratch) | Out-Null
[IO.File]::WriteAllText("$scratch/environment.cmd", "@call `"$VsDevCmd`" -arch=x64 >nul`r`n@set`r`n")
cmd /c "$scratch/environment.cmd" | ForEach-Object {
    if ($_ -cmatch '^Path=') { return } # Keep developer prompt PATH if the launcher also supplied Path.
    if ($_ -match '^([^=]+)=(.*)$') {
        [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process')
    }
}
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
function Require-Text([string]$source, [string]$text) {
    if (!$source.Contains($text)) { throw "Core routing assumption changed: $text" }
}
# These checks bind the hook tests to the available fork's routing. They are not combat simulation.
$unit = [IO.File]::ReadAllText("$Core/src/server/game/Entities/Unit/Unit.cpp")
$spell = [IO.File]::ReadAllText("$Core/src/server/game/Spells/Spell.cpp")
$effects = [IO.File]::ReadAllText("$Core/src/server/game/Spells/SpellEffects.cpp")
$auras = [IO.File]::ReadAllText("$Core/src/server/game/Spells/Auras/SpellAuraEffects.cpp")
$dk = [IO.File]::ReadAllText("$Core/src/server/scripts/Spells/spell_dk.cpp")
$strike = Function-Source $effects 'void Spell::EffectWeaponDmg('
Require-Text $strike 'weaponDamage += fixed_bonus;'
Require-Text $strike 'm_damage += eff_damage;'
Require-Text $spell 'caster->CalculateSpellDamageTaken(&damageInfo, m_damage, m_spellInfo'
$direct = Function-Source $unit 'void Unit::CalculateSpellDamageTaken('
if ([regex]::Matches($direct, 'sScriptMgr->ModifySpellDamageTaken\(').Count -ne 1) {
    throw 'Expected one direct hook after weapon/spell aggregation'
}
foreach ($signature in @('void AuraEffect::HandlePeriodicDamageAurasTick(',
    'void AuraEffect::HandlePeriodicHealthLeechAuraTick(')) {
    $periodic = Function-Source $auras $signature
    if ([regex]::Matches($periodic, 'sScriptMgr->ModifyPeriodicDamageAurasTick\(').Count -ne 1 -or
        $periodic.Contains('CalculateSpellDamageTaken(')) { throw 'Periodic routing changed' }
}
$white = Function-Source $unit 'void Unit::CalculateMeleeDamage('
if ($white.Contains('ModifySpellDamageTaken')) { throw 'White damage must not enter the ability hook' }
Require-Text $dk 'SPELL_DK_UNHOLY_BLIGHT_DOT                   = 50536'
Require-Text $dk 'eventInfo.GetDamageInfo()->GetDamage()'
Require-Text $dk 'CastDelayedSpellWithPeriodicAmount(caster, SPELL_DK_UNHOLY_BLIGHT_DOT'
Require-Text $dk 'OnEffectPeriodic += AuraEffectPeriodicFn(spell_dk_death_and_decay_aura::HandlePeriodic'
Write-Host 'Installed core damage routing checks passed'

foreach ($test in @('death_knight_policy_test', 'death_knight_damage_test')) {
    & cl /nologo /std:c++20 /EHsc "$PSScriptRoot/$test.cpp" "/Fo$scratch/$test.obj" "/Fe$scratch/$test.exe"
    if ($LASTEXITCODE) { throw "Compilation failed: $test" }
    & "$scratch/$test.exe"
    if ($LASTEXITCODE) { throw "Test failed: $test" }
}
$code = [IO.File]::ReadAllText("$PSScriptRoot/death_knight_hook_support.h") + "`n"
$production = [IO.File]::ReadAllText("$moduleRoot/src/death_knight/RoleplayDeathKnightScaling.cpp")
$code += $production.Substring($production.IndexOf('namespace Roleplay::DeathKnight')) + "`n"
$code += [IO.File]::ReadAllText("$PSScriptRoot/death_knight_hook_cases.h")
[IO.File]::WriteAllText("$scratch/hooks.cpp", $code, $utf8)
$includes = @("$Core/src/common", "$Core/src/common/Utilities", "$Core/src/common/Debugging",
    "$Core/src/server/shared", "$Core/src/server/shared/DataStores",
    "$Core/src/server/game/Spells/Auras", "$Core/deps/fmt/include",
    "$moduleRoot/src/death_knight") | ForEach-Object { "/I$_" }
& cl /nologo /std:c++20 /EHsc /utf-8 @includes "$scratch/hooks.cpp" "/Fo$scratch/hooks.obj" "/Fe$scratch/hooks.exe"
if ($LASTEXITCODE) { throw 'Actual hook test compilation failed' }
& "$scratch/hooks.exe"
if ($LASTEXITCODE) { throw 'Actual hook test failed' }

$progression = [IO.File]::ReadAllText("$moduleRoot/src/death_knight/RoleplayDeathKnightProgression.cpp")
$code = [IO.File]::ReadAllText("$PSScriptRoot/death_knight_progression_support.h") + "`n"
$code += "namespace Roleplay::DeathKnight {`n"
$start = $progression.IndexOf('struct Unlock')
$end = $progression.IndexOf("};", $progression.IndexOf('constexpr Unlock Unlocks[]')) + 2
$code += $progression.Substring($start, $end - $start) + "`n"
$code += (Function-Source $progression 'void ApplyProgression(') + "`n}`n"
$code += [IO.File]::ReadAllText("$PSScriptRoot/death_knight_progression_cases.h")
[IO.File]::WriteAllText("$scratch/progression.cpp", $code, $utf8)
& cl /nologo /std:c++20 /EHsc @includes "$scratch/progression.cpp" `
    "/Fo$scratch/progression.obj" "/Fe$scratch/progression.exe"
if ($LASTEXITCODE) { throw 'Actual progression test compilation failed' }
& "$scratch/progression.exe"
if ($LASTEXITCODE) { throw 'Actual progression test failed' }

& cl /nologo /std:c++20 /EHsc "$PSScriptRoot/death_knight_payload_model.cpp" `
    "/Fo$scratch/model.obj" "/Fe$scratch/model.exe"
if ($LASTEXITCODE) { throw 'Payload model compilation failed' }
# Synthetic inputs only, not claimed to describe Aldric, real gear or any particular level.
& "$scratch/model.exe" 15 30 0 3
if ($LASTEXITCODE) { throw 'Payload model failed' }

if ($CompileModule) {
    # Reuse only include paths from the installed generated build; never regenerate/link/install it.
    [xml]$project = Get-Content "$Core/build/modules/modules.vcxproj"
    $group = $project.Project.ItemDefinitionGroup | Select-Object -First 1
    $flags = @('/nologo', '/std:c++20', '/EHsc', '/utf-8', '/bigobj', '/c', '/DMOD_PLAYERBOTS',
        '/DNOMINMAX', '/DWIN32', '/D_WINDOWS', '/D_WIN64', '/DNO_CORE_FUNCS', '/DBOOST_ALL_NO_LIB',
        '/DBOOST_ASIO_NO_DEPRECATED', '/D_CRT_SECURE_NO_WARNINGS', '/DSFMT_MEXP=19937', '/DHAVE_SSE2')
    foreach ($dir in ($group.ClCompile.AdditionalIncludeDirectories -split ';')) {
        if (Test-Path $dir) { $flags += "/I$dir" }
    }
    foreach ($source in (Get-ChildItem "$moduleRoot/src/death_knight/*.cpp")) {
        & cl @flags $source.FullName "/Fo$scratch/$($source.BaseName).obj"
        if ($LASTEXITCODE) { throw "Production compilation failed: $($source.Name)" }
    }
}
Write-Host "Disposable test/build artifacts: $scratch"
