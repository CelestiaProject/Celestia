3DSCONV:

The 3dsconv utility converts a 3D Studio mesh to an ASCII format CMOD model.
The command line usage is very simple:

3dsconv <input 3ds file>

The cmod file is written to standard output, so you must use output redirection
to send it to a file.  The standard usage is something like this:

3dsconv in.3ds > out.cmod

The cmod files produced by 3dsconv are unoptimized and will benefit from
further processing with the cmodfix tool.



CMODFIX:

Cmodfix is a command line utility for performing some basic manipulations of
cmod 3D model files.  Various switches specify what operations are to be
performed on the cmod file.  The operations are always applied in a fixed
order, so the order of the switches is irrelevant.

cmodfix [options] [input cmod file [output cmod file]]
   --binary (or -b)      : output a binary .cmod file
   --ascii (or -a)       : output an ASCII .cmod file
   --uniquify (or -u)    : eliminate duplicate vertices
   --tangents (or -t)    : generate tangents
   --normals (or -n)     : generate normals
   --smooth (or -s) <angle> : smoothing angle for normal generation
   --weld (or -w)        : join identical vertices before normal generation
   --merge (or -m)       : merge submeshes to improve rendering performance
   --optimize (or -o)    : optimize by converting triangle lists to strips


The order in which the operations are applied is as follows:

   1. Weld vertices
   2. Generate normals
   3. Generate tangents
   4. Merge meshes
   5. Uniquify (eliminate duplicate vertices)
   6. Optimize triangle lists to strips
   7. Write output mesh


Weld vertices
This operation is relevant only when generating new normals or tangents for
a model.  It causes vertices with identical positions to be treated as
identical for normal/tangent generation even if they have other attributes
that differ.  It's most commonly used to eliminate ugly creases from models
that have cylindrically wrapping texture coordinates.  Although it may slightly
increase the time required to modify a cmod file, it's usually desirable to
enable vertex welding.

Generate normals
Compute new normals for a model by averaging face normals.  Faces are only
averaged when the angle between their normals is less than the smoothing angle.
The default smoothing angle of 60 degrees is reasonable for many models.  To
force faces to always be averaged, use a smoothing angle of 180 degrees.  This
will make the object appear curved and smooth everywhere.  A smoothing angle
of zero degrees results in an object that appears faceted, with a hard edge
between every pair of faces.

Generate tangents
Tangents are required in order to apply bump maps or other per-pixel lighting
effects to a cmod.  The tangent vectors are generated from the vertex
normals and texture coordinates--both must be present in order to create
tangents.

Merge meshes
This operation will pool compatible submeshes into a single large mesh.
Reducing the number of meshes can improve rendering performance, but it usually
only helps when there are a lot of very small meshes in the original cmod
file.

Uniquify vertices
Uniquify collapses identical vertices into a single vertex, reducing the size
of the cmod file and making it render faster (though the improvement won't
always be noticeable.)  It's almost always a good idea to uniquify model
vertices, and especially so when the input mesh is derived from unindexed
data such as the output of 3dstocmod.

Optimize triangle lists to strips
This option is only available when cmodfix has be built with NVIDIA's
NvTriStrip library (http://developer.nvidia.com/object/nvtristrip_library.html)
It converts triangle lists in a cmod file into much more efficient triangle
lists.  Converting lists to strips often has a profound impact on rendering
performance.  Calculating optimal triangle lists can be a slow process for
large models: it could take a minute to process a one million triangle model.

Output
CMOD files can be stored in either an ASCII or binary format.  The binary
format is more compact and loads more quickly but is not human readable.
The ASCII format is useful if for some reason you need to hand-modify the
model.  Otherwise, the binary format is prefered.


TYPICAL EXAMPLES:

Generate normals with a 45 degree smoothing angle:
cmodfix -u -w -n -s 45 in.cmod out.cmod

Convert a binary cmod file to ASCII:
cmodfix -a in.cmod out.cmod

Generate tangents:
cmodfix -u -w -n -t in.cmod out.cmod

Optimize a mesh:
cmodfix -u -o in.cmod out.cmod


BUGS:

The NvTriStrip library only handles 16-bit vertex indices, so submeshes with
more than 65536 vertices will be skipped when optimizing.

Binary file output does not work with redirection on Windows machines.  To
write a binary file on Windows, you must specify the output file name.
For example, instead of this:
   cmodfix -b in.cmod > out.cmod
Write this:
   cmodfix -b in.cmod out.cmod



LICENSE:

The 3dstocmod and cmodfix programs are free software.  You can redistribute
and modify them under the terms of the GNU General Public License, the full
text of which is in the COPYING file.  The source code for the programs is
available for free from the Celestia project on SourceForge:
http://www.sourceforge.net/projects/celestia/

cmodfix may be optionally compiled with NVIDIA's NvTriStrip library, available
in source form from NVIDIA's developer web site:
http://developer.nvidia.com/object/nvtristrip_library.html
NvTriStrip is /not/ covered by the same license as the cmodtools.



CREDITS:

The cmod tools were developed by Chris Laurel as part of the Celestia project.
Send bug reports and suggesestion to claurel@shatters.net.

