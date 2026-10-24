#!/usr/bin/env python3
"""device.py — install/run the hybrid APK on a device or emulator (arm64 only).

  device.py install --apk out/kotor2-hybrid.apk [--package P]   adb install + expansion files
  device.py run     [--package P] [--activity A] [--extra K=V]  force-stop, launch, follow logcat
  device.py log     [--package P]                               logcat for the app's pid

The expansion files are looked up in decomp_bins/swkotorii_obb/ (--obb-src; KOTOR 1: swkotor_obb with
--orig-package com.aspyr.swkotor) and land under the NEW package's
obb dir with the package name in the file name rewritten (the downloader looks for
main.<ver>.<package>.obb). If the original game is installed on the device with its OBBs, they are
copied device-side instead of pushing ~3.7 GB again.
"""
import argparse, os, re, subprocess, sys, time

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, '..', '..'))
ORIG_PKG = 'com.aspyr.swkotorii'
ACTIVITY = 'com.aspyr.kotor.StartupActivity'


def adb(*args, check=True, capture=False):
    cmd = ['adb'] + list(args)
    if capture:
        r = subprocess.run(cmd, capture_output=True, text=True)
        if check and r.returncode:
            sys.exit('adb %s failed: %s' % (' '.join(args), r.stderr.strip()))
        return r.stdout.strip()
    r = subprocess.run(cmd)
    if check and r.returncode:
        sys.exit('adb %s failed' % ' '.join(args))
    return r.returncode


def remote_size(path):
    out = adb('shell', 'stat -c %s ' + path + ' 2>/dev/null || echo -1', capture=True, check=False)
    try:
        return int(out.splitlines()[-1])
    except (ValueError, IndexError):
        return -1


def install(a):
    abi = adb('shell', 'getprop ro.product.cpu.abilist', capture=True)
    if 'arm64-v8a' not in abi:
        sys.exit('device ABIs are %r; the hybrid APK is arm64-v8a only' % abi)
    adb('install', '-r', '-d', a.apk)
    obb_src = a.obb_src or os.path.join(REPO, 'decomp_bins', 'swkotorii_obb')
    orig_pkg = a.orig_package
    dst_dir = '/sdcard/Android/obb/%s' % a.package
    adb('shell', 'mkdir -p ' + dst_dir)
    if not os.path.isdir(obb_src):
        print('obb: no %s; the device keeps the expansion files it has' % obb_src)
        return
    for fn in sorted(os.listdir(obb_src)):
        m = re.fullmatch(r'(main|patch)\.(\d+)\.%s\.obb' % re.escape(orig_pkg), fn)
        if not m:
            continue
        local = os.path.join(obb_src, fn)
        size = os.path.getsize(local)
        dst = '%s/%s.%s.%s.obb' % (dst_dir, m.group(1), m.group(2), a.package)
        if remote_size(dst) == size:
            print('obb: %s already present' % dst)
            continue
        orig_remote = '/sdcard/Android/obb/%s/%s' % (orig_pkg, fn)
        if remote_size(orig_remote) == size:
            print('obb: copying %s from the installed original' % fn)
            adb('shell', 'cp %s %s' % (orig_remote, dst))
        else:
            print('obb: pushing %s (%d MB)' % (fn, size >> 20))
            adb('push', local, dst)
        if remote_size(dst) != size:
            sys.exit('obb: %s has the wrong size after transfer' % dst)


def pid_of(pkg):
    for _ in range(50):
        p = adb('shell', 'pidof ' + pkg, capture=True, check=False)
        if p:
            return p.split()[0]
        time.sleep(0.2)
    return None


def log(a):
    pid = pid_of(a.package)
    if not pid:
        sys.exit('%s is not running' % a.package)
    os.execvp('adb', ['adb', 'logcat', '--pid', pid, '-v', 'time'])


def run(a):
    adb('shell', 'am force-stop ' + a.package, check=False)
    adb('logcat', '-c', check=False)
    extras = ''.join(" --es %s '%s'" % tuple(e.split('=', 1)) for e in a.extra)
    adb('shell', 'am start -n %s/%s%s' % (a.package, a.activity, extras))
    log(a)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('cmd', choices=['install', 'run', 'log'])
    ap.add_argument('--apk')
    ap.add_argument('--package', default=ORIG_PKG + '.hybrid')
    ap.add_argument('--orig-package', default=ORIG_PKG,
                    help='the original game, whose OBB names (and installed OBBs) are the source')
    ap.add_argument('--obb-src', default='', help='local OBB dir (default decomp_bins/swkotorii_obb)')
    ap.add_argument('--activity', default=ACTIVITY, help='the launch activity (platforms/android: k2.K2Activity)')
    ap.add_argument('--extra', action='append', default=[], help='KEY=VALUE string extra for am start')
    a = ap.parse_args()
    if a.cmd == 'install' and not a.apk:
        sys.exit('install needs --apk')
    {'install': install, 'run': run, 'log': log}[a.cmd](a)


if __name__ == '__main__':
    main()
