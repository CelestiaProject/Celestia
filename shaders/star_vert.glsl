#version 120
attribute float pointSize;
varying vec4 color;

void main(void)
{
    gl_PointSize = pointSize;
    color = gl_Color;
    gl_Position = ftransform();
}
