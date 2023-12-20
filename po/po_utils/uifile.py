"""Routines for processing Qt .ui files."""

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# Functionality based on extractrc.pl
# © 2004, Richard Evans <rich@ridas.com>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from __future__ import annotations

from typing import Generator, Optional, TYPE_CHECKING, Union
import xml.etree.ElementTree as ET  # nosec B405

from .utilities import Location, Message

if TYPE_CHECKING:
    import os


def _element_details(element: ET.Element) -> str:
    details = [
        element.attrib.get("class", None),
        element.attrib.get("name", None),
    ]

    details_str = ", ".join(detail for detail in details if detail)
    return f"{element.tag} ({details_str})"


def _get_ectx(ancestors: list[tuple[int, ET.Element]]) -> Optional[str]:
    if not ancestors:
        return None
    if len(ancestors) == 1:
        return f"ectx: {_element_details(ancestors[0][1])}"
    return f"ectx: {_element_details(ancestors[-1][1])}, {_element_details(ancestors[-2][1])}"


def extract_ui(
    filename: Union[str, os.PathLike]
) -> Generator[tuple[Message, Location], None, None]:
    """Extracts messages from a single Qt .ui file."""
    with open(filename, "rb") as file:
        parser = ET.XMLPullParser(events=["start", "end"])
        ancestors: list[ET.Element] = []
        for line_number, line in enumerate(file):
            parser.feed(line)
            for event, element in parser.read_events():
                if event == "start":
                    ancestors.append((line_number + 1, element))
                    continue
                start_line = ancestors.pop()[0]
                if element.tag != "string":
                    continue

                comments = [element.attrib.get("extracomment"), _get_ectx(ancestors)]
                if not element.text or element.attrib.get("notr") == "true":
                    continue
                message = Message(
                    message=element.text, context=element.attrib.get("comment")
                )
                location = Location(
                    file=str(filename),
                    line=start_line,
                    comments=[comment for comment in comments if comment],
                )
                yield message, location
