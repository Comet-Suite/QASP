#!/usr/bin/env python3
"""Check the real Windows portable tree and optionally stamp its exact source.

NOSA 1.3; see LICENSE. File/PE checks are not a Windows GUI runtime test.
"""
import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import struct
import zipfile

REQUIRED_PROGRAMS = ('vsp.exe', 'vspscript.exe', 'vspaero.exe',
                     'vspaero_opt.exe', 'vsploads.exe', 'vspviewer.exe')
REQUIRED_RUNTIME = ('concrt140.dll', 'msvcp140.dll', 'msvcp140_1.dll',
                    'msvcp140_2.dll', 'msvcp140_atomic_wait.dll',
                    'msvcp140_codecvt_ids.dll', 'vcomp140.dll',
                    'vcruntime140.dll', 'vcruntime140_1.dll')
REQUIRED_NOTICES = ('LICENSE', 'MODIFICATIONS.md', 'README.md', 'RISKS_AND_LIMITATIONS.md')


def pe_x64(data):
    if len(data) < 64 or data[:2] != b'MZ':
        return False
    offset = struct.unpack_from('<I', data, 60)[0]
    return offset + 6 <= len(data) and data[offset:offset+4] == b'PE\0\0' and \
        struct.unpack_from('<H', data, offset+4)[0] == 0x8664


def verify(package, source=None, stamp=False):
    if source and not re.fullmatch(r'[0-9a-f]{40}', source):
        raise ValueError('Expected a complete 40-character source commit, not a tag or short SHA')
    with zipfile.ZipFile(package) as archive:
        names = archive.namelist()
        if len(set(name.casefold() for name in names)) != len(names):
            raise ValueError('Case-insensitive duplicate archive paths')
        for name in names:
            path = PurePosixPath(name)
            if path.is_absolute() or '..' in path.parts or '\\' in name:
                raise ValueError('Unsafe archive path')
        programs = [name for name in names if PurePosixPath(name).name == 'vsp.exe']
        if len(programs) != 1:
            raise ValueError('Expected exactly one modeling application named vsp.exe')
        root = str(PurePosixPath(programs[0]).parent)
        root = '' if root == '.' else root + '/'
        for name in REQUIRED_PROGRAMS + REQUIRED_RUNTIME + REQUIRED_NOTICES:
            if root + name not in names:
                raise ValueError('Portable package is missing ' + name)
        for name in REQUIRED_PROGRAMS + REQUIRED_RUNTIME:
            if not pe_x64(archive.read(root + name)):
                raise ValueError('Not a Windows x64 PE executable/library: ' + name)
        if archive.testzip() is not None:
            raise ValueError('ZIP integrity check failed')
        info_name = root + 'QASP-BUILD-INFO.json'
        info = {
            'product': 'QASP', 'packaging': 'Windows x64 portable folder; not an installer',
            'repository': 'https://github.com/Comet-Suite/QASP',
            'source_commit': source,
            'source_url': 'https://github.com/Comet-Suite/QASP/tree/' + source if source else None,
            'validation': 'Package contents, CRC and Windows x64 PE checks; not a GUI runtime test',
            'license': 'NASA Open Source Agreement 1.3; see LICENSE',
            'risks': 'See RISKS_AND_LIMITATIONS.md; this is experimental software',
        }
        if info_name in names:
            actual = json.loads(archive.read(info_name))
            if source and actual.get('source_commit') != source:
                raise ValueError('Build stamp does not match requested source revision')
            info = actual
    if stamp:
        if not source:
            raise ValueError('Cannot stamp a package without its exact source commit')
        if info_name not in names:
            with zipfile.ZipFile(package, 'a', compression=zipfile.ZIP_DEFLATED) as archive:
                archive.writestr(info_name, json.dumps(info, indent=2)+'\n')
    elif source and info_name not in names:
        raise ValueError('Package has no source build stamp')
    return {'package': str(package), 'root': root, 'entries': len(names),
            'sha256': hashlib.sha256(Path(package).read_bytes()).hexdigest(),
            'source_commit': info.get('source_commit')}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package', type=Path)
    parser.add_argument('--source')
    parser.add_argument('--stamp', action='store_true')
    args = parser.parse_args()
    print(json.dumps(verify(args.package, args.source, args.stamp), indent=2))


if __name__ == '__main__':
    main()
