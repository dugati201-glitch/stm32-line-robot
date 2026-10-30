"""Compare archived B28/B29 against B30 PWM commands, without vehicle dynamics."""
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
    work = ROOT / 'build/corner-b30-comparison'
    work.mkdir(parents=True, exist_ok=True)
    sources = {'b30': ROOT / 'app/control.c'}
    references = [('b28', 'B28-before-B29-20261029/B28-source.zip'),
                  ('b29', 'B29-before-B30-20261029/B29-source.zip')]
    historical = 'B30-before-B31-20261029/B30-source.zip'
    if (ROOT / 'backups' / historical).exists():
        references.append(('b30', historical))
    for name, backup in references:
        saved = work / name
        saved.mkdir(parents=True, exist_ok=True)
        with zipfile.ZipFile(ROOT / 'backups' / backup) as archive:
            for filename in ['control.c', 'control.h', 'control_tuning.h']:
                (saved / filename).write_bytes(archive.read('app/' + filename))
        sources[name] = saved / 'control.c'
    compiler = Path(r'C:\Keil_v5\ARM\ARMCC\bin')
    traces = {}
    for name in ['b28', 'b29', 'b30']:
        source = sources[name]
        control, entry, axf = [work / (name + ext) for ext in ('.o', '-entry.o', '.axf')]
        common = ['--cpu', 'Cortex-M3', '--c99', '-O1', '-g', '-I', str(source.parent)]
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(source), '-o', str(control)], check=True)
        subprocess.run([str(compiler / 'armcc.exe'), *common, '-c', str(ROOT / 'tests/test_exit_trace.c'),
                        '-o', str(entry)], check=True)
        subprocess.run([str(compiler / 'armlink.exe'), '--cpu', 'Cortex-M3', '--entry', 'test_entry',
                        '--ro-base', '0x08000000', '--rw-base', '0x20000000', '--libpath',
                        str(compiler.parent / 'lib'), '--no_remove', '--output', str(axf),
                        str(control), str(entry)], check=True)
        rows = traces[name] = helper.emulate(axf)
        with (work / (name + '.csv')).open('w', newline='', encoding='utf-8') as stream:
            writer = csv.writer(stream)
            writer.writerow(['ms', 'error_x1000', 'correction_x1000', 'left_x10', 'right_x10',
                             'target_x1000', 'mode', 'extra_P_gain_x1000'])
            writer.writerows(rows)
    print('PWM commands only: no physical speed, trajectory, stopping distance or lap prediction.')
    print('candidate   first visible cue   cue+6ms   steady   extra gain at return+6ms')
    for name, rows in traces.items():
        print(f'{name:9} {rows[16][2]/1000:9.3f}%  {rows[18][2]/1000:9.3f}%'
              f' {rows[31][2]/1000:8.3f}%  {rows[34][7]/1000:9.3f}')
    before, current = traces['b29'], traces['b30']
    assert all(before[i][2] == current[i][2] for i in range(16)), 'Soft curve changed'
    assert current[16][2] > before[16][2], 'No earlier steering at first cue'
    assert current[31][2] == before[31][2], 'Steady gain ceiling changed'
    assert current[34][7] > before[34][7], 'Early release still active'
    assert all(row[5] == 45000 for row in current), 'Base target changed'
    print('PASS: earlier bounded steering, same ceiling/soft curve/Base45; B29 early release removed.')


if __name__ == '__main__':
    main()
