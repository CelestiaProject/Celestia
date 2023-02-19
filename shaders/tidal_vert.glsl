attribute vec3 in_Position;
attribute vec3 in_TexCoord0; // reuse [3] as colorIndex

uniform sampler2D colorTex;
uniform mat3 viewMat;
uniform float tidalSize;
uniform float brightness;
uniform float pixelWeight;

varying vec2 texCoord;
varying vec4 color;

void main(void)
{
    vec3 p = viewMat * in_Position.xyz * tidalSize;
    texCoord = in_TexCoord0.st;
    float colorIndex = in_TexCoord0.p;
    color = vec4(texture2D(colorTex, vec2(colorIndex, 0.0)).rgb, min(1.0, 2.0 * brightness * pixelWeight));
    set_vp(vec4(p, 1.0));
}
