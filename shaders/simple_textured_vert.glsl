#version 110

varying vec2 texCoord;

void main(void)
{
    gl_Position = ftransform();
    texCoord = gl_MultiTexCoord0.st;
}
