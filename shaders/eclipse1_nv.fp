!!FP1.0

# Eclipse shadow shader--this shader computes a black circle centered
# at texcoord (0.5, 0.5) and multiplies this by texture zero and the
# diffuse color.

# Parameters:
# p[5]  - ambient light color
# p[20] - shadow umbra/penumbra parameters (x = scale, y = bias)

# Compute the square of the distance from the center of the shadow
SUB R0, f[TEX0], { 0.5, 0.5, 0, 0 };
DP3 R1.w, R0, R0;

# Scale and bias the squared distance to get the right shadow umbra
# and penumbra sizes.
MAD_SAT R1.x, R1.w, p[20].x, p[20].y;

#MUL R0.xyz, R1.x, 2.0;
POW R0.xyz, R1.x, 0.5;
#TEX R0.xyz, R1.x, TEX0, 2D;

# Add the ambient light
ADDX_SAT R0.xyz, R0, p[5];

# Output it
MOVX o[COLR], R0;

END
