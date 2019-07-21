#version 120

uniform mat3 viewMat;
uniform float tidalSize;
varying vec2 texCoord;


void main(void)
{
    vec3 p = viewMat * gl_Vertex.xyz * tidalSize;
    gl_Position = gl_ModelViewProjectionMatrix * vec4(p, 1.0f);

    texCoord = gl_MultiTexCoord0.st;
}
