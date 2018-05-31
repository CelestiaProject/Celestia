#!/bin/bash

dir=$(dirname $0)

$dir/extractrc $(find src -name \*.ui -o -name \*.rc -o -name \*.kcfg) > testrc.cpp
xgettext -ki18n -k_ -F -j --qt \
         -d celestia --package-name=celestia --package-version=1.7.0 --msgid-bugs-address=chris@teyssier.org --copyright-holder="Chris Laurel" \
         $(find . -name \*.c -o -name \*.cpp -o -name \*.cc -o -name \*.h -o -name \*.hpp -o -name \*.qml) -o po/celestia.pot
rm testrc.cpp
