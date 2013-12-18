import os
import os.path

srcdir = "./content"
destdir = os.path.join(os.environ["OUTPUTDIR"], "data")
manifest = "manifest"

packs = []

def get_dirs(path):
    return sorted([d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))])

# Separate each alien into its own pack.
for alien in get_dirs(os.path.join(srcdir, 'addons/3dovoice')):
    packs.append([['addons/3dovoice/%s' % alien, '*', '']])

# Move ship images into a dedicated packs, except for the icons needed
# at startup.
ship_pack = []
ship_exclude = ('-icons.ani', '-icons-000.png', '-icons-001.png',
                '-meleeicons.ani', '-meleeicons-000.png', '-meleeicons-001.png',
                '.txt')
for ship in os.listdir(os.path.join(srcdir, 'base/ships')):
    for f in os.listdir(os.path.join(srcdir, 'base/ships', ship)):
        skip = False
        for suffix in ship_exclude:
            if f.endswith(suffix):
                skip = True
        if not skip:
            ship_pack.append(['base/ships/%s/%s' % (ship, f), '*', ''])
packs.append(ship_pack)

# Separate some more content
packs.append([['base/lander/', '*', ''],
              ['base/nav/', '*', ''],
              ['base/planets/', '*', '']])

# Pack comm files, along with corresponding fonts.
comm_pack = []
for c in get_dirs(os.path.join(srcdir, 'base/comm')):
    comm_pack.extend([['base/fonts/%s.fon' % c, '*', ''],
                      ['base/comm/%s' % c, '*', '']])
packs.append(comm_pack)

# Pack music needed on new game.
packs.append([['addons/3domusic/space.ogg', '*', ''],
              ['addons/3domusic/orbit*.ogg', '*', '']])
# Pack remaining music, except credits.ogg. That one is included in
# the main pack because it is loaded when you go idle, and hanging on
# idle is not very good.
packs.append([['addons/3domusic/*.ogg', '*', '*/credits.ogg']])

packs.append([['*', '*', '']])
