uniform sampler2D starTex;

in vec2 texCoord;
in vec4 v_color;

void main(void)
{
    fragColor = texture(starTex, texCoord) * v_color;
}
