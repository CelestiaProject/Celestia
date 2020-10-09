uniform sampler2D starTex;
uniform vec4 color;

varying vec2 texCoord;

void main(void)
{
    gl_FragColor = texture2D(starTex, texCoord) * color;
}
