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

"""Routines for parsing the TYC2 data."""

import gzip
import os
import tarfile

from typing import Dict, IO, Tuple

import numpy as np
import astropy.io.ascii as io_ascii
import astropy.units as u

from astropy.table import MaskedColumn, Table, join, unique, vstack

from parse_utils import TarCds, WorkaroundCDSReader, open_cds_tarfile, read_gaia

def make_tyc(tyc1: int, tyc2: int, tyc3: int) -> int:
    """Build a synthetic HIP identifier from TYC parts."""
    return tyc1 + tyc2*10000 + tyc3*1000000000

TYC_HD_ERRATA = {
    "add": [
        # B. Skiff, 30-Jan-2007
        (make_tyc(8599, 1797, 1), 298954),
        # B. Skiff, 12-Jul-2007
        (make_tyc(6886, 1389, 1), 177868),
        # LMC inner region
        (make_tyc(9161, 685, 1), 269051),
        (make_tyc(9165, 548, 1), 269052),
        (make_tyc(9169, 1563, 1), 269207),
        (make_tyc(9166, 2, 1), 269367),
        (make_tyc(9166, 540, 1), 269382),
        (make_tyc(8891, 278, 1), 269537),
        (make_tyc(9162, 657, 1), 269599),
        (make_tyc(9167, 730, 1), 269858),
        (make_tyc(9163, 960, 2), 269928),
        (make_tyc(9163, 751, 1), 270005),
        (make_tyc(8904, 686, 1), 270078),
        (make_tyc(9163, 887, 1), 270128),
        (make_tyc(8904, 766, 1), 270342),
        (make_tyc(9168, 1217, 1), 270435),
        (make_tyc(8904, 911, 1), 270467),
        (make_tyc(8904, 5, 1), 270485),
        # LMC outer region
        (make_tyc(9157, 1, 1), 270502),
        (make_tyc(9160, 1142, 1), 270526),
        (make_tyc(8888, 928, 1), 270765),
        (make_tyc(8888, 910, 1), 270794),
        (make_tyc(9172, 791, 1), 272092),
        # VizieR annotations
        (make_tyc(7389, 1138, 1), 320669),
        # extras
        (make_tyc(1209, 1833, 1), 11502),
        (make_tyc(1209, 1835, 1), 11503),
    ],
    "delete": [
        # B. Skiff (13-Nov-2007)
        32228,
        # LMC inner region
        269686,
        269784,
        # LMC outer region
        270653,
        271058,
        271224,
        271264,
        271389,
        271600,
        271695,
        271727,
        271764,
        271802,
        271875,
        # VizieR annotations
        181060,
    ]
}

def parse_tyc_string(data: Table, src_column: str, dest_column: str='TYC') -> None:
    """Parse a TYC string into a synthetic HIP identifier."""
    tycs = np.array(np.char.split(data[src_column], '-').tolist()).astype(np.int64)
    data[dest_column] = make_tyc(tycs[:, 0], tycs[:, 1], tycs[:, 2])
    data.remove_column(src_column)

def parse_tyc_cols(data: Table,
                   src_columns: Tuple[str, str, str]=('TYC1', 'TYC2', 'TYC3'),
                   dest_column: str='TYC') -> None:
    """Convert TYC identifier components into a synthetic HIP identifier."""
    data[dest_column] = make_tyc(data[src_columns[0]],
                                 data[src_columns[1]],
                                 data[src_columns[2]])
    data.remove_columns(src_columns)

def load_gaia_tyc() -> Table:
    """Load the Gaia DR2 TYC2 sources."""
    print('Loading Gaia DR2 sources for TYC2')

    file_names = ['gaiadr2_tyc-result.csv', 'gaiadr2_tyc-result-extra.csv']
    gaia = read_gaia([os.path.join('gaia', f) for f in file_names], 'tyc2_id')

    parse_tyc_string(gaia, 'tyc2_id')
    gaia.add_index('TYC')

    return unique(gaia.group_by('TYC'), keys=['source_id'])

def load_tyc_spec() -> Table:
    """Load the TYC2 spectral type catalogue."""
    print('Loading TYC2 spectral types')
    with open_cds_tarfile(os.path.join('vizier', 'tyc2spec.tar.gz')) as tf:
        data = tf.read_gzip('catalog.dat', ['TYC1', 'TYC2', 'TYC3', 'SpType'])

    parse_tyc_cols(data)
    data.add_index('TYC')
    return data

def _load_ascc_section(tf: TarCds, table: str) -> Table:
    print(f'  Loading {table}')
    section = tf.read_gzip(table,
                           ['Bmag', 'Vmag', 'e_Bmag', 'e_Vmag', 'd3', 'TYC1', 'TYC2', 'TYC3',
                            'Jmag', 'e_Jmag', 'Hmag', 'e_Hmag', 'Kmag', 'e_Kmag', 'SpType'])

    section = section[section['TYC1'] != 0]
    parse_tyc_cols(section)

    convert_cols = ['Bmag', 'Vmag', 'e_Bmag', 'e_Vmag', 'Jmag', 'e_Jmag', 'Hmag', 'e_Hmag',
                    'Kmag', 'e_Kmag']
    for col in convert_cols:
        section[col] = section[col].astype(np.float64)
        section[col].convert_unit_to(u.mag)
        section[col].format = '.3f'

    return section

def load_ascc() -> Table:
    """Load ASCC from VizieR archive."""

    print('Loading ASCC')
    with open_cds_tarfile(os.path.join('vizier', 'ascc.tar.gz')) as tf:
        data = None
        for data_file in tf.tf:
            sections = os.path.split(data_file.name)
            if (len(sections) != 2 or sections[0] != '.' or not sections[1].startswith('cc')):
                continue
            section_data = _load_ascc_section(tf, os.path.splitext(sections[1])[0])
            if data is None:
                data = section_data
            else:
                data = vstack([data, section_data], join_type='exact')

    data = unique(data.group_by(['TYC', 'd3']), keys=['TYC'])
    data.rename_column('d3', 'Comp')
    data.add_index('TYC')
    return data

def load_tyc_hd() -> Table:
    """Load the Tycho-HD cross index."""
    print('Loading TYC-HD cross index')
    with open_cds_tarfile(os.path.join('vizier', 'tyc2hd.tar.gz')) as tf:
        data = tf.read_gzip('tyc2_hd.dat', ['TYC1', 'TYC2', 'TYC3', 'HD'])

    parse_tyc_cols(data)

    err_del = np.array(TYC_HD_ERRATA['delete'] + [a[1] for a in TYC_HD_ERRATA['add']])
    data = data[np.logical_not(np.isin(data['HD'], err_del))]

    err_add = Table(np.array(TYC_HD_ERRATA['add']),
                    names=['TYC', 'HD'],
                    dtype=[np.int64, np.int64])

    data = vstack([data, err_add], join_type='exact')

    data = unique(data.group_by('HD'), keys='TYC')
    data = unique(data.group_by('TYC'), keys='HD')

    return data

class TYCTeffReader(WorkaroundCDSReader):
    """Custom CDS loader for the TYC Teff table to reduce memory usage."""

    def __init__(self, readme: IO):
        super().__init__('tycall.dat', ['Tycho', 'Teff'], [], readme)

    def create_table(self) -> Table:
        """Creates the table."""
        return Table(
            [
                np.empty(self.record_count, np.int64),
                np.empty(self.record_count, np.float64)
            ],
            names=['TYC', 'teff_val'])

    def process_line(self, table: Table, record: int, fields: Dict[str, str]) -> bool:
        """Processes fields from a line of the input file."""
        try:
            tycsplit = fields['Tycho'].split('-')
            tyc = int(tycsplit[0]) + int(tycsplit[1])*10000 + int(tycsplit[2])*1000000000
            teff = float(fields['Teff'])
        except ValueError:
            tyc = 0
            teff = 99999

        if teff != 99999:
            table['TYC'][record] = tyc
            table['teff_val'][record] = teff
            return True

        return False

def load_tyc_teff() -> Table:
    """Load the Tycho-2 effective temperatures."""
    print('Loading TYC2 effective temperatures')
    with tarfile.open(os.path.join('vizier', 'tyc2teff.tar.gz'), 'r:gz') as tf:
        with tf.extractfile('./ReadMe') as readme:
            reader = TYCTeffReader(readme)

        with tf.extractfile('./tycall.dat.gz') as gzf, gzip.open(gzf, 'rt', encoding='ascii') as f:
            data = reader.read(f)

        data['teff_val'].unit = u.K
        data.add_index('TYC')
        return unique(data, keys=['TYC'])

def load_sao() -> Table:
    """Load the SAO-TYC2 cross match."""
    print('Loading SAO-TYC2 cross match')
    xmatch_files = ['sao_tyc2_xmatch.csv',
                    'sao_tyc2_suppl1_xmatch.csv',
                    'sao_tyc2_suppl2_xmatch.csv']
    data = vstack(
        [io_ascii.read(os.path.join('xmatch', f),
                       include_names=['SAO', 'TYC1', 'TYC2', 'TYC3', 'angDist', 'delFlag'],
                       format='csv',
                       converters={'delFlag': [io_ascii.convert_numpy(np.str)]})
         for f in xmatch_files],
        join_type='exact')

    data = data[data['delFlag'].mask]
    data.remove_column('delFlag')

    parse_tyc_cols(data)

    data = unique(data.group_by(['TYC', 'angDist']), keys=['TYC'])
    data.remove_column('angDist')

    data.add_index('TYC')
    return data

def merge_tables() -> Table:
    """Merges the tables."""
    data = join(load_gaia_tyc(), load_tyc_spec(), keys=['TYC'], join_type='left')
    data = join(data, load_ascc(),
                keys=['TYC'],
                join_type='left',
                table_names=('gaia', 'ascc'),
                metadata_conflicts='silent')
    data['SpType'] = MaskedColumn(data['SpType_gaia'].filled(data['SpType_ascc'].filled('')))
    data['SpType'].mask = data['SpType'] == ''
    data.remove_columns(['SpType_gaia', 'SpType_ascc'])

    data = join(data, load_tyc_hd(), keys=['TYC'], join_type='left', metadata_conflicts='silent')

    data = join(data,
                load_tyc_teff(),
                keys=['TYC'],
                join_type='left',
                table_names=('gaia', 'tycteff'))

    data['teff_val'] = MaskedColumn(
        data['teff_val_gaia'].filled(data['teff_val_tycteff'].filled(np.nan)))
    data['teff_val'].mask = np.isnan(data['teff_val'])
    data.remove_columns(['teff_val_tycteff', 'teff_val_gaia'])

    data = join(data, load_sao(), keys=['TYC'], join_type='left')
    return data

def process_tyc() -> Table:
    """Processes the TYC data."""
    data = merge_tables()
    data.rename_column('r_est', 'dist_use')
    return data
