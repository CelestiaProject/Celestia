STARTEXTDUMP:

The startextdump program converts a binary Celestia star database to an
easy to edit ASCII format.  The command line is:

startextdump [options] [<input file> [<output file>]]

If the output file is not provided, the ASCII star catalog is written to the
standard output stream.  The options are:

  --old (or -o)
  The input file is an old (Celestia 1.3.2 or earlier) format star database
  file.

  --spherical (or -s)
  Output spherical coordinates (RA/declination/distance) instead of the usual
  rectangular coordinates.  RA is in degrees instead of the more conventional
  hours/minutes/seconds.  Distance is in units of light years.  Also, the
  apparent magnitude will be written instead of the absolute magnitude.
  The spherical option is ignored unless the input is an old format star
  database.

  --hd <filename>
  Dump the HD catalog numbers of an old format star database to a separate
  file.  Each line of the output file has an HD catalog number followed by
  the corresponding HIPPARCOS number.  The file can be converted to a
  Celestia cross index with the makexindex tool.



MAKESTARDB:

Makestardb converts an ASCII star database created by startextdump to a
binary star database readable by Celestia--it is the reverse of the
startextdump.  Makestardb does not support the old star database format;
the output files it produces are only usable with versions of Celestia
newer than 1.3.2.

The command line is:

makestardb [--spherical] [<input file> [<output file>]]

If an input or output file isn't provided, the standard input or output stream
is used.  The --spherical option will cause makestardb to convert the input
positions from spherical to rectangular coordinates, and to convert the
magnitude from apparent to absolute.  Use --spherical for ASCII star files
generated when startextdump is run with its own --spherical option.



MAKEXINDEX:

A cross index file maps numbers from a star catalog to Celestia catalog
numbers.  Makeindex converts ASCII files containing pairs of catalog numbers
into binary cross index files.  The command line is:

makexindex [<input file> [<output file>]]

Star catalog numbers in the input file must be positive integers less than
2^32 - 1.







  