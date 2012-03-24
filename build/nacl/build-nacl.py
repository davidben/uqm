#!/usr/bin/python
"""
NaCl build script
Copyright (c) 2012 David Benjamin

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
"""

import hashlib
import json
import os.path
import shutil
import subprocess
import sys
import tempfile

SCRIPT_DIR = sys.path[0]

# Map from ISA to NACL_PACKAGES_BITSIZE value from naclports.
PORTS = {
    "x86-32": "32",
    "x86-64": "64",
}

def hash_file(path):
    with open(path) as f:
        return hashlib.sha1(f.read()).hexdigest()

def build_nacl_port(workdir, bitsize):
    build_work = os.path.join(workdir, bitsize)
    os.mkdir(build_work)
    shutil.copyfile(os.path.join(SCRIPT_DIR, "config.state"),
                    os.path.join(build_work, "config.state"))
    env = dict(os.environ)
    env["BUILD_WORK"] = build_work
    env["NACL_PACKAGES_BITSIZE"] = bitsize
    subprocess.check_call(
        [os.path.join(SCRIPT_DIR, "build-nacl-nexe.sh"),
         "uqm", "reprocess_config"], env=env)
    subprocess.check_call(
        [os.path.join(SCRIPT_DIR, "build-nacl-nexe.sh"),
         "uqm"], env=env)
    return os.path.join(build_work, "uqm.nexe")

def main(outputdir):
    # Make the outputdir
    try:
        os.mkdir(outputdir)
    except OSError:
        pass

    workdir = tempfile.mkdtemp(prefix="uqm_nacl_")
    try:
        # Build all ports.
        manifest = { "program": { } }
        for isa, bitsize in PORTS.items():
            # Build it.
            print "=== Building for %s ===" % isa
            nexe = build_nacl_port(workdir, bitsize)
            # Name with SHA-1 for caching.
            sha1 = hash_file(nexe)
            filename = "uqm-%s.%s.nexe" % (isa, sha1)
            shutil.copyfile(nexe, os.path.join(outputdir, filename))
            # Put in the maniest.
            manifest["program"][isa] = { "url": filename }

        # Write the manifest
        print "=== Writing manifest ==="
        manifest_json = json.dumps(manifest)
        manifest_sha1 = hashlib.sha1(manifest_json).hexdigest()
        manifest_filename = "uqm.%s.nmf" % manifest_sha1
        with open(os.path.join(outputdir, manifest_filename), "w") as f:
            f.write(manifest_json)

        # Build static files.
        print "=== Copying shared files ==="
        subprocess.check_call(
            [os.path.join(SCRIPT_DIR, "build-nacl-shared.sh"),
             outputdir, manifest_filename,])
    finally:
        # Cleanup.
        shutil.rmtree(workdir)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print "Usage %s OUTPUTDIR" % sys.argv[0]
        sys.exit(1)
    main(sys.argv[1])
