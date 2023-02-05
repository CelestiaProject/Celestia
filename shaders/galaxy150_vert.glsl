in vec4 in_Position;
in float in_Size;
in float in_ColorIndex;
in float in_Brightness;

uniform sampler2D colorTex;

out Vertex
{
    vec3  color;
    float size;
    float brightness;
} vertex;

void main()
{
    gl_Position = in_Position;
    vertex.size = in_Size;
    vertex.brightness = in_Brightness;
    vertex.color = texture(colorTex, vec2(in_ColorIndex, 0.0)).rgb;
}
