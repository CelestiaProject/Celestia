Gaia DR2 for celestia.Sci/Celestia
==================================

This repository contains Python scripts to generate a celestia.Sci/Celestia
star database from the *Gaia* Data Release 2 data, supplemented by the XHIP
catalogue.

In order to limit the download size required and to maintain compatibility
with the use of HIP/TYC2 identifiers as the primary key for stars in
celestia.Sci/Celestia, only Gaia data for HIP and TYC2 stars is processed.

**Unless you are editing the code, it is recommended to use the pre-built
files in the release rather than running these scripts manually.** The data
files in the release may be used under a CC-BY-SA 4.0 license
(https://creativecommons.org/licenses/by-sa/4.0/legalcode)

## Prerequisites

-  Internet connection for downloading the data
-  *Gaia* archive account (https://gea.esac.esa.int/archive/)
-  Python 3.6 or higher (preferably 64-bit, as the memory usage can be quite
   high)
-  celestia.Sci/Celestia

## Folder contents

-  `download_data.py`: script to download the data files
-  `make_stardb.py`: script to build the star database and cross-index files

## How to use

Please ensure you have read through the Prerequisites section and all the
steps below **before** you begin.

1.  Clone or download this repository
2.  Open a command window in the repository directory
3.  Set up a Python 3 virtual environment

    `python3 -m venv myenv`

4.  Switch to the virtual environment and install the requirements

    `source myenv/bin/activate`

5.  Install the requirements:

    `python -m pip install -r requirements.txt`

6.  Run the download script. You will need your *Gaia* archive login.
    **This step may take several hours!**

    python download_data.py

7.  Run the build script.

    python make_stardb.py

8.  The stars.dat, hdxindex.dat and saoxindex.dat files will be written into
    the output folder

9.  Copy the files into the `data` folder of the celestia.Sci/Celestia
    distribution.

## Source catalogues

- *Gaia* Data Release 2 (https://gea.esac.esa.int/archive/)
    - *Gaia* Collaboration et al. (2016), A&A 595, id.A1, "The *Gaia* mission"
    - *Gaia* Collaboration et al. (2018), A&A 616, id.A1, "*Gaia* Data
      Release 2. Summary of the contents and survey properties"
    - Andrae et al. (2018), A&A 616, id.A8, "*Gaia* Data Release 2. First
      stellar parameters from Apsis"
    - Marrese et al. (2018), A&A 621, id.A144, "*Gaia* Data Release 2.
      Cross-match with external catalogues: algorithms and results"

- *Gaia* Data Release 2 Geometric Distances
  (http://www.mpia.de/~calj/gdr2_distances/main.html)

  Bailer-Jones et al. (2018), AJ 156(2), id.58 "Estimating Distance from
  Parallaxes. IV. Distances to 1.33 Billion Stars in *Gaia* Data Release 2"

- Extended Hipparcos Compilation (XHIP)
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/V/137D)

  Anderson & Francis (2012), AstL 38(5), pp.331–346 "XHIP: An extended
  Hipparcos compilation"

- ASCC-2.5, 3rd version (http://cdsarc.u-strasbg.fr/viz-bin/cat/I/280B)

  Kharchenko (2001), Kinematika i Fizika Nebesnykh Tel 17(5), pp.409-423
  "All-sky compiled catalogue of 2.5 million stars"

- Teff and metallicities for Tycho-2 stars
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/V/136)

  Ammons et al. (2006), ApJ 638(2), pp.1004–1017 "The N2K Consortium. IV. New
  Temperatures and Metallicities for More than 100,000 FGK Dwarfs"

- The Tycho-2 Spectral Type Catalog
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/III/231)

  Wright et al. (2003), AJ 125(1), pp.359–363 "The Tycho-2 Spectral Type
  Catalog"

- SAO Star Catalog J2000 (http://cdsarc.u-strasbg.fr/viz-bin/cat/I/131A)

  SAO Staff, "Smithsonian Astrophysical Observatory Star Catalog (1990)"

## Additional catalogues of interest

- The Hipparcos and Tycho Catalogues (1997)
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/I/239)
  - Perryman et al. (1997), A&A 500 pp.501–504 "The Hipparcos Catalogue"
  - Høg et al. (1997), A&A 323 pp.L57–L60 "The TYCHO Catalogue"

- Hipparcos, the New Reduction (http://cdsarc.u-strasbg.fr/viz-bin/cat/I/311)

  van Leeuwen (2007), A&A 474(2) pp.653–664 "Validation of the new Hipparcos
  reduction"

- The Tycho-2 Catalogue (http://cdsarc.u-strasbg.fr/viz-bin/cat/I/259)

  Høg et al. (2000), A&A 355 pp.L27–L30 "The Tycho-2 catalogue of the 2.5
  million brightest stars"

- Henry Draper Catalogue and Extension
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/III/135A)

  Cannon & Pickering (1918–1924), Annals of the Astronomical Observatory of
  Harvard College

## References used for data processing

- UBVRIJHK color-temperature calibration
  (http://cdsarc.u-strasbg.fr/viz-bin/cat/J/ApJS/193/1)

  Worthey & Lee (2011), ApJS, 193(1), id.1 "An Empirical UBV RI JHK
  Color-Temperature Calibration for Stars"

- Bailer-Jones (2015), PASP 127(956), pp.994 "Estimating Distances from
  Parallaxes"

- Astraatmadja & Bailer-Jones (2016), ApJ 833(1), id.119 "Estimating
  Distances from Parallaxes. III. Distances of Two Million Stars in the
  *Gaia* DR1 Catalogue"

## Acknowledgements

This work has made use of data from the European Space Agency (ESA) mission
*Gaia* (https://www.cosmos.esa.int/gaia), processed by the *Gaia* Data
Processing and Analysis Consortium (DPAC,
https://www.cosmos.esa.int/web/gaia/dpac/consortium). Funding for the DPAC has
been provided by national institutions, in particular the institutions
participating in the *Gaia* Multilateral Agreement.

This work made use of the cross-match service provided by CDS, Strasbourg.
