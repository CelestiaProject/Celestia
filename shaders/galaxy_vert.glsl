attribute vec4 in_Position;
attribute vec4 in_TexCoord0;

uniform sampler2D colorTex;

varying vec4 color;
varying vec2 texCoord;

void main(void)
{
    // we pass color index as a normalized byte reusing in_TexCoord0.z
    float t = in_TexCoord0.z;
    // we pass alpha values as a normalized byte reusing in_TexCoord0.w
    float a = in_TexCoord0.w;
    color = vec4(texture2D(colorTex, vec2(t, 0.0)).rgb, a);
    texCoord = in_TexCoord0.st;
    set_vp(in_Position);
}
