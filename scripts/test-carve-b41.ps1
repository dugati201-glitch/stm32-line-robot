$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$compilerBin='C:\Keil_v5\ARM\ARMCC\bin'
$testPath=Join-Path $repoPath 'build\carve-b41-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    @'
from pathlib import Path
import zipfile
p=Path('build/carve-b41-tests/b40');p.mkdir(exist_ok=True)
with zipfile.ZipFile('backups/B40-before-B41-20261030/B40-source.zip') as z:
    for n in ['control.c','control.h','control_tuning.h']:(p/n).write_bytes(z.read('app/'+n))
assert (p/'control.h').read_bytes()==Path('app/control.h').read_bytes()
'@ | python
    if($LASTEXITCODE -ne 0) { throw 'B40 comparison headers differ' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I "$testPath\b40" -DControl_Defaults=B40_Control_Defaults -DControl_Start=B40_Control_Start -DControl_Stop=B40_Control_Stop -DControl_Step=B40_Control_Step -c "$testPath\b40\control.c" -o "$testPath\b40.o"
    if($LASTEXITCODE -ne 0) { throw 'B40 compile failed' }
    foreach($source in @('app/control.c','tests/test_carve_b41.c')) {
        $object=Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I app -c $source -o $object
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $axf=Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\b40.o" "$testPath\control.o" "$testPath\test_carve_b41.o"
    if($LASTEXITCODE -ne 0) { throw 'B41 comparison link failed' }
    python scripts/emulate-control.py $axf
    if($LASTEXITCODE -ne 0) { throw 'B41 carve regression failed' }
} finally { Pop-Location }
