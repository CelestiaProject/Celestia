uniform sampler2D starTex;
in vec4 color;

void main(void)
{
    fragColor = texture(starTex, gl_PointCoord) * color;
}
