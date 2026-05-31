uniform sampler2D starTex;
uniform vec4 color;

in vec2 texCoord;

void main(void)
{
    fragColor = texture(starTex, texCoord) * color;
}
