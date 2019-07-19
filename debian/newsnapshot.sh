#!/bin/sh

usage() {
  echo >&2 <<EOF
WARNING!

This script should be run from the root repository directory,
not from ./debian subdirectory.
EOF

  exit 1
}

test -d debian || usage

VERSION="1.7.0"
DATE=$(date +"%Y%m%d")
COMMIT=$(git log --pretty=format:"%h" -1)
test -n "${RELEASE}" || RELEASE=0

dch -v "${VERSION}~git${DATE}+${COMMIT}+${RELEASE}" "New snapshot build"
dch -r
