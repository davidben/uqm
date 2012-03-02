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
packs.append([['base/lander/', '*', ''],
              ['base/nav/', '*', ''],
              ['base/planets/', '*', '']])

# Separate comm files. Pack with matching font.
for c in get_dirs(os.path.join(srcdir, 'base/comm')):
    packs.append([['base/fonts/%s.fon' % c, '*', ''],
                  ['base/comm/%s' % c, '*', '']])

# Pack 3domusic separately.
packs.append([['addons/3domusic', '*', '*.rmp']])

packs.append([['*', '*', '']])
