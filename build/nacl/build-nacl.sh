#!/bin/sh
# NaCl build script
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

# You can start this program as 'SH="/bin/sh -x" ./build.sh' to
# enable command tracing.

set -e

if [ $# -ne 1 ]; then
    echo "Usage $0 OUTPUTDIR"
    exit 1
fi

# Make output directory.
OUTPUTDIR=$(readlink -f $1)
[ -d "$OUTPUTDIR" ] || mkdir -p "$OUTPUTDIR"

WORKDIR=$(mktemp -d /tmp/uqm_nacl.XXXXXX)

cd "$(readlink -f "$(dirname "$0")")/../.."

build_nacl_port() {
    local BITSIZE
    BITSIZE="$1"

    mkdir "$WORKDIR/$BITSIZE"
    cp ./build/nacl/config.state "$WORKDIR/$BITSIZE"
    BUILD_WORK="$WORKDIR/$BITSIZE" NACL_PACKAGES_BITSIZE="$BITSIZE" ./build/nacl/build-nacl-nexe.sh uqm reprocess_config
    BUILD_WORK="$WORKDIR/$BITSIZE" NACL_PACKAGES_BITSIZE="$BITSIZE" ./build/nacl/build-nacl-nexe.sh uqm

    cp "$WORKDIR/$BITSIZE/uqm.nexe" "$OUTPUTDIR/uqm-$BITSIZE.nexe"
}

# Build binaries.
build_nacl_port 64
build_nacl_port 32

# Build filesystem and static files
./build/nacl/build-nacl-shared.sh "$OUTPUTDIR"

# Clean up.
rm -Rf "$WORKDIR"
