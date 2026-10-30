param([switch]$BaselineB41)
$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$compilerBin='C:\Keil_v5\ARM\ARMCC\bin'
$testPath=Join-Path $repoPath $(if($BaselineB41){'build\swing-b43-baseline'}else{'build\swing-b43-tests'})
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    $controlSource='app/control.c'
    $includePath='app'
    if($BaselineB41) {
        @'
from pathlib import Path
import zipfile
p=Path('build/swing-b43-baseline');p.mkdir(exist_ok=True)
with zipfile.ZipFile('backups/B41-before-B42-20261030/B41-source.zip') as z:
    for n in ['control.c','control.h','control_tuning.h']:(p/n).write_bytes(z.read('app/'+n))
'@ | python
        if($LASTEXITCODE -ne 0) { throw 'Baseline extraction failed' }
        $controlSource=Join-Path $testPath 'control.c'
        $includePath=$testPath
    }
    foreach($source in @($controlSource,'tests/test_swing_b43.c')) {
        $object=Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I $includePath -c $source -o $object
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $axf=Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\control.o" "$testPath\test_swing_b43.o"
    if($LASTEXITCODE -ne 0) { throw 'Swing link failed' }
    python scripts/emulate-control.py $axf
    if($LASTEXITCODE -ne 0) { throw 'Swing-turn tests failed' }
} finally { Pop-Location }
