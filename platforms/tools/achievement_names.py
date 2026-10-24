#!/usr/bin/env python3
"""achievement_names.py -- regenerate the achievement name tables the desktop builds embed.

    achievement_names.py [--schema UserGameStatsSchema_208580.bin] [--game k1|k2|all]

Writes resources/achievements/<game>/achievement_names.txt, which embedfiles.py compiles into the
binary and shim/shim_achievements.cpp reads for the unlock notice (the local backend's bubble and
message-log line):

    # version N                 (KOTOR II: the Steam schema's version; the runtime prefers the
                                 player's own cached schema when it is newer)
    tag<TAB>language<TAB>name<TAB>description

  k2  from Steam's cached stats schema for app 208580, a binary KeyValues file
      (<Steam>/appcache/stats/UserGameStatsSchema_208580.bin; the default is the macOS Steam
      client's copy). Every language the schema has (english, german, french, italian, spanish).
  k1  KOTOR 1 has no Steam achievements; the Android port's Google Play names are the strings
      src/game/swmain/Achievements.cpp's KOTOR1 dispatcher passes ("a_mandalorian_affair"). The
      APK carries only their Play Games ids; names and descriptions come from players' list of
      the Play achievements (K1_NAMES), English only.
"""
import argparse
import os
import re
import struct
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_SCHEMA = os.path.expanduser(
    '~/Library/Application Support/Steam/appcache/stats/UserGameStatsSchema_208580.bin')
K2_APPID = '208580'


def parse_kv(b, i=0):
    """Binary KeyValues: <type byte><key\\0><value>, a node ends at type 8 (or 11)."""
    node = {}
    while i < len(b):
        t = b[i]
        i += 1
        if t in (8, 11):
            return node, i
        j = b.index(0, i)
        key = b[i:j].decode('utf-8', 'replace')
        i = j + 1
        if t == 0:
            val, i = parse_kv(b, i)
        elif t == 1:
            j = b.index(0, i)
            val = b[i:j].decode('utf-8', 'replace')
            i = j + 1
        elif t in (2, 4, 6):
            val = struct.unpack_from('<i', b, i)[0]
            i += 4
        elif t == 3:
            val = struct.unpack_from('<f', b, i)[0]
            i += 4
        elif t in (7, 10):
            val = struct.unpack_from('<q', b, i)[0]
            i += 8
        elif t == 5:
            j = i
            while b[j:j + 2] != b'\0\0':
                j += 2
            val = b[i:j].decode('utf-16le', 'replace')
            i = j + 2
        else:
            raise ValueError(f'KeyValues type {t} at offset {i - 1}')
        node[key] = val
    return node, i


def clean(s):
    return ' '.join(str(s).split())


def tag_key(tag):
    m = re.match(r'(.*?)(\d+)$', tag)
    return (m.group(1), int(m.group(2))) if m else (tag, -1)


def k2_rows(schema_path):
    root, _ = parse_kv(open(schema_path, 'rb').read())
    app = root[K2_APPID]
    rows = []
    for stat in app.get('stats', {}).values():
        if not isinstance(stat, dict):
            continue
        for bit in stat.get('bits', {}).values():
            if not isinstance(bit, dict) or 'name' not in bit:
                continue
            disp = bit.get('display', {})
            names = disp.get('name', {})
            descs = disp.get('desc', {})
            if not isinstance(names, dict):
                continue
            for lang, name in names.items():
                if lang == 'token':
                    continue
                desc = descs.get(lang, '') if isinstance(descs, dict) else ''
                rows.append((bit['name'], lang, clean(name), clean(desc)))
    langs = ['english', 'german', 'french', 'italian', 'spanish']
    rows.sort(key=lambda r: (tag_key(r[0]), langs.index(r[1]) if r[1] in langs else 99, r[1]))
    return int(app.get('version', 0)), rows


# Contractions and possessives the Google Play names spell without their apostrophe.
K1_WORDS = {'thats': "That's", 'dont': "Don't", 'lets': "Let's", 'cant': "Can't",
            'wolfs': "Wolf's", 'wookies': "Wookiee's", 'hunters': "Hunter's"}
K1_SMALL = {'a', 'an', 'the', 'of', 'on', 'to', 'in', 'with', 'by', 'ur', 'for'}


def humanise(tag):
    words = [w for w in tag.split('_') if w]
    out = []
    for n, w in enumerate(words):
        if w in K1_WORDS:
            out.append(K1_WORDS[w])
        elif n > 0 and w in K1_SMALL and w != 'ur':
            out.append(w)
        else:
            out.append(w[:1].upper() + w[1:])
    return ' '.join(out)


# The Google Play list as players recorded it (r/kotor, "KOTOR1 for Android Achievements list"):
# name and description per tag. The tag keeps the words, so it settles the post's shorthand
# ("Chosen One", "Thats Not true/Impossible!"); it turns a trailing '.' into '_' and drops other
# punctuation ("roooarrgh_ur_roo_", "objection_your_honor"), so "End of a Slaver." has no period.
K1_NAMES = {
    'rude_awakening': ("Rude Awakening", "Escape the Endar Spire"),
    'jedi_knight': ("Jedi Knight", "Finish the game on the Light Side"),
    'sith_lord': ("Sith Lord", "Finish the game on the Dark Side"),
    'your_training_continues': ("Your Training Continues", "Recruit Bastila"),
    'allies_of_convenience': ("Allies of Convenience", "Recruit Carth"),
    'a_bigger_gun': ("A Bigger Gun", "Recruit Canderous"),
    'why_dont_you_put_her_in_charge': ("Why Don't You Put Her in Charge?", "Recruit Mission"),
    'the_wookies_with_me': ("The Wookie's with Me", "Recruit Zaalbar"),
    'back_on_the_wagon': ("Back on the Wagon", "Recruit Jolee"),
    'betrayed_by_anger': ("Betrayed by Anger", "Recruit Juhani"),
    'lets_find_some_meatbags': ("Let's Find Some Meatbags", "Recruit HK-47"),
    'skeleton_key': ("Skeleton Key", "Recruit T3-M4"),
    'objection_your_honor': ("Objection, Your Honor!", "Save Sunry from execution"),
    'roooarrgh_ur_roo_': ("Roooarrgh ur roo.", "Side with Zaalbar's brother or father"),
    'memory_backup_completed': ("Memory Backup Completed", "Unlock all of HK-47's memories"),
    'still_in_trouble': ("Still in Trouble", "Bring a tach gland to Griff Vao"),
    'a_mandalorian_affair': ("A Mandalorian Affair", "Duel with Jagi on Tatooine"),
    'revealing_the_obvious': ("Revealing the Obvious", "Prove to Dustil that the Sith are evil"),
    'end_of_a_slaver': ("End of a Slaver", "Defeat Xor, the Cathar slaver"),
    'treasure_hunters_legacy': ("Treasure Hunter's Legacy", "Find Bastila's father's holocron"),
    'an_extra_bond': ("An Extra Bond", "Kiss Bastila"),
    'cant_wait': ("Can't Wait", "Get a declaration of love from Carth"),
    'wrath_of_the_sith': ("Wrath of the Sith", "Steal the Ebon Hawk and flee Taris"),
    'student_of_the_force': ("Student of the Force", "Find the Star Map in the ancient ruins"),
    'beneath_the_sand': ("Beneath the Sand", "Find the Star Map in the krayt dragon's cave"),
    'in_wolfs_clothing': ("In Wolf's Clothing", "Find the Star Map in the tomb of Naga Sadow"),
    'sea_of_intrigue': ("Sea of Intrigue", "Find the Star Map and leave the underwater facility"),
    'lost_to_the_jungle': ("Lost to the Jungle", "Find the Star Map in the Shadowlands"),
    'captured': ("Captured", "Escape the Leviathan"),
    'thats_not_true_thats_impossible': ("That's Not True! That's Impossible!", "Learn your true identity"),
    'crash_landing': ("Crash Landing", "Reach the Unknown World"),
    'thats_no_moon': ("That's No Moon", "Infiltrate the Star Forge"),
    'the_chosen_one': ("The Chosen One", "Defeat Darth Malak"),
}


def k1_rows():
    src = open(os.path.join(REPO, 'src/game/swmain/Achievements.cpp'), encoding='utf-8').read()
    block = src[src.index('#if KOTOR1'):]
    tags = []
    for t in re.findall(r'UnlockAchievement\("([a-z0-9_]+)"\)', block):
        if t not in tags:
            tags.append(t)
    missing = sorted(set(tags) - set(K1_NAMES))
    if missing:
        print('K1 tags with no listed name (humanised):', ', '.join(missing), file=sys.stderr)
    return [(t, 'english') + K1_NAMES.get(t, (humanise(t), '')) for t in sorted(tags)]


def write(game, header, version, rows):
    path = os.path.join(REPO, 'resources/achievements', game, 'achievement_names.txt')
    lines = header + [f'# version {version}', '# tag\tlanguage\tname\tdescription']
    lines += ['\t'.join(r) for r in rows]
    text = '\n'.join(lines) + '\n'
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(text)
    print(f'achievement_names: {len(rows)} row(s) -> {os.path.relpath(path, REPO)}')


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--schema', default=DEFAULT_SCHEMA, help='Steam UserGameStatsSchema_208580.bin')
    ap.add_argument('--game', default='all', choices=['k1', 'k2', 'all'])
    args = ap.parse_args()
    if args.game in ('k2', 'all'):
        if not os.path.exists(args.schema):
            print(f'achievement_names: no schema at {args.schema} (--schema)', file=sys.stderr)
            return 1
        version, rows = k2_rows(args.schema)
        write('k2', ['# KOTOR II achievement names: platforms/tools/achievement_names.py, from Steam\'s',
                     f'# stats schema for app {K2_APPID}. Read by shim/shim_achievements.cpp.'], version, rows)
    if args.game in ('k1', 'all'):
        write('k1', ['# KOTOR achievement names: platforms/tools/achievement_names.py, the Android port\'s',
                     '# Google Play names and descriptions. Read by shim/shim_achievements.cpp.'], 0, k1_rows())
    return 0


if __name__ == '__main__':
    sys.exit(main())
