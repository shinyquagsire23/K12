#!/usr/bin/env python3
"""gcglyphs.py -- writes resources/gc_button_*.svg, the GameCube button-prompt art padglyphs.py
renders (GC_UIBUT / GC_LETTERS there). The SVGs are checked in and are what the build reads; edit
this and re-run it to change them:

    python3 platforms/tools/gcglyphs.py [resources]

The colours are sampled from a photo of the indigo controller: mint A, red B, light-grey X / Y
kidneys (turned as on the pad, letters upright), grey triggers / Start / stick / d-pad, the yellow
C-stick in its orange gate, and violet Z with a thinner ZL beside it (the Switch 2 GameCube
controller's left Z). Every button sits on a disc of the indigo shell so the light ones read on any
background. Letters are stroked paths (GLYPH), not text, so no font is needed.
"""
import os, sys
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))), 'resources')
BODY, BODY_DK = '#4b51a2', '#272d6e'          # indigo shell, its shadowed edge
TEAL, TEAL_LT, TEAL_INK = '#5fc9bb', '#b4ece4', '#12665c'
RED, RED_LT, RED_INK = '#d1182e', '#ff6670', '#fff0f0'
GREY, GREY_LT, GREY_INK = '#c8c6d4', '#f4f3f8', '#5d5b86'
VIOLET, VIOLET_LT, VIOLET_INK = '#5a52d6', '#9c96ff', '#f0eeff'
YEL, YEL_LT, YEL_INK = '#f2c51c', '#fbee70', '#b06a00'
GATE = '#f0a51a'   # the C-stick's orange gate

# stroke font, unit box 0..1 high (x scaled by `w`)
GLYPH = {
    'A': (0.84, 'M0 1L0.42 0L0.84 1M0.15 0.66H0.69'),
    'B': (0.64, 'M0 0V1H0.4C0.74 1 0.74 0.47 0.38 0.47H0M0 0H0.36C0.66 0 0.66 0.47 0.36 0.47'),
    'X': (0.8, 'M0 0L0.8 1M0.8 0L0 1'),
    'Y': (0.8, 'M0 0L0.4 0.5L0.8 0M0.4 0.5V1'),
    'Z': (0.74, 'M0 0H0.74L0 1H0.74'),
    'L': (0.62, 'M0 0V1H0.62'),
    'R': (0.72, 'M0 1V0H0.4C0.7 0 0.7 0.52 0.4 0.52H0M0.36 0.52L0.72 1'),
    'S': (0.7, 'M0.66 0.14C0.55 -0.03 0.06 -0.04 0.06 0.25C0.06 0.52 0.66 0.46 0.66 0.75C0.66 1.06 0.12 1.04 0.02 0.85'),
    'T': (0.72, 'M0 0H0.72M0.36 0V1'),
    'C': (0.76, 'M0.74 0.2C0.6 -0.06 0.02 -0.06 0.02 0.5C0.02 1.06 0.6 1.06 0.74 0.8'),
}

def text(s, cx, cy, h, color, sw, gap=0.22):
    """The stroked letters of s, h high, centred on (cx, cy)."""
    w = sum(GLYPH[c][0] for c in s) + gap * (len(s) - 1)
    x = cx - w * h / 2
    out = []
    for c in s:
        gw, d = GLYPH[c]
        out.append('<path d="%s" transform="translate(%.2f %.2f) scale(%.2f)" fill="none" stroke="%s" '
                   'stroke-width="%.3f" stroke-linejoin="round" stroke-linecap="round"/>'
                   % (d, x, cy - h / 2, h, color, sw / h))
        x += (gw + gap) * h
    return '\n'.join(out)

def grad(id_, lt, base, cx=0.38, cy=0.3):
    return ('<radialGradient id="%s" cx="%s" cy="%s" r="0.75"><stop offset="0" stop-color="%s"/>'
            '<stop offset="1" stop-color="%s"/></radialGradient>' % (id_, cx, cy, lt, base))

SHELL = ('<linearGradient id="shell" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="%s"/>'
         '<stop offset="1" stop-color="%s"/></linearGradient>' % (BODY, BODY_DK))

def svg(name, w, h, note, defs, body):
    s = ('<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d">\n'
         '<!-- %s -->\n<defs>\n%s\n</defs>\n%s\n</svg>\n' % (w, h, w, h, note, '\n'.join(defs), body))
    with open(os.path.join(OUT, 'gc_button_%s.svg' % name), 'w') as f:
        f.write(s)

NOTE = 'GameCube prompt art (indigo controller): '

def disc(name, letter, base, lt, ink, note, r=50):
    body = ['<circle cx="64" cy="64" r="62" fill="url(#shell)"/>',
            '<circle cx="64" cy="66" r="%d" fill="#000" fill-opacity="0.35"/>' % r,
            '<circle cx="64" cy="63" r="%d" fill="url(#face)"/>' % r,
            '<circle cx="64" cy="63" r="%.1f" fill="none" stroke="#000" stroke-opacity="0.18" stroke-width="2"/>' % (r - 1),
            text(letter, 64, 63, r * 1.06, ink, r * 0.2)]
    svg(name, 128, 128, NOTE + note, [SHELL, grad('face', lt, base)], '\n'.join(body))

disc('a', 'A', TEAL, TEAL_LT, TEAL_INK, 'the big mint A')
disc('b', 'B', RED, RED_LT, RED_INK, 'the red B')

def bean(name, letter, p0, p1, p2, angle, note):
    """A kidney: the quadratic p0-p1-p2 stroked fat, turned `angle` degrees (clockwise) and
    centred on the canvas; the letter stays upright on the curve's middle."""
    import math
    c, sn = math.cos(math.radians(angle)), math.sin(math.radians(angle))
    rot = lambda x, y: (64 + (x - 64) * c - (y - 64) * sn, 64 + (x - 64) * sn + (y - 64) * c)
    pts = [rot(*[(1 - t) ** 2 * p0[i] + 2 * t * (1 - t) * p1[i] + t * t * p2[i] for i in (0, 1)])
           for t in [k / 32 for k in range(33)]]
    dx = 64 - (min(x for x, _ in pts) + max(x for x, _ in pts)) / 2
    dy = 64 - (min(y for _, y in pts) + max(y for _, y in pts)) / 2
    d = 'M%g %gQ%g %g %g %g' % (p0 + p1 + p2)
    mx, my = pts[16]
    body = ['<g transform="translate(%.2f %.2f) rotate(%d 64 64)">' % (dx, dy, angle),
            '<path d="%s" fill="none" stroke="url(#shell)" stroke-width="58" stroke-linecap="round"/>' % d,
            '<path d="%s" transform="translate(0 3)" fill="none" stroke="#000" stroke-opacity="0.35" stroke-width="40" stroke-linecap="round"/>' % d,
            '<path d="%s" fill="none" stroke="url(#face)" stroke-width="40" stroke-linecap="round"/>' % d, '</g>',
            text(letter, mx + dx, my + dy, 25, GREY_INK, 6)]
    defs = [SHELL, '<linearGradient id="face" gradientUnits="userSpaceOnUse" x1="30" y1="30" x2="98" y2="98">'
            '<stop offset="0" stop-color="%s"/><stop offset="1" stop-color="%s"/></linearGradient>' % (GREY_LT, GREY)]
    svg(name, 128, 128, NOTE + note, defs, '\n'.join(body))

# Y above A, hollow side down; X right of A, hollow side left (as they hug the A button)
bean('y', 'Y', (32, 74), (64, 54), (96, 74), -15, 'the grey Y kidney, hollow side towards A, turned 15 degrees anticlockwise')
bean('x', 'X', (54, 32), (74, 64), (54, 96), -30, 'the grey X kidney, hollow side towards A, turned 30 degrees anticlockwise')

def pill(name, letters, hgt, note):
    y0, rx = 64 - hgt / 2, hgt / 2
    body = ['<rect x="2" y="%.1f" width="124" height="%.1f" rx="%.1f" fill="url(#shell)"/>' % (y0 - 13, hgt + 26, rx + 13),
            '<rect x="14" y="%.1f" width="100" height="%.1f" rx="%.1f" fill="#000" fill-opacity="0.35"/>' % (y0 + 3, hgt, rx),
            '<rect x="14" y="%.1f" width="100" height="%.1f" rx="%.1f" fill="url(#face)"/>' % (y0, hgt, rx),
            text(letters, 64, 64, hgt * 0.62, VIOLET_INK, 6.5, 0.4)]
    svg(name, 128, 128, NOTE + note, [SHELL, grad('face', VIOLET_LT, VIOLET, 0.4, 0.15)], '\n'.join(body))

pill('z', 'Z', 46, 'the violet Z pill (the right shoulder)')
pill('zl', 'ZL', 38, 'ZL, the Switch 2 GameCube controller\'s left Z: the Z pill, a little thinner')

def trigger(name, letter, mirror, note):
    # a wide analog trigger, its outer top corner rounded off
    d = 'M44 14H112A12 12 0 0 1 124 26V102A12 12 0 0 1 112 114H16A12 12 0 0 1 4 102V54A40 40 0 0 1 44 14Z'
    di = 'M45 26H104A8 8 0 0 1 112 34V94A8 8 0 0 1 104 102H24A8 8 0 0 1 16 94V55A29 29 0 0 1 45 26Z'
    g = '<g transform="translate(128 0) scale(-1 1)">' if mirror else '<g>'
    body = [g, '<path d="%s" fill="url(#shell)"/>' % d,
            '<path d="%s" transform="translate(0 3)" fill="#000" fill-opacity="0.35"/>' % di,
            '<path d="%s" fill="url(#face)"/>' % di, '</g>',
            text(letter, 64, 66, 46, GREY_INK, 9)]
    svg(name, 128, 128, NOTE + note, [SHELL, grad('face', GREY_LT, GREY, 0.5 if mirror else 0.35, 0.2)],
        '\n'.join(body))

trigger('l', 'L', False, 'the grey L trigger')
trigger('r', 'R', True, 'the grey R trigger')

# START/PAUSE: the small round grey button, unlabelled as on the pad
body = ['<circle cx="64" cy="64" r="62" fill="url(#shell)"/>',
        '<circle cx="64" cy="67" r="44" fill="#000" fill-opacity="0.35"/>',
        '<circle cx="64" cy="64" r="44" fill="url(#face)"/>',
        '<circle cx="64" cy="64" r="43" fill="none" stroke="#000" stroke-opacity="0.15" stroke-width="2"/>']
svg('start', 128, 128, NOTE + 'START/PAUSE', [SHELL, grad('face', GREY_LT, GREY)], '\n'.join(body))

# Home (the Switch 2 controller's): what Back shows, a GameCube pad has no Back
body = ['<circle cx="64" cy="64" r="62" fill="url(#shell)"/>',
        '<circle cx="64" cy="66" r="48" fill="#000" fill-opacity="0.35"/>',
        '<circle cx="64" cy="63" r="48" fill="url(#face)"/>',
        '<path d="M64 33L94 59H86V90H72V71H56V90H42V59H34Z" fill="%s" stroke="%s" stroke-width="2" stroke-linejoin="round"/>' % (GREY_INK, GREY_INK)]
svg('home', 128, 128, NOTE + 'Home (Switch 2 GameCube controller), shown for Back',
    [SHELL, grad('face', GREY_LT, GREY)], '\n'.join(body))

def octagon(cx, cy, r):
    import math
    pts = ['%.2f %.2f' % (cx + r * math.cos(math.radians(22.5 + 45 * i)),
                          cy + r * math.sin(math.radians(22.5 + 45 * i))) for i in range(8)]
    return 'M' + 'L'.join(pts) + 'Z'

def stick(cx, cy, s, base, lt, rings, letter=None, ink=None):
    """A stick in its octagonal gate, scale s (1 = the 128 badge)."""
    out = ['<circle cx="%.2f" cy="%.2f" r="%.2f" fill="url(#shell)"/>' % (cx, cy, 62 * s),
           '<path d="%s" fill="%s"/>' % (octagon(cx, cy, 50 * s), GATE if letter else BODY_DK),
           '<circle cx="%.2f" cy="%.2f" r="%.2f" fill="#000" fill-opacity="0.4"/>' % (cx, cy + 4 * s, 38 * s),
           '<circle cx="%.2f" cy="%.2f" r="%.2f" fill="url(#face)"/>' % (cx, cy, 38 * s)]
    for r in rings:
        out.append('<circle cx="%.2f" cy="%.2f" r="%.2f" fill="none" stroke="#000" stroke-opacity="0.16" stroke-width="%.2f"/>'
                   % (cx, cy, r * s, 2.2 * s))
    if letter:
        out.append(text(letter, cx, cy, 40 * s, ink, 7 * s))
    return out

svg('stick', 128, 128, NOTE + 'the grey control stick in its octagonal gate',
    [SHELL, grad('face', GREY_LT, GREY)], '\n'.join(stick(64, 64, 1, GREY, GREY_LT, (8, 18, 28))))
svg('cstick', 128, 128, NOTE + 'the yellow C-stick in its octagonal gate',
    [SHELL, grad('face', YEL_LT, YEL)], '\n'.join(stick(64, 64, 1, YEL, YEL_LT, (), 'C', YEL_INK)))
# the C-stick pushed up / down (64 x 128, as the Switch art's rstick_ud)
arrows = ('M32 2L54 24H42V32H22V24H10ZM32 126L54 104H42V96H22V104H10Z')
body = ['<path d="%s" fill="%s" stroke="%s" stroke-width="3" stroke-linejoin="round"/>' % (arrows, YEL, BODY_DK)]
body += stick(32, 64, 0.5, YEL, YEL_LT, (), 'C', YEL_INK)
svg('cstick_ud', 64, 128, NOTE + 'the C-stick, pushed up / down', [SHELL, grad('face', YEL_LT, YEL)], '\n'.join(body))

def dpad(cx, cy, s):
    a, b = 9 * s, 27 * s
    cross = ('M{0} {1}H{2}V{3}H{4}V{5}H{2}V{6}H{0}V{5}H{7}V{3}H{0}Z'.format(
        cx - a, cy - b, cx + a, cy - a, cx + b, cy + a, cy + b, cx - b))
    tri = []
    for dx, dy in ((0, -1), (1, 0), (0, 1), (-1, 0)):
        tx, ty, h = cx + dx * 18 * s, cy + dy * 18 * s, 4.5 * s
        px, py = -dy, dx
        tri.append('M%.2f %.2fL%.2f %.2fL%.2f %.2fZ' % (tx + dx * h, ty + dy * h,
                   tx - dx * h + px * h, ty - dy * h + py * h, tx - dx * h - px * h, ty - dy * h - py * h))
    return ['<circle cx="%.2f" cy="%.2f" r="%.2f" fill="url(#shell)"/>' % (cx, cy, 31 * s),
            '<path d="%s" transform="translate(0 %.2f)" fill="#000" fill-opacity="0.4"/>' % (cross, 2 * s),
            '<path d="%s" fill="url(#face)" stroke="#000" stroke-opacity="0.25" stroke-width="%.2f" stroke-linejoin="round"/>' % (cross, s),
            '<path d="%s" fill="%s" fill-opacity="0.55"/>' % (''.join(tri), GREY_INK),
            '<circle cx="%.2f" cy="%.2f" r="%.2f" fill="#000" fill-opacity="0.12"/>' % (cx, cy, 3.5 * s)]

svg('dpad', 64, 64, NOTE + 'the grey + Control Pad', [SHELL, grad('face', GREY_LT, GREY)], '\n'.join(dpad(32, 32, 1)))
body = dpad(44, 32, 1) + ['<path d="M76 8L122 32L76 56L85 32Z" fill="%s" stroke="%s" stroke-width="4" stroke-linejoin="round"/>' % (TEAL, BODY_DK)]
svg('dpad_arrow', 128, 64, NOTE + 'the Control Pad, and an arrow onward (party select)',
    [SHELL, grad('face', GREY_LT, GREY)], '\n'.join(body))
