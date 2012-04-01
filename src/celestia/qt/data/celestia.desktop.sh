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
Icon=$1/share/icons/celestia.png
MimeType=application/x-celestia-script
Terminal=false
EOF