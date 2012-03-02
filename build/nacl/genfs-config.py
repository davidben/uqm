import os
import os.path

srcdir = "./content"
destdir = os.path.join(os.environ["OUTPUTDIR"], "data")
manifest = "manifest"

packs = []

# Separate each alien into its own pack.
voice_dir = os.path.join(srcdir, 'addons/3dovoice')
for alien in os.listdir(voice_dir):
    if os.path.isdir(os.path.join(voice_dir, alien)):
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

# Separate each font into its own pack. (Too small?)
font_dir = os.path.join(srcdir, 'base/fonts')
for font in os.listdir(font_dir):
    if os.path.isdir(os.path.join(font_dir, font)):
        packs.append([['base/fonts/%s' % font, '*', '']])

packs.append([['*', '*', '']])
