#!/usr/bin/env python3
"""Capture the actual compiled FLTK guide component on an X11 display.

NASA NOSA 1.3; see LICENSE. Needs Pillow (with XCB), xdotool and an X11 display.
Example:
  xvfb-run -a -s '-screen 0 1440x1000x24' python3 tools/capture_qasp_field_guide.py \
    /path/to/field-guide --lesson thrust --output field-guide-thrust.png

This is a screenshot of the real help component in a focused Linux test harness,
NOT a full QASP geometry-engine screenshot and NOT a Windows runtime test.
"""
import argparse
import os
from pathlib import Path
import subprocess
import time
from PIL import ImageGrab


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('binary', type=Path)
    parser.add_argument('--lesson', default='thrust')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--size', nargs=2, type=int, metavar=('WIDTH', 'HEIGHT'))
    args = parser.parse_args()
    if not os.environ.get('DISPLAY'):
        parser.error('Use an X11 display or run this command under xvfb-run.')
    process = subprocess.Popen([str(args.binary.resolve()), '--lesson', args.lesson],
                               stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    try:
        result = subprocess.run(['xdotool', 'search', '--sync', '--onlyvisible', '--name',
                                 '^QASP Field Guide$'], capture_output=True, text=True,
                                timeout=15, check=True)
        wid = result.stdout.splitlines()[0]
        if args.size:
            subprocess.run(['xdotool', 'windowsize', '--sync', wid,
                            str(args.size[0]), str(args.size[1])], check=True, timeout=10)
        # One initial expose/resize needs to reach the native application's event
        # loop before reading pixels. This is not a synthesized UI illustration.
        time.sleep(0.25)
        geometry = subprocess.run(['xdotool', 'getwindowgeometry', '--shell', wid],
                                  capture_output=True, text=True, check=True).stdout
        values = dict(line.split('=', 1) for line in geometry.splitlines())
        x, y, w, h = (int(values[k]) for k in ('X', 'Y', 'WIDTH', 'HEIGHT'))
        image = ImageGrab.grab(bbox=(x, y, x + w, y + h), xdisplay=os.environ['DISPLAY'])
        if not image.getbbox():
            raise RuntimeError('Screenshot is blank; do not publish it as visual QA.')
        args.output.parent.mkdir(parents=True, exist_ok=True)
        image.save(args.output)
        print(f'Captured actual guide component: {args.output} ({w} x {h})')
    finally:
        process.terminate()
        stdout, stderr = process.communicate(timeout=5)
        if stderr:
            print(stderr.decode(errors='replace'))


if __name__ == '__main__':
    main()
