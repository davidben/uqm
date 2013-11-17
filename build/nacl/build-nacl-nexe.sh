#!/bin/bash
# NaCl Build helper
# Copyright (c) 2012 David Benjamin
# Copyright (c) 2013 Google, Inc.
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

cd "$(readlink -f "$(dirname "$0")")/../.."

if [ -z "$NACL_SDK_ROOT" ]; then
	echo "Error: NACL_SDK_ROOT not set" 1>&2
	exit 1
fi
if [ -z "$NACL_PORTS" ]; then
	echo "Error: NACL_PORTS not set" 1>&2
	exit 1
fi

export BUILD_HOST="NaCl"
export BUILD_HOST_ENDIAN="little"

# common.sh gets unhappy without setting this.
PACKAGE_NAME=uqm-0.7.0

# There's a nacl_env.sh --print, but it doesn't pick up where naclports builds
# into.
. "${NACL_PORTS}/src/build_tools/common.sh"

export CC="${NACLCC}"
export CXX="${NACLCXX}"
export AR="${NACLAR}"
export RANLIB="${NACLRANLIB}"
export PKG_CONFIG_PATH="${NACLPORTS_LIBDIR}/pkgconfig"
export PKG_CONFIG_LIBDIR="${NACLPORTS_LIBDIR}"
export CFLAGS="${NACLPORTS_CFLAGS}"
export CXXFLAGS="${NACLPORTS_CXXFLAGS}"
export LDFLAGS="${NACLPORTS_LDFLAGS}"
export PATH="${NACL_BIN_PATH}:${NACLPORTS_PREFIX_BIN}:${PATH}"

./build.sh uqm reprocess_config
./build.sh uqm
