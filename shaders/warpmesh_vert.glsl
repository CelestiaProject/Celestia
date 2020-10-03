attribute vec2 in_Position;
attribute vec2 in_TexCoord0;
attribute float in_Intensity;

varying vec2 texCoord;
varying float intensity;

uniform float screenRatio;

void main(void)
{
    float offset = 0.5 - screenRatio * 0.5;
    gl_Position = vec4(in_Position.x * screenRatio, in_Position.y, 0.0, 1.0);
    texCoord = vec2(in_TexCoord0.x * screenRatio + offset, in_TexCoord0.y);
    intensity = in_Intensity;
}
