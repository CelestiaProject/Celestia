#version 120

uniform sampler2D colorTex;

varying vec4 color;
varying vec2 texCoord;

void main(void)
{
    // we pass color index as short int
    // reusing gl_MultiTexCoord0.z
    // we use 255 only because we have 256 color indices
    float t = gl_MultiTexCoord0.z / 255.0f; // [0, 255] -> [0, 1]
    // we pass alpha values as as short int
    // reusing gl_MultiTexCoord0.w
    // we use 65535 for better precision
    float a = gl_MultiTexCoord0.w / 65535.0f; // [0, 65535] -> [0, 1]
    color = vec4(texture2D(colorTex, vec2(t, 0.0f)).rgb, a);
    texCoord = gl_MultiTexCoord0.st;
    gl_Position = ftransform();
}
