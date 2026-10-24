#!/usr/bin/env python3
"""k2apk.py -- package the platforms/ Android build into an installable APK (no Gradle).

  javac (SDL2's org.libsdl.app + k2.K2Activity, against android.jar)  -> classes/
  d8                                                                 -> classes.dex
  aapt2 compile --res (the launcher icons) + aapt2 link (the manifest) -> base.apk
  + classes.dex + lib/arm64-v8a/*.so, zipalign -p, apksigner (the debug keystore)

The debug keystore is ~/.android/debug.keystore, the one hybrid/tools/mkapk.py signs with: the two
APKs share a package and must share a signing key to be installed over each other.
"""
import argparse, glob, os, shutil, subprocess, sys, zipfile


def run(cmd, **kw):
    r = subprocess.run(cmd, **kw)
    if r.returncode:
        sys.exit('k2apk: failed: %s' % ' '.join(cmd))


def tool(d, name):
    """`name` in directory d, as the SDK ships it on this host: d8 and apksigner are .bat scripts on
    Windows, aapt2 and zipalign .exe."""
    for ext in ('', '.exe', '.bat') if os.name == 'nt' else ('',):
        p = os.path.join(d, name + ext)
        if os.path.isfile(p):
            return p
    sys.exit('k2apk: no %s in %s' % (name, d))


def strip_nameless_method_parameters(path):
    """Drop a method's MethodParameters attribute when it names a parameter with no name (index 0).

    javac 21+ records the mandated / synthetic parameters of an anonymous or inner class's
    constructor that way (JDK-8292275), and the d8 of build-tools 34 (R8 8.2) dies on them with a
    NullPointerException (SDL's HIDDeviceBLESteamController$1). The attribute only feeds reflection
    (Parameter.getName); nothing here reads it. Returns whether the file changed."""
    b = open(path, 'rb').read()
    u2 = lambda o: int.from_bytes(b[o:o + 2], 'big')
    u4 = lambda o: int.from_bytes(b[o:o + 4], 'big')
    utf8, i, o = {}, 1, 10
    sizes = {3: 4, 4: 4, 5: 8, 6: 8, 7: 2, 8: 2, 9: 4, 10: 4, 11: 4, 12: 4, 15: 3, 16: 2, 17: 4, 18: 4, 19: 2, 20: 2}
    while i < u2(8):                                   # the constant pool
        tag = b[o]
        if tag == 1:
            n = u2(o + 1)
            utf8[i] = b[o + 3:o + 3 + n]
            o += 3 + n
        else:
            o += 1 + sizes[tag]
        i += 2 if tag in (5, 6) else 1
    o += 6
    o += 2 + 2 * u2(o)                                 # interfaces

    def members(o, out, strip):
        n = u2(o)
        out += b[o:o + 2]
        o += 2
        for _ in range(n):
            out += b[o:o + 6]
            na = u2(o + 6)
            o += 8
            kept = []
            for _ in range(na):
                ln = u4(o + 2)
                attr = b[o:o + 6 + ln]
                if strip and utf8.get(u2(o)) == b'MethodParameters' and any(
                        u2(o + 7 + 4 * k) == 0 for k in range(b[o + 6])):
                    pass
                else:
                    kept.append(attr)
                o += 6 + ln
            out += len(kept).to_bytes(2, 'big') + b''.join(kept)
        return o

    out = bytearray(b[:o])
    o = members(o, out, False)                         # fields
    o = members(o, out, True)                          # methods
    out += b[o:]
    if bytes(out) == b:
        return False
    open(path, 'wb').write(out)
    return True


def jdk_tool(name):
    """javac / keytool: $JAVA_HOME/bin first (Android Studio's bundled JBR need not be on PATH),
    then PATH."""
    home = os.environ.get('JAVA_HOME')
    if home:
        for ext in ('.exe', '') if os.name == 'nt' else ('',):
            p = os.path.join(home, 'bin', name + ext)
            if os.path.isfile(p):
                return p
    p = shutil.which(name)
    if not p:
        sys.exit('k2apk: no %s (set JAVA_HOME to a JDK, e.g. Android Studio\'s jbr)' % name)
    return p


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--sdk', required=True)
    ap.add_argument('--build-tools', required=True, help='build-tools version, e.g. 34.0.0')
    ap.add_argument('--platform', required=True, help='android.jar platform, e.g. android-34')
    ap.add_argument('--min-sdk', default='23')
    ap.add_argument('--target-sdk', default='34')
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--java', action='append', default=[], help='a Java source root (repeatable)')
    ap.add_argument('--lib', action='append', default=[], help='a native library for lib/arm64-v8a')
    ap.add_argument('--asset', action='append', default=[], help='a file for assets/ (repeatable)')
    ap.add_argument('--res', default='', help='an Android res/ directory (the launcher icons)')
    ap.add_argument('--work', required=True)
    ap.add_argument('--out', required=True)
    ap.add_argument('--keystore', default=os.path.expanduser('~/.android/debug.keystore'))
    a = ap.parse_args()

    bt = os.path.join(a.sdk, 'build-tools', a.build_tools)
    jar = os.path.join(a.sdk, 'platforms', a.platform, 'android.jar')
    for p in (bt, jar):
        if not os.path.exists(p):
            sys.exit('k2apk: missing %s (install it with the SDK manager)' % p)
    if os.path.exists(a.work):
        shutil.rmtree(a.work)
    classes = os.path.join(a.work, 'classes')
    os.makedirs(classes)

    srcs = sorted(f for root in a.java for f in glob.glob(os.path.join(root, '**', '*.java'), recursive=True))
    # --release 8 with android.jar on the CLASSPATH (not the boot path), as the Android Gradle
    # plugin does: java.* comes from the JDK's Java 8 API -- android.jar has no LambdaMetafactory,
    # which SDL's lambdas compile against -- and d8 desugars the lambdas for the device.
    # The source list goes in an @argfile, the classes in one jar: Windows caps a command line (and
    # d8.bat's, through cmd.exe, at 8191 characters).
    argfile = os.path.join(a.work, 'javac-sources.txt')
    with open(argfile, 'w', encoding='utf-8') as f:
        f.writelines('"%s"\n' % p.replace('\\', '/') for p in srcs)
    run([jdk_tool('javac'), '-nowarn', '-Xlint:-options', '-encoding', 'UTF-8', '--release', '8',
         '-classpath', jar, '-d', classes, '@' + argfile])
    for p in glob.glob(os.path.join(classes, '**', '*.class'), recursive=True):
        strip_nameless_method_parameters(p)
    classes_jar = os.path.join(a.work, 'classes.jar')
    with zipfile.ZipFile(classes_jar, 'w', zipfile.ZIP_STORED) as z:
        for p in sorted(glob.glob(os.path.join(classes, '**', '*.class'), recursive=True)):
            z.write(p, os.path.relpath(p, classes).replace(os.sep, '/'))
    run([tool(bt, 'd8'), '--release', '--min-api', a.min_sdk, '--lib', jar,
         '--output', a.work, classes_jar])

    base = os.path.join(a.work, 'base.apk')
    compiled = []
    if a.res:
        res_zip = os.path.join(a.work, 'res.zip')
        run([tool(bt, 'aapt2'), 'compile', '--dir', a.res, '-o', res_zip])
        compiled = [res_zip]
    run([tool(bt, 'aapt2'), 'link', '-o', base, '--manifest', a.manifest, '-I', jar,
         '--min-sdk-version', a.min_sdk, '--target-sdk-version', a.target_sdk] + compiled)

    unaligned = os.path.join(a.work, 'unaligned.apk')
    shutil.copy(base, unaligned)
    with zipfile.ZipFile(unaligned, 'a', zipfile.ZIP_DEFLATED) as z:
        z.write(os.path.join(a.work, 'classes.dex'), 'classes.dex')
        for lib in a.lib:
            z.write(lib, 'lib/arm64-v8a/' + os.path.basename(lib))
        for asset in a.asset:
            z.write(asset, 'assets/' + os.path.basename(asset))

    aligned = os.path.join(a.work, 'aligned.apk')
    run([tool(bt, 'zipalign'), '-p', '-f', '4', unaligned, aligned])
    run([tool(bt, 'apksigner'), 'sign', '--ks', a.keystore, '--ks-pass', 'pass:android',
         '--key-pass', 'pass:android', '--out', a.out, aligned])
    print('k2apk: wrote %s (%d MB)' % (a.out, os.path.getsize(a.out) >> 20))


if __name__ == '__main__':
    main()
