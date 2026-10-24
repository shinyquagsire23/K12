#!/usr/bin/env python3
"""k2ipa.py -- finish, sign, install and package the iOS app (platforms/ios/CMakeLists.txt).

  sign     copy --asset files into the bundle, sign Frameworks/*.dylib, then the app. --identity -
           (the default) is an ad-hoc signature, which is what the Simulator runs; a device needs a
           developer identity and --profile <file.mobileprovision>, whose entitlements are signed in
           (and the profile embedded as embedded.mobileprovision).
  run-sim  install the app on the booted Simulator (booting one if none is), put the OBBs and a
           k2env.txt in its Documents, and launch it with the console attached (stdout/stderr here).
           The OBBs are APFS clones (cp -c), so a copy costs no space.
  run-device  the same on a connected iPhone / iPad (Xcode's devicectl, USB or the network): re-sign
           the app with a development identity and a provisioning profile that covers the device
           (picked from Xcode's profiles unless given), install it, copy the OBBs and k2env.txt into
           its Documents (devicectl skips files that have not changed) and launch it with the console
           attached.
  ipa      Payload/<app> zipped, for installing on a device (Apple Configurator, Xcode, ...).
"""
import argparse, datetime, glob, hashlib, json, os, plistlib, shutil, subprocess, sys, tempfile, zipfile


def run(cmd, **kw):
    print('k2ipa: ' + ' '.join(cmd), file=sys.stderr)
    return subprocess.run(cmd, check=True, **kw)


def read_profile(path):
    raw = subprocess.run(['security', 'cms', '-D', '-i', path], check=True, capture_output=True).stdout
    return plistlib.loads(raw)


def bundle_id_of(app):
    with open(os.path.join(app, 'Info.plist'), 'rb') as f:
        return plistlib.load(f)['CFBundleIdentifier']


def compile_icon(app, png, platform, min_os):
    """The app icon: a single-size (1024 px) asset catalog through actool, which writes Assets.car
    and the CFBundleIcons keys merged into the bundle's Info.plist."""
    tmp = tempfile.mkdtemp()
    cat = os.path.join(tmp, 'Assets.xcassets')
    iconset = os.path.join(cat, 'AppIcon.appiconset')
    os.makedirs(iconset)
    with open(os.path.join(cat, 'Contents.json'), 'w') as f:
        json.dump({'info': {'author': 'xcode', 'version': 1}}, f)
    shutil.copy2(png, os.path.join(iconset, 'icon.png'))
    with open(os.path.join(iconset, 'Contents.json'), 'w') as f:
        json.dump({'images': [{'filename': 'icon.png', 'idiom': 'universal', 'platform': 'ios',
                               'size': '1024x1024'}], 'info': {'author': 'xcode', 'version': 1}}, f)
    partial = os.path.join(tmp, 'partial.plist')
    run(['xcrun', 'actool', '--compile', app, '--platform', platform, '--minimum-deployment-target',
         min_os, '--app-icon', 'AppIcon', '--output-partial-info-plist', partial, '--target-device',
         'iphone', '--target-device', 'ipad', '--output-format', 'human-readable-text', cat],
        stdout=subprocess.DEVNULL)
    info = os.path.join(app, 'Info.plist')
    with open(info, 'rb') as f:
        plist = plistlib.load(f)
    with open(partial, 'rb') as f:
        plist.update(plistlib.load(f))
    with open(info, 'wb') as f:
        plistlib.dump(plist, f)
    shutil.rmtree(tmp)


def sign(a):
    app = a.app
    for f in a.asset or []:
        shutil.copy2(f, os.path.join(app, os.path.basename(f)))
    if getattr(a, 'icon', None):
        compile_icon(app, a.icon, a.platform, a.min_os)
    ents = None
    if a.profile:
        shutil.copy2(a.profile, os.path.join(app, 'embedded.mobileprovision'))
        ent = read_profile(a.profile)['Entitlements']
        # a wildcard profile (TEAM.*) signs the app as TEAM.<its bundle id>
        appid = ent.get('application-identifier', '')
        if appid.endswith('*'):
            ent['application-identifier'] = appid[:-1] + bundle_id_of(app)
        fd, ents = tempfile.mkstemp(suffix='.plist')
        with os.fdopen(fd, 'wb') as f:
            plistlib.dump(ent, f)
    fw = os.path.join(app, 'Frameworks')
    for n in sorted(os.listdir(fw)) if os.path.isdir(fw) else []:
        run(['codesign', '--force', '--sign', a.identity, '--timestamp=none', os.path.join(fw, n)])
    cmd = ['codesign', '--force', '--sign', a.identity, '--timestamp=none']
    if ents:
        cmd += ['--entitlements', ents]
    run(cmd + [app])
    if ents:
        os.unlink(ents)


def booted_device():
    out = subprocess.run(['xcrun', 'simctl', 'list', 'devices', '-j'], check=True,
                         capture_output=True, text=True).stdout
    devs = json.loads(out)['devices']
    for rt, ds in devs.items():
        for d in ds:
            if d.get('state') == 'Booted' and 'iOS' in rt:
                return d['udid']
    for rt, ds in sorted(devs.items(), reverse=True):
        if 'iOS' not in rt:
            continue
        for d in ds:
            if d.get('isAvailable') and ('iPhone' in d['name'] or 'iPad' in d['name']):
                run(['xcrun', 'simctl', 'boot', d['udid']])
                dev = subprocess.run(['xcode-select', '-p'], capture_output=True, text=True).stdout.strip()
                subprocess.run(['open', os.path.join(dev, 'Applications/Simulator.app')])
                return d['udid']
    sys.exit('k2ipa: no iOS Simulator device (Xcode > Settings > Platforms)')


def run_sim(a):
    udid = a.device or booted_device()
    run(['xcrun', 'simctl', 'install', udid, a.app])
    data = subprocess.run(['xcrun', 'simctl', 'get_app_container', udid, a.bundle_id, 'data'],
                          check=True, capture_output=True, text=True).stdout.strip()
    docs = os.path.join(data, 'Documents')
    os.makedirs(docs, exist_ok=True)
    if a.obb_dir and os.path.isdir(a.obb_dir):
        for n in sorted(os.listdir(a.obb_dir)):
            dst = os.path.join(docs, n)
            if n.endswith('.obb') and not os.path.exists(dst):
                run(['cp', '-c', os.path.join(a.obb_dir, n), dst])
    if a.env is not None:
        with open(os.path.join(docs, 'k2env.txt'), 'w') as f:
            for kv in a.env.split(';'):
                if kv.strip():
                    f.write(kv.strip() + '\n')
    print('k2ipa: Documents = ' + docs, file=sys.stderr)
    subprocess.run(['xcrun', 'simctl', 'terminate', udid, a.bundle_id], stderr=subprocess.DEVNULL)
    os.execvp('xcrun', ['xcrun', 'simctl', 'launch', '--console-pty', '--terminate-running-process',
                        udid, a.bundle_id])


def devicectl_json(args):
    fd, out = tempfile.mkstemp(suffix='.json')
    os.close(fd)
    try:
        subprocess.run(['xcrun', 'devicectl'] + args + ['--json-output', out, '-q'], check=True,
                       stdout=subprocess.DEVNULL)
        with open(out) as f:
            return json.load(f)
    finally:
        os.unlink(out)


def pick_device(want):
    """-> (devicectl identifier, UDID, name) of the wanted (UDID / name) or first connected iPhone/iPad."""
    devs = devicectl_json(['list', 'devices'])['result']['devices']
    found = []
    for d in devs:
        hw, cp = d.get('hardwareProperties', {}), d.get('connectionProperties', {})
        name = d.get('deviceProperties', {}).get('name', '?')
        if hw.get('reality') != 'physical' or hw.get('platform') not in ('iOS', 'iPadOS'):
            continue
        if want and want not in (hw.get('udid'), d.get('identifier'), name):
            continue
        if cp.get('pairingState') != 'paired':
            continue
        found.append((d['identifier'], hw.get('udid'), name, cp.get('tunnelState') == 'connected'))
    if not found:
        sys.exit('k2ipa: no paired iPhone / iPad%s (Xcode > Window > Devices and Simulators)'
                 % (' named ' + want if want else ''))
    found.sort(key=lambda d: not d[3])   # one already connected first
    return found[0][:3]


def identities():
    """SHA-1 (upper hex) -> name of each valid code-signing identity in the keychain."""
    out = subprocess.run(['security', 'find-identity', '-v', '-p', 'codesigning'], check=True,
                         capture_output=True, text=True).stdout
    ids = {}
    for line in out.splitlines():
        f = line.split(None, 2)
        if len(f) == 3 and len(f[1]) == 40:
            ids[f[1].upper()] = f[2].strip().strip('"')
    return ids


def pick_profile(bundle_id, udid, profile=''):
    """-> (profile path, identity SHA-1): a development profile that covers the device and the bundle
    id (an exact app id before a wildcard, then the latest expiry), and a keychain identity it lists."""
    ids = identities()
    dirs = [os.path.expanduser('~/Library/Developer/Xcode/UserData/Provisioning Profiles'),
            os.path.expanduser('~/Library/MobileDevice/Provisioning Profiles')]
    paths = [profile] if profile else [p for d in dirs for p in glob.glob(os.path.join(d, '*.mobileprovision'))]
    now = datetime.datetime.now(datetime.timezone.utc)
    best = None
    for path in paths:
        try:
            pp = read_profile(path)
        except subprocess.CalledProcessError:
            continue
        ent = pp.get('Entitlements', {})
        appid = ent.get('application-identifier', '')
        team = (pp.get('TeamIdentifier') or [''])[0]
        rest = appid[len(team) + 1:] if appid.startswith(team + '.') else appid
        exact = rest == bundle_id
        if not (exact or (rest.endswith('*') and bundle_id.startswith(rest[:-1]))):
            continue
        exp = pp.get('ExpirationDate')
        if exp is not None and exp.replace(tzinfo=datetime.timezone.utc) < now:
            continue
        if not ent.get('get-task-allow') or not (pp.get('ProvisionsAllDevices') or udid in pp.get('ProvisionedDevices', [])):
            continue
        certs = [hashlib.sha1(c).hexdigest().upper() for c in pp.get('DeveloperCertificates', [])]
        ident = next((c for c in certs if c in ids), None)
        if not ident:
            continue
        key = (exact, exp or now)
        if best is None or key > best[0]:
            best = (key, path, ident, pp.get('Name', '?'))
    if not best:
        sys.exit('k2ipa: no development provisioning profile covers %s on device %s with an identity in\n'
                 '  the keychain. In Xcode: Settings > Accounts (sign in), then build any app for the device\n'
                 '  once so Xcode makes a team profile, or pass --profile / --bundle-id.' % (bundle_id, udid))
    print('k2ipa: signing as %s with "%s"' % (ids[best[2]], best[3]), file=sys.stderr)
    return best[1], best[2]


def run_device(a):
    dev, udid, name = pick_device(a.device)
    print('k2ipa: device %s (%s)' % (name, udid), file=sys.stderr)
    bundle_id = bundle_id_of(a.app)
    profile, ident = pick_profile(bundle_id, udid, a.profile)
    sign(argparse.Namespace(app=a.app, identity=ident, profile=profile, asset=None, icon=''))
    run(['xcrun', 'devicectl', 'device', 'install', 'app', '--device', dev, a.app])
    copies = []
    if a.obb_dir and os.path.isdir(a.obb_dir):
        copies += [os.path.join(a.obb_dir, n) for n in sorted(os.listdir(a.obb_dir)) if n.endswith('.obb')]
    tmp = tempfile.mkdtemp()
    if a.env is not None:
        env = os.path.join(tmp, 'k2env.txt')
        with open(env, 'w') as f:
            for kv in a.env.split(';'):
                if kv.strip():
                    f.write(kv.strip() + '\n')
        copies.append(env)
    for src in copies:   # unchanged files are skipped, so the OBBs only cross once
        run(['xcrun', 'devicectl', 'device', 'copy', 'to', '--device', dev, '--domain-type',
             'appDataContainer', '--domain-identifier', bundle_id, '--source', src,
             '--destination', 'Documents/' + os.path.basename(src)])
    shutil.rmtree(tmp)
    if a.no_launch:
        return
    cmd = ['xcrun', 'devicectl', 'device', 'process', 'launch', '--device', dev,
           '--terminate-existing'] + ([] if a.no_console else ['--console']) + [bundle_id]
    print('k2ipa: ' + ' '.join(cmd), file=sys.stderr)
    p = subprocess.run(cmd, stderr=subprocess.PIPE, text=True)
    if p.returncode != 0:
        sys.stderr.write(p.stderr)
        if 'Locked' in p.stderr or 'unlocked' in p.stderr:
            sys.exit('k2ipa: %s is locked: unlock it and tap the app, or run again with --no-build '
                     '(the OBBs are already there)' % name)
        sys.exit(p.returncode)


def ipa(a):
    name = os.path.basename(a.app.rstrip('/'))
    with zipfile.ZipFile(a.out, 'w', zipfile.ZIP_DEFLATED) as z:
        for root, dirs, files in os.walk(a.app):
            for f in files:
                p = os.path.join(root, f)
                z.write(p, os.path.join('Payload', name, os.path.relpath(p, a.app)))
    print('k2ipa: wrote ' + a.out, file=sys.stderr)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = ap.add_subparsers(dest='cmd', required=True)
    s = sub.add_parser('sign')
    s.add_argument('--app', required=True)
    s.add_argument('--identity', default='-')
    s.add_argument('--profile', default='')
    s.add_argument('--asset', action='append')
    s.add_argument('--icon', default='', help='the 1024-px app icon (resources/icons/)')
    s.add_argument('--platform', default='iphoneos', help='actool platform: iphoneos / iphonesimulator')
    s.add_argument('--min-os', default='15.0')
    s.set_defaults(fn=sign)
    r = sub.add_parser('run-sim')
    r.add_argument('--app', required=True)
    r.add_argument('--bundle-id', required=True)
    r.add_argument('--device', default='', help='a Simulator UDID (default: the booted one)')
    r.add_argument('--obb-dir', default='')
    r.add_argument('--env', default=None, help='k2env.txt contents, KEY=VALUE;...')
    r.set_defaults(fn=run_sim)
    d = sub.add_parser('run-device')
    d.add_argument('--app', required=True)
    d.add_argument('--device', default='', help='UDID or name (default: the first paired iPhone / iPad)')
    d.add_argument('--profile', default='', help='a .mobileprovision (default: picked from Xcode\'s)')
    d.add_argument('--obb-dir', default='')
    d.add_argument('--env', default=None, help='k2env.txt contents, KEY=VALUE;...')
    d.add_argument('--no-launch', action='store_true')
    d.add_argument('--no-console', action='store_true', help='launch and return')
    d.set_defaults(fn=run_device)
    i = sub.add_parser('ipa')
    i.add_argument('--app', required=True)
    i.add_argument('--out', required=True)
    i.set_defaults(fn=ipa)
    a = ap.parse_args()
    a.fn(a)


if __name__ == '__main__':
    main()
