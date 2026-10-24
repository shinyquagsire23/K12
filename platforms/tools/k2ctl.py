#!/usr/bin/env python3
"""k2ctl.py CMD... -- send actions to a running k2play.sh game; `snap=NAME[@N]` also takes a
screenshot N frames later, waits for it and converts it to /tmp/k2/NAME.png (prints the path).
  k2ctl.py keydown:w +150:keyup:w snap=walk@160     k2ctl.py click:617,269 snap=menu@60
"""
import os, sys, time
from PIL import Image
CTL = os.environ.get('K2_CTL', '/tmp/k2/ctl')
cmds, snap = [], None
for a in sys.argv[1:]:
    if a.startswith('snap='):
        snap, _, d = a[5:].partition('@')
        cmds.append((f'+{d}:' if d else '') + f'shot:/tmp/k2/{snap}.ppm')
    else:
        cmds.append(a)
# wait until the previous batch was consumed
for _ in range(600):
    if not os.path.exists(CTL):
        break
    time.sleep(0.05)
if snap and os.path.exists(f'/tmp/k2/{snap}.ppm'):
    os.remove(f'/tmp/k2/{snap}.ppm')
with open(CTL + '.tmp', 'w') as f:
    f.write('\n'.join(cmds) + '\n')
os.rename(CTL + '.tmp', CTL)
if snap:
    p = f'/tmp/k2/{snap}.ppm'
    for _ in range(1200):
        if os.path.exists(p) and os.path.getsize(p) > 100:
            time.sleep(0.2)
            im = Image.open(p)
            im.save(f'/tmp/k2/{snap}.png')
            print(f'/tmp/k2/{snap}.png', im.size)
            break
        time.sleep(0.1)
    else:
        print('no shot (game dead or paused?)')
