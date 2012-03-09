#!/bin/sh
# NaCl filesystem builder
# Copyright (c) 2012 David Benjamin
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

set -e

if [ $# -ne 1 ]; then
    echo "Usage $0 OUTPUTDIR"
    exit 1
fi

export OUTPUTDIR=$(readlink -f $1)
cd "$(readlink -f "$(dirname "$0")")/../.."

# Drop in static files
for f in index.html uqm.js uqm.nmf uqm.css icon.png starfield.png; do
    cp "src/res/nacl/$f" "$OUTPUTDIR/$f"
done

# Build filesystem
./build/nacl/genfs.py ./build/nacl/genfs-config.py

# Record manifest information
MANIFEST_PATH=$(ls "$OUTPUTDIR"/data/manifest.*)
MANIFEST_SIZE=$(wc -c "$MANIFEST_PATH" | cut -f1 -d" ")
MANIFEST=$(basename "$MANIFEST_PATH")
cat > "$OUTPUTDIR/files.js" <<EOF
var manifest = "/$MANIFEST";
var manifestSize = $MANIFEST_SIZE;
EOF
