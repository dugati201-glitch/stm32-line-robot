"""Compile archived B41/current B42 with their own headers; replay CRC-checked raw ADC.
Uses defaults of each version. Sampled commands are not a vehicle/trajectory model.
"""
import csv
import hashlib
import importlib.util
import json
from pathlib import Path
import struct
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'build/test-deps'))
from elftools.elf.elffile import ELFFile
from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_SP


def emulate(path):
    with path.open('rb') as stream:
        elf = ELFFile(stream)
        cpu = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
        cpu.mem_map(0x08000000, 0x10000)
        cpu.mem_map(0x20000000, 0x5000)
        for section in elf.iter_sections():
            if section['sh_flags'] & 2 and section['sh_type'] != 'SHT_NOBITS':
                cpu.mem_write(section['sh_addr'], section.data())
        symbols = {s.name: s['st_value'] for s in elf.get_section_by_name('.symtab').iter_symbols()}
        done = symbols['test_done'] & ~1
        completed = []
        cpu.hook_add(UC_HOOK_CODE, lambda *args: (completed.append(True), cpu.emu_stop()), begin=done, end=done)
        cpu.reg_write(UC_ARM_REG_SP, 0x20004FF0)
        cpu.emu_start(symbols['test_entry'] | 1, 0, timeout=60000000, count=300000000)
        assert completed, 'Replay did not complete'
        count = int.from_bytes(cpu.mem_read(symbols['trace_count'], 4), 'little')
        assert count == 160
        rows = list(struct.iter_unpack('<12i', bytes(cpu.mem_read(symbols['trace'], count * 48))))
        assert all(-850 <= row[5] <= 850 and -850 <= row[6] <= 850 for row in rows)
        return rows


def main():
    work = ROOT / 'build/active-eye-b42-comparison'
    reference = work / 'b41'
    reference.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ROOT / 'backups/B41-before-B42-20261030/B41-source.zip') as archive:
        for filename in ('control.c', 'control.h', 'control_tuning.h', 'line_estimator.c', 'line_estimator.h'):
            (reference / filename).write_bytes(archive.read('app/' + filename))
    spec = importlib.util.spec_from_file_location('reader', ROOT / 'scripts/read-blackbox.py')
    reader = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(reader)
    fixture = ROOT / 'tests/fixtures/b39-corner-1cca0436.csv'
    text = fixture.read_text(encoding='utf-8-sig')
    log = reader.parse_log(text)
    assert log['crc32'] == '1CCA0436'
    samples = log['rows']
    lines = []
    for field in ('low', 'high', 'white'):
        row = next(line for line in text.splitlines() if line.startswith('# calib_' + field + ','))
        values = [int(value) for value in row.split(',')[1:]]
        assert len(values) == 8 and all(0 <= value <= 4095 for value in values)
        lines.append('static const unsigned short ' + field + '[8]={' + ','.join(map(str, values)) + '};')
    lines.append('static const unsigned timestamps[160]={' + ','.join(str(row['ms']) for row in samples) + '};')
    lines.append('static const unsigned short raw[160][8]={' + ','.join(
        '{' + ','.join(str(row['raw' + str(i)]) for i in range(8)) + '}' for row in samples) + '};')
    (work / 'input.h').write_text('\n'.join(lines) + '\n')
    b42_source = ROOT / 'app'
    if '"B42"' not in (ROOT / 'config/board.h').read_text():
        b42_source = work / 'b42'
        b42_source.mkdir(exist_ok=True)
        with zipfile.ZipFile(ROOT / 'backups/B42-before-B43-20261030/B42-source.zip') as archive:
            for filename in ('control.c', 'control.h', 'control_tuning.h', 'line_estimator.c', 'line_estimator.h'):
                (b42_source / filename).write_bytes(archive.read('app/' + filename))
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name, source in [('b41', reference), ('b42', b42_source)]:
        common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source), '-I', str(work)]
        if name == 'b42':
            common.append('-DTRACE_ACTIVE_EYE')
        objects = []
        for file in ('control.c', 'line_estimator.c', 'test_active_eye_trace.c'):
            path = source / file if file != 'test_active_eye_trace.c' else ROOT / 'tests' / file
            obj = work / (name + '-' + Path(file).stem + '.o')
            subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(path), '-o', str(obj)], check=True)
            objects.append(str(obj))
        axf = work / (name + '.axf')
        subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                        '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                        str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf), *objects], check=True)
        traces[name] = emulate(axf)
        with (work / (name + '.csv')).open('w', newline='') as stream:
            writer = csv.writer(stream)
            writer.writerow(['ms', 'error', 'valid', 'mask', 'active_eye', 'left', 'right', 'mode',
                             'reason', 'applied_d_x1000', 'common_x10', 'confidence'])
            writer.writerows(traces[name])
    old = {row[0]: row for row in traces['b41']}
    new = {row[0]: row for row in traces['b42']}
    assert old[32538][2] == 0 and new[32538][2] == 1
    assert new[32538][4] == 4 and new[32538][6] < 0
    for ms in (32538, 32540, 32544, 32548):
        a, b = old[ms], new[ms]
        assert b[5] <= 350 and b[6] < 0 and b[5] + b[6] > 0
        assert b[5] + b[6] < a[5] + a[6]
        print(f'{ms}: B41 valid={a[2]} L/R={a[5]/10:.1f}/{a[6]/10:.1f}; '
              f'B42 valid={b[2]} eye={b[4]} L/R={b[5]/10:.1f}/{b[6]/10:.1f}')
    (work / 'summary.json').write_text(json.dumps({
        'fixture_sha256': hashlib.sha256(fixture.read_bytes()).hexdigest(), 'crc32': log['crc32'],
        'count': 160, 'profiles': {'b41': '55/85,12/0/0.9', 'b42': '55/85,12/0/0.1'},
        'note': 'Identical sampled raw input, different controller defaults. No plant, lap or physical yaw prediction.'
    }, indent=2))
    print('PASS: sampled raw replay; all-white input still invokes bounded recovery in both versions.')


if __name__ == '__main__':
    main()
