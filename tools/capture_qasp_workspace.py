#!/usr/bin/env python3
"""Capture the real QASP application on X11, not a mockup or component harness.

NOSA 1.3; see LICENSE. Requires Pillow with XCB, xdotool and Xvfb/X11.
The capture is Linux-native evidence, not a Windows runtime/portable-package test.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
from PIL import ImageGrab


def command(*args, timeout=20):
    return subprocess.run(list(args), check=True, capture_output=True,
                          text=True, timeout=timeout).stdout.strip()


def capture(window, destination):
    values = dict(line.split('=', 1) for line in
                  command('xdotool', 'getwindowgeometry', '--shell', window).splitlines())
    x, y, w, h = (int(values[k]) for k in ('X', 'Y', 'WIDTH', 'HEIGHT'))
    image = ImageGrab.grab(bbox=(x, y, x+w, y+h), xdisplay=os.environ['DISPLAY'])
    if not image.getbbox() or len(image.getcolors(256) or []) == 1:
        raise RuntimeError('Blank screenshot; do not publish as application evidence.')
    image.save(destination)
    return {'path': str(destination), 'size': [w, h],
            'sha256': hashlib.sha256(destination.read_bytes()).hexdigest()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('model', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--two-tabs', action='store_true')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    log_path = args.output / 'application-capture.log'
    env = dict(os.environ, LIBGL_ALWAYS_SOFTWARE='1', LC_ALL='C.UTF-8', LANG='C.UTF-8')
    with log_path.open('w') as log:
        process = subprocess.Popen([str(args.binary.resolve()), str(args.model.resolve())],
                                   stdout=log, stderr=subprocess.STDOUT, env=env)
        try:
            windows = command('xdotool', 'search', '--sync', '--onlyvisible',
                              '--pid', str(process.pid), '--name', 'QASP', timeout=30)
            window = windows.splitlines()[0]
            command('xdotool', 'windowfocus', '--sync', window)
            time.sleep(2)
            # Select the actual isometric and fit toolbar controls.
            command('xdotool', 'mousemove', '--window', window, '298', '75', 'click', '1')
            command('xdotool', 'mousemove', '--window', window, '190', '75', 'click', '1')
            time.sleep(1)
            shots = [capture(window, args.output / 'workspace-canvas-linux.png')]
            if args.two_tabs:
                command('xdotool', 'key', '--clearmodifiers', 'ctrl+n')
                time.sleep(.5)
                # Continue is the Return button in the actual reload warning.
                command('xdotool', 'key', '--clearmodifiers', 'Return')
                time.sleep(1)
                command('xdotool', 'windowfocus', '--sync', window)
                command('xdotool', 'key', '--clearmodifiers', 'ctrl+shift+Tab')
                time.sleep(2)
            # Open the actual hosted component tree using its left activity icon.
            command('xdotool', 'mousemove', '--window', window, '24', '98', 'click', '1')
            command('xdotool', 'mousemove', '--window', window, '190', '75', 'click', '1')
            time.sleep(1)
            shots.append(capture(window, args.output / 'workspace-components-linux.png'))
            result = {'kind': 'Unmodified pixels from the running full Linux QASP application',
                      'not': 'Not a mockup, standalone component harness, or Windows runtime test',
                      'binary': str(args.binary.resolve()),
                      'model': str(args.model.resolve()),
                      'source_commit': command('git', 'rev-parse', 'HEAD'),
                      'window_title': command('xdotool', 'getwindowname', window),
                      'screenshots': shots}
            (args.output / 'capture-provenance.json').write_text(json.dumps(result, indent=2)+'\n')
            print(json.dumps(result, indent=2), flush=True)
        finally:
            process.terminate()
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.kill()
                process.wait()


if __name__ == '__main__':
    main()
