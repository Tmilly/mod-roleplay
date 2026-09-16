param([Parameter(Mandatory = $true)][string]$DbcPath)
$ErrorActionPreference = 'Stop'
# Read only. Offsets match this core's documented 3.3.5 SpellEntry in DBCStructure.h.
$bytes = [IO.File]::ReadAllBytes((Join-Path $DbcPath 'Spell.dbc'))
if ([Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'WDBC') { throw 'Expected WDBC Spell.dbc' }
$count = [BitConverter]::ToUInt32($bytes, 4)
$fields = [BitConverter]::ToUInt32($bytes, 8)
$size = [BitConverter]::ToUInt32($bytes, 12)
if ($fields -ne 234 -or $size -ne 936) { throw 'Expected stock 3.3.5 Spell.dbc layout' }
$records = @{}
for ($i = 0; $i -lt $count; $i++) {
    $offset = 20 + $i * $size
    $id = [BitConverter]::ToUInt32($bytes, $offset)
    $family = [BitConverter]::ToUInt32($bytes, $offset + 208 * 4)
    $records[[int]$id] = $offset
    if ($family -eq 15) {
        for ($j = 0; $j -lt 3; $j++) {
            $effect = [BitConverter]::ToUInt32($bytes, $offset + (71 + $j) * 4)
            $aura = [BitConverter]::ToUInt32($bytes, $offset + (95 + $j) * 4)
            if (($effect -eq 27 -and $aura -in 3, 53) -or $aura -eq 89) {
                throw "Re-audit DK dynamic/percent periodic path for spell $id"
            }
        }
    }
}
# ID -> family, first effect, first effective base, DBC spell level.
$expected = @{
    45477 = @(15, 2, 127, 55); 45462 = @(15, 121, 125, 55); 45902 = @(15, 121, 260, 55)
    47541 = @(15, 3, 167, 55); 47632 = @(15, 2, 600, 55); 49998 = @(15, 121, 112, 56)
    49020 = @(15, 121, 248, 61); 56815 = @(15, 58, 0, 67); 55078 = @(15, 6, 0, 0)
    55095 = @(15, 6, 0, 0); 50536 = @(15, 6, 1, 55); 50526 = @(0, 2, 1, 0)
    52212 = @(15, 2, 1, 0); 51460 = @(15, 2, 1, 55)
    78 = @(4, 58, 11, 1); 284 = @(4, 58, 21, 8); 285 = @(4, 58, 32, 16)
    1608 = @(4, 58, 44, 24); 11565 = @(4, 58, 93, 40); 11566 = @(4, 58, 136, 48)
}
foreach ($id in ($expected.Keys | Sort-Object)) {
    if (!$records.ContainsKey($id)) { throw "Missing spell $id" }
    $offset = $records[$id]
    $actual = @([BitConverter]::ToUInt32($bytes, $offset + 208 * 4),
        [BitConverter]::ToUInt32($bytes, $offset + 71 * 4),
        ([BitConverter]::ToInt32($bytes, $offset + 80 * 4) + 1),
        [BitConverter]::ToUInt32($bytes, $offset + 39 * 4))
    if (($actual -join ',') -ne ($expected[$id] -join ',')) { throw "Spell $id differs: $actual" }
}
Write-Host 'Installed Spell.dbc family, rank, payload and periodic-path assumptions passed'
