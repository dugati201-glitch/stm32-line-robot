"""Compare B36/B37 actual ARM requests at 55/85,Kd0.9. No lap prediction."""
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
    work = ROOT / 'build/high-kd-comparison'
    reference = work / 'reference-b36'
    reference.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(ROOT / 'backups/B36-before-B37-20261029/B36-source.zip') as archive:
        for filename in ['control.c', 'control.h', 'control_tuning.h']:
            (reference / filename).write_bytes(archive.read('app/' + filename))
    # Freeze this historical B36/B37 comparison once a later controller exists.
    b37 = work / 'reference-b37'
    b37_archive = ROOT / 'backups/B37-before-B38-20261030/B37-source.zip'
    b37_source = ROOT / 'app/control.c'
    if b37_archive.exists():
        b37.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(b37_archive) as archive:
            for filename in ['control.c', 'control.h', 'control_tuning.h']:
                (b37 / filename).write_bytes(archive.read('app/' + filename))
        b37_source = b37 / 'control.c'
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    print('Fixed profile55/85,Kp12,Ki0,Kd0.9; input scripts, not a physical trajectory.')
    for scenario, defines in [('jitter', []), ('exit', ['-DTRACE_EXIT=1'])]:
        traces = {}
        for name, source in [('b36', reference / 'control.c'), ('b37', b37_source)]:
            label = scenario + '-' + name
            control, entry, axf = [work / (label + ext) for ext in ('.o', '-entry.o', '.axf')]
            common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source.parent)]
            subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(source), '-o', str(control)], check=True)
            subprocess.run([str(compiler / 'armcc.exe'), *common, *defines, '-c',
                            str(ROOT / 'tests/test_high_kd_trace.c'), '-o', str(entry)], check=True)
            subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                            '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                            str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                            str(control), str(entry)], check=True)
            rows = traces[name] = helper.emulate(axf)
            with (work / (label + '.csv')).open('w', newline='', encoding='utf-8') as stream:
                writer = csv.writer(stream)
                writer.writerow(['ms', 'error_x1000', 'correction_x1000', 'left_x10', 'right_x10',
                                 'target_x1000', 'mode', 'speed_x1000'])
                writer.writerows(rows)
        if scenario == 'jitter':
            peaks = {name: max(abs(row[2]) for row in rows) / 1000 for name, rows in traces.items()}
            print(f'Alternating +/-0.5pitch: peak requested correction B36={peaks["b36"]:.2f}%, B37={peaks["b37"]:.2f}%.')
            assert peaks['b37'] < 12 < peaks['b36']
        else:
            times = {name: next((row[0] for row in rows if row[7] >= 54999), None) for name, rows in traces.items()}
            print(f'Calm fast-exit43->55 commanded duty: B36={times["b36"] or ">128"}ms, B37={times["b37"]}ms.')
            assert times['b36'] is None and times['b37'] == 50
    print('PASS: reduced scripted D jitter and faster eligible exit request; no measured lap/speed claim.')


if __name__ == '__main__':
    main()
