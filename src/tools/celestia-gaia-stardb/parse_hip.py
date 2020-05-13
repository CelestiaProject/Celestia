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

"""Routines for parsing the HIP data."""

import gzip
import os
import tarfile
import warnings

import numpy as np
import astropy.units as u

from astropy import io
from astropy.coordinates import ICRS, SkyCoord
from astropy.table import join, unique
from astropy.time import Time
from astropy.units import UnitsWarning

def load_gaia_hip():
    """Load the Gaia DR2 HIP sources."""
    print('Loading Gaia DR2 sources for HIP')
    col_names = ['source_id', 'hip_id', 'ra', 'dec', 'phot_g_mean_mag', 'bp_rp',
                 'teff_val', 'r_est']
    gaia = io.ascii.read(os.path.join('gaia', 'gaiadr2_hip-result.csv'),
                         include_names=col_names,
                         format='csv')
    gaia.rename_column('hip_id', 'HIP')

    gaia['ra'].unit = u.deg
    gaia['dec'].unit = u.deg
    gaia['phot_g_mean_mag'].unit = u.mag
    gaia['bp_rp'].unit = u.mag
    gaia['teff_val'].unit = u.K
    gaia['r_est'].unit = u.pc

    return gaia

def load_xhip():
    """Load the XHIP catalogue from the VizieR archive."""
    print('Loading XHIP')
    with tarfile.open(os.path.join('vizier', 'xhip.tar.gz'), 'r:gz') as tf:
        print('  Loading main catalog')
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['HIP', 'Comp', 'RAdeg', 'DEdeg', 'Plx', 'pmRA', 'pmDE',
                         'e_Plx', 'Dist', 'e_Dist', 'SpType', 'RV']
            fill_values = [
                ('', '-1', 'Tc', 'Lc'),
                ('', 'NaN', 'phi')
            ]
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names,
                                         # workaround for bug in astropy where nullability is
                                         # ignored for columns with limits, see
                                         # https://github.com/astropy/astropy/issues/9291
                                         fill_values=fill_values)
            reader.data.table_name = 'main.dat'
            with tf.extractfile('./main.dat.gz') as gzf, gzip.open(gzf, 'rb') as f:
                # Suppress a warning generated because the reader does not handle logarithmic units
                with warnings.catch_warnings():
                    warnings.simplefilter("ignore", UnitsWarning)
                    hip_data = reader.read(f)

        hip_data.add_index('HIP')

        print('  Loading photometric data')
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['HIP', 'Vmag', 'Jmag', 'Hmag', 'Kmag', 'e_Jmag',
                         'e_Hmag', 'e_Kmag', 'B-V', 'V-I', 'e_B-V', 'e_V-I']
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names)
            reader.data.table_name = 'photo.dat'
            with tf.extractfile('./photo.dat.gz') as gzf, gzip.open(gzf, 'rb') as f:
                photo_data = reader.read(f)

        photo_data['HIP'].unit = None # for some reason it is set to parsecs in the ReadMe
        photo_data.add_index('HIP')
        hip_data = join(hip_data, photo_data, join_type='left', keys='HIP')

        print('  Loading bibliographic data')
        with tf.extractfile('./ReadMe') as readme:
            col_names = ['HIP', 'HD']
            reader = io.ascii.get_reader(io.ascii.Cds,
                                         readme=readme,
                                         include_names=col_names)
            reader.data.table_name = 'biblio.dat'
            with tf.extractfile('./biblio.dat.gz') as gzf, gzip.open(gzf, 'rb') as f:
                biblio_data = reader.read(f)

        biblio_data.add_index('HIP')

        return join(hip_data, biblio_data, join_type='left', keys='HIP')

def load_sao():
    """Load the SAO-HIP cross match."""
    print('Loading SAO-HIP cross match')
    data = io.ascii.read(os.path.join('xmatch', 'sao_hip_xmatch.csv'),
                         include_names=['HIP', 'SAO', 'angDist', 'delFlag'],
                         format='csv')

    data = data[data['delFlag'].mask]
    data.remove_column('delFlag')

    data = unique(data.group_by(['HIP', 'angDist']), keys=['HIP'])
    data.remove_column('angDist')

    data.add_index('HIP')
    return data

def compute_distances(hip_data, length_kpc=1.35):
    """Compute the distance using an exponentially-decreasing prior.

    The method is described in:

    Bailer-Jones (2015) "Estimating Distances from Parallaxes"
    https://ui.adsabs.harvard.edu/abs/2015PASP..127..994B/abstract

    Using a uniform length scale of 1.35 kpc as suggested for the TGAS
    catalogue of stars in Gaia DR1.

    Astraatmadja & Bailer-Jones "Estimating distances from parallaxes.
    III. Distances of two million stars in the Gaia DR1 catalogue"
    https://ui.adsabs.harvard.edu/abs/2016ApJ...833..119A/abstract
    """

    print('Computing distances')

    eplx2 = hip_data['e_Plx'] ** 2

    r3coeff = np.full_like(hip_data['Plx'], 1/length_kpc)
    r2coeff = np.full_like(hip_data['Plx'], -2)

    roots = np.apply_along_axis(np.roots,
                                0,
                                [r3coeff, r2coeff, hip_data['Plx'] / eplx2, -1 / eplx2])
    roots[np.logical_or(np.real(roots) < 0.0, abs(np.imag(roots)) > 1.0e-6)] = np.nan
    parallax_distance = np.nanmin(np.real(roots), 0) * 1000

    # prefer cluster distances (e_Dist NULL), otherwise use computed distance
    is_cluster_distance = np.logical_and(np.logical_not(hip_data['Dist'].mask),
                                         hip_data['e_Dist'].mask)

    hip_data['r_est'] = np.where(is_cluster_distance, hip_data['Dist'], parallax_distance)
    hip_data['r_est'].unit = u.pc

HIP_TIME = Time('J1991.25')
GAIA_TIME = Time('J2015.5')

def update_coordinates(hip_data):
    """Update the coordinates from J1991.25 to J2015.5 to match Gaia."""
    print('Updating coordinates to J2015.5')
    coords = SkyCoord(frame=ICRS,
                      ra=hip_data['RAdeg'],
                      dec=hip_data['DEdeg'],
                      pm_ra_cosdec=hip_data['pmRA'],
                      pm_dec=hip_data['pmDE'],
                      distance=hip_data['r_est'],
                      radial_velocity=hip_data['RV'].filled(0),
                      obstime=HIP_TIME).apply_space_motion(GAIA_TIME)

    hip_data['ra'] = coords.ra / u.deg
    hip_data['ra'].unit = u.deg
    hip_data['dec'] = coords.dec / u.deg
    hip_data['dec'].unit = u.deg

def process_xhip():
    """Processes the XHIP data."""
    xhip = load_xhip()
    compute_distances(xhip)
    update_coordinates(xhip)
    xhip.remove_columns(['RAdeg', 'DEdeg', 'Plx', 'e_Plx', 'pmRA', 'pmDE', 'RV', 'Dist', 'e_Dist'])
    return xhip

def process_hip():
    """Process the Gaia and HIP data."""
    data = join(load_gaia_hip(),
                process_xhip(),
                keys=['HIP'],
                join_type='outer',
                table_names=['gaia', 'xhip'])

    data = join(data, load_sao(), keys=['HIP'], join_type='left')

    data['dist_use'] = data['r_est_gaia'].filled(data['r_est_xhip'].filled(np.nan))
    data['ra'] = data['ra_gaia'].filled(data['ra_xhip'])
    data['dec'] = data['dec_gaia'].filled(data['dec_xhip'])

    data.remove_columns(['ra_gaia', 'dec_gaia', 'r_est_gaia', 'ra_xhip', 'dec_xhip', 'r_est_xhip'])

    return data
