in vec4 v_Color;
in vec2 v_TexCoord;

uniform sampler2D galaxyTex;

out vec4 v_FragColor;

void main()
{
    v_FragColor = vec4(v_Color.rgb, v_Color.a * texture(galaxyTex, v_TexCoord).r);
}
