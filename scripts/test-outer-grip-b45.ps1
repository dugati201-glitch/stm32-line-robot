$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$compilerBin='C:\Keil_v5\ARM\ARMCC\bin'
$testPath=Join-Path $repoPath 'build\outer-grip-b45-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    @'
from pathlib import Path
import zipfile
p=Path('build/outer-grip-b45-tests/b43');p.mkdir(exist_ok=True)
with zipfile.ZipFile('backups/B43-before-B44-20261030/B43-source.zip') as z:
    for n in ['control.c','control.h','control_tuning.h']:(p/n).write_bytes(z.read('app/'+n))
assert (p/'control.h').read_bytes()==Path('app/control.h').read_bytes()
'@ | python
    if($LASTEXITCODE -ne 0) { throw 'B43 comparison headers differ' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I "$testPath\b43" -DControl_Defaults=B43_Control_Defaults -DControl_Start=B43_Control_Start -DControl_Stop=B43_Control_Stop -DControl_Step=B43_Control_Step -c "$testPath\b43\control.c" -o "$testPath\b43.o"
    if($LASTEXITCODE -ne 0) { throw 'B43 compile failed' }
    foreach($source in @('app/control.c','tests/test_outer_grip_b45.c')) {
        $object=Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I app -c $source -o $object
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $axf=Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\b43.o" "$testPath\control.o" "$testPath\test_outer_grip_b45.o"
    if($LASTEXITCODE -ne 0) { throw 'B41 comparison link failed' }
    python scripts/emulate-control.py $axf
    if($LASTEXITCODE -ne 0) { throw 'B45 outer grip failed' }
} finally { Pop-Location }
