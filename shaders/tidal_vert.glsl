attribute vec3 in_Position;
attribute vec2 in_TexCoord0;

uniform mat3 viewMat;
uniform float tidalSize;
varying vec2 texCoord;


void main(void)
{
    vec3 p = viewMat * in_Position.xyz * tidalSize;
    texCoord = in_TexCoord0.st;
    set_vp(vec4(p, 1.0));
}
