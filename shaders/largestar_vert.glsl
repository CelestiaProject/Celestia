// Batched textured-billboard fallback for point-sprite stars whose
// gl_PointSize would exceed the driver's GL_ALIASED_POINT_SIZE_RANGE.
// Six vertices per star; per-star fields are replicated.

layout(location = 0) in vec2  in_Position;   // quad corner in [-1, 1]
layout(location = 1) in vec3  in_Normal;     // star world position
layout(location = 2) in vec2  in_TexCoord0;  // quad UV in [0, 1]
layout(location = 8) in vec4  in_Color;
layout(location = 9) in float in_Intensity;  // size in physical pixels (full diameter)

uniform vec2 viewportRcp;   // (1/width, 1/height)

out vec2 texCoord;
out vec4 v_color;

void main(void)
{
    texCoord = in_TexCoord0;
    v_color  = in_Color;

    set_vp(vec4(in_Normal, 1.0));

    vec2 extent = in_Intensity * viewportRcp;
    gl_Position.xy += in_Position * extent * gl_Position.w;
}
