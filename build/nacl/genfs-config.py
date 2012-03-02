import os
import os.path

srcdir = "./content"
destdir = os.path.join(os.environ["OUTPUTDIR"], "data")
manifest = "manifest"

packs = []

def get_dirs(path):
    return [d for d in os.listdir(path) if os.path.isdir(os.path.join(path, d))]

# Separate each alien into its own pack.
for alien in get_dirs(os.path.join(srcdir, 'addons/3dovoice')):
    packs.append([['addons/3dovoice/%s' % alien, '*', '']])

# Move ship images in two dedicated packs. One contains files loaded
# at startup, the other contains the rest.
packs.append([['base/ships/', '*-icons-*.png', ''],
              ['base/ships/', '*-meleeicons-*.png', ''],
              ['base/ships/', '*.ani', ''],
              ['base/ships/', '*.txt', '']])
packs.append([['base/ships/', '*', '']])

# Separate some more content
packs.append([['base/lander/', '*', '']])
packs.append([['base/nav/', '*', '']])
packs.append([['base/planets/', '*', '']])

# Separate comm files. Pack with matching font.
comm = set(get_dirs(os.path.join(srcdir, 'base/comm')))
for c in comm:
    packs.append([['base/fonts/%s.fon' % c, '*', ''],
                  ['base/comm/%s' % c, '*', '']])

# Pack remaining non-startup fonts together.
startup_fonts = ('starcon', 'tiny', 'micro')
font_pack = []
for f in get_dirs(os.path.join(srcdir, 'base/fonts')):
    if f.endswith('.fon'):
        base = f[:-len('.fon')]
        if base not in comm and base not in startup_fonts:
            font_pack.append(['base/fonts/%s' % f, '*', ''])
packs.append(font_pack)

# Pack 3domusic separately.
packs.append([['addons/3domusic', '*', '*.rmp']])

packs.append([['*', '*', '']])
