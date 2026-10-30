"""Compare B38/B39 commands at equal gains/inputs. No motor model or lap prediction."""
import csv
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
        assert completed, 'Trace did not complete'
        count = int.from_bytes(cpu.mem_read(symbols['trace_count'], 4), 'little')
        assert count == 64
        rows = list(struct.iter_unpack('<8i', bytes(cpu.mem_read(symbols['trace'], count * 32))))
        assert all(-150 <= row[2] <= 850 and -150 <= row[3] <= 850 and row[4] in (1, 2) for row in rows)
        return rows


def main():
    work = ROOT / 'build/yaw-reserve-comparison'
    reference = work / 'b38'
    reference.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ROOT / 'backups/B38-before-B39-20261030/B38-source.zip') as archive:
        for filename in ('control.c', 'control.h', 'control_tuning.h'):
            (reference / filename).write_bytes(archive.read('app/' + filename))
    frozen = work / 'b39'
    frozen.mkdir(exist_ok=True)
    with zipfile.ZipFile(ROOT / 'backups/B39-before-B40-20261030/B39-source.zip') as archive:
        for filename in ('control.c', 'control.h', 'control_tuning.h'):
            (frozen / filename).write_bytes(archive.read('app/' + filename))
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name, source in [('b38', reference / 'control.c'), ('b39', frozen / 'control.c')]:
        control = work / (name + '.o')
        common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source.parent)]
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(source), '-o', str(control)], check=True)
        for base in (20, 30, 45, 55):
            for scenario, label in enumerate(('corner', 'soft', 'no-cue', 'dash')):
                filename = f'{name}-{base}-{label}'
                entry, axf = [work / (filename + suffix) for suffix in ('-entry.o', '.axf')]
                subprocess.run([str(compiler / 'armcc.exe'), *common,
                                f'-DTRACE_BASE={base}', f'-DTRACE_SCENARIO={scenario}', '-c',
                                str(ROOT / 'tests/test_yaw_reserve_trace.c'), '-o', str(entry)], check=True)
                subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                                '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                                str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                                str(control), str(entry)], check=True)
                rows = traces[name, base, label] = emulate(axf)
                with (work / (filename + '.csv')).open('w', newline='', encoding='utf-8') as stream:
                    writer = csv.writer(stream)
                    writer.writerow(['ms', 'error_x1000', 'left_x10', 'right_x10', 'mode',
                                     'correction_x1000', 'corner_gain_x1000', 'recovery_ms'])
                    writer.writerows(rows)
    for base in (20, 30, 45, 55):
        for label in ('corner', 'soft', 'no-cue', 'dash'):
            if base <= 30 or label != 'corner':
                assert traces['b38', base, label] == traces['b39', base, label], (base, label)
        old, new = [traces[name, base, 'corner'] for name in ('b38', 'b39')]
        print(f'Base{base}: inner min B38={min(r[3] for r in old)/10:.1f}%, '
              f'B39={min(r[3] for r in new)/10:.1f}%; '
              f'at8ms B38 L/R={old[3][2]/10:.1f}/{old[3][3]/10:.1f}, '
              f'B39={new[3][2]/10:.1f}/{new[3][3]/10:.1f}.')
        if base >= 45:
            assert old[3][3] >= 0 > new[3][3]
            assert all(r[2] + r[3] > 0 for r in new)
            assert old[25:] == new[25:], 'Ordinary returning-line response changed'
    print('PASS: low Base, ordinary soft curve, no-cue edge and dash command traces unchanged; '
          'confirmed tight corners gain bounded signed authority. No physical speed/2cm/lap claim.')


if __name__ == '__main__':
    main()
