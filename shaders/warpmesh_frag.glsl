in vec2 texCoord;
in float intensity;

uniform sampler2D tex;

void main(void)
{
    fragColor = vec4(texture(tex, texCoord).rgb * intensity, 1.0);
}
