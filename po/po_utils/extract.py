"""Handle the extract command."""

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from __future__ import annotations

from collections import defaultdict
import contextlib
import os
import pathlib
import subprocess  # nosec B404
from tempfile import NamedTemporaryFile
from typing import Mapping, TextIO, Union

from .rcfile import extract_rc
from .utilities import Location, Message, exe_path, quote
from .uifile import extract_ui


def _write_po(file: TextIO, messages: Mapping[Message, list[Location]]) -> None:
    print('msgid ""', file=file)
    print('msgstr ""', file=file)
    print('"Content-Type: text/plain; charset=UTF-8\\n"', file=file)
    for message, locations in messages.items():
        print("", file=file)

        output_comments = (comment for loc in locations for comment in loc.comments)
        for comment in output_comments:
            print(f"#. {comment}", file=file)
        for location in locations:
            print(f"#: {location.file}:{location.line}", file=file)
        if message.context:
            print(f"msgctxt {quote(message.context)}", file=file)
        print(f"msgid {quote(message.message)}", file=file)
        print('msgstr ""', file=file)


def _get_files() -> tuple[list[str], list[str], list[str]]:
    c_files = []
    rc_files = []
    ui_files = []
    for root, _dirs, files in os.walk("../src", topdown=True):
        for file in files:
            extension = os.path.splitext(file)[1]
            file_path = pathlib.Path(os.path.join(root, file)).as_posix()
            if extension in (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".hxx"):
                c_files.append(file_path)
            elif extension == ".rc":
                rc_files.append(file_path)
            elif extension == ".ui":
                ui_files.append(file_path)
    return c_files, rc_files, ui_files


@contextlib.contextmanager
def _temporary_po_file():
    # Workaround for missing delete_on_close in Python <3.12
    temp_file = NamedTemporaryFile(
        mode="wt", encoding="utf-8", suffix=".po", delete=False
    )
    try:
        yield temp_file.file
    finally:
        os.unlink(temp_file.name)


def extract_strings(
    gettext_dir: Union[str, os.PathLike],
    po_dir: Union[str, os.PathLike],
) -> None:
    """Extracts strings from the Celestia sources"""

    # Set current directory to get consistent file path output
    os.chdir(po_dir)

    xgettext = exe_path(gettext_dir, "xgettext")
    c_files, rc_files, ui_files = _get_files()
    extra_messages = defaultdict(list)
    for file in rc_files:
        for message, location in extract_rc(file):
            extra_messages[message].append(location)
    for file in ui_files:
        for message, location in extract_ui(file):
            extra_messages[message].append(location)

    if extra_messages:
        ctx = _temporary_po_file()
    else:
        ctx = contextlib.nullcontext()

    with ctx as extract_file:
        if extra_messages:
            _write_po(extract_file, extra_messages)
            c_files.append(extract_file.name)
            extract_file.close()

        subprocess.run(  # nosec B603
            [
                xgettext,
                "--keyword=_",
                "--keyword=N_",
                "--keyword=i18n",
                "--keyword=i18nc:1c,2",
                "--keyword=NC_:1c,2",
                "--keyword=C_:1c,2",
                "--sort-by-file",
                "--qt",
                "--add-comments",
                "--default-domain=celestia",
                "--package-name=celestia",
                "--package-version=1.7.0",
                "--msgid-bugs-address=team@celestiaproject.space",
                "--copyright-holder=Celestia Development Team",
                "--output=celestia.pot",
                "--from-code=utf-8",
                "--files-from=-",
            ],
            input="\n".join(c_files).encode("utf-8"),
            check=True,
        )
