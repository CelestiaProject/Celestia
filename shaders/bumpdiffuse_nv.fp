!!FP1.0

# Diffuse bump map + texture
# Base texture in tex 0
# Normal map in tex 1

# Ambient color in constant 5

# Get the base texture color
TEX H0, f[TEX0], TEX0, 2D;

# Get the normal map value
TEX H1, f[TEX1], TEX1, 2D;

# Unpack the unsigned texture components into a normal
MADX H1, H1, 2.0, -1.0;

# The diffuse color contains the tangent space normal; unpack it into H2
MADX H2, f[COL0], 2.0, -1.0;

# Compute the diffuse term
DP3X H3.w, H1, H2;

# Compute the self shadowing term--multiplying by a factor of 8 gives
# a steep ramp function
MULH_SAT H1.w, H2.z, 8.0;

# Add the product of the diffuse and self shadowing terms to the
# ambient color.
MADX H3.xyz, H1.w, H3.w, p[5];

MULX H0.xyz, H3, H0;

MOVX o[COLR], H0;

END

