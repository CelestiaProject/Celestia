in vec4 color;
in vec2 texCoord;

uniform sampler2D galaxyTex;

void main(void)
{
    fragColor = vec4(color.rgb, color.a * texture(galaxyTex, texCoord).r);
}
