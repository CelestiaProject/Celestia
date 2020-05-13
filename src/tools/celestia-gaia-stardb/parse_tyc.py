# gaia-stardb: Processing Gaia DR2 for celestia.Sci/Celestia
# Copyright (C) 2019  Andrew Tribick
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
import re
import tarfile

import numpy as np
import astropy.units as u

from astropy import io
from astropy.table import Table, join, unique, vstack

def parse_tyc_string(data, src_column, dest_column='TYC'):
    """Parse a TYC string into a synthetic HIP identifier."""
    tycs = np.array(np.char.split(data[src_column], '-').tolist()).astype(np.int64)
    data[dest_column] = tycs[:, 0] + tycs[:, 1]*10000 + tycs[:, 2]*1000000000
    data.remove_column(src_column)

def parse_tyc_cols(data, src_columns=('TYC1', 'TYC2', 'TYC3'), dest_column='TYC'):
    """Convert TYC identifier components into a synthetic HIP identifier."""
    data[dest_column] = (data[src_columns[0]]
                         + data[src_columns[1]]*10000
                         + data[src_columns[2]]*1000000000)
    data.remove_columns(src_columns)

def load_gaia_tyc():
    """Load the Gaia DR2 TYC2 sources."""
    print('Loading Gaia DR2 sources for TYC2')
    col_names = ['source_id', 'tyc2_id', 'ra', 'dec', 'phot_g_mean_mag', 'bp_rp',
                 'teff_val', 'r_est']
    gaia = io.ascii.read(os.path.join('gaia', 'gaiadr2_tyc-result.csv'),
                         include_names=col_names,
                         format='csv')

    parse_tyc_string(gaia, 'tyc2_id')
    gaia.add_index('TYC')

    gaia['ra'].unit = u.deg
    gaia['dec'].unit = u.deg
    gaia['phot_g_mean_mag'].unit = u.mag
    gaia['bp_rp'].unit = u.mag
    gaia['teff_val'].unit = u.K
    gaia['r_est'].unit = u.pc

    return gaia

def load_tyc_spec():
    """Load the TYC2 spectral type catalogue."""
    print('Loading TYC2 spectral types')
    with tarfile.open(os.path.join('vizier', 'tyc2spec.tar.gz')) as tf:
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['TYC1', 'TYC2', 'TYC3', 'SpType']
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names)
            reader.data.table_name = 'catalog.dat'
            with tf.extractfile('./catalog.dat.gz') as gzf, gzip.open(gzf, 'rb') as f:
                data = reader.read(f)

    parse_tyc_cols(data)
    data.add_index('TYC')
    return data

def load_ascc():
    """Load ASCC from VizieR archive."""
    def load_section(tf, info):
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['Bmag', 'Vmag', 'e_Bmag', 'e_Vmag', 'd3', 'TYC1', 'TYC2', 'TYC3', 'HD',
                         'Jmag', 'e_Jmag', 'Hmag', 'e_Hmag', 'Kmag', 'e_Kmag']
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names)
            reader.data.table_name = 'cc*.dat'
            print('  Loading ' + os.path.basename(info.name))
            with tf.extractfile(info) as gzf, gzip.open(gzf, 'rb') as f:
                section = reader.read(f)

        section = section[section['TYC1'] != 0]
        parse_tyc_cols(section)

        convert_cols = ['Bmag', 'Vmag', 'e_Bmag', 'e_Vmag', 'Jmag', 'e_Jmag', 'Hmag', 'e_Hmag',
                        'Kmag', 'e_Kmag']
        for col in convert_cols:
            section[col] = section[col].astype(np.float64)
            section[col].convert_unit_to(u.mag)
            section[col].format = '.3f'

        return section

    def is_data(info):
        sections = os.path.split(info.name)
        return (len(sections) == 2 and
                sections[0] == '.' and
                sections[1].startswith('cc'))

    print('Loading ASCC')
    with tarfile.open(os.path.join('vizier', 'ascc.tar.gz'), 'r:gz') as tf:
        data = None
        for data_file in tf:
            if not is_data(data_file):
                continue
            if data is None:
                data = load_section(tf, data_file)
            else:
                data = vstack([data, load_section(tf, data_file)], join_type='exact')

    data = unique(data.group_by(['TYC', 'd3']), keys=['TYC'])
    data.rename_column('d3', 'Comp')
    data.add_index('TYC')
    return data

def load_tyc_teff():
    """Load the Tycho-2 effective temperatures."""
    print('Loading TYC2 effective temperatures')
    with tarfile.open(os.path.join('vizier', 'tyc2teff.tar.gz'), 'r:gz') as tf:
        # unfortunately the behaviour of the CDS reader results in high memory
        # usage during loading due to parsing non-used fields, so use a custom
        # implementation
        tyc_range = None
        teff_range = None
        with tf.extractfile('./ReadMe') as readme:
            re_file = re.compile(r'tycall.dat\ +[0-9]+\ +(?P<length>[0-9]+)')
            re_table = re.compile(r'Byte-by-byte Description of file: (?P<name>\S+)$')
            re_field = re.compile(r'''\ *(?P<start>[0-9]+)\ *-\ *(?P<end>[0-9]+) # range
                                  \ +\S+ # format
                                  \ +\S+ # units
                                  \ +(?P<label>\S+) # label''', re.X)
            record_count = None
            current_table = None
            for bline in readme.readlines():
                line = bline.decode('ascii')
                match = re_file.match(line)
                if match:
                    record_count = int(match.group('length'))
                    continue
                match = re_table.match(line)
                if match:
                    current_table = match.group('name')
                    continue
                if current_table != 'tycall.dat':
                    continue
                match = re_field.match(line)
                if not match:
                    continue
                if match.group('label') == 'Tycho':
                    tyc_range = int(match.group('start'))-1, int(match.group('end'))
                elif match.group('label') == 'Teff':
                    teff_range = int(match.group('start'))-1, int(match.group('end'))
                if tyc_range is not None and teff_range is not None:
                    break

        if record_count is None:
            raise RuntimeError('Could not get record count')
        if tyc_range is None or teff_range is None:
            raise RuntimeError('Could not find Tycho, Teff fields')

        with tf.extractfile('./tycall.dat.gz') as gzf, gzip.open(gzf, 'rt', encoding='ascii') as f:
            tycs = np.empty(record_count, dtype=np.int64)
            teffs = np.empty(record_count, dtype=np.float64)
            record = 0
            for line in f:
                try:
                    tycsplit = line[tyc_range[0]:tyc_range[1]].split('-')
                    tyc = int(tycsplit[0]) + int(tycsplit[1])*10000 + int(tycsplit[2])*1000000000
                    teff = float(line[teff_range[0]:teff_range[1]])
                    if teff != 99999:
                        tycs[record] = tyc
                        teffs[record] = teff
                        record += 1
                except ValueError:
                    pass

        data = Table([tycs[0:record], teffs[0:record]], names=('TYC', 'teff_val'))
        data['teff_val'].unit = u.K
        data.add_index('TYC')
        return unique(data, keys=['TYC'])

def load_sao():
    """Load the SAO-TYC2 cross match."""
    print('Loading SAO-TYC2 cross match')
    data = io.ascii.read(os.path.join('xmatch', 'sao_tyc2_xmatch.csv'),
                         include_names=['SAO', 'TYC1', 'TYC2', 'TYC3', 'angDist', 'delFlag'],
                         format='csv')

    data = data[data['delFlag'].mask]
    data.remove_column('delFlag')

    parse_tyc_cols(data)

    data = unique(data.group_by(['TYC', 'angDist']), keys=['TYC'])
    data.remove_column('angDist')

    data.add_index('TYC')
    return data

def merge_tables():
    """Merges the tables."""
    data = join(load_gaia_tyc(), load_tyc_spec(), keys=['TYC'], join_type='left')
    data = join(data, load_ascc(), keys=['TYC'], join_type='left')
    data['HD'].mask = np.logical_or(data['HD'].mask, data['HD'] == 0)
    data = join(data,
                load_tyc_teff(),
                keys=['TYC'],
                join_type='left',
                table_names=('gaia', 'tycteff'))

    data['teff_val'] = data['teff_val_gaia'].filled(data['teff_val_tycteff'].filled(np.nan))
    data['teff_val'].mask = np.isnan(data['teff_val'])
    data.remove_columns(['teff_val_tycteff', 'teff_val_gaia'])

    data = join(data, load_sao(), keys=['TYC'], join_type='left')
    return data

def process_tyc():
    """Processes the TYC data."""
    data = merge_tables()
    data.rename_column('r_est', 'dist_use')
    return data
