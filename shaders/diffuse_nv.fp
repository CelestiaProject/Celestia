!!FP1.0

# Simple diffuse * texture shader

TEX R1, f[TEX0], TEX0, 2D;
MULX R0, R1, f[COL0];

MOVX o[COLR], R0;

END
