# gaia-stardb: Processing Gaia DR2 for celestia.Sci/Celestia
# Copyright (C) 2019–2020  Andrew Tribick
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

"""Common utilities for parsers."""

import gzip
import re
import tarfile
import warnings

from contextlib import contextmanager
from tarfile import TarFile
from typing import Dict, Generator, IO, List, TextIO, Tuple, Union

import astropy.io.ascii as io_ascii
import astropy.units as u
import numpy as np

from astropy.table import Table, vstack
from astropy.units import UnitsWarning

def read_gaia(files: Union[str, List[str]], id_name: str, *, extra_fields: List[str]=None):
    """Parse the CSV files produced by querying the Gaia TAP endpoint."""
    fields = ['source_id', id_name, 'ra', 'dec', 'phot_g_mean_mag', 'bp_rp', 'teff_val', 'r_est']
    if extra_fields is not None:
        fields += extra_fields

    if isinstance(files, str):
        gaia = io_ascii.read(files, include_names=fields, format='csv')
    else:
        gaia = vstack([io_ascii.read(f, include_names=fields, format='csv') for f in files],
                      join_type='exact')

    gaia['ra'].unit = u.deg
    gaia['dec'].unit = u.deg
    gaia['phot_g_mean_mag'].unit = u.mag
    gaia['bp_rp'].unit = u.mag
    gaia['teff_val'].unit = u.K
    gaia['r_est'].unit = u.pc

    return gaia

class TarCds:
    """Routines for accessing CDS files contained with a tar archive."""
    def __init__(self, tf: TarFile):
        self.tf = tf

    def read(self, table: str, names: List[str], *, readme_name=None, **kwargs) -> Table:
        """Reads a table from the CDS archive."""
        if readme_name is None:
            readme_name = table
        with self.tf.extractfile('./ReadMe') as readme:
            reader = self._create_reader(readme, readme_name, names, **kwargs)
            with self.tf.extractfile(f'./{table}') as f:
                return self._read(reader, f)

    def read_gzip(self, table: str, names: List[str], *, readme_name=None, **kwargs) -> Table:
        """Reads a gzipped table from the CDS archive."""
        if readme_name is None:
            readme_name = table
        with self.tf.extractfile('./ReadMe') as readme:
            reader = self._create_reader(readme, readme_name, names, **kwargs)
            with self.tf.extractfile(f'./{table}.gz') as gzf, gzip.open(gzf, 'rb') as f:
                return self._read(reader, f)

    @classmethod
    def _create_reader(cls, readme: IO, table: str, names: List[str], **kwargs) -> io_ascii.Cds:
        reader = io_ascii.get_reader(io_ascii.Cds,
                                     readme=readme,
                                     include_names=names,
                                     **kwargs)
        reader.data.table_name = table
        return reader

    @classmethod
    def _read(cls, reader: io_ascii.Cds, file: IO) -> Table:
        # Suppress a warning generated because the reader does not handle logarithmic units
        with warnings.catch_warnings():
            warnings.simplefilter('ignore', UnitsWarning)
            return reader.read(file)

@contextmanager
def open_cds_tarfile(file: str) -> Generator[TarCds, None, None]:
    """Opens a CDS tarfile."""
    with tarfile.open(file, 'r:gz') as tf:
        yield TarCds(tf)

class WorkaroundCDSReader:
    """Custom CDS file reader to work around errors in input data formats."""

    def __init__(self, table: str, labels: List[str], dtypes: List[np.dtype], readme: IO):
        self.labels = labels
        self.dtypes = dtypes
        self.record_count, self.ranges = self._get_fields(table, labels, readme)

    def read(self, file: TextIO) -> Table:
        """Reads the input file according to the field specifications."""
        table = self.create_table()
        record = 0
        for line in file:
            fields = { l: line[self.ranges[l][0]:self.ranges[l][1]].strip() for l in self.labels }
            if self.process_line(table, record, fields):
                record += 1
        return table[0:record]

    def create_table(self) -> Table:
        """Creates the table."""
        return Table([np.empty(self.record_count, d) for d in self.dtypes], names=self.labels)

    def process_line(self, table: Table, record: int, fields: Dict[str, str]) -> bool:
        """Processes fields from a line of the input file."""
        for label, dtype in zip(self.labels, self.dtypes):
            try:
                table[label][record] = np.fromstring(fields[label], dtype=dtype, sep=' ')[0]
            except IndexError:
                return False
        return True

    @classmethod
    def _get_fields(cls, table: str, labels: List[str], readme: IO) \
            -> Tuple[int, Dict[str, Tuple[int, int]]]:
        ranges = {}

        re_file = re.compile(re.escape(table) + r'\ +[0-9]+\ +(?P<length>[0-9]+)')
        re_table = re.compile(r'Byte-by-byte Description of file: (?P<name>\S+)$')
        re_field = re.compile(r'''\ *(?P<start>[0-9]+)\ *-\ *(?P<end>[0-9]+) # range
                                \ +\S+ # format
                                \ +\S+ # units
                                \ +(?P<label>\S+) # label''', re.X)
        record_count = None
        current_table = None
        for line in readme:
            try:
                line = line.decode('ascii')
            except AttributeError:
                pass
            match = re_file.match(line)
            if match:
                record_count = int(match.group('length'))
                continue
            match = re_table.match(line)
            if match:
                current_table = match.group('name')
                continue
            if current_table != table:
                continue
            match = re_field.match(line)
            if not match:
                continue

            label = match.group('label')
            if label in labels:
                ranges[label] = int(match.group('start'))-1, int(match.group('end'))

            if len(ranges) == len(labels):
                break

        if record_count is None:
            raise RuntimeError('Could not get record count')
        if len(ranges) != len(labels):
            missing = ", ".join(l for l in labels if l not in ranges)
            raise RuntimeError(f'Could not find {missing} fields')

        return record_count, ranges
