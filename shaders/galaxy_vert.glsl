#version 120

attribute vec4 in_Position;
attribute vec4 in_TexCoord0;
//attribute float in_ColorIndex;
//attribute float in_Alpha;

uniform sampler2D colorTex;

varying vec4 color;
varying vec2 texCoord;

void main(void)
{
    // we pass color index as short int
    // reusing gl_MultiTexCoord0.z
    // we use 255 only because we have 256 color indices
    float t = in_TexCoord0.z / 255.0f; // [0, 255] -> [0, 1]
    // we pass alpha values as as short int
    // reusing gl_MultiTexCoord0.w
    // we use 65535 for better precision
    float a = in_TexCoord0.w / 65535.0f; // [0, 65535] -> [0, 1]
    color = vec4(texture2D(colorTex, vec2(t, 0.0f)).rgb, a);
    texCoord = in_TexCoord0.st;
    gl_Position = gl_ModelViewProjectionMatrix * in_Position;
}
