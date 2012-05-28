#!/bin/sh

cat <<EOF
[Desktop Entry]
Version=1.0
Type=Application
Name=Celestia
GenericName=Space simulator
Comment=Open source space simulator
TryExec=$1/bin/celestia
Exec=$1/bin/celestia &
Categories=Astronomy;Science;Qt;
Icon=/usr/share/icons/hicolor/128x128/apps/celestia.png
MimeType=application/x-celestia-script
Terminal=false
EOF