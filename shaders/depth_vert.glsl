#version 120

uniform mat4 MVPMatrix;

void main(void)
{
    gl_Position = MVPMatrix * gl_Vertex;
}
