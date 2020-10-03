varying vec2 texCoord;

uniform sampler2D tex;

void main(void)
{
    gl_FragColor = texture2D(tex, texCoord);
}
