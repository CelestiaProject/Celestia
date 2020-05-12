uniform vec4 color;
uniform sampler2D tidalTex;
varying vec2 texCoord;

void main(void)
{
    gl_FragColor = texture2D(tidalTex, texCoord) * color;
}
