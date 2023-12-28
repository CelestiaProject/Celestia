"""Data types for extracting messages."""

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from __future__ import annotations

import codecs
import os
from typing import Optional, NamedTuple, Union


class Message(NamedTuple):
    """Represents a message in the source file"""

    message: str
    context: Optional[str]


class Location(NamedTuple):
    """Information about a message location"""

    file: str
    line: int
    comments: list[str]


_ESCAPES = {
    "\a": "\\a",
    "\b": "\\b",
    "\f": "\\f",
    "\n": "\\n",
    "\r": "",  # remove from output strings
    "\t": "\\t",
    "\v": "\\v",
    "\\": "\\\\",
    '"': '\\"',
}


def unquote(value: str) -> str:
    """Unquotes a string and resolves character escape sequences"""
    return codecs.unicode_escape_decode(value[1:-1])[0]


def quote(value: str) -> str:
    """Quotes a value for use in a C string"""
    result = '"'
    for ch in value:
        escape = _ESCAPES.get(ch)
        if escape:
            result += escape
            continue
        cp = ord(ch)
        if cp < 32 or cp == 127:
            result += f"\\{cp:03o}"
        else:
            result += ch
    return result + '"'


def exe_path(path: Union[str, os.PathLike], name: str) -> str:
    """Gets the path to an executable"""
    if os.name == "nt" and not os.path.splitext(name)[1]:
        name += ".exe"
    if path:
        return os.path.join(path, name)
    return name
