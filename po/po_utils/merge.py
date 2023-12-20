"""Handle the merge command."""

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from __future__ import annotations

import glob
import os
import re
import subprocess  # nosec B404
from typing import Union

from .utilities import exe_path

_LOCALE_PATTERN = re.compile(r"^[a-z]{2}(?:_[A-Z]{2})?$")


def merge_po(
    gettext_dir: Union[str, os.PathLike], po_dir: Union[str, os.PathLike]
) -> None:
    """Merges the po files"""
    msgmerge = exe_path(gettext_dir, "msgmerge")
    pot_file = os.path.join(po_dir, "celestia.pot")
    for po_file in glob.glob(os.path.join(po_dir, "*.po")):
        basename = os.path.basename(po_file)
        if not _LOCALE_PATTERN.match(os.path.splitext(basename)[0]):
            continue
        print(f"Processing {basename}")
        subprocess.run(  # nosec B603
            [
                msgmerge,
                "--update",
                "--quiet",
                po_file,
                pot_file,
            ],
            check=True,
        )
