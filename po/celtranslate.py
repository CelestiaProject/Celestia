#!/usr/bin/env python3

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# Functionality based on extract_resource_strings.pl
# Original version by Christophe Teyssier <chris@teyssier.org>
#
# Functionality based on extractrc.pl
# © 2004, Richard Evans <rich@ridas.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

"""Extracts strings from the Celestia source code"""

from __future__ import annotations

import argparse
import os
import pathlib

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Extract strings from Celestia sources"
    )
    parser.add_argument(
        "-p",
        "--po-dir",
        dest="po_dir",
        type=pathlib.Path,
        help="Path to po directory",
        required=False,
    )
    parser.add_argument(
        "-g",
        "--gettext-dir",
        dest="gettext_dir",
        type=pathlib.Path,
        help="Path to directory containing gettext executables",
        required=False,
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    extract_parser = subparsers.add_parser(
        "extract", help="Extract messages into celestia.pot"
    )

    merge_parser = subparsers.add_parser(
        "merge", help="Merge celestia.pot into the .po files"
    )

    args = parser.parse_args()
    po_dir = args.po_dir if args.po_dir else os.path.dirname(__file__)
    if args.command == "extract":
        from po_utils.extract import extract_strings

        extract_strings(
            args.gettext_dir,
            po_dir,
        )
    elif args.command == "merge":
        from po_utils.merge import merge_po

        merge_po(
            args.gettext_dir,
            po_dir,
        )
