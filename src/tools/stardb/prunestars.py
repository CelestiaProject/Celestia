#!/usr/bin/env python3

# prunestars.py
#
# Original version by Andrew Tribick
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.


"""Reduces the star database to HIP entries only for low resource environments."""

import argparse
import pathlib
import struct


def prune_stars_dat(stars_path: pathlib.Path, out_stars_path: pathlib.Path) -> None:
    """Prunes the stars.dat file"""
    with open(stars_path, 'rb') as file:
        data: bytes = file.read()

    header_format = struct.Struct('<8sHI')
    star_format = struct.Struct('<I')
    header, version, total_stars = header_format.unpack_from(data, 0)
    if header != b'CELSTARS' or version != 0x0100:
        raise RuntimeError('Bad format for stars.dat, aborting')

    with open(out_stars_path, 'wb') as file:
        file.write(header_format.pack(b'CELSTARS', 0x0100, 0))
        output_stars = 0
        for i in range(total_stars):
            offset = 14 + i*20
            hip = star_format.unpack_from(data, offset)[0]
            if hip >= 1_000_000:
                continue
            output_stars += 1
            file.write(data[offset:offset+20])

        file.seek(10)
        file.write(struct.pack('<I', output_stars))

    print(f"stars.dat: original {total_stars} => pruned {output_stars}")


def prune_xindex(xindex_path: pathlib.Path, out_xindex_path: pathlib.Path) -> None:
    """Prunes an xindex file"""
    basename = xindex_path.name
    with open(xindex_path, 'rb') as file:
        data: bytes = file.read()

    header_format = struct.Struct('<8sH')
    entry_format = struct.Struct('<I')
    header, version = header_format.unpack_from(data, 0)
    if header != b'CELINDEX' or version != 0x0100:
        raise RuntimeError(f"Bad format for {basename}, aborting")

    with open(out_xindex_path, 'wb') as file:
        file.write(header_format.pack(b'CELINDEX', 0x0100))
        total_entries = 0
        output_entries = 0
        while True:
            offset = 10 + total_entries*8
            if offset >= len(data):
                break

            total_entries += 1
            hip = entry_format.unpack_from(data, offset)[0]
            if hip >= 1_000_000:
                continue

            file.write(data[offset:offset+8])
            output_entries += 1

    print(f'{basename}: original {total_entries} => pruned {output_entries}')


def prune_starnames(names_path: pathlib.Path, out_names_path: pathlib.Path) -> None:
    """Prunes the starnames.dat file"""
    with open(names_path, 'rt', encoding='utf-8') as file:
        lines = file.readlines()

    output_lines = 0
    with open(out_names_path, 'wt', encoding='utf-8') as file:
        for line in lines:
            offset = line.index(':')
            hip = int(line[:offset])
            if hip >= 1_000_000:
                continue
            file.write(line)
            output_lines += 1

    print(f'starnames.dat: original {len(lines)} => pruned {output_lines}')


def prune_files(datadir: pathlib.Path, outdir: pathlib.Path) -> None:
    """Prunes the database to just HIP entries"""
    prune_stars_dat(datadir/'stars.dat', outdir/'stars.dat')

    for xindex in ['hdxindex.dat', 'saoxindex.dat']:
        prune_xindex(datadir/xindex, outdir/xindex)

    prune_starnames(datadir/'starnames.dat', outdir/'starnames.dat')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        prog='prunestars',
        description='Reduce the stars.dat and cross index files to HIP entries only',
    )

    parser.add_argument(
        'datadir', type=pathlib.Path,
        help='Path of the Celestia data directory',
    )
    parser.add_argument(
        'outdir', type=pathlib.Path, default='.', nargs='?',
        help='Directory to output the files, if not specified then the current directory is used',
    )

    args = parser.parse_args()
    prune_files(args.datadir, args.outdir)
