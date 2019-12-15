#version 120

varying vec2 texCoord;
varying vec4 color;

void main(void)
{
    gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xy, 0, 1);
    texCoord = gl_MultiTexCoord0.st;
    color = gl_Color;
}
