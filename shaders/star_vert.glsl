attribute vec3 in_Position;
attribute vec4 in_Color;
attribute float in_PointSize;
varying vec4 color;

void main(void)
{
    gl_PointSize = in_PointSize;
    color = in_Color;
    set_vp(vec4(in_Position, 1.0));
}
