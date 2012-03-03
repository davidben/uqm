#!/bin/sh
# NaCl Build helper
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

cd "$(readlink -f "$(dirname "$0")")/../.."

if [ -z "$NACL_SDK_ROOT" ]; then
	echo "Error: NACL_SDK_ROOT not set" 1>&2
	exit 1
fi

export BUILD_HOST="NaCl"
export BUILD_HOST_ENDIAN="little"

if [ "$NACL_GLIBC" = 1 ]; then
	NACL_TOOLCHAIN_ROOT="$NACL_SDK_ROOT/toolchain/linux_x86"
else
	NACL_TOOLCHAIN_ROOT="$NACL_SDK_ROOT/toolchain/linux_x86_newlib"
fi

if [ "${NACL_PACKAGES_BITSIZE}" = "32" ] ; then
	NACL_CROSS_PREFIX=i686-nacl
	# Buildsystem cannot easily launch a prefixed gcc, so pass
	# -m32 instead. i686-nacl-gcc is just a shell script that
	# passes -m32 anyway.
	export CFLAGS="-m32"
	export LDFLAGS="-m32"
elif [ "${NACL_PACKAGES_BITSIZE}" = "64" ] ; then
	NACL_CROSS_PREFIX=x86_64-nacl
elif [ "${NACL_PACKAGES_BITSIZE}" = "pnacl" ] ; then
	NACL_CROSS_PREFIX=pnacl
	echo "Do not know how to build pnacl"
	exit 1
else
	echo "Unknown value for NACL_PACKAGES_BITSIZE: '${NACL_PACKAGES_BITSIZE}'" 1>&2
	exit 1
fi

CROSS_ROOT="$NACL_TOOLCHAIN_ROOT/$NACL_CROSS_PREFIX"

# Unprefixed gcc lives in x86_64-nacl.
export PATH="$NACL_TOOLCHAIN_ROOT/x86_64-nacl/bin:$PATH"

export PATH="$CROSS_ROOT/usr/bin:$PATH"
export PKG_CONFIG_PATH="$CROSS_ROOT/usr/lib/pkgconfig"

./build.sh "$@"
