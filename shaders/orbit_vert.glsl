attribute vec4 in_Position;
attribute vec4 in_Color;
varying vec4 v_color;

void main(void)
{
    float alphaFactor = in_Position.w;
    vec4 pos = vec4(in_Position.xyz, 1.0);
    v_color = vec4(in_Color.rgb, in_Color.a * alphaFactor);
    set_vp(pos);
}
