#!/bin/sh
#
# This script extracts the version from include/asa/fg/Config.hpp,
# which is the master location for this information.
#

if [ ! -f include/asa/fg/Config.hpp ]; then
    echo "version.sh: error: include/asa/fg/Config.hpp does not exist" 1>&2
    exit 1
fi

MAJOR=$(grep '^#define ASA_FG_VERSION_MAJOR \+[0-9]\+' include/asa/fg/Config.hpp)
MINOR=$(grep '^#define ASA_FG_VERSION_MINOR \+[0-9]\+' include/asa/fg/Config.hpp)
PATCH=$(grep '^#define ASA_FG_VERSION_PATCH \+[0-9]\+' include/asa/fg/Config.hpp)

if [ -z "$MAJOR" -o -z "$MINOR" -o -z "$PATCH" ]; then
    echo "version.sh: error: could not extract version from include/asa/fg/Config.hpp" 1>&2
    exit 1
fi

MAJOR=$(echo $MAJOR | awk '{ print $3 }')
MINOR=$(echo $MINOR | awk '{ print $3 }')
PATCH=$(echo $PATCH | awk '{ print $3 }')

echo $MAJOR.$MINOR.$PATCH | tr -d '\n\r'
