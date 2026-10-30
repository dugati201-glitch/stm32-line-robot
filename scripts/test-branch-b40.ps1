$ErrorActionPreference='Stop'
$repoPath=Split-Path -Parent $PSScriptRoot
$compilerBin='C:\Keil_v5\ARM\ARMCC\bin'
$testPath=Join-Path $repoPath 'build\branch-b40-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    foreach($source in @('app/control.c','tests/test_branch_b40.c')) {
        $object=Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g -I app -c $source -o $object
        if($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $axf=Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --no_remove --output $axf "$testPath\control.o" "$testPath\test_branch_b40.o"
    if($LASTEXITCODE -ne 0) { throw 'Branch-entry test link failed' }
    python scripts/emulate-control.py $axf
    if($LASTEXITCODE -ne 0) { throw 'Branch-entry tests failed' }
} finally { Pop-Location }
