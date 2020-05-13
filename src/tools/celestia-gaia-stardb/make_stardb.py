#!/usr/bin/env python3

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

"""Makes the star database."""

import contextlib
import gzip
import os
import struct
import tarfile
import warnings

import numpy as np
import astropy.units as u

from astropy import io
from astropy.table import join, unique, vstack
from astropy.units import UnitsWarning

from parse_hip import process_hip
from parse_tyc import process_tyc
from spparse import CEL_UNKNOWN_STAR, parse_spectrum_vec

# temperatures from star.cpp, spectral types O3-M9

TEFF_SPEC = np.array([
    52500, 48000, 44500, 41000, 38000, 35800, 33000,
    30000, 25400, 22000, 18700, 17000, 15400, 14000, 13000, 11900, 10500,
    9520, 9230, 8970, 8720, 8460, 8200, 8020, 7850, 7580, 7390,
    7200, 7050, 6890, 6740, 6590, 6440, 6360, 6280, 6200, 6110,
    6030, 5940, 5860, 5830, 5800, 5770, 5700, 5630, 5570, 5410,
    5250, 5080, 4900, 4730, 4590, 4350, 4200, 4060, 3990, 3920,
    3850, 3720, 3580, 3470, 3370, 3240, 3050, 2940, 2640, 2000])

TEFF_BINS = (TEFF_SPEC[:-1] + TEFF_SPEC[1:]) // 2

CEL_SPECS = parse_spectrum_vec(['OBAFGKM'[i//10]+str(i%10) for i in range(3, 70)])

def load_ubvri():
    """Load UBVRI Teff calibration from VizieR archive."""
    print('Loading UBVRI calibration')
    with tarfile.open(os.path.join('vizier', 'ubvriteff.tar.gz'), 'r:gz') as tf:
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['V-K', 'B-V', 'V-I', 'J-K', 'H-K', 'Teff']
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names)
            reader.data.table_name = 'table3.dat'
            with tf.extractfile('./table3.dat.gz') as gzf, gzip.open(gzf, 'rb') as f:
                # Suppress a warning generated because the reader does not handle logarithmic units
                with warnings.catch_warnings():
                    warnings.simplefilter('ignore', UnitsWarning)
                    return reader.read(f)

def parse_spectra(data):
    """Parse the spectral types into the celestia.Sci format."""
    print('Parsing spectral types')
    data['SpType'] = data['SpType'].filled('')
    sptypes = unique(data['SpType',])
    sptypes['CelSpec'] = parse_spectrum_vec(sptypes['SpType'].filled(''))
    return join(data, sptypes)

def estimate_magnitudes(data):
    """Estimates magnitudes and color indices from G magnitude and BP-RP.

    Formula used is from Evans et al. (2018) "Gaia Data Release 2: Photometric
    content and validation".
    """
    print("Computing missing magnitudes and color indices")

    bp_rp = data['bp_rp'].filled(0)
    bp_rp2 = bp_rp**2

    data['Vmag'] = data['Vmag'].filled(data['phot_g_mean_mag'].filled(np.nan)
                                       + 0.01760
                                       + bp_rp*0.006860
                                       + bp_rp2*0.1732)
    data['e_Vmag'] = data['e_Vmag'].filled(0.045858)
    data['Vmag'].mask = np.logical_or(data['Vmag'].mask, np.isnan(data['Vmag']))
    data['e_Vmag'].mask = data['Vmag'].mask

    bp_rp = data['bp_rp'].filled(np.nan)
    bp_rp2 = bp_rp**2

    imag = data['phot_g_mean_mag'].filled(np.nan) - 0.02085 - bp_rp*0.7419 + bp_rp2*0.09631
    e_imag = np.where(np.isnan(imag), np.nan, 0.04956)

    f_bmag = data['Bmag'].filled(np.nan)
    f_vmag = data['Vmag'].filled(np.nan)
    f_jmag = data['Jmag'].filled(np.nan)
    f_hmag = data['Hmag'].filled(np.nan)
    f_kmag = data['Kmag'].filled(np.nan)
    f_e_bmag = data['e_Bmag'].filled(np.nan)
    f_e_vmag = data['e_Vmag'].filled(np.nan)
    f_e_jmag = data['e_Jmag'].filled(np.nan)
    f_e_hmag = data['e_Hmag'].filled(np.nan)
    f_e_kmag = data['e_Kmag'].filled(np.nan)

    data['B-V'] = data['B-V'].filled(f_bmag - f_vmag)
    data['e_B-V'] = data['e_B-V'].filled(np.sqrt(f_e_bmag**2 + f_e_vmag**2))
    data['V-I'] = data['V-I'].filled(f_vmag - imag)
    data['e_V-I'] = data['e_V-I'].filled(np.sqrt(f_e_vmag**2 + e_imag**2))
    data['V-K'] = f_vmag - f_kmag
    data['e_V-K'] = np.sqrt(f_e_vmag**2 + f_e_kmag**2)
    data['J-K'] = f_jmag - f_kmag
    data['e_J-K'] = np.sqrt(f_e_jmag**2 + f_e_kmag**2)
    data['H-K'] = f_hmag - f_kmag
    data['e_H-K'] = np.sqrt(f_e_hmag**2 + f_e_kmag**2)

    data['B-V'].mask = np.logical_or(data['B-V'].mask, np.isnan(data['B-V']))
    data['e_B-V'].mask = np.logical_or(data['e_B-V'].mask, np.isnan(data['e_B-V']))
    data['V-I'].mask = np.logical_or(data['V-I'].mask, np.isnan(data['V-I']))
    data['e_V-I'].mask = np.logical_or(data['e_V-I'].mask, np.isnan(data['e_V-I']))
    data['V-K'].mask = np.isnan(data['V-K'])
    data['e_V-K'].mask = np.isnan(data['e_V-K'])
    data['J-K'].mask = np.isnan(data['J-K'])
    data['e_J-K'].mask = np.isnan(data['e_J-K'])
    data['H-K'].mask = np.isnan(data['H-K'])
    data['e_H-K'].mask = np.isnan(data['e_H-K'])

    data.remove_columns(['Bmag', 'e_Bmag', 'e_Vmag', 'Jmag', 'e_Jmag', 'Hmag', 'e_Hmag',
                         'Kmag', 'e_Kmag'])

def estimate_temperatures(data):
    """Estimate the temperature of stars."""
    ubvri_data = load_ubvri()
    print('Estimating temperatures from color indices')

    data_bv = data['B-V'].filled(np.nan)
    data_vi = data['V-I'].filled(np.nan)
    data_vk = data['V-K'].filled(np.nan)
    data_jk = data['J-K'].filled(np.nan)
    data_hk = data['H-K'].filled(np.nan)

    data_e_bv = np.maximum(data['e_B-V'].filled(np.nan), 0.001)
    data_e_vi = np.maximum(data['e_V-I'].filled(np.nan), 0.001)
    data_e_vk = np.maximum(data['e_V-K'].filled(np.nan), 0.001)
    data_e_jk = np.maximum(data['e_J-K'].filled(np.nan), 0.001)
    data_e_hk = np.maximum(data['e_H-K'].filled(np.nan), 0.001)

    weights = np.full_like(data['HIP'], 0, dtype=np.float64)
    teffs = np.full_like(data['HIP'], 0, dtype=np.float64)
    for row in ubvri_data:
        sumsq = np.maximum(np.nan_to_num(((data_bv-row['B-V'])/data_e_bv)**2)
                           + np.nan_to_num(((data_vk-row['V-K'])/data_e_vk)**2)
                           + np.nan_to_num(((data_jk-row['J-K'])/data_e_jk)**2)
                           + np.nan_to_num(((data_vi-row['V-I'])/data_e_vi)**2)
                           + np.nan_to_num(((data_hk-row['H-K'])/data_e_hk)**2), 0.001)
        teffs += row['Teff'] / sumsq
        weights += 1.0 / sumsq

    data['teff_est'] = teffs / weights
    data['teff_est'].unit = u.K

def estimate_spectra(data):
    """Estimate the spectral type of stars."""
    no_teff = data[data['teff_val'].mask]
    # temporarily disable no-member error in pylint, as it cannot see the reduce method
    # pylint: disable=no-member
    has_indices = np.logical_and.reduce((no_teff['B-V'].mask,
                                         no_teff['V-I'].mask,
                                         no_teff['V-K'].mask,
                                         no_teff['J-K'].mask,
                                         no_teff['H-K'].mask))
    # pylint: enable=no-member
    no_teff = no_teff[np.logical_not(has_indices)]
    estimate_temperatures(no_teff)
    data = join(data,
                no_teff['HIP', 'teff_est'],
                keys=['HIP'],
                join_type='left')
    data['teff_val'] = data['teff_val'].filled(data['teff_est'].filled(np.nan))
    data = data[np.logical_not(np.isnan(data['teff_val']))]
    data['CelSpec'] = CEL_SPECS[np.digitize(data['teff_val'], TEFF_BINS)]
    return data

def merge_all():
    """Merges the HIP and TYC data."""
    hip_data = process_hip()
    tyc_data = join(process_tyc(),
                    hip_data[np.logical_not(hip_data['source_id'].mask)]['source_id', 'HIP'],
                    keys=['source_id'],
                    join_type='left')
    tyc_data = tyc_data[tyc_data['HIP'].mask]
    tyc_data.remove_columns('HIP')
    tyc_data.rename_column('TYC', 'HIP')
    return vstack([hip_data, tyc_data], join_type='outer', metadata_conflicts='silent')

OBLIQUITY = np.radians(23.4392911)
COS_OBLIQUITY = np.cos(OBLIQUITY)
SIN_OBLIQUITY = np.sin(OBLIQUITY)
ROT_MATRIX = np.array([[1, 0, 0],
                       [0, COS_OBLIQUITY, SIN_OBLIQUITY],
                       [0, -SIN_OBLIQUITY, COS_OBLIQUITY]])

def process_data():
    """Processes the missing data values."""
    data = merge_all()
    data = data[np.logical_not(data['dist_use'].mask)]
    estimate_magnitudes(data)
    data = parse_spectra(data)
    unknown_spectra = data[data['CelSpec'] == CEL_UNKNOWN_STAR]['HIP', 'teff_val', 'B-V', 'e_B-V',
                                                                'V-I', 'e_V-I', 'V-K', 'e_V-K',
                                                                'J-K', 'e_J-K', 'H-K', 'e_H-K']
    unknown_spectra = estimate_spectra(unknown_spectra)
    data = join(data,
                unknown_spectra['HIP', 'CelSpec'],
                keys=['HIP'],
                join_type='left',
                table_names=['data', 'est'])
    data['CelSpec'] = np.where(data['CelSpec_data'] == CEL_UNKNOWN_STAR,
                               data['CelSpec_est'].filled(CEL_UNKNOWN_STAR),
                               data['CelSpec_data'])
    data.remove_columns(['phot_g_mean_mag', 'bp_rp', 'teff_val', 'SpType', 'B-V', 'e_B-V', 'V-I',
                         'e_V-I', 'V-K', 'e_V-K', 'J-K', 'e_J-K', 'H-K', 'e_H-K', 'CelSpec_est',
                         'CelSpec_data'])

    data['Vmag_abs'] = data['Vmag'] - 5*(np.log10(data['dist_use'])-1)

    print('Converting coordinates to ecliptic frame')

    data['ra'].convert_unit_to(u.rad)
    data['dec'].convert_unit_to(u.rad)
    data['dist_use'].convert_unit_to(u.lyr)

    coords = np.matmul(ROT_MATRIX,
                       np.array([data['dist_use']*np.cos(data['ra'])*np.cos(data['dec']),
                                 data['dist_use']*np.sin(data['dec']),
                                 -data['dist_use']*np.sin(data['ra'])*np.cos(data['dec'])]))
    data['x'] = coords[0]
    data['y'] = coords[1]
    data['z'] = coords[2]

    data['x'].unit = u.lyr
    data['y'].unit = u.lyr
    data['z'].unit = u.lyr

    return data

def write_starsdat(data, outfile):
    """Write the stars.dat file."""
    print('Writing stars.dat')
    with open(outfile, 'wb') as f:
        f.write(struct.pack('<8sHL', b'CELSTARS', 0x0100, len(data)))
        print(f'  Writing {len(data)} records')
        for hip, x, y, z, vmag_abs, celspec in zip(data['HIP'], data['x'], data['y'], data['z'],
                                                   data['Vmag_abs'], data['CelSpec']):
            f.write(struct.pack('<L3fhH', hip, x, y, z, int(round(vmag_abs*256)), celspec))

def write_xindex(data, field, outfile):
    """Write a cross-index file."""
    print('Writing '+field+' cross-index')
    print('  Extracting cross-index data')
    data = data[np.logical_not(data[field].mask)]['HIP', 'Comp', field]
    data['Comp'] = data['Comp'].filled('')
    data = unique(data.group_by([field, 'Comp', 'HIP']), keys=[field])
    print(f'  Writing {len(data)} records')
    with open(outfile, 'wb') as f:
        f.write(struct.pack('<8sH', b'CELINDEX', 0x0100))
        for hip, cat in zip(data['HIP'], data[field]):
            f.write(struct.pack('<2L', cat, hip))

def make_stardb():
    """Make the Celestia star database files."""
    data = process_data()

    with contextlib.suppress(FileExistsError):
        os.mkdir('output')

    write_starsdat(data, os.path.join('output', 'stars.dat'))

    xindices = [
        ('HD', 'hdxindex.dat'),
        ('SAO', 'saoxindex.dat')
    ]

    for fieldname, outfile in xindices:
        write_xindex(data, fieldname, os.path.join('output', outfile))

if __name__ == '__main__':
    make_stardb()
