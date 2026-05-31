uniform sampler2D starTex;
in vec4 color;

void main(void)
{
    fragColor = vec4(color.rgb, color.a * texture(starTex, gl_PointCoord.xy).r);
}
