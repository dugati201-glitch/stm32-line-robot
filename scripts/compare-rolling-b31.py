"""Compare B28/B30/B31 rolling corner commands. No lap/trajectory prediction."""
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
    work = ROOT / 'build/rolling-b31-comparison'
    work.mkdir(parents=True, exist_ok=True)
    sources = {'b31': ROOT / 'app/control.c'}
    references = [('b28', 'B28-before-B29-20261029/B28-source.zip'),
                  ('b30', 'B30-before-B31-20261029/B30-source.zip')]
    historical = 'B31-before-B32-20261029/B31-source.zip'
    if (ROOT / 'backups' / historical).exists():
        references.append(('b31', historical))
    for name, backup in references:
        saved = work / name
        saved.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(ROOT / 'backups' / backup) as archive:
            for filename in ['control.c', 'control.h', 'control_tuning.h']:
                (saved / filename).write_bytes(archive.read('app/' + filename))
        sources[name] = saved / 'control.c'
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name in ['b28', 'b30', 'b31']:
        source = sources[name]
        control, entry, axf = [work / (name + ext) for ext in ('.o', '-entry.o', '.axf')]
        common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source.parent)]
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(source), '-o', str(control)], check=True)
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(ROOT / 'tests/test_rolling_trace.c'),
                        '-o', str(entry)], check=True)
        subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                        '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                        str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                        str(control), str(entry)], check=True)
        rows = traces[name] = helper.emulate(axf, allow_recovery=True)
        with (work / (name + '.csv')).open('w', newline='', encoding='utf-8') as stream:
            writer = csv.writer(stream)
            writer.writerow(['ms', 'error_x1000', 'correction_x1000', 'left_x10', 'right_x10',
                             'target_x1000', 'mode', 'recovery_arc'])
            writer.writerows(rows)
    print('Actual ARM command traces, pure P. No physical wheel speed or lap prediction.')
    print('candidate  visible extreme L/R   loss+20ms L/R   loss+34ms L/R   valid rejoin L/R')
    for name, rows in traces.items():
        pairs = [f'{rows[i][3]/10:.1f}/{rows[i][4]/10:.1f}' for i in (25, 36, 43, 44)]
        print(f'{name:9} {pairs[0]:20} {pairs[1]:17} {pairs[2]:18} {pairs[3]}')
    current = traces['b31']
    assert all(traces['b28'][i][3:5] == current[i][3:5] for i in range(16)), 'Soft curve changed'
    assert traces['b30'][25][4] <= 10 and current[25][4] >= 179
    assert traces['b30'][36][4] < 0 and current[36][4] > 0
    assert traces['b28'][43][4] < 0 and current[43][4] > 0
    assert all(row[3] > 0 and row[4] > 0 and row[7] for row in current[26:44])
    assert current[44][6] == 8 and current[44][3] > 0 and current[44][4] > 0
    print('PASS: soft tracking unchanged; observed corner keeps forward inner PWM and rolling reacquisition.')


if __name__ == '__main__':
    main()
