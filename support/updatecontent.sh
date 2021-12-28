#!/bin/sh

scripthome=$(dirname "$0" | while read a; do cd "$a" && pwd && break; done)
oldpath=$(pwd)
cd "$scripthome/.."

if [ ! -e "content" ]
then
    git clone --depth 1 "https://github.com/CelestiaProject/CelestiaContent.git" -b master content
elif [ -d "content" ]
then
    cd content
    git fetch --depth 1 origin master
    git reset --hard origin/master
else
    >&2 echo "content exists and is not a directory"
fi

cd "$oldpath"
