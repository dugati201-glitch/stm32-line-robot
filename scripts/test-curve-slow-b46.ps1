$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$compilerBin='C:\Keil_v5\ARM\ARMCC\bin'
$testPath=Join-Path $repoPath 'build\curve-slow-b46-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    @'
from pathlib import Path
import zipfile
p=Path('build/curve-slow-b46-tests/b45');p.mkdir(exist_ok=True)
with zipfile.ZipFile('backups/B45-before-B46-20261030/B45-source.zip') as z:
    for n in ['control.c','control.h','control_tuning.h']:(p/n).write_bytes(z.read('app/'+n))
assert (p/'control.h').read_bytes()==Path('app/control.h').read_bytes()
'@ | python
    if($LASTEXITCODE -ne 0) { throw 'B45 comparison headers differ' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I "$testPath\b45" -DControl_Defaults=B45_Control_Defaults -DControl_Start=B45_Control_Start -DControl_Stop=B45_Control_Stop -DControl_Step=B45_Control_Step -c "$testPath\b45\control.c" -o "$testPath\b45.o"
    if($LASTEXITCODE -ne 0) { throw 'B45 compile failed' }
    foreach($source in @('app/control.c','tests/test_curve_slow_b46.c')) {
        $object=Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I app -c $source -o $object
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $axf=Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\b45.o" "$testPath\control.o" "$testPath\test_curve_slow_b46.o"
    if($LASTEXITCODE -ne 0) { throw 'B46 comparison link failed' }
    python scripts/emulate-control.py $axf
    if($LASTEXITCODE -ne 0) { throw 'B46 curve speed failed' }
} finally { Pop-Location }
