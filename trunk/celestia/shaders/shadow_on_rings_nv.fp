!!FP1.0

# Eclipse shadow shader--this shader computes a black circle centered
# at texcoord (0.5, 0.5) and multiplies this by texture zero and the
# diffuse color.

# Parameters:
# p[5]  - ambient light color
# p[20] - shadow umbra/penumbra parameters (x = scale, y = bias)

# Compute the square of the distance from the center of the shadow
SUB R0, f[TEX1], { 0.5, 0.5, 0, 0 };
DP3 R1.w, R0, R0;

# Scale and bias the squared distance to get the right shadow umbra
# and penumbra sizes.
MAD_SAT R0.xyz, R1.w, p[20].x, p[20].y;

# Multiply by the diffuse term and add ambient light
MADX_SAT R0.xyz, R0, f[COL0], p[5];

# Mix in the texture color
TEX R1, f[TEX0], TEX0, 2D;
MULX R0, R0, R1;

# Output it
MOVX o[COLR], R0;

END
