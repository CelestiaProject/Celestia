spice2xyzv

Author: Chris Laurel (claurel@gmail.com)


Spice2xyzv is a command line tool that is used to produce Celestia xyzv files
from a set of SPICE SPK kernels. An xyzv file is simply an ASCII file with
position and velocity states at unequal time steps. Each record in the file
consists of seven double precision values, arranged as such:

<Julian date (TDB)> <x> <y> <z> <vx> <vy> <vz>

X, y, and z are components of the position vector; vx, vy, and vz are the
velocity vector components.


Why?
----

Celestia supports using SPICE SPK kernels directly, and when high accuracy is
required SPICE kernels are the preferred ephemeris source. But the large 
amount of data required to represent an entire mission with SPK kernels can
make this approach less attractive. This is the case for the spacecraft
trajectories in the default Celestia package. Spice2xyzv offers a tolerance
parameter that lets you pick the optimal balance between size and accuracy for
the situation. Spice2xyzv can thus be thought of as a sort of lossy
compression tool for SPK files.


Usage
-----

The spice2xyzv command line is extremely simple:

spice2xyzv <config file>

The xyzv file is written to standard output, thus you'll generally use the
tool with output redirection, e.g:

spice2xyzv cassini-cruise.cfg > cruise.xyzv

The configuration file is a text file with a list of named parameters. These
parameters have either string, numeric, or string list values. Some of the
parameters have defaults and can be omitted from the file. The order in which
the parameters appear is not important. The following example is used to
produce an xyzv file for the Huygens probe:


StartDate   "2004-Dec-25 02:01:04.183 TDB"
EndDate     "2005-Jan-14 09:07:00.000 TDB"
Observer    "SATURN"
Target      "CASP"
Frame       "eclipJ2000"
Tolerance   0.1
MinStep     60
MaxStep     86400
KernelDirectory "c:/dev/cassini/new"
Kernels [
    "de421.bsp"
    "sat252s.bsp"
    "041210AP_OPK_04329_08189.bsp" 
]


Configuration Parameters
------------------------

StartDate and EndDate (string)
These parameters specify the time range for the xyzv file. The set of SPK
kernels used must provide coverage for the target object (and any necessary
frame centers) for any time between start and end. The date strings are
parsed by SPICE, which accepts a wide range of different formats. By default,
all times are UTC, but the TDB or TDT suffix can be appended (as in the
example) to specify a different time system.

Observer (string)
The name or NAIF integer ID of the coordinate center. The choice of
coordinate center depends on how the resulting xyzv file will be used: it
must match the frame center chosen for the SSC file.

Target (string)
The name or NAIF integer ID of the object that you're generating a trajectory
for.

Frame (string)
The name of the reference frame for the xyzv coordinates. Two common frames
are:
"J2000" - Earth mean equator and equinox of J2000
"eclipJ2000" - Earth ecliptic and equinox of J2000
If the frame is not one of SPICE's built-in frames, it is necessary to
add a frame kernel to the Kernels list in the configuration file.

Tolerance (number)
Maximum permitted error in units of kilometers. Making this value smaller will
result in a larger xyzv file.

MinStep and MaxStep (number)
The minimum and maximum time interval (in seconds) between states. The
default values are 60 for MinStep and 432000 (5 days) for MaxStep.

KernelDirectory (string)
Directory containing the SPICE kernel files. The default value is ".",
indicating that the kernel files reside in the current directory.

Kernels (string list)
List of kernel files that need to be loaded to generate the xyzv file. This
list may include more than just SPK files. For instance, a frame kernel file
may be necessary to get the position of an object in the desired frame.



Method
------

Spice2xyzv uses a very simple method to choose sample times for xyzv states.
It calls SPICE to generate a state at a base time t0. It then generates two
more states: one at t0+dt/2 and one at t0+dt. Next, the position at t0+dt/2
is compared to the result of cubic Hermite interpolation of the SPICE
computed positions at t0 and t0+dt. If the distance is within the tolerance
specified in the configuration file, the test is repeated with dt*2. This
continues until either MaxStep is reached or the interpolated and SPICE
calculated positions are further than Tolerance kilometers apart. The last
value of dt for which the interpolated position was close enough the the
SPICE calculated position is used as the time step, t0 is incremented by
dt, and the process is repeated over the entire time span. This adaptive
sampling results in a low number of samples in slowly varying parts of the
trajectory and more samples at times when the trajectory changes more
dramatically.

