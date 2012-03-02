#!/usr/bin/python
# Copyright (c) 2012 The Native Client Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This scripts generates an HTTP2 file system from a set of files.
# File description comes from a configuration file (the first command line
# argument), which is a python script that defines the following variables:
#   srcdir = "/path/to/the/input/directory"
#   destdir = "/path/to/the/output/directory"
#   manifest = "prefix of the manifest file name (output)"
#   packs = [pack_description, ...] ; a list of packs
#     pack_description = [mask, ...] ; a list of masks
#       mask = (root_wildcard, include_wildcard, full_path_exclude_wildcard)
#   A mask defines the following set of files:
#     all files under (and including) the expansion of root_wildcard,
#     whose names match include_wildcard,
#     whose full path (with root_mask) does not match full_path_exclude_wildcard
#   All wildcards are expanded from srcdir.


import shutil
import glob
import os
import fnmatch
import sys
import hashlib
import tempfile

all_files = set()

def list_path_with_mask(path, mask, exclude_mask):
    files = set()
    if os.path.isdir(path):
        for (dirpath, dirnames, filenames) in os.walk(path):
            for filename in filenames:
                if (fnmatch.fnmatch(filename, mask) and
                    not fnmatch.fnmatch(os.path.join(dirpath, filename),
                                        exclude_mask)):
                    files.add(os.path.join(dirpath, filename))
    else:
        if (fnmatch.fnmatch(path, mask) and
            not fnmatch.fnmatch(path, exclude_mask)):
            files.add(path)

    return files

def list_pack_contents(masks):
    files = set()
    for (root_path, mask, exclude_mask) in masks:
        roots = glob.glob(root_path)
        for root in roots:
            mask_files = list_path_with_mask(root, mask, exclude_mask)
            new_files = mask_files.difference(all_files)
            files.update(new_files)
            all_files.update(new_files)
    return files


def build_pack(files, destdir, out):
    try:
        os.makedirs(os.path.dirname(os.path.join(destdir, out)))
    except OSError, e:
        pass
    fout = open(os.path.join(destdir, out), "w")
    h = hashlib.sha1()
    sz = 0
    pack_index = []
    for f in files:
        data = open(f).read()
        h.update(data)
        fout.write(data)
        pack_index.append((f, sz, len(data)))
        sz += len(data)
    fout.close()

    new_name = out + '.' + h.hexdigest()
    os.rename(os.path.join(destdir, out),
              os.path.join(destdir, new_name))
    print '%s: %d files, %d bytes total' % (new_name, len(files), sz)

    out_list = []
    for (path, offset, size) in pack_index:
        out_list.append('%s %d %d %s\n' % (new_name, offset, size, path))

    return ''.join(out_list)


if len(sys.argv) < 2:
    print "Usage: %s <config.py>" % (sys.argv[0],)
    sys.exit(1)
exec(open(sys.argv[1]).read())
try:
    (srcdir, destdir, packs, manifest)
except NameError:
    print "Config failed to define one or more of (srcdir, destdir, packs)"
    sys.exit(1)

destdir = os.path.abspath(destdir)
os.chdir(srcdir)

# build packs
manifest_tmp = tempfile.mkstemp()[1]
fmanifest = open(manifest_tmp, 'w')
for (index, pack) in enumerate(packs):
    pack_files = list_pack_contents(pack)
    manifest_data = build_pack(pack_files, destdir, 'pack' + str(index))
    fmanifest.write(manifest_data)

# build 1-file packs from remaining files
misc_files = list_pack_contents([['*', '*', '']])
for misc_file in misc_files:
    manifest_data = build_pack([misc_file], destdir, misc_file)
    fmanifest.write(manifest_data)

fmanifest.close()

# And the icing on the cake: pack the manifest itself
build_pack([manifest_tmp], destdir, manifest)
os.unlink(manifest_tmp)
