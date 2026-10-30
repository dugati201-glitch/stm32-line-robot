"""Verify B33 restores B28 commands on P-only and default-PD corner traces.

No vehicle dynamics or lap prediction. Historical versions use local backups.
"""
import csv
import importlib.util
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('corner_compare', ROOT / 'scripts/compare-corner.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


def main():
    work = ROOT / 'build/b28-restore-comparison'
    work.mkdir(parents=True, exist_ok=True)
    sources = {'b33': ROOT / 'app/control.c'}
    for name, backup in [('b28', 'B28-before-B29-20261029/B28-source.zip'),
                         ('b32', 'B32-before-B33-20261029/B32-source.zip')]:
        saved = work / name
        saved.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(ROOT / 'backups' / backup) as archive:
            for filename in ['control.c', 'control.h', 'control_tuning.h']:
                (saved / filename).write_bytes(archive.read('app/' + filename))
        sources[name] = saved / 'control.c'
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    print('Requested PWM/mode only, no physical speed or lap prediction.')
    for scenario, defines in [('pure-P', []), ('default-PD', ['-DTRACE_DEFAULT_PID=1'])]:
        traces = {}
        for name in ['b28', 'b32', 'b33']:
            source = sources[name]
            label = scenario + '-' + name
            control, entry, axf = [work / (label + ext) for ext in ('.o', '-entry.o', '.axf')]
            common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source.parent)]
            subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(source), '-o', str(control)], check=True)
            subprocess.run([str(compiler / 'armcc.exe'), *common, *defines, '-c',
                            str(ROOT / 'tests/test_rolling_trace.c'), '-o', str(entry)], check=True)
            subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                            '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                            str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                            str(control), str(entry)], check=True)
            rows = traces[name] = helper.emulate(axf, allow_recovery=True)
            with (work / (label + '.csv')).open('w', newline='', encoding='utf-8') as stream:
                writer = csv.writer(stream)
                writer.writerow(['ms', 'error_x1000', 'correction_x1000', 'left_x10', 'right_x10',
                                 'target_x1000', 'mode', 'recovery_arc'])
                writer.writerows(rows)
        assert traces['b33'] == traces['b28'], f'{scenario}: restored command trace differs from B28'
        assert traces['b33'] != traces['b32'], f'{scenario}: B32 policy was not removed'
        print(scenario + ': B33 matches all 64 B28 frames exactly.')
        for name, rows in traces.items():
            pairs = [f'{rows[i][3]/10:.1f}/{rows[i][4]/10:.1f}' for i in (25, 36, 43)]
            print(f'  {name}: extreme {pairs[0]}, loss+20ms {pairs[1]}, loss+34ms {pairs[2]}')
    print('PASS: both B28 reference traces restored. Recovery fixes are covered separately by C tests.')


if __name__ == '__main__':
    main()
