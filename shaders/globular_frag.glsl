uniform sampler2D starTex;
varying vec4 color;

void main(void)
{
    gl_FragColor = vec4(color.rgb, color.a * texture2D(starTex, gl_PointCoord.xy).r);
}
