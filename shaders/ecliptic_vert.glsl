attribute vec2 in_Position;

void main(void)
{
    vec4 p = vec4(in_Position.x, 0.0, in_Position.y, 1.0);
    set_vp(p);
}
