#version 120

attribute vec3 in_Position;

uniform mat3 viewMat;
uniform float tidalSize;
varying vec2 texCoord;


void main(void)
{
    vec3 p = viewMat * in_Position.xyz * tidalSize;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0f);

    texCoord = gl_MultiTexCoord0.st;
}
