$ErrorActionPreference = 'Stop'
$repoPath = Split-Path -Parent $PSScriptRoot
$compilerBin = 'C:\Keil_v5\ARM\ARMCC\bin'
$testPath = Join-Path $repoPath 'build\control-tests'
New-Item -ItemType Directory -Force -Path $testPath | Out-Null
Push-Location $repoPath
try {
    # Keep the expanded soft-float suite within the emulator budget; firmware uses its own Keil options.
    foreach ($source in @('app/control.c','app/line_estimator.c','tests/test_control.c','app/robot.c','tests/test_robot.c','app/menu.c','tests/test_menu.c','tests/test_recovery.c','tests/test_speed.c','app/blackbox.c','tests/test_blackbox.c','tests/test_ambiguous.c','tests/test_direct_turn.c','tests/test_fixed_base.c','tests/test_corner_flow.c','tests/test_gap_memory.c','tests/test_scheduled.c','tests/legacy_profile.c','tests/test_local_centroid.c','tests/test_immediate_corner.c')) {
        $object = Join-Path $testPath (([IO.Path]::GetFileNameWithoutExtension($source)) + '.o')
        & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -Dsnprintf=TestSnprintf -I tests/fakes -I app -I bsp -c $source -o $object
        if ($LASTEXITCODE -ne 0) { throw "Compile failed: $source" }
    }
    $motorIncludes=@('-I','config','-I','vendor/Device/Include','-I','vendor/CMSIS/Include','-I','vendor/SPL/inc')
    $motorAliases=@('-DMotor_Init=DriverMotor_Init','-DMotor_SetDuty=DriverMotor_SetDuty','-DMotor_StopBrake=DriverMotor_StopBrake','-DMotor_StopCoast=DriverMotor_StopCoast','-DMotor_SafetyTick=DriverMotor_SafetyTick')
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -I bsp -c bsp/board.c -o "$testPath\driver_board.o"
    if ($LASTEXITCODE -ne 0) { throw 'Board driver test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -I bsp -c tests/test_board.c -o "$testPath\test_board.o"
    if ($LASTEXITCODE -ne 0) { throw 'Board register test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes @motorAliases -c bsp/motor.c -o "$testPath\driver_motor.o"
    if ($LASTEXITCODE -ne 0) { throw 'Motor driver test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -I tests/fakes -c tests/test_motor.c -o "$testPath\test_motor.o"
    if ($LASTEXITCODE -ne 0) { throw 'Motor register test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -DLogSerial_Init=DriverLogSerial_Init -DLogSerial_Write=DriverLogSerial_Write -DTime_Cycles=SerialTest_Cycles -DTime_SaveIRQ=SerialTest_SaveIRQ -DTime_RestoreIRQ=SerialTest_RestoreIRQ -c bsp/log_serial.c -o "$testPath\driver_log_serial.o"
    if ($LASTEXITCODE -ne 0) { throw 'Serial driver test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -DLOG_FLASH_BASE=0x60008000UL -DLogStorage_Slot=DriverLogStorage_Slot -DLogStorage_Begin=DriverLogStorage_Begin -DLogStorage_Erase=DriverLogStorage_Erase -DLogStorage_Write=DriverLogStorage_Write -DLogStorage_End=DriverLogStorage_End -c bsp/log_storage.c -o "$testPath\driver_log_storage.o"
    if ($LASTEXITCODE -ne 0) { throw 'Flash driver test compile failed' }
    & "$compilerBin\armcc.exe" --cpu Cortex-M3 --c99 -O1 -g --split_sections -DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER @motorIncludes -I tests/fakes -c tests/test_log_drivers.c -o "$testPath\test_log_drivers.o"
    if ($LASTEXITCODE -ne 0) { throw 'Log register test compile failed' }
    $axf = Join-Path $testPath 'tests.axf'
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --remove --output $axf "$testPath\control.o" "$testPath\line_estimator.o" "$testPath\test_control.o" "$testPath\robot.o" "$testPath\test_robot.o" "$testPath\menu.o" "$testPath\test_menu.o" "$testPath\test_recovery.o" "$testPath\test_speed.o" "$testPath\driver_motor.o" "$testPath\test_motor.o" "$testPath\blackbox.o" "$testPath\test_blackbox.o" "$testPath\driver_log_serial.o" "$testPath\driver_log_storage.o" "$testPath\test_log_drivers.o" "$testPath\test_ambiguous.o" "$testPath\test_direct_turn.o" "$testPath\test_fixed_base.o" "$testPath\test_corner_flow.o" "$testPath\test_gap_memory.o" "$testPath\test_scheduled.o" "$testPath\legacy_profile.o" "$testPath\test_local_centroid.o" "$testPath\test_immediate_corner.o" "$testPath\driver_board.o" "$testPath\test_board.o"
    if ($LASTEXITCODE -ne 0) { throw 'Test link failed' }
    python scripts/emulate-control.py $axf
    if ($LASTEXITCODE -ne 0) { throw 'Control tests failed' }
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry_io --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --remove --output "$testPath\tests-io.axf" "$testPath\control.o" "$testPath\line_estimator.o" "$testPath\test_control.o" "$testPath\robot.o" "$testPath\test_robot.o" "$testPath\menu.o" "$testPath\test_menu.o" "$testPath\test_recovery.o" "$testPath\test_speed.o" "$testPath\driver_motor.o" "$testPath\test_motor.o" "$testPath\blackbox.o" "$testPath\test_blackbox.o" "$testPath\driver_log_serial.o" "$testPath\driver_log_storage.o" "$testPath\test_log_drivers.o" "$testPath\test_ambiguous.o" "$testPath\test_direct_turn.o" "$testPath\test_fixed_base.o" "$testPath\test_corner_flow.o" "$testPath\test_gap_memory.o" "$testPath\test_scheduled.o" "$testPath\legacy_profile.o" "$testPath\test_local_centroid.o" "$testPath\test_immediate_corner.o" "$testPath\driver_board.o" "$testPath\test_board.o"
    if ($LASTEXITCODE -ne 0) { throw 'IO test link failed' }
    python scripts/emulate-control.py "$testPath\tests-io.axf" test_entry_io
    if ($LASTEXITCODE -ne 0) { throw 'IO/UI tests failed' }
    & "$compilerBin\armlink.exe" --cpu Cortex-M3 --entry test_entry_motion --ro-base 0x08000000 --rw-base 0x20000000 --libpath 'C:\Keil_v5\ARM\ARMCC\lib' --remove --output "$testPath\tests-motion.axf" "$testPath\control.o" "$testPath\line_estimator.o" "$testPath\test_control.o" "$testPath\robot.o" "$testPath\test_robot.o" "$testPath\menu.o" "$testPath\test_menu.o" "$testPath\test_recovery.o" "$testPath\test_speed.o" "$testPath\driver_motor.o" "$testPath\test_motor.o" "$testPath\blackbox.o" "$testPath\test_blackbox.o" "$testPath\driver_log_serial.o" "$testPath\driver_log_storage.o" "$testPath\test_log_drivers.o" "$testPath\test_ambiguous.o" "$testPath\test_direct_turn.o" "$testPath\test_fixed_base.o" "$testPath\test_corner_flow.o" "$testPath\test_gap_memory.o" "$testPath\test_scheduled.o" "$testPath\legacy_profile.o" "$testPath\test_local_centroid.o" "$testPath\test_immediate_corner.o" "$testPath\driver_board.o" "$testPath\test_board.o"
    if ($LASTEXITCODE -ne 0) { throw 'Motion test link failed' }
    python scripts/emulate-control.py "$testPath\tests-motion.axf" test_entry_motion
    if ($LASTEXITCODE -ne 0) { throw 'Motion tests failed' }
    python tests/test_log_tools.py
    if ($LASTEXITCODE -ne 0) { throw 'CSV/layout tests failed' }
    python tests/test_receive_blackbox.py
    if ($LASTEXITCODE -ne 0) { throw 'Serial receiver tests failed' }
    # Separate images preserve the existing emulator map/count/time bounds.
    & (Join-Path $PSScriptRoot 'test-corner-video.ps1')
    & (Join-Path $PSScriptRoot 'test-high-kd.ps1')
    & (Join-Path $PSScriptRoot 'test-corner-b38.ps1')
    & (Join-Path $PSScriptRoot 'test-yaw-reserve.ps1')
    & (Join-Path $PSScriptRoot 'test-branch-b40.ps1')
    & (Join-Path $PSScriptRoot 'test-carve-b41.ps1')
    & (Join-Path $PSScriptRoot 'test-swing-b43.ps1')
    & (Join-Path $PSScriptRoot 'test-outer-grip-b45.ps1')
} finally { Pop-Location }
