!!FP1.0

# Eclipse shadow shader--this shader computes a black circle centered
# at texcoord (0.5, 0.5) and multiplies this by texture zero and the
# diffuse color.

# Parameters:
# p[5]  - ambient light color
# p[20] - shadow umbra/penumbra parameters (x = scale, y = bias)
# p[21] - shadow umbra/penumbra parameters (x = scale, y = bias)

SUB R0, f[TEX0], { 0.5, 0.5, 0, 0 };
DP3 R1.x, R0, R0;
MAD_SAT R1.x, R1.x, p[20].x, p[20].y;
#POW R1.x, R1.x, 0.5;
RSQ R1.x, R1.x;
RCP R1.x, R1.x;

SUB R0, f[TEX1], { 0.5, 0.5, 0, 0 };
DP3 R1.y, R0, R0;
MAD_SAT R1.y, R1.y, p[21].x, p[21].y;
#POW R1.y, R1.y, 0.5;
RSQ R1.y, R1.y;
RCP R1.y, R1.y;

MINX R0.xyz, R1.x, R1.y;

# Add the ambient light
ADDX_SAT R0.xyz, R0, p[5];

# Output it
MOVX o[COLR], R0;

END
