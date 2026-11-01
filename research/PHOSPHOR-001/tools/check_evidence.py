"""Offline finalization audit; requires the preserved local build for provenance hashes.

Does not launch Ghostty, manipulate windows, install software or run a workload.
"""
from pathlib import Path
from urllib.parse import unquote
from collections import Counter, defaultdict
import ast
import base64
import csv
import hashlib
import json
import math
import re
import statistics
import struct
import subprocess

repo = Path(__file__).resolve().parents[3]
research = repo / 'research/PHOSPHOR-001'
native = research / 'evidence/native'
measurements = native / 'measurements'

def load(path):
    return json.loads(path.read_text())

rows = list(csv.DictReader((measurements / 'samples.csv').open(newline='')))
assert len(rows) == 720
groups = defaultdict(list)
for row in rows:
    key = (row['surface'], int(row['rep']), row['mode'], row['workload'])
    groups[key].append(row)
    for keyname in ['elapsed_s', 'cpu_one_core_pct', 'rss_kib', 'gpu_pct', 'power_w', 'gpu_memory_mib']:
        assert math.isfinite(float(row[keyname])) and float(row[keyname]) >= 0, (keyname, row)
    assert float(row['elapsed_s']) > 0
    assert float(row['gpu_pct']) <= 100
expected = {(surface, rep, mode, work) for surface in ['1920x1080', '3840x2160'] for rep in [1, 2, 3] for mode in ['baseline', 'prototype'] for work in ['idle', 'scroll']}
assert set(groups) == expected
for key, group in groups.items():
    assert len(group) == 30 and [int(x['sample']) for x in group] == list(range(30)), key
progress = [json.loads(x) for x in (measurements / 'progress.jsonl').read_text().splitlines()]
assert Counter(x['event'] for x in progress) == {'start': 12, 'workload_done': 24, 'exit': 12}
assert 'COMPLETE: 12 paired native-window runs, 720 one-second samples' in (measurements / 'bench.log').read_text()
for entry in progress:
    if entry['event'] == 'exit':
        assert entry['returncode'] == 0
run_order = [f'{surface}-r{rep}-{mode}' for surface in ['1920x1080', '3840x2160'] for rep in [1, 2, 3] for mode in (['baseline', 'prototype'] if rep % 2 else ['prototype', 'baseline'])]
assert [x['run'] for x in progress if x['event'] == 'start'] == run_order
for run in run_order:
    folder = measurements / run
    geometry = load(folder / 'geometry.json')
    assert len(geometry) == 1
    window = geometry[0]
    width, height = map(int, run.split('-')[0].split('x'))
    for area in ['client', 'buffer']:
        assert (window[area]['width'], window[area]['height']) == (width, height), (run, area)
    assert window['resourceClass'] == 'org.phosphor.Spike001'
    log = (folder / 'ghostty.log').read_text()
    assert 'version=1.4.0' in log and 'EGL vendor=NVIDIA' in log and 'loaded OpenGL 4.3' in log, run
    assert ('loaded custom shader' in log) == run.endswith('prototype'), run
    assert 'font_size=12' in log and 'Noto Sans Mono Regular' in log
    assert not any(t in log.lower() for t in ['error loading custom', 'failed to import dmabuf', 'spirv error'])
    state = load(folder / 'final-state.json')
    assert state['rows_emitted'] > 50000 and state['mode'] == 'scroll'
    assert load(folder / 'control.json')['mode'] == 'exit'
print('PASS: 720 finite samples; 24 complete 30-sample groups; 12 successful ordered runs; native versions, mode logs and buffer geometry consistent.')

report = (research / 'REPORT.md').read_text()
summary = load(measurements / 'summary.json')
assert len(summary) == 8
for entry in summary:
    subset = [x for x in rows if (x['surface'], x['mode'], x['workload']) == (entry['client_buffer'], entry['mode'], entry['workload'])]
    assert len(subset) == entry['samples'] == 90
    cpu = statistics.mean(float(x['cpu_one_core_pct']) for x in subset)
    gpu = statistics.median(float(x['gpu_pct']) for x in subset)
    assert math.isclose(cpu, entry['mean_cpu_one_core_pct'], rel_tol=1e-12)
    assert gpu == entry['median_whole_gpu_pct']
    line = f"| {entry['client_buffer']} | {entry['mode']} | {entry['workload']} | 90 | {cpu:.2f}% | {gpu:.1f}% |"
    assert line in report, line
for name, expected in load(measurements / 'input-hashes.json').items():
    assert hashlib.sha256((measurements / name).read_bytes()).hexdigest() == expected
    assert hashlib.sha256((repo / 'prototypes/ghostty-molten-amber' / name).read_bytes()).hexdigest() == expected
print('PASS: all eight reported summary rows recompute from raw data; measured shader/config/launcher snapshots match retained prototype.')

from PIL import Image
pngs = list(native.rglob('*.png'))
for path in pngs:
    with Image.open(path) as img:
        assert img.format == 'PNG'
        img.verify()
for mode in ['baseline', 'prototype']:
    for label, size in [('1080', (1920, 1080)), ('4k-buffer', (3840, 2160))]:
        folder = native / 'screenshots'
        with Image.open(folder / f'{mode}-{label}-final.png') as img:
            assert img.size == (size[0] + 8, size[1] + 42)
            assert img.getpixel((1100, 1000))[:3] == (6, 5, 4)
        geometry = load(folder / f'{mode}-{label}-geometry.json')[0]
        for area in ['client', 'buffer']:
            assert (geometry[area]['width'], geometry[area]['height']) == size
for name, meta in load(native / 'session/metadata.json').items():
    path = native / 'session' / name
    assert path.stat().st_size == meta['bytes']
    assert hashlib.sha256(path.read_bytes()).hexdigest() == meta['sha256']
print(f'PASS: {len(pngs)} original PNG files decode; four final capture dimensions/geometry and dark backgrounds match; 26 archived session hashes match metadata.')

for path in repo.glob('*.png'):
    original = subprocess.check_output(['git', 'show', 'origin/main:' + path.name], cwd=repo)
    assert original == path.read_bytes(), path
for current, historical in [('Development_Plan.md', 'origin/main:Development_Plan.md'), ('research/PHOSPHOR-001/PHASE0_REPORT.md', '1dba303:research/PHOSPHOR-001/REPORT.md'), ('research/PHOSPHOR-001/evidence/validation.txt', '1dba303:research/PHOSPHOR-001/evidence/validation.txt')]:
    assert (repo / current).read_bytes() == subprocess.check_output(['git', 'show', historical], cwd=repo)
print('PASS: all approved artwork and development plan match accepted main; original Phase 0 report and validation record remain byte-identical.')

for path in research.rglob('*.json'):
    load(path)
for path in research.rglob('*.jsonl'):
    for line in path.read_text().splitlines():
        json.loads(line)
python_files = list((repo / 'prototypes').rglob('*.py')) + list(research.rglob('*.py'))
for path in python_files:
    ast.parse(path.read_text(), filename=str(path))
links = 0
for path in [repo / 'README.md', *research.rglob('*.md'), * (repo / 'prototypes').rglob('*.md')]:
    for target in re.findall(r'\]\(([^)]+)\)', path.read_text()):
        if target.startswith(('https://', 'http://', '#')):
            continue
        resolved = path.parent / unquote(target.split('#', 1)[0])
        assert resolved.exists(), (str(path), target)
        links += 1
assert not re.search(r'<!-- (PERFORMANCE_TABLE|INTERACTION_RESULTS) -->', report)
print(f'PASS: all JSON/JSONL parse; {len(python_files)} Python snapshots parse without executing native helpers; {links} local Markdown/image links resolve; no unfinished report placeholders.')

provenance = load(native / 'build-provenance.json')
root = Path('/tmp/phosphor-001-native')
source = root / 'downloads/ghostty-1.4.0.tar.gz'
zig = root / 'downloads/zig-0.17.0.tar.xz'
binary = root / 'install/bin/ghostty'
assert hashlib.sha256(source.read_bytes()).hexdigest() == provenance['source']['sha256']
assert hashlib.sha256(zig.read_bytes()).hexdigest() == provenance['zig']['shasum']
assert hashlib.sha256(binary.read_bytes()).hexdigest() == provenance['binary_sha256']
from nacl.signing import VerifyKey
pub = base64.b64decode(provenance['source']['public_key'])
lines = (native / 'ghostty-1.4.0.tar.gz.minisig').read_text().splitlines()
signature = base64.b64decode(lines[1])
assert signature[2:10] == pub[2:10] and signature[:2] == b'ED'
VerifyKey(pub[10:]).verify(hashlib.blake2b(source.read_bytes()).digest(), signature[10:])
VerifyKey(pub[10:]).verify(signature[10:] + lines[2].removeprefix('trusted comment: ').encode(), base64.b64decode(lines[3]))
count = 0
for line in (native / 'extracted-rpms.sha256').read_text().splitlines():
    expected, name = line.split('  ', 1)
    assert hashlib.sha256((root / 'rpms' / name).read_bytes()).hexdigest() == expected
    count += 1
assert hashlib.sha256((native / 'session/initial-user-template.ghostty').read_bytes()).hexdigest() == provenance['early_template_sha256']
print(f'PASS: existing source/compiler/binary and {count} RPM hashes match provenance; upstream source signature and trusted comment reverify offline; early template hash matches. No binary was launched or rebuilt.')
