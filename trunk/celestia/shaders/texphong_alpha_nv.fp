!!FP1.0

# Diffuse * texture + specular
# Specular term is modulated by a gloss factor stored in the alpha channel
#
# Textures:
# TEX0 - base texture with specular factor in alpha
#
# Interpolants:
# f[TEX0] - base texture coordinates
# f[TEX1] - surface normal in object space
# f[TEX2] - half angle vector in object space
#
# Constants:
# p[0]    - object space light vector
# p[2]    - diffuse color
# p[3]    - specular color
# p[4].x  - specular exponent
# p[5]    - ambient color

MOV R0, f[TEX1]; # surface normal in R0
MOV R1, f[TEX2]; # half angle vector in R1

# Normalize the surface normal (consider using a normalization cube map)
DP3 R0.w, R0, R0;
RSQ R0.w, R0.w;
MUL R0.xyz, R0, R0.w;

# Normalize the half-angle vector
DP3 R1.w, R1, R1;
RSQ R1.w, R1.w;
MUL R1.xyz, R1, R1.w;

# Compute the diffuse term
DP3 R2.x, R0, p[0];

# Compute the specular dot product
DP3 R2.y, R0, R1;

# Specular exponent in w
MOV R2.w, p[4].x;

LIT R3, R2;

TEX H0, f[TEX0], TEX0, 2D;   # Sample the base texture
MULX H2, R3.y, p[2];         # Diffuse color * diffuse factor
ADDX_SAT H2, H2, p[5];       # Add ambient light to get 'scene color'
MULX H0, H2, H0;             # Base texture * scene color
MULX R3.z, R3.z, H0.w;       # Multiply the specular factor by the gloss map value
MULX R3.z, R3.z, R3.y;       # Multiply the specular factor by the diffuse factor
MADX H0, R3.z, p[3], H0;     # Multiply the specular factor by
                             #   the specular color and add it in
MOVX o[COLR], H0;

END
