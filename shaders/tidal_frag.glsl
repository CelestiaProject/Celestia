uniform sampler2D tidalTex;

varying vec2 texCoord;
varying vec4 color;

void main(void)
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(tidalTex, texCoord).r);
}
