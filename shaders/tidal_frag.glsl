uniform sampler2D tidalTex;

in vec2 texCoord;
in vec4 color;

void main(void)
{
    fragColor = vec4(color.rgb, color.a * texture(tidalTex, texCoord).r);
}
