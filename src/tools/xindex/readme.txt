BUILDXINDICES.PL

This script builds the HD and SAO cross-indices (hdxindex.dat and
saoxindex.dat). It takes as its input a file containing a colon-separated list
of star designations called crossids.txt. Each star's designations should be on
one line. This file can be created using script queries to the SIMBAD database
(SIMBATCH, at http://simbad.u-strasbg.fr/simbad/sim-fscript).

Because of the limited number of output rows, the crossids.txt file must be
assembled from the results of several SIMBATCH queries. It is probably best to
use a query size of 20000. For example, to return all stars with HIP IDs
between 20000 and 39999, the following SIMBATCH script may be used:

format object "%IDLIST[%*(S):]"
set limit 20000
query id wildcard HIP [23]????

The header of each returned file should be discarded. The resultant files can
then be concatenated to produce the crossids.txt file.