layout(location = 0) in vec4 in_Position;
layout(location = 2) in vec2 in_TexCoord0;
in float in_Size;
in float in_ColorIndex;
in float in_Brightness;

uniform sampler2D colorTex;

uniform mat4 m;
uniform mat3 viewMat;

uniform float size;
uniform float brightness;

out vec4 color;
out vec2 texCoord;

void main(void)
{
    float s = size * in_Size;
    vec4 p = m * in_Position;
    float screenFrac = s / length(p);
    if (screenFrac >= 0.1)
    {
        // Output the same value for all discarded vertices so GPU will discard degenerate triangles
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec4 v = vec4(viewMat * vec3(in_TexCoord0.s * 2.0 - 1.0, in_TexCoord0.t * 2.0 - 1.0, 0.0) * s, 0.0);
    float alpha = (0.1 - screenFrac) * in_Brightness * brightness;

    color = vec4(texture(colorTex, vec2(in_ColorIndex, 0.0)).rgb, alpha);
    texCoord = in_TexCoord0;
    set_vp(p + v);
}
