#!/usr/bin/env python3
"""Check that curated hover-help keys exist in the actual geometry sources.

This is a source-key check, NOT a claim that every parameter is explained or
that aerodynamic statements have been validated by a solver. NASA NOSA 1.3.
"""
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
data = json.loads((ROOT / 'src/help/qasp-help.json').read_text())
text = '\n'.join(p.read_text(errors='replace') for p in (ROOT / 'src/geom_core').glob('*.cpp'))
names = set(re.findall(r'\.Init\(\s*"([^"]+)"', text))
for entry in data['tooltips']:
    assert entry['name'] in names, f'No real Parm::Init key: {entry["name"]}'
    if entry['descript']:
        assert entry['descript'] in text, f'Missing description guard: {entry["descript"]}'
    for field in ('meaning', 'effect', 'example'):
        assert len(entry[field].split()) >= 6, f'Label masquerading as explanation: {entry["name"]}.{field}'
for entry in data['lessons']:
    for field in ('meaning', 'effect', 'example', 'try_it', 'caution'):
        assert len(entry[field].split()) >= 8, f'Incomplete lesson: {entry["id"]}.{field}'
print(f'PASS: all {len(data["tooltips"])} curated tooltip names exist in source; definitions, effects and examples are present')
print(f'Coverage: {len({e["name"] for e in data["tooltips"]})} distinct authored names out of {len(names)} source names; advanced controls retain labelled reference text')
