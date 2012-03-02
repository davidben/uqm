import os
import os.path

srcdir = "../../../content"
destdir = "data"
manifest = "manifest"

# Separate each alien into its own pack.
packs = []
voice_dir = os.path.join(srcdir, 'addons/3dovoice')
for alien in os.listdir(voice_dir):
    if os.path.isdir(os.path.join(voice_dir, alien)):
        packs.append([['addons/3dovoice/%s' % alien, '*', '']])

# TODO: Separate out other files. Maybe landers and whatnot. The
# downside is that we spread the initial latency, so good to download
# in the background too.
packs.append([['*', '*', '']])
