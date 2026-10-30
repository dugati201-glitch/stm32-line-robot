"""Audit B28 vs B34 position on CRC-validated raw captures; not vehicle dynamics."""
import argparse
import csv
import importlib.util
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('blackbox_reader', ROOT / 'scripts/read-blackbox.py')
reader = importlib.util.module_from_spec(spec)
spec.loader.exec_module(reader)


def estimate(strength):
    mask = sum(1 << i for i, value in enumerate(strength) if value >= 500)
    groups = sum(bool(mask & (1 << i)) and (i == 0 or not mask & (1 << (i-1))) for i in range(8))
    if not mask or mask == 255 or groups != 1:
        return mask, None, None
    first = next(i for i in range(8) if mask & (1 << i))
    last = next(i for i in range(7, -1, -1) if mask & (1 << i))
    while first and strength[first-1]:
        first -= 1
    while last < 7 and strength[last+1]:
        last += 1
    old = sum(i*1000*value for i, value in enumerate(strength)) // sum(strength) - 3500
    local = sum(i*1000*strength[i] for i in range(first, last+1)) // sum(strength[first:last+1]) - 3500
    return mask, old, local


def audit(path, output):
    text = path.read_text(encoding='utf-8-sig')
    log = reader.parse_log(text)  # Reject missing/altered rows, CRC or schema.
    metadata = {}
    for line in text.splitlines():
        if line.startswith(('# calib_low,', '# calib_high,', '# calib_white,')):
            key, *values = line.split(',')
            metadata[key] = list(map(int, values))
    low, high, white = (metadata[key] for key in ['# calib_low', '# calib_high', '# calib_white'])
    assert all(len(values) == 8 for values in (low, high, white))
    polarity, direction = log['profile']['polarity'], log['profile']['dir']
    assert direction in (-1, 1)
    rows = []
    for row in log['rows']:
        strength = []
        for i in range(8):
            value = max(low[i], min(high[i], row[f'raw{i}']))
            if polarity & (1 << i):
                level = 0 if value <= white[i] else (value-white[i])*1000 // (high[i]-white[i])
            else:
                level = 0 if value >= white[i] else (white[i]-value)*1000 // (white[i]-low[i])
            strength.append(level if level >= 150 else 0)
        mask, old, local = estimate(strength)
        canonical = mask if direction > 0 else sum(((mask >> i) & 1) << (7-i) for i in range(8))
        assert canonical == row['mask'] and (old is not None) == bool(row['valid'])
        if old is None:
            continue  # Invalid error is held history, never reconstructed as fresh position.
        old, local = old*direction, local*direction
        reference = local if log['firmware'] == 'B34' else old
        assert reference == row['error'], f"Estimator/metadata mismatch at {row['ms']}"
        rows.append([row['ms'], canonical, row['error'], old, local, local-old])
    output.mkdir(parents=True, exist_ok=True)
    destination = output / (path.stem + '-centroid.csv')
    with destination.open('w', newline='', encoding='utf-8') as stream:
        writer = csv.writer(stream)
        writer.writerow(['ms', 'mask', 'recorded_error', 'b28_error', 'b34_error', 'delta'])
        writer.writerows(rows)
    changed = [row for row in rows if row[-1]]
    print(f'{path.name}: CRC {log["crc32"]}, valid={len(rows)}, changed={len(changed)}')
    for row in changed:
        print(f'  ms={row[0]} mask={row[1]:02X}: {row[3]:+d} -> {row[4]:+d}')
    print(f'  Audit: {destination}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('logs', nargs='+', type=Path)
    parser.add_argument('--output', type=Path, default=ROOT / 'build/centroid-audit')
    args = parser.parse_args()
    print('Position recalculation only: disconnected weak signal may be reflection OR a real approaching branch.')
    print('No ground-truth line position, vehicle speed or lap prediction.')
    for path in args.logs:
        audit(path, args.output)


if __name__ == '__main__':
    main()
