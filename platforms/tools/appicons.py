#!/usr/bin/env python3
"""appicons.py -- the app icons the builds embed, from the 1024-pixel masters in resources/icons/.

    python3 platforms/tools/appicons.py      (needs Pillow; the outputs are committed)

For each game (k1, k2), resources/icons/<game>_icon_1024.png -> resources/icons/android/<game>/:
  mipmap-<density>/ic_launcher.png             the legacy square icon (48 dp)
  mipmap-<density>/ic_launcher_foreground.png  the adaptive icon's art: the master at 72 dp -- the
                                               part a launcher's mask shows -- centred on a
                                               transparent 108 dp layer, as Aspyr's icon was built
  mipmap-anydpi-v26/ic_launcher.xml            the adaptive icon: that over a black background
android/tools/k2apk.py compiles the directory into the APK (--res). iOS takes the master itself
(ios/tools/k2ipa.py, an asset catalog through actool). And for the desktop builds:
  win32/<game>.ico                             k2.exe's / k1.exe's icon resource (windows/k2p_icon.rc):
                                               16 to 256 px, BMP entries below 256 and a PNG one at
                                               256, as Explorer and the taskbar pick them
  window/<game>/window_icon.bmp                the macOS / Linux window icon (256 px, 24-bit), compiled
                                               in by embedfiles.py and set by shim_display.cpp
                                               (k2shim_window_icon): the Dock's on macOS, the window
                                               manager's on Linux. Windows' windows take the .exe's.

The masters are the macOS Steam builds' icons (KOTOR2.app AppIcon.icns, Knights of the Old
Republic.app App.icns, 1024 px) cropped inside their frames to full-bleed art; K1's is set at 90 %
on a blurred copy of itself so the logo clears a launcher's corner mask.
"""
import io
import os
import struct
import sys

from PIL import Image

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ICONS = os.path.join(REPO, 'resources', 'icons')
DENSITIES = {'mdpi': 1.0, 'hdpi': 1.5, 'xhdpi': 2.0, 'xxhdpi': 3.0, 'xxxhdpi': 4.0}
ADAPTIVE = '''<?xml version="1.0" encoding="utf-8"?>
<adaptive-icon xmlns:android="http://schemas.android.com/apk/res/android">
    <background android:drawable="@android:color/black"/>
    <foreground android:drawable="@mipmap/ic_launcher_foreground"/>
</adaptive-icon>
'''


ICO_SIZES = (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)


def write_ico(master, path):
    """A multi-resolution .ico: 32-bit DIB entries (BGRA rows bottom-up, then an all-zero AND mask)
    below 256 px, a PNG entry at 256."""
    images = []
    for n in ICO_SIZES:
        im = master.resize((n, n), Image.LANCZOS)
        if n >= 256:
            buf = io.BytesIO()
            im.save(buf, 'PNG', optimize=True)
            images.append((n, buf.getvalue()))
            continue
        px = im.tobytes('raw', 'BGRA')
        rows = [px[y * n * 4:(y + 1) * n * 4] for y in range(n)]
        mask_row = ((n + 31) // 32) * 4
        dib = struct.pack('<IiiHHIIiiII', 40, n, n * 2, 1, 32, 0, n * n * 4 + mask_row * n, 0, 0, 0, 0)
        images.append((n, dib + b''.join(reversed(rows)) + bytes(mask_row * n)))
    out = struct.pack('<HHH', 0, 1, len(images))
    offset = 6 + 16 * len(images)
    for n, data in images:
        out += struct.pack('<BBBBHHII', n % 256, n % 256, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    with open(path, 'wb') as f:
        f.write(out + b''.join(data for _, data in images))


def main():
    for game in ('k1', 'k2'):
        master = Image.open(os.path.join(ICONS, game + '_icon_1024.png')).convert('RGBA')
        out = os.path.join(ICONS, 'android', game)
        for density, scale in DENSITIES.items():
            d = os.path.join(out, 'mipmap-' + density)
            os.makedirs(d, exist_ok=True)
            legacy = round(48 * scale)
            master.resize((legacy, legacy), Image.LANCZOS).save(os.path.join(d, 'ic_launcher.png'))
            layer, art = round(108 * scale), round(72 * scale)
            fg = Image.new('RGBA', (layer, layer), (0, 0, 0, 0))
            fg.paste(master.resize((art, art), Image.LANCZOS), ((layer - art) // 2, (layer - art) // 2))
            fg.save(os.path.join(d, 'ic_launcher_foreground.png'))
        d = os.path.join(out, 'mipmap-anydpi-v26')
        os.makedirs(d, exist_ok=True)
        with open(os.path.join(d, 'ic_launcher.xml'), 'w', encoding='utf-8') as f:
            f.write(ADAPTIVE)
        print('appicons: ' + out, file=sys.stderr)
        d = os.path.join(ICONS, 'win32')
        os.makedirs(d, exist_ok=True)
        write_ico(master, os.path.join(d, game + '.ico'))
        d = os.path.join(ICONS, 'window', game)
        os.makedirs(d, exist_ok=True)
        master.convert('RGB').resize((256, 256), Image.LANCZOS).save(os.path.join(d, 'window_icon.bmp'))
        print('appicons: ' + os.path.join(ICONS, 'win32', game + '.ico') + ', ' + d, file=sys.stderr)


if __name__ == '__main__':
    main()
