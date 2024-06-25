"""Routines for processing Windows .rc files."""

# Copyright © 2023, the Celestia Development Team
# Original version by Andrew Tribick, December 2023
#
# Functionality based on extract_resource_strings.pl
# Original version by Christophe Teyssier <chris@teyssier.org>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.

from __future__ import annotations

from enum import auto, Enum
import re
from typing import Generator, Optional, NamedTuple, TextIO, TYPE_CHECKING, Union

from .utilities import Location, Message, unquote

if TYPE_CHECKING:
    import os


class TokenType(Enum):
    """Token types"""

    WHITESPACE = auto()
    NEWLINE = auto()
    COMMENT = auto()
    INSTRUCTION = auto()
    OPEN = auto()
    CLOSE = auto()
    OPERATOR = auto()
    NUMBER = auto()
    KEYWORD = auto()
    QUOTED = auto()


class Token(NamedTuple):
    """Token info"""

    type: TokenType
    value: str
    line: int


_MATCHERS = [
    (re.compile(r"[ \t]+"), TokenType.WHITESPACE),
    (re.compile(r"[A-Za-z_][0-9A-Za-z_]*"), TokenType.KEYWORD),
    (re.compile(r"[1-9][0-9]*|0(?:[xX][0-9A-Fa-f]+|[0-7]*)"), TokenType.NUMBER),
]

_OPMATCH = {
    "(": TokenType.OPEN,
    "[": TokenType.OPEN,
    "{": TokenType.OPEN,
    "}": TokenType.CLOSE,
    "]": TokenType.CLOSE,
    ")": TokenType.CLOSE,
}


class RCTokenizer:
    """Simple RC file tokenizer, no preprocessor."""

    source: TextIO
    data: str
    line: int
    pos: int

    def __init__(self, source: TextIO) -> None:
        self.source = source
        self.data = source.readline().rstrip("\n")
        self.line = 1
        self.pos = 0

    def __iter__(self) -> RCTokenizer:
        return self

    def __next__(self) -> Token:
        """Gets a token from the file"""
        if self.pos >= len(self.data):
            self.data = self.source.readline()
            if not self.data:
                raise StopIteration
            self.data = self.data.rstrip("\n")
            self.line += 1
            self.pos = 0
            return Token(TokenType.NEWLINE, "\n", self.line - 1)

        ch = self.data[self.pos]
        if ch == "/":
            token = self._handle_slash()
        elif ch == "#":
            token = self._handle_instruction()
        elif ch == '"':
            token = self._handle_quoted()
        else:
            for matcher, token_type in _MATCHERS:
                match = matcher.match(self.data[self.pos :])
                if match:
                    self.pos += match.end()
                    token = Token(token_type, match.group(0), self.line)
                    break
            else:
                self.pos += 1
                token = Token(_OPMATCH.get(ch, TokenType.OPERATOR), ch, self.line)

        return token

    def _handle_slash(self) -> Token:
        self.pos += 1
        if self.pos == len(self.data):
            return Token(TokenType.OPERATOR, "/", self.line)
        ch = self.data[self.pos]
        if ch == "/":
            # supply prefix as we are positioned on second / of //
            return self._read_to_eol(TokenType.COMMENT, "/")
        if ch == "*":
            return self._handle_block_comment()
        return Token(TokenType.OPERATOR, "/", self.line)

    def _handle_instruction(self) -> Token:
        return self._read_to_eol(TokenType.INSTRUCTION, "")

    def _read_to_eol(self, token_type: TokenType, prefix: str) -> Token:
        token = prefix + self.data[self.pos :]
        self.pos = len(self.data)
        return Token(token_type, token, self.line)

    def _handle_block_comment(self) -> Token:
        token = "/*"  # nosec B105
        start_line = self.line
        self.pos += 1
        new_pos = self.data.find("*/", self.pos)
        while new_pos < 0:
            token += self.data[self.pos :]
            self.data = self.source.readline()
            if not self.data:
                raise ValueError("Unclosed block comment")
            self.line += 1
            self.pos = 0
            new_pos = self.data.find("*/")

        token += self.data[self.pos : new_pos]
        self.pos = new_pos + 2
        return Token(TokenType.COMMENT, token, start_line)

    def _handle_quoted(self) -> Token:
        new_pos = self.pos
        is_escape = False
        while True:
            new_pos += 1
            if new_pos == len(self.data):
                raise ValueError("Unclosed quotes")
            ch = self.data[new_pos]
            if is_escape:
                is_escape = False
            elif ch == "\\":
                is_escape = True
            elif ch == '"':
                break
        new_pos += 1
        token = self.data[self.pos : new_pos]
        self.pos = new_pos
        return Token(TokenType.QUOTED, token, self.line)


RC_KEYWORDS = {
    "POPUP",
    "MENUITEM",
    "DEFPUSHBUTTON",
    "CAPTION",
    "PUSHBUTTON",
    "RTEXT",
    "CTEXT",
    "LTEXT",
    "GROUPBOX",
    "AUTOCHECKBOX",
    "AUTORADIOBUTTON",
}

EXTRACT_SKIP_TYPES = {
    TokenType.NEWLINE,
    TokenType.WHITESPACE,
    TokenType.COMMENT,
    TokenType.INSTRUCTION,
}

NC_TOKENS = [
    (TokenType.OPEN, "("),
    (TokenType.QUOTED, None),
    (TokenType.OPERATOR, ","),
    (TokenType.QUOTED, None),
    (TokenType.CLOSE, ")"),
]


class _RCExtractor:
    filename: str
    tokenizer: RCTokenizer

    def __init__(
        self, filename: Union[str, os.PathLike], tokenizer: RCTokenizer
    ) -> None:
        self.filename = str(filename)
        self.tokenizer = tokenizer

    def __iter__(self) -> _RCExtractor:
        return self

    def __next__(self) -> tuple[Message, Location]:
        token = None
        retry = False
        while True:
            if retry:
                retry = False
            else:
                token = next(self.tokenizer)
            if token.type != TokenType.KEYWORD:
                continue
            if token.value in RC_KEYWORDS:
                result = self._try_get_simple()
                if result:
                    return result
                retry = True

    def _try_get_simple(self) -> Optional[tuple[Message, Location]]:
        while True:
            token = next(self.tokenizer)
            if token.type not in EXTRACT_SKIP_TYPES:
                break
        if token.type == TokenType.KEYWORD and token.value == "NC_":
            return self._try_get_nc()
        if token.type != TokenType.QUOTED:
            return None
        message = unquote(token.value)
        if message.strip():
            return Message(message, None), Location(self.filename, token.line, [])
        return None

    def _try_get_nc(self) -> Optional[tuple[Message, Location]]:
        location = Location(self.filename, self.tokenizer.line, [])
        buffer = []
        while len(buffer) < 5:
            token = next(self.tokenizer)
            if token.type in EXTRACT_SKIP_TYPES:
                continue
            expected_type, expected_value = NC_TOKENS[len(buffer)]
            if token.type != expected_type or (
                expected_value is not None and token.value != expected_value
            ):
                return None
            buffer.append(token)
        message = unquote(buffer[3].value)
        context = unquote(buffer[1].value)
        return Message(message, context), location


def extract_rc(
    filename: Union[str, os.PathLike]
) -> Generator[tuple[Message, Location], None, None]:
    """Extracts messages from a single Windows .rc file"""
    with open(filename, "rt", encoding="utf-16") as file:
        tokenizer = RCTokenizer(file)
        for result in _RCExtractor(filename, tokenizer):
            yield result
