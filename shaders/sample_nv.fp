!!FP1.0

# attributes
# f[WPOS]       Position of the fragment center.     (x,y,z,1/w)
# f[COL0]       Interpolated primary color           (r,g,b,a)
# f[COL1]       Interpolated secondary color         (r,g,b,a)
# f[FOGC]       Interpolated fog distance/coord      (z,0,0,0)
# f[TEX0]       Texture coordinate (unit 0)          (s,t,r,q)
# f[TEX1]       Texture coordinate (unit 1)          (s,t,r,q)
# f[TEX2]       Texture coordinate (unit 2)          (s,t,r,q)
# f[TEX3]       Texture coordinate (unit 3)          (s,t,r,q)
# f[TEX4]       Texture coordinate (unit 4)          (s,t,r,q)
# f[TEX5]       Texture coordinate (unit 5)          (s,t,r,q)
# f[TEX6]       Texture coordinate (unit 6)          (s,t,r,q)
# f[TEX7]       Texture coordinate (unit 7)          (s,t,r,q)

# temp registers
# R0-R31          Four 32-bit (fp32) floating point values (s.e8.m23)
# H0-H63          Four 16-bit (fp16) floating point values (s.e5.m10)

# output registers
# o[COLR]         Final RGBA fragment color, fp32 format
# o[COLH]         Final RGBA fragment color, fp16 format
# o[DEPR]         Final fragment depth value, fp32 format

# named constants
# DEFINE pi = 3.1415926535;
# DEFINE color = {0.2, 0.5, 0.8, 1.0};

# named local parameters
# DECLARE fog_color1;
# DECLARE fog_color2 = {0.3, 0.6, 0.9, 0.1};

# numbered parameters
# A numbered local parameter is accessed by a fragment program as members of
# an array called "p".  For example, the instruction
# MOV R0, p[31];
# copies the contents of numbered local parameter 31 into temporary register R0

# Instruction summary
#   ADD[RHX][C][_SAT]    v,v     v        add
#   COS[RH ][C][_SAT]    s       ssss     cosine
#   DDX[RH ][C][_SAT]    v       v        derivative relative to x
#   DDY[RH ][C][_SAT]    v       v        derivative relative to y
#   DP3[RHX][C][_SAT]    v,v     ssss     3-component dot product
#   DP4[RHX][C][_SAT]    v,v     ssss     4-component dot product
#   DST[RH ][C][_SAT]    v,v     v        distance vector
#   EX2[RH ][C][_SAT]    s       ssss     exponential base 2
#   FLR[RHX][C][_SAT]    v       v        floor
#   FRC[RHX][C][_SAT]    v       v        fraction
#   KIL                  none    none     conditionally discard fragment
#   LG2[RH ][C][_SAT]    s       ssss     logarithm base 2
#   LIT[RH ][C][_SAT]    v       v        compute light coefficients
#   LRP[RHX][C][_SAT]    v,v,v   v        linear interpolation
#   MAD[RHX][C][_SAT]    v,v,v   v        multiply and add
#   MAX[RHX][C][_SAT]    v,v     v        maximum
#   MIN[RHX][C][_SAT]    v,v     v        minimum
#   MOV[RHX][C][_SAT]    v       v        move
#   MUL[RHX][C][_SAT]    v,v     v        multiply
#   PK2H                 v       ssss     pack two 16-bit floats
#   PK2US                v       ssss     pack two unsigned 16-bit scalars
#   PK4B                 v       ssss     pack four signed 8-bit scalars
#   PK4UB                v       ssss     pack four unsigned 8-bit scalars
#   POW[RH ][C][_SAT]    s,s     ssss     exponentiation (x^y)
#   RCP[RH ][C][_SAT]    s       ssss     reciprocal
#   RFL[RH ][C][_SAT]    v,v     v        reflection vector
#   RSQ[RH ][C][_SAT]    s       ssss     reciprocal square root
#   SEQ[RHX][C][_SAT]    v,v     v        set on equal
#   SFL[RHX][C][_SAT]    v,v     v        set on false
#   SGE[RHX][C][_SAT]    v,v     v        set on greater than or equal
#   SGT[RHX][C][_SAT]    v,v     v        set on greater than
#   SIN[RH ][C][_SAT]    s       ssss     sine
#   SLE[RHX][C][_SAT]    v,v     v        set on less than or equal
#   SLT[RHX][C][_SAT]    v,v     v        set on less than
#   SNE[RHX][C][_SAT]    v,v     v        set on not equal
#   STR[RHX][C][_SAT]    v,v     v        set on true
#   SUB[RHX][C][_SAT]    v,v     v        subtract
#   TEX[C][_SAT]         v       v        texture lookup
#   TXD[C][_SAT]         v,v,v   v        texture lookup w/partials
#   TXP[C][_SAT]         v       v        projective texture lookup
#   UP2H[C][_SAT]        s       v        unpack two 16-bit floats
#   UP2US[C][_SAT]       s       v        unpack two unsigned 16-bit scalars
#   UP4B[C][_SAT]        s       v        unpack four signed 8-bit scalars
#   UP4UB[C][_SAT]       s       v        unpack four unsigned 8-bit scalars
#   X2D[RH ][C][_SAT]    v,v,v   v        2D coordinate transformation

