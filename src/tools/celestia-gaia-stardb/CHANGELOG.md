# Changelog

## v1.0.4

* Prefer XHIP distances where the relative parallax error is smaller than the
  Gaia value.
* Fix Pylint warnings.
* Refactor common parsing functionality.

## v1.0.3.1

* Update reference list for v1.0.3 changes.

## v1.0.3

* Use Tycho-HD cross index (VizieR IV/25) to provide HD identifications for
  TYC stars instead of ASCC.
* Preferentially use the HD identifications in the SAO catalogue as the basis
  for the SAO cross-index.
* Merge missing data when combining HIP and TYC subsets

## v1.0.2

* Fix an issue where missing spectral types were being converted to O.

## v1.0.1

* Remove duplicate HIP/TYC entries matched to the same Gaia DR2 ID.
* Use ASCC spectral types when Tycho-2 spectral type catalogue does not
  contain an entry.
* Use New spectral types for Tycho-2 stars to correct some XHIP spectral
  types.
* Cross-index SAO with stars in Tycho-2 supplements.

## v1.0.0

* More comprehensive HIP and TYC cross-matching (see README for references).
* Various code improvements.

## v0.1.2-alpha

* Fixes for duplicates in star database and cross-index files.

## v0.1.1-alpha

* Initial release with license files included

## v0.1.0-alpha

* Initial release
