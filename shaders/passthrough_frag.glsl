in vec2 texCoord;

uniform sampler2D tex;

void main(void)
{
    fragColor = texture(tex, texCoord);
}
