#version 120

attribute vec4 in_Position;
uniform mat4 MVPMatrix;

void main(void)
{
    gl_Position = MVPMatrix * in_Position;
}
